#include "cute_flash.h"

void flash_drv_test(void)
{
    int i;
    uint8_t data[10] = {0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E};
    uint8_t readData[128] = {0};
    drv_flash_read(0x00, readData, 10);
    printf("============[0]===========\r\n");
    for (i = 0; i < 10; i++)
    {
        printf("0x%02X,", readData[i]);
    }
    printf("\r\n");
    drv_flash_earse(0x00, 1);
    drv_flash_read(0x00, readData, 10);
    printf("============[1]===========\r\n");
    for (i = 0; i < 10; i++)
    {
        printf("0x%02X,", readData[i]);
    }
    printf("\r\n");
    drv_flash_write(0x00,data,10);
    drv_flash_read(0x00, readData, 10);
    printf("============[2]===========\r\n");
    for (i = 0; i < 10; i++)
    {
        printf("0x%02X,", readData[i]);
    }
    printf("\r\n");
}

typedef enum
{
    ID_A,
    ID_B,
    ID_C,
    ID_NUM,
}PARAM_ID;

typedef struct paramA_t
{
    uint8_t aa;
    uint8_t ab;
    uint8_t ac;
}paramA;

typedef struct paramB_t
{
    uint8_t ba;
    uint32_t bb;
    uint8_t bc;
}paramB;

typedef struct paramC_t
{
    uint16_t ca;
    uint32_t cb;
    uint32_t cc;
}paramC;

daMapInfo mapTable[ID_NUM] = {0};

int main()
{
    paramA pa = 
    {
        .aa = 0xAA,
        .ab = 0xBB,
        .ac = 0xCC,
    };
    paramB pb =
    {
        .ba = 0x44,
        .bb = 0x55555555,
        .bc = 0x66,
    };
    paramC pc = 
    {
        .ca = 0x9999,
        .cb = 0x11111111,
        .cc = 0x00000000,
    };
    mapTable[ID_A].len = sizeof(paramA);
    mapTable[ID_B].len = sizeof(paramB);
    mapTable[ID_C].len = sizeof(paramC);
    cute_flash_init(ID_NUM, mapTable);
    cute_flash_monitor();
    cute_flash_write(ID_A,&pa);
    cute_flash_monitor();
    cute_flash_write(ID_B,&pb);
    cute_flash_monitor();
    cute_flash_write(ID_C,&pc);
    cute_flash_monitor();
    cute_flash_read(ID_A,&pa);
    cute_flash_read(ID_B,&pb);
    cute_flash_read(ID_C,&pc);

    printf("pa.aa:%02x\r\n",pa.aa);
    printf("pa.ab:%02x\r\n",pa.ab);
    printf("pa.ac:%02x\r\n",pa.ac);

    printf("pb.ba:%02x\r\n",pb.ba);
    printf("pb.bb:%08x\r\n",pb.bb);
    printf("pb.bc:%02x\r\n",pb.bc);

    printf("pc.ca:%04x\r\n",pc.ca);
    printf("pc.cb:%08x\r\n",pc.cb);
    printf("pc.cc:%08x\r\n",pc.cc);
}
