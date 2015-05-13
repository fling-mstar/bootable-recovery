//<MStar Software>
//******************************************************************************
// MStar Software
// Copyright (c) 2010 - 2014 MStar Semiconductor, Inc. All rights reserved.
// All software, firmware and related documentation herein ("MStar Software") are
// intellectual property of MStar Semiconductor, Inc. ("MStar") and protected by
// law, including, but not limited to, copyright law and international treaties.
// Any use, modification, reproduction, retransmission, or republication of all
// or part of MStar Software is expressly prohibited, unless prior written
// permission has been granted by MStar.
//
// By accessing, browsing and/or using MStar Software, you acknowledge that you
// have read, understood, and agree, to be bound by below terms ("Terms") and to
// comply with all applicable laws and regulations:
//
// 1. MStar shall retain any and all right, ownership and interest to MStar
//    Software and any modification/derivatives thereof.
//    No right, ownership, or interest to MStar Software and any
//    modification/derivatives thereof is transferred to you under Terms.
//
// 2. You understand that MStar Software might include, incorporate or be
//    supplied together with third party`s software and the use of MStar
//    Software may require additional licenses from third parties.
//    Therefore, you hereby agree it is your sole responsibility to separately
//    obtain any and all third party right and license necessary for your use of
//    such third party`s software.
//
// 3. MStar Software and any modification/derivatives thereof shall be deemed as
//    MStar`s confidential information and you agree to keep MStar`s
//    confidential information in strictest confidence and not disclose to any
//    third party.
//
// 4. MStar Software is provided on an "AS IS" basis without warranties of any
//    kind. Any warranties are hereby expressly disclaimed by MStar, including
//    without limitation, any warranties of merchantability, non-infringement of
//    intellectual property rights, fitness for a particular purpose, error free
//    and in conformity with any international standard.  You agree to waive any
//    claim against MStar for any loss, damage, cost or expense that you may
//    incur related to your use of MStar Software.
//    In no event shall MStar be liable for any direct, indirect, incidental or
//    consequential damages, including without limitation, lost of profit or
//    revenues, lost or damage of data, and unauthorized system use.
//    You agree that this Section 4 shall still apply without being affected
//    even if MStar Software has been modified by MStar in accordance with your
//    request or instruction for your use, except otherwise agreed by both
//    parties in writing.
//
// 5. If requested, MStar may from time to time provide technical supports or
//    services in relation with MStar Software to you for your use of
//    MStar Software in conjunction with your or your customer`s product
//    ("Services").
//    You understand and agree that, except otherwise agreed by both parties in
//    writing, Services are provided on an "AS IS" basis and the warranty
//    disclaimer set forth in Section 4 above shall apply.
//
// 6. Nothing contained herein shall be construed as by implication, estoppels
//    or otherwise:
//    (a) conferring any license or right to use MStar name, trademark, service
//        mark, symbol or any other identification;
//    (b) obligating MStar or any of its affiliates to furnish any person,
//        including without limitation, you and your customers, any assistance
//        of any kind whatsoever, or any information; or
//    (c) conferring any license or right under any intellectual property right.
//
// 7. These terms shall be governed by and construed in accordance with the laws
//    of Taiwan, R.O.C., excluding its conflict of law rules.
//    Any and all dispute arising out hereof or related hereto shall be finally
//    settled by arbitration referred to the Chinese Arbitration Association,
//    Taipei in accordance with the ROC Arbitration Law and the Arbitration
//    Rules of the Association by three (3) arbitrators appointed in accordance
//    with the said Rules.
//    The place of arbitration shall be in Taipei, Taiwan and the language shall
//    be English.
//    The arbitration award shall be final and binding to both parties.
//
//******************************************************************************
//<MStar Software>

#include "ursautils.h"
#include <apiSWI2C.h>
#include "MsOS.h"
#include "MsTypes.h"
#include "drvCPU.h"
#include <mmap.h>
#include <iniparser.h>
#include <stdbool.h>

//============for ISP Upgrade============

#define URSA_ISP_I2C_BUS_SEL 0x0
#define SPI_WRITE           0x10
#define SPI_READ            0x11
#define SPI_STOP            0x12

#define CCMD_DATA_WRITE 0x10

#define BANKSIZE    0x10000
#define BLOCKNUM  0xff
#define BLOCKSIZE 0x100

#define ENTER_ISP_ERROR_MAX     20
#define GET_ID_ERROR_MAX        10

#define SW_IIC_READ             0
#define SW_IIC_WRITE            1
#define CONFIG_URSA_INI              "/config/Ursa/ursa_upgrade.ini"

typedef struct {
    uint32_t size;// 1-4
    uint8_t manuFacturerId;// 5
    uint8_t deviceId;// 6
    uint8_t WREN;// 7
    uint8_t WRDI;// 8
    uint8_t RDSR;// 9
    uint8_t WRSR;// 10
    uint8_t READS;// 11
    uint8_t FAST_READ;// 12
    uint8_t PG_PROG;// 13
    uint8_t SECTOR_ERASE;// 14
    uint8_t BLOCK_ERASE;// 15
    uint8_t CHIP_ERASE;// 16
    uint8_t RDID;// 17
    uint8_t LOCK;// 18
    uint16_t chipEraseMaxTime;// 19-20//Unit:100ms
} SFlashType;

uint8_t g_ucflashtype;
uint8_t g_ucbannumber;
SFlashType g_sflash;

uint8_t g_curpercent;
uint32_t g_ispsrcaddr;
uint32_t g_ispcodesize;
FlashProgErrorType g_errorflag;
FlashProgStateType g_eflashprogramstate;

uint8_t g_slaveaddr = 0x94;
uint8_t g_serialdebugaddr = 0xB8;
uint8_t g_ursai2cbusnum = 1;

static SWI2C_BusCfg g_ursai2cbuscfg[1] = {
    // Bus-0
    {0, 0, 0}, //IS_SW_I2C  /SCL_PAD /SDA_PAD /Delay
};

SFlashType g_supportsflash[Flash_NUMS] = {
// 1-4:size ,5:manuFacturerId,6:deviceId,7:WREN,8:WRDI,9:RDSR,10:WRSR,11:READ,12:FAST_READ,
// 13:PG_PROG,14:SECTOR_ERASE,15:BLOCK_ERASE,16:CHIP_ERASE,17:RDID,18:LOCK,19-20: chipEraseMaxTime

//                     1-4     ,5   ,6   ,7   ,8   ,9  ,10  ,11  ,12  ,13  ,14  ,15  ,16  ,17 ,18 ,19-20
//PMC
{/*"PMC25LV512A", */  0x20000,0x9D,0x7B,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD7,0xD8,0xC7,0xAB,0x0C,40  },
{/*" PMC25LV010", */  0x20000,0x9D,0x7C,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD7,0xD8,0xC7,0xAB,0x0C,40  },
{/*" PMC25LV020", */  0x40000,0x9D,0x7D,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD7,0xD8,0xC7,0xAB,0x0C,40  },
{/*" PMC25LV040", */  0x80000,0x9D,0x7E,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD7,0xD8,0xC7,0xAB,0x1C,40  },
//EON
{/*"    EN25P10", */  0x20000,0x1C,0x10,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD8,0xD8,0xC7,0xAB,0x0C,80  },
{/*"    EN25P20", */  0x40000,0x1C,0x11,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD8,0xD8,0xC7,0xAB,0x0C,80  },
{/*"    EN25P40", */  0x80000,0x1C,0x12,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD8,0xD8,0xC7,0xAB,0x1C,80  },
{/*"    EN25P80", */  0x100000,0x1C,0x13,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD8,0xD8,0xC7,0xAB,0x1C,80  },
{/*"    EN25F40", */  0x80000,0x1C,0x12,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD8,0xD8,0xC7,0xAB,0x1C,80  },
{/*"    EN25F80", */  0x100000,0x1C,0x13,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD8,0xD8,0xC7,0xAB,0x1C,220  },
{/*"    EN25F16", */  0x200000,0x1C,0x14,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD8,0xD8,0xC7,0xAB,0x1C,440  },
{/*"    EN25P16", */  0x80000,0x1C,0x14,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD8,0xD8,0xC7,0xAB,0x1C,220 },
{/*"    EN25B20", */  0x40000,0x1C,0x41,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD8,0xD8,0xC7,0xAB,0x0C,80  },
{/*"    EN25B40", */  0x80000,0x1C,0x42,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD8,0xD8,0xC7,0xAB,0x1C,80  },
{/*"   EN25B40T", */  0x80000,0x1C,0x42,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD8,0xD8,0xC7,0xAB,0x1C,80  },
{/*"    EN25B16", */  0x80000,0x1C,0x34,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD8,0xD8,0xC7,0xAB,0x1C,220 },
{/*"   EN25B16T", */  0x80000,0x1C,0x34,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD8,0xD8,0xC7,0xAB,0x1C,220 },
{/*"    EN25B32", */  0x80000,0x1C,0x35,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD8,0xD8,0xC7,0xAB,0x1C,440 },
//                     1-4     ,5   ,6   ,7   ,8   ,9  ,10  ,11  ,12  ,13  ,14  ,15  ,16  ,17 ,18 ,19-20
//SPANSION
{/*"  S25FL004A", */  0x80000,0x01,0x12,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD8,0xD8,0xC7,0xAB,0x1C,260 },
{/*"  S25FL016A", */0x1000000,0x01,0x14,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD8,0xD8,0xC7,0xAB,0x1C,1000},
{/*"  S25FL040A", */  0x80000,0x01,0x25,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD8,0xD8,0xC7,0xAB,0x1C,260 },
//Winbond & NX
{/*"    NX25P16", */0x1000000,0xEF,0x15,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD8,0xD8,0xC7,0xAB,0x1C,250 },
{/*"     W25X10", */  0x20000,0xEF,0x10,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD8,0xD8,0xC7,0xAB,0x0C,70  },
{/*"     W25X20", */  0x40000,0xEF,0x51,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD8,0xD8,0xC7,0xAB,0x1C,250 },
{/*"     W25X40", */  0x80000,0xEF,0x52,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD8,0xD8,0xC7,0xAB,0x1C,250 },
{/*"     W25P20", */  0x40000,0xEF,0x11,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD8,0xD8,0xC7,0xAB,0x1C,250 },
{/*"     W25P40", */  0x80000,0xEF,0x12,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD8,0xD8,0xC7,0xAB,0x1C,250 },
{/*"     W25X16", */  0x20000,0xEF,0x14,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD8,0xD8,0xC7,0xAB,0x0C,256  },
{/*"     W25X32", */  0x40000,0xEF,0x15,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD8,0xD8,0xC7,0xAB,0x1C,512 },
{/*"     W25X64", */  0x80000,0xEF,0x16,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD8,0xD8,0xC7,0xAB,0x1C,1000 },
//SST
{/*"SST25VF016B", */0x1000000,0xBF,0x41,0x06,0x04,0x05,0x01,0x03,NULL,0xAD,0xD8,0xD8,0xC7,0xAB,0x1C,40  },
{/*"SST25VF040B", */  0x80000,0xBF,0x8D,0x06,0x04,0x05,0x01,0x03,NULL,0xAD,0xD8,0xD8,0xC7,0xAB,0x1C,40  },
//MX
{/*" MX25L4005A", */  0x80000,0xC2,0x12,0x06,0x04,0x05,0x01,0x03,0x0B,0x02,0x20,0xD8,0xC7,0xAB,0x1C,80  },
{/*" MX25L8005", */  0x100000,0xC2,0x13,0x06,0x04,0x05,0x01,0x03,0x0B,0x02,0x20,0xD8,0xC7,0xAB,0x1C,160  },
{/*"  MX25L2005", */  0x40000,0xC2,0x11,0x06,0x04,0x05,0x01,0x03,0x0B,0x02,0x20,0xD8,0xC7,0xAB,0x1C,50  },
{/*"  MX25L1605", */0x1000000,0xC2,0x14,0x06,0x04,0x05,0x01,0x03,0x0B,0x02,0x20,0xD8,0xC7,0xAB,0x1C,550 },
{/*"  MX25L3205", */0x1000000,0xC2,0x15,0x06,0x04,0x05,0x01,0x03,0x0B,0x02,0x20,0xD8,0xC7,0xAB,0x1C,750 },
{/*"  MX25L6405", */0x1000000,0xC2,0x16,0x06,0x04,0x05,0x01,0x03,0x0B,0x02,0x20,0xD8,0xC7,0xAB,0x1C,1000 },
//INTEL
{/*"QB25F160S33B8", */0x20000,0x89,0x11,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD7,0xD8,0xC7,0xAB,0x0C,128  },
{/*"QB25F320S33B8", */0x40000,0x89,0x12,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD7,0xD8,0xC7,0xAB,0x0C,256  },
{/*"QB25F640S33B8", */0x80000,0x89,0x13,0x06,0x04,0x05,0x01,0x03,NULL,0x02,0xD7,0xD8,0xC7,0xAB,0x1C,512  },
//AMIC
{/*" A25L40P", */     0x80000,0x37,0x13,0x06,0x04,0x05,0x01,0x03,0x0B,0x02,0x20,0xD8,0xC7,0xAB,0x1C,128  },
{/*" GD25Q32", */     0x40000,0xC8,0x16,0x06,0x04,0x05,0x01,0x03,0x0B,0x02,0x20,0xD8,0xC7,0xAB,0x1C,512  },
{/*" A25L016", */     0x200000,0x37,0x14,0x06,0x04,0x05,0x01,0x03,0x0B,0x02,0x20,0xD8,0xC7,0xAB,0x1C,1000  },
};

static void i2c_read_datas(uint8_t  * pAddr, uint8_t addrSize, uint8_t * pDatabuf, uint8_t dataSize) {
    uint8_t k;
    MApi_SWI2C_UseBus(URSA_ISP_I2C_BUS_SEL);

    if (MApi_SWI2C_AccessStart(g_slaveaddr, SW_IIC_WRITE) == FALSE) {
        printf("i2c_read_datas->IIC_AccessStart Error\n");
    }

    for (k = 0;k < addrSize; k++) {
       if (MApi_SWI2C_SendByte(pAddr[k]) == FALSE) {
            printf("i2c_read_datas pAddr Error\n");
       }
    }

    if (MApi_SWI2C_AccessStart(g_slaveaddr, SW_IIC_READ) == FALSE) {
        printf("i2c_read_datas->IIC_AccessReStart Error\n");
    }

    for (k = 0; k < dataSize-1; k++) {
       pDatabuf[k] = MApi_SWI2C_GetByte(1);
    }
    //last byte
    pDatabuf[k]=MApi_SWI2C_GetByte(0);

    MApi_SWI2C_Stop();
}

static void i2c_write_datas(uint8_t * pAddr, uint8_t addrSize, uint8_t  * pDatabuf, uint8_t dataSize) {
    uint8_t k;
    MApi_SWI2C_UseBus(URSA_ISP_I2C_BUS_SEL);

    if (MApi_SWI2C_AccessStart(g_slaveaddr, SW_IIC_WRITE) == FALSE) {
        printf("i2c_write_datas->IIC_AccessStart Error\n");
    }

    for (k=0;k<addrSize;k++) {
        if(MApi_SWI2C_SendByte(pAddr[k])==FALSE) {
            printf("pAddr Error\n");
        }
    }

    for (k = 0; k < dataSize; k++) {
        if (MApi_SWI2C_SendByte(pDatabuf[k]) == FALSE) {
            printf("pDatabuf Error\n");
        }
    }

    MApi_SWI2C_Stop();
}

static void i2c_write_datas2(uint8_t * pAddr, uint8_t addrSize, uint8_t * pDatabuf, uint8_t dataSize) {
    uint8_t k;
    MApi_SWI2C_UseBus(URSA_ISP_I2C_BUS_SEL);
    MApi_SWI2C_AccessStart(g_serialdebugaddr, SW_IIC_WRITE);

    for (k = 0; k < addrSize; k++) {
        MApi_SWI2C_SendByte(pAddr[k]);
    }

    for (k = 0; k < dataSize; k++) {
        MApi_SWI2C_SendByte(pDatabuf[k]);
    }

    MApi_SWI2C_Stop();
}

static void serialflash_writereg(uint8_t bBank, uint8_t bAddr, uint8_t bData) {
    uint8_t bWriteData[4];

    bWriteData[0] = CCMD_DATA_WRITE;
    bWriteData[1] = bBank;
    bWriteData[2] = bAddr;
    bWriteData[3] = bData;

    i2c_write_datas2(bWriteData, sizeof(bWriteData),NULL,0);
}

static void i2c_write_stop(void) {
    MApi_SWI2C_UseBus(URSA_ISP_I2C_BUS_SEL);

    if (MApi_SWI2C_AccessStart(g_slaveaddr, SW_IIC_WRITE) == FALSE) {
        printf("i2c_write_stop->IIC_AccessStart Error\n");
    }
    if (MApi_SWI2C_SendByte(SPI_STOP) == FALSE) {
        printf("i2c_write_stop-> Error\n");
    }
    MApi_SWI2C_Stop();
}

static void i2c_commond_read(void) {
    MApi_SWI2C_UseBus(URSA_ISP_I2C_BUS_SEL);

    if (MApi_SWI2C_AccessStart(g_slaveaddr, SW_IIC_WRITE) == FALSE) {
        printf("i2c_commond_read Error\n");
    }
    if (MApi_SWI2C_SendByte(SPI_READ) == FALSE) {
        printf("i2c_commond_read-> SPI_READ\n");
    }
}

static uint8_t read_chip_id(void) {
    uint8_t readIdCommand[] = {SPI_WRITE,0xAB,0x00,0x00,0x00};
    uint8_t buf;

    i2c_write_datas(readIdCommand,sizeof(readIdCommand),NULL,0);

    buf = SPI_READ; //Manufacture ID
    i2c_read_datas(&buf,1,&buf,1);

    buf = SPI_READ; //Device ID1
    i2c_read_datas(&buf,1,&buf,1);

    buf = SPI_READ; //Device ID2
    i2c_read_datas(&buf,1,&buf,1);

    i2c_write_stop();
    return buf;
}

static uint8_t read_chip_jedec_id(uint8_t *readIdBuf) {
    uint8_t readIdCommand[] = {SPI_WRITE,0x9f};

    uint8_t buf;

    i2c_write_datas(readIdCommand,sizeof(readIdCommand),NULL,0);

    buf = SPI_READ; //Manufacture ID
    i2c_read_datas(&buf,1,&readIdBuf[0],1);

    buf = SPI_READ; //Device ID1
    i2c_read_datas(&buf,1,&readIdBuf[1],1);

    buf = SPI_READ; //Device ID2
    i2c_read_datas(&buf,1,&readIdBuf[2],1);

    i2c_write_stop();
    return buf;
}

static uint8_t read_chip_id_buffer(uint8_t *readIdBuf) {
    uint8_t readIdCommand[] = {SPI_WRITE,0xAB,0x00,0x00,0x00};
    uint8_t buf;

    i2c_write_datas(readIdCommand,sizeof(readIdCommand),NULL,0);

    buf = SPI_READ; //Manufacture ID
    i2c_read_datas(&buf,1,&readIdBuf[0],1);

    buf = SPI_READ; //Device ID1
    i2c_read_datas(&buf,1,&readIdBuf[1],1);

    buf = SPI_READ; //Device ID2
    i2c_read_datas(&buf,1,&readIdBuf[2],1);

    i2c_write_stop();
    return buf;
}

static bool get_sflash_type(void) {
    uint8_t temp;
    uint8_t ucTmpBuf[4];

    read_chip_jedec_id(ucTmpBuf);
    printf("***read_chip_jedec_id[%x]\n",ucTmpBuf[0]);
    switch (ucTmpBuf[0]) {
        //AL
        case 0x37:
            printf("AL or AMIC\n");
            temp = read_chip_id();
            printf("AL or AMIC ChipID=%x\n",temp);
            if (temp == 0x12) {
                g_ucflashtype = Flash_A25L40P;
                g_ucbannumber = 8;
                printf("A25L40P\n");
            } else if (temp == 0x14) {
                g_ucflashtype = Flash_A25L016;
                g_ucbannumber = 32;
                printf("A25L016\n");
            } else {
                return false;
            }
            break;
        //PMC
        case 0x7F:
            printf("PMC or AMIC\n");
            temp = read_chip_id();
            printf("PMC or AMIC ChipID=%x\n",temp);
            if (temp == 0x7C) {
                g_ucflashtype = Flash_PMC010;
                g_ucbannumber = 2;
            }
            else if (temp == 0x7D) {
                g_ucflashtype = Flash_PMC020;
                g_ucbannumber = 4;
            } else if (temp == 0x7E) {
                g_ucflashtype = Flash_PMC040;
                g_ucbannumber = 8;
            } else if (temp == 0x12) {
                g_ucflashtype = Flash_A25L40P;
                g_ucbannumber = 8;
                printf("A25L40P\n");
            } else {
                return false;
            }
            break;
        //EON
        case 0x1C:
            printf("EON\n");
            temp = read_chip_id();
            printf("EON ChipID=%x\n",temp);
            if (ucTmpBuf[2] == 0x20) {
                if (ucTmpBuf[1] == 0x11 && temp == 0x10) {
                    g_ucflashtype = Flash_EN25P10;
                    g_ucbannumber = 2;
                    printf("EN25P10\n");
                } else if (ucTmpBuf[1] == 0x12 && temp == 0x11) {
                    g_ucflashtype = Flash_EN25P20;
                    g_ucbannumber = 4;
                    printf("EN25P20\n");
                } else if (ucTmpBuf[1] == 0x13 && temp == 0x12) {
                    g_ucflashtype = Flash_EN25P40;
                    g_ucbannumber = 8;
                    printf("EN25P40\n");
                } else if (ucTmpBuf[1] == 0x14 && temp == 0x13) {
                    g_ucflashtype = Flash_EN25P80;
                    g_ucbannumber = 16;
                    printf("EN25P80\n");
                } else if (ucTmpBuf[1] == 0x12 && temp == 0x41) {
                    g_ucflashtype = Flash_EN25B20;
                    g_ucbannumber = 4;
                    printf("EN25B20\n");
                } else if (ucTmpBuf[1] == 0x13 && temp == 0x32) {
                    g_ucflashtype = Flash_EN25B40;
                    g_ucbannumber = 8;
                    printf("EN25B40\n");
                } else if (ucTmpBuf[1] == 0x13 && temp == 0x42) {
                    g_ucflashtype = Flash_EN25B40T;
                    g_ucbannumber = 8;
                    printf("EN25B40T\n");
                } else if (ucTmpBuf[1] == 0x15 && temp == 0x14) {
                    g_ucflashtype = Flash_EN25P16;
                    g_ucbannumber = 32;
                    printf("EN25P16\n");
                } else if (ucTmpBuf[1] == 0x15 && temp == 0x34) {
                    g_ucflashtype = Flash_EN25B16;
                    g_ucbannumber = 32;
                    printf("EN25B16\n");
                } else if (ucTmpBuf[1] == 0x15 && temp == 0x44) {
                    g_ucflashtype = Flash_EN25B16T;
                    g_ucbannumber = 32;
                    printf("EN25B16T\n");
                } else if (ucTmpBuf[1] == 0x16 && temp == 0x35) {
                    g_ucflashtype = Flash_EN25B32;
                    g_ucbannumber = 64;
                    printf("EN25B32\n");
                } else {
                    return false;
                }
            } else if (ucTmpBuf[2] == 0x31) {
                if (ucTmpBuf[1] == 0x13 && temp == 0x12) {
                    g_ucflashtype = Flash_EN25F40;
                    g_ucbannumber = 8;
                    printf("EN25F40\n");
                } else if (ucTmpBuf[1] == 0x14 && temp == 0x13) {
                    g_ucflashtype = Flash_EN25F80;
                    g_ucbannumber = 16;
                    printf("EN25F80\n");
                } else if (ucTmpBuf[1] == 0x15 && temp == 0x14) {
                    g_ucflashtype = Flash_EN25F16;
                    g_ucbannumber = 32;
                    printf("EN25F16\n");
                }
            } else{
                return false;
            }
            break;
        case 0x01:
            printf("SPANSION\n");
            printf("chip ID=%x\n",ucTmpBuf[1]);
            if (ucTmpBuf[1] == 0x14) {
                g_ucflashtype = Flash_S25FL016A;
                g_ucbannumber = 32;
                printf("S25FL016A\n");
            } else if (ucTmpBuf[1] == 0x12) {
                g_ucflashtype = Flash_S25FL004A;
                g_ucbannumber = 8;
                printf("S25FL004A\n");
            } else if (ucTmpBuf[1] == 0x25) {
                g_ucflashtype = Flash_S25FL040A;
                g_ucbannumber = 8;
                printf("S25FL040A\n");
            } else{
                return false;
            }
            break;
        case 0xEF:
            printf("WINBOND & NX\n");
            if (ucTmpBuf[1] == 0x15 && ucTmpBuf[2] == 0x20) {
                g_ucflashtype = Flash_NX25P16;
                g_ucbannumber = 32;
                printf("NX25P16 or W25P16\n");
            } else if (ucTmpBuf[1] == 0x11 && ucTmpBuf[2] == 0x30) {
                g_ucflashtype = Flash_W25X10;
                g_ucbannumber = 2;
                printf("W25X10\n");
            } else if (ucTmpBuf[1] == 0x12 && ucTmpBuf[2] == 0x30) {
                g_ucflashtype = Flash_W25X20;
                g_ucbannumber = 4;
                printf("W25X20\n");
            } else if (ucTmpBuf[1] == 0x13 && ucTmpBuf[2] == 0x30) {
                g_ucflashtype = Flash_W25X40;
                g_ucbannumber = 8;
                printf("W25X40\n");
            } else if (ucTmpBuf[1] == 0x15 && ucTmpBuf[2] == 0x00) {
                g_ucflashtype = Flash_W25X16;
                g_ucbannumber = 32;
                printf("W25X16\n");
            } else if (ucTmpBuf[1] == 0x16 && ucTmpBuf[2] == 0x00) {
                g_ucflashtype = Flash_W25X32;
                g_ucbannumber = 64;
                printf("W25X32\n");
            } else if (ucTmpBuf[1] == 0x17 && ucTmpBuf[2] == 0x00) {
                g_ucflashtype = Flash_W25X64;
                g_ucbannumber = 128;
                printf("W25X64\n");
            } else {
                temp = read_chip_id();
                printf("ID 2:%x\n",temp);
                if (temp == 0x51) {//because some W25X20 have no ucTmpBuf[2](=0x00)
                    g_ucflashtype = Flash_W25X20;
                    g_ucbannumber = 4;
                    printf("W25X20-0\n");
                } else if(temp == 0x10) {
                    g_ucflashtype = Flash_W25X10;
                    g_ucbannumber = 2;
                    printf("W25X10-0\n");
                } else if(temp == 0x52) {//because some W25X40 have no ucTmpBuf[2](=0x00)
                    g_ucflashtype = Flash_W25X40;
                    g_ucbannumber = 8;
                    printf("W25X40-0\n");
                } else if(temp == 0x11) {//because some W25X40 have no ucTmpBuf[2](=0x00)
                    g_ucflashtype = Flash_W25P20;
                    g_ucbannumber = 4;
                    printf("W25P20\n");
                } else if(temp == 0x12) {//because some W25X40 have no ucTmpBuf[2](=0x00)
                    g_ucflashtype = Flash_W25P40;
                    g_ucbannumber = 8;
                    printf("W25P40\n");
                } else if(temp == 0x14) {
                    g_ucflashtype = Flash_NX25P16;
                    g_ucbannumber = 32;
                    printf("W25P16\n");
                } else {
                    return false;
                }
            }
            break;
        case 0xBF:
            printf("SST\n");
            if ((ucTmpBuf[1] == 0x41) & (ucTmpBuf[2] == 0x25)) {
                g_ucflashtype = Flash_SST25VF016B;
                g_ucbannumber = 32;
                printf("SST25VF016B\n");
            } else if ((ucTmpBuf[1] == 0x8D) & (ucTmpBuf[2] == 0x25)) {
                g_ucflashtype = Flash_SST25VF040B;
                g_ucbannumber = 8;
                printf("SST25VF040B\n");
            } else {
                return false;
            }
            break;
        case 0xC2:
            printf("MX\n");
            printf("JEDEC_ID1 0:%x\n",ucTmpBuf[0]);
            printf("EDEC_ID1 1:%x\n",ucTmpBuf[1]);
            printf("JEDEC_ID1 2:%x",ucTmpBuf[2]);

            if ((ucTmpBuf[1] == 0x13) & (ucTmpBuf[2] == 0x20)) {
                g_ucflashtype = Flash_MX25L4005A;
                g_ucbannumber = 8;
                printf("MX25L4005A\n");
            } else if ((ucTmpBuf[1] == 0x14) & (ucTmpBuf[2] == 0x20)) {
                g_ucflashtype = Flash_MX25L8005;
                g_ucbannumber = 16;
                printf("MX25L8005\n");
            } else if ((ucTmpBuf[1] == 0x12) & (ucTmpBuf[2] == 0x20)) {
                g_ucflashtype = Flash_MX25L2005;
                g_ucbannumber = 4;
                printf("MX25L2005\n");
            } else if((ucTmpBuf[1] == 0x15) & (ucTmpBuf[2] == 0x20)) {
                g_ucflashtype = Flash_MX25L1605;
                g_ucbannumber = 32;
                printf("MX25L1605\n");
            } else if((ucTmpBuf[1] == 0x16) & (ucTmpBuf[2] == 0x20)) {
                g_ucflashtype = Flash_MX25L3205;
                g_ucbannumber = 32;
                printf("MX25L3205\n");
            } else if((ucTmpBuf[1] == 0x17) & (ucTmpBuf[2] == 0x20)) {
                g_ucflashtype = Flash_MX25L6405;
                g_ucbannumber = 128;
                printf("MX25L6405\n");
            } else {
                return false;
            }
            break;
        //intel
        case 0x89:
            printf("INTEL\n");
            temp = read_chip_id();
            printf("INTEL ChipID=%x\n",temp);
            if (ucTmpBuf[1] == 0x11) {
                g_ucflashtype = Flash_QB25F160S;
                g_ucbannumber = 32;
                printf("INTS33_16\n");
            } else if (ucTmpBuf[1] == 0x12) {
                g_ucflashtype = Flash_QB25F320S;
                g_ucbannumber = 64;
                printf("INTS33_32\n");
            } else if (ucTmpBuf[1] == 0x13) {
                g_ucflashtype = Flash_QB25F640S;
                g_ucbannumber = 128;
                printf("INTS33_64\n");
            } else {
                return false;
            }
            break;
            //GD
        case 0xC8:
            printf("GD\n");
            if ((ucTmpBuf[1] == 0x16) && (ucTmpBuf[2] == 0x40)) {
                g_ucflashtype = Flash_GD25Q32;
                g_ucbannumber = 32;
                printf("GD25Q32\n");
            } else {
                return false;
            }
            break;
        default:
            printf("JEDEC_ID 0:%x\n",ucTmpBuf[0]);
            printf("JEDEC_ID 1:%x\n",ucTmpBuf[1]);
            printf("JEDEC_ID 2:%x\n",ucTmpBuf[2]);
            read_chip_id_buffer(ucTmpBuf);
            printf("RDID 0:%x\n",ucTmpBuf[0]);
            printf("RDID 1:%x\n",ucTmpBuf[1]);
            printf("RDID 2:%x\n",ucTmpBuf[2]);
            if ((ucTmpBuf[0] == 0x9D) && (ucTmpBuf[1] == 0x7F) && (ucTmpBuf[2] == 0x7B)) {
                printf("PMC-RDID\n");
                printf("PM25LV512A/PM25LV512\n");
                g_ucflashtype=Flash_PMC512;
                g_ucbannumber=1;
            } else {
                return false;
            }
            break;
    }
    read_chip_id_buffer(ucTmpBuf);
    printf("RDID1 0:%x\n",ucTmpBuf[0]);
    printf("RDID1 1:%x\n",ucTmpBuf[1]);
    printf("RDID1 2:%x\n",ucTmpBuf[2]);
    return true;
}

static uint8_t src_read_flash(uint16_t pCount , uint8_t bankNum) {
    uint8_t MS_U8Data;
    memcpy(&MS_U8Data,(void *)(g_ispsrcaddr+(uint32_t)pCount + (uint32_t)bankNum*0x10000), 1);
    return MS_U8Data;
}

static uint8_t sflash_rdsr(void) {
    uint8_t buffer[2];

    buffer[0] = SPI_WRITE;
    buffer[1] = g_sflash.RDSR;
    i2c_write_datas(buffer,2,NULL,0);

    buffer[0] = SPI_READ;
    i2c_read_datas(buffer,1,&buffer[1],1);

    i2c_write_stop();

    return buffer[1];
}

static bool sflash_check_sr(void) {
    uint8_t ucDelay = 10;
    int64_t count = 1000;

    while (count--) {
        if ((sflash_rdsr()&0x01) == 0x00) {
            return true;
        }
        while(ucDelay--);
    }
    return false;
}

static void sflash_wrsr(uint8_t tData) {
    uint8_t buffer[3];

    sflash_check_sr();

    // for sst sflash (sst manufacturer id=0xBF )
    if (g_sflash.manuFacturerId == 0xBF) {
        buffer[0] = SPI_WRITE;
        buffer[1] = 0x50;
        i2c_write_datas(buffer,2,NULL,0);

        i2c_write_stop();
    }

    buffer[0] = SPI_WRITE;
    buffer[1] = g_sflash.WRSR;
    buffer[2] = tData;
    i2c_write_datas(buffer,sizeof(buffer),NULL,0);

    i2c_write_stop();
}

static void sflash_wren(void) {
    uint8_t buffer[2];

    buffer[0] = SPI_WRITE;
    buffer[1] = g_sflash.WREN;

    sflash_check_sr();
    i2c_write_datas(buffer,2,NULL,0);

    i2c_write_stop();
}

static void sflash_wrdi(void) {
    uint8_t buffer[2];

    buffer[0] = SPI_WRITE;
    buffer[1] = g_sflash.WRDI;

    sflash_check_sr();
    i2c_write_datas(buffer,2,NULL,0);

    buffer[0] = SPI_STOP;
    i2c_write_datas(buffer,1,NULL,0);
}

static bool verify_omc64k(uint8_t bankNum) {
    uint8_t tData[] = {SPI_WRITE,0x03,0x00,0x00,0x00};
    bool ret = true;
    uint16_t PcCounter=0;
    uint8_t FlashReadData;

    tData[1] = g_sflash.READS;
    tData[2] = bankNum;
    tData[3] = 0;
    tData[4] = 0;

    i2c_write_datas(tData,sizeof(tData),NULL,0);
    usleep(60*1000);
    {
        i2c_commond_read();
        usleep(60*1000);
        MApi_SWI2C_UseBus(URSA_ISP_I2C_BUS_SEL);

        if (MApi_SWI2C_AccessStart(g_slaveaddr, SW_IIC_READ) == FALSE) {
            printf("verify_omc64k Error\n");
        }
        for (PcCounter = 0; PcCounter <= BANKSIZE - 2; PcCounter++) {
            FlashReadData = MApi_SWI2C_GetByte(1);
            if (FlashReadData != src_read_flash(PcCounter,bankNum)) {
                ret = false;
                printf(" flash read date error[%x],[%x],[%x]\n",PcCounter,FlashReadData,src_read_flash(PcCounter,bankNum));
                break;
            }
        }

        //last byte
        {
            FlashReadData = MApi_SWI2C_GetByte(0);
            //PcCounter++;

            if (FlashReadData != src_read_flash(PcCounter,bankNum)) {
                ret = false;
            }
        }

        // 1 bank program end
        MApi_SWI2C_Stop();
    }

    i2c_write_stop();

    return ret;
}

static void prog_sst64k(uint8_t bankNum) {
    uint8_t k;
    uint8_t tData[7];
    uint16_t PcCounter = 0;

    sflash_wrsr(0x00);
    sflash_wren();
    sflash_check_sr();

    //AAI send the address
    tData[0] = SPI_WRITE;
    tData[1] = g_sflash.PG_PROG;
    tData[2] = bankNum;
    tData[3] = 0;
    tData[4] = 0;
    tData[5] = src_read_flash(PcCounter,bankNum);
    PcCounter++;
    tData[6] = src_read_flash(PcCounter,bankNum);
    PcCounter++;

    MApi_SWI2C_UseBus(URSA_ISP_I2C_BUS_SEL);
    MApi_SWI2C_AccessStart(g_slaveaddr, SW_IIC_WRITE);

    for (k = 0; k < sizeof(tData); k++) {
        MApi_SWI2C_SendByte(tData[k]);
    }
    i2c_write_stop();
    sflash_check_sr();

    //AAI send the datas
    tData[0] = SPI_WRITE;
    tData[1] = g_sflash.PG_PROG;
    for (;PcCounter < 0xFFFE;) {
        tData[2] = src_read_flash(PcCounter,bankNum);
        PcCounter++;
        tData[3] = src_read_flash(PcCounter,bankNum);
        PcCounter++;

        MApi_SWI2C_UseBus(URSA_ISP_I2C_BUS_SEL);
        MApi_SWI2C_AccessStart(g_slaveaddr, SW_IIC_WRITE);
        for (k = 0; k < 4; k++) {
            MApi_SWI2C_SendByte(tData[k]);
        }
        i2c_write_stop();
        //sflash_check_sr();
    }
    //AAI send the remain 2 bytes
    tData[2] = src_read_flash(PcCounter,bankNum);
    PcCounter++;
    tData[3] = src_read_flash(PcCounter,bankNum);

    MApi_SWI2C_UseBus(URSA_ISP_I2C_BUS_SEL);
    MApi_SWI2C_AccessStart(g_slaveaddr, SW_IIC_WRITE);
    for (k = 0; k < 4; k++) {
        MApi_SWI2C_SendByte(tData[k]);
    }
    i2c_write_stop();
    sflash_check_sr();
    //AAI stop
    sflash_wrdi();
    sflash_check_sr();
}

static void prog_omc64k(uint8_t bankNum) {
    uint8_t k;
    uint16_t i;
    uint8_t tData[5];
    uint8_t count;
    uint16_t PcCounter = 0;
    sflash_wrsr(0x00);

    for (i = 0; i <= BLOCKNUM; i++) {
        sflash_wren();
        sflash_check_sr();

        count = BLOCKSIZE - 1;

        tData[0] = SPI_WRITE;
        tData[1] = g_sflash.PG_PROG;
        tData[2] = bankNum;
        tData[3] = i;
        tData[4] = 0;

        MApi_SWI2C_UseBus(URSA_ISP_I2C_BUS_SEL);
        MApi_SWI2C_AccessStart(g_slaveaddr, SW_IIC_WRITE);

        for (k = 0; k < sizeof(tData); k++) {
            MApi_SWI2C_SendByte(tData[k]);
        }

        while (count--) {
            MApi_SWI2C_SendByte(src_read_flash(PcCounter,bankNum));
            PcCounter++;
        }

        // 1 byte more
        MApi_SWI2C_SendByte(src_read_flash(PcCounter,bankNum));
        PcCounter++;

        //block program end
        i2c_write_stop();
    }

    i2c_write_datas(tData,2,NULL,0);
    sflash_wrdi();
    sflash_check_sr();
}

static void sflashchip_erase(void) {
    //uint8_t tData[5]={SPI_WRITE,0x00,0x00,0x00,0x00};
    uint8_t addr[2];

    sflash_wrsr(0x00);
    sflash_wren();
    sflash_check_sr();

    addr[0] = SPI_WRITE;
    addr[1] = g_sflash.CHIP_ERASE;
    i2c_write_datas(addr,2,NULL,0);

    i2c_write_stop();

    sflash_check_sr();//Vick Add
}

static void sflash_program(void) {
    uint16_t num;
    uint8_t bankTmp;

    bankTmp = g_ucbannumber;
    printf("bankTmp:%d\n",bankTmp);
    for (num = 0; num < bankTmp; num++) {
        printf("Write Bank %x\n",num);
        if (g_ucflashtype==Flash_SST25VF016B||g_ucflashtype==Flash_SST25VF040B) {
            prog_sst64k(num);
        } else {
            prog_omc64k(num);
        }
        g_curpercent = 80 - 40*(num+1)/g_ucbannumber;
    }
}

static bool sflash_verify(void) {
    uint8_t num;
    uint8_t bankTmp;

    bankTmp = g_ucbannumber;
    for (num = 0; num < bankTmp; num++) {
        printf("Verify Bank %x\n",num);
        if (verify_omc64k(num) == FALSE) {
            return false;
        }
        g_curpercent = 40 - 40*(num+1)/g_ucbannumber;
    }
    return true;
}

static bool enter_isp_mode(void) {
    // 0x4D,0x53,0x54,0x41,0x52
    uint8_t enterISPdata[]="MSTAR";
    uint8_t addr=0x20;

    // enter isp mode
    i2c_write_datas(enterISPdata,sizeof(enterISPdata)-1,NULL,0);
    i2c_read_datas(&addr,1,&addr,1);

    if (addr == 0xC0) {
        printf("enter_isp_mode->OK\n");
        return true; // enter ok
    } else {
        printf("enter_isp_mode->fail\n");
        return false; // enter fail
    }
}

static bool exit_isp_mode(void) {
    uint8_t tData = 0x24;
    uint8_t addr = 0x20;
    // exit isp mode
    printf("1.exit_isp_mode\n");
    i2c_write_datas(&tData,1,NULL,0);
    i2c_read_datas(&addr,1,&addr,1);

    if (addr == 0xC0) {
        printf("exit_isp_mode->fail\n");
        return true; // enter ok
    } else {
        printf("exit_isp_mode->OK\n");
        return false; // enter fail
    }
}

static void serialflash_enterserialdebug(void) {
    uint8_t enterSerialDebug[] = {0x53, 0x45, 0x52, 0x44, 0x42};
    i2c_write_datas2(enterSerialDebug,sizeof(enterSerialDebug),NULL,0);
}

static void serialflash_entersinglestep(void) {
    uint8_t enterSingleStep[] = {CCMD_DATA_WRITE, 0xC0, 0xC1, 0x53};
    i2c_write_datas2(enterSingleStep,sizeof(enterSingleStep),NULL,0);
}

static void serialflash_exitsinglestep(void) {
    uint8_t exitSingleStep[] = {CCMD_DATA_WRITE, 0xC0, 0xC1, 0xFF};
    i2c_write_datas2(exitSingleStep,sizeof(exitSingleStep),NULL,0);
}

static void serialflash_exitserialdebug(void) {
    uint8_t exitSerialDebug[] = {0x45};
    i2c_write_datas2(exitSerialDebug,sizeof(exitSerialDebug),NULL,0);
}

static void serialflash_wp_pullhigh(void) {
    serialflash_enterserialdebug();
    serialflash_entersinglestep();

    // Set chip to power up for register setting
    serialflash_writereg(0x00, 0, 0);
    serialflash_writereg(0x00, 0xF0, 0x02);

    // Only for GIPO P2.4
    serialflash_writereg(0x00, 0x00, 0x01);    // BK1_50[7] = 1
    serialflash_writereg(0x00, 0x50, 0x80);    // Enable register clock for gaultier

    serialflash_writereg(0x00, 0x00, 0x00);
    serialflash_writereg(0x00, 0x63, 0x3D);    // Adjust P2.4 and P2.7
                                               // TSUM chip p2.4 at 0x63[4], p2.7 at 0x63[3]
                                               // TSUMO chip p2.4 at 0x63[5],p2.7 at 0x63[2]
    serialflash_writereg(0x00, 0xc5, 0xFF);    // PWM 1 set to high (P2.7)
    serialflash_writereg(0x00, 0xc7, 0xFF);    // PWM 2 set to high (P2.4)

    // Paulo series chip
    // MCU config pin PWM0~3 default resistor setting = 1001
    serialflash_writereg(0x00, 0x00, 0x0A);
    serialflash_writereg(0x00, 0xED, 0x3F);    // PWM1 pin set to PWM BK0A_ED[7:6]=00
                                               // the other pin set to input mode
    // Lopez series chip
    // MCU config pin PWM2
    serialflash_writereg(0x1E, 0xE5, 0xC8);
    serialflash_writereg(0x1E, 0x8A, 0xC0);

    serialflash_exitsinglestep();
    serialflash_exitserialdebug();
}

static void ursa_isp_init(uint32_t MS_U32Addr, uint32_t MS_U32Size) {
    g_errorflag = FlashProgOK;
    g_eflashprogramstate = FlashProgStateInit;
    g_ispsrcaddr = MS_U32Addr;
    g_ispcodesize = MS_U32Size;
    MApi_SWI2C_Init(g_ursai2cbuscfg, g_ursai2cbusnum);
}

static int32_t ursa_isp_program(void) {
    uint16_t wErrCounter;
    if (g_errorflag != FlashProgOK) {
        return (0xF0+g_errorflag);
    }

    switch (g_eflashprogramstate) {
        case FlashProgStateInit:
            g_curpercent = 100;
            g_eflashprogramstate = FlashProgStateEnterIsp;
            break;
        case FlashProgStateEnterIsp:
            printf("1:ISP Enter\n");
            enter_isp_mode();
            usleep(500*1000);
            exit_isp_mode();
            usleep(500*1000);
            for (wErrCounter = 0;wErrCounter<ENTER_ISP_ERROR_MAX;) {//for arcing
                serialflash_wp_pullhigh();
                if (!enter_isp_mode()) {
                    wErrCounter++;
                    usleep(10*1000);
                    exit_isp_mode();
                    usleep(80*1000);
                } else {
                    break;
                }
            }
            if (wErrCounter>=ENTER_ISP_ERROR_MAX) {
                g_errorflag = FlashEnterIspModeErr;
                printf("Can not enter ISP\n");
                goto ProgEnd;
            } else {
                printf("Enter ISP Successful[%x]\n",wErrCounter);
            }
            usleep(50*1000);//Wait the Protect pin to hight.
            for (wErrCounter=0;wErrCounter<GET_ID_ERROR_MAX;) {
                if (!get_sflash_type()) {
                    wErrCounter++;
                    usleep(50*1000);
                } else {
                    break;
                }
            }
            if (wErrCounter >= GET_ID_ERROR_MAX) {
                printf("Detect SPI Flash Failed\n");
                g_ucflashtype = Flash_A25L40P;
                g_ucbannumber = 8;
            }
            g_sflash = g_supportsflash[g_ucflashtype];

            printf("Bank Number=%d\n",g_ucbannumber);
            g_ucbannumber = g_ispcodesize/0x10000;
            if (0 != (g_ispcodesize%0x10000)) {
                g_ucbannumber = g_ucbannumber+1;
            }
            printf("Program Banks=%d\n",g_ucbannumber);

            //disable write protect and set status 0x00
            sflash_wren();
            sflash_wrsr(0x00);
            g_curpercent = 95;
            g_eflashprogramstate = FlashProgStateErase;
            break;

        case FlashProgStateErase:
            //Step2:Chip Erase
            printf("2:Chip Erase\n");
            sflashchip_erase();

            //wait the flash erase finished.
            for (wErrCounter = 0; wErrCounter < g_sflash.chipEraseMaxTime; wErrCounter++) {
                if (sflash_rdsr()&0x01) {
                    if (g_sflash.chipEraseMaxTime <= (uint16_t)40) {// 0.1s
                        usleep(100*1000);
                    } else if (g_sflash.chipEraseMaxTime <= (uint16_t)200) {// 0.2s
                        usleep(100*1000);
                        wErrCounter++;
                        usleep(100*1000);
                    } else if (g_sflash.chipEraseMaxTime <= (uint16_t)600) {// 0.5s
                        usleep(100*1000);
                        wErrCounter++;
                        usleep(100*1000);
                        wErrCounter++;
                        usleep(100*1000);
                        wErrCounter++;
                        usleep(100*1000);
                        wErrCounter++;
                        usleep(100*1000);
                    } else {// 1s
                        usleep(100*1000);
                        wErrCounter++;
                        usleep(100*1000);
                        wErrCounter++;
                        usleep(100*1000);
                        wErrCounter++;
                        usleep(100*1000);
                        wErrCounter++;
                        usleep(100*1000);
                        wErrCounter++;
                        usleep(100*1000);
                        wErrCounter++;
                        usleep(100*1000);
                        wErrCounter++;
                        usleep(100*1000);
                        wErrCounter++;
                        usleep(100*1000);
                        wErrCounter++;
                        usleep(100*1000);
                    }
                } else {
                        break;
                }
            }
            printf("Wait Count:%d\n",wErrCounter);
            g_curpercent = 80;
            g_eflashprogramstate = FlashProgStateBlank;
            break;
        case FlashProgStateBlank:
            #if 0
                //Step3:Blanking Check
                printf("3:Blanking");
                if (!SFlashBlanking(0,g_sflash.size)) {
                   g_errorflag=FlashBlankingErr;
                    goto ProgEnd;
                }
            #endif
            g_curpercent = 40;
            g_eflashprogramstate = FlashProgStateProgram;
            break;
        case FlashProgStateProgram:
            //Step4:Prog Flash
            printf("4:Prog...\n");
            sflash_program();
            g_curpercent = 20;
            g_eflashprogramstate = FlashProgStateVerify;
            break;

        case FlashProgStateVerify:
            //Step5:Verify
            printf("5:Verify\n");
            sflash_wren();
            sflash_wrsr(g_sflash.LOCK);
            sflash_wrdi();

            if (!sflash_verify()) {
                g_errorflag=FlashVerifyErr;
                goto ProgEnd;
            }
            printf("Verify->OK!\n");
            g_curpercent = 10;
            g_eflashprogramstate = FlashProgStateExit;
            break;

        case FlashProgStateExit:
            //Step6:Exit ISP
            printf("6:Exit ISP\n");
            exit_isp_mode();

            g_curpercent = 0;
            g_eflashprogramstate = FlashProgStateIdle;
            break;

        case FlashProgStateIdle:
            break;
    }

    ProgEnd:
        if (g_errorflag != FlashProgOK) {
            exit_isp_mode();
            return (0xF0+g_errorflag);
        } else {
            return g_curpercent;
        }
    return 0;
}

int32_t update_ursa(char* filename) {
#if 0
    int32_t ret = -1;
    uint32_t fileSize = 0;
    uint32_t updateAddrVa = 0;
    uint32_t updateAddrPa = 0;
    uint32_t updateAddrLen = 0;
    uint8_t percent= 0;
    dictionary *ursaIni;
    char* loadUrsaBinAddr = NULL;
    ursaIni = iniparser_load(CONFIG_URSA_INI);
    if (ursaIni == NULL) {
        printf("Load /config/sys.ini failed!");
        return ret;
    }

    g_slaveaddr = (uint8_t)iniparser_getunsignedint(ursaIni, "update_ursa_configuration:SLAVE_ADDR", 0);
    g_serialdebugaddr = (uint8_t)iniparser_getunsignedint(ursaIni, "update_ursa_configuration:SERIAL_DEBUG_ADDR", 0);
    g_ursai2cbuscfg[0].padSCL = (uint16_t)iniparser_getunsignedint(ursaIni, "update_ursa_configuration:SCL_PAD", 0);
    g_ursai2cbuscfg[0].padSDA = (uint16_t)iniparser_getunsignedint(ursaIni, "update_ursa_configuration:SDA_PAD", 0);
    g_ursai2cbuscfg[0].defDelay = (uint16_t)iniparser_getunsignedint(ursaIni, "update_ursa_configuration:DELAY", 0);
    loadUrsaBinAddr = iniparser_getstr(ursaIni, "update_ursa_configuration:LOAD_URSA_BIN_ADDR");
    if (loadUrsaBinAddr == NULL) {
        printf("Load ursa addr fail!");
        iniparser_freedict(ursaIni);
        return ret;
    }
    printf("loadUrsaBinAddr:%s\n",loadUrsaBinAddr);
    printf("SLAVE_ADDR:0x%x SERIAL_DEBUG_ADDR:0x%x\n",g_slaveaddr,g_serialdebugaddr);
    printf("SCL_PAD:%d SDA_PAD:%d DELAY:%d\n",g_ursai2cbuscfg[0].padSCL,g_ursai2cbuscfg[0].padSDA,g_ursai2cbuscfg[0].defDelay);
    FILE *fp;
    if (NULL == (fp = fopen(filename,"rb"))) {
        printf("open %s fail\n",filename);
        iniparser_freedict(ursaIni);
        return ret;
    }
    fseek(fp,0,SEEK_END);
    fileSize = (uint32_t)ftell(fp);
    printf("fileSize %d\n",(int32_t)fileSize);
    fseek(fp,0L,SEEK_SET);
    mmap_info_t* mInfo = NULL;
    if (mmap_init() != 0) {
        iniparser_freedict(ursaIni);
        fclose(fp);
        return -1;
    }

    mInfo = mmap_get_info(loadUrsaBinAddr);
    if (mInfo == NULL) {
        printf("mmap_get_info %s fail\n",loadUrsaBinAddr);
        return -1;
    }
    if (mInfo->size != 0) {
        if (!MsOS_MPool_Mapping(0, mInfo->addr, mInfo->size, 1)) {
            printf("mapping read buf failed, %lx,%ld\n", mInfo->addr, mInfo->size);
            iniparser_freedict(ursaIni);
            fclose(fp);
            return -1;
        }
        updateAddrPa = mInfo->addr;
        updateAddrLen = mInfo->size;
        updateAddrVa = MsOS_MPool_PA2KSEG1(updateAddrPa);
        printf("stHwAesPA : 0x%x,stHwAesLen : 0x%x,stHwAesVA : 0x%x\n",(uint32_t)updateAddrPa,(uint32_t)updateAddrLen,(uint32_t)updateAddrVa);
    }

    if (fileSize != fread((void *)updateAddrVa,1,(int32_t)fileSize,fp)) {
        printf("fread file fail\n");
        iniparser_freedict(ursaIni);
        fclose(fp);
        return ret;
    }
    fclose(fp);
    iniparser_freedict(ursaIni);
    printf("Load upgrade file OK!~\n");
    printf("Start Upgrade Ursa!~\n");
    ursa_isp_init((uint32_t)updateAddrVa,fileSize);
    ursa_isp_program();
    do {
        percent =  ursa_isp_program();
    } while ((percent < 0xF0) && (percent != 0));
    ret = 0;
    return ret;
#endif
return 0;
}
