#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vita2d.h>

#define SAVE_MANAGER    "SAVEMGRPP"
#define SAVEMGR_FOLDER  ":data/vitaSaveManager"

#define ICON_CIRCLE   "%"
#define ICON_CROSS    "&"
#define ICON_SQUARE   "'"
#define ICON_TRIANGLE "$"
#define ICON_TRIGGERS "01"
#define ICON_LJOY     "2"
#define ICON_SELECT   "4"

#define SCREEN_WIDTH                960
#define SCREEN_HEIGHT               544
#define SCREEN_HALF_WIDTH           (SCREEN_WIDTH / 2)
#define SCREEN_HALF_HEIGHT          (SCREEN_HEIGHT / 2)
#define HEADER_HEIGHT               40
#define FOOTER_HEIGHT               0
#define ICONS_ROW                   4
#define ICONS_COL                   7

#define MENU_ITEMS                  6

#define ITEMS_PANEL_PADDING         5
#define ITEMS_PANEL_WIDTH           (SCREEN_WIDTH)
#define ITEMS_PANEL_INNER_WIDTH     (ITEMS_PANEL_WIDTH - (ITEMS_PANEL_PADDING * 2))
#define ITEMS_PANEL_HEIGHT          (SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT)
#define ITEMS_PANEL_INNER_HEIGHT    (ITEMS_PANEL_HEIGHT - (ITEMS_PANEL_PADDING * 2))
#define ITEMS_PANEL_TOP             (HEADER_HEIGHT)
#define ITEMS_PANEL_LEFT            (0)
#define ITEMS_INNER_TOP             (ITEMS_PANEL_TOP + ITEMS_PANEL_PADDING)
#define ITEMS_INNER_LEFT            (ITEMS_PANEL_LEFT + ITEMS_PANEL_PADDING)
#define ITEM_BOX_MARGIN             5
#define ITEM_BOX_PADDING            0
#define ITEM_BOX_TOP(y, h)          (ITEMS_INNER_TOP + (ITEM_BOX_MARGIN * ((y) + 1)) + ((h) * (y)))
#define ITEM_BOX_LEFT(x, w)         (ITEMS_INNER_LEFT + (ITEM_BOX_MARGIN * ((x) + 1)) + ((w) * (x)))
#define ITEM_BOX_WIDTH(col)         (int)((ITEMS_PANEL_INNER_WIDTH - (ITEM_BOX_MARGIN * ((col) + 1))) / (col))
#define ITEM_BOX_HEIGHT(row)        (int)((ITEMS_PANEL_INNER_HEIGHT - (ITEM_BOX_MARGIN * ((row) + 1))) / (row))

#define ICON_BOX_WIDTH              ITEM_BOX_WIDTH(ICONS_COL)
#define ICON_BOX_HEIGHT             ITEM_BOX_HEIGHT(ICONS_ROW)
#define ICON_WIDTH                  (ICON_BOX_WIDTH - (ITEM_BOX_PADDING * 2))
#define ICON_HEIGHT                 (ICON_BOX_HEIGHT - (ITEM_BOX_PADDING * 2))
#define ICON_TOP(y)                 (ITEM_BOX_TOP((y), ICON_BOX_HEIGHT) + ITEM_BOX_PADDING)
#define ICON_LEFT(x)                (ITEM_BOX_LEFT((x), ICON_BOX_WIDTH) + ITEM_BOX_PADDING)

#define APPINFO_PANEL_TOP           (ITEMS_PANEL_TOP)
#define APPINFO_PANEL_LEFT          (ITEMS_PANEL_LEFT)
#define APPINFO_PANEL_WIDTH         (ITEMS_PANEL_WIDTH / 2)
#define APPINFO_PANEL_HEIGHT        (ITEMS_PANEL_HEIGHT)
#define APPINFO_PANEL_PADDING       5

#define APPINFO_DESC_TOP            (int)(APPINFO_PANEL_TOP + (APPINFO_PANEL_HEIGHT / 2) + APPINFO_PANEL_PADDING)
#define APPINFO_DESC_LEFT           (APPINFO_PANEL_LEFT + APPINFO_PANEL_PADDING)
#define APPINFO_DESC_WIDTH          (APPINFO_PANEL_WIDTH - (APPINFO_PANEL_PADDING * 2))
#define APPINFO_DESC_HEIGHT         (int)((APPINFO_PANEL_HEIGHT / 2) - (APPINFO_PANEL_PADDING * 2))
#define APPINFO_DESC_PADDING        5

#define APPINFO_ICON_PADDING        40
#define APPINFO_ICON_TOP            (APPINFO_PANEL_TOP + APPINFO_PANEL_PADDING + APPINFO_ICON_PADDING)
#define APPINFO_ICON_LEFT           (APPINFO_PANEL_LEFT + APPINFO_PANEL_PADDING + APPINFO_ICON_PADDING)
#define APPINFO_ICON_WIDTH          (int)((APPINFO_PANEL_WIDTH / 2) - (APPINFO_PANEL_PADDING * 2) - (APPINFO_ICON_PADDING * 2))
#define APPINFO_ICON_HEIGHT         (int)((APPINFO_PANEL_HEIGHT / 2) - (APPINFO_PANEL_PADDING * 2) - (APPINFO_ICON_PADDING * 2))

#define APPINFO_BUTTONS_TOP         (APPINFO_PANEL_TOP + APPINFO_PANEL_PADDING)
#define APPINFO_BUTTONS_LEFT        (int)(APPINFO_PANEL_LEFT + (APPINFO_PANEL_WIDTH / 2) + APPINFO_PANEL_PADDING)
#define APPINFO_BUTTONS_WIDTH       (int)((APPINFO_PANEL_WIDTH / 2) - (APPINFO_PANEL_PADDING * 2))
#define APPINFO_BUTTONS_HEIGHT      (int)((APPINFO_PANEL_HEIGHT / 2) - (APPINFO_PANEL_PADDING * 2))

#define APPINFO_BUTTON_MARGIN       5
#define APPINFO_BUTTON_WIDTH        (int)(APPINFO_BUTTONS_WIDTH - (APPINFO_BUTTON_MARGIN * 2))
#define APPINFO_BUTTON_HEIGHT(n)    (int)((APPINFO_BUTTONS_HEIGHT - (APPINFO_BUTTON_MARGIN * (n + 1))) / n)
#define APPINFO_BUTTON_LEFT         (APPINFO_BUTTONS_LEFT + APPINFO_BUTTON_MARGIN)
#define APPINFO_BUTTON_TOP(x, n)    (APPINFO_BUTTONS_TOP + APPINFO_BUTTON_MARGIN + (APPINFO_BUTTON_HEIGHT(n) + APPINFO_BUTTON_MARGIN) * (x))

#define SLOT_PANEL_TOP              (ITEMS_PANEL_TOP)
#define SLOT_PANEL_LEFT             (APPINFO_PANEL_LEFT + APPINFO_PANEL_WIDTH)
#define SLOT_PANEL_WIDTH            (ITEMS_PANEL_WIDTH / 2)
#define SLOT_PANEL_HEIGHT           (ITEMS_PANEL_HEIGHT)
#define SLOT_PANEL_PADDING          5

#define SLOT_HEADER_TOP             (ITEMS_PANEL_TOP + SLOT_PANEL_PADDING)
#define SLOT_HEADER_LEFT            (ITEMS_PANEL_LEFT + SLOT_PANEL_PADDING)
#define SLOT_HEADER_HEIGHT          40

#define SLOT_BUTTONS_TOP            (SLOT_HEADER_TOP + SLOT_HEADER_HEIGHT)
#define SLOT_BUTTONS_HEIGHT         (SLOT_PANEL_HEIGHT - SLOT_HEADER_HEIGHT - (SLOT_PANEL_PADDING * 2))
#define SLOT_BUTTON                 10
#define SLOT_BUTTON_MARGIN          5
#define SLOT_BUTTON_WIDTH           (int)(SLOT_PANEL_WIDTH - (SLOT_BUTTON_MARGIN * 2))
#define SLOT_BUTTON_HEIGHT          (int)((SLOT_BUTTONS_HEIGHT - (SLOT_BUTTON_MARGIN * (SLOT_BUTTON + 1))) / SLOT_BUTTON)
#define SLOT_BUTTON_LEFT            (SLOT_PANEL_LEFT + SLOT_BUTTON_MARGIN)
#define SLOT_BUTTON_TOP(x)          (SLOT_BUTTONS_TOP + SLOT_BUTTON_MARGIN + (SLOT_BUTTON_HEIGHT + SLOT_BUTTON_MARGIN) * (x))

#define ICON_BUF_SIZE               (50 * 1024)

#define DEFAULT             RGBA8(0, 111, 205, 255)
#define BLACK               RGBA8(0, 0, 0, 255)
#define LIGHT_SLATE_GRAY    RGBA8(119, 136, 153, 255)
#define LIGHT_GRAY          RGBA8(211, 211, 211, 255)
#define RED                 RGBA8(255, 60, 74, 255)
#define TRANSPARENT         RGBA8(255, 255, 255, 150)
#define WHITE               RGBA8(255, 255, 255, 255)

#define AQUA_BLUE           RGBA8(  0, 174, 199, 255)
#define COSMIC_RED          RGBA8(218,  36,  75, 255)
#define CRYSTAL_BLACK       RGBA8( 30,  30,  32, 255)
#define GRAY                RGBA8(210, 210, 210, 255)
#define GREEN               RGBA8( 76, 175,  80, 255)
#define KHAKI_BLACK         RGBA8(120, 108,  85, 255)
#define LIGHT_BLUE          RGBA8(120, 190, 225, 255)
#define LIME_GREEN          RGBA8(174, 213,  40, 255)
#define METALLIC_RED        RGBA8(150,  30,  40, 255)
#define NEON_ORANGE         RGBA8(255, 106,   0, 255)
#define ORANGE              RGBA8(255, 152,   0, 255)
#define PINK                RGBA8(233,  30,  99, 255)
#define PINK_BLACK          RGBA8(230, 100, 150, 255)
#define PURPLE              RGBA8(156,  39, 176, 255)
#define SAPPHIRE_BLUE       RGBA8( 24,  74, 148, 255)
#define TEAL                RGBA8(  0, 150, 136, 255)
#define YELLOW              RGBA8(255, 235,  59, 255)

#define COLOR_DARK_BG           RGBA8(30, 30, 30, 255)
#define COLOR_DARK_BTN          RGBA8(60, 60, 60, 255)
#define COLOR_DARK_BTN_TEXT     RGBA8(220, 220, 220, 255)
#define COLOR_DARK_GUIDE_BG     RGBA8(30, 30, 30, 180)
#define COLOR_DARK_GUIDE_TEXT   RGBA8(220, 220, 220, 255)
#define COLOR_DARK_INSET        RGBA8(20, 20, 20, 255)
#define COLOR_DARK_PANEL        RGBA8(45, 45, 45, 255)

#define COLOR_LIGHT_BG          RGBA8(255, 255, 255, 240)
#define COLOR_LIGHT_BTN         RGBA8(225, 225, 225, 255)
#define COLOR_LIGHT_BTN_TEXT    RGBA8(50, 50, 50, 255)
#define COLOR_LIGHT_GUIDE_BG    RGBA8(255, 255, 255, 180)
#define COLOR_LIGHT_GUIDE_TEXT  RGBA8(50, 50, 50, 255)
#define COLOR_LIGHT_INSET       RGBA8(240, 240, 240, 255)

#define THEME_BG         (is_dark_mode ? COLOR_DARK_BG       : COLOR_LIGHT_BG)
#define THEME_BTN_BG     (is_dark_mode ? COLOR_DARK_BTN      : COLOR_LIGHT_BTN)
#define THEME_BTN_TEXT   (is_dark_mode ? COLOR_DARK_BTN_TEXT : COLOR_LIGHT_BTN_TEXT)
#define THEME_GUIDE_BG   (is_dark_mode ? COLOR_DARK_GUIDE_BG   : COLOR_LIGHT_GUIDE_BG)
#define THEME_GUIDE_TEXT (is_dark_mode ? COLOR_DARK_GUIDE_TEXT : COLOR_LIGHT_GUIDE_TEXT)
#define THEME_INSET      (is_dark_mode ? COLOR_DARK_INSET    : COLOR_LIGHT_INSET)
#define THEME_PANEL      (is_dark_mode ? COLOR_DARK_PANEL    : WHITE)
#define THEME_TEXT       (is_dark_mode ? WHITE               : BLACK)
#define THEME_TEXT_MUTED (is_dark_mode ? LIGHT_GRAY          : LIGHT_SLATE_GRAY)

#define SCE_CTRL_HOLD 0x80000000

extern int SCE_CTRL_ENTER;
extern int SCE_CTRL_CANCEL;
extern char ICON_ENTER[2];
extern char ICON_CANCEL[2];

extern vita2d_pgf* font;
extern vita2d_pvf* symbol_font;

extern const unsigned int accent_colors[];
extern const char *accent_color_names[];
extern const int num_accent_colors;
extern unsigned int current_accent_color;
extern int is_dark_mode;

typedef struct point {
    int x;
    int y;
} point;

typedef struct rectangle {
    int left;
    int top;
    int right;
    int bottom;
} rectangle;

typedef struct icon_data {
    uint8_t *buf;
    vita2d_texture *texture;
} icon_data;
