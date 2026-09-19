#ifndef __TOOLS_H__
#define __TOOLS_H__


#include "gd32f30x.h"

// 将字符串, 转成数字 "1234" -> 1234
uint32_t Str2Int(uint8_t *inputstr, int32_t *intnum);

// 计算Ymodem协议的CRC16校验码
uint16_t Crc16Ymodem(uint8_t *data, uint16_t length);

#endif

