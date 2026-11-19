#include "utils.h"
#include "save_override.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

typedef struct {
    char title_id[16];    /* ux0:app/<title_id> */
    char name[64];
    char save_dir[192];
    char filter[64];      /* empty = whole dir */
} override_entry;

#define SOVR_MAX          256
#define SOVR_CFG_VERSION  1

#define SOVR_DIR          "ux0:user/00/savedata/" SAVE_MANAGER
#define SOVR_CFG_PATH     SOVR_DIR "/sovr_list.txt"
#define SOVR_CFG_BAK      SOVR_DIR "/sovr_list.txt.bak"
#define SOVR_VER_TAG      "#v="

static override_entry SOVR_APPS[SOVR_MAX];
static int  sovr_count  = 0;
static int  sovr_loaded = 0;
static char sovr_buf[16 * 1024];

static const char *SOVR_DEFAULT_CFG =
    SOVR_VER_TAG "1\n"
    "# title_id|name|save_dir|filter(optional, leave blank if whole dir. case-sensitive)\n"
    "ACGC00001|Animal Crossing|ux0:data/AnimalCrossing/saves|\n"
    "GTA000003|GTA III|ux0:data/gta3|GTA3sf?.b\n"
    "GTASA0000|GTA: San Andreas|ux0:data/gtasa|GTASAsf?.b\n"
    "GTAVICECT|GTA: Vice City|ux0:data/gtavc|GTAVCsf?.b\n"
    "MH3WII001|Monster Hunter Tri|ux0:data/mh3-wii/saves|\n"
    "NZZMBSPTB|NBA 2K16|ux0:data/nba2k16/internal|\n"
    "NZZMBSPTB|Nazi Zombies: Portable|ux0:data/nzp/nzp|config.cfg\n"
    "PSPEMUCFW|Adrenaline|ux0:pspemu/PSP/SAVEDATA|\n"
    "PSVRGP001|Riptide GP|ux0:data/riptidegp/files|profile\n"
    "RESI00001|Resident Evil 1|ux0:data/RE1/SAVE|\n"
    "SHPC00001|Silent Hill|ux0:data/silenthill/gamedata/save|\n"
    "SSXV00001|SSX (2012)|ux0:data/ssx/files|\n"
    "THUG00001|Tony Hawk's Underground|ux0:data/thug/save|\n"
    "TSUKI0001|Tsukihime Vita|ux0:data/tsukihime/save|save?.dat\n";

static char *sovr_trim(char *s) {
    while (*s == ' ' || *s == '\t' || *s == '\r') ++s;
    char *e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r'))
        *--e = 0;
    return s;
}

static int sovr_glob(const char *p, const char *s) {
    const char *star = NULL, *mark = s;

    while (*s) {
        if (*p == '*') {
            star = p++;
            mark = s;
        } else if (*p == '?' ||
                   tolower((unsigned char)*p) == tolower((unsigned char)*s)) {
            ++p; ++s;
        } else if (star) {
            p = star + 1;
            s = ++mark;
        } else {
            return 0;
        }
    }
    while (*p == '*') ++p;
    return *p == 0;
}

static int sovr_match(const char *filter, const char *name) {
    if (strpbrk(filter, "*?"))
        return sovr_glob(filter, name); /* wildcard: full-name match */
    return strncmp(name, filter, strlen(filter)) == 0; /* plain: prefix match */
}

static int sovr_read_file(void) {
    SceUID fd = sceIoOpen(SOVR_CFG_PATH, SCE_O_RDONLY, 0);
    if (fd < 0) return -1;
    int len = sceIoRead(fd, sovr_buf, sizeof(sovr_buf) - 1);
    sceIoClose(fd);
    if (len < 0) return -1;
    sovr_buf[len] = 0;
    return len;
}

static int sovr_file_version(void) {
    const char *p = sovr_buf;
    if ((unsigned char)p[0] == 0xEF && (unsigned char)p[1] == 0xBB &&
        (unsigned char)p[2] == 0xBF)
        p += 3;
    if (strncmp(p, SOVR_VER_TAG, strlen(SOVR_VER_TAG)) != 0) return 0;
    return atoi(p + strlen(SOVR_VER_TAG));
}

static void sovr_write_default(int keep_backup) {
    if (keep_backup) {
        sceIoRemove(SOVR_CFG_BAK);
        sceIoRename(SOVR_CFG_PATH, SOVR_CFG_BAK);
    }

    SceUID fd = sceIoOpen(SOVR_CFG_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fd < 0) return;
    sceIoWrite(fd, SOVR_DEFAULT_CFG, strlen(SOVR_DEFAULT_CFG));
    sceIoClose(fd);
}

static void sovr_load(void) {
    if (sovr_loaded) return;
    sovr_loaded = 1;

    int len = sovr_read_file();

    if (len < 0) {
        sovr_write_default(0);
        len = sovr_read_file();
    } else if (len == 0 || sovr_file_version() < SOVR_CFG_VERSION) {
        sovr_write_default(1);
        len = sovr_read_file();
    }
    if (len <= 0) return;

    char *save = NULL;
    for (char *line = strtok_r(sovr_buf, "\n", &save);
         line && sovr_count < SOVR_MAX;
         line = strtok_r(NULL, "\n", &save))
    {
        char *s = sovr_trim(line);
        if (!s[0] || s[0] == '#') continue;

        char *fld[4] = { s, "", "", "" };
        int n = 0;
        char *p = s;
        while (n < 3 && (p = strchr(p, '|'))) {
            *p++ = 0;
            fld[++n] = p;
        }
        if (n < 2) continue;

        char *id  = sovr_trim(fld[0]);
        char *nm  = sovr_trim(fld[1]);
        char *dir = sovr_trim(fld[2]);
        char *flt = sovr_trim(fld[3]);
        if (!id[0] || !dir[0]) continue;

        override_entry *e = &SOVR_APPS[sovr_count++];
        snprintf(e->title_id, sizeof(e->title_id), "%s", id);
        snprintf(e->name,     sizeof(e->name),     "%s", nm[0] ? nm : id);
        snprintf(e->save_dir, sizeof(e->save_dir), "%s", dir);
        snprintf(e->filter,   sizeof(e->filter),   "%s", flt);

        /* strip trailing slash in dirs if it exists */
        size_t dl = strlen(e->save_dir);
        while (dl > 1 && e->save_dir[dl - 1] == '/' && e->save_dir[dl - 2] != ':')
            e->save_dir[--dl] = 0;
    }
}

static const override_entry *override_find(const char *id) {
    if (!id) return NULL;
    sovr_load();
    for (int i = 0; i < sovr_count; ++i)
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
    return (e && e->filter[0]) ? e->filter : NULL;
}


int sovr_copy_dir(const char *title_id, const char *src, const char *dest,
                  int (*cb)(int*, int, const char*), int *curr, int max, int decrypted)
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
        if (!sovr_match(filter, ent.d_name)) continue;

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
        if (!sovr_match(filter, ent.d_name)) continue;
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
        if (!sovr_match(filter, ent.d_name)) continue;

        char full[256];
        snprintf(full, sizeof(full), "%s/%s", dir, ent.d_name);
        sceIoRemove(full);
    } while (r > 0);

    sceIoDclose(dfd);
    return 1;
}

static void sovr_add_entry(struct applist *list, const override_entry *e, int is_installed)
{
    appinfo *tmp = list->items;
    while (tmp && strcmp(tmp->title_id, e->title_id) != 0) tmp = tmp->next;
    if (tmp) return;

    appinfo *info = calloc(1, sizeof(appinfo));
    if (!info) return;

    info->is_installed = is_installed;
    snprintf(info->title_id, sizeof(info->title_id), "%s", e->title_id);
    snprintf(info->real_id,  sizeof(info->real_id),  "%s", e->title_id);
    snprintf(info->title,    sizeof(info->title),    "%s", e->name);
    snprintf(info->region,   sizeof(info->region),   "%s", "HB");
    snprintf(info->iconpath, sizeof(info->iconpath),
             "ux0:app/%s/sce_sys/icon0.png", e->title_id);

    if (list->count == 0) {
        list->items = list->tail = info;
    } else {
        list->tail->next = info;
        info->prev = list->tail;
        list->tail = info;
    }
    ++list->count;
}

void sovr_register_all(struct applist *list) {
    sovr_load();

    char app_path[64];
    signed char state[SOVR_MAX];   /* -1 skip, 0 not installed, 1 installed */

    for (int i = 0; i < sovr_count; ++i) {
        const override_entry *e = &SOVR_APPS[i];
        state[i] = -1;
        if (!path_exists((char *)e->save_dir)) continue;
        snprintf(app_path, sizeof(app_path), "ux0:app/%s", e->title_id);
        state[i] = path_exists(app_path) ? 1 : 0;
    }

    for (int i = 0; i < sovr_count; ++i)
        if (state[i] == 1) sovr_add_entry(list, &SOVR_APPS[i], 1);
    for (int i = 0; i < sovr_count; ++i)
        if (state[i] == 0) sovr_add_entry(list, &SOVR_APPS[i], 0);
}
