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
#include "env_mmc.h"

static bool g_init = false;

static env_info* g_env_info;
static uint32_t g_env_size;
static char g_env_buff[65536];

static int env_match(const char* s1, int i2) {
    char* data = g_env_info->data;

    while (*s1 == data[i2++]) {
        if (*s1++ == '=') {
            return(i2);
        }
    }

    if ((*s1 == '\0') && (data[i2-1] == '=')) {
        return(i2);
    }

    return(-1);
}

int envmmc_init() {
    if (g_init) {
        return -1;
    }
    env_config_mmc_init();
    bool bRet = false;
    memset(g_env_buff, 0, CFG_ENV_SIZE);
    bRet = env_config_mmc_read(CHUNK_HEADER_ITEM_ENV, 0, (uint32_t)g_env_buff, CFG_ENV_SIZE);
    g_env_info = (env_info*)g_env_buff;
    if (bRet == false) {
        return false;
    }
    g_env_size = CFG_ENV_SIZE - sizeof(uint32_t);
    //printf("ENV CONFTENT= \n");
#define ENV_DEBUG_NUM 0x100
    uint32_t i;
    for (i =0 ;i < ENV_DEBUG_NUM; i++) {
        if ((i%16) == 0) {
            printf(" --\n");
        }
        printf("%2x ", g_env_buff[i]);
    }
    printf("\n");

    g_init = true;
    return true;
}

int envmmc_save() {
    if (!g_init) {
        return -1;
    }

    g_env_info->crc = env_config_crc32(0, g_env_info->data, g_env_size);
    bool bRet = false;
    bool bRetback = false;
    bRet = env_config_mmc_write(CHUNK_HEADER_ITEM_ENV, 0, (uint32_t)g_env_buff, CFG_ENV_SIZE , false);
    bRetback = env_config_mmc_write(CHUNK_HEADER_ITEM_ENV, (uint32_t)CFG_ENV_SIZE, (uint32_t)g_env_buff, CFG_ENV_SIZE , false);
    if ((bRet == true) && (bRetback == true)) {
        return 0;
    } else {
        return -1;
    }
}

int mmcenv_get(const char* name, char* array, int array_size) {
    if (!g_init) {
        return -1;
    }

    char* data = g_env_info->data;

    int i, nxt;
    for (i = 0; data[i] != '\0'; i = nxt + 1) {
        int val;
        for (nxt = i; data[nxt] != '\0'; ++nxt) {
            if (nxt >= (&g_env_buff[CFG_ENV_SIZE] - data))
                return -1;
        }

        val = env_match(name, i);
        if (val < 0) {
            continue;
        }
        strncpy(array,&data[val],array_size);
        return true;
    }

    return 0;
}

int envmmc_set(const char* name, const char* value) {
    if (!g_init) {
        return -1;
    }

    int len, oldval;
    char *env, *nxt = NULL;
    char *env_data = g_env_info->data;

    if (!env_data) {
        return -1;
    }

    if (strchr(name, '=')) {
        return -1;
    }

    /*
     * search if variable with this name already exists
     */
    oldval = -1;
    for (env = env_data; *env != '\0' ; env = nxt + 1) {
        for (nxt = env; *nxt != '\0' ; ++nxt) {
            ;
        }

        oldval = env_match(name, env - env_data);
        if (oldval >= 0) {
            break;
        }
    }

    /*
     * Delete any existing definition
     */
    if (oldval >= 0) {
        /*
         * Ethernet Address and serial# can be set only once,
         * ver is readonly.
         */
        if (*++nxt == '\0') {
            if (env > env_data) {
                env--;
            } else {
                *env = '\0';
            }
        } else {
            for (;;) {
                *env = *nxt++;
                if ((*env == '\0') && (*nxt == '\0')) {
                    break;
                }
                ++env;
            }
        }
        *++env = '\0';
    }

    /* Delete only ? */
    if (!value) {
        return true;
    }

    /*
     * Append new definition at the end
     */
    for (env = env_data; *env || *(env + 1); ++env) {
        ;
    }

    if (env > env_data) {
        ++env;
    }

    /*
     * Overflow when:
     * "name" + "=" + "val" +"\0\0"  > ENV_SIZE - (env-env_data)
     */
    len = strlen(name) + 2;
    /* add '=' for first arg, ' ' for all others */
    len += strlen(value) + 1;
    if (len > (&g_env_buff[CFG_ENV_SIZE] - env)) {
        //printf("## Error: environment overflow, \"%s\" deleted\n", name);
        return false;
    }

    while ((*env = *name++) != '\0') {
        env++;
    }

    *env =  '=';
    while ((*++env = *value++) != '\0') {
        ;
    }

    /* end is marked with double '\0' */
    env++;
    *env = '\0';
    return true;
}