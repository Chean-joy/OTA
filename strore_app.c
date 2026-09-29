#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "flash_drv.h"
#include "store_app.h"

//引入硬件WIFI层
#include "wifi.h"

#define OTA_MODE 1 

#if OTA_MODE
uint8_t OTA_FILE_Inf[128]={0};
uint8_t OTA_FILE_CHEACK_inf[128]={0};
#endif

/***
//typedef struct {
//    char target[32]; // 固件名称
//    int  tid;        // 任务ID
//    int  size;       // 固件大小
//    char md5[33];    // md5 校验值 (32位字符串 + '\0')
//} ota_info_t;
*/


// 给App用, 将需要升级标志位写入Flash参数0x08004000区域
bool SetUpdateVerFlag(void){
    uint16_t update_flag = NEED_UPDATE_VERSION_FLAG;
    

    // 先擦除参数区域
    if (!FlashErase(PARAM_ADDR_IN_FLASH, sizeof(update_flag))) {
        return false;
    }
		
		#if OTA_MODE
		sprintf((char*)OTA_FILE_Inf,"terget:%s,tid:%d,size:%d",
			current_ota_info.target,current_ota_info.tid,current_ota_info.size);
		
		FlashWrite8bit(PARAM_ADDR_IN_FLASH,OTA_FILE_Inf,strlen((char*)OTA_FILE_Inf));
		
		//检查是否写入成功
		if (!FlashRead(PARAM_ADDR_IN_FLASH, OTA_FILE_CHEACK_inf,strlen((char*)OTA_FILE_Inf))) {
        return false;
    }
		uint8_t res = strcmp((char*)OTA_FILE_Inf,(char*)OTA_FILE_CHEACK_inf);
		
		memset(OTA_FILE_Inf,0,sizeof OTA_FILE_CHEACK_inf);
		memset(OTA_FILE_CHEACK_inf,0,sizeof OTA_FILE_CHEACK_inf);
		if(!res)
		{
				return true;
		}
		return false;
		
		#else
		uint16_t read_back = 0xFFFF;
		
    // 写入需要升级标志位
    if (!FlashWrite16bit(PARAM_ADDR_IN_FLASH, &update_flag, 1)) {
        return false;
    }
    // 检查是否写入成功
    if (!FlashRead(PARAM_ADDR_IN_FLASH, (uint8_t *)&read_back, sizeof(read_back))) {
        return false;
    }

		return (read_back == NEED_UPDATE_VERSION_FLAG);
		#endif
}		

// 给Bootloader用
bool CheckNeedUpdate(void){
    uint16_t update_flag = 0xFFFF;

    // 读取需要升级标志位
    if (!FlashRead(PARAM_ADDR_IN_FLASH, (uint8_t *)&update_flag, sizeof(update_flag))) {
        return false;
    }

    return (update_flag == NEED_UPDATE_VERSION_FLAG);
}
void ClearUpdateVerFlag(void){
    // 擦除参数区域
    (void)FlashErase(PARAM_ADDR_IN_FLASH, sizeof(uint16_t));
}
