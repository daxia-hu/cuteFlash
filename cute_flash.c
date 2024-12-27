#include "cute_flash.h"
#define MAGIC_NUM 0x5A
#define SZ_SIZE_MIN 24
#define SZ_NUM_DEFAULT 0xFFFFFFFF // 默认标志
#define ACTIVITY_FLAG 0xAB34CD78  // 使用标志
#define DISCARD_FLAG 0x00000000   // 废弃标志
typedef enum
{
    SZ_NOUSED = 0x06,  // 未使用
    SZ_NORMAL = 0x07,  // 正常使用
} STORE_ZONE_STATE;
typedef enum
{
    DA_A = 0x0A, // 数据区A
    DA_B = 0x0B, // 数据区B
} DATA_AREA_ID;
// 数据区头信息
typedef struct daHeadInfo_t
{
    uint32_t flag;   // 是否在使用
    uint32_t number; // 使用次数
} daHeadInfo;
#pragma pack(1)
// 数据块头信息
typedef struct dbHeadInfo_t
{
    uint8_t crc;
    uint8_t len;
    uint8_t id;
    uint8_t magic;
} dbHeadInfo;
#pragma pack()
// 组件类型
typedef struct cuteFlashType_t
{
    uint8_t UsedArea : 4; // 当前使用数据区
    uint8_t szState : 4;  // 存储区状态
    uint16_t idNum;       // id数量
    uint32_t daBase;      // 当前数据区基础地址
    uint32_t daBorder;    // 当前数据区边界
    uint32_t daOffset;    // 当前数据区偏移位置
    daMapInfo *mapTable;  // 映射表
} cuteFlashType;
static cuteFlashType actObj;
uint8_t CRC8Table[256] =
    {
        // 120424-1     CRC Table
        0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15, 0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
        0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65, 0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
        0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5, 0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
        0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85, 0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
        0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2, 0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
        0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2, 0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
        0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32, 0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
        0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42, 0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
        0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C, 0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
        0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC, 0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
        0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C, 0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
        0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C, 0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
        0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B, 0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
        0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B, 0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
        0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB, 0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
        0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB, 0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3,
};
uint8_t CrcCalculateCRC8(const uint8_t *p, uint32_t counter)
{
    uint8_t crc8 = 0;
    for (; counter > 0; counter--)
    {
        crc8 = CRC8Table[crc8 ^ *p];
        p++;
    }
    return (crc8);
}
// 存储区初始化检查
uint8_t cuteFlashInitCheck(uint32_t idNum, daMapInfo *mapTable)
{
    int i = 0;
    uint32_t totalSize = 0;
    // 1. 判断大小是否合法
    if (!(NULL == idNum || NULL == mapTable || SZ_CFG_SIZE < SZ_SIZE_MIN))
    {
        for (i = 0; i < idNum; i++)
        {
            totalSize += mapTable[i].len;
            totalSize += sizeof(dbHeadInfo);
        }
        if (SZ_CFG_SIZE / 4 > totalSize)
        {
            actObj.idNum = idNum;
            actObj.mapTable = mapTable;
            return 1;
        }
    }
    return 0;
}
// 获取当前使用的数据区
void cuteFlashCurrentDataArea(void)
{
    daHeadInfo szHeadA;
    daHeadInfo szHeadB;
    drv_flash_read(SZ_CFG_ADDR, (uint8_t *)&szHeadA, sizeof(daHeadInfo));
    drv_flash_read(SZ_CFG_ADDR + SZ_CFG_SIZE / 2, (uint8_t *)&szHeadB, sizeof(daHeadInfo));
    if (ACTIVITY_FLAG == szHeadA.flag)
    {
        actObj.UsedArea = DA_A;
        actObj.szState = SZ_NORMAL;
        return;
    }
    if (ACTIVITY_FLAG == szHeadB.flag)
    {
        actObj.UsedArea = DA_B;
        actObj.szState = SZ_NORMAL;
        return;
    }
    drv_flash_earse(SZ_CFG_ADDR, SECTION_MAX);
    actObj.szState = SZ_NOUSED;
}
// 填充映射数据表
void cuteFlashFillMapTable(void)
{
    uint32_t offset = 0;
    dbHeadInfo dbHead;
    uint8_t data[DA_SIZE_MAX] = {0};
    switch (actObj.UsedArea)
    {
    case DA_A:
        actObj.daBase = SZ_CFG_ADDR;
        actObj.daBorder = SZ_CFG_SIZE / 2 - 1;
        break;
    case DA_B:
        actObj.daBase = SZ_CFG_ADDR + SZ_CFG_SIZE / 2;
        actObj.daBorder = SZ_CFG_SIZE - 1;
        break;
    default:
        return;
    }
    actObj.daOffset = actObj.daBase + sizeof(daHeadInfo);
    while (1)
    {
        // 已经到达当前数据块边界
        if (actObj.daOffset + offset >= actObj.daBorder - sizeof(dbHead))
        {
            break;
        }
        else
        {
            drv_flash_read(actObj.daOffset + offset, (uint8_t *)&dbHead, sizeof(dbHead));
            // 数据块头校验、长度校验
            if (MAGIC_NUM == dbHead.magic && dbHead.id < actObj.idNum && actObj.mapTable[dbHead.id].len == dbHead.len)
            {
                drv_flash_read(actObj.daOffset + offset, (uint8_t *)data, dbHead.len);
                // 数据块CRC校验
                if (dbHead.crc != CrcCalculateCRC8(data, dbHead.len))
                {
                    actObj.mapTable[dbHead.id].addr = actObj.daOffset + offset + sizeof(dbHeadInfo);
                    offset += sizeof(dbHeadInfo);
                    offset += dbHead.len;
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }
    actObj.daOffset += offset;
}
/**
 * @brief 磨损均衡模块初始化
 * @param idNum 数据块数量
 * @param mapTable 映射表地址
*/
void cute_flash_init(uint32_t idNum,daMapInfo *mapTable)
{
    // 1. 存储区初始化合法性检查
    if(!cuteFlashInitCheck(idNum,mapTable))
    {
        printf("%s:%d\r\n",__func__,__LINE__);
        while(1);
        return;
    }
    // 2. 判断当前使用数据区 0x31、0x32代表数据区A、B其他数据则存储区未使用
    cuteFlashCurrentDataArea();
    // 3. 映射表地址填充
    cuteFlashFillMapTable();
}
/**
 * @brief 存储数据读取
 * @param id 数据块ID
 * @param pData 数据块目标地址
*/
uint8_t cute_flash_read(uint8_t id, void *pData)
{
    if (id >= actObj.idNum || NULL == pData || NULL == actObj.mapTable[id].len)
    {
        return 0;
    }
    drv_flash_read(actObj.mapTable[id].addr, (uint8_t *)pData, actObj.mapTable[id].len);
    return 1;
}
/**
 * @brief 存储数据写入
 * @param id 数据块ID
 * @param pData 写入数据块地址
*/
void cute_flash_write(uint8_t id,void* pData)
{
    int i = 0;
    dbHeadInfo dbHead;
    uint8_t data[DA_SIZE_MAX] = {0};
    if (id >= actObj.idNum || NULL == pData)
    {
        return;
    }
    dbHead.magic = MAGIC_NUM;
    dbHead.id = id;
    dbHead.len = actObj.mapTable[id].len;
    dbHead.crc = CrcCalculateCRC8(pData, dbHead.len);
    // 存储区未使用
    if (SZ_NOUSED == actObj.szState)
    {
        // 写入数据区头部信息
        daHeadInfo szHeadA;
        szHeadA.flag = ACTIVITY_FLAG;
        szHeadA.number = 1;
        actObj.daBase = SZ_CFG_ADDR;
        drv_flash_write(actObj.daBase, (uint8_t *)&szHeadA, sizeof(daHeadInfo));
        actObj.daOffset = actObj.daBase + sizeof(daHeadInfo);
        // 写入数据块头部信息
        drv_flash_write(actObj.daOffset, (uint8_t *)&dbHead, sizeof(dbHead));
        actObj.daOffset += sizeof(dbHead);
        // 更新映射表
        actObj.mapTable[id].addr = actObj.daOffset;
        // 写入数据块内容
        drv_flash_write(actObj.daOffset, (uint8_t *)pData, dbHead.len);
        actObj.daOffset += dbHead.len;
        // 设置当前数据区
        actObj.UsedArea = DA_A;
        actObj.daBorder = SZ_CFG_SIZE / 2 - 1;
        // 设置当前存储区状态
        actObj.szState = SZ_NORMAL;
    }
    else if (actObj.daOffset + actObj.mapTable[id].len + sizeof(dbHeadInfo) > actObj.daBorder)
    {
        daHeadInfo szHeadOld;
        daHeadInfo szHeadNew;
        uint32_t oldBase = actObj.daBase;
        drv_flash_read(oldBase, (uint8_t *)&szHeadOld, sizeof(daHeadInfo));
        szHeadOld.flag = DISCARD_FLAG;
        szHeadNew.number = szHeadOld.number + 1;
        szHeadNew.flag = ACTIVITY_FLAG;
        // 设置当前数据区
        actObj.UsedArea = (DA_A == actObj.UsedArea) ? DA_B : DA_A;
        switch (actObj.UsedArea)
        {
        case DA_A:
            actObj.daBase = SZ_CFG_ADDR;
            actObj.daBorder = SZ_CFG_SIZE / 2 - 1;
            drv_flash_earse(SZ_CFG_ADDR, SECTION_MAX / 2);
            break;
        case DA_B:
            actObj.daBase = SZ_CFG_ADDR + SZ_CFG_SIZE / 2;
            actObj.daBorder = SZ_CFG_SIZE - 1;
            drv_flash_earse(SZ_CFG_ADDR + SZ_CFG_SIZE / 2, SECTION_MAX / 2);
            break;
        default:
            return;
        }
        actObj.daOffset = actObj.daBase + sizeof(daHeadInfo);
        // 写入当前数据块到待迁移的数据区
        drv_flash_write(actObj.daOffset, (uint8_t *)&dbHead, sizeof(dbHead));
        actObj.daOffset += sizeof(dbHead);
        // 更新映射表
        actObj.mapTable[id].addr = actObj.daOffset;
        drv_flash_write(actObj.daOffset, (uint8_t *)pData, dbHead.len);
        actObj.daOffset += dbHead.len;
        // 迁移数据区，将原来数据区的内容迁移到新的数据区
        for (i = 0; i < actObj.idNum; i++)
        {
            if ((i == id) || (NULL == actObj.mapTable[i].addr))
            {
                continue;
            }
            else
            {
                // 拷贝数据头
                drv_flash_read(actObj.mapTable[i].addr - sizeof(dbHeadInfo), (uint8_t *)&dbHead, sizeof(dbHeadInfo));
                drv_flash_write(actObj.daOffset, (uint8_t *)&dbHead, sizeof(dbHeadInfo));
                actObj.daOffset += sizeof(dbHeadInfo);
                // 拷贝数据内容
                drv_flash_read(actObj.mapTable[i].addr, (uint8_t *)data, actObj.mapTable[i].len);
                // 更新映射表
                actObj.mapTable[i].addr = actObj.daOffset;
                drv_flash_write(actObj.daOffset, (uint8_t *)data, dbHead.len);
                actObj.daOffset += dbHead.len;
            }
        }
        // 设置活动数据区
        drv_flash_write(actObj.daBase, (uint8_t *)&szHeadNew, sizeof(daHeadInfo));
        drv_flash_write(oldBase, (uint8_t *)&szHeadOld, sizeof(daHeadInfo));
    }
    else
    {
        drv_flash_write(actObj.daOffset, (uint8_t *)&dbHead, sizeof(dbHeadInfo));
        actObj.daOffset += sizeof(dbHeadInfo);
        // 更新映射表
        actObj.mapTable[id].addr = actObj.daOffset;
        drv_flash_write(actObj.daOffset, (uint8_t *)pData, dbHead.len);
        actObj.daOffset += dbHead.len;
    }
}

void cute_flash_monitor(void)
{
    static uint32_t cnt = 0;
    printf("----------------[%d]---------------\r\n",cnt);
    printf("UsedArea:%d\r\n", actObj.UsedArea);
    printf("szState:%d\r\n", actObj.szState);
    printf("idNum:%d\r\n", actObj.idNum);
    printf("daBase:%d\r\n", actObj.daBase);
    printf("daBorder:%d\r\n", actObj.daBorder);
    printf("daOffset:%d\r\n", actObj.daOffset);
    int i = 0;
    for (i = 0; i < actObj.idNum; i++)
    {
        printf("id:%02d,%02d,%08x\r\n", i, actObj.mapTable[i].len, actObj.mapTable[i].addr);
    }
    printf("----------------[%d]---------------\r\n",cnt);
    cnt++;
}
