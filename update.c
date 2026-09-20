#include "update.h"
#include <stdio.h>
#include <string.h>
#include "tool.h"
#include "flash_drv.h"


// 实锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟捷斤拷锟秸ｏ拷锟斤拷录锟斤拷锟教ｏ拷锟斤拷锟斤拷实锟街ｏ拷
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

#define SOH (0x01)   // 128锟街斤拷锟斤拷锟捷帮拷锟斤拷始
#define STX (0x02)   // 1024锟街节碉拷锟斤拷锟捷帮拷锟斤拷始
#define EOT (0x04)   // 锟斤拷锟斤拷锟斤拷锟斤拷
#define ACK (0x06)   // 锟斤拷应
#define NAK (0x15)   // 锟秸伙拷应
#define CA (0x18)    // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟街棺?锟斤拷
#define CREQ (0x43)  //'C' == 0x43, 锟斤拷锟斤拷锟斤拷锟斤拷

#define ABORT1 (0x41)  //'A' == 0x41, 锟矫伙拷锟斤拷止
#define ABORT2 (0x61)  //'a' == 0x61, 锟矫伙拷锟斤拷止

#define NAK_TIMEOUT (0xffffff)
#define MAX_ERRORS (65565)

static uint8_t g_packetBuffer[YMODEM_PACKET_LENGTH];

// 锟侥硷拷锟斤拷
char g_imageName[FILE_NAME_LENGTH] = "abc.bin";


/**********************************************************
 * @brief 锟斤拷取锟侥硷拷锟斤拷息
 * @param packetData IN 要锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
 * @param size OUT 锟侥硷拷锟斤拷小
 * @return
 **********************************************************/
void getFileInfo(uint8_t *packetData, int32_t *size) {
  int32_t i;
  uint8_t fileSize[FILE_SIZE_LENGTH];

  uint8_t *filePtr;
  /* 锟斤拷锟斤拷3锟斤拷锟街节ｏ拷取锟斤拷锟侥硷拷锟斤拷锟斤拷直锟斤拷锟斤拷锟斤拷0x00(\0), Filename packet has valid data
   */
  for (i = 0, filePtr = packetData + PACKET_HEADER; (*filePtr != '\0') && (i < FILE_NAME_LENGTH); ) {
    g_imageName[i++] = *filePtr++;
  }
  g_imageName[i++] = '\0';

  /* 取锟斤拷锟斤拷锟斤拷锟杰筹拷锟饺ｏ拷直锟斤拷锟斤拷锟斤拷锟斤拷锟街凤拷 */
  for (i = 0, filePtr++; (*filePtr != ' ') && (i < FILE_SIZE_LENGTH);) {
    fileSize[i++] = *filePtr++;
  }
  fileSize[i++] = '\0';
  // 锟斤拷锟斤拷锟斤拷锟街凤拷锟斤拷转锟斤拷int32_t锟斤拷锟斤拷锟斤拷锟斤拷
  Str2Int(fileSize, size);
}

#define SendByte(byte)  USART0_send_byte(byte)
char bufArr[128]; // 锟斤拷锟斤拷志锟斤拷

/**********************************************************
 * @brief 锟斤拷锟斤拷式锟斤拷锟斤拷一锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷, 锟斤拷锟斤拷CRC锟斤拷锟斤拷校锟斤拷
 * @param data    锟秸碉拷锟斤拷锟街斤拷锟斤拷锟斤拷
 * @param length  锟秸碉拷锟斤拷锟斤拷锟捷筹拷锟斤拷 128, 1024
 * @param timeout 锟斤拷时时锟斤拷
 * @return 锟斤拷锟斤拷锟斤拷: 0锟缴癸拷, 锟斤拷锟斤拷失锟斤拷
 **********************************************************/
int RecevicePacket(uint8_t* data, int32_t* length, uint32_t timeout){

    uint32_t packageSize = 0; // 锟斤拷锟捷帮拷锟斤拷小
    *length = 0; // 一锟斤拷要锟窖筹拷锟斤拷锟斤拷锟斤拷
    
    uint8_t c = 0;
    
    // 锟斤拷取锟斤拷息头
    if(USART0_ReceiveByteTimeout(&c, timeout)){
        return -10; // 锟斤拷时
    }
    
    // 锟叫讹拷锟斤拷息头
    switch(c){
        case SOH:  // 128锟街斤拷锟斤拷锟捷帮拷锟斤拷始
            packageSize = 128;
            break;
        case STX:  // 1024锟街斤拷锟斤拷锟捷帮拷锟斤拷始
            packageSize = 1024;
            break;
        case EOT:  // 锟斤拷锟斤拷锟斤拷锟斤拷
            packageSize = 0;
            return 0;
        case CA:  // 0x18, 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟街棺?锟斤拷, 锟斤拷锟斤拷锟秸碉拷锟斤拷锟斤拷0x18,说锟斤拷锟矫伙拷锟斤拷锟斤拷锟斤拷Ctrl + C
            if((!USART0_ReceiveByteTimeout(&c, timeout)) && c == CA){
                return -11; // 锟矫伙拷锟斤拷锟斤拷锟叫断凤拷锟斤拷
            }else{
                return -12; // 锟矫伙拷锟斤拷锟斤拷锟叫断凤拷锟斤拷
            }
        case ABORT1:  // 'A' == 0x41, 锟矫伙拷锟斤拷止
        case ABORT2:  // 'a' == 0x61, 锟矫伙拷锟斤拷止
            return -13; // 锟矫伙拷锟斤拷锟斤拷锟斤拷止
        default:  // 锟斤拷锟斤拷锟斤拷锟?
            return -14; // 锟斤拷锟斤拷锟斤拷锟?
    }

    // 锟斤拷帧头锟芥到锟斤拷锟捷帮拷锟斤拷
    data[0] = c;
    // 循锟斤拷锟斤拷锟斤拷N-1锟斤拷锟斤拷锟斤拷
    uint32_t packageTotal = packageSize + PACKET_OVERHEAD;
    for(int i = 1; i < packageTotal; i++){
        if(USART0_ReceiveByteTimeout(&data[i], timeout)){
            return -15; // 锟斤拷时
        }
    }

    // 锟斤拷锟斤拷锟斤拷锟捷帮拷锟斤拷锟?, 校锟斤拷锟斤拷锟叫号和凤拷锟斤拷锟叫猴拷
    if((data[PACKET_SEQNO_INDEX] | data[PACKET_SEQNO_COMP_INDEX]) != 0xFF){
        return -16; // 锟斤拷锟叫号伙拷锟斤拷锟叫号达拷锟斤拷
    }

    // 锟斤拷锟斤拷实锟绞碉拷CRC16
    uint16_t expect_crc = (data[packageTotal - 2] << 8) | data[packageTotal - 1];
    uint16_t actual_crc = Crc16Ymodem(&data[3], packageSize);
    if(expect_crc != actual_crc){
        return -17; // CRC16校锟斤拷失锟斤拷
    }

    // 锟斤拷锟捷帮拷校锟斤拷通锟斤拷
    *length = packageSize;

    return 0;
}

// bss / data
// uint8_t packetData[1024 + 5] = {0};

/**********************************************************
 * @brief 锟斤拷锟秸诧拷锟斤拷录锟铰版本锟教硷拷
 * @param 
 * @return 锟教硷拷锟斤拷小>0, 锟斤拷锟斤拷锟斤拷<0
 **********************************************************/
int32_t YmodemReceive(void){
//     // 准锟斤拷锟斤拷锟斤拷锟斤拷锟矫恳伙拷锟斤拷锟斤拷锟斤拷锟?(锟斤拷锟秸伙拷锟叫√?锟斤拷, 锟斤拷锟杰会导锟斤拷栈锟斤拷锟?)
//     // 1. 锟斤拷锟斤拷栈锟斤拷小, 2. 锟斤拷锟斤拷锟街放碉拷全锟斤拷
//     uint8_t packetData[YMODEM_PACKET_LENGTH + PACKET_OVERHEAD];
//     // 锟斤拷锟捷帮拷锟斤拷锟斤拷锟斤拷Data锟斤拷锟斤拷
//     int32_t packetLength = 0, packetsReceived = 0, fileSize = 0;
//     uint8_t fileDone = 0; // 锟侥硷拷锟斤拷锟斤拷锟斤拷杀锟街?
//     // 锟斤拷锟斤拷锟斤拷锟?, 锟斤拷录要写锟斤拷锟紽lash锟斤拷位锟斤拷
//     uint32_t flashDestinaiton = APP_ADDR_IN_FLASH;
    
//     #ifdef MULTIPLE_FILE_TRANSFILE
//     uint8_t sessonDone = 0; // 锟结话锟斤拷杀锟街?
//     while(1)
//     {
//         flashDestinaiton = APP_ADDR_IN_FLASH;
//         packetsReceived = 0;
//         fileDone = 0;
//     #endif

//     // 锟斤拷锟斤拷式锟斤拷锟斤拷一锟斤拷锟侥硷拷锟侥讹拷锟斤拷锟斤拷莅锟?
//     while(1){
//         // 锟斤拷锟斤拷一锟斤拷锟斤拷锟捷帮拷
//         int rst = RecevicePacket(packetData, &packetLength, NAK_TIMEOUT);
        
//         #if MULTIPLE_FILE_TRANSFILE
//         if(rst == -13){ // 'A' 锟斤拷 'a', 锟矫伙拷锟斤拷止锟斤拷锟斤拷
//             SendByte(CA);
//             SendByte(CA);
//             return -4;
//         }else if(rst == -11 || rst == -12){ // 锟斤拷锟斤拷锟斤拷锟斤拷CA, 锟斤拷锟斤拷止锟斤拷锟斤拷, 说锟斤拷锟矫伙拷锟斤拷锟斤拷锟斤拷Ctrl+C
//             SendByte(ACK);
//             return -3;
//         }
//         #endif

//         if(rst != 0){
//             SendByte(CREQ);
//             continue;
//         }

//         if(packetLength == 0){ // 锟斤拷锟斤拷锟斤拷锟斤拷EOT
//             if(fileDone == 0){
//                 fileDone = 1;
//                 // 锟截革拷NAK
//                 SendByte(NAK);
//                 USART1_send_string("fileDone NAK\r\n");
//             }else if(fileDone == 1){
//                 
//         #if MULTIPLE_FILE_TRANSFILE
//
//                  fileDone = 2;
//                 // endif
//                // 锟截革拷ACK
//                 SendByte(ACK);
//                 USART1_send_string("fileDone ACK!!!\r\n");
//                 // 锟侥硷拷锟斤拷锟斤拷锟斤拷锟?
//                 break;
//             }
//             continue;
//         }
        
//         // 锟秸碉拷锟斤拷锟斤拷锟捷帮拷锟斤拷通锟斤拷锟斤拷校锟斤拷 --------------------------------------
// //        USART0_send_string("success\r\n");
        
//         if(packetsReceived == 0){ // 锟斤拷一锟斤拷锟斤拷: 锟侥硷拷锟斤拷0x00锟侥硷拷锟斤拷小锟斤拷息
//             #ifdef MULTIPLE_FILE_TRANSFILE
//             // 锟叫讹拷锟侥硷拷锟斤拷锟斤拷锟街斤拷, 锟角凤拷锟斤拷0x00                
//             if(packetData[PACKET_HEADER] == 0x00){
//                 // 锟斤拷锟斤拷锟?0x00锟斤拷峄帮拷锟斤拷锟?
//                 SendByte(ACK);
                
//                 // 锟斤拷锟斤拷锟结话锟斤拷锟斤拷锟斤拷!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//                 sessonDone = 1;
//                 break;
//             }
//             #endif


//             // 锟斤拷取锟侥硷拷锟斤拷锟斤拷锟侥硷拷锟斤拷小
//             getFileInfo(packetData, &fileSize);
            
//             sprintf(bufArr, "name: %s size->%d\r\n", g_imageName, fileSize);
//             USART1_send_string(bufArr);
            
            
//             // 准锟斤拷锟秸硷拷: 锟斤拷锟节猴拷锟叫达拷锟斤拷莸锟紽lash
//             if(fileSize > FLASH_APP_SIZE){ // 锟教硷拷太锟斤拷
//                 // 取锟斤拷锟斤拷锟斤拷
//                 SendByte(CA);
//                 SendByte(CA);
//                 return -1;
//             }
            
//             // 锟斤拷锟叫空硷拷锟斤拷锟?
//             FlashErase(flashDestinaiton, fileSize);
            
//             // 锟截革拷ACK锟斤拷C
//             SendByte(ACK);
//             SendByte(CREQ);
                
//         }else { // 锟斤拷锟斤拷锟斤拷: 锟斤拷锟斤拷锟斤拷锟斤拷
            
//             // 锟斤拷锟斤拷锟斤拷写锟诫到Flash
//             // 1. 锟斤拷packetData锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟捷匡拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷g_packetBuffer
//             memcpy(g_packetBuffer, packetData + PACKET_HEADER, packetLength);
//             // 2. 锟窖伙拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷写锟诫到Flash
//             FlashWrite(flashDestinaiton, g_packetBuffer, packetLength);
//             // 3. 锟斤拷锟斤拷要写锟斤拷锟轿伙拷锟?
//             flashDestinaiton += packetLength;
            
//             // 锟截革拷ACK
//             SendByte(ACK);
            
//             sprintf(bufArr, "packetLength: size->%d\r\n", packetLength);
//             USART1_send_string(bufArr);                
//         }   
//         packetsReceived++;
//     }
//     #ifdef  MULTIPLE_FILE_TRANSFILE
//         if(sessonDone >0){
//             break;
//         }
//     }
//     #endif
//     return fileSize;


    // 准锟斤拷锟斤拷锟斤拷锟斤拷锟矫恳伙拷锟斤拷锟斤拷锟斤拷锟?(锟斤拷锟秸伙拷锟叫√?锟斤拷, 锟斤拷锟杰会导锟斤拷栈锟斤拷锟?)
    // 1. 锟斤拷锟斤拷栈锟斤拷小, 2. 锟斤拷锟斤拷锟街放碉拷全锟斤拷
    uint8_t packetData[YMODEM_PACKET_LENGTH + PACKET_OVERHEAD];
    // 锟斤拷锟捷帮拷锟斤拷锟斤拷锟斤拷Data锟斤拷锟斤拷
    int32_t packetLength = 0, packetsReceived = 0, fileSize = 0;
    uint8_t fileDone = 0, sessonDone = 0; // 锟侥硷拷锟斤拷锟斤拷锟斤拷杀锟街?
    // 锟斤拷锟斤拷锟斤拷锟?, 锟斤拷录要写锟斤拷锟紽lash锟斤拷位锟斤拷
    uint32_t flashDestinaiton = APP_ADDR_IN_FLASH;
    
    // 锟斤拷锟斤拷式锟斤拷锟斤拷一锟斤拷锟结话锟侥讹拷锟斤拷募锟?
    while(1){
        // 锟斤拷锟斤拷目锟疥开始位锟斤拷
        flashDestinaiton = APP_ADDR_IN_FLASH;
        
        // 锟斤拷始锟斤拷一锟斤拷锟侥硷拷锟斤拷锟斤拷锟斤拷锟截诧拷锟斤拷
        packetsReceived = 0, fileDone = 0;
        
        // 锟斤拷锟斤拷式锟斤拷锟斤拷一锟斤拷锟侥硷拷锟侥讹拷锟斤拷锟斤拷莅锟?
        while(1){
            // 锟斤拷锟斤拷一锟斤拷锟斤拷锟捷帮拷
            int rst = RecevicePacket(packetData, &packetLength, NAK_TIMEOUT);
            
//            sprintf(bufArr, "rst->%d\r\n", rst);
//            USART1_send_string(bufArr);
            
            // 锟斤拷锟斤拷锟届常锟斤拷锟斤拷锟斤拷锟? ------------------------------------------------
            if(rst == -13){ // 'A' 锟斤拷 'a', 锟矫伙拷锟斤拷止锟斤拷锟斤拷
                SendByte(CA);
                SendByte(CA);
                return -4;
            }else if(rst == -11 || rst == -12){ // 锟斤拷锟斤拷锟斤拷锟斤拷CA, 锟斤拷锟斤拷止锟斤拷锟斤拷, 说锟斤拷锟矫伙拷锟斤拷锟斤拷锟斤拷Ctrl+C
                SendByte(ACK);
                return -3;
            }
            
            // 锟斤拷锟絩st锟斤拷锟斤拷0, 锟届常, 锟斤拷锟铰凤拷锟斤拷锟斤拷锟斤拷
            if(rst != 0){
                // 'C' when waiting for the filename packet, NAK when mid-transfer
                SendByte(packetsReceived == 0 ? CREQ : NAK);
                continue;
            }
            
            if(packetLength == 0){ // 锟斤拷锟斤拷锟斤拷锟斤拷
                if(fileDone == 0){
                    fileDone = 1;
                    // 锟截革拷NAK
                    SendByte(NAK);
                    USART1_send_string("fileDone NAK\r\n");
                }else if(fileDone == 1){
                    fileDone = 2;
                    // 锟截革拷ACK
                    SendByte(ACK);
                    USART1_send_string("fileDone ACK!!!\r\n");
                    // 锟侥硷拷锟斤拷锟斤拷锟斤拷锟? !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                    break;
                }
                continue;
            }
            
            // 锟秸碉拷锟斤拷锟斤拷锟捷帮拷锟斤拷通锟斤拷锟斤拷校锟斤拷 -------------------------------------
    //        USART0_send_string("success\r\n");
            
            if(packetsReceived == 0){ // 锟斤拷一锟斤拷锟斤拷: 锟侥硷拷锟斤拷0x00锟侥硷拷锟斤拷小锟斤拷息
                
                // 锟叫讹拷锟侥硷拷锟斤拷锟斤拷锟街斤拷, 锟角凤拷锟斤拷0x00                
                if(packetData[PACKET_HEADER] == 0x00){
                    // 锟斤拷锟斤拷锟?0x00锟斤拷峄帮拷锟斤拷锟?
                    SendByte(ACK);
                    
                    // 锟斤拷锟斤拷锟结话锟斤拷锟斤拷锟斤拷!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                    sessonDone = 1;
                    break;
                }
                
                
                // 锟斤拷取锟侥硷拷锟斤拷锟斤拷锟侥硷拷锟斤拷小
                getFileInfo(packetData, &fileSize);
                
                sprintf(bufArr, "name: %s size->%d\r\n", g_imageName, fileSize);
                USART1_send_string(bufArr);
                
                
                // 准锟斤拷锟秸硷拷: 锟斤拷锟节猴拷锟叫达拷锟斤拷莸锟紽lash
                if(fileSize > FLASH_APP_SIZE){ // 锟教硷拷太锟斤拷
                    // 取锟斤拷锟斤拷锟斤拷
                    SendByte(CA);
                    SendByte(CA);
                    return -1;
                }
                
                // 锟斤拷锟叫空硷拷锟斤拷锟?
                FlashErase(flashDestinaiton, fileSize);
                
                // 锟截革拷ACK锟斤拷C
                SendByte(ACK);
                SendByte(CREQ);
                    
            }else { // 锟斤拷锟斤拷锟斤拷: 锟斤拷锟斤拷锟斤拷锟斤拷
                
                // 锟斤拷锟斤拷锟斤拷写锟诫到Flash
                // 1. 锟斤拷packetData锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟捷匡拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷g_packetBuffer
                memcpy(g_packetBuffer, packetData + PACKET_HEADER, packetLength);
                // 2. 锟窖伙拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷写锟诫到Flash
                FlashWrite(flashDestinaiton, g_packetBuffer, packetLength);
                // 3. 锟斤拷锟斤拷要写锟斤拷锟轿伙拷锟?
                flashDestinaiton += packetLength;
                
                // 锟截革拷ACK
                SendByte(ACK);
                
                sprintf(bufArr, "packetLength: size->%d\r\n", packetLength);
                USART1_send_string(bufArr);                
            }
            
            packetsReceived++;
        }
    
        if(sessonDone > 0){
            break;
        }
        
    }
    
    return fileSize;
}
void UpdateApp(){
    int32_t imageSize = 0;

    // 阻塞式循环下载新版本固件
    printf("等待文件传输...(按下字母'a'中断传输abort)\n");

    // 通过Ymodem协议接收固件并烧录
    imageSize = YmodemReceive();
    
    // 需要等待一会儿, 否则SecureCRT不能正常显示如下日志
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

