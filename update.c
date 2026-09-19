#include "update.h"
#include <stdio.h>
#include <string.h>
#include "tool.h"
#include "flash_drv.h"


// 实现真正的数据接收，烧录过程（阻塞实现）
#define YMODEM_PACKET_LENGTH  1024

#define FILE_NAME_LENGTH 256
#define FILE_SIZE_LENGTH 16

#define PACKET_SEQNO_INDEX (1)
#define PACKET_SEQNO_COMP_INDEX (2)

#define PACKET_HEADER (3)
#define PACKET_TRAILER (2)
#define PACKET_OVERHEAD (PACKET_HEADER + PACKET_TRAILER)
#define PACKET_SIZE (128)
#define PACKET_1K_SIZE (1024)

#define SOH (0x01)   // 128字节数据包开始
#define STX (0x02)   // 1024字节的数据包开始
#define EOT (0x04)   // 结束传输
#define ACK (0x06)   // 回应
#define NAK (0x15)   // 空回应
#define CA (0x18)    // 这两个相继中止转移
#define CREQ (0x43)  //'C' == 0x43, 请求数据

#define ABORT1 (0x41)  //'A' == 0x41, 用户终止
#define ABORT2 (0x61)  //'a' == 0x61, 用户终止

#define NAK_TIMEOUT (0x4fffff)
#define MAX_ERRORS (65565)

static uint8_t g_packetBuffer[YMODEM_PACKET_LENGTH];

// 文件名
char g_imageName[FILE_NAME_LENGTH] = "abc.bin";


/**********************************************************
 * @brief 读取文件信息
 * @param packetData IN 要解析的数据
 * @param size OUT 文件大小
 * @return
 **********************************************************/
void getFileInfo(uint8_t *packetData, int32_t *size) {
  int32_t i;
  uint8_t fileSize[FILE_SIZE_LENGTH];

  uint8_t *filePtr;
  /* 跳过3个字节，取出文件名，直到遇到0x00(\0), Filename packet has valid data
   */
  for (i = 0, filePtr = packetData + PACKET_HEADER; (*filePtr != '\0') && (i < FILE_NAME_LENGTH); ) {
    g_imageName[i++] = *filePtr++;
  }
  g_imageName[i++] = '\0';

  /* 取出数据总长度，直到遇到空字符 */
  for (i = 0, filePtr++; (*filePtr != ' ') && (i < FILE_SIZE_LENGTH);) {
    fileSize[i++] = *filePtr++;
  }
  fileSize[i++] = '\0';
  // 将数字字符串转成int32_t类型数字
  Str2Int(fileSize, size);
}

#define SendByte(byte)  USART0_send_byte(byte)
char bufArr[128]; // 打日志用

/**********************************************************
 * @brief 阻塞式接收一个包的数据, 进行CRC数据校验
 * @param data    收到的字节数组
 * @param length  收到的数据长度 128, 1024
 * @param timeout 超时时间
 * @return 错误码: 0成功, 其他失败
 **********************************************************/
int RecevicePacket(uint8_t* data, int32_t* length, uint32_t timeout){

    uint32_t packageSize = 0; // 数据包大小
    *length = 0; // 一定要把长度清零
    
    uint8_t c;
    
    // 获取消息头
    if(USART0_ReceiveByteTimeout(&c, timeout)){
        return -10; // 超时
    }
    
    // 判断消息头
    switch(c){
        case SOH:  // 128字节数据包开始
            packageSize = 128;
            break;
        case STX:  // 1024字节数据包开始
            packageSize = 1024;
            break;
        case EOT:  // 结束传输
            packageSize = 0;
            return 0;
        case CA:  // 0x18, 这两个相继中止转移, 连续收到两个0x18,说明用户按下了Ctrl + C
            if((!USART0_ReceiveByteTimeout(&c, timeout)) && c == CA){
                return -11; // 用户主动中断发送
            }else{
                return -12; // 用户主动中断发送
            }
        case ABORT1:  // 'A' == 0x41, 用户终止
        case ABORT2:  // 'a' == 0x61, 用户终止
            return -13; // 用户主动终止
        default:  // 其他情况
            return -14; // 其他情况
    }

    // 将帧头存到数据包中
    data[0] = c;
    // 循环接收N-1个数据
    uint32_t packageTotal = packageSize + PACKET_OVERHEAD;
    for(int i = 1; i < packageTotal; i++){
        if(USART0_ReceiveByteTimeout(&data[i], timeout)){
            return -15; // 超时
        }
    }

    // 接收数据包完成, 校验序列号和反序列号
    if((data[PACKET_SEQNO_INDEX] | data[PACKET_SEQNO_COMP_INDEX]) != 0xFF){
        return -16; // 序列号或反序列号错误
    }

    // 计算实际的CRC16
    uint16_t expect_crc = (data[packageTotal - 2] << 8) | data[packageTotal - 1];
    uint16_t actual_crc = Crc16Ymodem(&data[3], packageSize);
    if(expect_crc != actual_crc){
        return -17; // CRC16校验失败
    }

    // 数据包校验通过
    *length = packageSize;

    return 0;
}

// bss / data
// uint8_t packetData[1024 + 5] = {0};

/**********************************************************
 * @brief 接收并烧录新版本固件
 * @param 
 * @return 固件大小>0, 错误码<0
 **********************************************************/
int32_t YmodemReceive(void){
    // 准备数组接收每一个包数据(如果栈大小太少, 可能会导致栈溢出)
    // 1. 调大栈大小, 2. 把数字放到全局
    uint8_t packetData[YMODEM_PACKET_LENGTH + PACKET_OVERHEAD];
    // 数据包的数据Data数量
    int32_t packetLength = 0, packetsReceived = 0, fileSize = 0;
    uint8_t fileDone = 0; // 文件传输完成标志
    // 定义变量, 记录要写入的Flash的位置
    uint32_t flashDestinaiton = APP_ADDR_IN_FLASH;
    
    // 阻塞式接收一个文件的多个数据包
    while(1){
        // 接收一个数据包
        int rst = RecevicePacket(packetData, &packetLength, NAK_TIMEOUT);
        
//        sprintf(bufArr, "rst->%d\r\n", rst);
//        USART1_send_string(bufArr);
        // 如果rst不是0, 异常, 重新发起请求
        if(rst != 0){
            SendByte(CREQ);
            continue;
        }

        if(packetLength == 0){ // 结束传输EOT
            if(fileDone == 0){
                fileDone = 1;
                // 回复NAK
                SendByte(NAK);
                USART1_send_string("fileDone NAK\r\n");
            }else if(fileDone == 1){
                // 回复ACK
                SendByte(ACK);
                USART1_send_string("fileDone ACK!!!\r\n");
                // 文件传输完成
                break;
            }
            continue;
        }
        
        // 收到了数据包并通过了校验 --------------------------------------
//        USART0_send_string("success\r\n");
        
        if(packetsReceived == 0){ // 第一个包: 文件名0x00文件大小信息
            
            // 读取文件名和文件大小
            getFileInfo(packetData, &fileSize);
            
            sprintf(bufArr, "name: %s size->%d\r\n", g_imageName, fileSize);
            USART1_send_string(bufArr);
            
            
            // 准备空间: 用于后边写数据到Flash
            if(fileSize > FLASH_APP_SIZE){ // 固件太大
                // 取消传输
                SendByte(CA);
                SendByte(CA);
                return -1;
            }
            
            // 进行空间擦除
            FlashErase(flashDestinaiton, fileSize);
            
            // 回复ACK和C
            SendByte(ACK);
            SendByte(CREQ);
                
        }else { // 其他包: 数据内容
            
            // 把数据写入到Flash
            // 1. 把packetData的数据区内容拷贝到缓冲区g_packetBuffer
            memcpy(g_packetBuffer, packetData + PACKET_HEADER, packetLength);
            // 2. 把缓冲区的数据写入到Flash
            FlashWrite(flashDestinaiton, g_packetBuffer, packetLength);
            // 3. 更新要写入的位置
            flashDestinaiton += packetLength;
            
            // 回复ACK
            SendByte(ACK);
            
            sprintf(bufArr, "packetLength: size->%d\r\n", packetLength);
            USART1_send_string(bufArr);                
        }
        
        packetsReceived++;
    }
    return fileSize;
}

void UpdateApp(){
    int32_t imageSize = 0;

    // 阻塞式循环下载新版本固件
    printf("等待文件传输...(按下字母'a'中断传输abort)\r\n");

    // 通过Ymodem协议接收固件并烧录
    imageSize = YmodemReceive();
    
    // 需要等待一会儿, 否则SecureCRT不能正常显示如下日志
//    DelayNms(100);
		delay_ms(100);
    
    if(imageSize > 0) {
        printf("烧录成功[ Name: %s ,imageSize: %d Bytes]\r\n", g_imageName, imageSize);
    } else if (imageSize == -1) {
        printf("固件过大，超出可用空间，无法下载\r\n");
    } else if (imageSize == -2) {
        printf("固件校验失败\r\n");
    } else if (imageSize == -3) {
        printf("用户主动中断发送\r\n"); // Ctrl + C
    } else if (imageSize == -4) {
        printf("用户主动取消abort\r\n"); // a A
    } else {
        printf("接收文件失败code: %d \r\n", imageSize);
    }

}
