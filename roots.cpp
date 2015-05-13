/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
// MStar Android Patch Begin
#include <dirent.h>
#include <fcntl.h>
#include <linux/msdos_fs.h>
#include "adb_install.h"
#include "blkid/blkid.h"
#include <linux/ext2_fs.h>
// MStar Android Patch End
#include <fs_mgr.h>
#include "mtdutils/mtdutils.h"
#include "mtdutils/mounts.h"
#include "roots.h"
#include "common.h"
#include "make_ext4fs.h"

static struct fstab *fstab = NULL;

extern struct selabel_handle *sehandle;

void load_volume_table()
{
    int i;
    int ret;

    fstab = fs_mgr_read_fstab("/etc/recovery.fstab");
    if (!fstab) {
        LOGE("failed to read /etc/recovery.fstab\n");
        return;
    }

    ret = fs_mgr_add_entry(fstab, "/tmp", "ramdisk", "ramdisk", 0);
    if (ret < 0 ) {
        LOGE("failed to add /tmp entry to fstab\n");
        fs_mgr_free_fstab(fstab);
        fstab = NULL;
        return;
    }

    printf("recovery filesystem table\n");
    printf("=========================\n");
    for (i = 0; i < fstab->num_entries; ++i) {
        Volume* v = &fstab->recs[i];
        printf("  %d %s %s %s %lld\n", i, v->mount_point, v->fs_type,
               v->blk_device, v->length);
    }
    printf("\n");
}

Volume* volume_for_path(const char* path) {
    return fs_mgr_get_entry_for_mount_point(fstab, path);
}

// MStar Android Patch Begin
int scan_device(const char* path){
    DIR *dirp;
    struct dirent *dp;
    int ret = -1;
    char name_dev[10];
    if (NULL == path) {
        return ret;
    }
    memset(name_dev,0,sizeof(name_dev));
    dirp = opendir("/dev/block");
    if(NULL == dirp) {
        LOGE("Can't open /dev/block\n");
        return ret;
    }
    Volume* v = volume_for_path(path);
    strcpy(name_dev, strrchr(v->blk_device,'/')+1);
    do {
        if (NULL == (dp = readdir(dirp))) {
            break;
        }
        if ((DT_BLK == dp->d_type) && (strlen(name_dev) == strlen(dp->d_name))) {
            if (0 == strcmp(dp->d_name,name_dev)) {
                ret = 0;
                break;
            }
        }
    } while(1);

    closedir(dirp);
    return ret;
}

int add_usb_device(char* path,char *devpath) {
    char pathname[128];
    int ret = 0;
    char filename[128];
    int length = 0;

    memset(filename, 0, 128);
    strcpy(filename, strrchr(path, '/')+1);
    LOGI("updatefilename: %s\n",filename);

    //Check the path used to mount USB device.
    //The pathname should be like "/mnt/usb/sdxx", for example "/mnt/usb/sda1".
    if ((NULL == path) || (length > strlen(path))) {
        LOGE("Invalid path to mount USB device!\n");
        ret = -1;
    }
    char *tmp1;
    if (0 == strncmp(path,"/mnt/usb/",strlen("/mnt/usb/")))
        tmp1 = strstr(path+9, "/");
    else if (0 == strncmp(path,"/mnt/sdcard",strlen("/mnt/sdcard")))
        tmp1 = strstr(path+11, "/");
    length = tmp1-path;
    memset(pathname, 0, 128);
    strncpy(pathname, path, length);
    LOGV("path:%s,pathname:%s,length:%d\n",path,pathname,length);
    fs_mgr_free_fstab(fstab);
    load_volume_table();
    ret = fs_mgr_add_entry(fstab, pathname, "vfat", devpath, 0);
    if (0 != ret)
        LOGE("fs_mgr_add_entry :%s fail \n",pathname);
    LOGI("Mount %s on %s\n", devpath, pathname);
    mkdir(pathname, 0755);
    umount(pathname);
    if (0 != mount(devpath, pathname, "vfat", MS_NOATIME|MS_NODEV|MS_NODIRATIME, "")) {
            LOGE("vfat can't mount %s on %s\n",devpath,pathname);
            //package in Mobile hard disk possible
            LOGV("--trying mount again--\n");
            unsigned long flags = MS_NODEV | MS_NOEXEC | MS_NOSUID | MS_DIRSYNC;
            if (0 != (ret = mount(devpath, pathname, "ntfs", flags, ""))) {
                LOGE("ntfs can't mount %s on %s\n",devpath,pathname);
                if (0 != (ret = mount(devpath, pathname, "ntfs3g", flags, ""))) {
                    LOGE("ntfs3g can't mount %s on %s\n",devpath,pathname);
                    ret = -1;
                }
            }
    }

    if (-1 == ret) {
        LOGI("no such device:/dev/block/sdxx\n");
    } else {
        LOGI("Mount %s on %s success\n", devpath, pathname);
    }
    return ret;
}
// MStar Android Patch End

int ensure_path_mounted(const char* path) {
    Volume* v = volume_for_path(path);
    if (v == NULL) {
        LOGE("unknown volume for path [%s]\n", path);
        return -1;
    }
    if (strcmp(v->fs_type, "ramdisk") == 0) {
        // the ramdisk is always mounted.
        return 0;
    }

    int result;
    result = scan_mounted_volumes();
    if (result < 0) {
        LOGE("failed to scan mounted volumes\n");
        return -1;
    }

    const MountedVolume* mv =
        find_mounted_volume_by_mount_point(v->mount_point);
    if (mv) {
        // volume is already mounted
        return 0;
    }

    mkdir(v->mount_point, 0755);  // in case it doesn't already exist

    if (strcmp(v->fs_type, "yaffs2") == 0) {
        // mount an MTD partition as a YAFFS2 filesystem.
        mtd_scan_partitions();
        const MtdPartition* partition;
        partition = mtd_find_partition_by_name(v->blk_device);
        if (partition == NULL) {
            LOGE("failed to find \"%s\" partition to mount at \"%s\"\n",
                 v->blk_device, v->mount_point);
            return -1;
        }
        return mtd_mount_partition(partition, v->mount_point, v->fs_type, 0);
    } else if (strcmp(v->fs_type, "ext4") == 0 ||
               strcmp(v->fs_type, "vfat") == 0) {
        result = mount(v->blk_device, v->mount_point, v->fs_type,
                       MS_NOATIME | MS_NODEV | MS_NODIRATIME, "");
        if (result == 0) return 0;

        LOGE("failed to mount %s (%s)\n", v->mount_point, strerror(errno));
        return -1;
    // MStar Android Patch Begin
    } else if (strcmp(v->fs_type, "ubifs") == 0) {
        char* ubi_dev;
        char* buff;

        //path formate : /dev/ubi0_x
        buff = (char*)malloc(sizeof(char) * (strlen(v->blk_device)+1));
        strcpy(buff, v->blk_device);
        ubi_dev = strtok(buff, "/dev");

        result = mount(ubi_dev, v->mount_point, v->fs_type,
                       MS_NODEV|MS_NOSUID, "");
        if (result == 0) {
            free(buff);
            return 0;
        }

        free(buff);

        LOGE("failed to mount %s (%s)\n", v->mount_point, strerror(errno));
        return -1;
    }
    // MStar Android Patch End

    LOGE("unknown fs_type \"%s\" for %s\n", v->fs_type, v->mount_point);
    return -1;
}

int ensure_path_unmounted(const char* path) {
    Volume* v = volume_for_path(path);
    if (v == NULL) {
        // MStar Android Patch Begin
        if (0 == (strncmp(path, "/mnt/usb", strlen("/mnt/usb")))) {
            if (umount(path)==0) {
                LOGI("umount %s\n",path);
                return 0;
            }
        }
        // MStar Android Patch End
        LOGE("unknown volume for path [%s]\n", path);
        return -1;
    }
    if (strcmp(v->fs_type, "ramdisk") == 0) {
        // the ramdisk is always mounted; you can't unmount it.
        return -1;
    }

    int result;
    result = scan_mounted_volumes();
    if (result < 0) {
        LOGE("failed to scan mounted volumes\n");
        return -1;
    }

    const MountedVolume* mv =
        find_mounted_volume_by_mount_point(v->mount_point);
    if (mv == NULL) {
        // volume is already unmounted
        return 0;
    }

    return unmount_mounted_volume(mv);
}

int format_volume(const char* volume) {
    Volume* v = volume_for_path(volume);
    if (v == NULL) {
        LOGE("unknown volume \"%s\"\n", volume);
        return -1;
    }
    if (strcmp(v->fs_type, "ramdisk") == 0) {
        // you can't format the ramdisk.
        LOGE("can't format_volume \"%s\"", volume);
        return -1;
    }
    if (strcmp(v->mount_point, volume) != 0) {
        LOGE("can't give path \"%s\" to format_volume\n", volume);
        return -1;
    }

    if (ensure_path_unmounted(volume) != 0) {
        LOGE("format_volume failed to unmount \"%s\"\n", v->mount_point);
        return -1;
    }

    if (strcmp(v->fs_type, "yaffs2") == 0 || strcmp(v->fs_type, "mtd") == 0) {
        mtd_scan_partitions();
        const MtdPartition* partition = mtd_find_partition_by_name(v->blk_device);
        if (partition == NULL) {
            LOGE("format_volume: no MTD partition \"%s\"\n", v->blk_device);
            return -1;
        }

        MtdWriteContext *write = mtd_write_partition(partition);
        if (write == NULL) {
            LOGW("format_volume: can't open MTD \"%s\"\n", v->blk_device);
            return -1;
        } else if (mtd_erase_blocks(write, -1) == (off_t) -1) {
            LOGW("format_volume: can't erase MTD \"%s\"\n", v->blk_device);
            mtd_write_close(write);
            return -1;
        } else if (mtd_write_close(write)) {
            LOGW("format_volume: can't close MTD \"%s\"\n", v->blk_device);
            return -1;
        }
        return 0;
    }

    if (strcmp(v->fs_type, "ext4") == 0) {
        int result = make_ext4fs(v->blk_device, v->length, volume, sehandle);
        if (result != 0) {
            LOGE("format_volume: make_extf4fs failed on %s\n", v->blk_device);
            return -1;
        }
        return 0;
    }

    LOGE("format_volume: fs_type \"%s\" unsupported\n", v->fs_type);
    return -1;
}

int setup_install_mounts() {
    if (fstab == NULL) {
        LOGE("can't set up install mounts: no fstab loaded\n");
        return -1;
    }
    for (int i = 0; i < fstab->num_entries; ++i) {
        Volume* v = fstab->recs + i;

        if (strcmp(v->mount_point, "/tmp") == 0 ||
            strcmp(v->mount_point, "/cache") == 0 ||
            strcmp(v->mount_point, "/tvconfig") == 0) {
            if (ensure_path_mounted(v->mount_point) != 0) return -1;

        } else {
            if (ensure_path_unmounted(v->mount_point) != 0) return -1;
        }
    }
    return 0;
}

// MStar Android Patch Begin

static bool get_volume_uuid(const char* path, char* buf, int size) {
    blkid_cache cache = NULL;
    blkid_dev dev = NULL;
    blkid_tag_iterate iter = NULL;
    const char *type = NULL, *value = NULL;
    bool ret = -1;

    if (path == NULL || buf == NULL || size < 0) {
        return -1;
    }

    if (blkid_get_cache(&cache, NULL) < 0) {
        return -1;
    }

    dev = blkid_get_dev(cache, path, BLKID_DEV_NORMAL);
    if (dev == NULL) {
        return -1;
    }
    iter = blkid_tag_iterate_begin(dev);
    while (blkid_tag_next(iter, &type, &value) == 0) {
        if (strcmp(type,"UUID") == 0) {
            int n = strlen(value);
            if (n > size) {
                n = size;
            }
            ret = 0;
            strncpy(buf,value,n);
            buf[n] = '\0';
        }
    }
    blkid_tag_iterate_end(iter);
    blkid_put_cache(cache);
    return ret;
}

#define IOCTL_GET_VOLUME_LABEL _IOR('r', 0x13, __u32)

static bool get_fat_volume_label(const char* devPath, char *label, int len, int* label_len) {
    int fd;
    int ret;
    char tmp[512];
    int size = sizeof(tmp);
    int i;

    if (devPath == NULL || label == NULL || len < 0 || label_len == NULL) {
        return false;
    }

    fd = open(devPath,O_RDONLY);
    if (fd < 0) {
        LOGE("Can not open device %s\n",devPath);
        return false;
    }

    memset(tmp,0,size);
    ret = ioctl(fd,IOCTL_GET_VOLUME_LABEL,tmp);
    if (ret == -1) {
        LOGE("Can not get the FAT label\n");
        close(fd);
        return false;
    }

    close(fd);

    i = 0;
    while (i < size && tmp[i]) {
        ++i;
    }
    if (i > len) {
        i = len;
    }
    *label_len = i;

    strncpy(label,tmp,i);
    close(fd);
    return true;
}

static bool get_ntfs_volume_label(const char* devPath, char *label, int len, int* label_len) {
    int fd;
    int ret;
    char tmp[512];
    int size = sizeof(tmp);
    int i;

    if (devPath == NULL || label == NULL || len < 0 || label_len == NULL) {
        return false;
    }

    fd = open(devPath,O_RDONLY);
    if (fd < 0) {
        LOGE("Can not open device %s\n",devPath);
        return false;
    }

    memset(tmp,0,size);
    ret = ioctl(fd,IOCTL_GET_VOLUME_LABEL,tmp);
    if (ret == -1) {
        LOGE("Can not get the NTFS label\n");
        close(fd);
        return false;
    }

    close(fd);

    i = 0;
    while (i < size && tmp[i]) {
        ++i;
    }
    if (i > len) {
        i = len;
    }
    *label_len = i;

    strncpy(label,tmp,i);
    close(fd);
    return true;
}

static bool get_ext_volume_label(const char* devPath, char *label, int len, int* label_len) {
    int fd;
    int ret;
    struct ext2_super_block super_block;
    int i;

    if (devPath == NULL || label == NULL || len < 0 || label_len == NULL) {
        return false;
    }

    fd = open(devPath,O_RDONLY);
    if (fd < 0) {
        LOGE("Can not open device %s\n",devPath);
        return false;
    }

    ret = lseek(fd,1024,0);
    if (ret < 0) {
        LOGE("Can not seek to ext2 super block\n");
        close(fd);
        return false;
    }

    ret = read(fd,&super_block,sizeof(super_block));
    if (ret < 0) {
        LOGE("Can not read ext2 super block\n");
        close(fd);
        return false;
    }

    if (__le16_to_cpu(super_block.s_magic) != EXT2_SUPER_MAGIC) {
        LOGE("Ext2 super block has error magic\n");
        close(fd);
        return false;
    }
    close(fd);

    i = strlen(super_block.s_volume_name);
    if (i > len) {
        i = len;
    }
    *label_len = i;
    strncpy(label,super_block.s_volume_name,i);
    return true;
}

static bool get_exfat_volume_label(const char* devPath, char *label, int len, int* label_len) {
    int fd;
    int ret;
    char tmp[512];
    int size = sizeof(tmp);
    int i;

    if (devPath == NULL || label == NULL || len < 0 || label_len == NULL) {
        return false;
    }

    fd = open(devPath,O_RDONLY);
    if (fd < 0) {
        LOGE("Can not open device %s\n",devPath);
        return false;
    }

    memset(tmp,0,size);
    ret = ioctl(fd,IOCTL_GET_VOLUME_LABEL,tmp);
    if (ret == -1) {
        LOGE("Can not get the Exfat label\n");
        close(fd);
        return false;
    }
    close(fd);

    i = 0;
    while (i < size && tmp[i]) {
        ++i;
    }
    if (i > len) {
        i = len;
    }
    *label_len = i;

    strncpy(label,tmp,i);
    close(fd);
    return true;
}

Volume* get_volume_table_head() {
    return fstab->recs;
}

const char* get_label_code_type(char* str, int size) {
    int i;
    int j;
    unsigned char* buf = (unsigned char*)str;

    if (size > 3 && buf[0] == 0xef && buf[1] == 0xbb && buf[2] == 0xbf) {
        buf += 3;
        size -= 3;
    }
    /* Fat volume label from windows is considered as GBK. Other volume
     * labels are considered as utf-8.
     */
    for (i = 0; i < size; ++i) {
        if ((buf[i] & 0x80) == 0) {
            continue;
        } else if ((buf[i] & 0x40) == 0) {
            return "GBK";
        } else {
            int following;

            if ((buf[i] & 0x20) == 0) {
                following = 1;
            } else if ((buf[i] & 0x10) == 0) {
                following = 2;
            } else if ((buf[i] & 0x08) == 0) {
                following = 3;
            } else if ((buf[i] & 0x04) == 0) {
                following = 4;
            } else if ((buf[i] & 0x02) == 0) {
                following = 5;
            } else
                return "GBK";

            /* ASCII in utf-8 is always like 0xxxxxxx.
             * Chineses in utf-8 is always like 1110xxxx 10xxxxxx 10xxxxxx.
             * So, if we find "110xxxxx 10xxxxxx", consider it as GBK.
             */
            if (following == 1) {
                return "GBK";
            }

            for (j = 0; j < following; j++) {
                i++;
                if (i >= size)
                    goto done;

                if ((buf[i] & 0x80) == 0 || (buf[i] & 0x40))
                    return "GBK";
            }
        }
    }
done:
    return "UTF-8";
}

int get_volume_label(const char *device,char *target_label) {
    char *fsType;
    char label[512];
    int size = sizeof(label);
    int label_len = 0;

    umount("/mnt/usb/sda1");
    if (0 == mount(device, "mnt/usb/sda1/", "vfat", MS_NOATIME|MS_NODEV|MS_NODIRATIME, "")) {
        fsType = "vfat";
    } else {
        unsigned long flags = MS_NODEV | MS_NOEXEC | MS_NOSUID | MS_DIRSYNC;
        if ((0 == mount(device, "mnt/usb/sda1/", "ntfs", flags, "")) || (0 == mount(device, "mnt/usb/sda1/", "ntfs3g", flags, ""))) {
            fsType = "ntfs";
        } else {
            if (0 == mount(device, "mnt/usb/sda1/", "ext", MS_NOATIME|MS_NODEV|MS_NODIRATIME, "")) {
            fsType = "ext";
            } else {
                if (0 == mount(device, "mnt/usb/sda1/", "exfat", MS_NOATIME|MS_NODEV|MS_NODIRATIME, "")) {
                    fsType = "exfat";
                } else {
                    fsType = NULL;
                }
            }
        }
    }

    if( NULL != fsType) {
        LOGI("The filesystem of %s is %s\n",device,fsType);
        if (strncmp(fsType,"vfat",4) == 0) {
            get_fat_volume_label("mnt/usb/sda1/",label,size,&label_len);
        } else if (strncmp(fsType,"ntfs",4) == 0) {
            get_ntfs_volume_label("mnt/usb/sda1/",label,size,&label_len);
        } else if (strncmp(fsType,"ext",3) == 0) {
            get_ext_volume_label(device,label,size,&label_len);
        } if (strncmp(fsType,"exfat",5) == 0) {
            get_exfat_volume_label("mnt/usb/sda1/",label,size,&label_len);
        }
        umount("/mnt/usb/sda1");
    } else {
        return -1;
    }
    label[label_len] = '\0';
    strncpy(target_label, label, strlen(label));

    return 0;
}

int get_volume_info(char *buf,char *label,char *uuid,char *devname) {
    int ret = -1;

    if ((0 == strncmp(buf,"/dev/block/s",strlen("/dev/block/s"))) ||
        (0 == strncmp(buf,"/dev/block/mmcblk1",strlen("/dev/block/mmcblk1")))) {
        //dev name
        strcpy(devname,buf + strlen("/dev/block/"));
        LOGI("devname:%s\n",devname);
        if (NULL != uuid) {
            ret = get_volume_uuid(buf, uuid, 255);
            LOGI("getVolumeinfo uuid:#%s#\n",uuid);
        }
        if (NULL != label) {
            int flag = 0;
            ret = get_volume_label(buf,label);
            LOGI("lable len:%d\n",strlen(label));
            for(int j = 0; j < strlen(label); j++) {
                if (0 != strncmp(label + j," ",1)) {
                    flag = -1;
                    break;
                }
            }
            if((0 == ret) && ((0 == strlen(label)) || (0 == flag))) {
                strcpy(label,"NO NAME");
            }
            LOGI("getVolumeinfo label:#%s#\n",label);
        }
    }

    return ret;
}
// MStar Android Patch End

