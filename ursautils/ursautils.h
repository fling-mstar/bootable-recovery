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
#ifndef URSA_UTILS_H
#define URSA_UTILS_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
typedef enum {
    FlashEnterIspModeErr,
    FlashBlankingErr,
    FlashVerifyErr,
    FlashIDErr,
    FlashProgOK,
} FlashProgErrorType;

typedef enum {
    FlashProgStateInit,
    FlashProgStateEnterIsp,
    FlashProgStateErase,
    FlashProgStateBlank,
    FlashProgStateProgram,
    FlashProgStateVerify,
    FlashProgStateExit,
    FlashProgStateIdle,
} FlashProgStateType;

typedef enum {
    Flash_PMC512,//PMC
    Flash_PMC010,
    Flash_PMC020,
    Flash_PMC040,
    Flash_EN25P10,//EON
    Flash_EN25P20,
    Flash_EN25P40,
    Flash_EN25P80,
    Flash_EN25F40,
    Flash_EN25F80,
    Flash_EN25F16,
    Flash_EN25P16,
    Flash_EN25B20,
    Flash_EN25B40,
    Flash_EN25B40T,
    Flash_EN25B16,
    Flash_EN25B16T,
    Flash_EN25B32,
    Flash_S25FL004A,//SPANSION
    Flash_S25FL016A,
    Flash_S25FL040A,
    Flash_NX25P16,//Winbond & NX
    Flash_W25X10,//not test
    Flash_W25X20,//not test
    Flash_W25X40,//not test
    Flash_W25P20,//not test
    Flash_W25P40,
    Flash_W25X16,
    Flash_W25X32,
    Flash_W25X64,
    Flash_SST25VF016B,//SST
    Flash_SST25VF040B,//not test
    Flash_MX25L4005A,//MX
    Flash_MX25L8005,//MX
    Flash_MX25L2005,//not test
    Flash_MX25L1605,
    Flash_MX25L3205,
    Flash_MX25L6405,
    Flash_QB25F160S,//intel QB25F160S33B8
    Flash_QB25F320S,//intel QB25F320S33B8
    Flash_QB25F640S,//intel QB25F640S33B8
    Flash_A25L40P,//AMIC A25L40P
    Flash_GD25Q32,//GD
    Flash_A25L016,
    Flash_NUMS,
} FlashType;

int32_t update_ursa(char* filename);
#ifdef __cplusplus
}
#endif

#endif  // URSA_UTILS_H