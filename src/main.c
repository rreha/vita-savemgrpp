#include "main.h"

static char slot_cache[10][64];
static char savedir_cache[128];
static appinfo *cached_info_ptr = NULL;
int force_cache_refresh = 1;
int current_accent_idx = 0;
unsigned int current_accent_color = DEFAULT;
int is_dark_mode = 0;

static char *save_dir_path(const appinfo *info) {
    size_t len = sizeof(char) * 40;
    char *path = malloc(len);

    if (path == NULL) return NULL;

    snprintf(path, len, "grw0:savedata/%s", info->real_id);
    if (path_exists(path)) return path;

    snprintf(path, len, "ux0:user/00/savedata/%s", info->real_id);
    if (path_exists(path)) return path;

    free(path);

    return NULL;
}

static char *slot_dir_path(const appinfo *info, int slot) {
    size_t len = sizeof(char) * 60;
    char *path = malloc(len);

    if (path == NULL) return NULL;

    snprintf(path, len, "%s/%s/SLOT%d", savemgr_fpath, info->title_id, slot);

    return path;
}

static char *slot_sfo_path(const appinfo *info, int slot) {
    size_t len = sizeof(char) * 80;
    char *path = malloc(len);

    if (path == NULL) return NULL;

    snprintf(path, len, "%s/%s/SLOT%d/sce_sys/param.sfo", savemgr_fpath, info->title_id, slot);
    if (path_exists(path)) return path;

    free(path);

    return NULL;
}

static char *save_dir_path_enc(const appinfo *info) {
    size_t len = sizeof(char) * 40;
    char *path = malloc(len);

    if (path == NULL) return NULL;

    snprintf(path, len, "gro0:app/%s", info->real_id);
    if (path_exists(path)) {
        snprintf(path, len, "grw0:savedata/%s", info->real_id);
        return path;
    }

    snprintf(path, len, "ux0:app/%s", info->real_id);
    if (path_exists(path)) {
        snprintf(path, len, "ux0:user/00/savedata/%s", info->real_id);
        return path;
    }

    free(path);

    return NULL;
}

static int is_save_decrypted(const appinfo *info, int slot) {
    char path[70] = {0};
    snprintf(path, sizeof(path), "%s/%s/SLOT%d/sce_pfs", savemgr_fpath, info->title_id, slot);
    if (path_exists(path)) return 0;
    return 1;
}

static void save_config() {
    char path[128];
    snprintf(path, sizeof(path), "%s%s/settings.cfg", devices[0], SAVEMGR_FOLDER);
    SceUID fd = sceIoOpen(path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fd >= 0) {
        sceIoWrite(fd, &current_accent_idx, sizeof(int));
        sceIoWrite(fd, &select_device, sizeof(int));
        sceIoWrite(fd, &is_dark_mode, sizeof(int));
        sceIoClose(fd);
    }
}

static void load_config() {
    char path[128];
    snprintf(path, sizeof(path), "%s%s/settings.cfg", devices[0], SAVEMGR_FOLDER);
    SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0);
    if (fd >= 0) {
        int saved_accent = 0;
        int saved_device = 0;
        sceIoRead(fd, &saved_accent, sizeof(int));
        sceIoRead(fd, &saved_device, sizeof(int));
        sceIoRead(fd, &is_dark_mode, sizeof(int));
        sceIoClose(fd);

        if (saved_accent >= 0 && saved_accent < num_accent_colors) {
            current_accent_idx = saved_accent;
            current_accent_color = accent_colors[current_accent_idx];
        }
        
        if (saved_device >= 0 && saved_device < device_num) {
            select_device = saved_device;
            snprintf(cur_device, 5, "%s", devices[select_device]);
            snprintf(savemgr_fpath, 26, "%s%s", devices[select_device], SAVEMGR_FOLDER);
        }
    }
}

static void draw_tabs() {
    int text_center = vita2d_pgf_text_width(font, 1.1, "GAMES") / 2;
    int text_height = vita2d_pgf_text_height(font, 1.1, "GAMES");
    int screen_center = SCREEN_HALF_WIDTH;
    int screen_half_center = screen_center/2;

    vita2d_draw_rectangle(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, current_accent_color);

    vita2d_pgf_draw_text(font, screen_half_center - text_center,
                            8 + text_height, (!savelist_tab ? WHITE : TRANSPARENT), 1.1, "GAMES");

    vita2d_pgf_draw_text(font, (screen_center + screen_half_center) - text_center,
                            8 + text_height, (savelist_tab ? WHITE : TRANSPARENT), 1.1, "SAVES");

    int indicator_y = HEADER_HEIGHT - 4;
    if (!savelist_tab) {
        vita2d_draw_rectangle(0, indicator_y, screen_center, 4, WHITE);
    } else {
        vita2d_draw_rectangle(screen_center, indicator_y, screen_center, 4, WHITE);
    }
}

static void draw_menu() {
    int fontH = vita2d_pgf_text_height(font, 1.2, "T") + 10;
    int menuW = SCREEN_HALF_WIDTH + 60;
    int menuH = (fontH * MENU_ITEMS) + 50;
    int menuX = (SCREEN_WIDTH - menuW) / 2;
    int menuY = (SCREEN_HEIGHT - menuH) / 2;
    int textX = menuX + 15; 
    int rightX = menuX + menuW - 15; 
    int textY = menuY + fontH + 5;
    
    char val_str[64] = {0};
    unsigned int color;
    color = (select_menu == SVMGR_DEVICE ? THEME_TEXT : THEME_TEXT_MUTED);

    vita2d_draw_rectangle(menuX-5, menuY-5, menuW+10, menuH+10, current_accent_color);
    vita2d_draw_rectangle(menuX, menuY, menuW, menuH, is_dark_mode ? COLOR_DARK_BG : COLOR_LIGHT_BG);
    vita2d_pgf_draw_text(font, textX, textY, color, 1.2, "Backup Device");
    snprintf(val_str, sizeof(val_str), "< %s >", cur_device);
    int val_w = vita2d_pgf_text_width(font, 1.2, val_str);
    vita2d_pgf_draw_text(font, rightX - val_w, textY, color, 1.2, val_str);
    textY += 20;
    vita2d_draw_line(textX, textY, rightX, textY, THEME_TEXT_MUTED);
    textY += fontH + 5;
    color = (select_menu == BACKUP_ALL ? THEME_TEXT : THEME_TEXT_MUTED);
    vita2d_pgf_draw_text(font, textX, textY, color, 1.2, "Backup All Saves (Decrypted)");
    textY += fontH;
    color = (select_menu == BACKUP_ALL_ENC ? THEME_TEXT : THEME_TEXT_MUTED);
    vita2d_pgf_draw_text(font, textX, textY, color, 1.2, "Backup All Saves (Encrypted)");
    textY += fontH;
    color = (select_menu == DELETE_ALL_SLOTS ? THEME_TEXT : THEME_TEXT_MUTED);
    vita2d_pgf_draw_text(font, textX, textY, color, 1.2, "Delete All Saves Slots");
    textY += fontH;
    color = (select_menu == ACCENT_COLOR ? THEME_TEXT : THEME_TEXT_MUTED);
    vita2d_pgf_draw_text(font, textX, textY, color, 1.2, "Accent Color");
    snprintf(val_str, sizeof(val_str), "< %s >", accent_color_names[current_accent_idx]);
    val_w = vita2d_pgf_text_width(font, 1.2, val_str);
    vita2d_pgf_draw_text(font, rightX - val_w, textY, color, 1.2, val_str);
    textY += fontH;
    color = (select_menu == DARK_MODE ? THEME_TEXT : THEME_TEXT_MUTED);
    vita2d_pgf_draw_text(font, textX, textY, color, 1.2, "Dark Mode");
    snprintf(val_str, sizeof(val_str), "< %s >", is_dark_mode ? "On" : "Off");
    val_w = vita2d_pgf_text_width(font, 1.2, val_str);
    vita2d_pgf_draw_text(font, rightX - val_w, textY, color, 1.2, val_str);
}

static void draw_icon(icon_data *icon, int row, int col) {
    float w = vita2d_texture_get_width(icon->texture);
    float h = vita2d_texture_get_height(icon->texture);
    float z0 = ICON_WIDTH / w;
    float z1 = ICON_HEIGHT / h;
    float zoom = z0 < z1 ? z0 : z1;
    float iconLeft = ICON_LEFT(col);
    float iconTop = ICON_TOP(row);

    if (row == select_row && col == select_col) {
        zoom *= 1.15f; 
    }

    vita2d_draw_texture_scale_rotate_hotspot(icon->texture,
        iconLeft + (ICON_WIDTH / 2),
        iconTop + (ICON_HEIGHT / 2),
        zoom, zoom,
        0,
        w / 2,
        h / 2
    );
}

static void draw_icons(appinfo *curr, int itemsC) {
    vita2d_draw_rectangle(ITEMS_PANEL_LEFT, ITEMS_PANEL_TOP, ITEMS_PANEL_WIDTH,
                          ITEMS_PANEL_HEIGHT, THEME_BG);

    if (!itemsC) return;

    static float cursor_x = -1;
    static float cursor_y = -1;
    float target_x = ICON_LEFT(select_col) + (ICON_WIDTH / 2.0f);
    float target_y = ICON_TOP(select_row) + (ICON_HEIGHT / 2.0f);

    if (cursor_x == -1) {
        cursor_x = target_x;
        cursor_y = target_y;
    } else {
        cursor_x = lerp(cursor_x, target_x, 0.3f);
        cursor_y = lerp(cursor_y, target_y, 0.3f);
    }

    int border_radius = (ICON_WIDTH / 2) + 6; 
    vita2d_draw_fill_circle(cursor_x, cursor_y, border_radius, current_accent_color);
    for (int i = 0; curr && i < (ICONS_COL * ICONS_ROW); ++i, curr = curr->next) {
        load_icon(curr);
        draw_icon(&curr->icon, i / ICONS_COL, i % ICONS_COL);
    }
}

static void draw_button(int left, int top, int width, int height, const char *text,
                 float zoom, int pressed, int hover) {
    int text_width = vita2d_pgf_text_width(font, zoom, text);
    int text_height = vita2d_pgf_text_height(font, zoom, text);
    int text_left_margin = (width - text_width) / 2;
    int text_top_margin = (height - text_height) / 2;
    
    unsigned int bg_color;
    unsigned int text_color;
    int push_offset = pressed ? 2 : 0;

    if (hover || pressed) {
        bg_color = current_accent_color;
        text_color = WHITE;
    } else {
        bg_color = THEME_BTN_BG; 
        text_color = THEME_BTN_TEXT;
    }

    if (!pressed) {
        vita2d_draw_rectangle(left, top + 2, width, height, RGBA8(0, 0, 0, 40));
    }
    vita2d_draw_rectangle(left, top + push_offset, width, height, bg_color);
    vita2d_pgf_draw_text(font,
                         left + text_left_margin,
                         top + text_top_margin + text_height + push_offset,
                         text_color, zoom, text);
}

static void refresh_info_cache(appinfo *info) {
    if (info == cached_info_ptr && !force_cache_refresh)
        return;

    for (int i=0; i<SLOT_BUTTON; ++i) {
        char *fn = slot_sfo_path(info, i);
        if (fn && path_exists(fn)) {
            SceIoStat stat = {0};
            sceIoGetstat(fn, &stat);
            SceRtcTick tick_utc, tick_local;
            SceDateTime time;
            sceRtcGetTick(&stat.st_mtime, &tick_utc);
            sceRtcConvertUtcToLocalTime(&tick_utc, &tick_local);
            sceRtcSetTick(&time, &tick_local);

            int isDec = is_save_decrypted(info, i);
            snprintf(slot_cache[i], 64, "%04d-%02d-%02d %02d:%02d:%02d [%s]",
                     time.year, time.month, time.day, time.hour, time.minute, time.second,
                     (isDec == -1 ? "UNK":(isDec == 0 ? "ENC":"DEC")));
        } else {
            snprintf(slot_cache[i], 64, "empty");
        }
        if (fn) free(fn);
    }

    char *sd = save_dir_path(info);
    if (sd) {
        snprintf(savedir_cache, 128, "%s", sd);
        free(sd);
    } else {
        snprintf(savedir_cache, 128, "Not Found");
    }

    cached_info_ptr = info;
    force_cache_refresh = 0;
}

static void draw_appinfo_icon(icon_data *icon) {
    float w = vita2d_texture_get_width(icon->texture);
    float h = vita2d_texture_get_height(icon->texture);
    float z0 = APPINFO_ICON_WIDTH / w;
    float z1 = APPINFO_ICON_HEIGHT / h;
    float zoom = z0 < z1 ? z0 : z1;

    vita2d_draw_texture_scale_rotate_hotspot(icon->texture,
        APPINFO_ICON_LEFT + (APPINFO_ICON_WIDTH / 2),
        APPINFO_ICON_TOP + (APPINFO_ICON_HEIGHT / 2),
        zoom, zoom,
        0,
        w / 2,
        h / 2
    );
}

static void draw_appinfo(ScreenState state, appinfo *info) {
    int isAppInfo = state == PRINT_APPINFO;
    int appInfoDescPad = 25, btnIdx = 0;
    size_t tmpLen = sizeof(char)*270;
    char tmp[270] = {0};
    refresh_info_cache(info);

    vita2d_draw_rectangle(APPINFO_PANEL_LEFT, APPINFO_PANEL_TOP,
                          APPINFO_PANEL_WIDTH, APPINFO_PANEL_HEIGHT, THEME_PANEL);

    draw_appinfo_icon(&info->icon);

    if (!savelist_tab) {
        draw_button(APPINFO_BUTTON_LEFT, APPINFO_BUTTON_TOP(btnIdx, appinfo_btns),
                    APPINFO_BUTTON_WIDTH, APPINFO_BUTTON_HEIGHT(appinfo_btns),
                    "BACKUP", 1.0,
                    (state >= BACKUP_MODE && state <= BACKUP_FAIL),
                    isAppInfo && select_appinfo_button == btnIdx);
        ++btnIdx;
    }

    draw_button(APPINFO_BUTTON_LEFT, APPINFO_BUTTON_TOP(btnIdx, appinfo_btns),
                APPINFO_BUTTON_WIDTH, APPINFO_BUTTON_HEIGHT(appinfo_btns),
                "RESTORE", 1.0,
                (state >= RESTORE_MODE && state <= RESTORE_FAIL),
                isAppInfo && select_appinfo_button == btnIdx);
    ++btnIdx;

    draw_button(APPINFO_BUTTON_LEFT, APPINFO_BUTTON_TOP(btnIdx, appinfo_btns),
                APPINFO_BUTTON_WIDTH, APPINFO_BUTTON_HEIGHT(appinfo_btns),
                "DELETE", 1.0,
                (state >= DELETE_MODE && state <= DELETE_FAIL),
                isAppInfo && select_appinfo_button == btnIdx);
    ++btnIdx;

    if (!savelist_tab) {
        draw_button(APPINFO_BUTTON_LEFT, APPINFO_BUTTON_TOP(btnIdx, appinfo_btns),
                    APPINFO_BUTTON_WIDTH, APPINFO_BUTTON_HEIGHT(appinfo_btns),
                    "FORMAT", 1.0,
                    (state >= FORMAT_MODE && state <= FORMAT_FAIL),
                    isAppInfo && select_appinfo_button == btnIdx);
    } else {
        draw_button(APPINFO_BUTTON_LEFT, APPINFO_BUTTON_TOP(btnIdx, appinfo_btns),
                    APPINFO_BUTTON_WIDTH, APPINFO_BUTTON_HEIGHT(appinfo_btns),
                    "CHANGE REGION", 1.0,
                    (state >= REGION_SET_MODE && state <= REGION_SET_FAIL),
                    isAppInfo && select_appinfo_button == btnIdx);
    }

    vita2d_draw_rectangle(APPINFO_DESC_LEFT, APPINFO_DESC_TOP,
                          APPINFO_DESC_WIDTH, APPINFO_DESC_HEIGHT, THEME_INSET);
        
    vita2d_draw_rectangle(APPINFO_DESC_LEFT, APPINFO_DESC_TOP,
                          APPINFO_DESC_WIDTH, 4, current_accent_color);

    if (!savelist_tab && info->is_installed) {
        snprintf(tmp, tmpLen, "Title: %s", info->title);
        vita2d_pgf_draw_text(font,
                         APPINFO_DESC_LEFT + APPINFO_DESC_PADDING,
                         APPINFO_DESC_TOP + APPINFO_DESC_PADDING + appInfoDescPad,
                         current_accent_color, 1.0, tmp);
        appInfoDescPad += 25;
    }

    snprintf(tmp, tmpLen, "Title ID: %s", info->title_id);
    vita2d_pgf_draw_text(font,
                         APPINFO_DESC_LEFT + APPINFO_DESC_PADDING,
                         APPINFO_DESC_TOP + APPINFO_DESC_PADDING + appInfoDescPad,
                         THEME_TEXT, 1.0, tmp);

    appInfoDescPad += 25;
    snprintf(tmp, tmpLen, "Region: %s", info->region);
    vita2d_pgf_draw_text(font,
                         APPINFO_DESC_LEFT + APPINFO_DESC_PADDING,
                         APPINFO_DESC_TOP + APPINFO_DESC_PADDING + appInfoDescPad,
                         THEME_TEXT, 1.0, tmp);

    if (!savelist_tab) {
        appInfoDescPad += 25;
        snprintf(tmp, tmpLen, "Installed: %s", (info->is_installed ? "Yes":"No"));
        vita2d_pgf_draw_text(font,
                         APPINFO_DESC_LEFT + APPINFO_DESC_PADDING,
                         APPINFO_DESC_TOP + APPINFO_DESC_PADDING + appInfoDescPad,
                         THEME_TEXT, 1.0, tmp);
    }

    appInfoDescPad += 45;

    snprintf(tmp, tmpLen, "Save dir: %s", savedir_cache);
    vita2d_pgf_draw_text(font,
                         APPINFO_DESC_LEFT + APPINFO_DESC_PADDING,
                         APPINFO_DESC_TOP + APPINFO_DESC_PADDING + appInfoDescPad,
                         THEME_TEXT, 1.0, tmp);
}

static void draw_slots(appinfo *info, int slot) {
    vita2d_draw_rectangle(SLOT_PANEL_LEFT, SLOT_PANEL_TOP,
                          SLOT_PANEL_WIDTH, SLOT_PANEL_HEIGHT, THEME_PANEL);

    refresh_info_cache(info);

    for (int i=0; i<SLOT_BUTTON; ++i) {
        int isHover = slot < 0 && select_slot == i;
        draw_button(SLOT_BUTTON_LEFT, SLOT_BUTTON_TOP(i),
                    SLOT_BUTTON_WIDTH, SLOT_BUTTON_HEIGHT,
                    slot_cache[i], 1.0,
                    (slot == i),
                    isHover);
    }
}

static char *error_message(ProcessError error) {
    switch (error) {
        case NO_ERROR:
            return "NO ERROR";
        case ERROR_NO_SAVE_DIR:
            return "Could not find save directory.\n\nPlease start the game at least once";
        case ERROR_NO_SLOT_DIR:
            return "Could not find backup slot";
        case ERROR_MEMORY_ALLOC:
            return "Allocated memory exceeded";
        case ERROR_DECRYPT_DIR:
            return "Invalid license";
        case ERROR_COPY_DIR:
            return "Could not copy directory";
        case ERROR_DELETE_DIR:
            return "Could not delete directory";
        case ERROR_NO_SFO:
            return "Could not find param.sfo";
        case ERROR_NO_AID_MATCH:
            return "Account ID does not mach";
        case ERROR_INV_TITLEID:
            return "Invalid Title ID";
        default:
            return "Error!";
    }
}

static int selectable_count(appinfo *curr, int row, int col) {
    int selectable_count = 0;
    while (curr && selectable_count < (row * col)) {
        ++selectable_count;
        curr = curr->next;
    }

    return selectable_count;
}

static ScreenState on_mainscreen_event(int steps, int *step, appinfo **curr, appinfo **choose, int itemsC) {
    int btn = read_buttons();
    int tx, ty;

    if (read_touch(&tx, &ty)) {
        int handled = 0;
        int max_selectable = selectable_count(*curr, ICONS_ROW, ICONS_COL);

        for (int row = 0; row < ICONS_ROW && !handled; ++row) {
            for (int col = 0; col < ICONS_COL && !handled; ++col) {
                if (touch_in_rect(tx, ty, ICON_LEFT(col), ICON_TOP(row), ICON_WIDTH, ICON_HEIGHT)) {
                    int local_index = (row * ICONS_COL) + col;

                    if (local_index < max_selectable) {
                        if (row == select_row && col == select_col) {
                            btn |= SCE_CTRL_ENTER;
                        } else {
                            select_row = row;
                            select_col = col;
                        }
                    }
                    handled = 1;
                }
            }
        }

        if (!handled && touch_in_rect(tx, ty, 0, 0, SCREEN_HALF_WIDTH, HEADER_HEIGHT) && savelist_tab)
            btn |= SCE_CTRL_LTRIGGER;
        else if (!handled && touch_in_rect(tx, ty, SCREEN_HALF_WIDTH, 0, SCREEN_HALF_WIDTH, HEADER_HEIGHT) && !savelist_tab)
            btn |= SCE_CTRL_RTRIGGER;
    }

    if (btn & SCE_CTRL_UP) {
        if (select_row == 0) {
            if (*step == 0)
                return UNKNOWN;

            --(*step);

            for (int i=0; i<ICONS_COL; ++i, *curr=(*curr)->prev)
                unload_icon(*curr);

        } else {
            --select_row;
        }

        return MAIN_SCREEN;

    } else if (btn & SCE_CTRL_DOWN) {
        if (select_row+1 == ICONS_ROW) {
            if (*step == steps)
                return UNKNOWN;

            ++(*step);

            for (int i=0; i<ICONS_COL; ++i, *curr=(*curr)->next)
                unload_icon(*curr);

        } else {
            ++select_row;
        }

        if (IS_OVERFLOW())
            --select_row;

        return MAIN_SCREEN;

    } else if (btn & SCE_CTRL_LEFT) {
        select_col = select_col-1 < 0 ? 0:select_col-1;

        return MAIN_SCREEN;

    } else if (btn & SCE_CTRL_RIGHT) {
        select_col = select_col+1 == ICONS_COL ? select_col:select_col+1;

        if (IS_OVERFLOW())
            --select_col;

        return MAIN_SCREEN;

    } else if ((btn & SCE_CTRL_LTRIGGER) || (btn & SCE_CTRL_RTRIGGER)) {
        if ((!savelist_tab && (btn & SCE_CTRL_LTRIGGER)) ||
                (savelist_tab && (btn & SCE_CTRL_RTRIGGER)))
            return MAIN_SCREEN;

        old_savelist_tab = savelist_tab;
        savelist_tab = !savelist_tab;
        appinfo_btns = appinfo_btns == 4 ? 3:4;
        refreshUI = 1;

        return MAIN_SCREEN;

    } else if (!(btn & SCE_CTRL_HOLD) && (btn & SCE_CTRL_ENTER)) {
        if (!itemsC)
            return MAIN_SCREEN;

        int len = (select_row * ICONS_COL) + select_col;

        appinfo *tmp = *curr;
        for (int i=0; tmp && i<len; ++i, tmp=tmp->next);
            *choose = tmp;

        select_appinfo_button = 0;

        return PRINT_APPINFO;

    } else if (!(btn & SCE_CTRL_HOLD) && (btn & SCE_CTRL_TRIANGLE)) {
        return MENU_OPEN;
    }

    return UNKNOWN;
}

static ScreenState on_appinfo_event() {
    int btn = read_buttons();
    int ret = UNKNOWN;
    int tx, ty;

    if (read_touch(&tx, &ty)) {
        if (!touch_in_rect(tx, ty, APPINFO_PANEL_LEFT, APPINFO_PANEL_TOP,
                            APPINFO_PANEL_WIDTH, APPINFO_PANEL_HEIGHT)) {
            btn |= SCE_CTRL_CANCEL;
        } else {
            for (int i = 0; i < appinfo_btns; ++i) {
                if (touch_in_rect(tx, ty, APPINFO_BUTTON_LEFT, APPINFO_BUTTON_TOP(i, appinfo_btns),
                                   APPINFO_BUTTON_WIDTH, APPINFO_BUTTON_HEIGHT(appinfo_btns))) {
                    select_appinfo_button = i;
                    btn |= SCE_CTRL_ENTER;
                    break;
                }
            }
        }
    }

    if (btn & SCE_CTRL_UP) {
        select_appinfo_button = select_appinfo_button-1 < 0 ? 0:select_appinfo_button-1;

        ret = PRINT_APPINFO;

    } else if (btn & SCE_CTRL_DOWN) {
        select_appinfo_button = select_appinfo_button+1 == appinfo_btns ?
                                    select_appinfo_button:select_appinfo_button+1;

        ret = PRINT_APPINFO;

     } else if (!(btn & SCE_CTRL_HOLD) && (btn & SCE_CTRL_CANCEL)) {
        ret = MAIN_SCREEN;

    } else if (!(btn & SCE_CTRL_HOLD) && (btn & SCE_CTRL_ENTER)) {
        select_slot = 0;

        switch (select_appinfo_button) {
            case 0:
                ret = (savelist_tab ? RESTORE_MODE:BACKUP_MODE);
                break;
            case 1:
                ret = (savelist_tab ? DELETE_MODE:RESTORE_MODE);
                break;
            case 2:
                ret = (savelist_tab ? REGION_SET_MODE:DELETE_MODE);
                break;
            case 3:
                ret = FORMAT_MODE;
                break;
            default:
                break;
        }
    }

    return ret;
}

static ScreenState on_slot_event(int *slot) {
    int btn = read_buttons();
    int tx, ty;

    *slot = -1;

    if (read_touch(&tx, &ty)) {
        if (!touch_in_rect(tx, ty, SLOT_PANEL_LEFT, SLOT_PANEL_TOP,
                            SLOT_PANEL_WIDTH, SLOT_PANEL_HEIGHT)) {
            btn |= SCE_CTRL_CANCEL;
        } else {
            for (int i = 0; i < SLOT_BUTTON; ++i) {
                if (touch_in_rect(tx, ty, SLOT_BUTTON_LEFT, SLOT_BUTTON_TOP(i),
                                   SLOT_BUTTON_WIDTH, SLOT_BUTTON_HEIGHT)) {
                    select_slot = i;
                    btn |= SCE_CTRL_ENTER;
                    break;
                }
            }
        }
    }

    if (btn & SCE_CTRL_UP)
        select_slot = select_slot-1 < 0 ? 0:select_slot-1;
    else if (btn & SCE_CTRL_DOWN)
        select_slot = select_slot+1 == SLOT_BUTTON ? select_slot:select_slot+1;
    else if (!(btn & SCE_CTRL_HOLD) && (btn & SCE_CTRL_CANCEL))
        return PRINT_APPINFO;
    else if (!(btn & SCE_CTRL_HOLD) && (btn & SCE_CTRL_ENTER))
        *slot = select_slot;

    return UNKNOWN;
}

static ScreenState on_menuopen_event() {
    int btn = read_buttons();
    int ret = UNKNOWN;

    if (!(btn & SCE_CTRL_HOLD) && (btn & SCE_CTRL_TRIANGLE)) {
        select_menu = 0;
        save_config();
        ret = MAIN_SCREEN;

    } else if (btn & SCE_CTRL_UP) {
        select_menu = select_menu-1 < 0 ? 0:select_menu-1;

    } else if (btn & SCE_CTRL_DOWN) {
        select_menu = select_menu+1 == MENU_ITEMS ? select_menu:select_menu+1;

    } else if (!select_menu && (btn & SCE_CTRL_LEFT) && select_device > 0) {
        snprintf(savemgr_fpath, sizeof(char)*26, "%s%s", devices[--select_device], SAVEMGR_FOLDER);
        snprintf(cur_device, sizeof(char)*5, "%s", devices[select_device]);
        ret = SWITCH_CURR_DEVICE;

    } else if (!select_menu && (btn & SCE_CTRL_RIGHT) && select_device+1 < device_num) {
        snprintf(savemgr_fpath, sizeof(char)*26, "%s%s", devices[++select_device], SAVEMGR_FOLDER);
        snprintf(cur_device, sizeof(char)*5, "%s", devices[select_device]);
        ret = SWITCH_CURR_DEVICE;
    
    } else if (select_menu == ACCENT_COLOR && (btn & SCE_CTRL_LEFT)) {
        current_accent_idx = current_accent_idx - 1 < 0 ? num_accent_colors - 1 : current_accent_idx - 1;
        current_accent_color = accent_colors[current_accent_idx];

    } else if (select_menu == ACCENT_COLOR && (btn & SCE_CTRL_RIGHT)) {
        current_accent_idx = (current_accent_idx + 1) % num_accent_colors;
        current_accent_color = accent_colors[current_accent_idx];

    } else if (select_menu == DARK_MODE && (btn & SCE_CTRL_LEFT || btn & SCE_CTRL_RIGHT)) {
        is_dark_mode = !is_dark_mode;

    } else if (!(btn & SCE_CTRL_HOLD) && (btn & SCE_CTRL_ENTER)) {
        save_config();
        switch (select_menu) {
            case BACKUP_ALL:
                ret = BACKUPALL_MODE;
                break;
            case BACKUP_ALL_ENC:
                ret = BACKUPALL_ENC_MODE;
                break;
            case DELETE_ALL_SLOTS:
                ret = DELETE_ALL_SLOTS_MODE;
                break;
            default:
                break;
        }
    }

    return ret;
}

static int copy_savedata_to_slot(appinfo *info, int slot, applist *savelist) {
    char *src = save_dir_path(info);
    char *dest = slot_dir_path(info, slot);
    char icn[54] = {0};
    int curr = 0, max = 0;
    int res = NO_ERROR;

    if (!src) {
        // TODO: need popup; need start game
        res = ERROR_NO_SAVE_DIR;
        goto exit;
    }

    if (!dest) {
        res = ERROR_MEMORY_ALLOC;
        goto exit;
    }

    lock_psbutton();

    if (info->is_decrypted && pfs_mount(src) < 0) {
        res = ERROR_DECRYPT_DIR;
        goto exit;
    }

    if (path_exists(dest)) // delete old files
        remove_dir_recursive(dest, NULL, 0, 0);

    create_dir(dest, 0777);

    snprintf(icn, sizeof(icn), "%s/%s/icon.png", savemgr_fpath, info->title_id);
    if (!path_exists(icn))
        copy_file(info->iconpath, icn, 0);

    max = count_files(src, info->is_decrypted);
    init_progress(max);

    if (!copy_dir_recursive(src, dest, incr_progress, &curr, max, info->is_decrypted)) {
        res = ERROR_COPY_DIR;
        goto exit;
    }

    update_list(savelist, 0, info->title_id);

exit:
    if (info->is_decrypted)
        pfs_unmount();

    unlock_psbutton();
    free(src);
    free(dest);

    force_cache_refresh = 1;
    return res;
}

static int copy_slot_to_savedata(appinfo *info, int slot, applist *savelist) {
    char *src = slot_dir_path(info, slot);
    char *dest = save_dir_path(info);
    char *sfo_path = NULL;
    size_t allocLen;
    int res = 0, curr = 0, max, forceRestore = 0;

    if (!src) {
        res = ERROR_MEMORY_ALLOC;
        goto exit;

    } else if (!path_exists(src)) {
        res = ERROR_NO_SLOT_DIR;
        goto exit;
    }

    lock_psbutton();

    if (!dest) {
        if (info->is_decrypted) {
            res = ERROR_NO_SAVE_DIR;
            goto exit;

        } else {
            dest = save_dir_path_enc(info);

            if (!dest) { // if we are here, we said yes
                forceRestore = 1;
                allocLen = sizeof(char) * 40;
                dest = malloc(allocLen);

                snprintf(dest, allocLen, "ux0:user/00/savedata/%s", info->title_id);
            }
        }
    }

    if (info->is_decrypted && pfs_mount(dest) < 0) {
        res = ERROR_DECRYPT_DIR;
        goto exit;

    } else if (!info->is_decrypted && !forceRestore) { // TODO useless?
        char *sfo = slot_sfo_path(info, slot);
        uint64_t sfoAID, aid;

        if (!sfo) {
            res = ERROR_NO_SFO;
            goto exit;
        }

        sfoAID = get_accountid_sfo(sfo);
        aid = get_accountid();

        free(sfo);

        if (sfoAID != aid) {
            res = ERROR_NO_AID_MATCH;
            goto exit;
        }
    }

    max = count_files(src, info->is_decrypted);

    if (!info->is_decrypted && path_exists(dest)) remove_dir_recursive(dest, NULL, NULL, 0);

    create_dir(dest, 0777);
    init_progress(max);

    if (!copy_dir_recursive(src, dest, incr_progress, &curr, max, info->is_decrypted)) {
        res = ERROR_COPY_DIR;
        goto exit;
    }

    if (info->is_decrypted) {
        allocLen = sizeof(char) * (strlen(dest) + 20);
        sfo_path = malloc(allocLen);
        snprintf(sfo_path, allocLen, "%s/sce_sys/param.sfo", dest);

        if (path_exists(sfo_path)) change_accountid(sfo_path, get_accountid());

        free(sfo_path);
    }

exit:
    if (info->is_decrypted)
        pfs_unmount();

    unlock_psbutton();
    free(src);
    free(dest);

    force_cache_refresh = 1;
    return res;
}

static int delete_slot(appinfo *info, int slot, applist *savelist) {
    char *target = slot_dir_path(info, slot);
    char parent[44] = {0};
    int curr = 0, max;
    int res = NO_ERROR;

    if (!target) {
        res = ERROR_MEMORY_ALLOC;
        goto exit;

    } else if (!path_exists(target)) {
        res = ERROR_NO_SLOT_DIR;
        goto exit;
    }

    lock_psbutton();

    max = count_files(target, 0)+1;
    init_progress(max);
    remove_dir_recursive(target, incr_progress, &curr, max);
    snprintf(parent, sizeof(parent), "%s/%s", savemgr_fpath, info->title_id);

    if (count_folders(parent) == 0) {
        remove_dir_recursive(parent, incr_progress, &curr, max);
        update_list(savelist, 1, info->title_id);
        refreshUI = 1;
    }

exit:
    unlock_psbutton();
    free(target);

    force_cache_refresh = 1;
    return res;
}

static int format_savedata(appinfo *info, applist *list) {
    char *target = save_dir_path(info);
    int curr = 0, max;
    int res = NO_ERROR;

    if (!target) {
        res = ERROR_NO_SAVE_DIR;
        goto exit;
    }

    lock_psbutton();

    max = count_files(target, 0);
    init_progress(max);
    remove_dir_recursive(target, incr_progress, &curr, max);

    if (!info->is_installed) {
        update_list(list, 1, info->title_id);
        refreshUI = 1;
    }

exit:
    unlock_psbutton();
    free(target);

    return res;
}

static int change_save_region(appinfo *info, applist *savelist) {
    int res = NO_ERROR;
    char *new_tid = NULL;
    char slot_path[47] = {0};
    int len = 0;

    new_tid = showImeDialog(info->title_id);
    if (!new_tid) {
        return res;

    } else if (new_tid[0] != 'P' || new_tid[1] != 'C' || new_tid[2] != 'S') {
        free(new_tid);
        return ERROR_INV_TITLEID;
    }

    snprintf(slot_path, sizeof(slot_path), "%s/%s", savemgr_fpath, info->title_id);
    len = count_folders(slot_path);

    for (int i=0; i<len; ++i) {
        char *sfo = slot_sfo_path(info, i);

        if (!sfo) {
            res = ERROR_NO_SFO;
            break;
        }

        res = change_titleid(sfo, new_tid);
        free(sfo);

        if (res < 0) {
            res = ERROR_NO_SFO;
            break;
        }
    }

    if (res) {
        char new_path[47] = {0};
        char new_icon_path[57] = {0};

        snprintf(new_icon_path, sizeof(new_icon_path), "%s/%s/icon.png", savemgr_fpath, new_tid);
        snprintf(info->title_id, sizeof(char)*16, "%s", new_tid);
        snprintf(info->real_id, sizeof(char)*16, "%s", new_tid);
        snprintf(info->iconpath, sizeof(char)*256, "%s", new_icon_path);
        setRegionLabel(new_tid[3], info);

        unload_icon(info);
        snprintf(new_path, sizeof(new_path), "%s/%s", savemgr_fpath, new_tid);
        sceIoRename(slot_path, new_path);
        load_icon(info);

        res = NO_ERROR;
    }

    free(new_tid);

    force_cache_refresh = 1;
    return res;
}

static int backup_all(const applist *gamelist, applist *savelist, int decrypt) {
    appinfo *tmp = gamelist->items;
    int curr = 0;
    int res = NO_ERROR;

    init_progress(gamelist->count);

    while (tmp) {
        int save_slot = 0;
        char *slotdir = slot_dir_path(tmp, save_slot);

        if (slotdir == NULL)
            return ERROR_MEMORY_ALLOC;

        while (path_exists(slotdir) && save_slot < 10) {
            free(slotdir);

            slotdir = slot_dir_path(tmp, ++save_slot);
            if (slotdir == NULL)
                return ERROR_MEMORY_ALLOC;
        }

        free(slotdir);
        incr_progress(&curr, gamelist->count);

        if (save_slot == 10)
            continue; // no free slot, skip :|

        tmp->is_decrypted = decrypt;
        res = copy_savedata_to_slot(tmp, save_slot, savelist);
        if (res != NO_ERROR && res != ERROR_NO_SAVE_DIR)
            break;

        tmp = tmp->next;
    }

    if (savelist_tab)
        refreshUI = 1;

    return res;
}

static int delete_all_slots(applist *savelist) {
    int curr = 0, res = NO_ERROR, max = 0;

    if (!savelist->count)
        return res;

    max = count_files(savemgr_fpath, 0);

    init_progress(max);
    res = remove_dir_recursive(savemgr_fpath, incr_progress, &curr, max);
    free_list(savelist);

    return (res > 0 ? NO_ERROR:ERROR_DELETE_DIR);
}

static void draw_screen(ScreenState state, applist *list, int slot) {
    vita2d_start_drawing();
    vita2d_clear_screen();

    draw_tabs();

    if (state >= MAIN_SCREEN)
        draw_icons(list->curr, list->count);

    if (state == MENU_OPEN) {
        vita2d_draw_rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, RGBA8(0, 0, 0, 150));
        draw_menu();
    }

    if (state >= PRINT_APPINFO && state <= REGION_SET_FAIL)
        draw_appinfo(state, list->choose);

    switch (state) {
        case BACKUP_MODE:
        case RESTORE_MODE:
        case DELETE_MODE:
            draw_slots(list->choose, -1);
            break;
        case BACKUP_CONFIRM:
        case BACKUP_PROGRESS:
        case BACKUP_FAIL:
        case RESTORE_CONFIRM:
        case RESTORE_PROGRESS:
        case RESTORE_FAIL:
        case DELETE_CONFIRM:
        case DELETE_PROGRESS:
        case DELETE_FAIL:
            draw_slots(list->choose, slot);
            break;
        default:
            break;
    }

   if (state == MAIN_SCREEN) {
        char guide_text[128];
        snprintf(guide_text, sizeof(guide_text), "%s %s Select   L R Switch Tabs   %s Confirm   %s Toggle Settings", 
                 ICON_UPDOWN, ICON_LEFTRIGHT, ICON_ENTER, ICON_TRIANGLE);

        int guide_w = vita2d_pgf_text_width(font, 1.0, guide_text);
        int guide_h = vita2d_pgf_text_height(font, 1.0, guide_text);
        int bg_width = guide_w + 30;
        int bg_height = guide_h + 16;
        int bg_x = SCREEN_WIDTH - bg_width;
        int bg_y = SCREEN_HEIGHT - bg_height;
        vita2d_pgf_draw_text(font, bg_x + 15, bg_y + guide_h + 8, THEME_GUIDE_TEXT, 1.0, guide_text);
    }

    vita2d_end_drawing();
    vita2d_wait_rendering_done();
    vita2d_swap_buffers();
}

static ScreenState noslot_state_machine(applist *list, ScreenState confirm_state,
                                 ScreenState progress_state, ScreenState fail_state,
                                 ScreenState exit_state, const char *confirm_msg,
                                 int (*progress_func)(appinfo*, applist*)) {
    // this hook will skip choice slot state
    ScreenState state = confirm_state;
    int last_error = NO_ERROR;

    while (1) {
        ScreenState new_state = UNKNOWN;

        draw_screen(state, list, -1);

        if (state == confirm_state) {
            new_state = confirm(confirm_msg, 1.0, 0) == CONFIRM ? progress_state:exit_state;

        } else if (state == progress_state) {
            draw_screen(state, list, -1);
            draw_screen(state, list, -1);
            last_error = progress_func(list->choose, list);
            new_state = last_error != NO_ERROR ? fail_state:exit_state;

        } else if (state == fail_state) {
            new_state = exit_state;
            alert(error_message(last_error), 1.0);

        } else {
            return state;
        }

        if (new_state != UNKNOWN)
            state = new_state;
    }
}

static ScreenState slot_state_machine(applist *list, applist *savelist,
                               ScreenState start_state, ScreenState confirm_state,
                               ScreenState progress_state, ScreenState fail_state,
                               const char *confirm_msg,
                               int (*progress_func)(appinfo*, int, applist*)) {
    ScreenState state = start_state;
    int last_error = NO_ERROR;
    int slot = -1;

    while (1) {
        ScreenState new_state = UNKNOWN;

        draw_screen(state, list, slot);

        if (state == start_state) {
            new_state = on_slot_event(&slot);

            if (new_state == UNKNOWN && slot >= 0)
                new_state = confirm_state;

        } else if (state == confirm_state) {
            char tmp[256];
            snprintf(tmp, sizeof(tmp), confirm_msg, slot);
            
            new_state = confirm(tmp, 1.0, 0) == CONFIRM ? progress_state:start_state;

            if (new_state == progress_state && start_state == BACKUP_MODE) {
                list->choose->is_decrypted = confirm("Do you want to decrypt the save?", 1.0, 1);

            } else if (new_state == progress_state && start_state == RESTORE_MODE) {
                list->choose->is_decrypted = is_save_decrypted(list->choose, slot);

                if (!list->choose->is_decrypted) {
                    char *path = save_dir_path_enc(list->choose);

                    if (!path)
                        new_state = confirm("Game not installed! Continue anyway?", 1.0, 1) == CONFIRM ?
                                    progress_state:start_state;
                    free(path);
                }
            }

        } else if (state == progress_state) {
            int oldSavelistC = savelist->count;
            draw_screen(state, list, slot);
            draw_screen(state, list, slot);

            last_error = progress_func(list->choose, slot, savelist);
            new_state = last_error != NO_ERROR ? fail_state:start_state;

            if (savelist_tab && start_state == DELETE_MODE && last_error == NO_ERROR && oldSavelistC > savelist->count)
                new_state = MAIN_SCREEN;

        } else if (state == fail_state) {
            alert(error_message(last_error), 1.0);
            new_state = start_state;

        } else {
            break;
        }

        if (new_state != UNKNOWN)
            state = new_state;
    }

    return state;
}

static ScreenState backupall_state_machine(applist *list, const applist *gamelist,
                                 applist *savelist, ScreenState start_state,
                                 ScreenState confirm_state, ScreenState progress_state,
                                 ScreenState fail_state, ScreenState exit_state,
                                 const char *confirm_msg,
                                 int (*progress_func)(const applist*, applist*, int)) {
    ScreenState state = start_state;
    int last_error = NO_ERROR;

    while (1) {
        ScreenState new_state = UNKNOWN;

        draw_screen(state, list, -1);

        if (state == start_state) {
            new_state = confirm_state;

        } else if (state == confirm_state) {
            new_state = confirm(confirm_msg, 1.0, 0) == CONFIRM ? progress_state:exit_state;

        } else if (state == progress_state) {
            int decrypt = start_state == BACKUPALL_MODE ? 1:0;
            draw_screen(state, list, -1);
            draw_screen(state, list, -1);

            last_error = progress_func(gamelist, savelist, decrypt);
            new_state = last_error != NO_ERROR ? fail_state:exit_state;

        } else if (state == fail_state) {
            new_state = exit_state;
            alert(error_message(last_error), 1.0);

        } else {
            break;
        }

        if (new_state != UNKNOWN)
            state = new_state;
    }

    return state;
}

static ScreenState deleteslots_state_machine(applist *list, applist *savelist,
                                 ScreenState start_state, ScreenState confirm_state,
                                 ScreenState progress_state, ScreenState fail_state,
                                 ScreenState exit_state, const char *confirm_msg,
                                 int (*progress_func)(applist*)) {
    ScreenState state = start_state;
    int last_error = NO_ERROR;

    while (1) {
        ScreenState new_state = UNKNOWN;

        draw_screen(state, list, -1);

        if (state == start_state) {
            new_state = confirm_state;

        } else if (state == confirm_state) {
            new_state = confirm(confirm_msg, 1.0, 0) == CONFIRM ? progress_state:exit_state;

        } else if (state == progress_state) {
            draw_screen(state, list, -1);
            draw_screen(state, list, -1);
            last_error = progress_func(savelist);
            new_state = last_error != NO_ERROR ? fail_state:exit_state;

        } else if (state == fail_state) {
            new_state = exit_state;
            alert(error_message(last_error), 1.0);

        } else {
            break;
        }

        if (new_state != UNKNOWN)
            state = new_state;
    }

    return state;
}

static ScreenState switch_device(applist *savelist) {
    int prog = 0;

    init_progress(2); // just to show something, it may take a second or two for big lists

    if (!path_exists(savemgr_fpath)) create_dir(savemgr_fpath, 0777);

    free_list(savelist);
    incr_progress(&prog, 2);
    get_savelist(savelist);
    incr_progress(&prog, 2);

    old_savelist_tab = 0; // to trigger list update
    refreshUI = 1;

    return MENU_OPEN;
}

static int mainloop() {
    ScreenState state = MAIN_SCREEN;
    applist gamelist = {0}, savelist = {0};
    applist *list = &gamelist;
    int slot = -1, step = 0;
    int ret, rows, steps;

    ret = get_applist(&gamelist);
    if (ret < 0) // loading error
        return -1;

    ret = get_savelist(&savelist);
    if (ret < 0) // loading error
        return -1;

    list->curr = list->items;
    select_row = select_col = 0;
    rows = (list->count / ICONS_COL) + ((list->count % ICONS_COL) ? 1:0);
    steps = rows - ICONS_ROW;

    if (steps < 0)
        steps = 0;

    while (1) {
        ScreenState new_state = UNKNOWN;

        draw_screen(state, list, slot);

        switch (state) {
            case MAIN_SCREEN:
                new_state = on_mainscreen_event(steps, &step, &list->curr, &list->choose, list->count);
                break;
            case PRINT_APPINFO:
                new_state = on_appinfo_event();
                break;
            case MENU_OPEN:
                new_state = on_menuopen_event();
                break;
            case BACKUP_MODE:
                new_state = slot_state_machine(list, &savelist,
                                                BACKUP_MODE, BACKUP_CONFIRM,
                                                BACKUP_PROGRESS, BACKUP_FAIL,
                                                "Backup savedata to slot %d",
                                                copy_savedata_to_slot);
                break;
            case RESTORE_MODE:
                new_state = slot_state_machine(list, NULL,
                                                RESTORE_MODE, RESTORE_CONFIRM,
                                                RESTORE_PROGRESS, RESTORE_FAIL,
                                                "Restore savedata from slot %d",
                                                copy_slot_to_savedata);
                break;
            case DELETE_MODE:
                new_state = slot_state_machine(list, &savelist,
                                                DELETE_MODE, DELETE_CONFIRM,
                                                DELETE_PROGRESS, DELETE_FAIL,
                                                "Delete save slot %d",
                                                delete_slot);
                break;
            case FORMAT_MODE:
                new_state = noslot_state_machine(list,
                                                FORMAT_CONFIRM, FORMAT_PROGRESS,
                                                FORMAT_FAIL, PRINT_APPINFO,
                                                "Format game savedata",
                                                format_savedata);
                break;
            case REGION_SET_MODE:
                new_state = noslot_state_machine(list,
                                                REGION_SET_CONFIRM, REGION_SET_PROGRESS,
                                                REGION_SET_FAIL, PRINT_APPINFO,
                                                "Change savefile region",
                                                change_save_region);
                break;
            case BACKUPALL_MODE:
                new_state = backupall_state_machine(list, &gamelist, &savelist,
                                                BACKUPALL_MODE, BACKUPALL_CONFIRM,
                                                BACKUPALL_PROGRESS, BACKUPALL_FAIL,
                                                MAIN_SCREEN,
                                                "Backup all saves? (Decrypted)",
                                                backup_all);
                break;
            case BACKUPALL_ENC_MODE:
                new_state = backupall_state_machine(list, &gamelist, &savelist,
                                                BACKUPALL_ENC_MODE, BACKUPALL_ENC_CONFIRM,
                                                BACKUPALL_ENC_PROGRESS, BACKUPALL_ENC_FAIL,
                                                MAIN_SCREEN,
                                                "Backup all saves? (Encrypted)",
                                                backup_all);
                break;
            case DELETE_ALL_SLOTS_MODE:
                new_state = deleteslots_state_machine(list, &savelist,
                                                DELETE_ALL_SLOTS_MODE, DELETE_ALL_SLOTS_CONFIRM,
                                                DELETE_ALL_SLOTS_PROGRESS, DELETE_ALL_SLOTS_FAIL,
                                                MAIN_SCREEN,
                                                "Delete all saves slots",
                                                delete_all_slots);
                break;
            case SWITCH_CURR_DEVICE:
                new_state = switch_device(&savelist);
                break;
            default:
                break;
        }

        if (refreshUI) {
            if (savelist_tab != old_savelist_tab) {
                old_savelist_tab = savelist_tab;
                list = savelist_tab ? &savelist:&gamelist;
                list->curr = list->items;
            }

            slot = -1;
            rows = (list->count / ICONS_COL) + ((list->count % ICONS_COL) ? 1:0);
            select_row = select_col = step = refreshUI = 0;
            steps = rows - ICONS_ROW;

            if (steps < 0)
                steps = 0;
        }

        state = new_state == UNKNOWN ? state:new_state;
    }

    return 0;
}

static int init_devices(char **error) {
    char *dev[5] = {"ux0","ur0","uma0","imc0","xmc0"};
    int devE[5] = {0,0,0,0,0};
    size_t len = sizeof(char) * 5;

    for (int i=0; i<5; ++i) {
        char tmp[6] = {0};

        snprintf(tmp, sizeof(tmp), "%s:", dev[i]);
        if (path_exists(tmp)) {
            devE[i] = 1;
            ++device_num;
        }
    }

    if (!device_num)
        goto exit_error;

    devices = malloc(sizeof(char*) * device_num);
    if (!devices)
        goto exit_error;

    for (int i=0, j=0; i<device_num; ++i, ++j) {
        while (!devE[j])
            ++j;

        devices[i] = malloc(len);
        if (!devices[i])
            goto exit_error;

        snprintf(devices[i], len, dev[j]);
    }

    snprintf(cur_device, len, "%s", devices[0]);
    snprintf(savemgr_fpath, sizeof(char)*26, "%s%s", devices[0], SAVEMGR_FOLDER);

    return 1;

exit_error:
    len = sizeof(char) * 20;
    *error = malloc(len);
    if (*error)
        snprintf(*error, len, "Cannot init devices");

    return 0;
}

int main() {
    char *error = NULL;

    vita2d_init();
    vita2d_set_clear_color(BLACK);

    font = load_system_fonts();

    if (!load_modules(&error))
        goto fatal_error;

    if (!init_devices(&error))
        goto fatal_error;

    if (!path_exists(savemgr_fpath))
        create_dir(savemgr_fpath, 0777);

    load_config();
    
    if (!path_exists(savemgr_fpath))
        create_dir(savemgr_fpath, 0777);

    sceAppMgrUmount("app0:");
    sceAppMgrUmount("savedata0:");

    init_input();
    init_console();

    while (mainloop() >= 0);

    for (int i=0; i<device_num; ++i)
        free(devices[i]);
    free(devices);

    sceKernelExitProcess(0);
    return 0;

fatal_error:
    vita2d_start_drawing();
    vita2d_clear_screen();

    if (error)
        vita2d_pgf_draw_text(font, 0, 40, RED, 2.0, error);
    else
        vita2d_pgf_draw_text(font, 0, 40, RED, 2.0, "Unknown fatal error");

    vita2d_end_drawing();
    vita2d_wait_rendering_done();
    vita2d_swap_buffers();

    free(error);
    sceKernelDelayThread(10 * 1000 * 1000);
    sceKernelExitProcess(1);
    return 1;
}
