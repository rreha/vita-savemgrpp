#include <stdint.h>

#include "sqlite3.h"
#include "utils.h"

char savemgr_fpath[128] = {0};

void setRegionLabel(char regionID, appinfo *info) {
    switch (regionID) {
        case 'B': case 'F':
            snprintf(info->region, sizeof(info->region), "EU");
            break;
        case 'C': case 'G':
            snprintf(info->region, sizeof(info->region), "JP");
            break;
        case 'D': case 'H':
            snprintf(info->region, sizeof(info->region), "ASIA");
            break;
        case 'A': case 'E':
            snprintf(info->region, sizeof(info->region), "US");
            break;
        case 'I':
            snprintf(info->region, sizeof(info->region), "INT");
            break;
        default:
            snprintf(info->region, sizeof(info->region), "N/A");
            break;
    }
}

static void get_orphan_saves(applist *list) {
    SceUID dfd = sceIoDopen("ux0:user/00/savedata");
    int res = 0;

    if (dfd < 0) return;

    do {
        SceIoDirent dir;
        memset(&dir, 0, sizeof(SceIoDirent));

        res = sceIoDread(dfd, &dir);

        if (res <= 0 || strcmp(dir.d_name, ".") == 0 || strcmp(dir.d_name, "..") == 0 ||
                dir.d_name[0] != 'P' || !SCE_S_ISDIR(dir.d_stat.st_mode))
            continue;

        if (dir.d_name[1] == 'C' && dir.d_name[2] == 'S') {
            appinfo *tmp = list->items;

            while (tmp) {
                if (strcmp(tmp->title_id, dir.d_name) == 0)
                    break;

                tmp = tmp->next;
            }

            if (!tmp) {
                appinfo *info = calloc(1, sizeof(appinfo));

                info->is_installed = 0;
                strncpy(info->title_id, dir.d_name, sizeof(info->title_id));
                strncpy(info->real_id, dir.d_name, sizeof(info->real_id));
                setRegionLabel(dir.d_name[3], info);

                list->tail->next = info;
                info->prev = list->tail;
                list->tail = info;

                ++list->count;
            }
        }
    } while (res > 0);

	sceIoDclose(dfd);
}

static int get_applist_callback(void *data, int argc, char **argv, char **cols) {
    applist *list = (applist*)data;
    appinfo *info = calloc(1, sizeof(appinfo));

    if (list->count == 0) {
        list->items = list->tail = info;

    } else {
        list->tail->next = info;
        info->prev = list->tail;
        list->tail = info;
    }

    ++list->count;

    info->is_installed = 1;

    snprintf(info->title_id, sizeof(info->title_id), "%s", argv[0]);
    snprintf(info->real_id, sizeof(info->real_id), "%s", argv[1]);
    snprintf(info->title, sizeof(info->title), "%s", argv[2]);
    snprintf(info->eboot, sizeof(info->eboot), "%s", argv[3]);
    snprintf(info->dev, sizeof(info->dev), "%s", argv[4]);
    snprintf(info->iconpath, sizeof(info->iconpath), "%s", argv[5]);

    for (int i=0; info->title[i] != '\0' && i < 256; ++i) {
        if (info->title[i] == '\n')
            info->title[i] = ' ';
    }

    setRegionLabel(info->title_id[3], info);

    return 0;
}

int get_applist(applist *list) {
    const char *query = "select a.titleid, b.realid, c.title, d.ebootbin,"
                  "       rtrim(substr(d.ebootbin, 0, 5), ':') as dev,"
                  "       e.iconpath"
                  "  from (select titleid"
                  "          from tbl_appinfo"
                  "         where key = 566916785"
                  "           and titleid like 'PCS%'"
                  "         order by titleid) a,"
                  "       (select titleid, val as realid"
                  "          from tbl_appinfo"
                  "         where key = 278217076) b,"
                  "       tbl_appinfo_icon c,"
                  "       (select titleid, val as ebootbin"
                  "          from tbl_appinfo"
                  "         where key = 3022202214) d,"
                  "       (select titleid, iconpath"
                  "          from tbl_appinfo_icon"
                  "         where type = 0) e"
                  " where a.titleid = b.titleid and a.titleid = c.titleid"
                  "   and a.titleid = d.titleid and a.titleid = e.titleid";
    sqlite3 *db;
    char *errMsg;

    int ret = sqlite3_open(APP_DB, &db);
    if (ret)
        return -1;


    ret = sqlite3_exec(db, query, get_applist_callback, (void *)list, &errMsg);
    if (ret != SQLITE_OK) {
        sqlite3_close(db);
        return -2;
    }

    sqlite3_close(db);

    get_orphan_saves(list);

    if (list->count < 1)
        return -3;

    return 0;
}

int get_savelist(applist *savelist) {
    SceUID dfd = sceIoDopen(savemgr_fpath);
    int res;

    if (dfd < 0)
        return -1;

    do {
        SceIoDirent dir;
        memset(&dir, 0, sizeof(SceIoDirent));

        res = sceIoDread(dfd, &dir);
        if (res <= 0 || strcmp(dir.d_name, ".") == 0 || strcmp(dir.d_name, "..") == 0 || !SCE_S_ISDIR(dir.d_stat.st_mode))
            continue;

        appinfo *saveinfo = calloc(1, sizeof(appinfo));
        char iconPath[128] = {0};

        if (savelist->count == 0) {
            savelist->items = savelist->tail = saveinfo;
        } else {
            savelist->tail->next = saveinfo;
            saveinfo->prev = savelist->tail;
            savelist->tail = saveinfo;
        }

        ++savelist->count;

        snprintf(saveinfo->title_id, sizeof(saveinfo->title_id), "%s", dir.d_name);
        snprintf(saveinfo->real_id, sizeof(saveinfo->real_id), "%s", dir.d_name);

        snprintf(iconPath, sizeof(iconPath), "%s/%s/icon.png", savemgr_fpath, dir.d_name);
        snprintf(saveinfo->iconpath, sizeof(saveinfo->iconpath), "%s", iconPath);

        setRegionLabel(saveinfo->title_id[3], saveinfo);
    } while (res > 0);

    sceIoDclose(dfd);

    return 0;
}

int update_list(applist *list, int cmd, const char *titleID) {
    int ret = 0;

    switch (cmd) {
        case 0: { // Add new item
            char iconPath[128] = {0};
            appinfo *tmp = list->items;
            appinfo *info;

            while (tmp && strcmp(tmp->title_id, titleID) != 0)
                tmp = tmp->next;

            if (tmp != NULL)
                break;

            info = calloc(1, sizeof(appinfo));

            if (list->count == 0) {
                list->items = list->tail = info;
            } else {
                list->tail->next = info;
                info->prev = list->tail;
                list->tail = info;
            }

            ++list->count;
            list->curr = list->choose = list->items;

            snprintf(info->title_id, sizeof(info->title_id), "%s", titleID);
            snprintf(info->real_id, sizeof(info->real_id), "%s", titleID);
            snprintf(iconPath, sizeof(iconPath), "%s/%s/icon.png", savemgr_fpath, titleID);
            snprintf(info->iconpath, sizeof(info->iconpath), "%s", iconPath);

            setRegionLabel(info->title_id[3], info);
        }
            break;
        case 1: { // Delete item
            appinfo *tmp = list->items;

            while (tmp && strcmp(tmp->title_id, titleID) != 0)
                tmp = tmp->next;

            if (tmp == NULL)
                break;

            if (tmp->prev) {
                tmp->prev->next = tmp->next;

            } else {
                list->items = tmp->next;

                if (!list->items || !list->items->next)
                    list->tail = list->items;
            }

            if (tmp->next)
                tmp->next->prev = tmp->prev;
            else
                list->tail = tmp->prev;

            --list->count;

            list->curr = list->choose = list->items;

            unload_icon(tmp);
            free(tmp);
        }
            break;
        default:
            ret = -1;
            break;
    }

    return ret;
}

void free_list(applist *list) {
    appinfo *tmp = list->items;

    while (tmp) {
        appinfo *aux = tmp->next;

        unload_icon(tmp);
        free(tmp);

        tmp = aux;
    }

    list->count = 0;
    list->curr = list->choose = list->tail = list->items = NULL;
}

void circle_mask(vita2d_texture *tex) {
    if (!tex) return;
    unsigned int stride = vita2d_texture_get_stride(tex);
    unsigned int w = vita2d_texture_get_width(tex);
    unsigned int h = vita2d_texture_get_height(tex);
    uint32_t *pixels = (uint32_t *)vita2d_texture_get_datap(tex);

    float cx = w / 2.0f;
    float cy = h / 2.0f;
    float radius = (w < h ? w : h) / 2.0f;
    float feather = 1.5f; 
    float rad_sq = radius * radius;
    float inner_rad = radius - feather;
    float inner_rad_sq = inner_rad * inner_rad;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float dx = (x + 0.5f) - cx;
            float dy = (y + 0.5f) - cy;
            float dist_sq = (dx * dx) + (dy * dy);
            
            uint32_t *pixel = &pixels[(y * stride / 4) + x];
            
            if (dist_sq >= rad_sq) {
                *pixel &= 0x00FFFFFF; 
            } 

            else if (dist_sq > inner_rad_sq) {
                float distance = sqrtf(dist_sq);
                float alpha_factor = (radius - distance) / feather;
                alpha_factor = alpha_factor * alpha_factor * (3.0f - 2.0f * alpha_factor);
                uint8_t old_alpha = (*pixel >> 24) & 0xFF;
                uint8_t new_alpha = (uint8_t)(old_alpha * alpha_factor);
                *pixel = (*pixel & 0x00FFFFFF) | (new_alpha << 24);
            }
        }
    }
}

float lerp(float current, float target, float speed) {
    return current + speed * (target - current);
}

void load_icon(appinfo *info) {
    if (info->icon.texture)
        return;

    info->icon.texture = vita2d_load_PNG_file(info->iconpath);

    if (!info->icon.texture) {
        extern unsigned char _binary_res_noicon_png_start;
        unload_icon(info);
        info->icon.texture = vita2d_load_PNG_buffer(&_binary_res_noicon_png_start);
    }

    if (info->icon.texture) {
        circle_mask(info->icon.texture);
    }
}

void unload_icon(appinfo *info) {
    if (info->icon.buf) {
        free(info->icon.buf);
        info->icon.buf = NULL;
    }

    if (info->icon.texture) {
        vita2d_free_texture(info->icon.texture);
        info->icon.texture = NULL;
    }
}