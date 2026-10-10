#include "utils.h"
#include "save_override.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
    const char *title_id;  /* ux0:app/<title_id> */
    const char *name;
    const char *save_dir;
    const char *filter;    /* NULL = whole dir */
} override_entry;

static const override_entry SOVR_APPS[] = {
    {"GTASA0000", "GTA: San Andreas", "ux0:data/gtasa", "GTASA"},
    {"THUG00001", "Tony Hawk's Underground", "ux0:data/thug/save", NULL},
    {NULL, NULL, NULL, NULL}};

static const override_entry *override_find(const char *id) {
    if (!id) return NULL;
    for (int i = 0; SOVR_APPS[i].title_id; ++i)
        if (strcmp(SOVR_APPS[i].title_id, id) == 0)
            return &SOVR_APPS[i];
    return NULL;
}

int is_sovr(const char *id) 
{ 
    return override_find(id) != NULL; 
}

const char *sovr_dir(const char *id)
{
    const override_entry *e = override_find(id);
    return e ? e->save_dir : NULL;
}

const char *sovr_filter(const char *id)
{
    const override_entry *e = override_find(id);
    return e ? e->filter : NULL;
}


int sovr_copy_dir(const char *title_id,
                      const char *src, const char *dest,
                      int (*cb)(int*, int, const char*),
                      int *curr, int max, int decrypted)
{
    const char *filter = sovr_filter(title_id);

    if (!filter || !filter[0])
        return copy_dir_recursive(src, dest, cb, curr, max, decrypted);

    SceUID dfd = sceIoDopen(src);
    if (dfd < 0) return 0;

    size_t flen = strlen(filter);
    SceIoDirent ent;
    int r;

    do {
        memset(&ent, 0, sizeof(ent));
        r = sceIoDread(dfd, &ent);
        if (r <= 0) break;
        if (ent.d_name[0] == '.') continue;
        if (SCE_S_ISDIR(ent.d_stat.st_mode)) continue;
        if (strncmp(ent.d_name, filter, flen) != 0) continue;

        char sf[256], df[256];
        snprintf(sf, sizeof(sf), "%s/%s", src,  ent.d_name);
        snprintf(df, sizeof(df), "%s/%s", dest, ent.d_name);

        copy_file(sf, df, 0);
        if (cb) cb(curr, max, ent.d_name);
    } while (r > 0);

    sceIoDclose(dfd);
    return 1;
}

int sovr_count_files(const char *title_id, const char *dir, int decrypted) {
    const char *filter = sovr_filter(title_id);
    if (!filter || !filter[0])
        return count_files(dir, decrypted);

    SceUID dfd = sceIoDopen(dir);
    if (dfd < 0) return 0;

    size_t flen = strlen(filter);
    SceIoDirent ent;
    int r, count = 0;

    do {
        memset(&ent, 0, sizeof(ent));
        r = sceIoDread(dfd, &ent);
        if (r <= 0) break;
        if (ent.d_name[0] == '.') continue;
        if (SCE_S_ISDIR(ent.d_stat.st_mode)) continue;
        if (strncmp(ent.d_name, filter, flen) != 0) continue;
        ++count;
    } while (r > 0);

    sceIoDclose(dfd);
    return count;
}

int sovr_format_dir(const char *title_id, const char *dir) {
    const char *filter = sovr_filter(title_id);
    if (!filter || !filter[0]) return 0;

    SceUID dfd = sceIoDopen(dir);
    if (dfd < 0) return 1;

    size_t flen = strlen(filter);
    SceIoDirent ent;
    int r;

    do {
        memset(&ent, 0, sizeof(ent));
        r = sceIoDread(dfd, &ent);
        if (r <= 0) break;
        if (ent.d_name[0] == '.') continue;
        if (SCE_S_ISDIR(ent.d_stat.st_mode)) continue;
        if (strncmp(ent.d_name, filter, flen) != 0) continue;

        char full[256];
        snprintf(full, sizeof(full), "%s/%s", dir, ent.d_name);
        sceIoRemove(full);
    } while (r > 0);

    sceIoDclose(dfd);
    return 1;
}

void sovr_register_all(struct applist *list) {
    for (int i = 0; SOVR_APPS[i].title_id; ++i) {
        const override_entry *e = &SOVR_APPS[i];

        if (!path_exists((char *)e->save_dir))
            continue;

        appinfo *tmp = list->items;
        while (tmp && strcmp(tmp->title_id, e->title_id) != 0) tmp = tmp->next;
        if (tmp) continue;

        appinfo *info = calloc(1, sizeof(appinfo));
        if (!info) return;

        info->is_installed = 1;
        snprintf(info->title_id, sizeof(info->title_id), "%s", e->title_id);
        snprintf(info->real_id,  sizeof(info->real_id),  "%s", e->title_id);
        snprintf(info->title,    sizeof(info->title),    "%s", e->name);
        snprintf(info->region, sizeof(info->region), "%s", "HB");
        snprintf(info->iconpath, sizeof(info->iconpath), "ux0:app/%s/sce_sys/icon0.png", e->title_id);

        if (list->count == 0) {
            list->items = list->tail = info;
        } else {
            list->tail->next = info;
            info->prev = list->tail;
            list->tail = info;
        }
        ++list->count;
    }
}
