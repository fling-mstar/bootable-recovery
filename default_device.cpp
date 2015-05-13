/*
 * Copyright (C) 2009 The Android Open Source Project
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

#include <linux/input.h>

#include "common.h"
#include "device.h"
#include "screen_ui.h"

// MStar Android Patch Begin
static const char* HEADERS[] = {
                                 "Volume up/down to move highlight;",
                                 "enter button to select.",
                                 "",
                                 NULL };

static const char* ITEMS[] =  {
                               "reboot system now",
                               "apply update from ADB",
                               "wipe data/factory reset",
                               "wipe cache partition",
                               "apply update from cache",
                               "apply update from external storage",
                               "system backup",
                               "system restore",
                               NULL };
// MStar Android Patch End

class DefaultUI : public ScreenRecoveryUI {
  public:
    virtual KeyAction CheckKey(int key) {
        if (key == KEY_HOME) {
            return TOGGLE;
        }
        return ENQUEUE;
    }
};

class DefaultDevice : public Device {
  public:
    DefaultDevice() :
        ui(new DefaultUI) {
    }

    RecoveryUI* GetUI() { return ui; }
    // MStar Android Patch Begin
    int HandleMenuKey(int key, int visible) {
        static int num = -1;
        static char key_group[256];
        char key_number = 'f';
        int ret = kNoAction;
        if (-1 == num) {
            for (int w = 0; w < 256; w++) {
                key_group[w] = 'f';
            }
        }
        num++;
        if (visible) {
            switch (key) {
              case KEY_DOWN:
              case KEY_VOLUMEDOWN:
                ret = kHighlightDown;
                break;

              case KEY_UP:
              case KEY_VOLUMEUP:
                ret = kHighlightUp;
                break;

              case KEY_ENTER:
                ret = kInvokeItem;
                break;

              case KEY_0:
                key_number = '0';
                break;

              case KEY_2:
                key_number = '2';
                break;

              case KEY_5:
                key_number = '5';
                break;

              case KEY_8:
                key_number = '8';
                break;

            }
        }

        key_group[num] = key_number;
        if (NULL != strstr(key_group,"2580")) {
            return kReboot;
        }

        if(num == 255) {
            static char tmp_char[3] = "0";
            tmp_char[0] = key_group[253];
            tmp_char[1] = key_group[254];
            tmp_char[2] = key_group[255];
            memset(key_group,0,256);
            strcpy(key_group,tmp_char);
            memset(tmp_char,0,3);
            num = 2;
        }

        return ret;
    }
    // MStar Android Patch End
    BuiltinAction InvokeMenuItem(int menu_position) {
        switch (menu_position) {
          case 0: return REBOOT;
          case 1: return APPLY_ADB_SIDELOAD;
          case 2: return WIPE_DATA;
          case 3: return WIPE_CACHE;
          // MStar Android Patch Begin
          case 4: return APPLY_CACHE;
          case 5: return APPLY_EXT;
          case 6: return SYSTEM_BACKUP;
          case 7: return SYSTEM_RESTORE;
          // MStar Android Patch End
          default: return NO_ACTION;
        }
    }

    const char* const* GetMenuHeaders() { return HEADERS; }
    const char* const* GetMenuItems() { return ITEMS; }

  private:
    RecoveryUI* ui;
};

Device* make_device() {
    return new DefaultDevice();
}