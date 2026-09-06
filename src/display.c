#include "common.h"
#include "display.h"
#include "input.h"

const unsigned int accent_colors[] = {
    DEFAULT, AQUA_BLUE, COSMIC_RED, CRYSTAL_BLACK, GRAY, GREEN, KHAKI_BLACK, LIGHT_BLUE, LIME_GREEN, METALLIC_RED, NEON_ORANGE, ORANGE, PINK, PINK_BLACK, PURPLE, SAPPHIRE_BLUE, TEAL, YELLOW
};
const char *accent_color_names[] = {
    "Default", "Aqua Blue", "Cosmic Red", "Crystal Black", "Gray", "Green", "Khaki Black", "Light Blue", "Lime Green", "Metallic Red", "Neon Orange", "Orange", "Pink", "Pink Black", "Purple", "Sapphire Blue", "Teal", "Yellow"
};
const int num_accent_colors = 18;

char confirm_msg[64];
char yesno_msg[64];
char close_msg[64];

static int countLines(const char *str) {
    int c = 1;
    while (*str) {
        if (*str == '\n') c++;
        str++;
    }
    return c;
}

void init_console() {
    snprintf(confirm_msg, sizeof(confirm_msg), "%s CANCEL    %s CONFIRM", ICON_CANCEL, ICON_ENTER);
    confirm_msg_width = vita2d_pgf_text_width(font, 1.0, confirm_msg);

    snprintf(yesno_msg, sizeof(yesno_msg), "%s No    %s Yes", ICON_CANCEL, ICON_ENTER);
    yesno_msg_width = vita2d_pgf_text_width(font, 1.0, yesno_msg);

    snprintf(close_msg, sizeof(close_msg), "%s CLOSE", ICON_ENTER);
    close_msg_width = vita2d_pgf_text_width(font, 1.0, close_msg);
}

int confirm(const char *msg, float zoom, int yesno) {
    int text_width = vita2d_pgf_text_width(font, zoom, msg);
    int text_height = vita2d_pgf_text_height(font, zoom, msg);

    int padding = 50;
    int width = text_width + (padding * 2);
    int height = text_height + (padding * 2);
    int left = (SCREEN_WIDTH - width) / 2;
    int top = (SCREEN_HEIGHT - height) / 2;
    int textXW = (width - (!yesno ? confirm_msg_width:yesno_msg_width)) / 2;

    vita2d_start_drawing();
    vita2d_draw_rectangle(left + 6, top + 6, width, height, RGBA8(0, 0, 0, 80));
    vita2d_draw_rectangle(left-4, top-4, width+8, height+8, current_accent_color);
    vita2d_draw_rectangle(left, top, width, height, THEME_PANEL);
    vita2d_pgf_draw_text(font, left + padding, top + padding, THEME_TEXT, zoom, msg);
    vita2d_pgf_draw_text(font, left + textXW,
                         top + height - 25, THEME_TEXT, zoom,
                         (!yesno ? confirm_msg:yesno_msg));           
    vita2d_end_drawing();
    vita2d_wait_rendering_done();
    vita2d_swap_buffers();

    while (1) {
        int btn = read_buttons();

        if (btn & SCE_CTRL_HOLD)
            continue;
        else if (btn & SCE_CTRL_ENTER)
            return CONFIRM;
        else if (btn & SCE_CTRL_CANCEL)
            return CANCEL;
    }
}

int alert(const char *msg, float zoom) {
    int text_width = vita2d_pgf_text_width(font, zoom, msg);
    int text_height = vita2d_pgf_text_height(font, zoom, msg);
    int padding = 50;
    int width = text_width + (padding * 2);
    int height = text_height + (padding * 2);
    int left = (SCREEN_WIDTH - width) / 2;
    int top = (SCREEN_HEIGHT - height) / 2;

    vita2d_start_drawing();
    vita2d_draw_rectangle(left + 6, top + 6, width, height, RGBA8(0, 0, 0, 80));
    vita2d_draw_rectangle(left-4, top-4, width+8, height+8, current_accent_color);
    vita2d_draw_rectangle(left, top, width, height, THEME_PANEL);
    vita2d_pgf_draw_text(font, left + padding, top + padding, THEME_TEXT, zoom, msg);
    vita2d_pgf_draw_text(font,
                         left + ((width - close_msg_width) / 2),
                         top + height - 25, THEME_TEXT, zoom, close_msg);
    vita2d_end_drawing();
    vita2d_wait_rendering_done();
    vita2d_swap_buffers();

    while (1) {
        int btn = read_buttons();

        if (btn & SCE_CTRL_HOLD)
            continue;
        else if (btn & SCE_CTRL_ENTER)
            return 0;
    }
}

void draw_progress(int current, int max) {
    int width = 500;
    int height = 140;
    int left = (SCREEN_WIDTH - width) / 2;
    int top = (SCREEN_HEIGHT - height) / 2;
    int gauge_width = 400;
    int gauge_height = 40;
    int gauge_left = left + 50;
    int gauge_top = top + 50;
    int progress_width = current * gauge_width / max;

    vita2d_start_drawing();
    vita2d_draw_rectangle(left + 6, top + 6, width, height, RGBA8(0, 0, 0, 80));
    vita2d_draw_rectangle(left-4, top-4, width+8, height+8, current_accent_color);
    vita2d_draw_rectangle(left, top, width, height, THEME_PANEL);
    vita2d_draw_rectangle(gauge_left, gauge_top, gauge_width, gauge_height, THEME_INSET);
    if (progress_width > 0) { 
        vita2d_draw_rectangle(gauge_left, gauge_top, progress_width, gauge_height, current_accent_color);
    }
    vita2d_end_drawing();
    vita2d_wait_rendering_done();
    vita2d_swap_buffers();
}

void init_progress(int max) {
    draw_progress(0, max);
}

void incr_progress(int *current, int max) {
    ++(*current);
    draw_progress(*current, max);
}
