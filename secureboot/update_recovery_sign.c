//<MStar Software>
//******************************************************************************
// MStar Software
// Copyright (c) 2010 - 2012 MStar Semiconductor, Inc. All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include "secure_updater.h"

#if defined(BUILD_WITH_SECURE_INFO_IN_SPI)
#include "spi_config.h"
#include "spi.h"
#define SECUREINFO_OFFSET (SPI_FLASH_SIZE-4*SPI_BLOCK_SIZE)
#define SECUREINFO_BACKUP_OFFSET  (SECUREINFO_OFFSET+SPI_BLOCK_SIZE)
#else
#define SECURE_INFO_VOLUME            "/dev/block/platform/mstar_mci.0/by-name/MPOOL"
#define SECUREINFO_OFFSET (0x200000-352*0x200)
#define SECUREINFO_BACKUP_OFFSET  (SECUREINFO_OFFSET+0x6000)
#endif


static uint32_t crc32_tab[] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};


static uint32_t crc32(uint32_t crc, const void *buf, size_t size)
{
    const uint8_t *p;
    p = buf;
    crc = crc ^ ~0U;
    while (size--)
        crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    return crc ^ ~0U;
}


///-----------------------------------------------------------------------------
/// get data from secure partition
/// @param  pBufferAddr \b IN: the address to store data
/// @param  offset \b IN: offset to start read
/// @param  Length \b IN: data length to read
/// @param  v \b IN: device to read
/// @return If sucess, return 0. If failed, return -1.
///-----------------------------------------------------------------------------
static int get_secure_info_block(const char *pBufferAddr,int offset, int Length, char *dev)
{
    DEBUG("IN\n");
    if (NULL == pBufferAddr || 0 == Length || NULL == dev) {
        return -1;
    }
    FILE* f = fopen(dev, "rb");
    if (NULL == f) {
        DEBUG("Can't open %s\n(%s)\n",dev, strerror(errno));
        return -1;
    }
    fseek(f,offset,SEEK_SET);
    int count = fread((void*)pBufferAddr, Length, 1, f);
    if (1 != count) {
        DEBUG("Failed reading %s\n(%s)\n", dev, strerror(errno));
        return -1;
    }
    if (0 != fclose(f)) {
        DEBUG("Failed closing %s\n(%s)\n", dev, strerror(errno));
        return -1;
    }
    DEBUG("OK\n");
    return 0;
}

///-----------------------------------------------------------------------------
/// write data to secure partition
/// @param  pBufferAddr \b IN: the buffer of data to write
/// @param  offset \b IN: offset to start write
/// @param  Length \b IN: data length to write
/// @param  v \b IN: device to write
/// @return If sucess, return 0. If failed, return -1.
///-----------------------------------------------------------------------------
static int set_secure_info_block(const char *pBufferAddr, int offset ,int Length, char *dev)
{
    DEBUG("IN\n");
    if (NULL == pBufferAddr || 0 == Length || NULL == dev) {
        return -1;
    }

    FILE* f = fopen(dev, "wb");
    if (NULL == f) {
        DEBUG("Can't open %s\n(%s)\n",dev, strerror(errno));
        return -1;
    }
    fseek(f,offset,SEEK_SET);
    int count = fwrite(pBufferAddr, Length, 1, f);
    if (1 != count) {
        DEBUG("Failed writing %s\n(%s)\n",dev, strerror(errno));
        return -1;
    }
    if (0 != fclose(f)) {
        DEBUG("Failed closing %s\n(%s)\n",dev, strerror(errno));
        return -1;
    }
    DEBUG("OK\n");
    return 0;
}


///-----------------------------------------------------------------------------
/// read data to secure partition
/// @param  pBufferAddr \b IN: the buffer of data to read
/// @param  offset \b IN: offset to start read
/// @param  Length \b IN: data length to read
/// @return If sucess, return 0. If failed, return -1.
///-----------------------------------------------------------------------------
static int read_secure_info(char * pBufferAddr, int offset, int Length)
{
    DEBUG("IN\n");
    int ret = -1;
    ret = get_secure_info_block(pBufferAddr,offset,Length,SECURE_INFO_VOLUME);
    if (0 != ret) {
       DEBUG("Load data from %s err\n",SECURE_INFO_VOLUME);
    }
    DEBUG("OK\n");
    return ret;
}

///-----------------------------------------------------------------------------
/// write data to secure partition
/// @param  pBufferAddr \b IN: the buffer of data to write
/// @param  offset \b IN: offset to start write
/// @param  Length \b IN: data length to write
/// @return If sucess, return 0. If failed, return -1.
///-----------------------------------------------------------------------------
static int write_secure_info(char * pBufferAddr,int offset,int Length)
{
    DEBUG("IN\n");
    int ret = -1;
    #if defined(BUILD_WITH_SECURE_INFO_IN_SPI)
    ret = spi_wrc(pBufferAddr,offset,Length);
    #else
    ret = set_secure_info_block(pBufferAddr,offset,Length, SECURE_INFO_VOLUME);
    if (0 != ret) {
        DEBUG("Write data to %s error\n",SECURE_INFO_VOLUME);
    }
    #endif
    DEBUG("OK\n");
    return ret;
}

///-----------------------------------------------------------------------------
/// load SecureInfo struct from flash.
/// @If success, return 0; else return -1;
///-----------------------------------------------------------------------------
static int load_secure_info(SECURITY_INFO * pSecureInfo)
{
    DEBUG("IN\n");
    int ret;
    memset(pSecureInfo, 0, sizeof(SECURITY_INFO));
    DEBUG("SECUREINFO_OFFSET:0x%x\n",SECUREINFO_OFFSET);
    DEBUG("SECUREINFO_BACKUP_OFFSET:0x%x\n",SECUREINFO_BACKUP_OFFSET);
    DEBUG("start to Load Signature from first section\n");
    ret = read_secure_info((char*)pSecureInfo,SECUREINFO_OFFSET,sizeof(SECURITY_INFO));
    if  (0 == ret && pSecureInfo->crc == crc32(0, (unsigned char const *)&pSecureInfo->data,2*sizeof(_SECURITY_INFO_DATA))) {
        DEBUG("Load Signature from first section OK\n");
    } else {
        DEBUG("start to Load Signature from backup\n");
        ret = read_secure_info((char*)pSecureInfo,SECUREINFO_BACKUP_OFFSET,sizeof(SECURITY_INFO));
        if(0 == ret && pSecureInfo->crc == crc32(0, (unsigned char const *)&pSecureInfo->data,2*sizeof(_SECURITY_INFO_DATA))) {
            DEBUG("start to Load Signature from backup OK\n");
        }
        else {
            DEBUG("load signature from backup ERROR\n");
            ret = -1;
        }
    }
    DEBUG("OK\n");
    return ret;
}


///-----------------------------------------------------------------------------
/// save SecureInfo struct into flash.
/// @If success, return 0; else return -1;
///-----------------------------------------------------------------------------
static int save_secure_info(SECURITY_INFO * pSecureInfo)
{
    DEBUG("save SecureInfo\n");
    int ret1,ret2;

    //calculate the crc of struct SECURITY_INFO
    pSecureInfo->crc = crc32(0L, (unsigned char*)&(pSecureInfo->data), (2*sizeof(_SECURITY_INFO_DATA)));
    ret1 = write_secure_info((char*)pSecureInfo,SECUREINFO_OFFSET, sizeof(SECURITY_INFO));
    ret2 = write_secure_info((char*)pSecureInfo,SECUREINFO_BACKUP_OFFSET, sizeof(SECURITY_INFO));
    if (0 != ret1 || 0 != ret2) {
        DEBUG("Write data to flash error\n");
        return -1;
    } else {
        DEBUG("save SecureInfo OK\n");
        return 0;
    }
}
int main(int argc, char **argv)
{
    printf("store secure info partition:%s filename:%s\n",argv[1],argv[2]);
    SECURITY_INFO SecureInfo;
    SUB_SECURE_INFO SubSecureInfo;
    memset(&SecureInfo,0,sizeof(SecureInfo));
    memset(&SubSecureInfo,0,sizeof(SubSecureInfo));
    int ret = -1;
    ret = load_secure_info(&SecureInfo);
    if (ret == -1) {
        return -1;
    }
    FILE *fp;
    if (NULL == (fp = fopen(argv[2],"rb"))) {
        printf("open %s fail\n(%s)\n",argv[2], strerror(errno));
        return -1;
    }
    fread(&SubSecureInfo, 1, sizeof(SUB_SECURE_INFO),fp);
    fclose(fp);
    if (0 == strcmp(argv[1],"recovery")) {
        memcpy(&(SecureInfo.data.Recovery),&SubSecureInfo.sInfo,sizeof(_SUB_SECURE_INFO));
        memcpy(&(SecureInfo.data_interleave.Recovery),&SubSecureInfo.sInfo_Interleave,sizeof(_SUB_SECURE_INFO));
    } else if(0 == strcmp(argv[1],"boot")) {
        memcpy(&(SecureInfo.data.Boot),&SubSecureInfo.sInfo,sizeof(_SUB_SECURE_INFO));
        memcpy(&(SecureInfo.data_interleave.Boot),&SubSecureInfo.sInfo_Interleave,sizeof(_SUB_SECURE_INFO));
    } else {
        printf("partition name error :%s\n",argv[1]);
        return -1;
    }
    ret = save_secure_info(&SecureInfo);
    if (ret == -1) {
        printf("Failed to store secureinfo\n");
    }
	printf("Failed to store secureinfo\n");
	printf("Failed to store secureinfo\n");
	printf("Failed to store secureinfo\n");
	printf("Failed to store secureinfo\n");
	printf("Failed to store secureinfo\n");
	printf("Failed to store secureinfo\n");
    printf("update secureinfo for %s finished\n",argv[1]);
    return 0;
}
