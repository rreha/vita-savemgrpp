#pragma once

#include <psp2/ctrl.h>

void init_input();
void lock_psbutton();
void unlock_psbutton();
int read_buttons();
int read_touch(int *x, int *y);
int touch_in_rect(int tx, int ty, int left, int top, int width, int height);
