#pragma once

enum {
    CANCEL = 0,
    CONFIRM = 1
};

void init_console();
int confirm(const char *msg, float zoom, int yesno);
int alert(const char *msg, float zoom);
void init_progress(int max);
void incr_progress(int *current, int max);
