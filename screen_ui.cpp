/*
 * Copyright (C) 2011 The Android Open Source Project
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
#include <fcntl.h>
#include <linux/input.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "device.h"
#include "minui/minui.h"
#include "screen_ui.h"
#include "ui.h"

// MStar Android Patch Begin
#define CHAR_WIDTH 10
#define CHAR_HEIGHT 18
// MStar Android Patch End
// There's only (at most) one of these objects, and global callbacks
// (for pthread_create, and the input event system) need to find it,
// so use a global variable.
static ScreenRecoveryUI* self = NULL;

// Return the current time as a double (including fractions of a second).
static double now() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

ScreenRecoveryUI::ScreenRecoveryUI() :
    currentIcon(NONE),
    // MStar Android Patch Begin
    currentTip(TIP_TITLE_NONE),
    // max seconds to wait
    exitRecoveryTipTitleNum(15),
    currentExitRecoveryTip(-1),
    // MStar Android Patch End
    installingFrame(0),
    rtl_locale(false),
    progressBarType(EMPTY),
    progressScopeStart(0),
    progressScopeSize(0),
    progress(0),
    pagesIdentical(false),
    text_cols(0),
    text_rows(0),
    text_col(0),
    text_row(0),
    text_top(0),
    show_text(false),
    show_text_ever(false),
    show_menu(false),
    menu_top(0),
    menu_items(0),
    menu_sel(0),

    // These values are correct for the default image resources
    // provided with the android platform.  Devices which use
    // different resources should have a subclass of ScreenRecoveryUI
    // that overrides Init() to set these values appropriately and
    // then call the superclass Init().
    // MStar Android Patch Begin
    animation_fps(20),
    indeterminate_frames(6),
    installing_frames(7),
    install_overlay_offset_x(13),
    install_overlay_offset_y(190+15),
    overlay_offset_x(-1),
    overlay_offset_y(-1),
    stage(-1),
    max_stage(-1) {

    pthread_mutex_init(&updateMutex, NULL);
    self = this;
    // MStar Android Patch End
}

// Draw the given frame over the installation overlay animation.  The
// background is not cleared or draw with the base icon first; we
// assume that the frame already contains some other frame of the
// animation.  Does nothing if no overlay animation is defined.
// Should only be called with updateMutex locked.
void ScreenRecoveryUI::draw_install_overlay_locked(int frame) {
    // MStar Android Patch Begin
    if (installationOverlay == NULL) return;
    gr_surface surface = installationOverlay[frame];
    int iconWidth = gr_get_width(surface);
    int iconHeight = gr_get_height(surface);
    gr_blit(surface, 0, 0, iconWidth, iconHeight,
            install_overlay_offset_x, install_overlay_offset_y);
    // MStar Android Patch End
}

// Clear the screen and draw the currently selected background icon (if any).
// Should only be called with updateMutex locked.
void ScreenRecoveryUI::draw_background_locked(Icon icon)
{
    // MStar Android Patch Begin
    pagesIdentical = false;
    gr_color(0, 0, 0, 254);
    gr_fill(0, 0, gr_fb_width(), gr_fb_height());

    if (icon) {
        gr_surface surface = backgroundIcon[icon];

        int iconWidth = gr_get_width(surface);
        int iconHeight = gr_get_height(surface);

        int iconX = (gr_fb_width() - iconWidth) / 2;
        int iconY = (gr_fb_height() - iconHeight) / 2 + 15;

        gr_blit(surface, 0, 0, iconWidth, iconHeight, iconX, iconY);
        if (icon == INSTALLING) {
            draw_install_overlay_locked(installingFrame);
        }
    }
    // MStar Android Patch End
}

// MStar Android Patch Begin
// Draw the title bitmaps on the top of panel.
// Should only be called with gUpdateMutex locked.
void ScreenRecoveryUI::draw_top_title_locked()
{
    gr_surface titleASurface = titleNotPowerOff;
    int titleAWidth = gr_get_width(titleASurface);
    int titleAHeight = gr_get_height(titleASurface);
    int titleAX = (gr_fb_width() - titleAWidth) / 2;
    int titleAY =  20;
    gr_blit(titleASurface, 0, 0, titleAWidth, titleAHeight, titleAX, titleAY);

    gr_surface titleBSurface = titleNotUnplug;
    int titleBWidth = gr_get_width(titleBSurface);
    int titleBHeight = gr_get_height(titleBSurface);
    int titleBX = (gr_fb_width() - titleBWidth) / 2;
    int titleBY = titleAY + titleAHeight;
    gr_blit(titleBSurface, 0, 0, titleBWidth, titleBHeight, titleBX, titleBY);
}

// Draw the title bitmaps to show the tip message.
// Should only be called with gUpdateMutex locked.
void ScreenRecoveryUI::draw_tip_title_locked(Tip tip)
{
    if (tip) {
        gr_surface tipSurface = tipTitle[tip];
        int tipWidth = gr_get_width(tipSurface);
        int tipHeight = gr_get_height(tipSurface);
        int tipX = (gr_fb_width() - tipWidth) / 2;
        int tipY =  gr_fb_height() - tipHeight - 50;
        gr_blit(tipSurface, 0, 0, tipWidth, tipHeight, tipX, tipY);
    }
}

void ScreenRecoveryUI::draw_exit_recovery_tip_title_locked(int exitRecoveryTip)
{
    if ((exitRecoveryTip >= 0)&&(exitRecoveryTip<=14)) {
        gr_surface exitRecoveryTipSurface = exitRecoveryTipTitle[exitRecoveryTip];
        int tipWidth = gr_get_width(exitRecoveryTipSurface);
        int tipHeight = gr_get_height(exitRecoveryTipSurface);
        int tipX = (gr_fb_width() - tipWidth) / 2;
        int tipY = (gr_fb_height() - tipHeight)/2;
        gr_blit(exitRecoveryTipSurface, 0, 0, tipWidth, tipHeight, tipX, tipY);
    }
}

// MStar Android Patch End

// Draw the progress bar (if any) on the screen.  Does not flip pages.
// Should only be called with updateMutex locked.
void ScreenRecoveryUI::draw_progress_locked()
{
    // MStar Android Patch Begin
    if (currentIcon == ERROR) return;

    if (currentIcon == INSTALLING) {
        draw_install_overlay_locked(installingFrame);
    }

    if (progressBarType != EMPTY) {
        int iconHeight = gr_get_height(backgroundIcon[INSTALLING]);
        int width = gr_get_width(progressBarEmpty);
        int height = gr_get_height(progressBarEmpty);

        int dx = (gr_fb_width() - width)/2;
        int dy = (3*gr_fb_height() + iconHeight - 2*height)/4 - 50;

        // Erase behind the progress bar (in case this was a progress-only update)
        gr_color(0, 0, 0, 254);
        gr_fill(dx, dy, width, height);

        if (progressBarType == DETERMINATE) {
            float p = progressScopeStart + progress * progressScopeSize;
            int pos = (int) (p * width);

            if (pos > 0) {
                gr_blit(progressBarFill, 0, 0, pos, height, dx, dy);
            }
            if (pos < width-1) {
                gr_blit(progressBarEmpty, pos, 0, width-pos, height, dx+pos, dy);
            }
        }

        if (progressBarType == INDETERMINATE) {
            static int frame = 0;
            gr_blit(progressBarIndeterminate[frame], 0, 0, width, height, dx, dy);
            // in RTL locales, we run the animation backwards, which
            // makes the spinner spin the other way.
            frame = (frame + 1) % indeterminate_frames;
        }
    }
    // MStar Android Patch End
}

void ScreenRecoveryUI::SetColor(UIElement e) {
    switch (e) {
        case HEADER:
            gr_color(247, 0, 6, 255);
            break;
        case MENU:
        case MENU_SEL_BG:
            gr_color(0, 106, 157, 255);
            break;
        case MENU_SEL_FG:
            gr_color(255, 255, 255, 255);
            break;
        case LOG:
            gr_color(249, 194, 0, 255);
            break;
        case TEXT_FILL:
            gr_color(0, 0, 0, 160);
            break;
        default:
            gr_color(255, 255, 255, 255);
            break;
    }
}

// MStar Android Patch Begin
void ScreenRecoveryUI::draw_text_line(int row, const char* t) {
  if (t[0] != '\0') {
    gr_text(0, (row+1)*CHAR_HEIGHT-1, t);
  }
}
// MStar Android Patch End

// Redraw everything on the screen.  Does not flip pages.
// Should only be called with updateMutex locked.
void ScreenRecoveryUI::draw_screen_locked()
{
    // MStar Android Patch Begin
    draw_background_locked(currentIcon);
    draw_progress_locked();
    draw_top_title_locked();
    draw_tip_title_locked(currentTip);
    draw_exit_recovery_tip_title_locked(currentExitRecoveryTip);
    if (show_text) {
        gr_color(0, 0, 0, 0);
        gr_fill(0, 0, gr_fb_width(), gr_fb_height());

        int i = 0;
        if (show_menu) {
            gr_color(64, 96, 255, 254);
            gr_fill(0, (menu_top+menu_sel) * CHAR_HEIGHT,
                    gr_fb_width(), (menu_top+menu_sel+1)*CHAR_HEIGHT+1);

            for (; i < menu_top + menu_items; ++i) {
                if (i == menu_top + menu_sel) {
                    gr_color(255, 255, 255, 254);
                    draw_text_line(i, menu[i]);
                    gr_color(64, 96, 255, 254);
                } else {
                    draw_text_line(i, menu[i]);
                }
            }
            gr_fill(0, i*CHAR_HEIGHT+CHAR_HEIGHT/2-1,
                    gr_fb_width(), i*CHAR_HEIGHT+CHAR_HEIGHT/2+1);
            ++i;
        }

        gr_color(64, 96, 255, 254);

        for (; i < text_rows; ++i) {
            draw_text_line(i, text[(i+text_top) % text_rows]);
        }
    }
    // MStar Android Patch End
}

// Redraw everything on the screen and flip the screen (make it visible).
// Should only be called with updateMutex locked.
void ScreenRecoveryUI::update_screen_locked()
{
    draw_screen_locked();
    gr_flip();
}

// Updates only the progress bar, if possible, otherwise redraws the screen.
// Should only be called with updateMutex locked.
void ScreenRecoveryUI::update_progress_locked()
{
    if (show_text || !pagesIdentical) {
        draw_screen_locked();    // Must redraw the whole screen
        pagesIdentical = true;
    } else {
        draw_progress_locked();  // Draw only the progress bar and overlays
    }
    gr_flip();
}

// Keeps the progress bar updated, even when the process is otherwise busy.
void* ScreenRecoveryUI::progress_thread(void *cookie) {
    self->progress_loop();
    return NULL;
}

void ScreenRecoveryUI::progress_loop() {
    double interval = 1.0 / animation_fps;
    for (;;) {
        double start = now();
        pthread_mutex_lock(&updateMutex);

        int redraw = 0;

        // update the installation animation, if active
        // skip this if we have a text overlay (too expensive to update)
        // MStar Android Patch Begin
        if (currentIcon == INSTALLING && installing_frames > 0 && !show_text) {
        // MStar Android Patch End
            installingFrame = (installingFrame + 1) % installing_frames;
            redraw = 1;
        }

        // update the progress bar animation, if active
        // skip this if we have a text overlay (too expensive to update)
        if (progressBarType == INDETERMINATE && !show_text) {
            redraw = 1;
        }

        // move the progress bar forward on timed intervals, if configured
        int duration = progressScopeDuration;
        if (progressBarType == DETERMINATE && duration > 0) {
            double elapsed = now() - progressScopeTime;
            float p = 1.0 * elapsed / duration;
            if (p > 1.0) p = 1.0;
            if (p > progress) {
                progress = p;
                redraw = 1;
            }
        }

        if (redraw) update_progress_locked();

        pthread_mutex_unlock(&updateMutex);
        double end = now();
        // minimum of 20ms delay between frames
        double delay = interval - (end-start);
        if (delay < 0.02) delay = 0.02;
        usleep((long)(delay * 1000000));
    }
}

void ScreenRecoveryUI::LoadBitmap(const char* filename, gr_surface* surface) {
    int result = res_create_surface(filename, surface);
    if (result < 0) {
        LOGE("missing bitmap %s\n(Code %d)\n", filename, result);
    }
}

void ScreenRecoveryUI::LoadLocalizedBitmap(const char* filename, gr_surface* surface) {
    int result = res_create_localized_surface(filename, surface);
    if (result < 0) {
        LOGE("missing bitmap %s\n(Code %d)\n", filename, result);
    }
}

void ScreenRecoveryUI::Init()
{
    gr_init();
    // MStar Android Patch Begin
    text_col = text_row = 0;
    text_rows = gr_fb_height() / CHAR_HEIGHT;
    if (text_rows > kMaxRows) text_rows = kMaxRows;
    text_top = 1;

    text_cols = gr_fb_width() / CHAR_WIDTH;
    if (text_cols > kMaxCols - 1) text_cols = kMaxCols - 1;


    memset(text,0,sizeof(text));
    LoadBitmap("icon_installing", &backgroundIcon[INSTALLING]);
    LoadBitmap("icon_error", &backgroundIcon[ERROR]);

    LoadBitmap("progress_empty", &progressBarEmpty);
    LoadBitmap("progress_fill", &progressBarFill);
    LoadBitmap("stage_empty", &stageMarkerEmpty);
    LoadBitmap("stage_fill", &stageMarkerFill);

    //load title/tip bitmaps ,added by mstar semiconductor
    LoadBitmap("title_not_power_off",&titleNotPowerOff);
    LoadBitmap("title_not_unplug",&titleNotUnplug);
    LoadBitmap("tip_error", &tipTitle[TIP_TITLE_ERROR]);
    LoadBitmap("tip_ready", &tipTitle[TIP_TITLE_READY]);
    LoadBitmap("tip_success", &tipTitle[TIP_TITLE_SUCCESS]);
    LoadBitmap("tip_wipe_cache", &tipTitle[TIP_TITLE_WIPE_CACHE]);
    LoadBitmap("tip_wipe_data", &tipTitle[TIP_TITLE_WIPE_DATA]);
    LoadBitmap("tip_install_package", &tipTitle[TIP_TITLE_INSTALL_PACKAGE]);
    LoadBitmap("tip_backup_system", &tipTitle[TIP_TITLE_BACKUP_SYSTEM]);
    LoadBitmap("tip_restore_system", &tipTitle[TIP_TITLE_RESTORE_SYSTEM]);
    LoadBitmap("tip_blank_screen", &tipTitle[TIP_TITLE_BLANK_SCREEN]);

    // init exit recovery tip picture
    int j=0;
    char tipFileNmae[40];
    exitRecoveryTipTitle = (gr_surface*)malloc(exitRecoveryTipTitleNum *
                                                   sizeof(gr_surface));
    for (j = 0;j < exitRecoveryTipTitleNum;++j) {
        // "tip_exit_recovery_1.png", "tip_exit_recovery_2.png", ...
        sprintf(tipFileNmae, "tip_exit_recovery_%d", j+1);
        LoadBitmap(tipFileNmae, exitRecoveryTipTitle+j);
    }
    // MStar Android Patch End

    int i;

    progressBarIndeterminate = (gr_surface*)malloc(indeterminate_frames *
                                                    sizeof(gr_surface));
    for (i = 0; i < indeterminate_frames; ++i) {
        char filename[40];
        // "indeterminate01.png", "indeterminate02.png", ...
        sprintf(filename, "indeterminate%02d", i+1);
        LoadBitmap(filename, progressBarIndeterminate+i);
    }

    if (installing_frames > 0) {
        installationOverlay = (gr_surface*)malloc(installing_frames *
                                                   sizeof(gr_surface));
        for (i = 0; i < installing_frames; ++i) {
            char filename[40];
            // "icon_installing_overlay01.png",
            // "icon_installing_overlay02.png", ...
            sprintf(filename, "icon_installing_overlay%02d", i+1);
            LoadBitmap(filename, installationOverlay+i);
        }
        // MStar Android Patch Begin
        // Adjust the offset to account for the positioning of the
        // base image on the screen.
        if (backgroundIcon[INSTALLING] != NULL) {
            gr_surface bg = backgroundIcon[INSTALLING];
            install_overlay_offset_x += (gr_fb_width() - gr_get_width(bg)) / 2;
            install_overlay_offset_y += (gr_fb_height() - gr_get_height(bg)) / 2;
        }
        // MStar Android Patch End
    } else {
        installationOverlay = NULL;
    }

    pthread_create(&progress_t, NULL, progress_thread, NULL);

    RecoveryUI::Init();
}

void ScreenRecoveryUI::SetLocale(const char* locale) {
    if (locale) {
        char* lang = strdup(locale);
        for (char* p = lang; *p; ++p) {
            if (*p == '_') {
                *p = '\0';
                break;
            }
        }

        // A bit cheesy: keep an explicit list of supported languages
        // that are RTL.
        if (strcmp(lang, "ar") == 0 ||   // Arabic
            strcmp(lang, "fa") == 0 ||   // Persian (Farsi)
            strcmp(lang, "he") == 0 ||   // Hebrew (new language code)
            strcmp(lang, "iw") == 0 ||   // Hebrew (old language code)
            strcmp(lang, "ur") == 0) {   // Urdu
            rtl_locale = true;
        }
        free(lang);
    }
}

void ScreenRecoveryUI::SetBackground(Icon icon)
{
    // MStar Android Patch Begin
    pthread_mutex_lock(&updateMutex);

    currentIcon = icon;
    update_screen_locked();

    pthread_mutex_unlock(&updateMutex);
    // MStar Android Patch End
}

// MStar Android Patch Begin
void ScreenRecoveryUI::SetTipTitle(Tip tip)
{
    pthread_mutex_lock(&updateMutex);
    currentTip = tip;
    update_screen_locked();
    pthread_mutex_unlock(&updateMutex);
}

void ScreenRecoveryUI::SetExitRecoveryTipTitle(int exitRecoveryTip)
{
    pthread_mutex_lock(&updateMutex);
    currentExitRecoveryTip = exitRecoveryTip;
    update_screen_locked();
    pthread_mutex_unlock(&updateMutex);
}

// return max wait times before starting to recovery system
int ScreenRecoveryUI::GetMaxWaitTime()
{
    return exitRecoveryTipTitleNum;
}

// MStar Android Patch End

void ScreenRecoveryUI::SetProgressType(ProgressType type)
{
    pthread_mutex_lock(&updateMutex);
    if (progressBarType != type) {
        progressBarType = type;
    }
    progressScopeStart = 0;
    progressScopeSize = 0;
    progress = 0;
    update_progress_locked();
    pthread_mutex_unlock(&updateMutex);
}

void ScreenRecoveryUI::ShowProgress(float portion, float seconds)
{
    pthread_mutex_lock(&updateMutex);
    progressBarType = DETERMINATE;
    progressScopeStart += progressScopeSize;
    progressScopeSize = portion;
    progressScopeTime = now();
    progressScopeDuration = seconds;
    progress = 0;
    update_progress_locked();
    pthread_mutex_unlock(&updateMutex);
}

void ScreenRecoveryUI::SetProgress(float fraction)
{
    pthread_mutex_lock(&updateMutex);
    if (fraction < 0.0) fraction = 0.0;
    if (fraction > 1.0) fraction = 1.0;
    if (progressBarType == DETERMINATE && fraction > progress) {
        // Skip updates that aren't visibly different.
        int width = gr_get_width(progressBarIndeterminate[0]);
        float scale = width * progressScopeSize;
        if ((int) (progress * scale) != (int) (fraction * scale)) {
            progress = fraction;
            update_progress_locked();
        }
    }
    pthread_mutex_unlock(&updateMutex);
}

void ScreenRecoveryUI::SetStage(int current, int max) {
    pthread_mutex_lock(&updateMutex);
    stage = current;
    max_stage = max;
    pthread_mutex_unlock(&updateMutex);
}

void ScreenRecoveryUI::Print(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, 256, fmt, ap);
    va_end(ap);

    fputs(buf, stdout);

    // This can get called before ui_init(), so be careful.
    pthread_mutex_lock(&updateMutex);
    if (text_rows > 0 && text_cols > 0) {
        char *ptr;
        for (ptr = buf; *ptr != '\0'; ++ptr) {
            if (*ptr == '\n' || text_col >= text_cols) {
                text[text_row][text_col] = '\0';
                text_col = 0;
                text_row = (text_row + 1) % text_rows;
                if (text_row == text_top) text_top = (text_top + 1) % text_rows;
            }
            if (*ptr != '\n') text[text_row][text_col++] = *ptr;
        }
        text[text_row][text_col] = '\0';
        update_screen_locked();
    }
    pthread_mutex_unlock(&updateMutex);
}

void ScreenRecoveryUI::StartMenu(const char* const * headers, const char* const * items,
                                 int initial_selection) {
    int i;
    pthread_mutex_lock(&updateMutex);
    // MStar Android Patch Begin
    memset(menu,0,sizeof(menu));
    // MStar Android Patch End
    if (text_rows > 0 && text_cols > 0) {
        for (i = 0; i < text_rows; ++i) {
            if (headers[i] == NULL) break;
            strncpy(menu[i], headers[i], text_cols-1);
            menu[i][text_cols-1] = '\0';
        }
        menu_top = i;
        for (; i < text_rows; ++i) {
            if (items[i-menu_top] == NULL) break;
            strncpy(menu[i], items[i-menu_top], text_cols-1);
            menu[i][text_cols-1] = '\0';
        }
        menu_items = i - menu_top;
        show_menu = 1;
        menu_sel = initial_selection;
        update_screen_locked();
    }
    pthread_mutex_unlock(&updateMutex);
}

int ScreenRecoveryUI::SelectMenu(int sel) {
    int old_sel;
    pthread_mutex_lock(&updateMutex);
    if (show_menu > 0) {
        old_sel = menu_sel;
        menu_sel = sel;
        if (menu_sel < 0) menu_sel = 0;
        if (menu_sel >= menu_items) menu_sel = menu_items-1;
        sel = menu_sel;
        if (menu_sel != old_sel) update_screen_locked();
    }
    pthread_mutex_unlock(&updateMutex);
    return sel;
}

void ScreenRecoveryUI::EndMenu() {
    int i;
    pthread_mutex_lock(&updateMutex);
    if (show_menu > 0 && text_rows > 0 && text_cols > 0) {
        show_menu = 0;
        update_screen_locked();
    }
    pthread_mutex_unlock(&updateMutex);
}

bool ScreenRecoveryUI::IsTextVisible()
{
    pthread_mutex_lock(&updateMutex);
    int visible = show_text;
    pthread_mutex_unlock(&updateMutex);
    return visible;
}

bool ScreenRecoveryUI::WasTextEverVisible()
{
    pthread_mutex_lock(&updateMutex);
    int ever_visible = show_text_ever;
    pthread_mutex_unlock(&updateMutex);
    return ever_visible;
}

void ScreenRecoveryUI::ShowText(bool visible)
{
    pthread_mutex_lock(&updateMutex);
    show_text = visible;
    if (show_text) show_text_ever = 1;
    update_screen_locked();
    pthread_mutex_unlock(&updateMutex);
}

void ScreenRecoveryUI::Redraw()
{
    pthread_mutex_lock(&updateMutex);
    update_screen_locked();
    pthread_mutex_unlock(&updateMutex);
}
