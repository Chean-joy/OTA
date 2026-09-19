#ifndef __UPDATE_H__
#define __UPDATE_H__

#include "gd32f30x.h"
#include "usart.h"
// Flash�ܴ�С
#define FLASH_SIZE          0x80000     // 512K

// App�Ŀ�ʼλ��
#define APP_ADDR_IN_FLASH   0x08004000

// App�Ŀ��ÿռ�
#define FLASH_APP_SIZE      (FLASH_SIZE - (APP_ADDR_IN_FLASH - FLASH_BASE))

void UpdateApp(void);


#endif
