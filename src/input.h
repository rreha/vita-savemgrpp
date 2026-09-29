#pragma once

#include <psp2/apputil.h>
#include <psp2/ctrl.h>
#include <psp2/shellutil.h>
#include <psp2/system_param.h>
#include <psp2/touch.h>

void init_input();
void lock_psbutton();
void unlock_psbutton();
int read_buttons();
int read_touch(int *x, int *y);
int touch_in_rect(int tx, int ty, int left, int top, int width, int height);
