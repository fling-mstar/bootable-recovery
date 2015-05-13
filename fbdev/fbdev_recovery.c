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

#define ALOGV(...) fprintf(stdout, "V:" __VA_ARGS__)
#define ALOGI(...) fprintf(stdout, "I:" __VA_ARGS__)
#define ALOGD(...) fprintf(stdout, "D:" __VA_ARGS__)
#define ALOGW(...) fprintf(stdout, "W:" __VA_ARGS__)
#define ALOGE(...) fprintf(stderr, "E:" __VA_ARGS__)

#include "stdbool.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <linux/fb.h>
#include <cutils/properties.h>
#include <iniparser.h>
#include <mmap.h>
#include <MsCommon.h>
#include <MsOS.h>
#include <drvMMIO.h>
#include <drvSEM.h>
#include <drvGPIO.h>
#include <drvXC_IOPort.h>
#include <apiXC.h>
#include <apiXC_Ace.h>
#include <apiGFX.h>
#include <apiGOP.h>
#include <apiPNL.h>
#include <drvSERFLASH.h>
#include <assert.h>
#include "fbdev_recovery.h"
#include <tvini.h>
struct BufferInfo {
    int width;
    int height;
    int lineLength;
    unsigned int physicAddr;
    unsigned int backupBufferPhyAddr;
    int backupFBID;
};

typedef struct Region_st {
    int x;
    int y;
    int width;
    int height;
} Region;

typedef struct TimingInfo_st {
    int width;
    int height;
}TimingInfo;

typedef enum {
    E_PANEL_FULLHD=1,
    E_PANEL_4K2K_CSOT,
    E_PANEL_FULLHD_URSA6,
    E_PANEL_4K2K_URSA6,
    E_PANEL_FULLHD_URSA7,
    E_PANEL_4K2K_URSA7,
    E_PANEL_FULLHD_URSA8,
    E_PANEL_4K2K_URSA8,
    E_PANEL_FULLHD_URSA9,
    E_PANEL_4K2K_URSA9,
    E_PANEL_DEFAULT,
}EN_PANEL_TYPE;

static int andGOPdest= -1;
static bool bOSDRegionChanged = false;
static TimingInfo pre_OPTimingInfo;
static TimingInfo cur_OPTiming;
static TimingInfo pre_OCTimingInfo;
static TimingInfo cur_OCTiming;

static int fake_fd = -100; /* -1 is used in mmap, so use -100 instead */
static void* fb_mem_mapped;
static int gwin = 0;
static int gop_and = 0;
static int gfmt = E_MS_FMT_ABGR8888;
static int mirrorMode = 0;
static int mirrorModeH = 0;
static int mirrorModeV = 0;
static int panelWidth = 1920, panelHeight = 1080, panelHStart = 112;
static int panelWidth4k2k = 3840, panelHeight4k2k = 2160, panelHStart4k2k = 112;
static int osdWidth = 1920, osdHeight = 1080;
static int gPanelMode = 0;
static int bPanelDualPort = 1;
static int gSafeFbQueueCnt = 1; //SafeFbQueueCnt should be always be bufferCnt-1 to balance performance and tearing problem.
static MS_U32 u32SubAddrOffset = 0;
static MS_U8 u8MainFbId = 0xff;
static MS_U8 u8SubFbId = 0xff;
static MS_BOOL bEnableCropUiFeature = DISABLE;
static MS_U32 u32CropUiAddrOffset = 0;

static MS_BOOL bLRSwitchFlag = FALSE;    //3D OSD LR switch

static int ursaVsersion = 0;
static MS_U8 gFbId4k2k = -1;
static MS_U8 gMiu4k2kSel = -1;

static MS_U8 Androidfbid = 0xff;
static MS_U32 u32FBLength = 0;
static MS_BOOL resChanged = false;
static int lastBufferSize= 0;
static struct BufferInfo LastBuffer;
#define GOP_TIMEOUT_CNT1 100
#define CO_BUFFER_MUTEX 1
#define CHUNK_NUM 8

#define DISPLAYMODE_NORMAL                          0
#define DISPLAYMODE_LEFTRIGHT                       1
#define DISPLAYMODE_TOPBOTTOM                       2
#define DISPLAYMODE_TOPBOTTOM_LA                    3
#define DISPLAYMODE_NORMAL_LA                       4
#define DISPLAYMODE_TOP_LA                          5
#define DISPLAYMODE_BOTTOM_LA                       6
#define DISPLAYMODE_LEFT_ONLY                       7
#define DISPLAYMODE_RIGHT_ONLY                      8
#define DISPLAYMODE_TOP_ONLY                        9
#define DISPLAYMODE_BOTTOM_ONLY                     10
#define DISPLAYMODE_LEFTRIGHT_FR                    11 //Duplicate Left to right and frame sequence output
#define DISPLAYMODE_NORMAL_FR                       12 //Normal frame sequence output
#define DISPLAYMODE_NORMAL_FP                       13
#define DISPLAYMODE_MAX                             14

#define PANELMODE_NORMAL                            0
#define PANELMODE_4KX2K_2KX1K                       1
#define PANELMODE_4KX2K_4KX1K                       2
#define PANELMODE_4KX2K_4KX2K                       3

#define GOP_PIXEL_ALIGNMENT_FACTOR                  0x0F
#define GOP_PIXEL_ALIGNMENT_SHIFT                   4UL

#define DFBRC_INI_PATH_FILENAME        "/config/dfbrc.ini"
#define  MSTAR_DESK_ENABLE_HWCURSOR       "mstar.desk-enable-hwcursor"
#include <pthread.h>
#define ALIGNMENT( value, base ) (((value) + ((base) - 1)) & ~((base) - 1))

typedef struct {
    MS_U8 u8LSideFBID;
    MS_U8 u8LSideGOPNum;
    MS_U8 u8LSideGwinID;
    MS_U8 u8RSideFBID;
    MS_U8 u8RSideGOPNum;
    MS_U8 u8RSideGwinID;

    EN_GOP_DST_TYPE eGOPDst;
}AndroidFRCPropertyInfo;
static AndroidFRCPropertyInfo* qpAndroidFRCPropInfo=NULL;

static bool bNeedMonitorTiming=false;
static int gop_and_FRC = 2;
static int gwin_frc = 4;
static bool bEnableFRC = false;
static EN_PANEL_TYPE panelType = E_PANEL_DEFAULT;
#define RES_4K2K_WIDTH 3840
#define RES_4K2K_HEIGHT 2160
// check OP timing changed thread
static pthread_t gTimingCheck_tid = -1;

extern void* timingCheck_workthread(void *param);
void SetAndroidGOPDest();
void UpdateAndroidWinInfo(AndroidFRCPropertyInfo* pAndroidFRCPropInfo);

static pthread_mutex_t gFrcDest_mutex = PTHREAD_MUTEX_INITIALIZER;

static Region req_OSDRegion;
static Region cur_OSDRegion;
static MS_U32 curFBPhysicAddr = 0;
static bool bURSA_4k2kPanel = false;

static struct fb_var_screeninfo fb_vinfo = {
    .activate     = FB_ACTIVATE_NOW,
    .height       = -1,
    .width        = -1,
    .right_margin = 0,
    .upper_margin = 0,
    .lower_margin = 0,
    .vsync_len    = 4,
    .vmode        = FB_VMODE_NONINTERLACED,
};

static struct fb_fix_screeninfo fb_finfo = {
    .id    = "MSTAR VFB",
    .type  = FB_TYPE_PACKED_PIXELS,
    .accel = FB_ACCEL_NONE,
};

static int gop_4k2kphoto = 2;
static int gwin_4k2kphoto = 4;
static void* fb_4k2kphoto_mem_mapped = NULL;
static unsigned long fb_4k2kphoto_mem_addr = 0;
static unsigned long fb_4k2kphoto_mem_len = 0;
static int g_fbdev_displaymode = DISPLAYMODE_NORMAL;
static int ePanelLinkExtType = LINK_EXT;

static int getBufferCount(int width, int height);
static int stretchBlit( GFX_BufferInfo srcbuf, GFX_BufferInfo  dstbuf,GFX_Block src_rect, GFX_Block dest_rect);
static void blitLastBufferToTempBuffer();
static void blitTempBufferToFirstBuffer();
static bool ConfigureGopGwin(int osdMode,MS_U32 physicalAddr,Region OSDRegion);
static bool getCurOPTiming(int *ret_width, int *ret_height);
static bool getCurOCTiming(int *ret_width,int *ret_height);
static MS_U32 getCurrentFBPhys();
static void setCurrentFBPhys(MS_U32 physicalAddr);

// CallBack functions
static MS_U32 _SetFBFmt(MS_U16 pitch, MS_U32 addr, MS_U16 fmt) {
    // Set FB info to GE (For GOP callback fucntion use)
    GFX_BufferInfo dstbuf;

    dstbuf.u32ColorFmt = (GFX_Buffer_Format)(fmt & 0xff);
    dstbuf.u32Addr = addr;
    dstbuf.u32Pitch = pitch;
    if (MApi_GFX_SetDstBufferInfo(&dstbuf, 0) != GFX_SUCCESS) {
        ALOGE("GFX set DetBuffer failed\n");
        return 0;
    }

    return 1;
}

static MS_BOOL _XC_IsInterlace(void) {
#if 0
    XC_ApiStatus stXCStatus;

    if (MApi_XC_GetStatus(&stXCStatus, MAIN_WINDOW) == FALSE) {
        LOWE("MApi_XC_GetStatus failed(), because of InitData wrong, please update header file and compile again\n");
    }

    return stXCStatus.bInterlace;
#endif
    return 0;
}

static void _SetPQReduceBWForOSD(MS_U8 PqWin, MS_BOOL enable) {
#if 0
    MDrv_PQ_ReduceBW_ForOSD((PQ_WIN)PqWin, enable);
#endif
}

static MS_U16 _XC_GetCapwinHStr(void) {
#if 0
    MS_WINDOW_TYPE sCapwin;

    MApi_XC_GetCaptureWindow(&sCapwin, MAIN_WINDOW);

    return sCapwin.x;
#endif
    return 0x80;
}

static int gfx_GOPStretch(MS_U8 gop, MS_U16 srcWidth, MS_U16 srcHeight, MS_U16 dstWidth, MS_U16 dstHeight) {
    MS_U8 currentGOP;
    MS_U8 gopNum = MApi_GOP_GWIN_GetMaxGOPNum();
    if (gop>=gopNum) {
        ALOGE("gfx_GOPStretch was invoked invalid gop=%d",gop);
        return -1;
    }
    MApi_GOP_GWIN_BeginDraw();
    currentGOP = MApi_GOP_GWIN_GetCurrentGOP();
    MApi_GOP_GWIN_SwitchGOP(gop);

    /* GOP stretch setting */
    MApi_GOP_GWIN_Set_HSCALE(TRUE, srcWidth, dstWidth);
    MApi_GOP_GWIN_Set_VSCALE(TRUE, srcHeight, dstHeight);
    MApi_GOP_GWIN_Set_STRETCHWIN(gop, E_GOP_DST_OP0, 0, 0, srcWidth, srcHeight);

    MApi_GOP_GWIN_SwitchGOP(currentGOP);
    MApi_GOP_GWIN_EndDraw();

    return 0;
}

static EN_Gop_MuxSel  getDefaultMutex(int index) {
    switch (index) {
        case 0:
            return EN_GOP_MUX0;
        case 1:
            return EN_GOP_MUX1;
        case 2:
            return EN_GOP_MUX2;
        case 3:
            return EN_GOP_MUX3;
        case 4:
            return EN_GOP_MUX4;
        default:
            assert(0);
    }
    return EN_GOP_MUX0;
}

static int gfx_Init(dictionary* panelINI,dictionary* modelINI) {
    char property[PROPERTY_VALUE_MAX];
    GOP_InitInfo gopInitInfo;
    const mmap_info_t* mmapInfo;
    GFX_Config geConfig;
    GFX_Point clipA, clipB;
    MS_U32 addr = 0;
    EN_GOP_TILE_DATA_TYPE tileType = E_GOP_TILE_DATA_32BPP;
    GOP_GwinFBAttr fbInfo;
    GOP_MuxConfig muxConfig;
    memset(&gopInitInfo, 0, sizeof(GOP_InitInfo));
    dictionary dictInit;
    dictionary *dfbiniFile;
    memset(&dictInit, 0x00, sizeof(dictInit));
    dfbiniFile = &dictInit;
    int osdMirrorType;
    int pre_panelWidth = 1920;
    int pre_panelHeight = 1080;
    int i4K2KUI = 0;
    int newGOPdest=-1;

#ifdef BUILD_FOR_STB
#ifdef FORCE_DISPLAY_FHD
    osdWidth = 1920;
    osdHeight = 1080;
    panelWidth = 1920;
    panelHeight = 1080;
#else
    osdWidth = 1280;
    osdHeight = 720;
    panelWidth = 1280;
    panelHeight = 720;
#endif
#else
    osdWidth = iniparser_getint(panelINI, "panel:osdWidth", 1920);
    osdHeight = iniparser_getint(panelINI, "panel:osdHeight", 1080);
    panelWidth = iniparser_getint(panelINI, "panel:m_wPanelWidth", 1920);
    panelHeight = iniparser_getint(panelINI, "panel:m_wPanelHeight", 1080);
#endif
    panelHStart = iniparser_getint(panelINI, "panel:m_wPanelHStart", 112);
    mirrorMode = iniparser_getboolean(modelINI, "MISC_MIRROR_CFG:MIRROR_OSD", 0);
    osdMirrorType = iniparser_getint(modelINI, "MISC_MIRROR_CFG:MIRROR_OSD_TYPE", 0);
    bLRSwitchFlag = iniparser_getint(panelINI, "panel:b3DOSDLRSwitchFlag", 0);    //3D OSD lR switch
    bEnableCropUiFeature = iniparser_getint(panelINI, "panel:bEnableCropUiFeature", 0);
    bPanelDualPort = iniparser_getint(panelINI, "panel:m_bPanelDualPort", 1);
    ePanelLinkExtType = iniparser_getint(panelINI, "panel:m_ePanelLinkExtType", 1);
    gPanelMode = iniparser_getint(panelINI, "panel:panelMode", 0);
    i4K2KUI = iniparser_getint(panelINI, "panel:4k2kUI", 0);
#ifdef BUILD_FOR_STB
    panelHStart=274;
#endif
    // in 4K2K UI case, panel width/height can't be used to judge the GOP buffer size, should be refine later
    if (i4K2KUI == 1) {
        pre_panelWidth = panelWidth;
        pre_panelHeight = panelHeight;
        panelWidth = osdWidth;
        panelHeight = osdHeight;
    }

    ALOGD("4K2KUI Enable = %d\n",i4K2KUI);
    switch (ursaVsersion) {
        case -1:
        case 0:
            {
                if ((panelWidth==RES_4K2K_WIDTH)&&(panelHeight==RES_4K2K_HEIGHT)) {
                    panelType = E_PANEL_4K2K_CSOT;
                    bNeedMonitorTiming = true;
                } else {
                    panelType = E_PANEL_FULLHD;
                    bNeedMonitorTiming = false;
                }
                break;
            }
        case 6:
            {
                if ((panelWidth==RES_4K2K_WIDTH)&&(panelHeight==RES_4K2K_HEIGHT)) {
                    panelType = E_PANEL_4K2K_URSA6;
                    bNeedMonitorTiming = true;
                } else {
                    panelType = E_PANEL_FULLHD_URSA6;
                    bNeedMonitorTiming = true;
                }
                break;
            }
        case 7:
            {
                if ((panelWidth==RES_4K2K_WIDTH)&&(panelHeight==RES_4K2K_HEIGHT)) {
                    panelType = E_PANEL_4K2K_URSA7;
                    bNeedMonitorTiming = true;
                } else {
                    panelType = E_PANEL_FULLHD_URSA7;
                    bNeedMonitorTiming = true;
                }
                break;
            }
        case 8:
            {
                if ((panelWidth==RES_4K2K_WIDTH)&&(panelHeight==RES_4K2K_HEIGHT)) {
                    panelType = E_PANEL_4K2K_URSA8;
                    bNeedMonitorTiming = true;
                } else {
                    panelType = E_PANEL_FULLHD_URSA8;
                    bNeedMonitorTiming = true;
                }
                break;
            }
        case 9:
            {
                if ((panelWidth==RES_4K2K_WIDTH)&&(panelHeight==RES_4K2K_HEIGHT)) {
                    panelType = E_PANEL_4K2K_URSA9;
                    bNeedMonitorTiming = true;
                } else {
                    panelType = E_PANEL_FULLHD_URSA9;
                    bNeedMonitorTiming = true;
                }
                break;
            }
        default :
            panelType = E_PANEL_DEFAULT;
            bNeedMonitorTiming = true;
    }

    if ( mirrorMode == 1) {
        switch (osdMirrorType) {
            case 1:
                mirrorModeH = 1;
                mirrorModeV = 0;
                break;
            case 2:
                mirrorModeH = 0;
                mirrorModeV = 1;
                break;
            case 3:
                mirrorModeH = 1;
                mirrorModeV = 1;
                break;
            default:
                //if OSD mirror enable but not setting mirror type, then setting HV-mirror.
                ALOGE("Mirror Type = %d. it is incorrect! repair to HV-mirror\n",osdMirrorType);
                mirrorModeH = 1;
                mirrorModeV = 1;
                osdMirrorType = 3;
                break;
        }
    } else {
        mirrorModeH = 0;
        mirrorModeV = 0;
    }

    ALOGD("OSD Mirror Enable = %d, Mirror Type = %d\n",mirrorMode,osdMirrorType);

    if (property_get("mstar.gop", property, NULL) > 0)
        gop_and = atoi(property);
    if (property_get("mstar.gwin", property, NULL) > 0)
        gwin = atoi(property);

    mmapInfo = mmap_get_info("E_MMAP_ID_GOP_FB");
    if (mmapInfo != NULL) {
        addr = gopInitInfo.u32GOPRBAdr = GE_ADDR_ALIGNMENT(mmapInfo->addr);
        gopInitInfo.u32GOPRBLen = mmapInfo->size;
        u32FBLength = mmapInfo->size;
    }

    mmapInfo = mmap_get_info("E_MMAP_ID_GOP_REGDMA");
    if (mmapInfo != NULL) {
        gopInitInfo.u32GOPRegdmaAdr = GE_ADDR_ALIGNMENT(mmapInfo->addr);
        gopInitInfo.u32GOPRegdmaLen = mmapInfo->size;
        gopInitInfo.bEnableVsyncIntFlip = TRUE;
    }

    if (OSD_FB_FMT == FBDEV_FMT_ARGB8888) {
        fb_vinfo.bits_per_pixel = 32;
        fb_vinfo.grayscale = 0;
        fb_vinfo.transp.offset = 24;
        fb_vinfo.transp.length = 8;
        fb_vinfo.red.offset = 16;
        fb_vinfo.red.length = 8;
        fb_vinfo.green.offset = 8;
        fb_vinfo.green.length = 8;
        fb_vinfo.blue.offset = 0;
        fb_vinfo.blue.length = 8;
    } else if (OSD_FB_FMT == FBDEV_FMT_ABGR8888) {
        fb_vinfo.bits_per_pixel = 32;
        fb_vinfo.grayscale = 0;
        fb_vinfo.transp.offset = 24;
        fb_vinfo.transp.length = 8;
        fb_vinfo.red.offset = 0;
        fb_vinfo.red.length = 8;
        fb_vinfo.green.offset = 8;
        fb_vinfo.green.length = 8;
        fb_vinfo.blue.offset = 16;
        fb_vinfo.blue.length = 8;
    } else if (OSD_FB_FMT == FBDEV_FMT_ARGB4444) {
        fb_vinfo.bits_per_pixel = 16;
        fb_vinfo.grayscale = 0;
        fb_vinfo.transp.offset = 12;
        fb_vinfo.transp.length = 4;
        fb_vinfo.red.offset = 8;
        fb_vinfo.red.length = 4;
        fb_vinfo.green.offset = 4;
        fb_vinfo.green.length = 4;
        fb_vinfo.blue.offset = 0;
        fb_vinfo.blue.length = 4;
    } else {
        fb_vinfo.bits_per_pixel = 16;
        fb_vinfo.grayscale = 0;
        fb_vinfo.blue.offset = 0;
        fb_vinfo.blue.length = 5;
        fb_vinfo.green.offset = 5;
        fb_vinfo.green.length = 6;
        fb_vinfo.red.offset = 11;
        fb_vinfo.red.length = 5;
        fb_vinfo.transp.offset = 0;
        fb_vinfo.transp.length = 0;
    }

    fb_vinfo.reserved[0] = osdWidth;
    fb_vinfo.reserved[1] = osdHeight;
    if ((osdWidth > panelWidth) || (osdHeight > panelHeight)) {
        //check GOP buffer alignment
        //fb_vinfo.xres = ALIGNMENT(panelWidth, 16);
        fb_vinfo.xres = panelWidth & ~7U;
        fb_vinfo.yres = panelHeight;
        fb_vinfo.xres_virtual = panelWidth;
        fb_vinfo.yres_virtual = panelHeight * 3;
        gSafeFbQueueCnt = 2;
    } else {
        //check GOP buffer alignment
        int bufferCount = getBufferCount(osdWidth,osdHeight);
        //osdWidth = ALIGNMENT(osdWidth, 16);
        osdWidth = osdWidth & ~7U;
        fb_vinfo.xres = osdWidth;
        fb_vinfo.yres = osdHeight;
        fb_vinfo.xres_virtual = osdWidth;
        fb_vinfo.yres_virtual = osdHeight * bufferCount;

        gSafeFbQueueCnt = bufferCount - 1;
    }
    fb_vinfo.xoffset = 0;
    fb_vinfo.yoffset = 0;

    // Some dummy values for timing to make fbset happy
    fb_vinfo.pixclock = 100000000000LLU / (6 *  fb_vinfo.xres * fb_vinfo.yres);
    fb_vinfo.left_margin = 0;
    fb_vinfo.hsync_len = 0;
    qpAndroidFRCPropInfo = (AndroidFRCPropertyInfo*)malloc(sizeof(AndroidFRCPropertyInfo));
    memset(qpAndroidFRCPropInfo, 0 , sizeof(AndroidFRCPropertyInfo));
    gopInitInfo.u16PanelWidth  = panelWidth;
    gopInitInfo.u16PanelHeight = panelHeight;
    gopInitInfo.u16PanelHStr   = panelHStart;

    // Register Call Back Function for GOP use
    MApi_GOP_RegisterFBFmtCB(_SetFBFmt);
    MApi_GOP_RegisterXCIsInterlaceCB(_XC_IsInterlace);
    MApi_GOP_RegisterXCGetCapHStartCB(_XC_GetCapwinHStr);
    MApi_GOP_RegisterXCReduceBWForOSDCB(_SetPQReduceBWForOSD);

    geConfig.u8Miu = 0;
    geConfig.u8Dither = FALSE;
    geConfig.u32VCmdQSize = 0;
    geConfig.u32VCmdQAddr = 0;
    geConfig.bIsHK = 1;
    geConfig.bIsCompt = FALSE;

    MApi_GFX_Init(&geConfig);
    MApi_GOP_InitByGOP(&gopInitInfo, 1);
    MApi_GOP_InitByGOP(&gopInitInfo, gop_and);
    if (bNeedMonitorTiming) {
        //recoverymode not need FRC
        if (!(property_get("mstar.recoverymode", property, NULL) > 0) && (property_get("mstar.gopfrc", property, NULL) > 0)) {
            gop_and_FRC = atoi(property);
            if (panelType ==E_PANEL_4K2K_CSOT) {
                bEnableFRC = true;
                gwin_frc= fbdev_getAvailGwinNum_ByGop(gop_and_FRC);
                MApi_GOP_InitByGOP(&gopInitInfo, gop_and_FRC);
                ALOGD("the gop_and_frc from property: %d",gop_and_FRC);
                ALOGD("the gwin_frc:%d",gwin_frc);
            }
        }
    }

#ifdef ENABLE_CMD_QUEUE
    mmapInfo = mmap_get_info("E_MMAP_ID_CMDQ");
    if (mmapInfo != NULL) {
        ALOGD("miuno = %d, addr = %lx,size = %lx",mmapInfo->miuno,mmapInfo->addr,mmapInfo->size);
        MS_BOOL ret;
        ret = MsOS_Init();
        if (mmapInfo->miuno == 0) {
            ret = MsOS_MPool_Mapping(0, mmapInfo->addr,  mmapInfo->size, 1);
        } else if (mmapInfo->miuno == 1) {
            ret = MsOS_MPool_Mapping(1, (mmapInfo->addr - mmap_get_miu0_offset()),  mmapInfo->size, 1);
        } else if (mmapInfo->miuno == 2) {
            ret = MsOS_MPool_Mapping(2, (mmapInfo->addr - mmap_get_miu1_offset()),  mmapInfo->size, 1);
        }
        if (ret) {
            ret = MDrv_CMDQ_Init(mmapInfo->miuno);
            ret = MDrv_CMDQ_Get_Memory_Size(mmapInfo->addr,mmapInfo->addr + mmapInfo->size,mmapInfo->miuno);
        }
    } else {
        ALOGD("there is no E_MMAP_ID_CMDQ in mmap.ini");
    }
#endif

#if 0
    //protect logic
    if (addr>=mmap_get_miu0_offset) {
        ALOGD("gfx_Init E_MMAP_ID_GOP_FB is on MIU1");
        MApi_GOP_MIUSel(gop_and,E_GOP_SEL_MIU1);
    }
#endif
#ifdef BUILD_FOR_STB
    MApi_GOP_GWIN_OutputColor(GOPOUT_YUV);
#endif
    // MIRROR mode
    if (mirrorModeH == 1) {
        MApi_GOP_GWIN_SetHMirror(true);
        ALOGD("SET THE HMirror!!!!!");
    }
    if (mirrorModeV == 1) {
        MApi_GOP_GWIN_SetVMirror(true);
        ALOGD("SET THE VMirror!!!!");
    }

    memset(&muxConfig, 0, sizeof(muxConfig));
    dfbiniFile = iniparser_load(DFBRC_INI_PATH_FILENAME);
    char muxStr[] = "DirectFBRC:DFBRC_MUX0_GOPINDEX";
    if (dfbiniFile == &dictInit) {
        ALOGE("Load DFBRC_INI_PATH_FILENAME fail");
    } else {
        muxConfig.u8MuxCounts = iniparser_getunsignedint(dfbiniFile, "DirectFBRC:DFBRC_MUXCOUNTS", 0);
#if 0
        muxConfig.GopMux[0].u8MuxIndex = 0;
        muxConfig.GopMux[0].u8GopIndex = iniparser_getunsignedint(dfbiniFile, "DirectFBRC:DFBRC_MUX0_GOPINDEX", 0);
        muxConfig.GopMux[1].u8MuxIndex = 1;
        muxConfig.GopMux[1].u8GopIndex = iniparser_getunsignedint(dfbiniFile, "DirectFBRC:DFBRC_MUX1_GOPINDEX", 0);
        muxConfig.GopMux[2].u8MuxIndex = 2;
        muxConfig.GopMux[2].u8GopIndex = iniparser_getunsignedint(dfbiniFile, "DirectFBRC:DFBRC_MUX2_GOPINDEX", 0);
        muxConfig.GopMux[3].u8MuxIndex = 3;
        muxConfig.GopMux[3].u8GopIndex = iniparser_getunsignedint(dfbiniFile, "DirectFBRC:DFBRC_MUX3_GOPINDEX", 0);
#else
        int muxIdx = 0;
        for (muxIdx = 0; muxIdx < muxConfig.u8MuxCounts; muxIdx++) {
            muxConfig.GopMux[muxIdx].u8MuxIndex = getDefaultMutex(muxIdx);
            //convert integer (range from 0 to 9) to char
            //example convert 0 to '0'
            muxStr[20] = muxIdx + 0x30;
            muxConfig.GopMux[muxIdx].u8GopIndex = iniparser_getunsignedint(dfbiniFile, muxStr, 0);
        }
#endif
        MApi_GOP_GWIN_SetMux(&muxConfig,sizeof(muxConfig));
        if (bNeedMonitorTiming) {
            if (bEnableFRC) {
                muxConfig.u8MuxCounts = 2;
                muxConfig.GopMux[0].u8MuxIndex = EN_GOP_FRC_MUX2;
                muxConfig.GopMux[0].u8GopIndex = gop_and;
                muxConfig.GopMux[1].u8MuxIndex = EN_GOP_FRC_MUX3;
                muxConfig.GopMux[1].u8GopIndex = gop_and_FRC;
                MApi_GOP_GWIN_SetMux(&muxConfig,sizeof(muxConfig));
                MApi_GOP_GWIN_SetGOPDst(gop_and, E_GOP_DST_BYPASS);
                MApi_GOP_GWIN_SetGOPDst(gop_and_FRC, E_GOP_DST_BYPASS);
                qpAndroidFRCPropInfo->eGOPDst =E_GOP_DST_BYPASS;
                //disable hwcursor when dst on FRC
                property_set(MSTAR_DESK_ENABLE_HWCURSOR, "0");
            } else {
                newGOPdest = getNewGOPdest();
                MApi_GOP_GWIN_SetGOPDst(gop_and, newGOPdest);
                qpAndroidFRCPropInfo->eGOPDst =newGOPdest;
                //enable hwcursor when dst on OP
                property_set(MSTAR_DESK_ENABLE_HWCURSOR, "1");
            }
        } else {
            MApi_GOP_GWIN_SetGOPDst(gop_and, E_GOP_DST_OP0);
        }
        iniparser_freedict(dfbiniFile);
    }

    clipA.x = 0;
    clipA.y = 0;
    clipB.x = panelWidth;
    clipB.y = panelHeight;
    MApi_GFX_SetClip(&clipA, &clipB);
    if (bNeedMonitorTiming) {
        if (bEnableFRC) {
            MApi_GOP_GWIN_SwitchGOP(gop_and);
            MApi_GOP_GWIN_EnableTransClr(GOPTRANSCLR_FMT0, false);

            MApi_GOP_GWIN_SwitchGOP(gop_and_FRC);
            MApi_GOP_GWIN_EnableTransClr(GOPTRANSCLR_FMT0, false);
        } else {
            MApi_GOP_GWIN_SwitchGOP(gop_and);
            MApi_GOP_GWIN_EnableTransClr(GOPTRANSCLR_FMT0, false);
        }
    } else {
        gfx_GOPStretch(gop_and, fb_vinfo.xres, fb_vinfo.yres, panelWidth, panelHeight);
        MApi_GOP_GWIN_SwitchGOP(gop_and);
        MApi_GOP_GWIN_EnableTransClr(GOPTRANSCLR_FMT0, false);
    }
    if (property_get("mstar.framebuffer.format", property, NULL) > 0) {
        int format = atoi(property);
        if (format == 0) { // RGB565
            gfmt = E_MS_FMT_RGB565;
            tileType = E_GOP_TILE_DATA_16BPP;
        } else if (format == 1) { // ARGB4444
            gfmt = E_MS_FMT_ARGB4444;
            tileType = E_GOP_TILE_DATA_16BPP;
        } else if (format == 2) { // ARGB8888
            gfmt = E_MS_FMT_ARGB8888;
            tileType = E_GOP_TILE_DATA_32BPP;
        } else if (format == 3) { // ABGR8888
            gfmt = E_MS_FMT_ABGR8888;
            tileType = E_GOP_TILE_DATA_32BPP;
        }
    }

    //mmapInfo = mmap_get_info("E_MMAP_ID_GOP_FB");
    //strcpy(fb_finfo.id, "MSatr VFB");

/*
    gwin = MApi_GOP_GWIN_CreateWin(fb_vinfo.xres, fb_vinfo.yres, gfmt);
    //gwin = 0;
    //MApi_GOP_GWIN_CreateStaticWin(gwin, fb_vinfo.xres, fb_vinfo.yres, gfmt)
        ALOGI("Create Gwin  %d %d, %d, %d\n", gwin, fb_vinfo.xres, fb_vinfo.yres, gfmt);
    fbid = MApi_GOP_GWIN_GetFBfromGWIN(gwin);
    MApi_GOP_GWIN_GetFBInfo(fbid, &fbInfo);
    fbInfo.addr = addr;
    MApi_GOP_GWIN_SetFBInfo(fbid, &fbInfo);
*/
    Androidfbid = MApi_GOP_GWIN_GetFreeFBID();
    MApi_GOP_GWIN_CreateFBbyStaticAddr(Androidfbid, 0, 0, fb_vinfo.xres, fb_vinfo.yres, gfmt, addr);
    setCurrentFBPhys(addr);
    cur_OSDRegion.x = 0;
    cur_OSDRegion.y = 0;
    //cur_OSDRegion.width = osdWidth;
    //cur_OSDRegion.height = osdHeight;
    cur_OSDRegion.width = fb_vinfo.xres;
    cur_OSDRegion.height = fb_vinfo.yres;
    getCurOPTiming(&(cur_OPTiming.width),&(cur_OPTiming.height));
    if (bNeedMonitorTiming) {
        qpAndroidFRCPropInfo->u8LSideFBID =     Androidfbid;
        qpAndroidFRCPropInfo->u8LSideGOPNum =   gop_and;
        qpAndroidFRCPropInfo->u8LSideGwinID =   gwin;
        qpAndroidFRCPropInfo->u8RSideFBID =     Androidfbid;
        qpAndroidFRCPropInfo->u8RSideGOPNum =   (bEnableFRC==TRUE)?gop_and_FRC: gop_and;
        qpAndroidFRCPropInfo->u8RSideGwinID =   (bEnableFRC==TRUE)?gwin_frc: gwin;
        UpdateAndroidWinInfo(qpAndroidFRCPropInfo);
    } else {
        MApi_GOP_GWIN_MapFB2Win(Androidfbid, gwin);
    }
    MApi_GOP_GWIN_GetFBInfo(Androidfbid, &fbInfo);

    fb_finfo.smem_start  = fbInfo.addr;
    fb_finfo.smem_len    = fbInfo.size;
    fb_finfo.visual      = FB_VISUAL_TRUECOLOR;
    fb_finfo.line_length = fbInfo.pitch;

    MS_U8 miu_sel;
    if (fbInfo.addr >= mmap_get_miu1_offset()) {
        miu_sel = 2;
    } else if (fbInfo.addr >= mmap_get_miu0_offset()) {
        miu_sel = 1;
    } else {
        miu_sel = 0;
    }
    unsigned int maskPhyAddr =  fbdev_getMaskPhyAdr(miu_sel);
    ALOGI("gfx_Init invoked maskPhyAddr=%lx", maskPhyAddr);

    if (fbInfo.addr >= mmap_get_miu1_offset()) {
        if (!MsOS_MPool_Mapping(2, (fbInfo.addr & maskPhyAddr), ((gopInitInfo.u32GOPRBLen+4095)&~4095U), 1)) {
            ALOGE("MsOS_MPool_Mapping failed in %s:%d\n",__FUNCTION__,__LINE__);
        }
        fb_mem_mapped = (void*)MsOS_PA2KSEG1((fbInfo.addr & maskPhyAddr) | mmap_get_miu1_offset());

        ALOGI("GOP FB in miu2 0x%lx, map to 0x%lx\n", fbInfo.addr & maskPhyAddr, fb_mem_mapped);
    } else if (fbInfo.addr >= mmap_get_miu0_offset()) {
        if (!MsOS_MPool_Mapping(1, (fbInfo.addr & maskPhyAddr), ((gopInitInfo.u32GOPRBLen+4095)&~4095U), 1)) {
            ALOGE("MsOS_MPool_Mapping failed in %s:%d\n",__FUNCTION__,__LINE__);
        }
        fb_mem_mapped = (void*)MsOS_PA2KSEG1((fbInfo.addr & maskPhyAddr) | mmap_get_miu0_offset());
        ALOGI("GOP FB in miu1 0x%lx, map to 0x%lx\n", fbInfo.addr & maskPhyAddr, fb_mem_mapped);
    } else {
        if (!MsOS_MPool_Mapping(0, (fbInfo.addr & maskPhyAddr), ((gopInitInfo.u32GOPRBLen+4095)&~4095U), 1)) {
            ALOGE("MsOS_MPool_Mapping failed in %s:%d\n",__FUNCTION__,__LINE__);
        }
        fb_mem_mapped = (void*)MsOS_PA2KSEG1(fbInfo.addr & maskPhyAddr);
        ALOGI("GOP FB in miu0 0x%lx, map to 0x%lx\n", fbInfo.addr & maskPhyAddr, fb_mem_mapped);
    }

    memset(fb_mem_mapped, 0, gopInitInfo.u32GOPRBLen);
    MApi_GFX_BeginDraw();
    if (miu_sel==0) {
        MApi_GOP_MIUSel(gop_and,E_GOP_SEL_MIU0);
    } else if (miu_sel==1) {
        MApi_GOP_MIUSel(gop_and,E_GOP_SEL_MIU1);
    } else if (miu_sel==2) {
#ifdef ENABLE_CMD_QUEUE
        MApi_GOP_MIUSel(gop_and,E_GOP_SEL_MIU2);
#else
        ALOGE("gop_and select miu2 failed in %s:%d\n",__FUNCTION__,__LINE__);
#endif
    }


    {
        E_GOP_API_Result _ret;
        MS_U16 _x, _y, _width, _height;
        MS_U16 _pitch, _fmt;
        MS_U32 _addr;
        MS_U32 _size;

        _ret = MApi_GOP_GWIN_GetWinAttr(gwin, &_x, &_y, &_width, &_height, &_pitch, &_fmt, &_addr, &_size);
        if (_ret != GOP_API_SUCCESS) {
            ALOGE("MApi_GOP_GWIN_GetWinAttr() failed, ret=%d\n", _ret);
        }
        ALOGI("OSD: x=%d y=%d w=%d h=%d pitch=%d fmt=%d addr=%lx size=%lx\n",
              _x, _y, _width, _height, _pitch, _fmt, _addr, _size);
    }
    MApi_GOP_GWIN_SetBlending(gwin, true, 0x3F);

    MApi_GFX_EndDraw();
    MApi_GOP_GWIN_Enable(gwin, true);

    MApi_GOP_GWIN_Switch2Gwin(gwin); //????
    if (bNeedMonitorTiming) {
        if (bEnableFRC) {
            MApi_GOP_GWIN_SetBlending(gwin_frc, true, 0x3F);
            MApi_GOP_GWIN_Enable(gwin_frc, false);
            MApi_GOP_GWIN_Switch2Gwin(gwin_frc);
            //multi thread to check timming change
        }
        pthread_create(&gTimingCheck_tid, NULL, timingCheck_workthread, NULL);
    }

    if (i4K2KUI == 1) {
        panelWidth = pre_panelWidth;
        panelHeight = pre_panelHeight;
    }

    req_OSDRegion = cur_OSDRegion;

    ALOGI("MStar graphic Config:\n");
    ALOGI("OSD WIDTH    : %d\n", osdWidth);
    ALOGI("OSD HEIGHT   : %d\n", osdHeight);
    ALOGI("PANEL WIDTH  : %d\n", panelWidth);
    ALOGI("PANEL HEIGHT : %d\n", panelHeight);
    ALOGI("PANEL HSTART : %d\n", panelHStart );
    ALOGI("MIRROE MODE  : %d ,H : %d,V : %d\n", mirrorMode,mirrorModeH,mirrorModeV);
    ALOGI("EnableCropUiEnableFeature : %d\n", bEnableCropUiFeature);
    ALOGI("GOP          : %d\n", gop_and);
    ALOGI("GWIN         : %d\n", gwin);
    ALOGI("FORMAT       : %d\n", gfmt);
    ALOGI("TILE TYPE    : %d\n", tileType);
    return 0;
}

static MS_BOOL _Dwin_GetBuffer(unsigned long* start, unsigned long* end, int* mutexId) {
    MS_BOOL ret = FALSE;
    const mmap_info_t* mmapInfo = NULL;
    XC_INITDATA XC_InitData;
    char property[PROPERTY_VALUE_MAX];

    mmapInfo = mmap_get_info("E_DFB_JPD_WRITE");
    if (mmapInfo == NULL) {
        ALOGE("[DwinHardware]get E_DFB_JPD_WRITE buffer in mmap failed");
        return false;
    }

    if (mmapInfo->size <= 0)
        return false;

    if (mmapInfo->miuno == 0) {
        ret = MsOS_MPool_Mapping(0, mmapInfo->addr,  mmapInfo->size, 1);
    } else if (mmapInfo->miuno == 1) {
        ret = MsOS_MPool_Mapping(1, (mmapInfo->addr - mmap_get_miu0_offset()),  mmapInfo->size, 1);
    } else if (mmapInfo->miuno == 2) {
        ret = MsOS_MPool_Mapping(2, (mmapInfo->addr - mmap_get_miu1_offset()),  mmapInfo->size, 1);
    }

    if (!ret) {
        ALOGE("[DwinHardware]E_DFB_JPD_WRITE buffer Mpool mapping error mmapInfo->addr %08x mmapInfo->size %08x**************\n", (unsigned int)mmapInfo->addr, (unsigned int)mmapInfo->size);
        return FALSE;
    }

    *start = mmapInfo->addr;
    *end = *start + mmapInfo->size;

    /* wait for utopia update(implement mutex in kernel) */
#if CO_BUFFER_MUTEX
    if (property_get("dfb_jpd_write_buffer", property, NULL) > 0) {
        //ALOGD("dfb_jpd_write_buffer mutex get success");
    } else {
        snprintf(property, sizeof(property), "%s", "SkiaDecodeMutex");
    }

    *mutexId = MsOS_CreateNamedMutex((MS_S8*)property);
    if (*mutexId < 0) {
        ALOGE("[DwinHardware]: create named mutex SkiaDecodeMutex failed.");
        return false;
    }
#endif

    // xc not init.
    memset((void*)&XC_InitData, 0, sizeof(XC_InitData));
    MApi_XC_Init(&XC_InitData, 0);
    MApi_PNL_IOMapBaseInit();

    return true;
}


static int _MstarBytesPerPixel(GFX_Buffer_Format pixelForamat) {
    int bytesPerPixel = 0;;
    switch (pixelForamat) {
        case GFX_FMT_YUV422 :
            {
                bytesPerPixel = 2;
                break;
            }
        case GFX_FMT_ARGB8888 :
            {
                bytesPerPixel = 4;
                break;
            }
        default :
            {
                bytesPerPixel = 4;
                break;
            }
    }
    return bytesPerPixel;
}

static MS_BOOL _CaptureScreen(GFX_Block* Dwin_info, EN_GOP_DWIN_DATA_FMT fmt, unsigned int u32BufAddr, unsigned int u32BufSize, int mode) {
    GOP_DwinProperty dwinProperty;
    PNL_ApiExtStatus stPnlExtStatus;
    MS_BOOL bFreeze = FALSE;
    MS_U32 u32SLZCaps = 0;
#ifdef DWIN_MIU_PATCH
    MS_U16 u16OldReg21 = 0, u16OldReg41 = 0, u16OldReg51 = 0;
    MS_U16 u16OldReg24 = 0, u16OldReg34 = 0, u16OldReg44 = 0, u16OldReg54 = 0;
    MS_U16 u16OldReg20 = 0, u16OldReg30 = 0, u16OldReg40 = 0, u16OldReg50 = 0;
    MS_U16 u16OldReg05 = 0;
#endif

    memset(&stPnlExtStatus, 0, sizeof(PNL_ApiExtStatus));
    stPnlExtStatus.u16ApiStatusEX_Length = sizeof(PNL_ApiExtStatus);
    stPnlExtStatus.u32ApiStatusEx_Version = API_PNLSTATUS_EX_VERSION;
    if (!MApi_PNL_GetStatusEx(&stPnlExtStatus)) {
        ALOGE("[fbdev_read] MApi_PNL_GetStatusEx error\n");
        return FALSE;
    }

#ifndef ENABLE_4K2K_OP_DEFAULT
    if (7 == ursaVsersion && 0 == bPanelDualPort) {
        MApi_GOP_GWIN_SetGOPDst(gop_and, E_GOP_DST_OP0);
    }
#endif

    // When doing freeze, needs to check if freeze already
    if (!MApi_XC_IsFreezeImg(MAIN_WINDOW)) {

        // FIXME: it needs to check if SLZ is enable or not, wait new XC API
        MApi_XC_GetChipCaps(E_XC_HW_SEAMLESS_ZAPPING, &u32SLZCaps, sizeof(u32SLZCaps));
        if (u32SLZCaps == 1) {
            MApi_XC_SetSeamlessZapping(MAIN_WINDOW, FALSE);
        }

        MApi_XC_FreezeImg(TRUE, MAIN_WINDOW);
        bFreeze = TRUE;
    }

//4K2K dwin capture miu setting patch
#ifdef DWIN_MIU_PATCH
    u16OldReg21 = ((MS_U16)MApi_XC_ReadByte(0x101243)<<8) | MApi_XC_ReadByte(0x101242);
    u16OldReg41 = ((MS_U16)MApi_XC_ReadByte(0x101283)<<8) | MApi_XC_ReadByte(0x101282);
    u16OldReg51 = ((MS_U16)MApi_XC_ReadByte(0x1012A3)<<8) | MApi_XC_ReadByte(0x1012A2);

    u16OldReg24 = ((MS_U16)MApi_XC_ReadByte(0x101249)<<8) | MApi_XC_ReadByte(0x101248);
    u16OldReg34 = ((MS_U16)MApi_XC_ReadByte(0x101269)<<8) | MApi_XC_ReadByte(0x101268);
    u16OldReg44 = ((MS_U16)MApi_XC_ReadByte(0x101289)<<8) | MApi_XC_ReadByte(0x101288);
    u16OldReg54 = ((MS_U16)MApi_XC_ReadByte(0x1012A9)<<8) | MApi_XC_ReadByte(0x1012A8);

    u16OldReg20 = ((MS_U16)MApi_XC_ReadByte(0x101241)<<8) | MApi_XC_ReadByte(0x101240);
    u16OldReg30 = ((MS_U16)MApi_XC_ReadByte(0x101261)<<8) | MApi_XC_ReadByte(0x101260);
    u16OldReg40 = ((MS_U16)MApi_XC_ReadByte(0x101281)<<8) | MApi_XC_ReadByte(0x101280);
    u16OldReg50 = ((MS_U16)MApi_XC_ReadByte(0x1012A1)<<8) | MApi_XC_ReadByte(0x1012A0);

    u16OldReg05 = ((MS_U16)MApi_XC_ReadByte(0x120E0B)<<8) | MApi_XC_ReadByte(0x120E0A);

    MApi_XC_Write2ByteMask(0x101242, 0x0002, 0xFFFF);
    MApi_XC_Write2ByteMask(0x101282, 0x0004, 0xFFFF);
    MApi_XC_Write2ByteMask(0x1012A2, 0x0002, 0xFFFF);

    MApi_XC_Write2ByteMask(0x101248, 0xFFFF, 0xFFFF);
    MApi_XC_Write2ByteMask(0x101268, 0xFFEF, 0xFFFF);
    MApi_XC_Write2ByteMask(0x101288, 0xFFFF, 0xFFFF);
    MApi_XC_Write2ByteMask(0x1012A8, 0xFFFF, 0xFFFF);

    MApi_XC_Write2ByteMask(0x101240, 0x8015, 0xFFFF);
    MApi_XC_Write2ByteMask(0x101260, 0x8015, 0xFFFF);
    MApi_XC_Write2ByteMask(0x101280, 0x8015, 0xFFFF);
    MApi_XC_Write2ByteMask(0x1012A0, 0x8015, 0xFFFF);

    MApi_XC_Write2ByteMask(0x120E0A, 0x0004, 0xFFFF);
#endif

    MApi_GOP_GWIN_SetForceWrite(TRUE);
    if (fmt == DWIN_DATA_FMT_UV8Y8) {
        if (!stPnlExtStatus.bYUVOutput) //op output argb
            MApi_GOP_DWIN_EnableR2YCSC(ENABLE);
        MApi_GFX_ClearFrameBufferByWord(u32BufAddr, u32BufSize, 0x80108010);
    } else {
        MApi_GOP_DWIN_EnableR2YCSC(DISABLE);
        MApi_GFX_ClearFrameBufferByWord(u32BufAddr, u32BufSize, 0x0);
    }

    MApi_GOP_DWIN_Init();
    MApi_GOP_DWIN_SetSourceSel(DWIN_SRC_OP);
    MApi_GOP_DWIN_SelectSourceScanType(DWIN_SCAN_MODE_PROGRESSIVE);
    MApi_GOP_DWIN_SetDataFmt(fmt);

    if (mode == 1) {
        MApi_XC_OP2VOPDESel(E_OP2VOPDE_WHOLEFRAME);             //capture video only
    } else {
        MApi_XC_OP2VOPDESel(E_OP2VOPDE_WHOLEFRAME_WITHOSD);     //capture screen include video
    }

    dwinProperty.u16x = Dwin_info->x;
    dwinProperty.u16y = Dwin_info->y;
    dwinProperty.u16w = Dwin_info->width;
    dwinProperty.u16fbw = dwinProperty.u16w;
    dwinProperty.u16h = Dwin_info->height;
    dwinProperty.u32fbaddr0 = u32BufAddr;
    dwinProperty.u32fbaddr1 = u32BufAddr + u32BufSize;
    MApi_GOP_DWIN_SetWinProperty(&dwinProperty);
    MApi_GOP_DWIN_SetAlphaValue(0xff);

    MsOS_DelayTask(5);
    MApi_GFX_FlushQueue();

    MApi_GOP_DWIN_CaptureOneFrame();
    MsOS_DelayTask(10);
    MApi_GOP_GWIN_SetForceWrite(FALSE);

//4K2K dwin capture miu setting patch
#ifdef DWIN_MIU_PATCH
    MApi_XC_Write2ByteMask(0x101242, u16OldReg21, 0xFFFF);
    MApi_XC_Write2ByteMask(0x101282, u16OldReg41, 0xFFFF);
    MApi_XC_Write2ByteMask(0x1012A2, u16OldReg51, 0xFFFF);

    MApi_XC_Write2ByteMask(0x101248, u16OldReg24, 0xFFFF);
    MApi_XC_Write2ByteMask(0x101268, u16OldReg34, 0xFFFF);
    MApi_XC_Write2ByteMask(0x101288, u16OldReg44, 0xFFFF);
    MApi_XC_Write2ByteMask(0x1012A8, u16OldReg54, 0xFFFF);

    MApi_XC_Write2ByteMask(0x101240, u16OldReg20, 0xFFFF);
    MApi_XC_Write2ByteMask(0x101260, u16OldReg30, 0xFFFF);
    MApi_XC_Write2ByteMask(0x101280, u16OldReg40, 0xFFFF);
    MApi_XC_Write2ByteMask(0x1012A0, u16OldReg50, 0xFFFF);

    MApi_XC_Write2ByteMask(0x120E0A, u16OldReg05, 0xFFFF);
#endif

    if (bFreeze) {
        MApi_XC_FreezeImg(FALSE, MAIN_WINDOW);
        if (u32SLZCaps == 1) {
            MApi_XC_SetSeamlessZapping(MAIN_WINDOW, TRUE);
        }
    }

#ifndef ENABLE_4K2K_OP_DEFAULT
    if (7 == ursaVsersion && 0 == bPanelDualPort && LINK_HS_LVDS ==  ePanelLinkExtType) {
        MApi_GOP_GWIN_SetGOPDst(gop_and, E_GOP_DST_FRC);
    }
#endif
    return TRUE;
}

static MS_BOOL _StretchBlit(GFX_Block* SrcDwin_info, GFX_Block* DestDwin_info, unsigned long srcPAaddr, unsigned long destPAaddr, GFX_Buffer_Format srcColorFmt, GFX_Buffer_Format destColorFmt, MS_BOOL MirrorH, MS_BOOL MirrorV) {
    GFX_BufferInfo srcbuf_bak, dstbuf_bak;
    GFX_BufferInfo srcbuf, dstbuf;
    GFX_DrawRect bitbltInfo;
    GFX_Point v0, v1;
    MS_U32 src_chunk_line = SrcDwin_info->height/CHUNK_NUM;
    MS_U32 dest_chunk_line = DestDwin_info->height/CHUNK_NUM;
    MS_U32 src_chunk_size = SrcDwin_info->width * _MstarBytesPerPixel(srcColorFmt) * src_chunk_line;
    MS_U32 dest_chunk_size = DestDwin_info->width * _MstarBytesPerPixel(destColorFmt) * dest_chunk_line;

    if (MirrorV) {
        if (src_chunk_size < dest_chunk_size) {
            ALOGE("_StretchBlit not support this case: srcchunk  %u destchunk %u\n", src_chunk_size, dest_chunk_size);
            return FALSE;
        }
    }

    MApi_GFX_BeginDraw();
    MApi_GFX_EnableDFBBlending(FALSE);
    MApi_GFX_EnableAlphaBlending(FALSE);
    MApi_GFX_SetAlphaSrcFrom(ABL_FROM_ASRC);
    MApi_GFX_GetBufferInfo(&srcbuf_bak, &dstbuf_bak);  //get current

    v0.x = 0;
    v0.y = 0;
    v1.x = SrcDwin_info->width;
    v1.y = src_chunk_line * (1+CHUNK_NUM); //9/8: cover buffer size width*height + 1/8* width*height
    MApi_GFX_SetClip(&v0, &v1);

    //V mirror
    if (MirrorV) {
        int i;

        bitbltInfo.srcblk.x = 0;
        bitbltInfo.srcblk.y = 0;
        bitbltInfo.srcblk.width = SrcDwin_info->width;
        bitbltInfo.srcblk.height = src_chunk_line;

        bitbltInfo.dstblk.x = 0;
        bitbltInfo.dstblk.y = 0;
        //stretchblit src to buffer which has been moved from top to bottom
        for (i=0; i<(CHUNK_NUM>>1); ++i) {
            //move first
            srcbuf.u32ColorFmt = srcColorFmt;
            srcbuf.u32Pitch = SrcDwin_info->width * _MstarBytesPerPixel(srcColorFmt);
            srcbuf.u32Addr = srcPAaddr + srcbuf.u32Pitch * i*src_chunk_line;
            //ALOGD("i %d  srcPAaddr %lx, srcbuf.u32Addr %lx ", i, srcPAaddr, srcbuf.u32Addr);

            if (MApi_GFX_SetSrcBufferInfo(&srcbuf, 0) != GFX_SUCCESS) {
                ALOGE("GFX set SrcBuffer failed\n");
                goto STRETCH_BLIT_ERROR;
            }

            dstbuf.u32ColorFmt = srcColorFmt;
            dstbuf.u32Pitch = SrcDwin_info->width * _MstarBytesPerPixel(srcColorFmt);
            dstbuf.u32Addr = destPAaddr + dstbuf.u32Pitch * (CHUNK_NUM-i)*src_chunk_line;
            //ALOGD("i %d  destPAaddr %lx, dstbuf.u32Addr %lx ", i, destPAaddr, dstbuf.u32Addr);
            if (MApi_GFX_SetDstBufferInfo(&dstbuf, 0) != GFX_SUCCESS) {
                ALOGE("GFX set DetBuffer failed\n");
                goto STRETCH_BLIT_ERROR;
            }

            bitbltInfo.srcblk.x = 0;
            bitbltInfo.srcblk.y = 0;
            bitbltInfo.dstblk.width = SrcDwin_info->width;
            bitbltInfo.dstblk.height = src_chunk_line;
            MApi_GFX_SetMirror(false, false);
            if (MApi_GFX_BitBlt(&bitbltInfo, GFXDRAW_FLAG_DEFAULT) != GFX_SUCCESS) { //move data
                ALOGE("copy GFX BitBlt failed i %d\n", i);
                goto STRETCH_BLIT_ERROR;
            }

            //stretchblit
            srcbuf.u32ColorFmt = srcColorFmt;
            srcbuf.u32Pitch = SrcDwin_info->width * _MstarBytesPerPixel(srcColorFmt);
            srcbuf.u32Addr = srcPAaddr + srcbuf.u32Pitch * (CHUNK_NUM-1-i)*src_chunk_line;
            //ALOGD("stretch i %d  srcPAaddr %lx, srcbuf.u32Addr %lx ", i, srcPAaddr, srcbuf.u32Addr);
            if (MApi_GFX_SetSrcBufferInfo(&srcbuf, 0) != GFX_SUCCESS) {
                ALOGE("GFX set SrcBuffer failed\n");
                goto STRETCH_BLIT_ERROR;
            }

            dstbuf.u32ColorFmt = destColorFmt;
            dstbuf.u32Pitch = DestDwin_info->width * _MstarBytesPerPixel(destColorFmt);
            dstbuf.u32Addr = destPAaddr + dstbuf.u32Pitch * i*dest_chunk_line;
            //ALOGD("stretch i %d  destPAaddr %lx, dstbuf.u32Addr %lx ", i, destPAaddr, dstbuf.u32Addr);
            if (MApi_GFX_SetDstBufferInfo(&dstbuf, 0) != GFX_SUCCESS) {
                ALOGE("GFX set DetBuffer failed\n");
                goto STRETCH_BLIT_ERROR;
            }

            if (MirrorH) {
                bitbltInfo.srcblk.x += bitbltInfo.srcblk.width - 1;
            }

            if (MirrorV) {
                bitbltInfo.srcblk.y += bitbltInfo.srcblk.height - 1;
            }

            MApi_GFX_SetMirror(MirrorH, MirrorV);
            bitbltInfo.dstblk.width = DestDwin_info->width;
            bitbltInfo.dstblk.height = dest_chunk_line;
            if (MApi_GFX_BitBlt(&bitbltInfo, GFXDRAW_FLAG_SCALE) != GFX_SUCCESS) { //move data
                ALOGE("blit GFX BitBlt failed i %d\n", i);
                goto STRETCH_BLIT_ERROR;
            }
        }

        //for last half data stretchblit (no need data move)
        bitbltInfo.srcblk.x = 0;
        bitbltInfo.srcblk.y = 0;
        if (MirrorH) {
            bitbltInfo.srcblk.x += bitbltInfo.srcblk.width - 1;
        }

        if (MirrorV) {
            bitbltInfo.srcblk.y += bitbltInfo.srcblk.height - 1;
        }

        srcbuf.u32ColorFmt = srcColorFmt;
        srcbuf.u32Pitch = SrcDwin_info->width * _MstarBytesPerPixel(srcColorFmt);
        dstbuf.u32ColorFmt = destColorFmt;
        dstbuf.u32Pitch = DestDwin_info->width * _MstarBytesPerPixel(destColorFmt);
        MApi_GFX_SetMirror(MirrorH, MirrorV);

        for (i=0; i<(CHUNK_NUM>>1); ++i) {
            srcbuf.u32Addr = srcPAaddr + srcbuf.u32Pitch * (1+(CHUNK_NUM>>1)+i)*src_chunk_line;
            if (MApi_GFX_SetSrcBufferInfo(&srcbuf, 0) != GFX_SUCCESS) {
                ALOGE("GFX set SrcBuffer failed\n");
                goto STRETCH_BLIT_ERROR;
            }

            dstbuf.u32Addr = destPAaddr + dstbuf.u32Pitch * ((CHUNK_NUM>>1) + i)*dest_chunk_line;
            if (MApi_GFX_SetDstBufferInfo(&dstbuf, 0) != GFX_SUCCESS) {
                ALOGE("GFX set DetBuffer failed\n");
                goto STRETCH_BLIT_ERROR;
            }

            bitbltInfo.dstblk.width = DestDwin_info->width;
            bitbltInfo.dstblk.height = dest_chunk_line;
            if (MApi_GFX_BitBlt(&bitbltInfo, GFXDRAW_FLAG_SCALE) != GFX_SUCCESS) { //move data
                ALOGE("GFX BitBlt failed\n");
                goto STRETCH_BLIT_ERROR;
            }
        }
    } else {  //not v mirror
        srcbuf.u32ColorFmt = srcColorFmt;
        srcbuf.u32Addr = srcPAaddr;
        srcbuf.u32Pitch = SrcDwin_info->width * _MstarBytesPerPixel(srcColorFmt);
        if (srcColorFmt == GFX_FMT_YUV422) {
            MApi_GFX_SetDC_CSC_FMT(GFX_YUV_RGB2YUV_PC, GFX_YUV_OUT_255, GFX_YUV_IN_255, GFX_YUV_YVYU,  GFX_YUV_YVYU);
        }

        if (MApi_GFX_SetSrcBufferInfo(&srcbuf, 0) != GFX_SUCCESS) {
            ALOGE("GFX set SrcBuffer failed\n");
            goto STRETCH_BLIT_ERROR;
        }

        dstbuf.u32ColorFmt = destColorFmt;
        dstbuf.u32Addr = destPAaddr;
        dstbuf.u32Pitch = DestDwin_info->width * _MstarBytesPerPixel(destColorFmt);
        if (MApi_GFX_SetDstBufferInfo(&dstbuf, 0) != GFX_SUCCESS) {
            ALOGE("GFX set DetBuffer failed\n");
            goto STRETCH_BLIT_ERROR;
        }

        bitbltInfo.srcblk.x = 0;
        bitbltInfo.srcblk.y = 0;
        bitbltInfo.srcblk.width = SrcDwin_info->width;
        bitbltInfo.srcblk.height = SrcDwin_info->height;

        bitbltInfo.dstblk.x = 0;
        bitbltInfo.dstblk.y = 0;
        bitbltInfo.dstblk.width = DestDwin_info->width;
        bitbltInfo.dstblk.height = DestDwin_info->height;

        if (MirrorH) {
            bitbltInfo.srcblk.x += bitbltInfo.srcblk.width - 1;
        }
        MApi_GFX_SetMirror(MirrorH, MirrorV);
        if (MApi_GFX_BitBlt(&bitbltInfo, GFXDRAW_FLAG_SCALE) != GFX_SUCCESS) {
            ALOGE("GFX BitBlt failed\n");
            goto STRETCH_BLIT_ERROR;
        }
    }

    if (MApi_GFX_FlushQueue() != GFX_SUCCESS) {
        ALOGE("GFX FlushQueue failed\n");
        goto STRETCH_BLIT_ERROR;
    }
    //restore to original
    if (MApi_GFX_SetSrcBufferInfo(&srcbuf_bak, 0) != GFX_SUCCESS) {
        ALOGE("GFX set SrcBuffer failed\n");
    }

    if (MApi_GFX_SetDstBufferInfo(&dstbuf_bak, 0) != GFX_SUCCESS) {
        ALOGE("GFX set DetBuffer failed\n");
    }
    MApi_GFX_EndDraw();

    return TRUE;
    STRETCH_BLIT_ERROR:
    MApi_GFX_EndDraw();
    return FALSE;
}

/// enter crop UI mode
static MS_BOOL _enterCropUiMode(MS_U32 u32BaseAddr) {
    MS_BOOL bResult = TRUE;
    MS_U32 u32WordUnit = 0;
    char property[PROPERTY_VALUE_MAX];
    GOP_GwinInfo GwinInfo;
    memset(&GwinInfo, 0, sizeof(GOP_GwinInfo));
    memset(property, 0, sizeof(char)*PROPERTY_VALUE_MAX);

    u32CropUiAddrOffset = 0;

    if ((GOP_API_SUCCESS != MApi_GOP_GetChipCaps(E_GOP_CAP_WORD_UNIT, &u32WordUnit, sizeof(MS_U32)))
        || (0 == u32WordUnit)) {
        bResult = FALSE;
        ALOGE("[%s %d]Get WORD UNIT %lu %u fail for crop ui!!!",__FUNCTION__,__LINE__, u32WordUnit, MApi_GOP_GetChipCaps(E_GOP_CAP_WORD_UNIT, &u32WordUnit, sizeof(MS_U32)));
    } else {
        MS_U32 u32UserCropUiHStart = 0;
        MS_U32 u32UserCropUiVStart = 0;
        MS_U32 u32UserCropUiWidth = 0;
        MS_U32 u32UserCropUiHeight = 0;
        // get user UI crop window h start
        if (property_get("mstar.ui-x", property, NULL) > 0) {
            u32UserCropUiHStart = atoi(property);
            u32UserCropUiHStart = ALIGN(u32UserCropUiHStart, u32WordUnit);
            if (u32UserCropUiHStart >= fb_vinfo.xres) {
                bResult = FALSE;
                ALOGE("[%s %d]UserCropUiHStart %lu error! fb_vinfo.xres = %d\n",__FUNCTION__,__LINE__, u32UserCropUiHStart, fb_vinfo.xres);
            }
        } else {
            bResult = FALSE;
            ALOGE("[%s %d]can't get UserCropUiHStart\n",__FUNCTION__,__LINE__);
        }

        // get user UI crop window v start
        if (property_get("mstar.ui-y", property, NULL) > 0) {
            u32UserCropUiVStart = atoi(property);
            u32UserCropUiVStart = ALIGN(u32UserCropUiVStart, u32WordUnit);
            if (u32UserCropUiVStart >= fb_vinfo.yres) {
                bResult = FALSE;
                ALOGE("[%s %d]UserCropUiVStart %lu error! fb_vinfo.yres = %d\n",__FUNCTION__,__LINE__, u32UserCropUiVStart, fb_vinfo.yres);
            }
        } else {
            bResult = FALSE;
            ALOGE("[%s %d]can't get UserCropUiVStart\n",__FUNCTION__,__LINE__);
        }

        // get user UI crop window width
        if (property_get("mstar.ui-w", property, NULL) > 0) {
            u32UserCropUiWidth = atoi(property);
            u32UserCropUiWidth = ALIGN(u32UserCropUiWidth - u32WordUnit, u32WordUnit);
            if (u32UserCropUiWidth + u32UserCropUiHStart >= fb_vinfo.xres) {
                bResult = FALSE;
                ALOGE("[%s %d]UserCropUiWidth %lu error! UserCropUiHStart = %lu, fb_vinfo.xres = %d\n",__FUNCTION__,__LINE__, u32UserCropUiWidth, u32UserCropUiHStart, fb_vinfo.xres);
            }
        } else {
            bResult = FALSE;
            ALOGE("[%s %d]can't get UserCropUiWidth\n",__FUNCTION__,__LINE__);
        }

        // get user UI crop window height
        if (property_get("mstar.ui-h", property, NULL) > 0) {
            u32UserCropUiHeight = atoi(property);
            if (u32UserCropUiHeight + u32UserCropUiVStart >= fb_vinfo.yres) {
                bResult = FALSE;
                ALOGE("[%s %d]UserCropUiHeight %lu error! UserCropUiVStart = %lu, fb_vinfo.yres = %d\n",__FUNCTION__,__LINE__, u32UserCropUiHeight, u32UserCropUiVStart, fb_vinfo.yres);
            }
        } else {
            bResult = FALSE;
            ALOGE("[%s %d]can't get UserCropUiHeight\n",__FUNCTION__,__LINE__);
        }

        if (TRUE == bResult) {
            ALOGI("[%s %d][UI crop window] Aligned HStart=%lu VStart=%lu Width=%lu Height=%lu\n",__FUNCTION__,__LINE__,u32UserCropUiHStart,u32UserCropUiVStart,u32UserCropUiWidth,u32UserCropUiHeight);

            if (GOP_API_SUCCESS != MApi_GOP_GWIN_GetWinInfo(gwin, &GwinInfo)) {
                bResult = FALSE;
                ALOGE("[%s %d]Get gwin info fail for UI crop window!!!\n",__FUNCTION__,__LINE__);
            } else {
                MS_U32 u32FbId = MApi_GOP_GWIN_Get32FBfromGWIN(gwin);
                GOP_ApiStatus stGopApiStatus;
                GOP_StretchInfo StretchInfo;
                EN_GOP_STRETCH_DIRECTION enDirection = E_GOP_NO_STRETCH;
                memset(&stGopApiStatus, 0, sizeof(GOP_ApiStatus));
                memset(&StretchInfo, 0, sizeof(GOP_StretchInfo));

                GwinInfo.u16DispHPixelEnd = u32UserCropUiWidth;
                GwinInfo.u16DispHPixelStart = 0;
                GwinInfo.u16DispVPixelEnd = u32UserCropUiHeight;
                GwinInfo.u16DispVPixelStart = 0;
                u32CropUiAddrOffset = GwinInfo.u16RBlkHRblkSize * u32UserCropUiVStart + u32UserCropUiHStart*(fb_vinfo.bits_per_pixel/8);
                GwinInfo.u32DRAMRBlkStart = u32BaseAddr + u32CropUiAddrOffset;

                MApi_GOP_GetStatus(&stGopApiStatus);
                StretchInfo.eDstType = stGopApiStatus.eGOPNumDstType[gwin];
                StretchInfo.x = 0;
                StretchInfo.y = 0;
                StretchInfo.width = u32UserCropUiWidth;
                StretchInfo.height = u32UserCropUiHeight;

                enDirection = E_GOP_HV_STRETCH;
                if (GOP_API_SUCCESS != MApi_GOP_GWIN_SetResolution_32FB(gwin, u32FbId, &GwinInfo, &StretchInfo, enDirection, panelWidth, panelHeight)) {
                    bResult = FALSE;
                    u32CropUiAddrOffset = 0;
                    ALOGE("[%s %d][UI crop window] GOP set resolution fail!!\n",__FUNCTION__,__LINE__);
                } else {
                    bResult = TRUE;
                }
            }
        }
    }
    return bResult;
}

/// exit crop UI mode
static MS_BOOL _exitCropUiMode(MS_U32 u32BaseAddr) {
    MS_BOOL bResult = TRUE;
    GOP_GwinInfo GwinInfo;
    memset(&GwinInfo, 0, sizeof(GOP_GwinInfo));
    u32CropUiAddrOffset = 0;
    if (GOP_API_SUCCESS != MApi_GOP_GWIN_GetWinInfo(gwin, &GwinInfo)) {
        bResult = FALSE;
        ALOGE("[%s %d]MApi_GOP_GetChipCaps fail for full screen ui!!!\n",__FUNCTION__,__LINE__);
    } else {
        MS_U32 u32FbId = MApi_GOP_GWIN_Get32FBfromGWIN(gwin);
        GOP_ApiStatus stGopApiStatus;
        GOP_StretchInfo StretchInfo;
        EN_GOP_STRETCH_DIRECTION enDirection = E_GOP_NO_STRETCH;
        memset(&stGopApiStatus, 0, sizeof(GOP_ApiStatus));
        memset(&StretchInfo, 0, sizeof(GOP_StretchInfo));

        GwinInfo.u16DispHPixelEnd = fb_vinfo.xres;
        GwinInfo.u16DispHPixelStart = 0;
        GwinInfo.u16DispVPixelEnd = fb_vinfo.yres;
        GwinInfo.u16DispVPixelStart = 0;
        GwinInfo.u32DRAMRBlkStart = u32BaseAddr;

        MApi_GOP_GetStatus(&stGopApiStatus);
        StretchInfo.eDstType = stGopApiStatus.eGOPNumDstType[gwin];
        StretchInfo.x = 0;
        StretchInfo.y = 0;
        StretchInfo.width = fb_vinfo.xres;
        StretchInfo.height = fb_vinfo.yres;

        enDirection = E_GOP_HV_STRETCH;
        if (GOP_API_SUCCESS != MApi_GOP_GWIN_SetResolution_32FB(gwin, u32FbId, &GwinInfo, &StretchInfo, enDirection, panelWidth, panelHeight)) {
            bResult = FALSE;
            ALOGE("[%s %d][UI crop window] GOP set resolution fail!!\n",__FUNCTION__,__LINE__);
        }
    }
    return bResult;
}

int fbdev_init(void) {
    char* modelName = NULL, *panelName = NULL, *panelName4k2k = NULL;
    char property[PROPERTY_VALUE_MAX];
    dictionary dictInit;
    dictionary *sysINI, *modelINI, *panelINI, *panelINI4k2k;
    memset(&dictInit, 0x00, sizeof(dictInit));
    sysINI = modelINI = panelINI = panelINI4k2k = &dictInit;

    if (mmap_init() != 0) {
        return -1;
    }

    MDrv_SYS_GlobalInit();
    mdrv_gpio_init();
    MApi_PNL_IOMapBaseInit();
    u8MainFbId = MAX_GWIN_FB_SUPPORT;
    u8SubFbId =  MAX_GWIN_FB_SUPPORT;
    Androidfbid = MAX_GWIN_FB_SUPPORT;

    int ret = 0;
    ret = tvini_init(&sysINI, &modelINI, &panelINI);
    if (ret == -1) {
        ALOGE("tvini_init: get ini fail!!!\n");
        return -1;
    }
    panelName4k2k = iniparser_getstr(modelINI, "panel:m_p4K2KPanelName");
    panelName = iniparser_getstr(modelINI, "panel:m_pPanelName");
    ursaVsersion = iniparser_getint(modelINI, "Ursa:UrsaSelect", 0);
    ALOGD("panel:m_pPanelName = %s  panel:m_p4K2KPanelName = %s the ursaVersion is %d\n", panelName,panelName4k2k,ursaVsersion);

    if (panelName4k2k != NULL) {
        panelINI4k2k = iniparser_load(panelName4k2k);
        if (panelINI4k2k == &dictInit) {
            tvini_free(sysINI, modelINI, panelINI);
            ALOGE("4k2k Load %s failed!", panelName4k2k);
            return -1;
        }

        panelWidth4k2k = iniparser_getint(panelINI4k2k, "panel:m_wPanelWidth", 3840);
        panelHeight4k2k = iniparser_getint(panelINI4k2k, "panel:m_wPanelHeight", 2160);
        panelHStart4k2k = iniparser_getint(panelINI4k2k, "panel:m_wPanelHStart", 112);
        ALOGD("4k2k panelWidth4k2k=%d, panelHeight4k2k=%d, panelHStart4k2k=%d", panelWidth4k2k, panelHeight4k2k, panelHStart4k2k);
    }

    if (!(property_get("mstar.recoverymode", property, NULL) > 0)) {
        if (ursaVsersion) {
            if (property_get("mstar.4k2k.enable", property, NULL) > 0) {
                if (!strcmp(property,"true")) {
                    bURSA_4k2kPanel = true;
                }
            }
            ALOGD("bURSA_4k2kPanel is %d ",bURSA_4k2kPanel);
        }
    }

    if ((panelName4k2k != NULL) && bURSA_4k2kPanel) {
        gfx_Init(panelINI4k2k,modelINI);
    } else {
        gfx_Init(panelINI,modelINI);
    }
    gfx_4k2kphoto_mem_init();

    tvini_free(sysINI, modelINI, panelINI);
    if (panelName4k2k) {
        iniparser_freedict(panelINI4k2k);
    }
    return 0;
}

int fbdev_open(const char* pathname, int flags) {
    if (fake_fd < 0) {
        fake_fd = open("/dev/null", O_RDWR);
    }
    return fake_fd;
}

int fbdev_close(int fd) {
    fake_fd = -100;
    if (qpAndroidFRCPropInfo != NULL) {
        free(qpAndroidFRCPropInfo);
    }
    return 0;
}

//only used to test FPS
#if 0
static int fps = 0;
time_t last;
#endif

int fbdev_ioctl(int fd, int request, void* data) {
    struct fb_var_screeninfo* vinfo;
    struct fb_fix_screeninfo* finfo;
    static int gPrevOSDmode = 0;
    char property[PROPERTY_VALUE_MAX] = {0};
    MS_U16 u16QueueCnt;
    MS_U16 tagID;
    MS_U32 physicalAddr;
    static MS_U32 prePhysicalAddr = 0;  //for Frame by Frame osd output
    int lvdsOff = 0;
    GOP_GwinFBAttr fbAttr, Main_fbAttr;
    MS_U32 u32FlipAddr_A, u32FlipAddr_B=0;
    MS_U16 u16QueueCnt1 = 1;
    GOP_MultiFlipInfo stMultiFlipInfo;

    switch (request) {
        case FBIOGET_VSCREENINFO:
            vinfo = (struct fb_var_screeninfo*)data;
            if (vinfo) {
                memcpy(vinfo, &fb_vinfo, sizeof(struct fb_var_screeninfo));
                return 0;
            }
            break;

        case FBIOPUT_VSCREENINFO:
            vinfo = (struct fb_var_screeninfo*)data;
            if (vinfo) {
                static int bPreUserCropUiEnable = 0;  // previous value
                int retry_times;

                if (vinfo->xres_virtual > fb_vinfo.xres_virtual) {
                    vinfo->xres_virtual = fb_vinfo.xres_virtual;
                }
                if (vinfo->yres_virtual > fb_vinfo.yres_virtual) {
                    vinfo->yres_virtual = fb_vinfo.yres_virtual;
                }

                vinfo->transp = fb_vinfo.transp;
                vinfo->red    = fb_vinfo.red;
                vinfo->green  = fb_vinfo.green;
                vinfo->blue   = fb_vinfo.blue;

                fb_vinfo.yoffset = vinfo->yoffset;
#if 0
                fps++;
                if (time(NULL)!=last) {
                    last = time(NULL);
                    ALOGE("flip first fb fbdev fps=%d\n", fps);
                    fps = 0;
                }
#endif

#if 0  //wenjing - ge VQ
                MApi_GFX_SetTAGID(0x77);
                MApi_GFX_WaitForTAGID(0x77);
                MApi_GFX_FlushQueue();
#endif

                if (andGOPdest==E_GOP_DST_FRC) {
                    getCurOCTiming(&(cur_OCTiming.width),&(cur_OCTiming.height));
                    if ((cur_OCTiming.width >1000 && cur_OCTiming.height>1000)||/*Cause of MApi_XC_OSDC_InitSetting is later*/
                        (cur_OCTiming.width!=pre_OCTimingInfo.width)||
                        (cur_OCTiming.height!=pre_OCTimingInfo.height)) {
                        pre_OCTimingInfo = cur_OCTiming;
                        gfx_GOPStretch(gop_and, req_OSDRegion.width,  req_OSDRegion.height, cur_OCTiming.width, cur_OCTiming.height);
                    }
                }

                // MApi_GOP_GWIN_GetWinInfo(gwin, &gwinInfo);
                // gwinInfo.u32DRAMRBlkStart = fb_finfo.smem_start + fb_finfo.line_length * fb_vinfo.yoffset;
                // MApi_GOP_GWIN_SetWinInfo(gwin, &gwinInfo)

                //Update FBID addr from queue;
                physicalAddr  = fb_finfo.smem_start + fb_finfo.line_length * fb_vinfo.yoffset;
                setCurrentFBPhys(physicalAddr);
                pthread_mutex_lock(&gFrcDest_mutex);
                if ((req_OSDRegion.width!= cur_OSDRegion.width)||
                    (req_OSDRegion.height!= cur_OSDRegion.height)) {
                    bOSDRegionChanged = true;
                    cur_OSDRegion = req_OSDRegion;

                    SetAndroidGOPDest();

                    ALOGD("the req OSD Region width is %d height is %d\n",req_OSDRegion.width,req_OSDRegion.height);
                    bOSDRegionChanged = false;
                }

                int osdMode = 0;
                MS_BOOL bUserCropUiEnable = 0;
#if 0
                if (property_get("mstar.desk-display-mode", property, NULL) > 0) {
                    osdMode = atoi(property);
                }
#endif

                osdMode = g_fbdev_displaymode;
                if (property_get("mstar.lvds-off", property, NULL) > 0) {
                    lvdsOff = atoi(property);
                }
                // Line alternative for top-bottom mode
                if ((osdMode != gPrevOSDmode)) {
                    MApi_GOP_GWIN_SetForceWrite(FALSE);
                    MApi_GOP_GWIN_UpdateRegOnce(TRUE);
                    ConfigureGopGwin(osdMode,physicalAddr,cur_OSDRegion);
                    MApi_GOP_GWIN_UpdateRegOnce(FALSE);
                    gPrevOSDmode = osdMode;
                }

                // use crop UI mode only in normal case
                if ((DISPLAYMODE_NORMAL == osdMode) && (0 == lvdsOff)) {
                    if (property_get("mstar.ui-user-screen", property, NULL) > 0) {
                        bUserCropUiEnable = (0 != atoi(property));
                    }
                } else if (ENABLE == bPreUserCropUiEnable) {
                    // exit crop UI mode
                    _exitCropUiMode(physicalAddr);
                    bPreUserCropUiEnable = DISABLE;
                    bUserCropUiEnable = DISABLE;
                }

                tagID = MApi_GFX_GetNextTAGID(TRUE);
                MApi_GFX_SetTAGID(tagID);
                retry_times = 0;

                while (1) {
                    u16QueueCnt = gSafeFbQueueCnt;
                    if (bNeedMonitorTiming) {
                        MS_U32 u32FRCAddrOffset=0;
                        if (osdMode == DISPLAYMODE_LEFTRIGHT_FR
                            ||osdMode == DISPLAYMODE_NORMAL_FR) {
                            u32FRCAddrOffset = fb_vinfo.xres;
                        } else {
                            u32FRCAddrOffset = fb_vinfo.xres * 2;
                        }
                        if (bEnableFRC && (andGOPdest==E_GOP_DST_BYPASS)) {
                            if (mirrorModeH && mirrorModeV) {
                                u32FlipAddr_A = physicalAddr +u32CropUiAddrOffset+ u32FRCAddrOffset;
                                u32FlipAddr_B = physicalAddr +u32CropUiAddrOffset;
                            } else {
                                u32FlipAddr_A = physicalAddr +u32CropUiAddrOffset;
                                u32FlipAddr_B = physicalAddr +u32CropUiAddrOffset+ u32FRCAddrOffset;
                            }
                        }
                    }
                    if (!lvdsOff) {
                        if (gPrevOSDmode != DISPLAYMODE_NORMAL) {
                            //ALOGE("@-Ad=%lx,%lx\n", physicalAddr, physicalAddr+u32SubAddrOffset);
                            MS_U32 u32MAddrOffset=0;
                            MS_U32 u32SAddrOffset=0;
                            if (bLRSwitchFlag) {
                                //3D OSD LR switch
                                u32MAddrOffset= u32SubAddrOffset;
                                u32SAddrOffset = 0;
                            } else {
                                u32MAddrOffset= 0;
                                u32SAddrOffset = u32SubAddrOffset;
                            }

                            if (andGOPdest==E_GOP_DST_BYPASS) {
                                stMultiFlipInfo.u8InfoCnt = 2;
                                stMultiFlipInfo.astGopInfo[0].gWinId = gwin;
                                stMultiFlipInfo.astGopInfo[0].u32FlipAddr = u32FlipAddr_A + u32MAddrOffset;
                                stMultiFlipInfo.astGopInfo[0].u32SubAddr = u32FlipAddr_A +u32SAddrOffset;
                                stMultiFlipInfo.astGopInfo[0].u16WaitTagID = tagID;
                                stMultiFlipInfo.astGopInfo[0].pU16QueueCnt = &u16QueueCnt;
                                stMultiFlipInfo.astGopInfo[1].gWinId = (bEnableFRC==TRUE)?gwin_frc: gwin;
                                stMultiFlipInfo.astGopInfo[1].u32FlipAddr = u32FlipAddr_B +u32MAddrOffset;
                                stMultiFlipInfo.astGopInfo[1].u32SubAddr = u32FlipAddr_B+ u32SAddrOffset;
                                stMultiFlipInfo.astGopInfo[1].u16WaitTagID = tagID;
                                stMultiFlipInfo.astGopInfo[1].pU16QueueCnt = &u16QueueCnt1;
                                if (MApi_GOP_Switch_Multi_GWIN_2_FB_BY_ADDR(stMultiFlipInfo))
                                    break;
                            } else {
                                if (bLRSwitchFlag) {  //3D OSD LR switch
                                    if (gPrevOSDmode != DISPLAYMODE_NORMAL_FR) {
                                        if (MApi_GOP_Switch_3DGWIN_2_FB_BY_ADDR(gwin, physicalAddr+u32SubAddrOffset, physicalAddr, tagID, &u16QueueCnt)) {
                                            break;
                                        }
                                    } else {
                                        if (prePhysicalAddr != 0) {
                                            MApi_GOP_Switch_3DGWIN_2_FB_BY_ADDR(gwin, prePhysicalAddr+u32SubAddrOffset, physicalAddr,tagID, &u16QueueCnt);
                                            u16QueueCnt = gSafeFbQueueCnt;
                                            usleep(1000);
                                            MApi_GOP_Switch_3DGWIN_2_FB_BY_ADDR(gwin, physicalAddr+u32SubAddrOffset, physicalAddr,tagID, &u16QueueCnt);
                                        }
                                        prePhysicalAddr = physicalAddr;
                                        break;
                                    }
                                } else {
                                    if (gPrevOSDmode != DISPLAYMODE_NORMAL_FR) {
                                        if (MApi_GOP_Switch_3DGWIN_2_FB_BY_ADDR(gwin, physicalAddr, physicalAddr+u32SubAddrOffset, tagID, &u16QueueCnt)) {
                                            break;
                                        }
                                    } else {
                                        if (prePhysicalAddr != 0) {
                                            MApi_GOP_Switch_3DGWIN_2_FB_BY_ADDR(gwin, prePhysicalAddr, physicalAddr+u32SubAddrOffset, tagID, &u16QueueCnt);
                                            u16QueueCnt = gSafeFbQueueCnt;
                                            usleep(1000);
                                            MApi_GOP_Switch_3DGWIN_2_FB_BY_ADDR(gwin, physicalAddr, physicalAddr+u32SubAddrOffset, tagID, &u16QueueCnt);
                                        }
                                        prePhysicalAddr = physicalAddr;
                                        break;
                                    }
                                }
                            }
                        } else {
                            if (bNeedMonitorTiming) {
                                if (andGOPdest==E_GOP_DST_BYPASS) {
                                    stMultiFlipInfo.u8InfoCnt = 2;
                                    stMultiFlipInfo.astGopInfo[0].gWinId = gwin;
                                    stMultiFlipInfo.astGopInfo[0].u32FlipAddr = u32FlipAddr_A;
                                    stMultiFlipInfo.astGopInfo[0].u32SubAddr = 0;
                                    stMultiFlipInfo.astGopInfo[0].u16WaitTagID = tagID;
                                    stMultiFlipInfo.astGopInfo[0].pU16QueueCnt = &u16QueueCnt;
                                    stMultiFlipInfo.astGopInfo[1].gWinId = (bEnableFRC==TRUE)?gwin_frc: gwin;
                                    stMultiFlipInfo.astGopInfo[1].u32FlipAddr = u32FlipAddr_B;
                                    stMultiFlipInfo.astGopInfo[1].u32SubAddr = 0;
                                    stMultiFlipInfo.astGopInfo[1].u16WaitTagID = tagID;
                                    stMultiFlipInfo.astGopInfo[1].pU16QueueCnt = &u16QueueCnt1;
                                    if (MApi_GOP_Switch_Multi_GWIN_2_FB_BY_ADDR(stMultiFlipInfo))
                                        break;
                                } else {
                                    if (MApi_GOP_Switch_GWIN_2_FB_BY_ADDR(gwin, physicalAddr + u32CropUiAddrOffset, tagID, &u16QueueCnt))
                                        break;
                                }
                            } else {
                                if (MApi_GOP_Switch_GWIN_2_FB_BY_ADDR(gwin, physicalAddr + u32CropUiAddrOffset, tagID, &u16QueueCnt)) {
                                    break;
                                }
                            }
                        }
                        if (u16QueueCnt <= 0) {
                            ALOGE("Serious warning, unknow error!!\n");
                            break;
                        }
                    }

                    retry_times++;
                    if (retry_times >= 2 || lvdsOff) {
                        GOP_GwinInfo gwinInfo;
                        memset(&gwinInfo, 0, sizeof(GOP_GwinInfo));
                        if (GOP_API_SUCCESS == MApi_GOP_GWIN_GetWinInfo(gwin, &gwinInfo)) {
                            gwinInfo.u32DRAMRBlkStart = physicalAddr;
                            MApi_GOP_GWIN_SetForceWrite(TRUE);
                            MApi_GOP_GWIN_SetWinInfo(gwin, &gwinInfo);
                            MApi_GOP_GWIN_SetForceWrite(FALSE);
                            if (!lvdsOff) {
                                ALOGE("Attention! GFLIP time out, force update gop ring buffer!\n");
                            }
                        } else {
                            ALOGE("Attention! Get Gwin info fail, skip update gop ring buffer!\n");
                        }
                        break;
                    }
                }
                pthread_mutex_unlock(&gFrcDest_mutex);
#if 0
                {
                    static int frames = 0;
                    static struct timeval begin;
                    static struct timeval end;
                    if (frames==0) {
                        gettimeofday(&begin,NULL);
                        end = begin;
                    }
                    frames++;
                    if (frames%100==0) {
                        gettimeofday(&end,NULL);
                        float fps = 100.0*1000/((end.tv_sec-begin.tv_sec)*1000+(end.tv_usec-begin.tv_usec)/1000);
                        begin = end;
                        ALOGI("FBDEV================the fps is %f",fps);
                    }
                }
#endif

                if (bEnableCropUiFeature) {
                    if (bPreUserCropUiEnable != bUserCropUiEnable) {
                        bPreUserCropUiEnable = bUserCropUiEnable;
                        if (ENABLE == bUserCropUiEnable) {
                            if (!_enterCropUiMode(physicalAddr)) {
                                _exitCropUiMode(physicalAddr);
                            }
                        } else {
                            _exitCropUiMode(physicalAddr);
                        }
                    }
                }
                return 0;
            }
            break;

        case FBIOGET_FSCREENINFO:
            finfo = (struct fb_fix_screeninfo*)data;
            if (finfo) {
                memcpy(finfo, &fb_finfo, sizeof(struct fb_fix_screeninfo));
                return 0;
            }
            break;

        case FBIOPUTCMAP:
            break;

        case FBIOGETCMAP:
            break;

        default:
            break;
    }
    errno = EINVAL;
    return -1;
}

void* fbdev_mmap(void* start, size_t length, int prot, int flags, int fd, off_t offset) {
    return fb_mem_mapped;
}

int fbdev_munmap(void* start, size_t length) {
    return 0;
}

int fbdev_getAvailGwinNum_ByGop(int gopNo) {
    if (gopNo < 0) {
        ALOGE("fbdev_getAvailGwinNum_ByGop the given gopNo is %d,check it!",gopNo);
        return -1;
    }
    int availGwinNum = 0;
    int i = 0;
    for (i; i < gopNo; i++) {
        availGwinNum += MApi_GOP_GWIN_GetGwinNum(i);
    }
    return availGwinNum;
}

//gop 2 for 4k2k photo
int gfx_4k2kphoto_mem_init() {
    const mmap_info_t* mmapInfo;
    mmapInfo = mmap_get_info("E_MMAP_ID_GOP_4K2K");
    if (mmapInfo != NULL) {
        fb_4k2kphoto_mem_addr = GE_ADDR_ALIGNMENT(mmapInfo->addr);
        fb_4k2kphoto_mem_len = mmapInfo->size;
    } else {
        ALOGD("gfx_4k2kphoto_mem_init return without E_MMAP_ID_GOP_4K2K");
        return 0;
    }

    MS_U8 miu_sel;
    if (fb_4k2kphoto_mem_addr >= mmap_get_miu1_offset()) {
        miu_sel = 2;
    } else if (fb_4k2kphoto_mem_addr >= mmap_get_miu0_offset()) {
        miu_sel = 1;
    } else {
        miu_sel = 0;
    }
    unsigned int maskPhyAddr =  fbdev_getMaskPhyAdr(miu_sel);
    ALOGI("gfx_4k2kphoto_mem_init was invoked maskPhyAddr=%lx", maskPhyAddr);

    if (fb_4k2kphoto_mem_addr >= mmap_get_miu1_offset()) {
        if (!MsOS_MPool_Mapping(2, (fb_4k2kphoto_mem_addr & maskPhyAddr), ((fb_4k2kphoto_mem_len+4095)&~4095U), 1)) {
            ALOGE("gfx2_mem_init MsOS_MPool_Mapping failed in %s:%d\n",__FUNCTION__,__LINE__);
        }
        fb_4k2kphoto_mem_mapped = (void*)MsOS_PA2KSEG1((fb_4k2kphoto_mem_addr & maskPhyAddr) | mmap_get_miu1_offset());
    } else if (fb_4k2kphoto_mem_addr >= mmap_get_miu0_offset()) {
        if (!MsOS_MPool_Mapping(1, (fb_4k2kphoto_mem_addr & maskPhyAddr), ((fb_4k2kphoto_mem_len+4095)&~4095U), 1)) {
            ALOGE("gfx2_mem_init MsOS_MPool_Mapping failed in %s:%d\n",__FUNCTION__,__LINE__);
        }
        fb_4k2kphoto_mem_mapped = (void*)MsOS_PA2KSEG1((fb_4k2kphoto_mem_addr & maskPhyAddr) | mmap_get_miu0_offset());
    } else {
        if (!MsOS_MPool_Mapping(0, (fb_4k2kphoto_mem_addr & maskPhyAddr), ((fb_4k2kphoto_mem_len+4095)&~4095U), 1)) {
            ALOGE("gfx2_mem_init MsOS_MPool_Mapping failed in %s:%d\n",__FUNCTION__,__LINE__);
        }
        fb_4k2kphoto_mem_mapped = (void*)MsOS_PA2KSEG1(fb_4k2kphoto_mem_addr & maskPhyAddr);
    }
    ALOGD("gfx_4k2kphoto_mem_init fb_4k2kphoto_mem_addr=0x%lx,fb_4k2kphoto_mem_len=0x%lx,fb_4k2kphoto_mem_mapped=%p", fb_4k2kphoto_mem_addr, fb_4k2kphoto_mem_len, fb_4k2kphoto_mem_mapped);
    memset(fb_4k2kphoto_mem_mapped, 0x0, fb_4k2kphoto_mem_len);
    return 0;
}

int gfx_4k2kphoto_init() {
    GOP_InitInfo gopInitInfo;

    gopInitInfo.u16PanelWidth  = panelWidth4k2k;
    gopInitInfo.u16PanelHeight = panelHeight4k2k;
    gopInitInfo.u16PanelHStr   = panelHStart4k2k;

    gopInitInfo.u32GOPRBAdr = GE_ADDR_ALIGNMENT(fb_4k2kphoto_mem_addr);
    gopInitInfo.u32GOPRBLen = fb_4k2kphoto_mem_len;

    gopInitInfo.bEnableVsyncIntFlip = TRUE;

    gop_4k2kphoto = MApi_GOP_GWIN_GetMaxGOPNum() - 2;

    if (gop_4k2kphoto < 0) {
        ALOGE("The GOP Num less than 2 can not show 4k2kphoto!!!!");
        return -1;
    }
    gMiu4k2kSel =  MApi_GOP_GetMIUSel(gop_4k2kphoto);

    gwin_4k2kphoto = fbdev_getAvailGwinNum_ByGop(gop_4k2kphoto);
    ALOGD("gfx_4k2kphoto_init gop_4k2kphoto=%d,gwin_4k2kphoto=%d",gop_4k2kphoto,gwin_4k2kphoto);
    MApi_GOP_InitByGOP(&gopInitInfo, gop_4k2kphoto);

    if (fb_4k2kphoto_mem_addr >= mmap_get_miu1_offset()) {
        ALOGD("gfx_4k2kphoto_init fb_4k2kphoto_mem_addr in MIU2 fb_4k2kphoto_mem_addr=0x%lx", fb_4k2kphoto_mem_addr);
#ifdef ENABLE_CMD_QUEUE
        MApi_GOP_MIUSel(gop_4k2kphoto,E_GOP_SEL_MIU2);
#else
        ALOGE("gop_4k2kphoto select miu2 failed in %s:%d\n",__FUNCTION__,__LINE__);
#endif
    } else if (fb_4k2kphoto_mem_addr >= mmap_get_miu0_offset()) {
        ALOGD("gfx_4k2kphoto_init fb_4k2kphoto_mem_addr in MIU1 fb_4k2kphoto_mem_addr=0x%lx", fb_4k2kphoto_mem_addr);
        MApi_GOP_MIUSel(gop_4k2kphoto,E_GOP_SEL_MIU1);
    } else {
        ALOGD("gfx_4k2kphoto_init fb_4k2kphoto_mem_addr in MIU0 fb_4k2kphoto_mem_addr=0x%lx", fb_4k2kphoto_mem_addr);
        MApi_GOP_MIUSel(gop_4k2kphoto,E_GOP_SEL_MIU0);
    }

    // Register Call Back Function for GOP use
    MApi_GOP_RegisterFBFmtCB(_SetFBFmt);
    MApi_GOP_RegisterXCIsInterlaceCB(_XC_IsInterlace);
    MApi_GOP_RegisterXCGetCapHStartCB(_XC_GetCapwinHStr);
    MApi_GOP_RegisterXCReduceBWForOSDCB(_SetPQReduceBWForOSD);

    MApi_GOP_GWIN_SetGOPDst(gop_4k2kphoto, E_GOP_DST_OP0);
    MApi_GOP_GWIN_EnableTransClr(GOPTRANSCLR_FMT0, FALSE);

    gfx_GOPStretch(gop_4k2kphoto, panelWidth4k2k, panelHeight4k2k, panelWidth4k2k, panelHeight4k2k);

    MApi_GOP_GWIN_SwitchGOP(gop_4k2kphoto);

    gFbId4k2k = MApi_GOP_GWIN_GetFreeFBID();
    //MApi_GOP_GWIN_CreateFBbyStaticAddr(fbid, 0, 0, panelWidth, panelHeight, gfmt, fb2_mem_addr);
    MApi_GOP_GWIN_Create32FBFrom3rdSurf(panelWidth4k2k, panelHeight4k2k, gfmt, fb_4k2kphoto_mem_addr, panelWidth4k2k*4, &gFbId4k2k);
    MApi_GOP_GWIN_MapFB2Win(gFbId4k2k, gwin_4k2kphoto);

    MApi_GOP_GWIN_SetBlending(gwin_4k2kphoto, TRUE, 0x3F);
    MApi_GOP_GWIN_SetGWinShared(gwin_4k2kphoto, TRUE);
    MApi_GOP_GWIN_Enable(gwin_4k2kphoto, TRUE);

    return 0;
}

void* fbdev_4k2kphoto_mmap(unsigned long *hwaddr) {
    *hwaddr = fb_4k2kphoto_mem_addr;
    return fb_4k2kphoto_mem_mapped;
}

int fbdev_postSecFb(int secFbId) {
    MS_U16 u16QueueCnt;
    int lvdsOff = 0;
    int retry_times = 0;
    MS_U16 tagID;
    char property[PROPERTY_VALUE_MAX] = {0};
    tagID = MApi_GFX_GetNextTAGID(TRUE);
    MApi_GFX_SetTAGID(tagID);
    if (property_get("mstar.lvds-off", property, NULL) > 0) {
        lvdsOff = atoi(property);
    }

#if 0
    GOP_GwinInfo gwinInfo;
    memset(&gwinInfo, 0, sizeof(GOP_GwinInfo));
    if (GOP_API_SUCCESS == MApi_GOP_GWIN_GetWinInfo(gwin_4k2kphoto, &gwinInfo)) {
        gwinInfo.u32DRAMRBlkStart = fb_4k2kphoto_mem_addr + secFbId * 3840 * 2160 * 4;
        MApi_GOP_GWIN_SetForceWrite(TRUE);
        MApi_GOP_GWIN_SetWinInfo(gwin_4k2kphoto, &gwinInfo);
        MApi_GOP_GWIN_SetForceWrite(FALSE);
    } else {
        ALOGE("fbdev_postSecFb Attention! Get Gwin info fail, skip update gop ring buffer!\n");
    }
#else
    while (1) {
        if (gPanelMode != PANELMODE_4KX2K_4KX2K) {
            ALOGE("fbdev_postSecFb gPanelMode is not PANELMODE_4KX2K_4KX2K");
            break;
        }
        u16QueueCnt = gSafeFbQueueCnt;
        if (!lvdsOff) {
            if (MApi_GOP_Switch_GWIN_2_FB_BY_ADDR(gwin_4k2kphoto, fb_4k2kphoto_mem_addr + secFbId * 3840 * 2160 * 4, tagID, &u16QueueCnt)) {
                break;
            }
            if (u16QueueCnt <= 0) {
                ALOGE("fbdev_postSecFb Serious warning, unknow error!!\n");
                break;
            }
        }

        retry_times++;
        if (retry_times >= 2 || lvdsOff) {
            GOP_GwinInfo gwinInfo;
            memset(&gwinInfo, 0, sizeof(GOP_GwinInfo));
            if (GOP_API_SUCCESS == MApi_GOP_GWIN_GetWinInfo(gwin_4k2kphoto, &gwinInfo)) {
                gwinInfo.u32DRAMRBlkStart = fb_4k2kphoto_mem_addr + secFbId * 3840 * 2160 * 4;
                MApi_GOP_GWIN_SetForceWrite(TRUE);
                MApi_GOP_GWIN_SetWinInfo(gwin_4k2kphoto, &gwinInfo);
                MApi_GOP_GWIN_SetForceWrite(FALSE);
                if (!lvdsOff) {
                    ALOGE("fbdev_postSecFb Attention! GFLIP time out, force update gop ring buffer!\n");
                }
            } else {
                ALOGE("fbdev_postSecFb Attention! Get Gwin info fail, skip update gop ring buffer!\n");
            }
            break;
        }
    }
#endif

    return 0;
}

int fbdev_set_panel_mode(int panelMode) {
    ALOGD("fbdev_set_panel_mode panelMode=%d", panelMode);
    MS_U8 u8OrgFbId=MAX_GWIN_FB_SUPPORT;

    if (panelMode == PANELMODE_4KX2K_2KX1K) {
        if (gPanelMode == PANELMODE_4KX2K_4KX2K) {
            //MApi_GOP_GWIN_DeleteWin(gwin_4k2kphoto);
            MApi_GOP_GWIN_DestroyWin(gwin_4k2kphoto);
            if (gFbId4k2k != -1) {
                MApi_GOP_GWIN_DeleteFB(gFbId4k2k);
                gFbId4k2k = -1;
            }
            MApi_GOP_MIUSel(gop_4k2kphoto,gMiu4k2kSel);
        }
        gPanelMode = PANELMODE_4KX2K_2KX1K;
    } else if (panelMode == PANELMODE_4KX2K_4KX1K) {
        if (gPanelMode == PANELMODE_4KX2K_4KX2K) {
            //MApi_GOP_GWIN_DeleteWin(gwin_4k2kphoto);
            MApi_GOP_GWIN_DestroyWin(gwin_4k2kphoto);
            if (gFbId4k2k != -1) {
                MApi_GOP_GWIN_DeleteFB(gFbId4k2k);
                gFbId4k2k = -1;
            }
            MApi_GOP_MIUSel(gop_4k2kphoto,gMiu4k2kSel);
        }
        gPanelMode = PANELMODE_4KX2K_4KX1K;
    } else if (panelMode == PANELMODE_4KX2K_4KX2K) {
        gfx_4k2kphoto_init();
        gPanelMode = PANELMODE_4KX2K_4KX2K;
    } else {
        ALOGD("fbdev_set_panel_mode error panelMode=%d", panelMode);
    }

    return 0;

}

int fbdev_get_panel_mode() {
    return gPanelMode;
}

int fbdev_read(int left, int top, int* width, int* height, void* data, int mode) {
    static unsigned long paStart = 0;
    static unsigned long paEnd = 0;
    static int dwinCaptureMutexId = -1;

    unsigned int u32DwinBufSize, u32DestBufSize, gap = 1024*1024;
    unsigned long srcPAaddr = 0, destPAaddr = 0;
    MS_BOOL bMirrorH = FALSE, bMirrorV = FALSE;
    int m_panelWidth = 0;
    int m_panelHeight = 0;
    EN_GOP_DWIN_DATA_FMT stDWINFmt = DWIN_DATA_FMT_ARGB8888;
    GFX_Buffer_Format stGFXFormat = GFX_FMT_ARGB8888;
    PNL_ApiExtStatus stPnlExtStatus;
    GFX_Block mSrcWindow_info, mDestWindow_info;
    MS_U32 u32PNL_Width = 0, u32PNL_Height = 0;

    if ((paStart==0) && (paEnd==0)) {
        if (!_Dwin_GetBuffer(&paStart, &paEnd, &dwinCaptureMutexId)) {
            ALOGE("[fbdev_read]_Dwin_GetBuffer failed");
            return -1;
        }
    }

    memset(&stPnlExtStatus, 0, sizeof(PNL_ApiExtStatus));
    stPnlExtStatus.u16ApiStatusEX_Length = sizeof(PNL_ApiExtStatus);
    stPnlExtStatus.u32ApiStatusEx_Version = API_PNLSTATUS_EX_VERSION;
    if (!MApi_PNL_GetStatusEx(&stPnlExtStatus)) {
        ALOGE("[fbdev_read] MApi_PNL_GetStatusEx error\n");
        return -1;
    }

    u32PNL_Width = stPnlExtStatus.u16DEHEnd-stPnlExtStatus.u16DEHStart+1;
    u32PNL_Height = stPnlExtStatus.u16DEVEnd-stPnlExtStatus.u16DEVStart+1;
    m_panelWidth = u32PNL_Width & (~GOP_PIXEL_ALIGNMENT_FACTOR);
    m_panelHeight = u32PNL_Height;

    if (data == 0) {
        if ((m_panelWidth > panelWidth) || (m_panelHeight > panelHeight)) {
            *width = panelWidth;
            *height = panelHeight;
        } else {
            *width = m_panelWidth;
            *height = m_panelHeight;
        }
        ALOGD("[fbdev_read] u32PNL_Width %lu u32PNL_Height %lu , software Width %lu Height %lu\n", u32PNL_Width, u32PNL_Height, *width, *height);
        return 0;
    }

    mSrcWindow_info.x = 0;
    mSrcWindow_info.y = 0;
    mSrcWindow_info.width = m_panelWidth;
    mSrcWindow_info.height = m_panelHeight;

    mDestWindow_info.x = 0;
    mDestWindow_info.y = 0;
    mDestWindow_info.width = *width;
    mDestWindow_info.height = *height;

    u32DestBufSize = mDestWindow_info.width * mDestWindow_info.height *4;  //ARGB8888
    if (!stPnlExtStatus.bYUVOutput) {
        u32DwinBufSize = m_panelWidth * m_panelHeight *4; //tv: DWIN_DATA_FMT_ARGB8888
        stDWINFmt = DWIN_DATA_FMT_ARGB8888;
        stGFXFormat = GFX_FMT_ARGB8888;
    } else {
        u32DwinBufSize = m_panelWidth * m_panelHeight *2; //Dwin YUV
        stDWINFmt = DWIN_DATA_FMT_UV8Y8;
        stGFXFormat = GFX_FMT_YUV422;
        gap = 4*1024*1024;
    }

    //when 4K2K video or photo, dwin caputer with YUV format to decrease MIU BW using
    if ((m_panelWidth > panelWidth) || (m_panelHeight > panelHeight)) {
        u32DwinBufSize = m_panelWidth * m_panelHeight *2; //Dwin YUV
        stDWINFmt = DWIN_DATA_FMT_UV8Y8;
        stGFXFormat = GFX_FMT_YUV422;
        gap = 4*1024*1024;
    }

    if (mirrorMode) {
        if (mirrorModeH) {
            bMirrorH = TRUE;
        }
        if (mirrorModeV) {
            bMirrorV = TRUE;
        }
    }

    //we should reserve gap between ge input buffer(dwin buffer)  and ge output buffer!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //for src GFX_FMT_ARGB8888 dest GFX_FMT_ABGR8888 not mirror case, gap 1M(1920*1080)
    //for src DWIN_DATA_FMT_UV8Y8 dest GFX_FMT_ABGR8888, gap 4M(1920*1080)
    destPAaddr = paStart;                                   //ge buffer,
    if (bMirrorV) {
        if (stPnlExtStatus.bYUVOutput) { //box not support mirror
            ALOGE("[DwinHardware] box not support mirror\n");
            return -1;
        }
        srcPAaddr = paStart;
    } else {
        srcPAaddr = ((paEnd - u32DwinBufSize) & (~GOP_PIXEL_ALIGNMENT_FACTOR)); //dwin output buffer, ge src buffer
        if ((srcPAaddr-destPAaddr) < gap) {
            ALOGE("[DwinHardware] dwin capture buffer too small srcaddr %lu destaddr %lu\n", srcPAaddr, destPAaddr);
            return -1;
        }
    }

    if ((u32DwinBufSize+gap) > (paEnd-paStart)) {
        ALOGE("[DwinHardware]membuffer size too small, expected %08x but now %08x", (unsigned int)(u32DwinBufSize+gap), (unsigned int)(paEnd-paStart));
        return -1;
    }

    //lock buffer E_DFB_JPD_WRITE in mmap for dwin capture
#if CO_BUFFER_MUTEX
    MsOS_LockMutex(dwinCaptureMutexId, 0);
#endif
    GOP_DWIN_CAP stCap={FALSE, FALSE};
    if (GOP_API_SUCCESS != MApi_GOP_GetChipCaps(E_GOP_CAP_DWIN, (MS_U32 *)&stCap, sizeof(GOP_DWIN_CAP)))
        goto DWINCAPTURE_ERROR;

    if (stCap.bSupportOSDCapture == TRUE) { //A3 support dwin
        if (!_CaptureScreen(&mSrcWindow_info, stDWINFmt, srcPAaddr, u32DwinBufSize, mode))
            goto DWINCAPTURE_ERROR;
    } else {
        ALOGD("bSupportOSDCapture is false\n");
        goto DWINCAPTURE_ERROR;
    }

    ALOGD("w=%d, h=%d, mirrorH %d, mirrorV %d, src = %08x, dest = %08x", m_panelWidth, m_panelHeight, bMirrorH, bMirrorV,(unsigned int)srcPAaddr,(unsigned int)destPAaddr);

    if (stGFXFormat == GFX_FMT_YUV422)
        MApi_GFX_SetDC_CSC_FMT(GFX_YUV_RGB2YUV_PC, GFX_YUV_OUT_255, GFX_YUV_IN_255, GFX_YUV_YVYU,  GFX_YUV_YVYU);

    if (bMirrorV) {
        if (!_StretchBlit(&mSrcWindow_info, &mDestWindow_info, srcPAaddr, srcPAaddr, stGFXFormat, GFX_FMT_ABGR8888, bMirrorH, bMirrorV))
            goto DWINCAPTURE_ERROR;
        destPAaddr = srcPAaddr;
    } else {
        if (!_StretchBlit(&mSrcWindow_info, &mDestWindow_info, srcPAaddr, destPAaddr, stGFXFormat, GFX_FMT_ABGR8888, bMirrorH, bMirrorV))
            goto DWINCAPTURE_ERROR;
    }
    memcpy(data, (void*)MsOS_PA2KSEG1(destPAaddr), u32DestBufSize);

    //unlock buffer E_DFB_JPD_WRITE in mmap for dwin capture
#if CO_BUFFER_MUTEX
    MsOS_UnlockMutex(dwinCaptureMutexId, 0);
#endif
    return 0;

    DWINCAPTURE_ERROR:
#if CO_BUFFER_MUTEX
    MsOS_UnlockMutex(dwinCaptureMutexId, 0);
#endif
    return -1;
}

int fbdev_setGopStretchWin(int gopNo, int srcWidth, int srcHeight, int dest_Width, int dest_Height) {
    panelWidth = dest_Width;
    panelHeight = dest_Height;
    return gfx_GOPStretch(gopNo, srcWidth, srcHeight, dest_Width, dest_Height);
}

unsigned int fbdev_getMIU1Offset() {
    return mmap_get_miu0_offset();
}

int fbdev_setDisplayMode(int  displayMode) {
    g_fbdev_displaymode = displayMode;
    return 0;
}

static int getBufferCount(int width, int height) {
    int bufferCount = 0;
    int bufferSize = width * height *fb_vinfo.bits_per_pixel/8;
    if (u32FBLength/bufferSize >= 3) {
        bufferCount= 3;
    } else {
        bufferCount = u32FBLength/bufferSize;
    }
    return bufferCount ;
}

static unsigned int getPhyAddrForTempBuffer(unsigned int lastBufferStartAddr,int oldBuffSize,int newBufferSize,unsigned int fbstart,int fbLength) {
    unsigned int lastBufferEndAddr = lastBufferStartAddr + oldBuffSize;
    unsigned int newBufferEndAddr = fbstart + newBufferSize*2;
    unsigned int fbEnd = fbstart + fbLength;

    if ((newBufferEndAddr+oldBuffSize) > fbEnd) {
        newBufferEndAddr = fbstart + newBufferSize;
    }

    if ((lastBufferEndAddr+oldBuffSize) > fbEnd) {
        ALOGD("the last buffer the largest-index buffer, so no need to blit the last buffer to the temp buffer ");
        return lastBufferStartAddr;
    } else {
        if (lastBufferEndAddr>=newBufferEndAddr) {
            if (lastBufferStartAddr > newBufferEndAddr) {
                return lastBufferStartAddr;
            } else {
                return lastBufferEndAddr;
            }
        } else {
            return newBufferEndAddr;
        }
    }
}

static int stretchBlit( GFX_BufferInfo srcbuf, GFX_BufferInfo  dstbuf,GFX_Block src_rect, GFX_Block dest_rect) {
    GFX_Point v0, v1;
    GFX_DrawRect bitbltInfo;
    MApi_GFX_BeginDraw();

    bitbltInfo.srcblk = src_rect;
    bitbltInfo.dstblk = dest_rect;

    v0.x = 0;
    v0.y = 0;
    v1.x = dstbuf.u32Width;
    v1.y = dstbuf.u32Height;
    MApi_GFX_SetClip(&v0, &v1);

    if (MApi_GFX_SetSrcBufferInfo(&srcbuf, 0) != GFX_SUCCESS) {
        ALOGE("stretchblit GFX set SrcBuffer failed\n");
        goto STRETCH_BLIT_ERROR;
    }

    if (MApi_GFX_SetDstBufferInfo(&dstbuf, 0) != GFX_SUCCESS) {
        ALOGE("stretchblit GFX set DetBuffer failed\n");
        goto STRETCH_BLIT_ERROR;
    }

    if (MApi_GFX_BitBlt(&bitbltInfo, GFXDRAW_FLAG_SCALE) != GFX_SUCCESS) {
        ALOGE("stretch GFX BitBlt failed\n");
        goto STRETCH_BLIT_ERROR;
    }

    MApi_GFX_EndDraw();
    MApi_GFX_SetTAGID(MApi_GFX_GetNextTAGID(TRUE));

    return true;
    STRETCH_BLIT_ERROR:
    MApi_GFX_EndDraw();
    return false;
}

int fbdev_setGopInfo(int width, int height) {
    int oldBufferSize = 0;
    int newBufferSize = 0;
    LastBuffer.width = fb_vinfo.xres;
    LastBuffer.height = fb_vinfo.yres;
    LastBuffer.lineLength = fb_finfo.line_length;
    oldBufferSize = LastBuffer.height*LastBuffer.lineLength;
    newBufferSize = height*width*fb_vinfo.bits_per_pixel/8;
    LastBuffer.physicAddr = fb_finfo.smem_start+fb_finfo.line_length * fb_vinfo.yoffset;
    LastBuffer.backupBufferPhyAddr = getPhyAddrForTempBuffer(LastBuffer.physicAddr,oldBufferSize,newBufferSize,fb_finfo.smem_start,u32FBLength);
    lastBufferSize = oldBufferSize;
    resChanged = true;

    // set fb_Info and fb_vinfo
    int bufferCount = getBufferCount(width,height);
    fb_vinfo.xres = width;
    fb_vinfo.yres = height;
    fb_vinfo.xres_virtual = width;
    fb_vinfo.yres_virtual = height*bufferCount;
    fb_vinfo.xoffset = 0;
    fb_vinfo.yoffset = 0;

    fb_vinfo.reserved[0] = width;
    fb_vinfo.reserved[1] = height;

    fb_vinfo.pixclock = 100000000000LLU / (6 *  fb_vinfo.xres * fb_vinfo.yres);
    fb_vinfo.left_margin = 0;
    fb_vinfo.hsync_len = 0;

    fb_finfo.smem_len = width * height * fb_vinfo.bits_per_pixel/8;
    fb_finfo.line_length = width * fb_vinfo.bits_per_pixel/8;

    gSafeFbQueueCnt = bufferCount - 1;

    blitLastBufferToTempBuffer();

#if 0
    if (Androidfbid !=- 1) {
        MApi_GOP_GWIN_DestroyFB(Androidfbid);
    }
    MApi_GOP_GWIN_CreateFBFrom3rdSurf(fb_vinfo.xres, fb_vinfo.yres, gfmt,fb_finfo.smem_start,fb_finfo.line_length,&Androidfbid);
    MApi_GOP_GWIN_MapFB2Win(Androidfbid, gwin);
    if ((width<=panelWidth)&&(height<=panelHeight)) {
        gfx_GOPStretch(gop_and, fb_vinfo.xres, fb_vinfo.yres, panelWidth, panelHeight);
    } else {
        gfx_GOPStretch(gop_and, fb_vinfo.xres, fb_vinfo.yres, width, height);
    }
    ALOGI("fbdev_setGopInfo was finished!");
#endif
    return 0;
}

int fbdev_getCurrentRes(int * width, int * height) {
    *width = fb_vinfo.xres;
    *height = fb_vinfo.yres;
    return 0;
}

int fbdev_clearFramebuffer() {
#if 0
    MS_U32 length = fb_vinfo.yres_virtual*fb_finfo.line_length;
    GFX_Result result = MApi_GFX_ClearFrameBufferByWord(fb_finfo.smem_start, length , 0x00);
    if (result!=GFX_SUCCESS) {
        ALOGI("Johnny fbdev_clearFramebuffer encounter error!!");
        return -1;
    }
#endif
    return 0;
}

static void blitLastBufferToTempBuffer() {
    int currentGOP = -1;
    int goptimeout = 0;
    unsigned int tagID = 0;
    LastBuffer.backupFBID = -1;
    GFX_BufferInfo srcBuffer;
    GFX_BufferInfo destBuffer;
    GFX_Block src_rect;
    GFX_Block dest_rect;

    ALOGD("the fb start addr is %x",fb_finfo.smem_start);
    ALOGD("the length is %x",u32FBLength);
    ALOGD("the lastbuffer addr is %x",LastBuffer.physicAddr);
    ALOGD("the tempBuffer addr is %x",LastBuffer.backupBufferPhyAddr);
    ALOGD("the oldBufferSize is %x",LastBuffer.height*LastBuffer.lineLength);
    ALOGD("the newBufferSize is %x",fb_finfo.line_length*fb_vinfo.yres);
    ALOGD("the newBuffer staringaddr:%x  ending addr %x",fb_finfo.smem_start,fb_finfo.smem_start+fb_finfo.line_length*fb_vinfo.yres);
    srcBuffer.u32Addr = LastBuffer.physicAddr;
    srcBuffer.u32Width = LastBuffer.width;
    srcBuffer.u32Height = LastBuffer.height;
    srcBuffer.u32Pitch = LastBuffer.lineLength;
    srcBuffer.u32ColorFmt = GFX_FMT_ABGR8888;
    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = LastBuffer.width;
    src_rect.height = LastBuffer.height;

    // blit the lastbuffer to tempbufferaddr
    if (LastBuffer.physicAddr!=LastBuffer.backupBufferPhyAddr) {
        destBuffer.u32Addr = LastBuffer.backupBufferPhyAddr;
        destBuffer.u32Width = LastBuffer.width;
        destBuffer.u32Height = LastBuffer.height;
        destBuffer.u32Pitch = LastBuffer.lineLength;
        destBuffer.u32ColorFmt = GFX_FMT_ABGR8888;
        dest_rect.x = 0;
        dest_rect.y = 0;
        dest_rect.width = LastBuffer.width;
        dest_rect.height = LastBuffer.height;
        MApi_GFX_EnableDFBBlending(FALSE);
        MApi_GFX_EnableAlphaBlending(FALSE);
        stretchBlit(srcBuffer,destBuffer,src_rect,dest_rect);
        tagID = MApi_GFX_GetNextTAGID(FALSE);
        MApi_GFX_WaitForTAGID(tagID);
        MApi_GOP_GWIN_CreateFBFrom3rdSurf(LastBuffer.width, LastBuffer.height, gfmt,LastBuffer.backupBufferPhyAddr,LastBuffer.lineLength,&LastBuffer.backupFBID);
        currentGOP = MApi_GOP_GWIN_GetCurrentGOP();
        MApi_GOP_GWIN_SwitchGOP(0);
        MApi_GOP_GWIN_UpdateRegOnceEx(TRUE, FALSE);
        MApi_GOP_GWIN_MapFB2Win(LastBuffer.backupFBID, gwin);
        MApi_GOP_GWIN_UpdateRegOnceEx(FALSE, FALSE);

        while (!MApi_GOP_IsRegUpdated(gop_and) && (goptimeout++ <= GOP_TIMEOUT_CNT1))
            usleep(10000);

        MApi_GOP_GWIN_SwitchGOP(currentGOP);
    }
}


static void blitTempBufferToFirstBuffer() {

    GFX_BufferInfo srcBuffer;
    GFX_BufferInfo destBuffer;
    GFX_Block src_rect;
    GFX_Block dest_rect;

    srcBuffer.u32Addr = LastBuffer.backupBufferPhyAddr;
    srcBuffer.u32Width = LastBuffer.width;
    srcBuffer.u32Height = LastBuffer.height;
    srcBuffer.u32Pitch = LastBuffer.lineLength;
    srcBuffer.u32ColorFmt = GFX_FMT_ABGR8888;
    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = LastBuffer.width;
    src_rect.height = LastBuffer.height;

    // blit the backupBuffer to fb_finfo.smem_start
    destBuffer.u32Addr = fb_finfo.smem_start;
    destBuffer.u32Width = fb_vinfo.xres;
    destBuffer.u32Height = fb_vinfo.yres;
    destBuffer.u32Pitch = fb_vinfo.xres*fb_vinfo.bits_per_pixel/8;
    destBuffer.u32ColorFmt = GFX_FMT_ABGR8888;
    dest_rect.x = 0;
    dest_rect.y = 0;
    dest_rect.width  = fb_vinfo.xres;
    dest_rect.height = fb_vinfo.yres;
    MApi_GFX_EnableDFBBlending(FALSE);
    MApi_GFX_EnableAlphaBlending(FALSE);
    MApi_GFX_SetAlphaSrcFrom(ABL_FROM_ASRC);
    stretchBlit(srcBuffer,destBuffer,src_rect,dest_rect);
}

unsigned int fbdev_getMaskPhyAdr(int u8MiuSel) {
    unsigned int miu0Len = mmap_get_miu0_length();
    unsigned int miu1Len = mmap_get_miu1_length();

    if (u8MiuSel==0) {
        ALOGI("fbdev_getMaskPhyAdr MIU0 mask=%lx", (miu0Len-1));
        return(miu0Len - 1) ;
    } else {
        ALOGI("fbdev_getMaskPhyAdr MIU1 mask=%lx", (miu1Len-1));
        return(miu1Len - 1);
    }
}
static bool ConfigureGopGwin(int osdMode,MS_U32 physicalAddr,Region OSDRegion) {
    MS_U8 u8OrgFbId = MApi_GOP_GWIN_GetFBfromGWIN(gwin);
    switch (osdMode) {
        case DISPLAYMODE_TOPBOTTOM_LA:
        case DISPLAYMODE_NORMAL_LA:
        case DISPLAYMODE_TOP_LA:
        case DISPLAYMODE_BOTTOM_LA:
            {
                GOP_GwinFBAttr fbInfo;
                MS_U32 fb2_addr;
                if (u8SubFbId != MAX_GWIN_FB_SUPPORT) {
                    MApi_GOP_GWIN_DeleteFB(u8SubFbId);
                    u8SubFbId = MAX_GWIN_FB_SUPPORT;
                }
                MApi_GOP_GWIN_CreateFBFrom3rdSurf(cur_OSDRegion.width, cur_OSDRegion.height>>1, gfmt,physicalAddr,fb_finfo.line_length,&u8MainFbId);
                MApi_GOP_GWIN_GetFBInfo(u8MainFbId,&fbInfo);
                u32SubAddrOffset = fbInfo.pitch * fbInfo.height;
                fb2_addr = fbInfo.addr + u32SubAddrOffset;
                MApi_GOP_GWIN_CreateFBFrom3rdSurf(cur_OSDRegion.width, cur_OSDRegion.height>>1, gfmt,fb2_addr,fb_finfo.line_length,&u8SubFbId);
                MApi_GOP_GWIN_MapFB2Win(u8MainFbId, gwin);
                MApi_GOP_GWIN_DeleteFB(u8OrgFbId);
                MApi_GOP_Set3DOSDMode(gwin, u8MainFbId, u8SubFbId, E_GOP_3D_LINE_ALTERNATIVE);
                if (bNeedMonitorTiming) {
                    MApi_GOP_Set3DOSDMode((bEnableFRC==TRUE)?gwin_frc: gwin, u8MainFbId, u8SubFbId, E_GOP_3D_LINE_ALTERNATIVE);
                    qpAndroidFRCPropInfo->u8LSideFBID   =   u8MainFbId;
                    qpAndroidFRCPropInfo->u8LSideGOPNum =   gop_and;
                    qpAndroidFRCPropInfo->u8LSideGwinID =   gwin;
                    qpAndroidFRCPropInfo->u8RSideFBID   =   u8MainFbId;
                    qpAndroidFRCPropInfo->u8RSideGOPNum =   (bEnableFRC==TRUE)?gop_and_FRC: gop_and;
                    qpAndroidFRCPropInfo->u8RSideGwinID =   (bEnableFRC==TRUE)?gwin_frc: gwin;
                    UpdateAndroidWinInfo(qpAndroidFRCPropInfo);
                } else {
                    gfx_GOPStretch(gop_and, fb_vinfo.xres, (fb_vinfo.yres>>1), panelWidth, (panelHeight>>1));
                    //Set GWIN info again after re-set stretchwin and enable 3D
                    MApi_GOP_GWIN_MapFB2Win(u8MainFbId, gwin);
                }
                break;
            }
        case DISPLAYMODE_LEFTRIGHT_FR:
        case DISPLAYMODE_NORMAL_FR:
            {
                MS_U32 fb2_addr=0, u32WordUnit=0;
                MS_U8 u8BytePerPixel = 4;
                E_GOP_API_Result result=GOP_API_SUCCESS;

                //For SBS->FR, re-create FB as H-Half frame size
                result = MApi_GOP_GetChipCaps(E_GOP_CAP_WORD_UNIT,&u32WordUnit,sizeof(MS_U32));
                if ((GOP_API_SUCCESS==result)&&(u32WordUnit != 0)) {
                    MS_U32 u32FbId = MAX_GWIN_FB_SUPPORT;
                    //address to half line end
                    u32SubAddrOffset = (cur_OSDRegion.width>>1)*u8BytePerPixel;
                    if ((u32SubAddrOffset%u32WordUnit) != 0) {
                        ALOGE("Attention! Wrong H resolution[%u] for 3D output, fmt=%u\n", fb_vinfo.xres, gfmt);
                        return false; //Wrong res, must obey GOP address alignment rule(1 Word)
                    }
                    if (u8SubFbId != MAX_GWIN_FB_SUPPORT) {
                        MApi_GOP_GWIN_DeleteFB(u8SubFbId);
                        u8SubFbId = MAX_GWIN_FB_SUPPORT;
                    }
                    MApi_GOP_GWIN_CreateFBFrom3rdSurf(cur_OSDRegion.width>>1,cur_OSDRegion.height, gfmt, physicalAddr, fb_finfo.line_length, &u8MainFbId);
                    //Set sub address to half line end
                    fb2_addr = physicalAddr + u32SubAddrOffset;
                    MApi_GOP_GWIN_CreateFBFrom3rdSurf(cur_OSDRegion.width>>1, cur_OSDRegion.height, gfmt, fb2_addr, fb_finfo.line_length, &u8SubFbId);
                    MApi_GOP_GWIN_MapFB2Win(u8MainFbId, gwin);
                    MApi_GOP_GWIN_DeleteFB(u8OrgFbId);
                    MApi_GOP_Set3DOSDMode(gwin, u8MainFbId, u8SubFbId, E_GOP_3D_SWITH_BY_FRAME);
                    if (bNeedMonitorTiming) {
                        MApi_GOP_Set3DOSDMode((bEnableFRC==TRUE)?gwin_frc: gwin, u8MainFbId, u8SubFbId, E_GOP_3D_SWITH_BY_FRAME);
                        qpAndroidFRCPropInfo->u8LSideFBID   = u8MainFbId;
                        qpAndroidFRCPropInfo->u8LSideGOPNum = gop_and;
                        qpAndroidFRCPropInfo->u8LSideGwinID = gwin;
                        qpAndroidFRCPropInfo->u8RSideFBID   = u8MainFbId;
                        qpAndroidFRCPropInfo->u8RSideGOPNum = (bEnableFRC==TRUE)?gop_and_FRC: gop_and;
                        qpAndroidFRCPropInfo->u8RSideGwinID = (bEnableFRC==TRUE)?gwin_frc: gwin;
                        UpdateAndroidWinInfo(qpAndroidFRCPropInfo);
                    } else {
                        gfx_GOPStretch(gop_and, (fb_vinfo.xres>>1), fb_vinfo.yres, panelWidth, panelHeight);
                        MApi_GOP_GWIN_MapFB2Win(u8MainFbId, gwin);//Set GWIN info again after re-set stretchwin and enable 3D
                    }
                }
                break;
            }
        case DISPLAYMODE_NORMAL_FP:
            {
                if (((cur_OSDRegion.width==1920)&&(cur_OSDRegion.height==1080))
                    ||((cur_OSDRegion.width==1280)&&(cur_OSDRegion.height==720))) {
                    u32SubAddrOffset =0;
                    if (u8SubFbId != MAX_GWIN_FB_SUPPORT) {
                        MApi_GOP_GWIN_DeleteFB(u8SubFbId);
                        u8SubFbId = MAX_GWIN_FB_SUPPORT;
                    }
                    MApi_GOP_GWIN_CreateFBFrom3rdSurf(cur_OSDRegion.width,cur_OSDRegion.height, gfmt, physicalAddr, fb_finfo.line_length, &u8MainFbId);
                    MApi_GOP_GWIN_MapFB2Win(u8MainFbId, gwin);
                    MApi_GOP_GWIN_DeleteFB(u8OrgFbId);
                    MApi_GOP_Set3DOSDMode(gwin, u8MainFbId, u8MainFbId, E_GOP_3D_FRAMEPACKING);
                    if (bNeedMonitorTiming) {
                        // if frc enable, set 3d mode for the frc gwin
                        MApi_GOP_Set3DOSDMode((bEnableFRC==TRUE)?gwin_frc: gwin, u8MainFbId, u8MainFbId, E_GOP_3D_FRAMEPACKING);
                        qpAndroidFRCPropInfo->u8LSideFBID   = u8MainFbId;
                        qpAndroidFRCPropInfo->u8LSideGOPNum = gop_and;
                        qpAndroidFRCPropInfo->u8LSideGwinID = gwin;
                        qpAndroidFRCPropInfo->u8RSideFBID   = u8MainFbId;
                        qpAndroidFRCPropInfo->u8RSideGOPNum = (bEnableFRC==TRUE)?gop_and_FRC: gop_and;
                        qpAndroidFRCPropInfo->u8RSideGwinID = (bEnableFRC==TRUE)?gwin_frc: gwin;
                        UpdateAndroidWinInfo(qpAndroidFRCPropInfo);
                    } else {
                        gfx_GOPStretch(gop_and, cur_OSDRegion.width, cur_OSDRegion.height,cur_OSDRegion.width, cur_OSDRegion.height);
                        MApi_GOP_GWIN_MapFB2Win(u8MainFbId, gwin);//Set GWIN info again after re-set stretchwin
                    }
                }
                break;
            }
        case DISPLAYMODE_NORMAL:
        case DISPLAYMODE_LEFTRIGHT:
        case DISPLAYMODE_TOPBOTTOM:
        case DISPLAYMODE_TOP_ONLY:
        case DISPLAYMODE_BOTTOM_ONLY:
        case DISPLAYMODE_RIGHT_ONLY:
        default:
            {
                u32SubAddrOffset =0;
                if (u8SubFbId != MAX_GWIN_FB_SUPPORT) {
                    MApi_GOP_GWIN_DeleteFB(u8SubFbId);
                    u8SubFbId = MAX_GWIN_FB_SUPPORT;
                }
                MApi_GOP_GWIN_CreateFBFrom3rdSurf(cur_OSDRegion.width,cur_OSDRegion.height, gfmt, physicalAddr, fb_finfo.line_length, &u8MainFbId);
                MApi_GOP_GWIN_MapFB2Win(u8MainFbId, gwin);
                MApi_GOP_GWIN_DeleteFB(u8OrgFbId);
                MApi_GOP_Set3DOSDMode(gwin, u8MainFbId, MAX_GWIN_FB_SUPPORT, E_GOP_3D_DISABLE);
                if (bNeedMonitorTiming) {
                    MApi_GOP_Set3DOSDMode((bEnableFRC==TRUE)?gwin_frc: gwin, u8MainFbId, MAX_GWIN_FB_SUPPORT, E_GOP_3D_DISABLE);
                    qpAndroidFRCPropInfo->u8LSideFBID   = u8MainFbId;
                    qpAndroidFRCPropInfo->u8LSideGOPNum = gop_and;
                    qpAndroidFRCPropInfo->u8LSideGwinID = gwin;
                    qpAndroidFRCPropInfo->u8RSideFBID   = u8MainFbId;
                    qpAndroidFRCPropInfo->u8RSideGOPNum = (bEnableFRC==TRUE)?gop_and_FRC: gop_and;
                    qpAndroidFRCPropInfo->u8RSideGwinID = (bEnableFRC==TRUE)?gwin_frc: gwin;
                    UpdateAndroidWinInfo(qpAndroidFRCPropInfo);
                } else {
                    gfx_GOPStretch(gop_and, fb_vinfo.xres, fb_vinfo.yres, panelWidth, panelHeight);
                    MApi_GOP_GWIN_MapFB2Win(u8MainFbId, gwin);//Set GWIN info again after re-set stretchwin
                }
                break;
            }
    }
    return true;
}

static MS_U32 getCurrentFBPhys() {
    return curFBPhysicAddr;
}

static void setCurrentFBPhys(MS_U32 physicalAddr) {
    curFBPhysicAddr = physicalAddr;
}

static bool getCurOPTiming(int *ret_width, int *ret_height) {
    MS_PNL_DST_DispInfo dstDispInfo;
    memset(&dstDispInfo,0,sizeof(MS_PNL_DST_DispInfo));
    MApi_PNL_GetDstInfo(&dstDispInfo, sizeof(MS_PNL_DST_DispInfo));
    *ret_width  = dstDispInfo.DEHEND - dstDispInfo.DEHST + 1;
    *ret_height = dstDispInfo.DEVEND - dstDispInfo.DEVST + 1;
    return true;
}


static bool getCurOCTiming(int *ret_width,int *ret_height) {
    MS_OSDC_DST_DispInfo dstDispInfo;
    memset(&dstDispInfo,0,sizeof(MS_OSDC_DST_DispInfo));
    dstDispInfo.ODSC_DISPInfo_Version =ODSC_DISPINFO_VERSIN;
    MApi_XC_OSDC_GetDstInfo(&dstDispInfo,sizeof(MS_OSDC_DST_DispInfo));
    *ret_width  = dstDispInfo.DEHEND-dstDispInfo.DEHST + 1;
    *ret_height = dstDispInfo.DEVEND - dstDispInfo.DEVST + 1;
    return true;
}

void* timingCheck_workthread(void *param) {
    MS_BOOL bNeedfrcdest = false;
    MS_BOOL bPrevfrcdest = false;
    while (1) {
        pthread_mutex_lock(&gFrcDest_mutex);
        if ((req_OSDRegion.width==cur_OSDRegion.width)&&
            (req_OSDRegion.height==cur_OSDRegion.height)) {
            SetAndroidGOPDest();
        }
        pthread_mutex_unlock(&gFrcDest_mutex);
        usleep(16000);
    }
}

int getNewGOPdest() {
    int newGOPdest = -1;
    char property[PROPERTY_VALUE_MAX];

    if (bNeedMonitorTiming) {
        switch (panelType) {
            case E_PANEL_FULLHD:
            case E_PANEL_FULLHD_URSA6:
            case E_PANEL_4K2K_URSA6:
            case E_PANEL_FULLHD_URSA8:
            case E_PANEL_4K2K_URSA8:
            case E_PANEL_4K2K_CSOT:
                newGOPdest = E_GOP_DST_OP0;
                break;
            case E_PANEL_FULLHD_URSA7:
            case E_PANEL_4K2K_URSA7:
                if ((cur_OSDRegion.width<RES_4K2K_WIDTH)&&(cur_OSDRegion.height<RES_4K2K_HEIGHT)) {
                    newGOPdest = E_GOP_DST_FRC;
                } else if ((cur_OSDRegion.width==RES_4K2K_WIDTH)&&(cur_OSDRegion.height==RES_4K2K_HEIGHT)) {
                    newGOPdest = E_GOP_DST_OP0;
                }
                break;
            case E_PANEL_FULLHD_URSA9:
            case E_PANEL_4K2K_URSA9:
                newGOPdest = E_GOP_DST_FRC;
                break;
            default:
                newGOPdest = E_GOP_DST_OP0;
                break;
        }
    } else {
        newGOPdest = E_GOP_DST_OP0;
    }

    if ((property_get("mstar.recoverymode", property, NULL) > 0)) {
        newGOPdest = E_GOP_DST_OP0;
    }
    return newGOPdest;
}

void SetAndroidGOPDest() {
    int newGOPdest = -1;
    bool bTimingChanged = false;
    char property[PROPERTY_VALUE_MAX];

    if (!(property_get("mstar.recoverymode", property, NULL) > 0)) {
        if ((panelType==E_PANEL_FULLHD_URSA7)||(panelType==E_PANEL_4K2K_URSA7)||
            (panelType==E_PANEL_FULLHD_URSA9)||(panelType==E_PANEL_4K2K_URSA9)) {
            getCurOCTiming(&(cur_OCTiming.width),&(cur_OCTiming.height));
            if ((cur_OCTiming.width!=pre_OCTimingInfo.width)||(cur_OCTiming.height!=pre_OCTimingInfo.height)) {
                bTimingChanged = true;
                pre_OCTimingInfo = cur_OCTiming;
            }
        }
    }

    getCurOPTiming(&(cur_OPTiming.width),&(cur_OPTiming.height));
    //ALOGD("\nthe cur_OPTiming width is %d the height is %d\n",cur_OPTiming.width,cur_OPTiming.height);
    //ALOGD("\nthe pre_OPTiming width is %d the height is %d\n",pre_OPTimingInfo.width,pre_OPTimingInfo.height);
    if ((cur_OPTiming.width!=pre_OPTimingInfo.width)||(cur_OPTiming.height!=pre_OPTimingInfo.height)) {
        bTimingChanged = true;
        pre_OPTimingInfo = cur_OPTiming;
    }


    if (bTimingChanged||bOSDRegionChanged) {
        int goptimeout = 0;
        int currentGOP = MApi_GOP_GWIN_GetCurrentGOP();
        newGOPdest = getNewGOPdest();
        MApi_GOP_GWIN_Enable(gwin, false);
        MApi_GOP_GWIN_SetBnkForceWrite(gop_and,TRUE);
        MApi_GOP_GWIN_SwitchGOP(gop_and);
        MApi_GOP_GWIN_UpdateRegOnceEx(FALSE, FALSE);

        if (bEnableFRC) {
            MApi_GOP_GWIN_Enable(gwin_frc, false);
            MApi_GOP_GWIN_SetBnkForceWrite(gop_and_FRC,TRUE);
            MApi_GOP_GWIN_SwitchGOP(gop_and_FRC);
            MApi_GOP_GWIN_UpdateRegOnceEx(FALSE, FALSE);
        }


        MApi_GOP_GWIN_UpdateRegOnceEx(TRUE, FALSE);
        if (newGOPdest==E_GOP_DST_BYPASS) {
            GOP_MuxConfig muxConfig;
            GOP_GwinInfo info;
            MS_U32 physicalAddr = 0;
            muxConfig.u8MuxCounts = 2;
            muxConfig.GopMux[0].u8MuxIndex = EN_GOP_FRC_MUX2;
            muxConfig.GopMux[0].u8GopIndex = gop_and;
            muxConfig.GopMux[1].u8MuxIndex = EN_GOP_FRC_MUX3;
            muxConfig.GopMux[1].u8GopIndex = (bEnableFRC==TRUE)?gop_and_FRC: gop_and;
            MApi_GOP_GWIN_SetMux(&muxConfig,sizeof(muxConfig));

            MApi_GOP_GWIN_SetGOPDst(gop_and, E_GOP_DST_BYPASS);
            MApi_GOP_GWIN_SetGOPDst((bEnableFRC==TRUE)?gop_and_FRC: gop_and, E_GOP_DST_BYPASS);
            qpAndroidFRCPropInfo->eGOPDst = E_GOP_DST_BYPASS;
            physicalAddr = getCurrentFBPhys();
            ConfigureGopGwin(g_fbdev_displaymode,physicalAddr,cur_OSDRegion);
            MApi_GOP_GWIN_Enable(gwin, true);
            MApi_GOP_GWIN_Enable((bEnableFRC==TRUE)?gwin_frc: gwin, true);
            //disable hwcurosr when dst on FRC
            property_set(MSTAR_DESK_ENABLE_HWCURSOR, "0");
        } else if (newGOPdest==E_GOP_DST_OP0) {
            MS_U32 physicalAddr = 0;
            MApi_GOP_GWIN_SetGOPDst(gop_and, E_GOP_DST_OP0);
            qpAndroidFRCPropInfo->eGOPDst = E_GOP_DST_OP0;
            physicalAddr = getCurrentFBPhys();
            ConfigureGopGwin(g_fbdev_displaymode,physicalAddr,cur_OSDRegion);
            if ((cur_OPTiming.width>=cur_OSDRegion.width)&&(cur_OPTiming.height>=cur_OSDRegion.height)) {
                MApi_GOP_GWIN_Enable(gwin, true);
            }
            if (bEnableFRC) {
                MApi_GOP_GWIN_Enable(gwin_frc, false);
            }
            //enable hwcursor when dst on op
            property_set(MSTAR_DESK_ENABLE_HWCURSOR, "1");
        } else if (newGOPdest==E_GOP_DST_FRC) {
            MS_U32 physicalAddr = 0;
            MApi_GOP_GWIN_SetGOPDst(gop_and, E_GOP_DST_FRC);
            qpAndroidFRCPropInfo->eGOPDst = E_GOP_DST_FRC;
            physicalAddr = getCurrentFBPhys();
            ConfigureGopGwin(g_fbdev_displaymode,physicalAddr,cur_OSDRegion);
            MApi_GOP_GWIN_Enable(gwin, true);
            property_set(MSTAR_DESK_ENABLE_HWCURSOR, "1");
        }

        MApi_GOP_GWIN_SwitchGOP(gop_and);
        MApi_GOP_GWIN_UpdateRegOnceEx(FALSE, FALSE);
        if (bEnableFRC) {
            MApi_GOP_GWIN_SwitchGOP(gop_and_FRC);
            MApi_GOP_GWIN_UpdateRegOnceEx(FALSE, FALSE);
        }

        andGOPdest = newGOPdest;
        while (!MApi_GOP_IsRegUpdated(gop_and) && (goptimeout++ <= GOP_TIMEOUT_CNT1))
            usleep(10000);
        MApi_GOP_GWIN_SwitchGOP(currentGOP);
    }

    if (andGOPdest==E_GOP_DST_BYPASS) {
        //ALOGD(" !!!!set the gop dest to bFRCDest");
    } else if (andGOPdest==E_GOP_DST_OP0) {
        //ALOGD("!!!! set the GOP dest to OP0");
    }
}

void UpdateAndroidWinInfo(AndroidFRCPropertyInfo* pAndroidFRCPropInfo) {
    int osdMode = 0;
    GOP_GwinInfo info_L, info_R;
    MS_U32 u32FlipAddr_A=0, u32FlipAddr_B=0;
    memset(&info_L, 0, sizeof(GOP_GwinInfo));
    memset(&info_R, 0, sizeof(GOP_GwinInfo));
    osdMode = g_fbdev_displaymode;
    int srcWidth =  cur_OSDRegion.width;
    int srcHeight = cur_OSDRegion.height;

    MApi_GOP_GWIN_MapFB2Win( pAndroidFRCPropInfo->u8LSideFBID, pAndroidFRCPropInfo->u8LSideGwinID);
    MApi_GOP_GWIN_GetWinInfo(pAndroidFRCPropInfo->u8LSideGwinID,&info_L);
    MApi_GOP_GWIN_MapFB2Win( pAndroidFRCPropInfo->u8RSideFBID, pAndroidFRCPropInfo->u8RSideGwinID);
    MApi_GOP_GWIN_GetWinInfo(pAndroidFRCPropInfo->u8RSideGwinID,&info_R);
    switch (osdMode) {
        case DISPLAYMODE_TOPBOTTOM_LA:
        case DISPLAYMODE_NORMAL_LA:
        case DISPLAYMODE_TOP_LA:
        case DISPLAYMODE_BOTTOM_LA:
            if (pAndroidFRCPropInfo->eGOPDst == E_GOP_DST_BYPASS) {
                gfx_GOPStretch(pAndroidFRCPropInfo->u8LSideGOPNum, (srcWidth>>1), (srcHeight>>1), (panelWidth>>1), (panelHeight>>1));
                gfx_GOPStretch(pAndroidFRCPropInfo->u8RSideGOPNum, (srcWidth>>1), (srcHeight>>1), (panelWidth>>1), (panelHeight>>1));
            } else if (pAndroidFRCPropInfo->eGOPDst == E_GOP_DST_FRC) {
                gfx_GOPStretch(pAndroidFRCPropInfo->u8LSideGOPNum, srcWidth, (srcHeight>>1), cur_OCTiming.width, (cur_OCTiming.height>>1));
            } else {
                gfx_GOPStretch(pAndroidFRCPropInfo->u8LSideGOPNum, srcWidth, (srcHeight>>1), cur_OPTiming.width, (cur_OPTiming.height>>1));
            }
            info_L.u16DispHPixelEnd = (srcWidth>>1);
            info_R.u16DispHPixelEnd = (srcWidth>>1);
            break;

        case DISPLAYMODE_LEFTRIGHT_FR:
        case DISPLAYMODE_NORMAL_FR:
            if (pAndroidFRCPropInfo->eGOPDst == E_GOP_DST_BYPASS) {
                gfx_GOPStretch(pAndroidFRCPropInfo->u8LSideGOPNum, (srcWidth>>2), srcHeight, panelWidth>>1, panelHeight);
                gfx_GOPStretch(pAndroidFRCPropInfo->u8RSideGOPNum, (srcWidth>>2), srcHeight, panelWidth>>1, panelHeight);
            } else if (pAndroidFRCPropInfo->eGOPDst == E_GOP_DST_FRC) {
                gfx_GOPStretch(pAndroidFRCPropInfo->u8LSideGOPNum, (srcWidth>>1), srcHeight, cur_OCTiming.width, cur_OCTiming.height);
            } else {
                gfx_GOPStretch(pAndroidFRCPropInfo->u8LSideGOPNum, (srcWidth>>1), srcHeight, cur_OPTiming.width, cur_OPTiming.height);
            }
            info_L.u16DispHPixelEnd = (srcWidth>>2);
            info_R.u16DispHPixelEnd = (srcWidth>>2);
            break;

        case DISPLAYMODE_NORMAL_FP:
            gfx_GOPStretch(pAndroidFRCPropInfo->u8LSideGOPNum, srcWidth, srcHeight, srcWidth, srcHeight);
            break;

        case DISPLAYMODE_NORMAL:
        case DISPLAYMODE_LEFTRIGHT:
        case DISPLAYMODE_TOPBOTTOM:
        case DISPLAYMODE_TOP_ONLY:
        case DISPLAYMODE_BOTTOM_ONLY:
        case DISPLAYMODE_RIGHT_ONLY:
        default:
            if (pAndroidFRCPropInfo->eGOPDst == E_GOP_DST_BYPASS) {
                gfx_GOPStretch(pAndroidFRCPropInfo->u8LSideGOPNum, (srcWidth>>1), srcHeight, panelWidth>>1, panelHeight);
                gfx_GOPStretch(pAndroidFRCPropInfo->u8RSideGOPNum, (srcWidth>>1), srcHeight, panelWidth>>1, panelHeight);
            } else if (pAndroidFRCPropInfo->eGOPDst == E_GOP_DST_FRC) {
                gfx_GOPStretch(pAndroidFRCPropInfo->u8LSideGOPNum, srcWidth, srcHeight, cur_OCTiming.width, cur_OCTiming.height);
            } else {
                gfx_GOPStretch(pAndroidFRCPropInfo->u8LSideGOPNum, srcWidth, srcHeight, cur_OPTiming.width, cur_OPTiming.height);
            }

            info_L.u16DispHPixelEnd = (srcWidth>>1);
            info_R.u16DispHPixelEnd = (srcWidth>>1);
            break;
    }

    if (pAndroidFRCPropInfo->eGOPDst == E_GOP_DST_BYPASS) {
        if (mirrorModeH && mirrorModeV) {
            u32FlipAddr_A = info_L.u32DRAMRBlkStart +u32CropUiAddrOffset+(srcWidth*2);
            u32FlipAddr_B = info_R.u32DRAMRBlkStart +u32CropUiAddrOffset ;
        } else {
            u32FlipAddr_A = info_L.u32DRAMRBlkStart +u32CropUiAddrOffset;
            u32FlipAddr_B = info_R.u32DRAMRBlkStart +u32CropUiAddrOffset+(srcWidth*2);
        }
        info_L.u16RBlkHPixSize = (info_L.u16DispHPixelEnd - info_L.u16DispHPixelStart);
        info_L.u32DRAMRBlkStart = u32FlipAddr_A;
        MApi_GOP_GWIN_SetWinInfo(pAndroidFRCPropInfo->u8LSideGwinID,&info_L);

        info_R.u16RBlkHPixSize = (info_R.u16DispHPixelEnd - info_R.u16DispHPixelStart);
        info_R.u32DRAMRBlkStart = u32FlipAddr_B;
        MApi_GOP_GWIN_SetWinInfo(pAndroidFRCPropInfo->u8RSideGwinID,&info_R);
    } else if (pAndroidFRCPropInfo->eGOPDst == E_GOP_DST_OP0) {
        MApi_GOP_GWIN_MapFB2Win(pAndroidFRCPropInfo->u8LSideFBID, pAndroidFRCPropInfo->u8LSideGwinID);
    }
}


int fbdev_setOSDContentRegion(int x,int y,int width,int height) {
    req_OSDRegion.x = x;
    req_OSDRegion.y = y;
    req_OSDRegion.width = width;
    req_OSDRegion.height = height;
    return 1;
}

int fbdev_getCurOPTiming(int *ret_width, int *ret_height) {
    getCurOPTiming(ret_width,  ret_height);
    return 1;
}

int fbdev_getUrsaVsersion() {
    return ursaVsersion;
}

int fbdev_getCurrentOSDWidth() {
    return cur_OSDRegion.width;
}

int fbdev_getCurrentOSDHeight() {
    return cur_OSDRegion.height;
}

int fbdev_getCurrentOPTimingWidth() {
    int ret_width = panelWidth;
    int ret_height = panelHeight;
    getCurOPTiming(&ret_width,  &ret_height);
    return ret_width;
}

int fbdev_getCurrentOPTimingHeight() {
    int ret_width = panelWidth;
    int ret_height = panelHeight;
    getCurOPTiming(&ret_width,  &ret_height);
    return ret_height;
}
int fbdev_getCurrentOCTimingWidth() {
    int ret_width = panelWidth;
    int ret_height = panelHeight;
    getCurOCTiming(&ret_width,  &ret_height);
    return ret_width;
}
int fbdev_getCurrentOCTimingHeight() {
    int ret_width = panelWidth;
    int ret_height = panelHeight;
    getCurOCTiming(&ret_width,  &ret_height);
    return ret_height;
}
