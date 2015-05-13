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
//    supplied together with third party's software and the use of MStar
//    Software may require additional licenses from third parties.
//    Therefore, you hereby agree it is your sole responsibility to separately
//    obtain any and all third party right and license necessary for your use of
//    such third party's software.
//
// 3. MStar Software and any modification/derivatives thereof shall be deemed as
//    MStar's confidential information and you agree to keep MStar's
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
//    MStar Software in conjunction with your or your customer's product
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
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <fs_mgr.h>
#include <dirent.h>

#include "ui.h"
#include "md5.h"
#include "common.h"
#include "install.h"
#include "roots.h"
#include "backup_restore.h"

extern RecoveryUI* ui;

extern int get_menu_selection(const char* const * headers, const char* const * items,
  int menu_only, int initial_selection, Device* device);


#ifndef DEFINE_BACKUP_OPERATION_DATA
#define DEFINE_BACKUP_OPERATION_DATA          1
#endif

//#define MAKE_BLOCK_TGZ    1
#define MAKE_BLOCK_GZIP    1

#if MAKE_BLOCK_GZIP
#warning "make backup file type .gz! "
#define MAKE_PREFIX_TAG                       "busybox dd"
#define MAKE_POSTFIX_TAG                      "gz"
#define GZIP_PREFIX_TAG                       "busybox gzip"

#elif    MAKE_BLOCK_TGZ
#warning "make backup file type .tgz! "
#define MAKE_PREFIX_TAG                       "busybox tar"
#define MAKE_POSTFIX_TAG                      "tgz"
#define GZIP_PREFIX_TAG                       "busybox gzip"

#else
#warning "make backup file type .img! "
#define MAKE_PREFIX_TAG                       "busybox cat"
#define MAKE_POSTFIX_TAG                      "img"
#endif

#define SDCARD_RET_VAL                        0
#define UDISK_RET_VAL                         1
#define SDCARD_MOUNT_POINT                    "/sdcard"
#define SDCARD_MOUNT_POINT2                   "/mnt/usb/mmcblka1"
#define UDISK_MOUNT_POINT                     "/mnt/usb/sda1"
#define DEFAULT_BACKUP_MEDIUM                 SDCARD_MOUNT_POINT

#define FILE_NAME                             "MStaredison_recovery"
#define WRITE_MD5SUM_CMD                      "busybox echo"
#define MD5SUM_POSTFIX_TAG                    "md5"
#define MD5_CODE_LENGTH                       32
#define MD5_BUFF_LENGTH                       (MD5_CODE_LENGTH+2)

#define ENABLE_PROGRESS_DISPLAY_IN_SYSTEM_BACKUP_RESTORE    1

static __volume_backup volume_backup_table[BACKUP_VOLUME_NUMBER];

time_t time1;
struct tm *time2;
static char time_slice[32];
static char folder_head[64];
static char folder_name[256];
static int recovery_folder_find = 2;
static char recovery_folder[64];
static char md5code_new[MD5_BUFF_LENGTH]={'\0'};
static char md5code_old[MD5_BUFF_LENGTH]={'\0'};

static int tmp_is_mounted = 0;
static int sdcard_is_mounted = 0;
static int udisk_is_mounted = 0;
static char *BACKUP_MEDIUM = DEFAULT_BACKUP_MEDIUM;

static int
ensure_backup_path( Device* device) {
    char dev_string[128][20];
    char tmp_label[128];
    char tmp_uuid[128];
    char tmp_dev[128];
    DIR *dirp;
    struct dirent *dp;
    int ret = -1;
    int dev_num = 0;
    int d_alloc = 5;


    char **label = (char**)malloc(d_alloc * sizeof(char*));
    if (NULL == label) {
        LOGE("malloc error\n");
        return ret;
    }

    const char* MENU_HEADERS[] = { "Choose a package to install:",
                                   "external storage",
                                   "",
                                   NULL };
    const char** headers = prepend_title(MENU_HEADERS);
    memset(dev_string, 0,sizeof(dev_string));
    strcpy(dev_string[dev_num],"../");
    label[dev_num] = strdup("../");
    dev_num++;

    dirp = opendir("/dev/block");
    if (NULL == dirp) {
        LOGE("Can't open /dev/block\n");
        free(label);
        return ret;
    }
    char* p = (char*)malloc(128);
    if (NULL == p) {
        free(label);
        LOGE("malloc error\n");
    }
    
    do {
        if (NULL == (dp = readdir(dirp))) {
            break;
        }
        memset(p,0,128);
        strncpy(p,"/dev/block/",strlen("/dev/block/"));
        strcat(p,dp->d_name);
        memset(tmp_label,0,128);
        memset(tmp_uuid,0,128);
        memset(tmp_dev,0,128);

        if (0 == get_volume_info(p,tmp_label,tmp_uuid,tmp_dev)) {
            if (dev_num >= d_alloc) {
                d_alloc *= 2;
                label = (char**)realloc(label, d_alloc * sizeof(char*));
            }
            //dev
            strcpy(dev_string[dev_num],tmp_dev);
            //label
            label[dev_num] = strdup(tmp_label);
            dev_num++;
        }
        ret = 0;
    } while(1);
    free(p);
    closedir(dirp);
    label[dev_num] = NULL;

    //display the label to panel
    int chosen_item = 0;
    do {
        chosen_item = get_menu_selection(headers, label, 1, chosen_item, device);

        char* item = dev_string[chosen_item];
        int item_len = strlen(item);
        if (chosen_item == 0) {          // item 0 is always "../"
            // go up but continue browsing (if the caller is update_directory)
            ret = -1;
            break;
        } else {
            // recurse down into a subdirectory
            char new_path[PATH_MAX];
            char dev[128];
            memset(dev,0,128);
            strcpy(dev,"/dev/block/");
            strlcpy(new_path, "/mnt/usb/sda1/",PATH_MAX);
            strcat(dev, item);
            LOGI("devname:%s\n",dev);
            ret = add_usb_device(new_path,dev);
            BACKUP_MEDIUM = UDISK_MOUNT_POINT;
            if (ret >= 0) break;
        }
    } while (true);

    for(int i = 0;i < dev_num; i++) free(label[i]);
    free(label);
    free(headers);
    return ret;
}

static int mstar_system(const char * cmdstring) {
    pid_t pid;
    int status;

    LOGI("[%s: %d] cmdstring=\"%s\"\n",__func__,__LINE__,cmdstring);
    if (cmdstring == NULL) {
        return (1);
    }

    if ((pid = fork())<0) {
        status = -1;
    } else if (pid == 0) {
        execl("/sbin/busybox", "sh", "-c", cmdstring, (char *)0);
        exit(127);
    } else {
        while (waitpid(pid, &status, 0) < 0){
            if (errno != EINTR) {
                status = -1;
                break;
            }
        }
    }

    return status;
}

static int check_is_sdcard_or_udisk(const char* path) {
    int ret_val;
    int mount_flag = 1;
    char *p;
    char path_back[256];
    char local_path[256];
    struct stat buff;
    sleep(2);
    if (path == NULL) {
        LOGE("[%s: %d] the path is NULL, please difine the path!\n",__func__, __LINE__);
        return -1;
    } else {
        memset(path_back, 0, sizeof(path_back));
        strcpy(path_back, path);
        memset(local_path, 0, sizeof(local_path));
        if (path_back[0] ==  '/') {
            strcat(local_path,"/");
        }

        p = strtok(path_back, "/");
        while (p!=NULL) {
            strcat(local_path,p);
            LOGI("[%s: %d] local_path=\"%s\"\n",__func__, __LINE__,local_path);
            if (mount_flag && (!strcmp(local_path, SDCARD_MOUNT_POINT))) {
                if (ensure_path_mounted(local_path) != 0) {
                    LOGE("[%s: %d] Can't mount \"%s\"\n",__func__,__LINE__, local_path);
                    sdcard_is_mounted = 0;
                    ret_val = -1;
                } else {
                    LOGI("[%s: %d] Mount \"%s\" ok\n",__func__,__LINE__, local_path);
                    sdcard_is_mounted = 1;
                    ret_val = SDCARD_RET_VAL;
                }
                mount_flag = 0;
            } else if (mount_flag && (!strncmp(local_path, UDISK_MOUNT_POINT, strlen(UDISK_MOUNT_POINT)-1))) {
                if (ensure_path_mounted(local_path) != 0) {
                    LOGE("[%s: %d] Can't mount \"%s\"\n",__func__,__LINE__, local_path);
                    udisk_is_mounted = 0;
                    ret_val = -1;
                } else {
                    LOGI("[%s: %d] Mount \"%s\" ok\n",__func__,__LINE__, local_path);
                    udisk_is_mounted = 1;
                    ret_val = UDISK_RET_VAL;
                }
                mount_flag = 0;
            } else if (mount_flag && (!strncmp(local_path, SDCARD_MOUNT_POINT2, strlen(SDCARD_MOUNT_POINT2)-1))) {
                if (ensure_path_mounted(local_path) != 0) {
                    LOGE("[%s: %d] Can't mount \"%s\"\n",__func__,__LINE__, local_path);
                    sdcard_is_mounted = 0;
                    ret_val = -1;
                } else {
                    LOGI("[%s: %d] Mount \"%s\" ok\n",__func__,__LINE__, local_path);
                    sdcard_is_mounted = 1;
                    ret_val = SDCARD_RET_VAL;
                }
                mount_flag = 0;
            }

            // check if back up path is exist or no
            if (stat(local_path, &buff)<0) {
                mkdir(local_path, 0755);
            }

            strcat(local_path,"/");
            p = strtok(NULL, "/");
        }
    }
    return ret_val;
}

static int update_medium_directory(const char* path, const char* unmount_when_done, char* folder_name, Device* device) {
    int wipe_cache = 0;
    const char* MENU_HEADERS[] = { "Choose a folder to recovery:",
                                   path,
                                   "",
                                   NULL };
    DIR* d;
    struct dirent* de;

    if (strncmp(path, SDCARD_MOUNT_POINT, strlen(SDCARD_MOUNT_POINT))) {
        ensure_path_mounted(path);
    } else {
        if (ensure_path_mounted(SDCARD_MOUNT_POINT) != 0) {
            LOGE("Can't mount \"%s\"\n", SDCARD_MOUNT_POINT);
        } else {
            LOGI("\"%s\" partition is mounted\n", SDCARD_MOUNT_POINT);
        }
    }

    d = opendir(path);
    if (d == NULL) {
        LOGE("error opening %s: %s\n", path, strerror(errno));
        if (unmount_when_done != NULL) {
            ensure_path_unmounted(unmount_when_done);
        }
        return 0;
    }

    const char** headers = prepend_title(MENU_HEADERS);

    int d_size = 0;
    int d_alloc = 10;
    char** dirs = (char** )malloc(d_alloc * sizeof(char*));
    int z_size = 1;
    int z_alloc = 10;
    char** zips = (char** )malloc(z_alloc * sizeof(char*));
    zips[0] = strdup("../");

    while ((de = readdir(d)) != NULL) {
        int name_len = strlen(de->d_name);

        if (de->d_type == DT_DIR) {
            // skip "." and ".." entries
            if (name_len == 1 && de->d_name[0] == '.') continue;
            if (name_len == 2 && de->d_name[0] == '.' &&
                de->d_name[1] == '.') continue;

            if (d_size >= d_alloc) {
                d_alloc *= 2;
                dirs = (char** )realloc(dirs, d_alloc * sizeof(char*));
            }
            dirs[d_size] = (char* )malloc(name_len + 2);
            strcpy(dirs[d_size], de->d_name);
            dirs[d_size][name_len] = '/';
            dirs[d_size][name_len+1] = '\0';
            ++d_size;
        } else if (de->d_type == DT_REG &&
                   name_len >= 4 &&
                   strncasecmp(de->d_name + (name_len-4), ".zip", 4) == 0) {
            if (z_size >= z_alloc) {
                z_alloc *= 2;
                zips = (char** )realloc(zips, z_alloc * sizeof(char*));
            }
            zips[z_size++] = strdup(de->d_name);
        }
    }
    closedir(d);

    qsort(dirs, d_size, sizeof(char*), compare_string);
    qsort(zips, z_size, sizeof(char*), compare_string);

    // append dirs to the zips list
    if (d_size + z_size + 1 > z_alloc) {
        z_alloc = d_size + z_size + 1;
        zips = (char** )realloc(zips, z_alloc * sizeof(char*));
    }
    memcpy(zips + z_size, dirs, d_size * sizeof(char*));
    free(dirs);
    z_size += d_size;
    zips[z_size] = NULL;

    int result;
    int chosen_item = 0;
    do {
        chosen_item = get_menu_selection(headers, zips, 1, chosen_item,device);

        char* item = zips[chosen_item];
        int item_len = strlen(item);
        if (chosen_item == 0) {          // item 0 is always "../"
            LOGI("[%s: %d] recovery_folder_find=%d, chosen_item=%d, item=\"%s\"\n", __func__,__LINE__,recovery_folder_find, chosen_item, item);
            if (!recovery_folder_find) return 0;
            recovery_folder_find = 2;
            LOGI("[%s: %d] recovery_folder_find=%d, chosen_item=%d, item=\"%s\"\n", __func__,__LINE__,recovery_folder_find, chosen_item, item);
            // go up but continue browsing (if the caller is update_directory)
            result = -1;
            break;
        } else if (item[item_len-1] == '/') {
            //--recovery_folder_find;
            // recurse down into a subdirectory
            char new_path[PATH_MAX];
            strlcpy(new_path, path, PATH_MAX);
            strlcat(new_path, "/", PATH_MAX);
            strlcat(new_path, item, PATH_MAX);
            new_path[strlen(new_path)-1] = '\0';  // truncate the trailing '/'
            LOGI("[%s: %d] new_path=\"%s\", chosen_item=%d, item=\"%s\"\n", __func__,__LINE__,new_path,chosen_item, item);
            strcpy(folder_name, item);
            LOGI("[%s: %d] folder_name=\"%s\", recovery_folder_find=%d\n", __func__,__LINE__,folder_name, recovery_folder_find);
            if ((strncmp(item, FILE_NAME,strlen(FILE_NAME)) == 0)||(recovery_folder_find < 2)) {
                --recovery_folder_find;
                LOGI("[%s: %d] folder_name=\"%s\", recovery_folder_find=%d\n", __func__,__LINE__,folder_name, recovery_folder_find);
                if (!recovery_folder_find) {
                    LOGI("[%s: %d] folder_name=\"%s\", recovery_folder_find=%d\n", __func__,__LINE__,folder_name, recovery_folder_find);
                    return 0;
                }
            }
            result = update_medium_directory(new_path, unmount_when_done, folder_name, device);
            if (result >= 0) break;
        } else {
            // selected a zip file:  attempt to install it, and return
            // the status to the caller.
            char new_path[PATH_MAX];
            strlcpy(new_path, path, PATH_MAX);
            strlcat(new_path, "/", PATH_MAX);
            strlcat(new_path, item, PATH_MAX);
            recovery_folder_find = 2;
            LOGI("[%s: %d] new_path=\"%s\", recovery_folder_find=%d, chosen_item=%d, item=\"%s\"\n",\
                                         __func__,__LINE__,new_path, recovery_folder_find, chosen_item, item);

            LOGI("\n-- Install %s ...\n", path);
            set_sdcard_update_bootloader_message();
            char* copy = copy_sideloaded_package(new_path);
            if (unmount_when_done != NULL) {
                ensure_path_unmounted(unmount_when_done);
            }
            if (copy) {
                result = install_package(copy, &wipe_cache, get_temporary_install_file_path());
                free(copy);
            } else {
                result = INSTALL_ERROR;
            }
            break;
        }
    } while (1);

    int i;
    for (i = 0; i < z_size; ++i) free(zips[i]);
    free(zips);
    free(headers);

    if (unmount_when_done != NULL) {
        ensure_path_unmounted(unmount_when_done);
    }
    return result;
}

int mstaredison_system_mount(void) {
    int system_ret = 0;

#if BACKUP_SYSTEM_BLOCK_ENABLE
    if (ensure_path_mounted(SYSTEM_MOUNT_POINT) != 0) {
        LOGE("Can't mount %s\n", SYSTEM_MOUNT_POINT);
        return INSTALL_CORRUPT;
    }
    LOGI("%s partition is mounted\n",SYSTEM_MOUNT_POINT);
#endif

#if BACKUP_USERDATA_BLOCK_ENABLE
    if (ensure_path_mounted(USERDATA_MOUNT_POINT) != 0) {
        LOGE("Can't mount %s\n", USERDATA_MOUNT_POINT);
        return INSTALL_CORRUPT;
    }
    LOGI("%s partition is mounted\n",USERDATA_MOUNT_POINT);
#endif

#if 1
//#if BACKUP_CACHE_BLOCK_ENABLE
    if (ensure_path_mounted(CACHE_MOUNT_POINT) != 0) {
        LOGE("Can't mount %s\n", CACHE_MOUNT_POINT);
        return INSTALL_CORRUPT;
    }
    LOGI("%s partition is mounted\n",CACHE_MOUNT_POINT);
#endif

#if BACKUP_TVSERVICE_BLOCK_ENABLE
    if (ensure_path_mounted(TVSERVICE_MOUNT_POINT) != 0) {
        LOGE("Can't mount %s\n", TVSERVICE_MOUNT_POINT);
        return INSTALL_CORRUPT;
    }
    LOGI("%s partition is mounted\n",TVSERVICE_MOUNT_POINT);
#endif

#if BACKUP_TVCUSTOMER_BLOCK_ENABLE
    if (ensure_path_mounted(TVCUSTOMER_MOUNT_POINT) != 0) {
        LOGE("Can't mount %s\n", TVCUSTOMER_MOUNT_POINT);
        return INSTALL_CORRUPT;
    }
    LOGI("%s partition is mounted\n",TVCUSTOMER_MOUNT_POINT);
#endif

#if BACKUP_TVDATABASE_BLOCK_ENABLE
    if (ensure_path_mounted(TVDATABASE_MOUNT_POINT) != 0) {
        LOGE("Can't mount %s\n", TVDATABASE_MOUNT_POINT);
        return INSTALL_CORRUPT;
    }
    LOGI("%s partition is mounted\n",TVDATABASE_MOUNT_POINT);
#endif

#if 0
    if (ensure_path_mounted(SDCARD_MOUNT_POINT) != 0) {
        sdcard_is_mounted = 0;
        LOGE("Can't mount \"%s\"\n", SDCARD_MOUNT_POINT);
    } else {
        sdcard_is_mounted = 1;
        LOGE("\"%s\" partition is mounted\n", SDCARD_MOUNT_POINT);
    }
#endif

    if (ensure_path_mounted("/tmp") != 0) {
        tmp_is_mounted = 0;
        LOGE("Can't mount %s\n", "/tmp");
        return INSTALL_CORRUPT;
    } else {
        tmp_is_mounted = 1;
        LOGI("/tmp partition is mounted\n");
    }

    return 0;
}

int mstaredison_system_erase(void) {
    int system_ret = 0;

    mstaredison_system_mount();

#if BACKUP_SYSTEM_BLOCK_ENABLE
    system_ret = format_volume(SYSTEM_MOUNT_POINT);
    if (system_ret) {
        LOGE("erase volume \"%s\" err(%d)\n",SYSTEM_MOUNT_POINT,system_ret);
    } else {
        LOGI("erase volume \"%s\" return value=%d\n",SYSTEM_MOUNT_POINT,system_ret);
    }
#endif

#if BACKUP_USERDATA_BLOCK_ENABLE
    system_ret = format_volume(USERDATA_MOUNT_POINT);
    if (system_ret) {
        LOGE("erase volume \"%s\" err(%d)\n",USERDATA_MOUNT_POINT,system_ret);
    } else {
        LOGI("erase volume \"%s\" return value=%d\n",USERDATA_MOUNT_POINT,system_ret);
    }
#endif

#if BACKUP_TVSERVICE_BLOCK_ENABLE
    system_ret = format_volume(TVSERVICE_MOUNT_POINT);
    if (system_ret) {
        LOGE("erase volume \"%s\" err(%d)\n",TVSERVICE_MOUNT_POINT,system_ret);
    } else {
        LOGI("erase volume \"%s\" return value=%d\n",TVSERVICE_MOUNT_POINT,system_ret);
    }
#endif

#if BACKUP_TVCUSTOMER_BLOCK_ENABLE
    system_ret = format_volume(TVCUSTOMER_MOUNT_POINT);
    if (system_ret) {
        LOGE("erase volume \"%s\" err(%d)\n",TVCUSTOMER_MOUNT_POINT,system_ret);
    } else {
        LOGI("erase volume \"%s\" return value=%d\n",TVCUSTOMER_MOUNT_POINT,system_ret);
    }
#endif

#if BACKUP_TVDATABASE_BLOCK_ENABLE
    system_ret = format_volume(TVDATABASE_MOUNT_POINT);
    if (system_ret) {
        LOGE("erase volume \"%s\" err(%d)\n",TVDATABASE_MOUNT_POINT,system_ret);
    } else {
        LOGI("erase volume \"%s\" return value=%d\n",TVDATABASE_MOUNT_POINT,system_ret);
    }
#endif

    return 0;
}

int mstaredison_block_backup(__volume_backup* vl, unsigned int i, const char* folder_buff) {
    int system_ret = 0;
    char cmd_buff[256];
    char doc_buff[64];
    char document_name[64];
    char * p_name;

    p_name = &vl[i].mount_point[1];
    LOGI("[%s: %d] vl[%d].mount_point=\"%s\" \n", __func__,__LINE__, i, vl[i].mount_point);
    LOGI("[%s: %d] vl[%d].device_path=\"%s\" \n", __func__,__LINE__, i, vl[i].device_path);
    LOGI("[%s: %d] p_name=\"%s\", folder_buff=\"%s\" \n", __func__,__LINE__, p_name, folder_buff);

    if ((!strcmp(vl[i].mount_point, RECOVERY_MOUNT_POINT)) || \
        (!strcmp(vl[i].mount_point, BOOT_MOUNT_POINT)) || \
        (!strcmp(vl[i].mount_point, SYSTEM_MOUNT_POINT))) {
        sprintf(document_name,"%s.%s", p_name, "gz");
        LOGI("[%s: %d] document_name=\"%s\"\n",__func__,__LINE__, document_name);

        //Command: busybox dd if=/dev/block/mmcblkX | busybox gzip > /sdcard/Mstaredison_recovery/20120220125959/mmcblkX.gz;
        sprintf(cmd_buff, "busybox dd if=%s | busybox gzip > %s/%s", vl[i].device_path, folder_buff, document_name);
        //LOGE("cmd_buff=\"%s\"\n",cmd_buff );

        system_ret = mstar_system(cmd_buff);
        if (system_ret) {
            LOGE("\"%s\" erre(err code:%d)!\n",cmd_buff, system_ret);
            return -1;
        }

    } else {
        sprintf(document_name,"%s.%s", p_name, MAKE_POSTFIX_TAG);
        LOGI("[%s: %d] document_name=\"%s\"\n",__func__,__LINE__, document_name);

#ifdef MAKE_BLOCK_GZIP
        //Command: busybox dd if=/dev/block/mmcblkX | busybox gzip > /sdcard/Mstaredison_recovery/20120220125959/mmcblkX.gz;
        sprintf(cmd_buff, "%s if=%s | %s > %s/%s", MAKE_PREFIX_TAG, vl[i].device_path, GZIP_PREFIX_TAG, folder_buff, document_name);
#elif MAKE_BLOCK_TGZ
        // mount the block
        if (ensure_path_mounted(vl[i].mount_point)!= 0) {
            LOGE("Can't mount %s\n", vl[i].mount_point);
            return INSTALL_CORRUPT;
        }
        LOGI("%s partition is mounted\n", vl[i].mount_point);

        //Command: busybox tar cvf /sdcard/Mstaredison_recovery/xxx/data.tar /data
        sprintf(cmd_buff, "%s cvzf %s/%s  %s", MAKE_PREFIX_TAG, folder_buff, document_name, vl[i].mount_point);
#else
        //Command: busybox cat /dev/block/mmcblkX > /sdcard/Mstaredison_recovery/20120220125959/mmcblkX.img;
        sprintf(cmd_buff, "%s %s > %s/%s", MAKE_PREFIX_TAG, vl[i].device_path, folder_buff, document_name);
#endif
        //LOGE("cmd_buff=\"%s\"\n",cmd_buff );

        system_ret = mstar_system(cmd_buff);
        if (system_ret) {
            LOGE("\"%s\" erre(err code:%d)!\n",cmd_buff, system_ret);
        }
    }

    // generate the md5 code
    LOGI("[%s: %d] Generate the md5 code for block \"%s\"!\n", __func__,__LINE__,p_name);

    sprintf(cmd_buff, "%s/%s", folder_buff, document_name);
    LOGI("cmd_buff=\"%s\" \n", cmd_buff);
    mstar_md5sum(cmd_buff, md5code_new);
    LOGI("[%s: %d] md5code=\"%s\"\n",__func__,__LINE__, md5code_new);
    if (!strcmp(md5code_new, "")) {
        LOGE("[%s: %d] Generate \"%s\" md5code is NULL!\n",__func__,__LINE__, cmd_buff);
        return -1;
    }

    sprintf(document_name,"%s.%s", p_name, MD5SUM_POSTFIX_TAG);
    LOGI("document_name=\"%s\"\n", document_name);
    sprintf(cmd_buff, "%s %s > %s/%s", WRITE_MD5SUM_CMD, md5code_new, folder_buff, document_name);
    LOGI("cmd_buff=\"%s\"\n",cmd_buff );

    system_ret = mstar_system(cmd_buff);
    if (system_ret) {
        LOGE("\"%s\" erre(err code:%d)!\n\n",cmd_buff, system_ret);
        return -1;
    }

    return 0;
}

int mstaredison_system_backup(const char* path) {
    int ret;
    int system_ret = 0;
    char *p;
    DIR* d;
    char path_back[256];
    char local_path[256];
    struct stat buff;

    char md5code[MD5_BUFF_LENGTH]={'\0'};

    mstaredison_system_mount();
    if (path) {
        ret = check_is_sdcard_or_udisk(path);
        if (ret == -1) {
            LOGE("[%s: %d] Check and mount err!\n",__func__,__LINE__);
          goto done;
        }
        if ((ret==SDCARD_RET_VAL) && (1==sdcard_is_mounted)) {
            LOGI("[%s: %d] Detect SDcard is mounted!\n",__func__,__LINE__);
        }
        if ((ret==UDISK_RET_VAL) && (1==udisk_is_mounted)) {
            LOGI("[%s: %d] Detect UDisk is mounted!\n",__func__,__LINE__);
        }

        strcpy(folder_name, path);
        d = opendir(folder_name);
        if (d == NULL) {
            LOGE("[%s: %d] The path:\"%s\" is not exist!\n",__func__, __LINE__, folder_name);
            ret = -1;
            goto done;
        }
        LOGI("[%s: %d] folder_name=\"%s\"\n",__func__, __LINE__, folder_name);

    } else {
        if (!strncmp(BACKUP_MEDIUM, SDCARD_MOUNT_POINT, strlen(SDCARD_MOUNT_POINT))) {
            // remount the sdcard
            if (ensure_path_mounted(SDCARD_MOUNT_POINT) != 0) {
                sdcard_is_mounted = 0;
                LOGE("\n***********************************************************************************\n");
                LOGE("*    Can't mount the default backup medium[ SDCard ], Please insert a new one!    *\n");
                LOGE("***********************************************************************************\n\n");
                ret = -1;
                goto done;
            } else {
                sdcard_is_mounted = 1;
                LOGI("[%s: %d] Mount \"%s\"\n",__func__,__LINE__, SDCARD_MOUNT_POINT);
            }
        } else {
            // remount the udisk
            if (ensure_path_mounted(UDISK_MOUNT_POINT) != 0) {
                udisk_is_mounted = 0;
                LOGE("\n**********************************************************************************\n");
                LOGE("*    Can't mount the default backup medium[ UDisk ], Please insert a new one!    *\n");
                LOGE("**********************************************************************************\n\n");
                ret = -1;
                goto done;
            } else {
                udisk_is_mounted = 1;
                LOGI("[%s: %d] Mount \"%s\"\n",__func__,__LINE__, UDISK_MOUNT_POINT);
            }
        }
        d = opendir(folder_head);
        if (d == NULL) {
            mkdir(folder_head, 0755);
        }
        sprintf(folder_name, "%s/%s", folder_head, time_slice);
        d = opendir(folder_name);
        if (d == NULL) {
            mkdir(folder_name, 0755);
        }
        LOGI("[%s: %d] folder_name=\"%s\"\n",__func__, __LINE__, folder_name);
    }
#if ENABLE_PROGRESS_DISPLAY_IN_SYSTEM_BACKUP_RESTORE
    ui->SetBackground(RecoveryUI::INSTALLING);
    ui->SetTipTitle(RecoveryUI::TIP_TITLE_BACKUP_SYSTEM);
    ui->SetProgressType(RecoveryUI::INDETERMINATE);
    ui->SetProgressType(RecoveryUI::DETERMINATE);
#endif

#if BACKUP_RECOVERY_BLOCK_ENABLE
#if ENABLE_PROGRESS_DISPLAY_IN_SYSTEM_BACKUP_RESTORE
    ui->ShowProgress(0.02, 0);
#endif
    LOGI("\n\n[%s: %d] Backup the partition \"%s\" ...\n",__func__,__LINE__, RECOVERY_MOUNT_POINT);
    ret = find_define_volume(RECOVERY_MOUNT_POINT, volume_backup_table);
    if (ret > BACKUP_VOLUME_NUMBER) {
        LOGE("[%s: %d] Cann't find volume \"%s\" in volume_backup_table!\n",__func__,__LINE__,RECOVERY_MOUNT_POINT);
    } else {
        ret = mstaredison_block_backup(volume_backup_table, ret, folder_name);
        if (ret) {
            LOGE("[%s: %d] Backup \"%s\" err!\n",__func__,__LINE__,RECOVERY_MOUNT_POINT);
            goto done;
        }
    }
#endif

#if BACKUP_BOOT_BLOCK_ENABLE
#if ENABLE_PROGRESS_DISPLAY_IN_SYSTEM_BACKUP_RESTORE
    ui->ShowProgress(0.02, 5);
#endif

    LOGI("\n\n[%s: %d] Backup the partition \"%s\" ...\n",__func__,__LINE__, BOOT_MOUNT_POINT);
    ret = find_define_volume(BOOT_MOUNT_POINT, volume_backup_table);
    if (ret > BACKUP_VOLUME_NUMBER) {
        LOGE("[%s: %d] Cann't find volume \"%s\" in volume_backup_table!\n",__func__,__LINE__,BOOT_MOUNT_POINT);
    } else {
        ret = mstaredison_block_backup(volume_backup_table, ret, folder_name);
        if (ret) {
            LOGE("[%s: %d] Backup \"%s\" err!\n",__func__,__LINE__,BOOT_MOUNT_POINT);
            goto done;
        }
    }
#endif

#if BACKUP_SYSTEM_BLOCK_ENABLE
#if ENABLE_PROGRESS_DISPLAY_IN_SYSTEM_BACKUP_RESTORE
     ui->ShowProgress(0.7, 450);
#endif

    LOGI("\n\n[%s: %d] Backup the partition \"%s\" ...\n",__func__,__LINE__, SYSTEM_MOUNT_POINT);
    ret = find_define_volume(SYSTEM_MOUNT_POINT, volume_backup_table);
    if (ret > BACKUP_VOLUME_NUMBER) {
        LOGE("[%s: %d] Cann't find volume \"%s\" in volume_backup_table!\n",__func__,__LINE__,SYSTEM_MOUNT_POINT);
    } else {
        ret = mstaredison_block_backup(volume_backup_table, ret, folder_name);
        if (ret) {
            LOGE("[%s: %d] Backup \"%s\" err!\n",__func__,__LINE__,SYSTEM_MOUNT_POINT);
            goto done;
        }
    }
#endif

#if BACKUP_USERDATA_BLOCK_ENABLE
#if ENABLE_PROGRESS_DISPLAY_IN_SYSTEM_BACKUP_RESTORE
     ui->ShowProgress(0.04, 10);
#endif

    LOGI("\n\n[%s: %d] Backup the partition \"%s\" ...\n",__func__,__LINE__, USERDATA_MOUNT_POINT);
    ret = find_define_volume(USERDATA_MOUNT_POINT, volume_backup_table);
    if (ret > BACKUP_VOLUME_NUMBER) {
        LOGE("[%s: %d] Cann't find volume \"%s\" in volume_backup_table!\n",__func__,__LINE__,USERDATA_MOUNT_POINT);
    } else {
        ret = mstaredison_block_backup(volume_backup_table, ret, folder_name);
        if (ret) {
            LOGE("[%s: %d] Backup \"%s\" err!\n",__func__,__LINE__,USERDATA_MOUNT_POINT);
            goto done;
        }
    }
#endif

#if BACKUP_CACHE_BLOCK_ENABLE
#if ENABLE_PROGRESS_DISPLAY_IN_SYSTEM_BACKUP_RESTORE
     ui->ShowProgress(0.04, 10);
#endif

    LOGI("\n\n[%s: %d] Backup the partition \"%s\" ...\n",__func__,__LINE__, CACHE_MOUNT_POINT);
    ret = find_define_volume(CACHE_MOUNT_POINT, volume_backup_table);
    if (ret > BACKUP_VOLUME_NUMBER) {
        LOGE("[%s: %d] Cann't find volume \"%s\" in volume_backup_table!\n",__func__,__LINE__,CACHE_MOUNT_POINT);
    } else {
        ret = mstaredison_block_backup(volume_backup_table, ret, folder_name);
        if (ret) {
            LOGE("[%s: %d] Backup \"%s\" err!\n",__func__,__LINE__,CACHE_MOUNT_POINT);
            goto done;
        }
    }
#endif

#if BACKUP_TVSERVICE_BLOCK_ENABLE
#if ENABLE_PROGRESS_DISPLAY_IN_SYSTEM_BACKUP_RESTORE
     ui->ShowProgress(0.06, 40);
#endif

    LOGI("\n\n[%s: %d] Backup the partition \"%s\" ...\n",__func__,__LINE__, TVSERVICE_MOUNT_POINT);
    ret = find_define_volume(TVSERVICE_MOUNT_POINT, volume_backup_table);
    if (ret > BACKUP_VOLUME_NUMBER) {
        LOGE("[%s: %d] Cann't find volume \"%s\" in volume_backup_table!\n",__func__,__LINE__,TVSERVICE_MOUNT_POINT);
    } else {
        ret = mstaredison_block_backup(volume_backup_table, ret, folder_name);
        if (ret) {
            LOGE("[%s: %d] Backup \"%s\" err!\n",__func__,__LINE__,TVSERVICE_MOUNT_POINT);
            goto done;
        }
    }
#endif

#if BACKUP_TVCUSTOMER_BLOCK_ENABLE
#if ENABLE_PROGRESS_DISPLAY_IN_SYSTEM_BACKUP_RESTORE
     ui->ShowProgress(0.06, 10);
#endif

    LOGI("\n\n[%s: %d] Backup the partition \"%s\" ...\n",__func__,__LINE__, TVCUSTOMER_MOUNT_POINT);
    ret = find_define_volume(TVCUSTOMER_MOUNT_POINT, volume_backup_table);
    if (ret > BACKUP_VOLUME_NUMBER) {
        LOGE("[%s: %d] Cann't find volume \"%s\" in volume_backup_table!\n",__func__,__LINE__,TVCUSTOMER_MOUNT_POINT);
    } else {
        ret = mstaredison_block_backup(volume_backup_table, ret, folder_name);
        if (ret) {
            LOGE("[%s: %d] Backup \"%s\" err!\n",__func__,__LINE__,TVCUSTOMER_MOUNT_POINT);
            goto done;
        }
    }
#endif

#if BACKUP_TVDATABASE_BLOCK_ENABLE
#if ENABLE_PROGRESS_DISPLAY_IN_SYSTEM_BACKUP_RESTORE
     ui->ShowProgress(0.04, 10);
#endif

    LOGI("\n\n[%s: %d] Backup the partition \"%s\" ...\n",__func__,__LINE__, TVDATABASE_MOUNT_POINT);
    ret = find_define_volume(TVDATABASE_MOUNT_POINT, volume_backup_table);
    if (ret > BACKUP_VOLUME_NUMBER) {
        LOGE("[%s: %d] Cann't find volume \"%s\" in volume_backup_table!\n",__func__,__LINE__,TVDATABASE_MOUNT_POINT);
    } else {
        ret = mstaredison_block_backup(volume_backup_table, ret, folder_name);
        if (ret) {
            LOGE("[%s: %d] Backup \"%s\" err!\n",__func__,__LINE__,TVDATABASE_MOUNT_POINT);
            goto done;
        }
    }
#endif
done:
    if (1 == sdcard_is_mounted){
        ensure_path_unmounted(SDCARD_MOUNT_POINT);
        sdcard_is_mounted = 0;
    } else if (1 == udisk_is_mounted){
        ensure_path_unmounted(UDISK_MOUNT_POINT);
        udisk_is_mounted = 0;
    }

    #if ENABLE_PROGRESS_DISPLAY_IN_SYSTEM_BACKUP_RESTORE
        ui->ShowProgress(0.02, 0);
    #endif
    if (ret == 0)
        ui->SetTipTitle(RecoveryUI::TIP_TITLE_SUCCESS);
    return ret;
}

int mstaredison_block_restore(__volume_backup* vl, unsigned int i, const char* folder_buff) {
    int system_ret = 0;
    char md5code_local[MD5_BUFF_LENGTH];
    FILE *fp = NULL;

    char cmd_buff[256];
    char doc_buff[64];
    char document_name[64];
    char * p_name;

    p_name = &vl[i].mount_point[1];
    LOGI("[%s: %d] vl[%d].mount_point=\"%s\" \n", __func__,__LINE__, i, vl[i].mount_point);
    LOGI("[%s: %d] vl[%d].device_path=\"%s\" \n", __func__,__LINE__, i, vl[i].device_path);
    LOGI("[%s: %d] p_name=\"%s\", folder_buff=\"%s\" \n", __func__,__LINE__, p_name, folder_buff);
    LOGI("[%s: %d] Restore the block \"%s\"! \n", __func__,__LINE__, p_name);

    if ((!strcmp(vl[i].mount_point, RECOVERY_MOUNT_POINT)) || \
        (!strcmp(vl[i].mount_point, BOOT_MOUNT_POINT)) || \
        (!strcmp(vl[i].mount_point, SYSTEM_MOUNT_POINT))) {
        // generate the md5 code
        LOGI("[%s: %d] Generate the md5 code for block %s!\n", __func__,__LINE__,p_name);
        sprintf(doc_buff,"%s.%s", p_name, "gz");
        sprintf(cmd_buff, "%s%s", folder_buff, doc_buff);
        LOGI("cmd_buff=\"%s\" \n", cmd_buff);
        mstar_md5sum(cmd_buff, md5code_local);
        LOGI("[%s: %d] md5code=\"%s\"\n",__func__,__LINE__, md5code_local);
        if (!strcmp(md5code_local, "")) {
            LOGE("[%s: %d] Generate \"%s\" md5code is NULL!\n",__func__,__LINE__, cmd_buff);
            return -1;
        }

        // get the md5 code
        LOGI("[%s: %d] Get the md5 code for block \"%s\"!\n", __func__,__LINE__,p_name);
        sprintf(doc_buff,"%s.%s", p_name, MD5SUM_POSTFIX_TAG);
        sprintf(cmd_buff, "%s%s", folder_buff, doc_buff);
        LOGI("cmd_buff=\"%s\" \n", cmd_buff);

        fp = fopen(cmd_buff, "r");
        if (NULL == fp) {
            LOGE("[%s: %d] Open file \"%s\" Err!\n", __func__,__LINE__,cmd_buff);
            return -1;
        }

        fgets(md5code_new, sizeof(md5code_local), fp);
        system_ret = fclose(fp);

        md5code_new[MD5_CODE_LENGTH] = '\0';
        LOGI("[%s: %d] md5code=\"%s\"\n",__func__,__LINE__, md5code_new);
        if (!strcmp(md5code_new, "")) {
            LOGE("[%s: %d] Get \"%s\" md5code is NULL!\n",__func__,__LINE__, cmd_buff);
            return -1;
        }

        system_ret = strncmp(md5code_local, md5code_new, MD5_CODE_LENGTH);
        //system_ret = strncmp(md5code_local, md5code_new);
        if (system_ret) {
            LOGI("[%s: %d] Get md5code:\"%s\";\n", __func__,__LINE__,md5code_new);
            LOGI("[%s: %d] Generate md5code:\"%s\";\n", __func__,__LINE__,md5code_local);
            return -1;
        }

        // formate the block
        system_ret = format_volume(vl[i].mount_point);
        if (system_ret) {
            LOGE("erase volume \"%s\" err(%d)\n",vl[i].mount_point, system_ret);
        } else {
            LOGE("erase volume \"%s\" return value=%d\n",vl[i].mount_point, system_ret);
        }

        // mount the block
        if (!strcmp(vl[i].mount_point, SYSTEM_MOUNT_POINT)) {
            if (ensure_path_mounted(vl[i].mount_point)!= 0) {
                LOGE("Can't mount %s\n", vl[i].mount_point);
                return INSTALL_CORRUPT;
            }
            LOGI("%s partition is mounted\n", vl[i].mount_point);
        }

        sprintf(doc_buff,"%s.%s", p_name, "gz");
        LOGI("[%s: %d] doc_buff=\"%s\"\n",__func__,__LINE__, doc_buff);

        //Command: busybox gzip -dc /sdcard/Mstaredison_recovery/20120220125959/mmcblkX.gz | busybox dd of=/dev/block/mmcblkX;
        sprintf(cmd_buff, "busybox gzip -dc %s%s | busybox dd of=%s", folder_buff, doc_buff, vl[i].device_path);

        system_ret = mstar_system(cmd_buff);
        if (system_ret) {
            LOGE("\"%s\" erre(err code:%d)!\n",cmd_buff, system_ret);
            return -1;
        }
        else{
            LOGI("\"%s\" done(ret code:%d)\n",cmd_buff, system_ret);
        }

    } else {
        // generate the md5 code
        LOGI("[%s: %d] Generate the md5 code for block %s!\n", __func__,__LINE__,p_name);
        sprintf(doc_buff,"%s.%s", p_name, MAKE_POSTFIX_TAG);
        sprintf(cmd_buff, "%s%s", folder_buff, doc_buff);
        LOGI("cmd_buff=\"%s\" \n", cmd_buff);
        mstar_md5sum(cmd_buff, md5code_local);
        LOGI("[%s: %d] md5code=\"%s\"\n",__func__,__LINE__, md5code_local);

        // get the md5 code
        LOGI("[%s: %d] Get the md5 code for block \"%s\"!\n", __func__,__LINE__,p_name);
        sprintf(doc_buff,"%s.%s", p_name, MD5SUM_POSTFIX_TAG);
        sprintf(cmd_buff, "%s%s", folder_buff, doc_buff);
        LOGI("cmd_buff=\"%s\" \n", cmd_buff);

        fp = fopen(cmd_buff, "r");
        if (NULL == fp) {
            LOGE("[%s: %d] Open file \"%s\" Err!\n", __func__,__LINE__,cmd_buff);
            return -1;
        }

        fgets(md5code_new, sizeof(md5code_local), fp);
        md5code_new[MD5_CODE_LENGTH] = '\0';
        LOGI("[%s: %d] md5code=\"%s\"\n",__func__,__LINE__, md5code_new);
        system_ret = strncmp(md5code_local, md5code_new, MD5_CODE_LENGTH);
        //system_ret = strncmp(md5code_local, md5code_new);
        if (system_ret) {
            LOGI("[%s: %d] Get md5code:\"%s\";\n", __func__,__LINE__,md5code_new);
            LOGI("[%s: %d] Generate md5code:\"%s\";\n", __func__,__LINE__,md5code_local);
            return -1;
        }

        system_ret = fclose(fp);

        sprintf(doc_buff,"%s.%s", p_name, MAKE_POSTFIX_TAG);
        LOGI("[%s: %d] doc_buff=\"%s\"\n",__func__,__LINE__, doc_buff);

#ifdef MAKE_BLOCK_GZIP
        //Command: busybox gzip -dc /sdcard/Mstaredison_recovery/20120220125959/mmcblkX.gz | busybox dd of=/dev/block/mmcblkX;
        sprintf(cmd_buff, "%s -dc %s%s | %s of=%s", GZIP_PREFIX_TAG, folder_buff, doc_buff, MAKE_PREFIX_TAG, vl[i].device_path);
#elif MAKE_BLOCK_TGZ
        // formate the block
        system_ret = format_volume(vl[i].mount_point);
        if (system_ret) {
            LOGE("erase volume \"%s\" err(%d)\n",vl[i].mount_point, system_ret);
        } else {
            LOGI("erase volume \"%s\" return value=%d\n",vl[i].mount_point, system_ret);
        }

        // mount the block
        if (ensure_path_mounted(vl[i].mount_point)!= 0) {
            LOGE("Can't mount %s\n", vl[i].mount_point);
            return INSTALL_CORRUPT;
        }
        LOGI("%s partition is mounted\n", vl[i].mount_point);

        chmod(vl[i].mount_point, 0777);
        chown(vl[i].mount_point, 1000, 1000);   // system user

        //Command: cd / && busybox tar xvf /sdcard/Mstaredison_recovery/xxx/data.tar
        sprintf(cmd_buff, "cd / && %s xvzf %s%s", MAKE_PREFIX_TAG, folder_buff, doc_buff);
#else
        //Command: busybox cat /sdcard/Mstaredison_recovery/20120220125959/mmcblkX.img > /dev/block/mmcblkX
        sprintf(cmd_buff, "%s %s%s > %s", MAKE_PREFIX_TAG, folder_buff, doc_buff, vl[i].device_path);
#endif

        system_ret = mstar_system(cmd_buff);
        if (system_ret) {
            LOGE("\"%s\" erre(err code:%d)!\n",cmd_buff, system_ret);
            return -1;
        } else {
            LOGI("\"%s\" done(ret code:%d)\n",cmd_buff, system_ret);
        }
    }

    return 0;
}

int mstaredison_system_restore(const char* path, Device* device) {
    int ret;
    int system_ret = 0;
    char md5code_old[34];
    char md5code_new[34];
    DIR* d;
    recovery_folder_find = 2;
    recovery_folder[0] = '\0';
    recovery_folder[1] = '\0';
    recovery_folder[2] = '\0';

    //mstaredison_system_erase();

    if (path) {
        ret = check_is_sdcard_or_udisk(path);
        if (ret == -1) {
            LOGE("[%s: %d] Check and mount err!\n",__func__,__LINE__);
            goto done;
        }
        if ((ret==SDCARD_RET_VAL) && (1==sdcard_is_mounted)) {
            LOGI("[%s: %d] Detect SDcard is mounted!\n",__func__,__LINE__);
        }
        if ((ret==UDISK_RET_VAL) && (1==udisk_is_mounted)) {
            LOGI("[%s: %d] Detect UDisk is mounted!\n",__func__,__LINE__);
        }

        strcpy(folder_name, path);
        d = opendir(folder_name);
        if (d == NULL) {
            LOGE("[%s: %d] The path:\"%s\" is not exist!\n",__func__, __LINE__, folder_name);
            ret = -1;
            goto done;
        }
        LOGI("[%s: %d] folder_name=\"%s\"\n",__func__,__LINE__, folder_name);

    } else {
        if (!strncmp(BACKUP_MEDIUM, SDCARD_MOUNT_POINT, strlen(SDCARD_MOUNT_POINT))) {
            // remount the sdcard
            if (ensure_path_mounted(SDCARD_MOUNT_POINT) != 0) {
                sdcard_is_mounted = 0;
                LOGE("\n***********************************************************************************\n");
                LOGE("*    Can't mount the default backup medium[ SDCard ], Please insert a new one!    *\n");
                LOGE("***********************************************************************************\n\n");
                ret = -1;
                goto done;
            } else {
                sdcard_is_mounted = 1;
                LOGI("[%s: %d] Mount \"%s\"\n",__func__,__LINE__, SDCARD_MOUNT_POINT);
                ret = update_medium_directory(SDCARD_MOUNT_POINT, NULL, recovery_folder, device);
                if (-1 == ret) {
                    goto done;
                }
            }
        } else {
            // remount the udisk
            if (ensure_path_mounted(UDISK_MOUNT_POINT) != 0) {
                udisk_is_mounted = 0;
                LOGE("\n**********************************************************************************\n");
                LOGE("*    Can't mount the default backup medium[ UDisk ], Please insert a new one!    *\n");
                LOGE("**********************************************************************************\n\n");
                ret = -1;
                goto done;
            } else {
                udisk_is_mounted = 1;
                LOGI("[%s: %d] Mount \"%s\"\n",__func__,__LINE__, UDISK_MOUNT_POINT);
                ret = update_medium_directory(UDISK_MOUNT_POINT, NULL, recovery_folder, device);
                if (-1 == ret){
                    goto done;
                }
            }
        }

        LOGI("[%s: %d] folder_name=\"%s\"\n",__func__,__LINE__, folder_name);
        LOGI("[%s: %d] recovery_folder=\"%s\"\n",__func__,__LINE__, recovery_folder);
        sprintf(folder_name, "%s/%s", folder_head, recovery_folder);
        LOGI("[%s: %d] folder_name=\"%s\"\n",__func__,__LINE__, folder_name);
    }

#if ENABLE_PROGRESS_DISPLAY_IN_SYSTEM_BACKUP_RESTORE
    ui->SetBackground(RecoveryUI::INSTALLING);
    ui->SetTipTitle(RecoveryUI::TIP_TITLE_RESTORE_SYSTEM);
    ui->SetProgressType(RecoveryUI::INDETERMINATE);
    ui->SetProgressType(RecoveryUI::DETERMINATE);
#endif

#if BACKUP_RECOVERY_BLOCK_ENABLE
#if ENABLE_PROGRESS_DISPLAY_IN_SYSTEM_BACKUP_RESTORE
     ui->ShowProgress(0.02, 5);
#endif

    LOGI("\n\n[%s: %d] Restore the partition \"%s\" ...\n",__func__,__LINE__, RECOVERY_MOUNT_POINT);
    ret = find_define_volume(RECOVERY_MOUNT_POINT, volume_backup_table);
    if (ret > BACKUP_VOLUME_NUMBER) {
        LOGE("[%s: %d] Cann't find volume \"%s\" in volume_backup_table!\n",__func__,__LINE__,RECOVERY_MOUNT_POINT);
    } else {
        ret = mstaredison_block_restore(volume_backup_table, ret, folder_name);
        if (ret) {
            LOGE("[%s: %d] Restore \"%s\" err!\n",__func__,__LINE__, RECOVERY_MOUNT_POINT);
            goto done;
        }
    }
#endif

#if BACKUP_BOOT_BLOCK_ENABLE
#if ENABLE_PROGRESS_DISPLAY_IN_SYSTEM_BACKUP_RESTORE
     ui->ShowProgress(0.02, 5);
#endif

    LOGI("\n\n[%s: %d] Restore the partition \"%s\" ...\n",__func__,__LINE__, BOOT_MOUNT_POINT);
    ret = find_define_volume(BOOT_MOUNT_POINT, volume_backup_table);
    if (ret > BACKUP_VOLUME_NUMBER) {
        LOGE("[%s: %d] Cann't find volume \"%s\" in volume_backup_table!\n",__func__,__LINE__,BOOT_MOUNT_POINT);
    } else {
        ret = mstaredison_block_restore(volume_backup_table, ret, folder_name);
        if (ret) {
            LOGE("[%s: %d] Restore \"%s\" err!\n",__func__,__LINE__, BOOT_MOUNT_POINT);
            goto done;
        }
    }
#endif

#if BACKUP_SYSTEM_BLOCK_ENABLE
#if ENABLE_PROGRESS_DISPLAY_IN_SYSTEM_BACKUP_RESTORE
     ui->ShowProgress(0.67, 150);
#endif

    LOGI("\n\n[%s: %d] Restore the partition \"%s\" ...\n",__func__,__LINE__, SYSTEM_MOUNT_POINT);
    ret = find_define_volume(SYSTEM_MOUNT_POINT, volume_backup_table);
    if (ret > BACKUP_VOLUME_NUMBER) {
        LOGE("[%s: %d] Cann't find volume \"%s\" in volume_backup_table!\n",__func__,__LINE__,SYSTEM_MOUNT_POINT);
    } else {
        ret = mstaredison_block_restore(volume_backup_table, ret, folder_name);
        if (ret) {
            LOGE("[%s: %d] Restore \"%s\" err!\n",__func__,__LINE__, SYSTEM_MOUNT_POINT);
            goto done;
        }
    }
#endif

#if BACKUP_USERDATA_BLOCK_ENABLE
#if ENABLE_PROGRESS_DISPLAY_IN_SYSTEM_BACKUP_RESTORE
     ui->ShowProgress(0.02, 5);
#endif

    LOGI("\n\n[%s: %d] Restore the partition \"%s\" ...\n",__func__,__LINE__, USERDATA_MOUNT_POINT);
    ret = find_define_volume(USERDATA_MOUNT_POINT, volume_backup_table);
    if (ret > BACKUP_VOLUME_NUMBER) {
        LOGE("[%s: %d] Cann't find volume \"%s\" in volume_backup_table!\n",__func__,__LINE__,USERDATA_MOUNT_POINT);
    } else {
        ret = mstaredison_block_restore(volume_backup_table, ret, folder_name);
        if (ret) {
            LOGE("[%s: %d] Restore \"%s\" err!\n",__func__,__LINE__, USERDATA_MOUNT_POINT);
            goto done;
        }
    }
#endif

#if BACKUP_CACHE_BLOCK_ENABLE
#if ENABLE_PROGRESS_DISPLAY_IN_SYSTEM_BACKUP_RESTORE
     ui->ShowProgress(0.02, 5);
#endif

    LOGI("\n\n[%s: %d] Restore the partition \"%s\" ...\n",__func__,__LINE__, CACHE_MOUNT_POINT);
    ret = find_define_volume(CACHE_MOUNT_POINT, volume_backup_table);
    if (ret > BACKUP_VOLUME_NUMBER) {
        LOGE("[%s: %d] Cann't find volume \"%s\" in volume_backup_table!\n",__func__,__LINE__,CACHE_MOUNT_POINT);
    } else {
        ret = mstaredison_block_restore(volume_backup_table, ret, folder_name);
        if (ret) {
            LOGE("[%s: %d] Restore \"%s\" err!\n",__func__,__LINE__, CACHE_MOUNT_POINT);
            goto done;
        }
    }
#endif

#if BACKUP_TVSERVICE_BLOCK_ENABLE
#if ENABLE_PROGRESS_DISPLAY_IN_SYSTEM_BACKUP_RESTORE
    ui->ShowProgress(0.1, 10);
#endif

    LOGI("\n\n[%s: %d] Restore the partition \"%s\" ...\n",__func__,__LINE__, TVSERVICE_MOUNT_POINT);
    ret = find_define_volume(TVSERVICE_MOUNT_POINT, volume_backup_table);
    if (ret > BACKUP_VOLUME_NUMBER) {
        LOGE("[%s: %d] Cann't find volume \"%s\" in volume_backup_table!\n",__func__,__LINE__,TVSERVICE_MOUNT_POINT);
    } else {
        ret = mstaredison_block_restore(volume_backup_table, ret, folder_name);
        if (ret) {
            LOGE("[%s: %d] Restore \"%s\" err!\n",__func__,__LINE__, TVSERVICE_MOUNT_POINT);
            goto done;
        }
    }
#endif

#if BACKUP_TVCUSTOMER_BLOCK_ENABLE
#if ENABLE_PROGRESS_DISPLAY_IN_SYSTEM_BACKUP_RESTORE
     ui->ShowProgress(0.05, 10);
#endif

    LOGI("\n\n[%s: %d] Restore the partition \"%s\" ...\n",__func__,__LINE__, TVCUSTOMER_MOUNT_POINT);
    ret = find_define_volume(TVCUSTOMER_MOUNT_POINT, volume_backup_table);
    if (ret > BACKUP_VOLUME_NUMBER) {
        LOGE("[%s: %d] Cann't find volume \"%s\" in volume_backup_table!\n",__func__,__LINE__, TVCUSTOMER_MOUNT_POINT);
    } else {
        ret = mstaredison_block_restore(volume_backup_table, ret, folder_name);
        if (ret) {
            LOGE("[%s: %d] Restore \"%s\" err!\n",__func__,__LINE__, TVCUSTOMER_MOUNT_POINT);
            goto done;
        }
    }
#endif

#if BACKUP_TVDATABASE_BLOCK_ENABLE
#if ENABLE_PROGRESS_DISPLAY_IN_SYSTEM_BACKUP_RESTORE
    ui->ShowProgress(0.05, 10);
#endif

    LOGI("\n\n[%s: %d] Restore the partition \"%s\" ...\n",__func__,__LINE__, TVDATABASE_MOUNT_POINT);
    ret = find_define_volume(TVDATABASE_MOUNT_POINT, volume_backup_table);
    if (ret > BACKUP_VOLUME_NUMBER) {
        LOGE("[%s: %d] Cann't find volume \"%s\" in volume_backup_table!\n",__func__,__LINE__, TVDATABASE_MOUNT_POINT);
    } else {
        ret = mstaredison_block_restore(volume_backup_table, ret, folder_name);
        if (ret) {
            LOGE("[%s: %d] Restore \"%s\" err!\n",__func__,__LINE__, TVDATABASE_MOUNT_POINT);
            goto done;
        }
    }
#endif

done:
    if (1 == sdcard_is_mounted) {
        ensure_path_unmounted(SDCARD_MOUNT_POINT);
        sdcard_is_mounted = 0;
    } else if (1 == udisk_is_mounted) {
        ensure_path_unmounted(UDISK_MOUNT_POINT);
        udisk_is_mounted = 0;
    }

    #if ENABLE_PROGRESS_DISPLAY_IN_SYSTEM_BACKUP_RESTORE
        ui->ShowProgress(0.05, 0);
    #endif
    if (0 == ret)
        ui->SetTipTitle(RecoveryUI::TIP_TITLE_SUCCESS);
    return ret;
}

int find_define_volume(char* find, __volume_backup* vl) {
    int i,j;
    int ret;

    if ((find==NULL)||(vl==NULL)) {
        printf("The point \"find\" or \"vl\" is NULL!\n");
        return -1;
    }

    for (i=0;i<BACKUP_VOLUME_NUMBER;i++) {
        if (!(ret = strcmp(find, vl[i].mount_point))) {
            break;
        }
    }

    if (i<BACKUP_VOLUME_NUMBER) {
        return i;
    } else {
        return 0xffff;
    }
}

int copy_define_volume(char* find,  __volume_backup* vl, Volume* v) {
    int i,j;
    int ret;
    int bingo;

    if ((find==NULL)||(vl==NULL)||(v==NULL)) {
        printf("The point \"find\", \"vl\" or \"v\" is NULL!\n");
        return -1;
    }

    bingo = 0;
    for(i=0;i<16;i++) {
        if (!(ret = strcmp(find, v[i].mount_point))) {
            bingo = 1;
            break;
        }
    }

    if (bingo) {
        for(j=0;j<BACKUP_VOLUME_NUMBER;j++) {
            if (vl[j].mount_point[0] == '/') {
                LOGI("[%d: %d,%d] mount_point=%s, device_path=%s\n", __LINE__,i,j,vl[j].mount_point,vl[j].device_path);
            } else {
                strcpy(vl[j].mount_point, v[i].mount_point);
                strcpy(vl[j].device_path, v[i].blk_device);
                LOGI("[%d: %d,%d] mount_point=%s, device_path=%s\n", __LINE__,i,j,vl[j].mount_point,vl[j].device_path);
                break;
            }
        }
        if (j<BACKUP_VOLUME_NUMBER) {
            return 0;
        } else {
            return -1;
        }
    }
    return -2;
}

int init_backup_recovery(Device* device, int flag) {
    if (flag) {
        int ret = ensure_backup_path(device);
        if (-1 == ret) {
            return 0;
        }
    }
    Volume* p_device_volumes = get_volume_table_head();
    memset(volume_backup_table, 0, sizeof(__volume_backup)*BACKUP_VOLUME_NUMBER);

#if BACKUP_RECOVERY_BLOCK_ENABLE
    copy_define_volume(RECOVERY_MOUNT_POINT, volume_backup_table, p_device_volumes);
#endif

#if BACKUP_BOOT_BLOCK_ENABLE
    copy_define_volume(BOOT_MOUNT_POINT, volume_backup_table, p_device_volumes);
#endif

#if BACKUP_SYSTEM_BLOCK_ENABLE
    copy_define_volume(SYSTEM_MOUNT_POINT, volume_backup_table, p_device_volumes);
#endif

#if BACKUP_USERDATA_BLOCK_ENABLE
    copy_define_volume(USERDATA_MOUNT_POINT, volume_backup_table, p_device_volumes);
#endif

#if 1
//#if BACKUP_CACHE_BLOCK_ENABLE
    copy_define_volume(CACHE_MOUNT_POINT, volume_backup_table, p_device_volumes);
#endif

#if BACKUP_TVSERVICE_BLOCK_ENABLE
    copy_define_volume(TVSERVICE_MOUNT_POINT, volume_backup_table, p_device_volumes);
#endif

#if BACKUP_TVCUSTOMER_BLOCK_ENABLE
    copy_define_volume(TVCUSTOMER_MOUNT_POINT, volume_backup_table, p_device_volumes);
#endif

#if BACKUP_TVDATABASE_BLOCK_ENABLE
    copy_define_volume(TVDATABASE_MOUNT_POINT, volume_backup_table, p_device_volumes);
#endif

    time(&time1);
    time2=gmtime(&time1);
    //time2=localtime(&time1);
    sprintf(time_slice,"%04d%02d%02d%02d%02d%02d",
        time2->tm_year+1900,time2->tm_mon+1,time2->tm_mday,time2->tm_hour,time2->tm_min,time2->tm_sec);
    sprintf(folder_head,"%s/%s", BACKUP_MEDIUM, FILE_NAME);
    LOGI("[%s: %d] folder_head=%s\n", __func__,__LINE__,folder_head);
    return 1;
}
