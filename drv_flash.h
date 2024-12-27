#ifndef DRV_FLASH_H
#define DRV_FLASH_H
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
// 扇区大小
#define SECTION_SIZE 128
// 扇区数量
#define SECTION_MAX 2
/**
 * @brief 读取flash
 * @param addr 读取起始地址
 * @param pData 目标指针
 * @param len 读取长度 
*/
void drv_flash_read(uint32_t addr,uint8_t *pData,uint16_t len);
/**
 * @brief 擦除flash
 * @param addr 擦除起始地址
 * @param sec_num 擦除扇区数量
*/
void drv_flash_earse(uint32_t addr,uint16_t sec_num);
/**
 * @brief 写入flash
 * @param addr 写起始地址
 * @param pData 待写数据指针
 * @param len 写入长度 
*/
void drv_flash_write(uint32_t addr,uint8_t *pData,uint16_t len);
#endif
