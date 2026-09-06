#pragma once

#include <psp2/io/stat.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/dirent.h>

#define SCE_ERROR_ERRNO_EEXIST (int)(0x80010011)
#define MAX_PATH_LENGTH 1024
#define DIRECTORY_SIZE (4 * 1024)
#define TRANSFER_SIZE (128 * 1024)

int path_exists(const char *path);
int create_dir(const char *path, int mode);
int remove_dir_recursive(const char *path, void (*callback)(int*, int), int *curr, int max);
int copy_file(const char *src, const char *dest, int check_blacklist);
int copy_dir_recursive(const char *src, const char *dest, void (*callback)(int*, int), int *curr, int max, int check_blacklist);
int count_files(const char *path, int check_blacklist);
int count_folders(const char *path);
