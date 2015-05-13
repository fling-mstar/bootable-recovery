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

#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include "gb2313_unicode.h"
#include <stdlib.h>
#define UNICODE
#define _UNICODE
static unsigned short Unicode2GBcode(unsigned short iUnicode) {
    int i, j, n;
    switch (iUnicode) {
    case 0x0002:
        return 0x24;
        break;
    case 0x000a:
        return 0xa;
        break;
    case 0x000d:
        return 0xd;
        break;
    case 0x0040:
        return 0xA1;
        break;
    }

    if ((iUnicode >= 0x20 && iUnicode <= 0x5a) || (iUnicode >= 0x61 && iUnicode
            <= 0x7e))
        return iUnicode;

    int low, high, mid;
    n = sizeof(Unicode_GB2312) / sizeof(Unicode_GB2312[0]) - 1;
    low = 0;
    high = n;
    while (low <= high) {
        mid = (low + high) / 2;
        if (Unicode_GB2312[mid][0] == iUnicode) {
            return Unicode_GB2312[mid][1];
        }
        if (iUnicode > Unicode_GB2312[mid][0])
            low = mid + 1;
        else
            high = mid - 1;
    }

    return 0;

    for (i = 0, j = 0, n = sizeof(Unicode_GB2312) / sizeof(Unicode_GB2312[0])
            - 1; n > 0; n >>= 1, ++j) {
        if (Unicode_GB2312[i][0] == iUnicode) {
            printf("\nUnicode_GB2312[%d][1]=[%x]", i, Unicode_GB2312[i][1]);
            return Unicode_GB2312[i][1];
        }

        if (j > 1) {
            if (Unicode_GB2312[i - 1][0] == iUnicode)
                return Unicode_GB2312[i - 1][1];
            if (Unicode_GB2312[i + 1][0] == iUnicode)
                return Unicode_GB2312[i + 1][1];
        }

        if (Unicode_GB2312[i][0] < iUnicode)
            i = i + n;
        else
            i = i - n;
    }

    if (Unicode_GB2312[i][0] == iUnicode)
        return Unicode_GB2312[i][1];
    if (Unicode_GB2312[i - 1][0] == iUnicode)
        return Unicode_GB2312[i - 1][1];
    if (Unicode_GB2312[i + 1][0] == iUnicode)
        return Unicode_GB2312[i + 1][1];
    return 0;
}

/*convert Unicde string to GB£¬return  number of Chinese font*/
int strUnicode2GB(const unsigned char *strSourcer, char *strDest, int n) {
    char cTmp;
    unsigned short hz, tmphz;

    const unsigned char *pSrc;
    char *pDest;
    int i;

    for (i = 0, pSrc = strSourcer, pDest = strDest; n > 0; n -= 2, pSrc += 2, ++i, ++pDest) {
        hz = 0;
        hz = *pSrc << 8 | (*(pSrc + 1) & 0x00FF);
        tmphz = Unicode2GBcode(hz);

        if (!tmphz || (tmphz > 0x7F && tmphz < 0xFF)) {
            *pDest = '.';
            ++pDest;
            *pDest = '.';
            continue;
        } else if (tmphz > 0x00 && tmphz <= 0x7F) {
            cTmp = tmphz;
            *pDest = cTmp;
        } else {
            cTmp = tmphz;
            *pDest = (tmphz >> 8);
            ++pDest;
            *pDest = cTmp;
        }
    }

    *pDest = '\0';
    return i;
}
int strGB2Unicode(const char *str, unsigned char *dst) {

    if (!str)
        return -1;
    int len = strlen(str);
    int j = 0;
    int k = 0;
    int i = 0;
    unsigned char *s = dst;
    unsigned short int tmp = 0;
    while (i < len) {
        if ((unsigned char) str[i] < 0xa1) {
            s[j++] = 0x0;
            s[j++] = str[i++];
        } else {
            tmp = str[i] & 0xff;
            tmp = ((tmp << 8) & 0xff00) | (str[i + 1] & 0xff);
            i += 2;
            for (k = 0; k < sizeof(Unicode_GB2312) / sizeof(Unicode_GB2312[0])
                    - 1; k++) {
                if (tmp == Unicode_GB2312[k][1]) {
                    s[j++] = (Unicode_GB2312[k][0] >> 8) & 0xff;
                    s[j++] = Unicode_GB2312[k][0] & 0xff;
                    break;
                }
            }
            /*DONOT FOUND*/
            if (k == sizeof(Unicode_GB2312) / sizeof(Unicode_GB2312[0]) - 1) {
                printf("DONOT FOUND \n");
                return 0;
                }
        }
    }
    s[j] = '\0';
    return j;
}

int UTF2UnicodeOne(const char* utf8, wchar_t &wch) {
    unsigned char firstCh = utf8[0];
    int k;
    if(firstCh >= 0xC0) {
        //according the first char to judge that char is how much code of UTF-8
        int afters,code;
        if((firstCh & 0xE0) == 0xC0) {
            afters = 2;
            code = firstCh & 0x1F;
        } else if((firstCh & 0xF0) == 0xE0) {
            afters = 3;
            code = firstCh & 0xF;
        } else if((firstCh & 0xF8) == 0xF0) {
            afters = 4;
            code = firstCh & 0x7;
        } else if((firstCh & 0xFC) == 0xF8) {
            afters = 5;
            code = firstCh & 0x3;
        } else if((firstCh & 0xFE) == 0xFC) {
            afters = 6;
            code = firstCh & 0x1;
        } else {
            wch= firstCh;
            return 1;
        }

        //now need to check the char if failure means
        //the coding of utf8 is wrong after know the numbers of bytes
        for (k = 1; k < afters; k++) {
            if ((utf8[k] & 0xC0) != 0x80) {
                //judging result is failure return the value of ASCII
                wch = firstCh;
                return 1;
            }

            code <<= 6;
            code |= (unsigned char)utf8[k] & 0x3F;
        }
        wch = code;
        return afters;
    } else {
        wch = firstCh;
    }

    return 1;
}

int UTF2Unicode(const char* utfBuf, wchar_t *pUnicode) {
    int len = strlen(utfBuf);
    int i = 0,count = 0;
    wchar_t w_Unicode[30];

    memset(w_Unicode,0,30*sizeof(wchar_t));
    while(i < len) {
        i += UTF2UnicodeOne(utfBuf + i,w_Unicode[count]);
        count++;
    }
    wcsncpy(pUnicode,w_Unicode,count);
    pUnicode[count] = '\0';
    return count;
}