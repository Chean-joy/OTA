#include "tool.h"

#define IS_AF(c) ((c >= 'A') && (c <= 'F'))
#define IS_af(c) ((c >= 'a') && (c <= 'f'))
#define IS_09(c) ((c >= '0') && (c <= '9'))
#define ISVALIDHEX(c) (IS_AF(c) || IS_af(c) || IS_09(c))
#define ISVALIDDEC(c) IS_09(c)
#define CONVERTDEC(c) (c - '0')

#define CONVERTHEX_alpha(c) (IS_AF(c) ? (c - 'A' + 10) : (c - 'a' + 10))
#define CONVERTHEX(c) (IS_09(c) ? (c - '0') : CONVERTHEX_alpha(c))
/**
 * @brief  Convert a string to an integer
 * @param  inputstr: The string to be converted
 * @param  intnum: The intger value
 * @retval 1: Correct
 *         0: Error
 */
uint32_t Str2Int(uint8_t *inputstr, int32_t *intnum) {
  uint32_t i = 0, res = 0;
  uint32_t val = 0;

  if (inputstr[0] == '0' && (inputstr[1] == 'x' || inputstr[1] == 'X')) {
    if (inputstr[2] == '\0') {
      return 0;
    }
    for (i = 2; i < 11; i++) {
      if (inputstr[i] == '\0') {
        *intnum = val;
        /* return 1; */
        res = 1;
        break;
      }
      if (ISVALIDHEX(inputstr[i])) {
        val = (val << 4) + CONVERTHEX(inputstr[i]);
      } else {
        /* return 0, Invalid input */
        res = 0;
        break;
      }
    }
    /* over 8 digit hex --invalid */
    if (i >= 11) {
      res = 0;
    }
  } else /* max 10-digit decimal input */
  {
    for (i = 0; i < 11; i++) {
      if (inputstr[i] == '\0') {
        *intnum = val;
        /* return 1 */
        res = 1;
        break;
      } else if ((inputstr[i] == 'k' || inputstr[i] == 'K') && (i > 0)) {
        val = val << 10;
        *intnum = val;
        res = 1;
        break;
      } else if ((inputstr[i] == 'm' || inputstr[i] == 'M') && (i > 0)) {
        val = val << 20;
        *intnum = val;
        res = 1;
        break;
      } else if (ISVALIDDEC(inputstr[i])) {
        val = val * 10 + CONVERTDEC(inputstr[i]);
      } else {
        /* return 0, Invalid input */
        res = 0;
        break;
      }
    }
    /* Over 10 digit decimal --invalid */
    if (i >= 11) {
      res = 0;
    }
  }

  return res;
}


/******************************************************************************
 * Name:    CRC-16/XMODEM       x16+x12+x5+1
 * Poly:    0x1021
 * Init:    0x0000
 * Refin:   False
 * Refout:  False
 * Xorout:  0x0000
 * Alias:   CRC-16/ZMODEM,CRC-16/ACORN
 *****************************************************************************/
uint16_t Crc16Ymodem(uint8_t *data, uint16_t length) 
{
    uint8_t i;
    uint16_t crc = 0;  // Initial value
    while (length--) {
      crc ^= (uint16_t)(*data++) << 8;
      for (i = 0; i < 8; ++i) {
        if (crc & 0x8000)
          crc = (crc << 1) ^ 0x1021;
        else
          crc <<= 1;
      }
    }
    return crc;
}
