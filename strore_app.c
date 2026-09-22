#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "flash_drv.h"
#include "store_app.h"

// 给App用, 将需要升级标志位写入Flash参数0x08004000区域
bool SetUpdateVerFlag(void){
    uint16_t update_flag = NEED_UPDATE_VERSION_FLAG;
    uint16_t read_back = 0xFFFF;

    // 先擦除参数区域
    if (!FlashErase(PARAM_ADDR_IN_FLASH, sizeof(update_flag))) {
        return false;
    }
    // 写入需要升级标志位
    if (!FlashWrite16bit(PARAM_ADDR_IN_FLASH, &update_flag, 1)) {
        return false;
    }
    // 检查是否写入成功
    if (!FlashRead(PARAM_ADDR_IN_FLASH, (uint8_t *)&read_back, sizeof(read_back))) {
        return false;
    }

    return (read_back == NEED_UPDATE_VERSION_FLAG);
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
