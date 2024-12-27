#ifndef CUTE_FLASH_H
#define CUTE_FLASH_H
#include "drv_flash.h"

#define SZ_CFG_ADDR 0x00 // 存储区起始地址
#define SZ_CFG_SIZE 256  // 存储区总大小
#define DA_SIZE_MAX 256  // 单个数据块最大大小

typedef struct daMapInfo_t
{
    uint8_t len;   // 长度
    uint32_t addr; // 地址
} daMapInfo;
/**
 * @brief 磨损均衡模块初始化
 * @param idNum 数据块数量
 * @param mapTable 映射表地址
*/
void cute_flash_init(uint32_t idNum,daMapInfo *mapTable);
/**
 * @brief 存储数据读取
 * @param id 数据块ID
 * @param pData 数据块目标地址
 * @return 0：失败，1：成功
*/
uint8_t cute_flash_read(uint8_t id,void* pData);
/**
 * @brief 存储数据写入
 * @param id 数据块ID
 * @param pData 写入数据块地址
*/
void cute_flash_write(uint8_t id,void* pData);

void cute_flash_monitor(void);
#endif
