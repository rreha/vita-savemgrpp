#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include "file.h"

const char *const blacklists[] = {
    "sce_pfs/",
    "sce_sys/safemem.dat",
    "sce_sys/keystone",
    "sce_sys/sealedkey",
    NULL,
};

int path_exists(const char *path) {
	SceIoStat stat = {0};
	return sceIoGetstat(path, &stat) >= 0;
}

static int is_dir(const char *path) {
	SceIoStat stat = {0};
	if (sceIoGetstat(path, &stat) < 0) return 0;
	return SCE_S_ISDIR(stat.st_mode);
}

int create_dir(const char *path, int mode) {
    if (is_dir(path)) return 1;

    char npath[MAX_PATH_LENGTH] = {0};
    int len = strlen(path);
    if (len >= sizeof(npath)) return 0;

    int start = 0;
    int ret;

    for (int i = 0; i < len; ++i) {
        npath[i] = path[i];
        if (start < 2 && path[i] == ':') start = 1;
        if (path[i] != '/' || !start || is_dir(npath)) continue;
        ret = sceIoMkdir(npath, mode);
        if (ret < 0 && ret != SCE_ERROR_ERRNO_EEXIST) return 0;
    }

    ret = sceIoMkdir(path, mode);

    if (ret < 0 && ret != SCE_ERROR_ERRNO_EEXIST) return 0;

    return 1;
}

int remove_dir_recursive(const char *path, void (*callback)(int*, int), int *curr, int max) {
    SceUID dfd = sceIoDopen(path);

    if (dfd < 0) {
        int ret = sceIoRemove(path);
        if (ret < 0) return ret;
        return 1;
    }

    int res = 0;
    char new_path[MAX_PATH_LENGTH];
    
    do {
        SceIoDirent dir;
        memset(&dir, 0, sizeof(SceIoDirent));

        res = sceIoDread(dfd, &dir);

        if (res <= 0 || strcmp(dir.d_name, ".") == 0 || strcmp(dir.d_name, "..") == 0)
            continue;

        snprintf(new_path, sizeof(new_path), "%s/%s", path, dir.d_name);

        if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
            int ret = remove_dir_recursive(new_path, callback, curr, max);

            if (ret <= 0) {
                sceIoDclose(dfd);
                return ret;
            }

        } else {
            int ret = sceIoRemove(new_path);
            if (callback)
                callback(curr, max);

            if (ret < 0) {
                sceIoDclose(dfd);
                return ret;
            }
        }
    } while (res > 0);

    sceIoDclose(dfd);

    int ret = sceIoRmdir(path);
    if (ret < 0)
        return ret;

    return 1;
}

int copy_file(const char *src, const char *dest, int check_blacklist) {
    if (strcasecmp(src, dest) == 0) return 1;
    else if (!path_exists(src)) return 0;

    int i = 0;
    while (check_blacklist && blacklists[i]) {
        if (strstr(dest, blacklists[i]))
            return 2;
        ++i;
    }

    int ignore_error = strncmp(dest, "savedata0:", 10) == 0;
    void *buf = memalign(DIRECTORY_SIZE, TRANSFER_SIZE);
    if (!buf) return 0;

    SceUID fsrc = sceIoOpen(src, SCE_O_RDONLY, 0);
    SceUID fdst = sceIoOpen(dest, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777); 
    
    int read = 0, written = 0;
    SceIoStat stat;

    if (!ignore_error && (fsrc < 0 || fdst < 0)) {
        free(buf);
        if (fsrc >= 0) sceIoClose(fsrc);
        if (fdst >= 0) sceIoClose(fdst);
        return 0;
    }

    while (1) {
        read = sceIoRead(fsrc, buf, TRANSFER_SIZE);
        if (read < 0) {
            free(buf);
            if (fsrc >= 0) sceIoClose(fsrc);
            if (fdst >= 0) sceIoClose(fdst);
            sceIoRemove(dest);
            return 0;
        } else if (read == 0) {
            break;
        }

        written = sceIoWrite(fdst, buf, read);
        if (written < 0 || written != read) {
            free(buf);
            if (fsrc >= 0) sceIoClose(fsrc);
            if (fdst >= 0) sceIoClose(fdst);
            sceIoRemove(dest);
            return 0;
        }
    }

    free(buf);

    memset(&stat, 0, sizeof(SceIoStat));
    if (sceIoGetstat(src, &stat) >= 0) {
        sceIoChstat(dest, &stat, 0x3B);
    }

    if (fsrc >= 0) sceIoClose(fsrc);
    if (fdst >= 0) sceIoClose(fdst);

    return 1;
}

int copy_dir_recursive(const char *src, const char *dest, void (*callback)(int*, int), int *curr, int max, int check_blacklist) {
    if (strcasecmp(src, dest) == 0) return 1;

    int i = 0;
    while (check_blacklist && blacklists[i]) {
        if (strstr(dest, blacklists[i]))
            return 2;
        ++i;
    }

    SceUID dfd = sceIoDopen(src);
    int res = 0, ret = 0;

    if (dfd < 0) return copy_file(src, dest, check_blacklist); // not a folder?

    if (!path_exists(dest)) {
        ret = create_dir(dest, 0777);
        if (!ret) {
            sceIoDclose(dfd);
            return 0;
        }
    }

    char new_src[MAX_PATH_LENGTH];
    char new_dst[MAX_PATH_LENGTH];

    do {
        SceIoDirent dir;
        memset(&dir, 0, sizeof(SceIoDirent));

        res = sceIoDread(dfd, &dir);
        if (res <= 0 || strcmp(dir.d_name, ".") == 0 || strcmp(dir.d_name, "..") == 0)
            continue;

        snprintf(new_src, sizeof(new_src), "%s/%s", src, dir.d_name);
        snprintf(new_dst, sizeof(new_dst), "%s/%s", dest, dir.d_name);

        if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
            ret = copy_dir_recursive(new_src, new_dst, callback, curr, max, check_blacklist);
        } else {
            ret = copy_file(new_src, new_dst, check_blacklist);
            if (callback)
                callback(curr, max);
        }

        if (ret == 0) { // copy_file failed
            sceIoDclose(dfd);
            return 0;
        }
    } while (res > 0);

    sceIoDclose(dfd);
    return 1;
}

int count_files(const char *path, int check_blacklist) {
    if (!path_exists(path)) return 0;

	int i = 0;
	while (check_blacklist && blacklists[i]) {
		if (strstr(path, blacklists[i]))
			return 0;

		++i;
	}

    SceUID dfd = sceIoDopen(path);
    if (dfd < 0) return 0;

    int cnt = 0, res = 0;
    char new_path[MAX_PATH_LENGTH];

    do {
        SceIoDirent dir;
        memset(&dir, 0, sizeof(SceIoDirent));

        res = sceIoDread(dfd, &dir);

        if (res <= 0 || strcmp(dir.d_name, ".") == 0 || strcmp(dir.d_name, "..") == 0)
            continue;

        snprintf(new_path, sizeof(new_path), "%s/%s", path, dir.d_name);

        if (SCE_S_ISDIR(dir.d_stat.st_mode))
            cnt += count_files(new_path, check_blacklist);
        else
            ++cnt;
    } while (res > 0);

    sceIoDclose(dfd);
    return cnt;
}

int count_folders(const char *path) {
	if (!path_exists(path)) return -1;

	SceUID dfd = sceIoDopen(path);
    if (dfd < 0) return 0;

    int cnt = 0, res = 0;

	do {
        SceIoDirent dir;
        memset(&dir, 0, sizeof(SceIoDirent));

        res = sceIoDread(dfd, &dir);

        if (res <= 0 || strcmp(dir.d_name, ".") == 0 ||
				strcmp(dir.d_name, "..") == 0 || !SCE_S_ISDIR(dir.d_stat.st_mode))
            continue;

        ++cnt;
    } while (res > 0);

	sceIoDclose(dfd);
	return cnt;
}