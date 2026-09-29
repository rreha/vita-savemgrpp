#pragma once

enum {
    CANCEL = 0,
    CONFIRM = 1
};

int confirm(const char *msg, float zoom, int yesno);
int alert(const char *msg, float zoom);
void draw_progress(int current, int max, const char *msg);
void init_progress(int max, const char *msg);
void incr_progress(int *current, int max, const char *msg);
