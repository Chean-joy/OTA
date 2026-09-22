#ifndef _STORE_APP_H_
#define _STORE_APP_H_

#include <stdint.h>
#include <stdbool.h>

#define NEED_UPDATE_VERSION_FLAG          0xABCD

/* OTA相关参数  0x80000
start										  total
0x8000000                 0x80000 = (512K)

Flash存储方案
Sector	0             1	                      234567
	  Bootloader	Param					  APP_SIZE
|---------------|------------|------------------------------------------------|
	   16K    		 16K		               480K
UPDATE PARAM->0xABCD
*/
#define FLASH_SIZE                        0x80000
#define BOOTLOADER_SIZE                   (16 * 1024)       // 0x4000
#define PARAMETER_SIZE                    (16 * 1024)       // 0x4000

#define PARAM_ADDR_IN_FLASH            	  (0x08000000 + BOOTLOADER_SIZE)					// 0x8004000
#define APP_ADDR_IN_FLASH                 (0x08000000 + BOOTLOADER_SIZE + PARAMETER_SIZE)	// 0x8008000

#define NEED_UPDATE_VERSION_FLAG          0xABCD
#define VECTOR_OFFSET                     (BOOTLOADER_SIZE + PARAMETER_SIZE) // 0x8000

// 给App用
bool SetUpdateVerFlag(void);   // 设置升级标记

// 给Bootloader用
bool CheckNeedUpdate(void);    // 检查升级标记
void ClearUpdateVerFlag(void); // 清理升级标记

void print_arr(uint8_t * data, uint32_t len);
#endif
