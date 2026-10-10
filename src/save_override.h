#pragma once
#define sovr_skip_pfs(id) is_sovr(id)

struct applist;

void sovr_register_all(applist *list);

int is_sovr    (const char *title_id);
const char *sovr_dir (const char *title_id);
const char *sovr_filter   (const char *title_id);

int sovr_count_files(const char *title_id, const char *dir, int decrypted);

int sovr_copy_dir(const char *title_id, const char *src, const char *dest, int (*cb)(int*, int, const char*), int *curr, int max, int decrypted);

int sovr_format_dir(const char *title_id, const char *dir);