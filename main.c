/************************************************
 GenBotter Mini GD32������
 Template����ģ��-�½�����ʹ��
************************************************/

#include "sys.h"
#include "usart.h"		
#include "delay.h"	
#include "flash_drv.h"
#include "update.h"
#include <stdio.h>
#include "store_app.h"

#include "oled.h"
#include "key01.h"
#include "wifi.h"
#define RAM_START_ADDR 0x20000000
#define RAM_SIZE 0x020000


#define IAP_BOOTLOADER_MODE1_DEFT 0
#define IAP_BOOTLOADER_MODE2 0
#define OTA_BOOTLOADER_MODE 1

#define BOOT_DELAY_COUNT 16

// 定义一个名字FuncPtr函数指针类型
typedef void (*FuncPtr)(void);



void BoottoApp(void)
{
	//获取0x800C000地址里的栈顶地址 (例如:0x20000000) 0x20000000~0x20000000+RAM_SIZE
	uint32_t stackTopAddr = *((__IO uint32_t *)APP_ADDR_IN_FLASH);
	//确定栈顶地址在合法范围内
	printf("stackTopAddr:%08x\r\n",stackTopAddr);
	if((stackTopAddr >= RAM_START_ADDR) && (stackTopAddr < (RAM_START_ADDR + RAM_SIZE)))
	{
			//关闭所有中断
		__disable_irq();
		//设置新的栈顶地址 到SP寄存器(MAP)
		__set_MSP(stackTopAddr);
		//取出0x0800C004地址的重置函数地址 Reset_Handler
		uint32_t resetHandlerAddr = *((__IO uint32_t *)(APP_ADDR_IN_FLASH + 4));
        printf("resetHandlerAddr: 0x%08X\n", resetHandlerAddr);
		//把函数地址转成 指向函数的指针,用来跳转APP
		FuncPtr jumpToApp = (FuncPtr)resetHandlerAddr;
		jumpToApp(); // 跳转
	}
	else{
//		 printf("栈顶地址不在合法范围, 无法启动\r\n");
		printf("ERROR\n");
	}

}

void main_menu_cmd(){
	
	 printf("按下任意键, 停止自启动:   ");

		uint16_t tick_count = 0;
	
		uint8_t ch = 0;
	
    while(tick_count < BOOT_DELAY_COUNT)
		{
			tick_count += 1;
			
			for(uint16_t i = 0;i < 1000; i++)
			{
				if (usart_flag_get(USART0, USART_FLAG_RBNE) != RESET)
        {
						ch = (uint8_t)usart_data_receive(USART0);
					printf("ch: %c\r\n",ch);
						goto a ;
				}
				delay_ms(1);
			}
			
			printf("\b\b%d",BOOT_DELAY_COUNT - tick_count);
		}
		
		printf("\r\n启动到App\r\n");
		delay_ms(100);
    BoottoApp();

		a:
			printf("停止自启动\r\n");
		
		uint8_t serialKey = 0x00;
		
		while(1){
            // 用户提前结束倒计时显示菜单
        printf("\r\n\n======================= Main Menu ============================\r\n\n");
        printf("************[1].下载固件到Flash*************\r\n\n");
        printf("************[2].启动App*********************\r\n\n");
        printf("==============================================================\r\n\n");
        
        // 阻塞式等待用户输入
        while(1)
				{
						if (usart_flag_get(USART0, USART_FLAG_RBNE) != RESET)
						{
								serialKey = (uint8_t)usart_data_receive(USART0);
								printf("serialKey: %c\r\n",serialKey);
								break;
						}
						
				}
        printf("\r\n Input -> %c 0x%2X\r\n", serialKey, serialKey); 
        
        if(serialKey == '1'){
            printf("下载固件\r\n");
            UpdateApp();
            delay_ms(100);
            BoottoApp();
        }
        if(serialKey == '2'){
            printf("启动到App\r\n");
            BoottoApp();
        }
        
    }
		
}


int main(void)
{
	 	delay_init(120);                     //��ʼ����ʱ���� 
	  	
		usart_init(115200);											//��ʼ������
	
		usart1_init(115200);

		printf("Hello World\r\n");
	
		printf("Test......\r\n");
		
#if IAP_BOOTLOADER_MODE1
	
		if(CheckNeedUpdate())
		{ 
			//接收Ymodem协议传输的固件并烧录
			UpdateApp();
			
			//清理升级标记
			ClearUpdateVerFlag();
			
			//直接跳转到APP
			BoottoApp();
		}
		else
		{
			//直接跳转到APP
			BoottoApp();
		}
	
#elif IAP_BOOTLOADER_MODE2
		
		main_menu_cmd();
		
#elif OTA_BOOTLOADER_MODE		
		//OTA任务
		delay_init(120);                     //��ʼ����ʱ���� 
	  	
		usart_init(115200);											//��ʼ������
	
		usart1_init(115200);

		printf("Hello World\r\n");
	
		printf("Test......\r\n");
		
		key_01_init();
		
		OLED_Init();
	
		OLED_Clear(1);
	
		OLED_ShowString(0,0,"BOOTLOAD",12,1);
	
		OLED_Refresh();
		
		if(CheckNeedUpdate()) //确实检测到了OTA升级
		{
//			//
			printf("OTA_NAME:%s,%d,%d\n",OTA_NAME,OTA_TID,OTA_SIZE);
			
			OLED_ShowString(0,12,"UPDATE EVENT!",12,1);
			OLED_ShowString(0,24,"PRESS WAIT TIME:",12,1);
			
			OLED_Refresh();
			
			for(uint8_t i = 0;i <10; i++)
			{
				
				if(i == 0) OLED_ShowNum(100,24,(10 - i),2,12,1);
				else OLED_ShowNum(100,24,(10 - i),2,12,1);
				
				OLED_Refresh();
				
				delay_ms(1000);
				
				if(key_val)
				{
					key_val = 0;
					printf("PRESS\n");
					
					OLED_ShowString(0,36,"START UPDATE!",12,1);
					
					OLED_Refresh();
					//开始下载数据
			
			   if(Wifi_DOWNLOAD_OTA_Version())
				 {
					 //进行OTA状态上报
				 
					 printf("OTA固件下载固件SUCCESS\n");
				 }
				 else
				 {
					 printf("OTA固件下载固件FAIL\n");
				 }
				 ClearUpdateVerFlag();//清理标志位区
				 
				 printf("FLAG FLASH COMPLATE FINISH!!!\n");
				 
				 OLED_ShowString(0,48,"DOWNLOAD FINISH!",12,1);
					
					OLED_Refresh();
				 
				 break;
				}
			}
			
			BoottoApp();
		}
		else
		{
			OLED_ShowString(0,12,"NO UPDATE EVENT!",12,1);
			OLED_ShowString(0,24,"GO TO APP TIME:",12,1);
			for(uint8_t i = 0;i <10; i++)
			{
				
				if(i == 0) OLED_ShowNum(100,24,(10 - i),2,12,1);
				else OLED_ShowNum(100,24,(10 - i),2,12,1);
				
				OLED_Refresh();
				
				delay_ms(1000);	
			}
			BoottoApp();
		}
		
#endif
		
		
   while(1)
	{
		
		
	}
}


