#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "gd32f30x.h"
#include "gd32f30x_fmc.h"
#include "fmc_operation.h"
#include "flash_drv.h"

/* GD32F303VET6 512KB Flash，页大小 2KB */
#define FLASH_PAGE_SIZE       0x800
#define FLASH_END_ADDRESS     0x0807FFFF   /* 512K */

/* F30x 的 fmc_flag_clear 一次只能清一个标志 */
#define FMC_CLS_FLAG()   do { \
    fmc_flag_clear(FMC_FLAG_BANK0_END); \
    fmc_flag_clear(FMC_FLAG_BANK0_PGERR); \
    fmc_flag_clear(FMC_FLAG_BANK0_WPERR); \
} while(0)

#define FMC_CLS_FLAG2()  FMC_CLS_FLAG()

/**
*******************************************************************
* @function 指定地址开始读出指定个数的数据
* @param    readAddr,读取地址
* @param    pBuffer,数组首地址
* @param    numToRead,要读出的数据个数
* @return
*******************************************************************
*/
bool FlashRead(uint32_t readAddr, uint8_t *pBuffer, uint32_t numToRead)
{
    if ((readAddr + numToRead) > (FLASH_END_ADDRESS + 1)) {
        return false;
    }

    uint32_t addr = readAddr;
    for (uint32_t i = 0; i < numToRead; i++) {
        *pBuffer = *((uint8_t *)addr);
        addr++;
        pBuffer++;
    }
    return true;
}

/**
*******************************************************************
* @function 指定地址开始写入指定个数的数据
* @param    writeAddr,写入地址
* @param    pBuffer,数组首地址
* @param    numToWrite,要写入的数据个数
* @return
* @note     GD32F30x 不支持字节编程，这里用半字编程模拟。
*           要求 writeAddr 必须半字对齐（偶数地址）。
*******************************************************************
*/
bool FlashWrite8bit(uint32_t writeAddr, uint8_t *pBuffer, uint32_t numToWrite)
{
    if ((writeAddr + numToWrite) > (FLASH_END_ADDRESS + 1)) {
        return false;
    }
    if (writeAddr % 2 != 0) {   /* 半字编程要求地址对齐 */
        return false;
    }

    fmc_state_enum fmcState = FMC_READY;

    fmc_unlock();
    FMC_CLS_FLAG2();

    uint32_t i = 0;
    while (i < numToWrite) {
        uint16_t halfword;
        if ((i + 1) < numToWrite) {
            halfword = (uint16_t)pBuffer[i] | ((uint16_t)pBuffer[i + 1] << 8);
        } else {
            /* 最后一个字节，高字节保持擦除态 0xFF */
            halfword = 0xFF00U | (uint16_t)pBuffer[i];
        }

        fmcState = fmc_halfword_program(writeAddr, halfword);
        if (fmcState != FMC_READY) {
            fmc_lock();
            return false;
        }
        writeAddr += 2;
        i += 2;
    }

    fmc_lock();
    return true;
}

/**
*******************************************************************
* @function 指定地址开始写入指定个数的半字数据
* @param    writeAddr,写入地址
* @param    pBuffer,数组首地址
* @param    numToWrite,要写入的半字个数
* @return
*******************************************************************
*/
bool FlashWrite16bit(uint32_t writeAddr, uint16_t *pBuffer, uint32_t numToWrite)
{
    if ((writeAddr + numToWrite * 2) > (FLASH_END_ADDRESS + 1)) {
        return false;
    }
    if (writeAddr % 2 != 0) {
        return false;
    }

    fmc_state_enum fmcState = FMC_READY;

    fmc_unlock();
    FMC_CLS_FLAG2();

    for (uint32_t i = 0; i < numToWrite; i++) {
        fmcState = fmc_halfword_program(writeAddr, pBuffer[i]);
        if (fmcState != FMC_READY) {
            fmc_lock();
            return false;
        }
        writeAddr += 2;
    }

    fmc_lock();
    return true;
}

/**
*******************************************************************
* @function 擦除从 eraseAddr 开始到 eraseAddr + numToErase 覆盖的页
* @param    eraseAddr,地址
* @param    numToErase,数据字节数
* @return
*******************************************************************
*/
bool FlashErase(uint32_t eraseAddr, uint32_t numToErase)
{
    if (numToErase == 0) {
        return false;
    }
    if (eraseAddr > FLASH_END_ADDRESS) {
        return false;
    }
    if ((eraseAddr + numToErase - 1) > FLASH_END_ADDRESS) {
        return false;
    }

    uint32_t startPage = eraseAddr & ~(FLASH_PAGE_SIZE - 1);
    uint32_t endAddr   = eraseAddr + numToErase - 1;
    uint32_t endPage   = endAddr & ~(FLASH_PAGE_SIZE - 1);

    fmc_unlock();
    FMC_CLS_FLAG2();

    for (uint32_t page = startPage; page <= endPage; page += FLASH_PAGE_SIZE) {
        if (fmc_page_erase(page) != FMC_READY) {
            fmc_lock();
            return false;
        }
    }

    fmc_lock();
    return true;
}

#define BUFFER_SIZE                   50
#define FLASH_TEST_ADDRESS            0x08004000

uint8_t bufferWrite[BUFFER_SIZE];
uint8_t bufferRead[BUFFER_SIZE];

void FlashDrvTestStr(void)
{
    printf("开始擦除写入字符串：\n");

    if (!FlashErase(FLASH_TEST_ADDRESS, BUFFER_SIZE)) {
        printf("Flash擦除数据故障，请排查！\n");
        return;
    }

    if (!FlashWrite8bit(FLASH_TEST_ADDRESS, (uint8_t*)"Hello itheima", 13)) {
        printf("Flash写数据故障，请排查！\n");
        return;
    }

    printf("开始读取\n");
    if (!FlashRead(FLASH_TEST_ADDRESS, bufferRead, 13)) {
        printf("Flash读数据故障，请排查！\n");
        return;
    }

    bufferRead[13] = '\0';
    printf("\nFlash写入读取结果：%s\n", bufferRead);
}

void FlashDrvTest(void)
{
    printf("准备数据：\n");
    for (uint16_t i = 0; i < BUFFER_SIZE; i++) {
        bufferWrite[i] = i + 1;
        printf("0x%02X ", bufferWrite[i]);
    }

    printf("\n开始擦除\n");
    if (!FlashErase(FLASH_TEST_ADDRESS, BUFFER_SIZE)) {
        printf("Flash擦除数据故障，请排查！\n");
        return;
    }

    printf("\n开始写入\n");
    if (!FlashWrite8bit(FLASH_TEST_ADDRESS, bufferWrite, BUFFER_SIZE)) {
        printf("Flash写数据故障，请排查！\n");
        return;
    }

    printf("\n开始读取\n");
    if (!FlashRead(FLASH_TEST_ADDRESS, bufferRead, BUFFER_SIZE)) {
        printf("Flash读数据故障，请排查！\n");
        return;
    }

    for (uint16_t i = 0; i < BUFFER_SIZE; i++) {
        if (bufferRead[i] != bufferWrite[i]) {
            printf("0x%02X ", bufferRead[i]);
            printf("Flash测试故障，请排查！\n");
            return;
        }
        printf("0x%02X ", bufferRead[i]);
    }
    printf("\nFlash测试通过！\n");
}

void FlashDrvTestModity(void)
{
    printf("开始读取\n");
    if (!FlashRead(FLASH_TEST_ADDRESS, bufferRead, BUFFER_SIZE)) {
        printf("Flash读数据故障，请排查！\n");
        return;
    }

    printf("\n开始修改\n");
    bufferRead[3] = 0x33;
    bufferRead[4] = 0x44;

    printf("\n开始擦除\n");
    if (!FlashErase(FLASH_TEST_ADDRESS, BUFFER_SIZE)) {
        printf("Flash擦除数据故障，请排查！\n");
        return;
    }

    printf("\n开始写入\n");
    if (!FlashWrite8bit(FLASH_TEST_ADDRESS, bufferRead, BUFFER_SIZE)) {
        printf("Flash写数据故障，请排查！\n");
        return;
    }

    printf("\n打印修改后的数据: \n");
    for (uint16_t i = 0; i < BUFFER_SIZE; i++) {
        printf("0x%02X ", bufferRead[i]);
    }
    printf("\nFlash测试通过！\n");
}
