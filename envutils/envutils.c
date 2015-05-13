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

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "env_spi.h"
#include "env_mmc.h"
#include "envutils.h"

static char env_flag[10] = "\0";

int envutils_init() {
    char *buffer;
    int ret = -1;
    FILE *fp;
    char *p1;
    char *p2;

    if (NULL == (fp = fopen("/proc/cmdline", "r"))) {
        printf("open proc/cmdline error!\n");
        return ret;
    }

    buffer = (char *)malloc(BOOTARGS_LENGTH * sizeof(char));
    if (NULL == buffer) {
        printf("malloc memory error!\n");
        fclose(fp);
        return ret;
    }
    memset(buffer, 0, BOOTARGS_LENGTH);
    fseek(fp, 0L, SEEK_SET);
    fread(buffer, BOOTARGS_LENGTH, 1, fp);

    p1 = strstr(buffer, "ENV=");

    if (NULL == p1) {
        return ret;
    }
    p2 = strchr(p1, ' ');

    memcpy(env_flag, p1 + strlen("ENV="), p2 - (p1 + strlen("ENV=")));
    if (0 == strcmp(env_flag, "EMMC")) {
        printf("ENV in EMMC!\n");
        envmmc_init();
    } else if (0 == strcmp(env_flag, "SPI")) {
        envspi_init();
    }
    free(buffer);
    ret = 0;
    fclose(fp);
    return ret;
}

int envutils_save() {
    int ret = -1;
    if (0 == strcmp(env_flag, "EMMC")) {
        ret = envmmc_save();
    } else if (0 == strcmp(env_flag, "SPI")) {
        ret = envspi_save();
    }
    return ret;
}

int envutils_set(const char* name, const char* value) {
    int ret = -1;
    if (0 == strcmp(env_flag, "EMMC")) {
        ret = envmmc_set(name, value);
    } else if (0 == strcmp(env_flag, "SPI")) {
        ret = envspi_set(name, value);
    }
    return ret;
}

int envutils_get(const char* name, char* array, int array_size) {
    int ret = -1;
    if (0 == strcmp(env_flag, "EMMC")) {
        ret = mmcenv_get(name, array, array_size);
    } else if (0 == strcmp(env_flag, "SPI")) {
        array = envspi_get(name);
        ret = 0;
    }
    return ret;
}

unsigned char envutils_set2bootargs(const char* name,const char* value) {
    if (NULL == name|| NULL == value)
        return -1;

    int reslen = (strlen(name) + strlen(value) + 1);
    char* pResolution = (char*)malloc(reslen);

    if(NULL == pResolution) {
        printf("%s: Error: ou of memory, at %d\n", __func__, __LINE__);
        return -1;
    }
    envutils_init();

    snprintf(pResolution, reslen, "%s%s", name, value);

    char pArgs[1024];
    memset(pArgs, 0, 1024);
    envutils_get("bootargs", pArgs, 1024 * sizeof(char));

    if (NULL != pArgs) {
        int len = strlen(pArgs);
        char* pOldArgs = (char*)malloc(len + 1);
        if(NULL == pOldArgs) {
            printf("%s: Error: ou of memory, at %d\n", __func__, __LINE__);
            free(pResolution);
            return -1;
        }
        strcpy(pOldArgs, pArgs);

        //if resolution exist, delete it.
        char* pPreEnv = strstr(pOldArgs, name);
        if (NULL != pPreEnv) {
            char* pPreEnvEnd = strchr(pPreEnv, ' ');
            if (NULL != pPreEnvEnd) {
                int remain = len - (++pPreEnvEnd - pOldArgs);
                if (remain > 0) {
                    char* pRemainBuf = (char*)malloc(remain + 1);
                    if(NULL == pRemainBuf) {
                        printf("%s: Error: ou of memory, at %d\n", __func__, __LINE__);
                        free(pOldArgs);
                        free(pResolution);
                        return false;
                    }
                    strcpy(pRemainBuf, pPreEnvEnd);
                    strcpy(pPreEnv, pRemainBuf);
                    free(pRemainBuf);
                } else {
                    *pPreEnv = 0;
                }
            } else {
                *pPreEnv = 0;
            }
        }

        const int NewArgsLen = len + 1+1 + reslen;
        char* pNewArgs = (char*)malloc(NewArgsLen);
        if (NULL == pNewArgs) {
            printf("%s: Error: ou of memory, at %d\n", __func__, __LINE__);
            free(pOldArgs);
            free(pResolution);
            return -1;
        }
        memset(pNewArgs, 0 , NewArgsLen);
        int templen = strlen(pOldArgs) - 1;
        if( (templen >= 1) && (templen < len) ) {
            if (pOldArgs[templen] != ' ') {
                snprintf(pNewArgs, NewArgsLen, "%s %s ", pOldArgs, pResolution);
            } else {
                snprintf(pNewArgs, NewArgsLen, "%s%s ", pOldArgs, pResolution);
            }
        }
        envutils_set("bootargs", pNewArgs);
        free(pNewArgs);
        free(pOldArgs);
    }

    envutils_save();
    free(pResolution);
    return 0;
}