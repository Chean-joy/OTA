#include "update.h"
#include <stdio.h>
#include <string.h>
#include "tool.h"
#include "flash_drv.h"


// ʵ�����������ݽ��գ���¼���̣�����ʵ�֣�
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

#define SOH (0x01)   // 128�ֽ����ݰ���ʼ
#define STX (0x02)   // 1024�ֽڵ����ݰ���ʼ
#define EOT (0x04)   // ��������
#define ACK (0x06)   // ��Ӧ
#define NAK (0x15)   // �ջ�Ӧ
#define CA (0x18)    // �����������ֹ�?��
#define CREQ (0x43)  //'C' == 0x43, ��������

#define ABORT1 (0x41)  //'A' == 0x41, �û���ֹ
#define ABORT2 (0x61)  //'a' == 0x61, �û���ֹ

#define NAK_TIMEOUT (0x1fffff)
#define MAX_ERRORS (65565)

static uint8_t g_packetBuffer[YMODEM_PACKET_LENGTH];

// �ļ���
char g_imageName[FILE_NAME_LENGTH] = "abc.bin";


/**********************************************************
 * @brief ��ȡ�ļ���Ϣ
 * @param packetData IN Ҫ����������
 * @param size OUT �ļ���С
 * @return
 **********************************************************/
void getFileInfo(uint8_t *packetData, int32_t *size) {
  int32_t i;
  uint8_t fileSize[FILE_SIZE_LENGTH];

  uint8_t *filePtr;
  /* ����3���ֽڣ�ȡ���ļ�����ֱ������0x00(\0), Filename packet has valid data
   */
  for (i = 0, filePtr = packetData + PACKET_HEADER; (*filePtr != '\0') && (i < FILE_NAME_LENGTH); ) {
    g_imageName[i++] = *filePtr++;
  }
  g_imageName[i++] = '\0';

  /* ȡ�������ܳ��ȣ�ֱ���������ַ� */
  for (i = 0, filePtr++; (*filePtr != ' ') && (i < FILE_SIZE_LENGTH);) {
    fileSize[i++] = *filePtr++;
  }
  fileSize[i++] = '\0';
  // �������ַ���ת��int32_t��������
  Str2Int(fileSize, size);
}

#define SendByte(byte)  USART0_send_byte(byte)
char bufArr[128]; // ����־��

/**********************************************************
 * @brief ����ʽ����һ����������, ����CRC����У��
 * @param data    �յ����ֽ�����
 * @param length  �յ������ݳ��� 128, 1024
 * @param timeout ��ʱʱ��
 * @return ������: 0�ɹ�, ����ʧ��
 **********************************************************/
int RecevicePacket(uint8_t* data, int32_t* length, uint32_t timeout){

    uint32_t packageSize = 0; // ���ݰ���С
    *length = 0; // һ��Ҫ�ѳ�������
    
    uint8_t c = 0;
    
    // ��ȡ��Ϣͷ
    if(USART0_ReceiveByteTimeout(&c, timeout)){
        return -10; // ��ʱ
    }
    
    // �ж���Ϣͷ
    switch(c){
        case SOH:  // 128�ֽ����ݰ���ʼ
            packageSize = 128;
            break;
        case STX:  // 1024�ֽ����ݰ���ʼ
            packageSize = 1024;
            break;
        case EOT:  // ��������
            packageSize = 0;
            return 0;
        case CA:  // 0x18, �����������ֹ�?��, �����յ�����0x18,˵���û�������Ctrl + C
            if((!USART0_ReceiveByteTimeout(&c, timeout)) && c == CA){
                return -11; // �û������жϷ���
            }else{
                return -12; // �û������жϷ���
            }
        case ABORT1:  // 'A' == 0x41, �û���ֹ
        case ABORT2:  // 'a' == 0x61, �û���ֹ
            return -13; // �û�������ֹ
        default:  // �������?
            return -14; // �������?
    }

    // ��֡ͷ�浽���ݰ���
    data[0] = c;
    // ѭ������N-1������
    uint32_t packageTotal = packageSize + PACKET_OVERHEAD;
    for(int i = 1; i < packageTotal; i++){
        if(USART0_ReceiveByteTimeout(&data[i], timeout)){
            return -15; // ��ʱ
        }
    }

    // �������ݰ����?, У�����кźͷ����к�
    if((data[PACKET_SEQNO_INDEX] | data[PACKET_SEQNO_COMP_INDEX]) != 0xFF){
        return -16; // ���кŻ����кŴ���
    }

    // ����ʵ�ʵ�CRC16
    uint16_t expect_crc = (data[packageTotal - 2] << 8) | data[packageTotal - 1];
    uint16_t actual_crc = Crc16Ymodem(&data[3], packageSize);
    if(expect_crc != actual_crc){
        return -17; // CRC16У��ʧ��
    }

    // ���ݰ�У��ͨ��
    *length = packageSize;

    return 0;
}

// bss / data
// uint8_t packetData[1024 + 5] = {0};

/**********************************************************
 * @brief ���ղ���¼�°汾�̼�
 * @param 
 * @return �̼���С>0, ������<0
 **********************************************************/
int32_t YmodemReceive(void){
    // ׼���������ÿһ��������?(���ջ��С�?��, ���ܻᵼ��ջ���?)
    // 1. ����ջ��С, 2. �����ַŵ�ȫ��
    uint8_t packetData[YMODEM_PACKET_LENGTH + PACKET_OVERHEAD];
    // ���ݰ�������Data����
    int32_t packetLength = 0, packetsReceived = 0, fileSize = 0;
    uint8_t fileDone = 0; // �ļ�������ɱ��?
    // �������?, ��¼Ҫд���Flash��λ��
    uint32_t flashDestinaiton = APP_ADDR_IN_FLASH;
    
    #ifdef MULTIPLE_FILE_TRANSFILE
    uint8_t sessonDone = 0; // �Ự��ɱ��?
    while(1)
    {
        flashDestinaiton = APP_ADDR_IN_FLASH;
        packetsReceived = 0;
        fileDone = 0;
    #endif

    // ����ʽ����һ���ļ��Ķ�����ݰ�?
    while(1){
        // ����һ�����ݰ�
        int rst = RecevicePacket(packetData, &packetLength, NAK_TIMEOUT);
        
        #if MULTIPLE_FILE_TRANSFILE
        if(rst == -13){ // 'A' �� 'a', �û���ֹ����
            SendByte(CA);
            SendByte(CA);
            return -4;
        }else if(rst == -11 || rst == -12){ // ��������CA, ����ֹ����, ˵���û�������Ctrl+C
            SendByte(ACK);
            return -3;
        }
        #endif

        if(rst != 0){
            SendByte(packetsReceived == 0 ? CREQ : NAK);
            continue;
        }

        if(packetLength == 0){ // ��������EOT
            if(fileDone == 0){
                fileDone = 1;
                // �ظ�NAK
                SendByte(NAK);
                USART1_send_string("fileDone NAK\r\n");
            }else if(fileDone == 1){
                
        #if MULTIPLE_FILE_TRANSFILE

                 fileDone = 2;
        #endif
               // �ظ�ACK
                SendByte(ACK);
                USART1_send_string("fileDone ACK!!!\r\n");
                // �ļ��������?
                break;
            }
            continue;
        }
        
        // �յ������ݰ���ͨ����У�� --------------------------------------
//        USART0_send_string("success\r\n");
        
        if(packetsReceived == 0){ // ��һ����: �ļ���0x00�ļ���С��Ϣ
            #ifdef MULTIPLE_FILE_TRANSFILE
            // �ж��ļ������ֽ�, �Ƿ���0x00                
            if(packetData[PACKET_HEADER] == 0x00){
                // �����?0x00��Ự����?
                SendByte(ACK);
                
                // �����Ự������!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                sessonDone = 1;
                break;
            }
            #endif


            // ��ȡ�ļ������ļ���С
            getFileInfo(packetData, &fileSize);
            
            sprintf(bufArr, "name: %s size->%d\r\n", g_imageName, fileSize);
            USART1_send_string(bufArr);
            
            
            // ׼���ռ�: ���ں��д���ݵ�Flash
            if(fileSize > FLASH_APP_SIZE){ // �̼�̫��
                // ȡ������
                SendByte(CA);
                SendByte(CA);
                return -1;
            }
            
            // ���пռ����?
            FlashErase(flashDestinaiton, fileSize);
            
            // �ظ�ACK��C
            SendByte(ACK);
            SendByte(CREQ);
                
        }else { // ������: ��������
            
            // ������д�뵽Flash
            // 1. ��packetData�����������ݿ�����������g_packetBuffer
            memcpy(g_packetBuffer, packetData + PACKET_HEADER, packetLength);
            // 2. �ѻ�����������д�뵽Flash
            FlashWrite(flashDestinaiton, g_packetBuffer, packetLength);
            // 3. ����Ҫд���λ��?
            flashDestinaiton += packetLength;
            
            // �ظ�ACK
            SendByte(ACK);
            
            sprintf(bufArr, "packetLength: size->%d\r\n", packetLength);
            USART1_send_string(bufArr);                
        }   
        packetsReceived++;
    }
    #ifdef  MULTIPLE_FILE_TRANSFILE
        if(sessonDone >0){
            break;
        }
    }
    #endif
    return fileSize;
}
void UpdateApp(){
    int32_t imageSize = 0;

   // 阻塞式循环下载新版本固件
    printf("等待文件传输...(按下字母'a'中断传输abort)\n");

    // 通过Ymodem协议接收固件并烧录
    imageSize = YmodemReceive();
    
    // ��Ҫ�ȴ�һ���, ����SecureCRT����������ʾ������־
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

