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


static int countLines(const char *str) {
    int c = 1;
    while (*str) {
        if (*str == '\n') c++;
        str++;
    }
    return c;
}

static int get_dialog_button_width(const char *sym, const char *text, float zoom) {
    int spacing = 6;
    int sym_w = vita2d_pvf_text_width(symbol_font, zoom, sym);
    int text_w = vita2d_pgf_text_width(font, zoom, text);
    return sym_w + spacing + text_w;
}

static void draw_dialog_button(int *cursor_x, int y, const char *sym, const char *text, float zoom) {
    int spacing = 6;
    int sym_w = vita2d_pvf_text_width(symbol_font, zoom, sym);
    
    vita2d_pvf_draw_text(symbol_font, *cursor_x, y, THEME_TEXT, zoom, sym);
    *cursor_x += sym_w + spacing;
    
    vita2d_pgf_draw_text(font, *cursor_x, y, THEME_TEXT, zoom, text);
    *cursor_x += vita2d_pgf_text_width(font, zoom, text);
}

static void draw_centered_text_multiline(int center_x, int start_y, float zoom, const char *text) {
    char buffer[512];
    strncpy(buffer, text, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';
    int line_height = vita2d_pgf_text_height(font, zoom, "T");
    int current_y = start_y;

    char *line = buffer;
    char *next = NULL;

    while (line != NULL) {
        next = strchr(line, '\n');
        if (next != NULL) {
            *next = '\0'; // split string at newline
            next++;       // point to next char
        }

        if (*line != '\0') {
            int w = vita2d_pgf_text_width(font, zoom, line);
            vita2d_pgf_draw_text(font, center_x - (w / 2), current_y, THEME_TEXT, zoom, line);
        }
        
        current_y += line_height; 
        line = next;
    }
}

int confirm(const char *msg, float zoom, int yesno) {
    int text_width = vita2d_pgf_text_width(font, zoom, msg);
    int text_height = vita2d_pgf_text_height(font, zoom, msg) * countLines(msg); 

    int padding = 50;
    int width = text_width + (padding * 2);
    int height = text_height + (padding * 2);
    int left = (SCREEN_WIDTH - width) / 2;
    int top = (SCREEN_HEIGHT - height) / 2;

    const char *text1 = yesno ? "NO" : "CANCEL";
    const char *text2 = yesno ? "YES" : "CONFIRM";
    int middle_margin = 30;

    int btn1_w = get_dialog_button_width(ICON_CANCEL, text1, zoom);
    int btn2_w = get_dialog_button_width(ICON_ENTER, text2, zoom);
    int total_btn_w = btn1_w + middle_margin + btn2_w;

    if (width < total_btn_w + (padding * 2)) {
        width = total_btn_w + (padding * 2);
        left = (SCREEN_WIDTH - width) / 2;
    }

    int cursor_x = left + (width - total_btn_w) / 2;
    int center_x = left + (width / 2);

    vita2d_start_drawing();
    vita2d_draw_rectangle(left + 6, top + 6, width, height, RGBA8(0, 0, 0, 80));
    vita2d_draw_rectangle(left-4, top-4, width+8, height+8, current_accent_color);
    vita2d_draw_rectangle(left, top, width, height, THEME_PANEL);
    
    draw_centered_text_multiline(center_x, top + padding, zoom, msg);
    
    draw_dialog_button(&cursor_x, top + height - 25, ICON_CANCEL, text1, zoom);
    cursor_x += middle_margin;
    draw_dialog_button(&cursor_x, top + height - 25, ICON_ENTER, text2, zoom);
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
    int text_height = vita2d_pgf_text_height(font, zoom, msg) * countLines(msg);
    int padding = 50;
    int width = text_width + (padding * 2);
    int height = text_height + (padding * 2);
    int left = (SCREEN_WIDTH - width) / 2;
    int top = (SCREEN_HEIGHT - height) / 2;

    int btn_w = get_dialog_button_width(ICON_ENTER, "CLOSE", zoom);

    if (width < btn_w + (padding * 2)) {
        width = btn_w + (padding * 2);
        left = (SCREEN_WIDTH - width) / 2;
    }

    int cursor_x = left + (width - btn_w) / 2;
    int center_x = left + (width / 2);

    vita2d_start_drawing();
    vita2d_draw_rectangle(left + 6, top + 6, width, height, RGBA8(0, 0, 0, 80));
    vita2d_draw_rectangle(left-4, top-4, width+8, height+8, current_accent_color);
    vita2d_draw_rectangle(left, top, width, height, THEME_PANEL);
    
    draw_centered_text_multiline(center_x, top + padding, zoom, msg);
    draw_dialog_button(&cursor_x, top + height - 25, ICON_ENTER, "CLOSE", zoom);
    
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

void draw_progress(int current, int max, const char *msg) {
    int width = 500;
    int height = 140;
    int left = (SCREEN_WIDTH - width) / 2;
    int top = (SCREEN_HEIGHT - height) / 2;
    int gauge_width = 400;
    int gauge_height = 40;
    int gauge_left = left + 50;
    int gauge_top = top + 70;
    int progress_width = max > 0 ? (current * gauge_width / max) : 0;
    int center_x = left + (width / 2);

    vita2d_start_drawing();
    vita2d_draw_rectangle(left + 6, top + 6, width, height, RGBA8(0, 0, 0, 80));
    vita2d_draw_rectangle(left-4, top-4, width+8, height+8, current_accent_color);
    vita2d_draw_rectangle(left, top, width, height, THEME_PANEL);
    
    if (msg) {
        char txt[256];
        snprintf(txt, sizeof(txt), "%s (%d/%d)", msg, current, max);
        int txt_w = vita2d_pgf_text_width(font, 1.0, txt);
        vita2d_pgf_draw_text(font, center_x - (txt_w / 2), gauge_top - 15, THEME_TEXT, 1.0, txt);
    }

    vita2d_draw_rectangle(gauge_left, gauge_top, gauge_width, gauge_height, THEME_INSET);
    if (progress_width > 0) { 
        vita2d_draw_rectangle(gauge_left, gauge_top, progress_width, gauge_height, current_accent_color);
    }
    vita2d_end_drawing();
    vita2d_wait_rendering_done();
    vita2d_swap_buffers();
}

void init_progress(int max, const char *msg) {
    draw_progress(0, max, msg);
}

void incr_progress(int *current, int max, const char *msg) {
    ++(*current);
    draw_progress(*current, max, msg);
}