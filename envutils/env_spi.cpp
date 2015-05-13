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

#include <fcntl.h>
#include <unistd.h>
#include "env_spi.h"
#include "env_config.h"
#include "drvSERFLASH.h"

/// The path to read boot cmd from Mboot
#define PROC_CMD_LINE               "/proc/cmdline"
/// The spi protect process capability configuration
#define SPI_PROTECT_PROCESS_PREFIX  "SPI_PROTECT_PROCESS="
/// The spi protect process capability enable
#define SPI_PROTECT_PROCESS_ON      "ON"
/// The spi protect process capability disable
#define SPI_PROTECT_PROCESS_OFF     "OFF"

static bool g_init = false;

static env_info* g_env_info;
static char g_env_buff[CFG_ENV_SIZE];
static uint32_t g_env_addr = 0;
static uint32_t g_erase_size = 0;
static uint32_t g_env_size = 0;

static int env_match(const char* s1, int i2) {
    char* data = g_env_info->data;

    if (NULL == data) {
        return -1;
    }

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

static bool is_write_protect_restrict(void) {
    static bool bCapabilityRead = false;
    static bool bWriteProtectRestrict = false;
    if (true == bCapabilityRead) {
        return bWriteProtectRestrict;
    }
    int fd = open(PROC_CMD_LINE, O_RDONLY);
    if (fd >= 0) {
        char buf[2048] = {0};
        buf[2047] = '\0';
        if (read(fd, buf, 2047) >= 0) {
            char* pEnv = strstr(buf, SPI_PROTECT_PROCESS_PREFIX);
            if (pEnv) {
                char* pTypeStart = strchr(pEnv, '=');

                if (pTypeStart) {
                    char* pTypeEnd = strchr(pEnv, ' ');

                    if (pTypeEnd) {
                        *pTypeEnd = 0;
                    }

                    ++pTypeStart;

                    if (0 == strncmp(pTypeStart, SPI_PROTECT_PROCESS_ON, strlen(SPI_PROTECT_PROCESS_ON))) {
                        bWriteProtectRestrict = true;
                    } else {
                        bWriteProtectRestrict = false;
                    }
                }
            }
        }
        close(fd);
    }
    bCapabilityRead = true;
    return bWriteProtectRestrict;
}

static int write_flash(uint32_t addr, char* src, uint32_t size) {
    int ret = -1;

    if (!is_write_protect_restrict()) {

        if (!MDrv_SERFLASH_WriteProtect(false)) {
            //ASSERT(0);
            return -1;
        }
    }

    if (size == CFG_ENV_SIZE) {
        if (MDrv_SERFLASH_AddressErase(addr, size, true)) {
            if (MDrv_SERFLASH_Write(addr, size, reinterpret_cast<unsigned char*>(src))) {
                ret = 0;
            }
        }
    } else {
        if (MDrv_SERFLASH_SectorErase(addr, addr+size-1)) {
            if (MDrv_SERFLASH_Write(addr, size, reinterpret_cast<unsigned char*>(src))) {
                ret = 0;
            }
        }
    }

    if (!is_write_protect_restrict()) {
        if (!MDrv_SERFLASH_WriteProtect(true)) {

            return -1;
        }
    }

    return ret;
}

int envspi_init() {
    if (g_init) {
        return 0;
    }

    g_env_info = reinterpret_cast<env_info*>(g_env_buff);

    MDrv_SERFLASH_Init();

    const SERFLASH_Info* pSPIFlashInfo = MDrv_SERFLASH_GetInfo();

    if (!pSPIFlashInfo) {
        return -1;
    }

    if (pSPIFlashInfo->u32TotalSize == 0x80000) {
        g_erase_size = 0x1000; //4K
    } else {
        g_erase_size = CFG_ENV_SIZE; //64K
    }

    g_env_addr = pSPIFlashInfo->u32TotalSize - (g_erase_size << 1);
    g_env_size = g_erase_size - sizeof(uint32_t);

    if ((!MDrv_SERFLASH_Read(g_env_addr, g_erase_size, (uint8_t*)g_env_buff))
        || (g_env_info->crc != env_config_crc32(0, g_env_info->data, g_env_size))) {

        if (!MDrv_SERFLASH_Read(g_env_addr + g_erase_size, g_erase_size, (uint8_t*)g_env_buff)) {
            return -1;
        }

        if (g_env_info->crc != env_config_crc32(0, g_env_info->data, g_env_size)) {
            return -1;
        }
    }

    g_init = true;
    return 1;
}

int envspi_save() {
    if (!g_init) {
        return -1;
    }

    g_env_info->crc = env_config_crc32(0, g_env_info->data, g_env_size);

    int ret = write_flash(g_env_addr, g_env_buff, g_erase_size);

    int retBak = write_flash(g_env_addr + g_erase_size, g_env_buff, g_erase_size);

    if ((ret == 0) && (retBak == 0)) {
        return 0;
    } else {
        return -1;
    }
}

int envspi_set(const char* name, const char* value) {
    //mapi_scope_lock(scopeLock, &m_Mutex_inner);

    if (!g_init) {
        printf("Env Not init!!\n");
        return -1;
    }

    int len, oldval;
    char *env, *nxt = NULL;

    char *env_data = g_env_info->data;

    if (!env_data) {
        return -1;
    }

    if (strchr(name, '=')) {
        printf("## Error: illegal character '=' in variable name \"%s\"\n", name);
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
        return 0;
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
        printf("## Error: environment overflow, \"%s\" deleted\n", name);
        return -1;
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

    return 0;
}

const char* envspi_get(const char* name) {
    //mapi_scope_lock(scopeLock, &m_Mutex_inner);

    if (!g_init) {
        printf("Env Not init!!");
        return NULL;
    }

    char* data = g_env_info->data;
    if (NULL == data) {
        return NULL;
    }

    int i, nxt;

    for (i = 0; data[i] != '\0'; i = nxt + 1) {
        int val;

        for (nxt = i; data[nxt] != '\0'; ++nxt) {
            if (nxt >= (&g_env_buff[CFG_ENV_SIZE] - data)) {
                return(NULL);
            }
        }

        val = env_match(name, i);
        if (val < 0) {
            continue;
        }

        return(&data[val]);
    }

    return(NULL);
}