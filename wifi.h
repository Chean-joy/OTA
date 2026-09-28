#ifndef __WIFI_H__
#define __WIFI_H__

#include "usart.h"


// 假设你有一个用于存储 OTA 信息的结构体或全局变量
typedef struct {
    char target[32]; // 固件名称
    int  tid;        // 任务ID
    int  size;       // 固件大小
    char md5[33];    // md5 校验值 (32位字符串 + '\0')
} ota_info_t;


#define CPU_FREQ_MHZ  120
// NOP延时函数，单位为微秒
void nop_delay_us(uint32_t us);

uint8_t Wifi_init(void);

uint8_t Wifi_check_OTA_Version(void); 

uint8_t Wifi_connect(void);





#endif

