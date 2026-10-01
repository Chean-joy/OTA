#include "wifi.h"
#include "cJSON.h"

#include "flash_drv.h"

//C标准库
#include <string.h>
#include <stdlib.h>

#define WIFI_SSID "iPhone13Pro"
#define WIFI_PASSWORD "chean666"

#define OTA_VERSION_CHEACK_URL "AT+HTTPCLIENT=2,0,\"http://iot-api.heclouds.com/fuse-ota/H14gVsQnXZ/device1/check?\
type=1&version=1.0\",\"\",\"\",1,\"Authorization: version=2018-10-31&res=products%2FH14gVsQnXZ&et=2011523689&method=md5&sign=EWBvCjrXulhL5P8AFyUUhQ%3D%3D\"\r\n"

#define TOKEN "version=2018-10-31&res=products%2FH14gVsQnXZ&et=2011523689&method=md5&sign=EWBvCjrXulhL5P8AFyUUhQ%3D%3D"

#define OTA_STATUS_POST_URL101 \
    "AT+HTTPCLIENT=3,1," \
    "\"https://iot-api.heclouds.com/fuse-ota/oN0s560311/device002/1372950/status\"," \
    "\"\"," \
    "\"\"," \
    "2," \
    "\"{\\\"step\\\":101}\"," \
    "\"Authorization: version=2018-10-31&res=products%2FoN0s560311&et=1810052414&method=md5&sign=fanGm12AsIKpJ%2BfgQhWh3Q%3D%3D\"" \
    "\r\n"
#define OTA_STATUS_POST_URL201 \
    "AT+HTTPCLIENT=3,1," \
    "\"https://iot-api.heclouds.com/fuse-ota/oN0s560311/device002/1372950/status\"," \
    "\"\"," \
    "\"\"," \
    "2," \
    "\"{\\\"step\\\":201}\"," \
    "\"Authorization: version=2018-10-31&res=products%2FoN0s560311&et=1810052414&method=md5&sign=fanGm12AsIKpJ%2BfgQhWh3Q%3D%3D\"" \
    "\r\n"
#define OTA_STATUS_POST_URL202 \
    "AT+HTTPCLIENT=3,1," \
    "\"https://iot-api.heclouds.com/fuse-ota/oN0s560311/device002/1372950/status\"," \
    "\"\"," \
    "\"\"," \
    "2," \
    "\"{\\\"step\\\":202}\"," \
    "\"Authorization: version=2018-10-31&res=products%2FoN0s560311&et=1810052414&method=md5&sign=fanGm12AsIKpJ%2BfgQhWh3Q%3D%3D\"" \
    "\r\n"

#define OTA_STATUS_POST_URL203 \
    "AT+HTTPCLIENT=3,1," \
    "\"https://iot-api.heclouds.com/fuse-ota/oN0s560311/device002/1372950/status\"," \
    "\"\"," \
    "\"\"," \
    "2," \
    "\"{\\\"step\\\":203}\"," \
    "\"Authorization: version=2018-10-31&res=products%2FoN0s560311&et=1810052414&method=md5&sign=fanGm12AsIKpJ%2BfgQhWh3Q%3D%3D\"" \
    "\r\n"

		
//step 是关键
//101	升级包下载成功（设备状态变成：升级中）。
//102	下载失败,空间不足（设备状态变成：升级失败）。
//103	下载失败,内存溢出（设备状态变成：升级失败）。
//104	下载失败,下载请求超时（设备状态变成：升级失败）。
//105	下载失败,电量不足（设备状态变成：升级失败）。
//106	下载失败,信号不良（设备状态变成：升级失败）。
//107	下载失败,未知异常（设备状态变成：升级失败）。
//201	升级成功，此时会把设备的版本号修改为任务的目标版本（设备状态变成：升级完成）。
//202	升级失败,电量不足（设备状态变成：升级失败）。
//203	升级失败,内存溢出（设备状态变成：升级失败）。
//204	升级失败,升级包与当前任务目标版本不一致（设备状态变成：升级失败）。
//205	升级失败,MD5校验失败（设备状态变成：升级失败）。
//206	升级失败,未知异常（设备状态变成：升级失败）。
//207	达到最大重试次数（设备状态变成：升级失败）。


static uint8_t wifi_buff[1024];
static uint16_t wifi_buff_lens;
uint8_t Wifi_init(void)
{
    uint8_t res;

		delay_ms(1000);
	
    /* 1. 复位模块：发完即走，不等应答，下面用延时等它重启 */
    res = Send_data_get_reply("AT+RST\r\n",wifi_buff,&wifi_buff_lens,2000);
	
		delay_ms(500);
	
		printf("wifi_buff : %s\n",wifi_buff);

    if(res) { printf("RST ERROR"); return 1;}
    else
    {
        if((strstr((char*)wifi_buff,"OK") != NULL)
					||(strstr((char*)wifi_buff,"AT+RST") != NULL));
        else {
				
				printf("RST ERROR"); return 1;
				
				}
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


/**
 * @brief 从 ESP-AT 的 +HTTPCLIENT 响应里提取二进制数据
 * @return 实际提取的字节数，0 表示失败/没数据
 */
uint32_t parse_httpclient_data(uint8_t *src, uint32_t src_len,
                               uint8_t *dst, uint32_t dst_max)
{
    /* 1. 找 "+HTTPCLIENT:" 的位置 */
    char *p = strstr((char*)src, "+HTTPCLIENT:");
    if (!p) return 0;   /* 没找到（可能是 busy p... 或空响应） */
    p += 12;            /* 跳过 "+HTTPCLIENT:" 共 12 个字符 */

    /* 2. 读取 size（十进制，直到逗号） */
    uint32_t size = 0;
    while (*p >= '0' && *p <= '9') {
        size = size * 10 + (*p - '0');
        p++;
    }
    if (*p != ',') return 0;
    p++;   /* 跳过逗号，此时 p 指向二进制数据的第一个字节 */

    /* 3. 检查是否真的收到了这么多字节 */
    uint32_t header_len = (uint8_t*)p - src;
    if (header_len + size > src_len) {
        /* 数据不完整，可能被截断 */
        printf("httpclient truncated: expect %u, have %u\r\n",
               size, src_len - header_len);
        return 0;
    }

    /* 4. 拷贝到目标缓冲区 */
    uint32_t copy = (size > dst_max) ? dst_max : size;
    memcpy(dst, p, copy);
    return copy;
}


char command_buff[300] = {0};

//uint8_t Wifi_DOWNLOAD_OTA_Version(void)
//{
//		memset(command_buff,0,sizeof command_buff);
//	
//    uint16_t count = (OTA_SIZE + 199) / 200;   // 总包数，向上取整

////	printf("count:%d\n",count);
//	
//    for (uint16_t i = 0; i < count; i++)
//    {
//        uint32_t start = i * 200;
//        uint32_t end = start + 199;
//        if (end >= OTA_SIZE) {
//            end = OTA_SIZE - 1;   // 最后一包可能不足 200 字节
//        }

//        /* 
//         * 注意几点：
//         * 1. 字符串中的双引号必须用 \" 转义
//         * 2. Range 头格式是 "Range: bytes=start-end"，注意是 bytes=
//         * 3. 签名里的 % 必须写成 %% 才能被 sprintf 正确输出
//         * 4. URL、Authorization 等参数请以你实际 AT 固件手册为准
//         */
//        sprintf(command_buff,
//            "AT+HTTPCLIENT=2,0,"
//            "\"https://iot-api.heclouds.com/fuse-ota/H14gVsQnXZ/device1/1513559/download/\","
//            "\"\","
//            "\"\","
//            "1,"
//            "\"Authorization: version=2018-10-31&res=products%%2FH14gVsQnXZ&et=2011523689&method=md5&sign=EWBvCjrXulhL5P8AFyUUhQ%%3D%%3D\","
//            "\"Range: bytes=%u-%u\""
//            "\r\n",
//            start, end);

//        uint8_t res = Send_data_get_reply(command_buff, wifi_buff, &wifi_buff_lens, 1500);
////        if (res) {
////            return 0;   // 失败
////        }

//        /* 到这里 wifi_buff 里就是本次接收到的数据（最多 200 字节） */
//        /* 你可以在这里把数据写入 Flash、解析、打印等 */
//        printf("data len=%u: ", wifi_buff_lens);
//					for (uint16_t k = 0; k < wifi_buff_lens; k++) {
//							printf("%02X ", wifi_buff[k]);
//					}
//					printf("\r\n");
//				
//				
//    }

//    return 1;   // 全部下载完成
//}

uint8_t Wifi_DOWNLOAD_OTA_Version(void)
{
    memset(command_buff, 0, sizeof(command_buff));

//    uint16_t count = (OTA_SIZE + 199) / 200;
	
		uint16_t count = (OTA_SIZE + 599) / 600;
    uint32_t write_offset = 0;

		if(FlashErase(APP_ADDR_BUFF_IN_FLASH, OTA_SIZE+5))
		{
			printf("clear flash complate!!!\n");
		}
		else
		{
			printf("clear flash ERROR!!!\n");
		}
	
			
		
    for (uint16_t i = 0; i < count; i++)
    {
//        uint32_t start = i * 200;
//        uint32_t end = start + 199;
					uint32_t start = i * 600;
        uint32_t end = start + 599;
			
        if (end >= OTA_SIZE) end = OTA_SIZE - 1;

        sprintf(command_buff,
            "AT+HTTPCLIENT=2,0,"
            "\"https://iot-api.heclouds.com/fuse-ota/H14gVsQnXZ/device1/1513559/download/\","
            "\"\","
            "\"\","
            "1,"
            "\"Authorization: version=2018-10-31&res=products%%2FH14gVsQnXZ&et=2011523689&method=md5&sign=EWBvCjrXulhL5P8AFyUUhQ%%3D%%3D\","
            "\"Range: bytes=%u-%u\""
            "\r\n",
            start, end);

        /* 重试直到拿到真数据 */
//        uint8_t bin_buffer[256];
			uint8_t bin_buffer[800];
        uint32_t bin_len = 0;
        for (int retry = 0; retry < 10; retry++)
        {
            if (Send_data_get_reply(command_buff, wifi_buff, &wifi_buff_lens, 2000) != 0) {
                printf("timeout at %u, retry %d\r\n", start, retry);
                delay_ms(300);
                continue;
            }

            bin_len = parse_httpclient_data(wifi_buff, wifi_buff_lens,
                                            bin_buffer, sizeof(bin_buffer));
            if (bin_len > 0) break;

            /* 拿到的是空 / busy p... → 等一下再试 */
            printf("retry %d at %u, len=%u\r\n", retry, start, wifi_buff_lens);
            delay_ms(1000);
        }

        if (bin_len == 0) {
            printf("give up at offset %u\r\n", start);
            return 0;
        }

        /* 调试：打印前 16 字节 */
        printf("got %u bytes at %u: ", bin_len, start);
        for (uint32_t k = 0; k < bin_len && k < 16; k++) printf("%02X ", bin_buffer[k]);
        printf("\r\n");

				if (!WriteintoFLASHData(APP_ADDR_BUFF_IN_FLASH + write_offset, bin_buffer, bin_len)) {
    printf("Flash write FAILED at offset %u, len %u\r\n", write_offset, bin_len);
    return 0;
					}

				/* 写入 Flash（后面再加） */
        write_offset += bin_len;
				
        /* 在两次请求之间给 ESP 喘息时间 */
        delay_ms(600);
    }
		
		
		//下载完成
		Wifi_STATUS_OTA_POST(OTA_STATUS_POST_URL101);
		

    printf("download done, total %u bytes\r\n", write_offset);
    
		//进行数据替换
		if(!FLASH_BUFF_REPLACE(APP_ADDR_IN_FLASH,APP_ADDR_BUFF_IN_FLASH,OTA_SIZE))
		{
				printf("REPLACE SUCCESS!!!\n");
			
		}
			
		
		//升级成功
		Wifi_STATUS_OTA_POST(OTA_STATUS_POST_URL201);
		
		return 1;
}




//{
//	"code": 0,
//	"msg": "succ",
//	"request_id": "**********"
//}

/**
 * @brief  解析服务器返回的状态上报 JSON
 * @param  json_str  HTTP 响应字符串（可能包含 AT 头尾）
 * @return 0-成功，非0-失败
 */
uint8_t Wifi_PARSE_OTA_POST_RESPONSE(const char *json_str)
{
    if (json_str == NULL) return 1;

    /* 1. 找 JSON 起始位置 */
    const char *p = strchr(json_str, '{');
    if (p == NULL) { printf("JSON not found\r\n"); return 2; }

    /* 2. 找 JSON 结束位置 */
    const char *end = strrchr(p, '}');
    if (end == NULL) { printf("JSON end not found\r\n"); return 3; }

    /* 3. 可选：从 +HTTPCLIENT:<size>, 里提取长度做完整性校验 */
    const char *hdr = strstr(json_str, "+HTTPCLIENT:");
    if (hdr != NULL) {
        uint32_t expect_len = (uint32_t)atoi(hdr + 12);
        uint32_t actual_len = end - p + 1;
        if (actual_len < expect_len) {
            printf("incomplete: expect %u, have %u\r\n", expect_len, actual_len);
            return 4;
        }
    }

    /* 4. 拷贝纯 JSON 到本地缓冲区 */
    static char json_buf[256];
    uint32_t len = end - p + 1;
    if (len >= sizeof(json_buf)) { printf("JSON too long\r\n"); return 5; }
    memcpy(json_buf, p, len);
    json_buf[len] = '\0';
    printf("JSON: %s\r\n", json_buf);

    /* 5. 解析 */
    cJSON *root = cJSON_Parse(json_buf);
    if (root == NULL) {
        const char *err = cJSON_GetErrorPtr();
        printf("cJSON parse error near: %s\r\n", err ? err : "unknown");
        return 6;
    }

    /* 6. 提取字段（兼容旧版 cJSON，用 type 判断） */
    int result_code = -1;

    cJSON *item_code = cJSON_GetObjectItem(root, "code");
    if (item_code != NULL && item_code->type == cJSON_Number) {
        result_code = item_code->valueint;
        printf("code = %d\r\n", result_code);
    }

    cJSON *item_msg = cJSON_GetObjectItem(root, "msg");
    if (item_msg != NULL && item_msg->type == cJSON_String) {
        printf("msg = %s\r\n", item_msg->valuestring);
    }

    cJSON *item_rid = cJSON_GetObjectItem(root, "request_id");
    if (item_rid != NULL && item_rid->type == cJSON_String) {
        printf("request_id = %s\r\n", item_rid->valuestring);
        /* 如果后续要用到 request_id，可以拷贝到全局变量 */
        // strncpy(g_request_id, item_rid->valuestring, sizeof(g_request_id)-1);
    }

    /* 7. 释放 */
    cJSON_Delete(root);

    /* 8. 返回结果 */
    return (result_code == 0) ? 0 : 7;
}
//数据更新完成,然后进行上报状态

uint8_t Wifi_STATUS_OTA_POST(const char *command)
{
    uint8_t res = Send_data_get_reply(command, wifi_buff, &wifi_buff_lens, 2000);
    if (res != 0) {
        printf("post error!!!\n");
        return 1;
    }

    /* 解析服务器返回的 JSON */
    if (Wifi_PARSE_OTA_POST_RESPONSE((char *)wifi_buff) != 0) {
        printf("parse response failed\r\n");
        return 2;
    }

    printf("OTA status posted successfully\r\n");
    return 0;
}












