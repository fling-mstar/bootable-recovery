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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/vfs.h>
#include <ext4.h>
#include <fat.h>
#include <sys/statfs.h>
#include <sys/mount.h>
#include <unistd.h>
#include "mtdutils/mtdutils.h"
#include "minzip/Zip.h"
#include "crypto_common.h"
#include "crypto_auth.h"
#include "secure_updater.h"
#include "crypto_rsa.h"
#include "zlib.h"
#include <fs_mgr.h>

#define UPDATE_ZIP_FILENAME "update.zip"
#define UPDATE_SIGN_FILENAME "signfile"

#if defined(BUILD_WITH_SECURE_INFO_IN_SPI)
#include "spi_config.h"
#include "spi.h"
#define SECUREINFO_OFFSET (SPI_FLASH_SIZE-4*SPI_BLOCK_SIZE)
#define SECUREINFO_BACKUP_OFFSET  (SECUREINFO_OFFSET+SPI_BLOCK_SIZE)
#else
#define SECURE_INFO_VOLUME            "/MPOOL"
#define SECUREINFO_OFFSET (0x200000-352*0x200)
#define SECUREINFO_BACKUP_OFFSET  (SECUREINFO_OFFSET+0x6000)
#endif
#ifdef BUILD_WITH_TEE
#define NUTTX_OFFSET (0x200000-608*0x200)
#define NUTTX_BACKUP_OFFSET (NUTTX_OFFSET+0x10000)
#define NUTTX_LEN (0x10000)
#endif
static unsigned char const u8MstarDefRSAImagePublicKey[RSA_PUBLIC_KEY_LEN]={
    0x84 ,0x62 ,0x76 ,0xCC ,0xBD ,0x5A ,0x5A ,0x40 ,0x30 ,0xC0 ,0x96 ,0x40 ,0x87 ,0x28 ,0xDB ,0x85 ,
    0xED ,0xED ,0x9F ,0x3E ,0xDE ,0x4E ,0x65 ,0xE6 ,0x7B ,0x1B ,0x78 ,0x17 ,0x87 ,0x9D ,0xF6 ,0x16 ,
    0xC3 ,0xD3 ,0x27 ,0xBC ,0xB4 ,0x5A ,0x3  ,0x13 ,0x35 ,0xB0 ,0x96 ,0x5A ,0x96 ,0x41 ,0x74 ,0x4E ,
    0xB9 ,0xD1 ,0x77 ,0x96 ,0xF7 ,0x8D ,0xE2 ,0xE7 ,0x15 ,0x9  ,0x65 ,0x9C ,0x46 ,0x79 ,0xEA ,0xF0 ,
    0x91 ,0x67 ,0x35 ,0xFA ,0x69 ,0x4C ,0x83 ,0xF7 ,0xDC ,0xCF ,0x97 ,0x20 ,0xF2 ,0xA5 ,0xBA ,0x72 ,
    0x80 ,0x9D ,0x55 ,0x79 ,0x17 ,0xDC ,0x6E ,0x60 ,0xA5 ,0xE7 ,0xE  ,0x9E ,0x89 ,0x9B ,0x46 ,0x6  ,
    0x52 ,0xFC ,0x64 ,0x56 ,0x2  ,0x8  ,0x9A ,0x96 ,0x41 ,0xE2 ,0x4F ,0xDB ,0xB6 ,0x60 ,0xC3 ,0x38 ,
    0xDF ,0xF4 ,0x97 ,0x81 ,0x5D ,0x12 ,0x2  ,0xAE ,0x2B ,0x9F ,0x9  ,0x29 ,0xB9 ,0x9D ,0x51 ,0x45 ,
    0xD2 ,0x9E ,0x2B ,0xAF ,0x64 ,0xCA ,0x9A ,0x6  ,0x4E ,0x94 ,0x35 ,0x67 ,0xF7 ,0x8E ,0x4  ,0x7B ,
    0x24 ,0x38 ,0xA0 ,0xDF ,0xE7 ,0x5F ,0x1E ,0x6D ,0x29 ,0x8E ,0x30 ,0xD7 ,0x83 ,0x8C ,0xB4 ,0x41 ,
    0xD2 ,0xFD ,0xBF ,0x5B ,0x18 ,0xCA ,0x50 ,0xD1 ,0x27 ,0xD1 ,0xF6 ,0x7D ,0x54 ,0x3E ,0x80 ,0x5F ,
    0x20 ,0xDC ,0x88 ,0x82 ,0xCF ,0xBE ,0xE1 ,0x46 ,0x2A ,0xD6 ,0x63 ,0xB9 ,0xB9 ,0x9D ,0xA3 ,0xC7 ,
    0x68 ,0x3E ,0x48 ,0xCE ,0x6A ,0x62 ,0x6F ,0xD1 ,0x6A ,0xC3 ,0xB6 ,0xDE ,0xF3 ,0x39 ,0x25 ,0xEC ,
    0xF6 ,0x79 ,0x20 ,0xB5 ,0xF2 ,0x30 ,0x25 ,0x6E ,0x99 ,0xAE ,0x39 ,0x56 ,0xDA ,0xAF ,0x83 ,0xD6 ,
    0xB8 ,0x49 ,0x15 ,0x78 ,0x81 ,0xCC ,0x3C ,0x4F ,0x66 ,0x5D ,0x95 ,0x7E ,0x31 ,0xD4 ,0x37 ,0x2A ,
    0xBE ,0xFC ,0xB4 ,0x66 ,0xF8 ,0x91 ,0x1  ,0xA  ,0x53 ,0x3C ,0x3C ,0xAB ,0x86 ,0xB9 ,0x80 ,0xB7 ,
    0x0  ,0x1  ,0x0  ,0x1};

///-----------------------------------------------------------------------------
/// get data from secure partition
/// @param  pBufferAddr \b IN: the address to store data
/// @param  offset \b IN: offset to start read
/// @param  Length \b IN: data length to read
/// @param  v \b IN: device to read
/// @return If sucess, return 0. If failed, return -1.
///-----------------------------------------------------------------------------
static int get_secure_info_mtd(const char *pBufferAddr,int offset, int Length, const Volume* v)
{
    DEBUG("not Implement\n");
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
static int set_secure_info_mtd(const char *pBufferAddr,int offset, int Length, const Volume* v)
{
    DEBUG("not Implement\n");
    return 0;
}

///-----------------------------------------------------------------------------
/// get data from secure partition
/// @param  pBufferAddr \b IN: the address to store data
/// @param  offset \b IN: offset to start read
/// @param  Length \b IN: data length to read
/// @param  v \b IN: device to read
/// @return If sucess, return 0. If failed, return -1.
///-----------------------------------------------------------------------------
static int get_secure_info_block(const char *pBufferAddr,int offset, int Length, const Volume* v)
{
    DEBUG("IN\n");
    if (NULL == pBufferAddr || 0 == Length || NULL == v) {
        return -1;
    }
    FILE* f = fopen(v->blk_device, "rb");
    if (NULL == f) {
        DEBUG("Can't open %s\n(%s)\n",v->blk_device, strerror(errno));
        return -1;
    }
    fseek(f,offset,SEEK_SET);
    int count = fread((void*)pBufferAddr, Length, 1, f);
    if (1 != count) {
        DEBUG("Failed reading %s\n(%s)\n",v->blk_device, strerror(errno));
        return -1;
    }
    if (0 != fclose(f)) {
        DEBUG("Failed closing %s\n(%s)\n",v->blk_device, strerror(errno));
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
static int set_secure_info_block(const char *pBufferAddr, int offset ,int Length, const Volume* v)
{
    DEBUG("IN\n");
    if (NULL == pBufferAddr || 0 == Length || NULL == v) {
        return -1;
    }

    FILE* f = fopen(v->blk_device, "wb");
    if (NULL == f) {
        DEBUG("Can't open %s\n(%s)\n",v->blk_device, strerror(errno));
        return -1;
    }
    fseek(f,offset,SEEK_SET);
    int count = fwrite(pBufferAddr, Length, 1, f);
    if (1 != count) {
        DEBUG("Failed writing %s\n(%s)\n",v->blk_device, strerror(errno));
        return -1;
    }
    if (0 != fclose(f)) {
        DEBUG("Failed closing %s\n(%s)\n",v->blk_device, strerror(errno));
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
    #if defined(BUILD_WITH_SECURE_INFO_IN_SPI)
    ret = spi_rdc(pBufferAddr,offset,Length);
    #else
    Volume*  v = volume_for_path(SECURE_INFO_VOLUME);
    if (v == NULL) {
        DEBUG("no volume for %s!\n",SECURE_INFO_VOLUME);
        return -1;
    }
    if (0 == strcmp(v->fs_type, "mtd")) {
        ret = get_secure_info_mtd(pBufferAddr,offset,Length,v);
    } else if (0 == strcmp(v->fs_type, "emmc")) {
        ret = get_secure_info_block(pBufferAddr,offset,Length,v);
        if (0 != ret) {
            DEBUG("Load data from %s err\n",v->blk_device);
        }
    } else {
        DEBUG("unknown MPOOL partition fs_type:%s\n",v->fs_type);
    }
    #endif
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
    Volume*  v = volume_for_path(SECURE_INFO_VOLUME);
    if (NULL == v) {
        DEBUG("Cannot load volume %s\n",SECURE_INFO_VOLUME);
    }
    if (0 == strcmp(v->fs_type, "mtd")) {
        ret = set_secure_info_mtd(pBufferAddr,offset,Length,v);
    } else if (0 == strcmp(v->fs_type, "emmc")) {
        ret = set_secure_info_block(pBufferAddr,offset,Length, v);
        if (0 != ret) {
            DEBUG("Write data to %s error\n",v->blk_device);
        }
    } else {
        DEBUG("unknown MPOOL partition fs_type:%s\n",v->fs_type);
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


///-----------------------------------------------------------------------------
/// chose the usb disk path properly
/// @param  ota_size \b IN: the size of ota
/// @param  updatefile \b IN: new path returned to caller
/// @return If sucess, return 0. If failed, return -1.
///-----------------------------------------------------------------------------
static int scan_usb(int ota_size,char* updatefile)
{
    if (NULL == updatefile || 0 >= ota_size) {
        DEBUG("parameters error\n");
    }
    DIR *dirp;
    struct dirent *dp;
    FILE *fp;
    long long usb_free = 0;
    char path[54];
    struct fat_bootinfo fat_info;
    char devpath[30];
    dirp = opendir("/dev/block");
    if (NULL == dirp) {
        DEBUG("Can't open /dev/block\n");
        return -1;
    }

    do {
        if (NULL == (dp = readdir(dirp))) {
            DEBUG("read dev/block fail\n\n");
            continue;
        }
        if (strcmp(dp->d_name,".") == 0 || strcmp(dp->d_name,"..") == 0) {
            continue;
        }
        if ((4 == strlen(dp->d_name)) &&('s' == dp->d_name[0]) && ('d' == dp->d_name[1])) {
            memset(devpath,0,sizeof(devpath));
            strcpy(devpath,"/dev/block/");
            strcat(devpath,dp->d_name);
            if (NULL == (fp = fopen(devpath,"r+"))) {
                continue;
            }
            fread(&fat_info,sizeof(struct fat_bootinfo), 1, fp);
            fclose(fp);
            usb_free = (long long)fat_info.info.free_clusters * (long long)fat_info.boot.sector_size * (long long)fat_info.boot.sectors_per_cluster;
            DEBUG("usb_free:%lld\n", usb_free);
            if (usb_free > ota_size) {
                memset(path,0,sizeof(path));
                strcpy(path,"/mnt/usb/");
                strcat(path,dp->d_name);
                //if path not existed , create  path
                if(access(path,F_OK)!=0) {
                    mkdir(path,S_IRWXU);
                }
                //try to mount devpath on path
                if (mount(devpath, path, "vfat", MS_NOATIME|MS_NODEV|MS_NODIRATIME, "")<0&&errno!=EBUSY) {
                    continue;
                }
                DEBUG("usb disk path:%s\n",path);
                strcpy(updatefile,path);
                strcat(updatefile,"/");
                closedir(dirp);
                return 0;
            }
        }
    }while(1);
    closedir(dirp);
    return -1;
}

///-----------------------------------------------------------------------------
/// secure verify OTA package
/// @param  zip \b IN: the zip file
/// @param  path \b IN: the path of secure ota, such as /mnt/usb/sda1/update_signed.zip
/// @param  updatefilepath \b OUT: the secure ota file to decompression path.
/// @return If sucess, return 0. If failed, return -1.
///-----------------------------------------------------------------------------
int  secure_verify(ZipArchive* zip,const char* path,char* updatefilepath)
{
    DEBUG("secure upgrade verify...\n");
    int file_length = 0;
    int fd;
    if (NULL == zip || NULL == path || NULL == updatefilepath) {
        DEBUG("parameters error\n");
        return -1;
    }
    char updatefile[128];
    char signfile[128];
    char command[256];

    memset(updatefile,0,128);
    memset(signfile,0,128);

    Volume* data_volume = volume_for_path("/data");
    if (NULL == data_volume) {
        DEBUG("no device of /data\n");
        return -1;
    }
    FILE *fp;
    fp = fopen(path,"rb");
    fseek(fp,0,SEEK_END);

    file_length = ftell(fp);
    DEBUG("the size of update_signed.zip is :%d\n", file_length);
    fclose(fp);

    //check free size of data partition or usb disd
    struct ext4_super_block data_ext4;
    memset(&data_ext4,0,sizeof(struct ext4_super_block));
    double datafree = 0;
    fd = open(data_volume->blk_device,O_RDONLY);
    if (-1 == fd) {
        DEBUG("failed to open %s\n",data_volume->blk_device);
        return -1;
    }
    lseek(fd,1024,SEEK_SET);
    read(fd,&data_ext4,sizeof(struct ext4_super_block));
    close(fd);
    datafree = (data_ext4.s_free_blocks_count_lo) * (data_ext4.s_log_block_size)*2*1024;
    if (datafree > file_length) {
        strcpy(updatefile,"/data/");
        //mount  /data
        if (mount(data_volume->blk_device, data_volume->mount_point, data_volume->fs_type, MS_NOATIME | MS_NODEV | MS_NODIRATIME, "") < 0 && errno != EBUSY) {
             DEBUG("fail to mount %s on %s\n",data_volume->blk_device,data_volume->mount_point);
             return -1;
        }
    }
    else {
        //try USB disk
        if (0 != scan_usb(file_length,updatefile)) {
            return -1;
        }
    }
    strcpy(signfile,updatefile);
    strcat(updatefile,UPDATE_ZIP_FILENAME);
    strcat(signfile,UPDATE_SIGN_FILENAME);
    const ZipEntry* updatefile_entry =mzFindZipEntry(zip,UPDATE_ZIP_FILENAME);
    const ZipEntry* signfile_entry =mzFindZipEntry(zip,UPDATE_SIGN_FILENAME);
    if (updatefile_entry == NULL||signfile_entry==NULL) {
        DEBUG("no %s or %s signfile in %s\n", UPDATE_ZIP_FILENAME, UPDATE_SIGN_FILENAME, path);
        return -1;
    }
    DEBUG("create file %s,%s\n", updatefile, signfile);
    unlink(updatefile);
    unlink(signfile);
    int updatefd = creat(updatefile, 0755);
    int signfd = creat(signfile, 0755);
    if (updatefd < 0||signfd < 0) {
        DEBUG("Can't create %s or %s\n", updatefile, signfile);
        return -1;
    }
    bool updateok = mzExtractZipEntryToFile(zip, updatefile_entry, updatefd);
    bool signok = mzExtractZipEntryToFile(zip, signfile_entry, signfd);
    DEBUG("minzip %s %s finished...\n", updatefile, signfile);
    close(updatefd);
    close(signfd);
    if (!updateok||!signok) {
        DEBUG("Can't copy %s or %s\n", UPDATE_ZIP_FILENAME, UPDATE_SIGN_FILENAME);
        return -1;
    }
    //read updatefile
    fp = fopen(updatefile,"rb");
    fseek(fp,0,SEEK_END);
    unsigned int filesize = ftell(fp);

    unsigned char* filedata = (unsigned char*)malloc(filesize);
    if (NULL == filedata) {
        DEBUG("failed to malloc\n");
        return -1;
    }
    fseek(fp,0,SEEK_SET);
    if (filesize != fread(filedata,1,filesize,fp)) {
        DEBUG("failed to read %s\n",updatefile);
        free(filedata);
        return -1;
    }
    fclose(fp);

    //read signfile
    fp = fopen(signfile,"rb");
    unsigned char *signture = (unsigned char*)malloc(SIGNATURE_LEN);
    if (SIGNATURE_LEN != fread(signture,1,SIGNATURE_LEN,fp)) {
        DEBUG("failed to read %s\n",updatefile);
        free(filedata);
        free(signture);
        return -1;
    }
    fclose(fp);

    unsigned char *RSAKey = (unsigned char*)u8MstarDefRSAImagePublicKey;
    int ret =  Secure_doAuthentication(RSAKey,RSAKey+RSA_PUBLIC_KEY_N_LEN,signture,filedata,filesize);
    free(filedata);
    free(signture);
    if (-1 == ret) {
        return -1;
    }
    // return path to caller
    strcpy(updatefilepath,updatefile);
    sync();
    DEBUG("secure upgrade verify finished...\n");
    unlink(signfile);
    return 0;
}

///-----------------------------------------------------------------------------
/// print secureinfo
/// @param  pSecureInfo \b IN: secureinfo data to print
/// @param  partition \b IN: boot or recovery
///-----------------------------------------------------------------------------
static void print_secure_info(SECURITY_INFO *pSecureInfo,char* partition)
{
    if (NULL == pSecureInfo|| NULL == partition) {
        DEBUG("parameters error\n");
        return;
    }
    if (strcmp(partition,"recovery")==0) {
        DEBUG("recovery.data:\n");
        mem_dump((int)&pSecureInfo->data.Recovery,sizeof(_SUB_SECURE_INFO));
        DEBUG("recovery.data_interleave:\n");
        mem_dump((int)&pSecureInfo->data_interleave.Recovery,sizeof(_SUB_SECURE_INFO));
    } else if(strcmp(partition,"boot")==0) {
        DEBUG("boot.data:\n");
        mem_dump((int)&pSecureInfo->data.Boot,sizeof(_SUB_SECURE_INFO));
        DEBUG("boot.data_interleave:\n");
        mem_dump((int)&pSecureInfo->data_interleave.Boot,sizeof(_SUB_SECURE_INFO));
#ifdef BUILD_WITH_TEE
    } else if(strcmp(partition,"tee")==0) {
        DEBUG("tee.data:\n");
        mem_dump((int)&pSecureInfo->data.tee,sizeof(_SUB_SECURE_INFO));
        DEBUG("tee.data_interleave:\n");
        mem_dump((int)&pSecureInfo->data_interleave.tee,sizeof(_SUB_SECURE_INFO));
#endif
    } else {
        DEBUG("partition error:%s\n",partition);
    }
}


///-----------------------------------------------------------------------------
/// upgrade secureinfo for partition
/// @param  partition \b IN: partition to upgrade secureinfo
/// @param  filename \b IN: file updated secureinfo
/// @ If success, return 0; else return -1;
///-----------------------------------------------------------------------------
int update_secure_info(char* partition,char* filename)
{
    if (partition == NULL || filename == NULL) {
        DEBUG("parameters error\n");
        return -1;
    }
    DEBUG("update secure info partition:%s filename:%s\n",partition,filename);
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
    if (NULL == (fp = fopen(filename,"rb"))) {
        DEBUG("open %s fail\n(%s)\n",filename, strerror(errno));
        return -1;
    }
    fread(&SubSecureInfo, 1, sizeof(SUB_SECURE_INFO),fp);
    fclose(fp);
    if (0 == strcmp(partition,"recovery")) {
        memcpy(&(SecureInfo.data.Recovery),&SubSecureInfo.sInfo,sizeof(_SUB_SECURE_INFO));
        memcpy(&(SecureInfo.data_interleave.Recovery),&SubSecureInfo.sInfo_Interleave,sizeof(_SUB_SECURE_INFO));
    } else if(0 == strcmp(partition,"boot")) {
        memcpy(&(SecureInfo.data.Boot),&SubSecureInfo.sInfo,sizeof(_SUB_SECURE_INFO));
        memcpy(&(SecureInfo.data_interleave.Boot),&SubSecureInfo.sInfo_Interleave,sizeof(_SUB_SECURE_INFO));
#ifdef BUILD_WITH_TEE
    } else if(0 == strcmp(partition,"tee")) {
        memcpy(&(SecureInfo.data.tee),&SubSecureInfo.sInfo,sizeof(_SUB_SECURE_INFO));
        memcpy(&(SecureInfo.data_interleave.tee),&SubSecureInfo.sInfo_Interleave,sizeof(_SUB_SECURE_INFO));
#endif
    } else {
        DEBUG("partition name error :%s\n",partition);
        return -1;
    }
    print_secure_info(&SecureInfo,partition);
    ret = save_secure_info(&SecureInfo);
    if (ret == -1) {
        DEBUG("Failed to update secureinfo\n");
    }
    DEBUG("update secureinfo for %s finished\n",partition);
    return 0;
}

#ifdef BUILD_WITH_TEE
int NConfigSave(U8* pBufferAddr, U32 u32NConfigLen)
{
    int ret=-1;
    int ret_bk =-1;
    unsigned int u32NConfigOffset = 0;
    unsigned int u32NConfigBkOffset = 0;
    unsigned int u32NConfigSize=0;
    unsigned int NConfig_raw_data_size=0;
    DEBUG("IN\n");

    if (NULL == pBufferAddr) {
        DEBUG("The input parameter pBufferAddr' is a null pointer\n");
        return -1;
    }

    u32NConfigOffset = NUTTX_OFFSET;
    u32NConfigBkOffset = NUTTX_BACKUP_OFFSET;
    u32NConfigSize = NUTTX_LEN;
    #define NCONFIG_RAW_DATA_SIZE (u32NConfigSize-4)

    if (u32NConfigLen>=NCONFIG_RAW_DATA_SIZE) {
        DEBUG("NCofngi length is larger than %x\n",NConfig_raw_data_size);
        return -1;
    }

    unsigned long Crc32=0;
    // update CRC
    Crc32 = crc32(0, pBufferAddr,NConfig_raw_data_size);
    DEBUG("Crc32 =%lx\n",Crc32);
    memcpy(pBufferAddr+NConfig_raw_data_size, &Crc32, sizeof(Crc32));
    DEBUG("Burn NConfig to blcok\n");
    #if defined(BUILD_WITH_SECURE_INFO_IN_SPI)
    ret = spi_wrc(pBufferAddr,u32NConfigOffset,u32NConfigSize);
    #else
    Volume*  v = volume_for_path(SECURE_INFO_VOLUME);
    if (NULL == v) {
        DEBUG("Cannot load volume %s\n",SECURE_INFO_VOLUME);
    }
    DEBUG("volume dev %s\n",v->blk_device);
    if (0 == strcmp(v->fs_type, "mtd")) {
        ret = set_secure_info_mtd(pBufferAddr,u32NConfigOffset,u32NConfigSize,v);
    } else if (0 == strcmp(v->fs_type, "emmc")) {
        ret = set_secure_info_block((unsigned int)pBufferAddr,u32NConfigOffset,u32NConfigSize, v);
        DEBUG("Burn NConfig to  backup blcok\n");
        ret_bk = set_secure_info_block((unsigned int)pBufferAddr,u32NConfigBkOffset,u32NConfigSize, v);
        if (0 == ret || 0 == ret_bk) {
            DEBUG("ret : %d , ret_bk : %d \n",ret,ret_bk);
            DEBUG("OK\n");
            ret = 0;
        } else {
            ret = -1;
            DEBUG("NConfig Save fail\n");
        }
    }
    #endif
    DEBUG("OK\n");
    return ret;
    #undef NCONFIG_RAW_DATA_SIZE
}

///-----------------------------------------------------------------------------
/// upgrade nuttx
/// @param  config   \b IN: MPOOL partition to which area
/// @param  filename \b IN: file updated nuttx_config
/// @ If success, return 0; else return -1;
///-----------------------------------------------------------------------------
int update_nuttx_config(char* config,char* filename)
{
    if (NULL == config || NULL == filename) {
        DEBUG("parameters error\n");
        return -1;
    }

    if (0 != strcmp(config,"NuttxConfig")) {
        DEBUG("config is error :%s\n",config);
        return -1;
    }

    FILE *fp;
    long length = 0;
    char buffer[NUTTX_LEN];

    memset(buffer, 0, NUTTX_LEN);

    if (NULL == (fp = fopen(filename,"rb"))) {
        DEBUG("open %s fail\n(%s)\n",filename, strerror(errno));
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    length = ftell(fp);
    printf("the length for %s is %d\n", filename, length);

    if (length > NUTTX_LEN) {
        printf("the %s is more than its allotted space!\n");
        return -1;
    }
    fseek(fp, 0, SEEK_SET);
    fread(buffer, 1, length,fp);
    fclose(fp);
    ST_SIGN_CONFIG* pstHeaderInfo = NULL;
    U32 u32SignConfigPhyAddr=buffer;
    U32 u32SignConfigLen=0;
    U32 u32SigPhyAddr=0;
    U32 u32ConfigPhyAddr=0;
    U32 u32SigLen=256;
    U32 u32ConfigLen=0;
    DEBUG("u32SignConfigPhyAddr =%x \n",u32SignConfigPhyAddr);

    pstHeaderInfo=(ST_SIGN_CONFIG*)u32SignConfigPhyAddr;
    u32SigPhyAddr=u32SignConfigPhyAddr+pstHeaderInfo->stSignature.Offset;
    u32SigLen=pstHeaderInfo->stSignature.Length;
    u32ConfigPhyAddr=u32SignConfigPhyAddr+pstHeaderInfo->stConfig.Offset;
    u32ConfigLen=pstHeaderInfo->stConfig.Length;
    #define HEADER_LEN 256
    u32SignConfigLen=HEADER_LEN+u32SigLen+u32ConfigLen;

    DEBUG("****u32SignConfigPhyAddr : %x \n",u32SignConfigPhyAddr);
    DEBUG("****u32SigPhyAddr : %x \n",u32SigPhyAddr);
    DEBUG("****u32SigLen : %x \n",u32SigLen);
    DEBUG("****u32ConfigPhyAddr : %x \n",u32ConfigPhyAddr);
    DEBUG("****u32ConfigLen : %x \n",u32ConfigLen);
    int ret = NConfigSave(buffer, u32SignConfigLen);
    if(0 != ret) {
        DEBUG("NConfig Save fail\n");
        return -1;
    }

    DEBUG("update nuttx config finished\n");
    return 0;
}
#endif