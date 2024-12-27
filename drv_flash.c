#include "drv_flash.h"
/**
 * @brief 读取flash
 * @param addr 读取起始地址
 * @param pData 目标指针
 * @param len 读取长度 
*/
void drv_flash_read(uint32_t addr,uint8_t *pData,uint16_t len)
{
    FILE *file = fopen("example.bin", "rb+");
    fseek(file, addr, SEEK_SET);
    fread(pData, 1, len, file);
    fclose(file);
}
/**
 * @brief 擦除flash
 * @param addr 擦除起始地址
 * @param sec_num 擦除扇区数量
*/
void drv_flash_earse(uint32_t addr,uint16_t sec_num)
{
    if(sec_num > SECTION_MAX)
    {
        return;
    }
    int i;
    uint8_t emptyData[SECTION_SIZE];
    memset(emptyData, 0xFF, SECTION_SIZE);

    FILE *file = fopen("example.bin", "rb+");
    for (i = 0; i < sec_num; i++)
    {
        fseek(file, addr + SECTION_SIZE * i, SEEK_SET);
        fwrite(emptyData, 1, SECTION_SIZE, file);
    }
    fclose(file);
}
/**
 * @brief 写入flash
 * @param addr 写起始地址
 * @param pData 待写数据指针
 * @param len 写入长度 
*/
void drv_flash_write(uint32_t addr,uint8_t *pData,uint16_t len)
{
    FILE *file = fopen("example.bin", "rb+");
    fseek(file, addr, SEEK_SET);
    fwrite(pData, 1, len, file);
    fclose(file);
}
