## 一、需求介绍
+ 寿命问题
    - 问题描述：在嵌入式环境中常用的存储器有NORFlash、NANDFlash、EEPROM，前两个一般擦写寿命约为10w次，EEPROM的使用次数约为100w次，寿命长的我先不管QAQ，这里的寿命指的是当flash中的存储单元写入或者擦除超过这个次数，这个存储单元可能会出现出错、变慢等无法正常读写的问题。按照木桶效应最先到达存储寿命的存储单元就是整个存储器的整体寿命。
    - 解决思路：存储器单个存储单元的寿命有10w次，假如数据存储的时候先后在10个地址存储确保每个存储单元每隔10次数据改动才需要擦写消耗一次寿命，那存储器的整体寿命就会提升到100w。
+ 读写问题
    - 问题描述：Flash一般最小擦写单位为扇区，因为这个特性在进行数据修改的时候就需要扇区地址的对齐并且在操作扇区时将整个扇区读取在RAM中修改完后整体擦除再写进去非常的麻烦。
    - 解决思路：数据库的增删改查操作都是通过key-value实现，key-value是指键值对，应用程序通过数据的键操作数据值，对应用程序简单友好。假如Flash中的数据块操作也通过kv操作，上层应用程序就不需要考虑因为Flash特性带来的地址对齐、擦除问题。

## 二、软件设计
### 2.1 数据结构
+ 映射表：ID与数据块地址映射表，表中数据块ID与数据块地址一一对应，模块初始化时从存储区中得到，读取时不会更新，当写入时会因为数据迁移或者数据区迁移变更对应地址
+ 存储区（逻辑区）：实际的FLASH划分区域，比如数据需要存储在FLASH的0x00-0x800区域内，0x00-0x800就是存储区的范围，大小为2K，存储区包含两个数据区。每个数据区包含一个及以上个FLASH最小擦写区，如FLASH最小操作一个扇区为1K，则数据区为N个扇区的大小，N为整数，存储区为两个数据区则为2N个扇区的大小。
+ 数据区：一个存储区包含两个数据区，两个数据区大小相同，循环使用，同一时刻只使用其中一个数据区，数据区包含头部信息和数据存储区两个部分，头部信息包含存储区的使用次数信息；

| 组成 | 长度（byte） | 说明 |
| --- | --- | --- |
| flag | 4 | 废弃标志/使用标志 |
| number | 4 | 数据区的使用次数，每次使用都会依次累加 |


+ 数据块：数据块存储在数据存储区中，多个数据块链式连接，当出现多个相同ID的数据块时地址最大的数据块为最新的数据块。

| 组成 | 长度（byte） | 说明 |
| --- | --- | --- |
| head | 1 | 数据块的头标识，默认值为0x5A |
| id | 1 | 数据块唯一ID |
| len | 1 | 数据块中的数据长度 |
| crc8 | 1 | 数据块中的数据CRC8校验码 |
| data | len | 数据块中的数据内容 |


![结构说明:数据块∈数据区∈存储区](https://cdn.nlark.com/yuque/0/2024/png/1561832/1735192960703-e351fdc0-c587-484a-853d-3bdc342f18b6.png)

### 2.2 设计思路
+ KV操作
    - 读取：应用层通过ID查询指定存储参数内容，读取接口首先通过ID和映射表找到存储器中对应的数据块地址然后将数据块拷贝一份到目标地址，当未在存储区找到正确的ID时读取失败，应用层如果读取失败则说明未写入或者写入数据发生错误，此时使用默认参数。
    - 修改：应用层将待修改参数的ID和待修改参数内容通过映射表找到当前数据区空闲的位置，将此数据块变更到新增的数据地址后将映射表中的数据块地址进行更新
+ 磨损均衡
    - 具体方法：在上述KV操作中的修改方法中，每当数据块更新时如果当前使用数据区还有空闲空间则更新数据块在空闲空间位置，当当前数据区没有空闲空间时则迁移数据当另外一个数据区

### 2.3 关键流程
+ 初始化

![组件初始模块](https://cdn.nlark.com/yuque/0/2024/png/1561832/1735192812759-53b6c62f-c5f1-484b-9b0b-7809e5459123.png)

+ 读取数据

![数据读取流程](https://cdn.nlark.com/yuque/0/2024/png/1561832/1735192775834-b45b484b-e5ef-447b-995a-41b90e17479a.png)

+ 修改数据

![数据修改流程](https://cdn.nlark.com/yuque/0/2024/png/1561832/1735193631753-85af67df-6133-4276-adc1-1828e5fb2c50.png)

## 三、软件实践
> 当前示例在windows平台上使用一个bin文件模拟Flash，使用文件读写的方式模拟flash数据读写，bin文件总大小为256字节，最小擦写的单位为扇区大小为128字节。单个数据区大小为128字节意味着总存储器大小为两个扇区，每个数据区占用一个扇区。
>

+ Flash数据读写通用驱动接口

```c
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
```

+ 类型定义

```c
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
```

+ 组件初始化

```c
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
```

+ 数据块读取

```c
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
```

+ 数据块修改

```c
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
```

