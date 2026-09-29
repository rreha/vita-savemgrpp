#pragma once

#include "common.h"
#include "file.h"
#include "display.h"
#include <math.h>

#define APP_DB "ur0:shell/db/app.db"

typedef struct appinfo {
    char title_id[16];
    char real_id[16];
    char region[5];
    char title[256];
    char eboot[256];
    char dev[5];
    char iconpath[256];
    icon_data icon;
    int is_decrypted; // aux var
    int is_installed;
    struct appinfo *next;
    struct appinfo *prev;
} appinfo;

typedef struct applist {
    uint32_t count;
    appinfo *items;
    appinfo *tail;
    appinfo *curr;
    appinfo *choose;
} applist;

extern char savemgr_fpath[128];

void setRegionLabel(char regionID, appinfo *info);
int get_applist(applist *list);
int get_savelist(applist *savelist);
int update_list(applist *list, int cmd, const char *deleteID);
void free_list(applist *list);
void circle_mask(vita2d_texture *tex);
float lerp(float current, float target, float speed);
void load_icon(appinfo *info);
void unload_icon(appinfo *icon);
