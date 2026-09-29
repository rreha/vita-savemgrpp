/*
 * below codes use part of vitashell codeset
*/
#include <stdint.h>

#include <psp2/registrymgr.h>
#include <psp2/appmgr.h>
#include <vitashell_user.h>
#include <psp2/kernel/modulemgr.h>
#include <taihen.h>

#include "common.h"
#include "file.h"

extern SceUID _vshKernelSearchModuleByName(const char *module_name, int *unk);

#define SFO_MAGIC 0x46535000 // \x00PSF
#define MAX_MOUNT_POINT_LENGTH 16

struct sfo_header {
    uint32_t magic;
    uint32_t version;
    uint32_t key_table_offset;
    uint32_t data_table_offset;
    uint32_t entries;
};

struct sfo_index {
    uint16_t key_offset;
    uint16_t param_format;
    uint32_t param_length;
    uint32_t param_max_length;
    uint32_t data_offset;
};

uint8_t g_aid_loaded = 0;
uint64_t g_aid;

/*
  SceAppMgr mount IDs:
  0x64: ux0:picture
  0x65: ur0:user/00/psnfriend
  0x66: ur0:user/00/psnmsg
  0x69: ux0:music
  0x6E: ux0:appmeta
  0xC8: ur0:temp/sqlite
  0xCD: ux0:cache
  0x12E: ur0:user/00/trophy/data/sce_trop
  0x12F: ur0:user/00/trophy/data
  0x3E8: ux0:app, vs0:app, gro0:app
  0x3E9: ux0:patch
  0x3EB: ?
  0x3EA: ux0:addcont
  0x3EC: ux0:theme
  0x3ED: ux0:user/00/savedata
  0x3EE: ur0:user/00/savedata
  0x3EF: vs0:sys/external
  0x3F0: vs0:data/external
*/
char pfs_mount_point[MAX_MOUNT_POINT_LENGTH];
int known_pfs_ids[] = {
  0x6E,
  0x12E,
  0x12F,
  0x3ED,
};

// Thanks Princess-of-Sleeping
int load_modules(char **error) {
    SceUID search_modid, kern_modid, user_modid;
    size_t len = sizeof(char) * 128;
    char module_path[60] = {0};
    int search_unk[2];
    int res = 0;

    // Load kernel module
    search_modid = _vshKernelSearchModuleByName("VitaShellKernel2", search_unk);
    if(search_modid < 0) {
        snprintf(module_path, len, "ux0:VitaShell/module/kernel.skprx");
        if (!path_exists(module_path)) {
            snprintf(module_path, len, "ux0:"SAVEMGR_FOLDER"/kernel.skprx");
            
            if (!path_exists(module_path)) {
                *error = malloc(len);
                if (*error) snprintf(*error, len, "Kernel module not found!");
                return 0;
            }
        }

        kern_modid = taiLoadKernelModule(module_path, 0, NULL);
        if (kern_modid >= 0) {
            res = taiStartKernelModule(kern_modid, 0, NULL, 0, NULL, NULL);

            if (res < 0)
                taiStopUnloadKernelModule(kern_modid, 0, NULL, 0, NULL, NULL);
        }

        if (kern_modid < 0 || res < 0) {
            *error = malloc(len);
            if (*error)
                snprintf(*error, len, "Kernel module load error: %x\nPlease reboot.", kern_modid);

            return 0;
        }
    }

    // Load user module
    snprintf(module_path, len, "ux0:VitaShell/module/user.suprx");

    if (!path_exists(module_path)) {
        snprintf(module_path, len, "ux0:"SAVEMGR_FOLDER"/user.suprx");

        if (!path_exists(module_path)) {
            *error = malloc(len);
            if (*error) snprintf(*error, len, "User module not found!");
            return 0;
        }
    }

    user_modid = sceKernelLoadStartModule(module_path, 0, NULL, 0, NULL, NULL);
    if (user_modid < 0) {
        *error = malloc(len);
        if (*error)
            snprintf(*error, len, "User module load error: %x\nPlease reboot.", user_modid);

        return 0;
    }

    return 1;
}

uint64_t get_accountid() {
    if (g_aid_loaded)
        return g_aid;
    else if (sceRegMgrGetKeyBin("/CONFIG/NP", "account_id", &g_aid, sizeof(uint64_t)) < 0)
        return 0;

    g_aid_loaded = 1;

    return g_aid;
}

uint64_t get_accountid_sfo(const char *sfo_path) {
    uint32_t data_offset = -1;
    struct sfo_header hdr = {0};
    uint64_t aid;
    FILE *f = NULL;

    f = fopen(sfo_path, "r+b");
    if (f == NULL) {
        //printf("cannot open sfo file: %s\n", sfo_path);
        return -1;
    }

    fread(&hdr, sizeof(struct sfo_header), 1, f);

    if (hdr.magic != SFO_MAGIC) {
        //printf("magic mismatch\n");
        fclose(f);
        return -1;
    }

    for (int i=0; i < hdr.entries; ++i) {
        struct sfo_index idx = {0};
        char key[64];

        fseek(f, sizeof(struct sfo_header) + sizeof(struct sfo_index) * i, SEEK_SET);
        fread(&idx, sizeof(struct sfo_index), 1, f);

        fseek(f, hdr.key_table_offset + idx.key_offset, SEEK_SET);
        fread(&key, sizeof(char), 64, f);

        if (strncmp(key, "ACCOUNT_ID", 10) != 0)
            continue;

        data_offset = hdr.data_table_offset + idx.data_offset;
        break;
    }

    if (data_offset == -1) {
        //printf("not exist ACCOUNT_ID\n");
        fclose(f);
        return -2;
    }

    fseek(f, data_offset, SEEK_SET);
    fread(&aid, sizeof(uint64_t), 1, f);
    fclose(f);

    return aid;
}

int8_t change_accountid(const char *sfo_path, const uint64_t aid) {
    uint32_t data_offset = -1;
    struct sfo_header hdr = {0};
    uint64_t old_aid;
    FILE *f = NULL;

    f = fopen(sfo_path, "r+b");
    if (f == NULL)
        return -1;

    fread(&hdr, sizeof(struct sfo_header), 1, f);

    if (hdr.magic != SFO_MAGIC) {
        //printf("magic mismatch\n");
        fclose(f);
        return -1;
    }

    for (int i=0; i < hdr.entries; ++i) {
        struct sfo_index idx = {0};
        char key[64];

        fseek(f, sizeof(struct sfo_header) + sizeof(struct sfo_index) * i, SEEK_SET);
        fread(&idx, sizeof(struct sfo_index), 1, f);

        fseek(f, hdr.key_table_offset + idx.key_offset, SEEK_SET);
        fread(&key, sizeof(char), 64, f);

        if (strncmp(key, "ACCOUNT_ID", 10) != 0)
            continue;

        data_offset = hdr.data_table_offset + idx.data_offset;
        break;
    }

    if (data_offset == -1) {
        //printf("not exist ACCOUNT_ID\n");
        fclose(f);
        return -2;
    }

    fseek(f, data_offset, SEEK_SET);
    fread(&old_aid, sizeof(uint64_t), 1, f);

    if (old_aid == aid) {
        fclose(f);
        //printf("aid is already same\n");
        return 1;
    }

    //printf("replace ACCOUNT_ID - %08x to %08x\n", old_aid, aid);

    fseek(f, data_offset, SEEK_SET);
    fwrite(&aid, sizeof(uint64_t), 1, f);
    fclose(f);

    return 0;
}

int change_titleid(const char *sfo_path, const char *new_titleID) {
    struct sfo_header hdr = {0};
    FILE *f = NULL;

    f = fopen(sfo_path, "r+b");
    if (f == NULL)
        return -1;

    fread(&hdr, sizeof(struct sfo_header), 1, f);

    if (hdr.magic != SFO_MAGIC) {
        //printf("magic mismatch\n");
        fclose(f);
        return -1;
    }

    fseek(f, 0x51C, SEEK_SET);
    fwrite(new_titleID, sizeof(char), 9, f);

    fseek(f, 0x528, SEEK_SET);
    fwrite(new_titleID, sizeof(char), 9, f);

    fseek(f, 0x8F5, SEEK_SET);
    fwrite(new_titleID, sizeof(char), 9, f);

    fclose(f);

    return 1;
}

int pfs_mount(const char *path) {
    int res;
    char klicensee[0x10];
    ShellMountIdArgs args;

    memset(klicensee, 0, sizeof(klicensee));

    args.process_titleid = SAVE_MANAGER;
    args.path = path;
    args.desired_mount_point = NULL;
    args.klicensee = klicensee;
    args.mount_point = pfs_mount_point;

    for (int i=0; i < sizeof(known_pfs_ids) / sizeof(int); ++i) {
        args.id = known_pfs_ids[i];

        res = shellUserMountById(&args);
        if (res >= 0)
            return res;
    }

    return sceAppMgrGameDataMount(path, 0, 0, pfs_mount_point);
}

int pfs_unmount() {
    if (pfs_mount_point[0] == 0)
        return -1;

    int res = sceAppMgrUmount(pfs_mount_point);
    if (res >= 0)
        memset(pfs_mount_point, 0, sizeof(pfs_mount_point));

    return res;
}
