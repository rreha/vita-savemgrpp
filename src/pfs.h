#pragma once

// below codes use part of vitashell codeset
int load_modules(char **error);
uint64_t get_accountid();
uint64_t get_accountid_sfo(const char *sfo_path);
int8_t change_accountid(const char *sfo_path, const uint64_t aid);
int change_titleid(const char *sfo_path, const char *new_titleID);
int pfs_mount(const char *path);
int pfs_unmount();
