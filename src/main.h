#pragma once

#include <psp2/appmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/system_param.h>
#include <psp2/rtc.h>
#include <psp2/ime_dialog.h>
#include <psp2/shellutil.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/sysmodule.h>
#include <ftpvita.h>
#include <strings.h>

#include "utils.h"
#include "font.h"
#include "input.h"
#include "pfs.h"
#include "ime_dialog.h"

#define IS_OVERFLOW() ((select_row*ICONS_COL)+select_col >= selectable_count(*curr, ICONS_ROW, ICONS_COL))

vita2d_pgf* font;
vita2d_pvf* symbol_font;

int SCE_CTRL_ENTER;
int SCE_CTRL_CANCEL;
extern char ICON_ENTER[2];
extern char ICON_CANCEL[2];

char confirm_msg[64];
int confirm_msg_width;
char close_msg[64];
int close_msg_width;
char yesno_msg[64];
int yesno_msg_width;

char devices[5][6];
char cur_device[5];

int select_row = 0;
int select_col = 0;
int select_appinfo_button = 0;
int select_slot = 0;
int appinfo_btns = 4;
int refreshUI = 0;
int savelist_tab = 0;
int old_savelist_tab = 0;
int select_menu = 0;
int select_device = 0;
int device_num = 0;

typedef enum {
    UNKNOWN = 0,
    MAIN_SCREEN = 1,
    MENU_OPEN,
    SORT_MENU_OPEN,
    PRINT_APPINFO,
    BACKUP_MODE,
    BACKUP_CONFIRM,
    BACKUP_PROGRESS,
    BACKUP_FAIL,
    RESTORE_MODE,
    RESTORE_CONFIRM,
    RESTORE_PROGRESS,
    RESTORE_FAIL,
    DELETE_MODE,
    DELETE_CONFIRM,
    DELETE_PROGRESS,
    DELETE_FAIL,
    FORMAT_MODE,
    FORMAT_CONFIRM,
    FORMAT_PROGRESS,
    FORMAT_FAIL,
    REGION_SET_MODE,
    REGION_SET_CONFIRM,
    REGION_SET_PROGRESS,
    REGION_SET_FAIL,
    BACKUPALL_MODE,
    BACKUPALL_CONFIRM,
    BACKUPALL_PROGRESS,
    BACKUPALL_FAIL,
    BACKUPALL_ENC_MODE,
    BACKUPALL_ENC_CONFIRM,
    BACKUPALL_ENC_PROGRESS,
    BACKUPALL_ENC_FAIL,
    DELETE_ALL_SLOTS_MODE,
    DELETE_ALL_SLOTS_CONFIRM,
    DELETE_ALL_SLOTS_PROGRESS,
    DELETE_ALL_SLOTS_FAIL,
    SWITCH_CURR_DEVICE,
} ScreenState;

typedef enum {
    SVMGR_DEVICE = 0,
    BACKUP_ALL = 1,
    BACKUP_ALL_ENC,
    DELETE_ALL_SLOTS,
    ACCENT_COLOR,
    DARK_MODE
} MenuOption;

typedef enum {
    NO_ERROR = 0,
    ERROR_NO_SAVE_DIR = 1,
    ERROR_NO_SLOT_DIR,
    ERROR_MEMORY_ALLOC,
    ERROR_DECRYPT_DIR,
    ERROR_COPY_DIR,
    ERROR_DELETE_DIR,
    ERROR_NO_SFO,
    ERROR_NO_AID_MATCH,
    ERROR_INV_TITLEID,
} ProcessError;