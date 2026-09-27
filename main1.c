/************************************************
 GenBotter Mini GD32开发板
 Template工程模板-新建工程使用
************************************************/

#include "sys.h"
#include "usart.h"		
#include "delay.h"	
#include "flash_drv.h"
#include "update.h"
#include <stdio.h>
#include "iap_driver.h"
#include "store_app.h"

#include "oled.h"

void RESET_START_UP_FUNC(uint8_t *C)
{
	uint8_t ch = *C;
	
	printf("data:%c \r\n",ch);
	
	if(ch == 'U' || ch == 'u'){  
        // 添加升级标记
        SetUpdateVerFlag();
        // 停机, 重启进入升级流程
        iap_reset_to_boot();
    }
	
}


int main(void)
{
		//设置中断向量表起始地址偏移量
//		iap_init_irq();
	
	 	delay_init(120);                     //初始化延时函数 
	  	
		usart_init(115200);											//初始化串口
	
		usart1_init(115200);

		
	
		printf("Hello World\r\n");
	
		printf("Test......\r\n");
	
		OLED_Init();
	
		OLED_Clear(1);
	
		OLED_ShowString(0,0,"Version:1.0",12,1);
	
		OLED_Refresh();
//	

//	
   while(1)
	{
			printf("v1.0\r\n");
			delay_ms(1000);
		
			if(Rx_flag)
			{
				Rx_flag = 0x00;
				RESET_START_UP_FUNC(&Rxdata);
			}
	}
}


