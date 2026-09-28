#include "wifi.h"
#include "cJSON.h"

//C标准库
#include <string.h>

#define WIFI_SSID "iPhone13Pro"
#define WIFI_PASSWORD "chean666"




#define OTA_VERSION_CHEACK_URL "AT+HTTPCLIENT=2,0,\"http://iot-api.heclouds.com/fuse-ota/oN0s560311/device003/check?\
type=1&version=1.0\",\"\",\"\",1,\"Authorization: version=2018-10-31&res=products%2FH14gVsQnXZ&et=2011523689&method=md5&sign=EWBvCjrXulhL5P8AFyUUhQ%3D%3D\"\r\n"

#define TOKEN "version=2018-10-31&res=products%2FH14gVsQnXZ&et=2011523689&method=md5&sign=EWBvCjrXulhL5P8AFyUUhQ%3D%3D"

static uint8_t wifi_buff[1024];
static uint16_t wifi_buff_lens;
uint8_t Wifi_init(void)
{
    uint8_t res;

	delay_ms(1000);
	
    /* 1. 复位模块：发完即走，不等应答，下面用延时等它重启 */
    Send_data_get_reply("AT+RST\r\n",wifi_buff,&wifi_buff_lens,2000);
	
	printf("wifi_buff : %s\n",wifi_buff);

    if(res) return 1;
    else
    {
        if(strstr((char*)wifi_buff,"OK") != NULL);
        else return 1;
    }


    delay_ms(1000);   /* 等模块重启完成 */

    /* 2. 关闭回显 */
		delay_ms(1000);
	
    res = Send_data_get_reply("ATE0\r\n",wifi_buff,&wifi_buff_lens,1000);
		
	
		printf("wifi_buff : %s\n",wifi_buff);

    if(res) return 1;
    else
    {
        if(strstr((char*)wifi_buff,"ATE0") != NULL);
        else return 1;
    }

    /* 3. 设置为 Station 模式 */
		delay_ms(1000);
		
    res = Send_data_get_reply("AT+CWMODE=1\r\n",wifi_buff,&wifi_buff_lens,1000);
	
		printf("wifi_buff : %s\n",wifi_buff);
    if(res) return 1;
    else
    {
        if(strstr((char*)wifi_buff,"OK") != NULL);
        else return 1;
    }

    return 0;
}
/**
  +HTTPCLIENT:184,{"code":0,"msg":"succ","data":{"target":"v2.0","tid":1323672,"size":32160,"md5":"9c4644110e3af1b7d01b0a0c9afc3148","status":2,"type":1},"request_id":"8464370fa2a546e4b6e946c1f873a7f4"}
  
  json ->
  {
	  "code": 0,
	  "msg": "succ",
	  "data": {
		  "target": "v2.0",
		  "tid": 1323672,
		  "size": 32160,
		  "md5": "9c4644110e3af1b7d01b0a0c9afc3148",
		  "status": 2,
		  "type": 1
	  },
	  "request_id": "8464370fa2a546e4b6e946c1f873a7f4"
  }
  通过json字符串提取:target固件名称, tid任务id, size固件大小, md5校验值
  
  @param jsonStr json字符串
  @return 0 成功 -1 失败
 */

ota_info_t current_ota_info; // 用于存储解析后的 OTA 信息

uint8_t Wifi_check_OTA_Version(void)
{
   // 1. 发送 HTTP 请求获取响应
    uint8_t res = Send_data_get_reply(OTA_VERSION_CHEACK_URL, wifi_buff, &wifi_buff_lens, 1000);
    
    if (res != 0) {
        printf("Send_data_get_reply failed, res = %d\n", res);
        return 1;
    }

    // 2. 检查是否接收到服务器响应
    if (strstr((char*)wifi_buff, "+HTTPCLIENT") == NULL) {
        printf("No +HTTPCLIENT found in response.\n");
        return 1;
    }

    // 3. 查找 JSON 字符串的起始位置（跳过 +HTTPCLIENT:184, 等前缀）
    char *json_str = strstr((char*)wifi_buff, "{");
    if (json_str == NULL) {
        printf("JSON start symbol '{' not found.\n");
        return 1;
    }

    // 4. 解析 JSON
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        printf("cJSON_Parse failed.\n");
        return 1;
    }

    // 5. 获取 code 字段并判断状态
    cJSON *code = cJSON_GetObjectItem(root, "code");
    if (code == NULL || code->type != cJSON_Number || code->valueint != 0) {
        printf("JSON response error, code != 0.\n");
        cJSON_Delete(root);
        return 1;
    }

    // 6. 获取 data 对象
    cJSON *data = cJSON_GetObjectItem(root, "data");
    if (data == NULL || data->type != cJSON_Object) {
        printf("JSON 'data' object not found.\n");
        cJSON_Delete(root);
        return 1;
    }

    // 7. 提取 target (固件名称)
    cJSON *target = cJSON_GetObjectItem(data, "target");
    if (target && target->type == cJSON_String && target->valuestring) {
        strncpy(current_ota_info.target, target->valuestring, sizeof(current_ota_info.target) - 1);
        current_ota_info.target[sizeof(current_ota_info.target) - 1] = '\0'; // 确保字符串以 '\0' 结尾
        printf("target: %s\n", current_ota_info.target);
    }

    // 8. 提取 tid (任务ID)
    cJSON *tid = cJSON_GetObjectItem(data, "tid");
    if (tid && tid->type == cJSON_Number) {
        current_ota_info.tid = tid->valueint;
        printf("tid: %d\n", current_ota_info.tid);
    }

    // 9. 提取 size (固件大小)
    cJSON *size = cJSON_GetObjectItem(data, "size");
    if (size && size->type == cJSON_Number) {
        current_ota_info.size = size->valueint;
        printf("size: %d\n", current_ota_info.size);
    }

    // 10. 提取 md5 (校验值)
    cJSON *md5 = cJSON_GetObjectItem(data, "md5");
    if (md5 && md5->type == cJSON_String && md5->valuestring) {
        strncpy(current_ota_info.md5, md5->valuestring, sizeof(current_ota_info.md5) - 1);
        current_ota_info.md5[sizeof(current_ota_info.md5) - 1] = '\0';
        printf("md5: %s\n", current_ota_info.md5);
    }

    // 11. 释放 cJSON 占用的内存，防止内存泄漏
    cJSON_Delete(root);

    return 0; // 成功
}


uint8_t Wifi_connect(void)
{
     delay_ms(1000);
    //首先检查有无WIFI的连接模式
    uint8_t res  =  Send_data_get_reply("AT+CWMODE?\r\n",wifi_buff,&wifi_buff_lens,1000);

    printf("wifi_buff : %s\n",wifi_buff);

    if(res) return 1;
    else
    {
        if(strstr((char*)wifi_buff,"+CWMODE:1") != NULL);
        else return 1;
    }

    //在检查有无连接到WIFI
    res  =  Send_data_get_reply("AT+CWJAP?\r\n",wifi_buff,&wifi_buff_lens,1000);
    printf("wifi_buff : %s\n",wifi_buff);
    if(res) return 1;
    else
    {
        if(strstr((char*)wifi_buff,WIFI_SSID) != NULL)
        {
            //表明已经连接到WIFI
            return 0;
        }
        else
        {
            //表明没有连接到WIFI
            res = Send_data_get_reply("AT+CWJAP=\"" WIFI_SSID "\",\"" WIFI_PASSWORD "\"\r\n",wifi_buff,&wifi_buff_lens,2500);
            //尝试连接WIFI
            if(res) return 1;
            else
            {
                if(strstr((char*)wifi_buff,"OK") != NULL);
                else return 1;
            }
        }
    }
    return 0; //连接成功
}

void nop_delay_us(uint32_t us)
{
    // 根据实测调整，假设每次循环约 4 个周期
    // 120MHz 下，1us = 120 个周期，循环次数 = 120 / 4 = 30
    uint32_t count = us * (CPU_FREQ_MHZ / 4) * 1000; // 计算循环次数
    while (count--)
    {
        __NOP();
    }
}
