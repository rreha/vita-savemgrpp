// vitasdk sample (yne)
#include <string.h>
#include <stdlib.h>

#include <psp2/types.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/message_dialog.h>
#include <psp2/ime_dialog.h>
#include <psp2/display.h>
#include <psp2/gxm.h>
#include <psp2/kernel/sysmem.h>

#define ALIGN(x, a)                 (((x) + ((a) - 1)) & ~((a) - 1))
#define DISPLAY_WIDTH               960
#define DISPLAY_HEIGHT              544
#define DISPLAY_STRIDE_IN_PIXELS    1024
#define DISPLAY_BUFFER_COUNT        2
#define DISPLAY_MAX_PENDING_SWAPS   1

typedef struct{
    void *data;
    SceGxmSyncObject*sync;
    SceGxmColorSurface surf;
    SceUID uid;
} displayBuffer;

static unsigned int backBufferIndex = 0;
static unsigned int frontBufferIndex = 0;
displayBuffer dbuf[DISPLAY_BUFFER_COUNT];

// VitaShell code
static void utf16_to_utf8(const uint16_t *src, uint8_t *dst) {
    int i;

    for (i = 0; src[i]; ++i) {
        if ((src[i] & 0xFF80) == 0) {
            *(dst++) = src[i] & 0xFF;

        } else if((src[i] & 0xF800) == 0) {
            *(dst++) = ((src[i] >> 6) & 0xFF) | 0xC0;
            *(dst++) = (src[i] & 0x3F) | 0x80;

        } else if((src[i] & 0xFC00) == 0xD800 && (src[i + 1] & 0xFC00) == 0xDC00) {
            *(dst++) = (((src[i] + 64) >> 8) & 0x3) | 0xF0;
            *(dst++) = (((src[i] >> 2) + 16) & 0x3F) | 0x80;
            *(dst++) = ((src[i] >> 4) & 0x30) | 0x80 | ((src[i + 1] << 2) & 0xF);
            *(dst++) = (src[i + 1] & 0x3F) | 0x80;
            ++i;

        } else {
            *(dst++) = ((src[i] >> 12) & 0xF) | 0xE0;
            *(dst++) = ((src[i] >> 6) & 0x3F) | 0x80;
            *(dst++) = (src[i] & 0x3F) | 0x80;
        }
    }

    *dst = '\0';
}

// VitaShell code
static void utf8_to_utf16(const uint8_t *src, uint16_t *dst) {
    int i;

    for (i = 0; src[i];) {
        if ((src[i] & 0xE0) == 0xE0) {
            *(dst++) = ((src[i] & 0x0F) << 12) | ((src[i + 1] & 0x3F) << 6) | (src[i + 2] & 0x3F);
            i += 3;

        } else if ((src[i] & 0xC0) == 0xC0) {
            *(dst++) = ((src[i] & 0x1F) << 6) | (src[i + 1] & 0x3F);
            i += 2;

        } else {
            *(dst++) = src[i];
            ++i;
        }
    }

    *dst = '\0';
}

static void *dram_alloc(unsigned int size, SceUID *uid){
    void *mem = NULL;

    *uid = sceKernelAllocMemBlock("gpu_mem", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, ALIGN(size,256*1024), NULL);

    sceKernelGetMemBlockBase(*uid, &mem);
    sceGxmMapMemory(mem, ALIGN(size,256*1024), SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE);

    return mem;
}

static void gxm_init(){
    unsigned int i;

    for (i=0; i<DISPLAY_BUFFER_COUNT; ++i) {
        dbuf[i].data = dram_alloc(4 * DISPLAY_STRIDE_IN_PIXELS*DISPLAY_HEIGHT, &dbuf[i].uid);

        sceGxmColorSurfaceInit(&dbuf[i].surf, SCE_GXM_COLOR_FORMAT_A8B8G8R8,
                                SCE_GXM_COLOR_SURFACE_LINEAR, SCE_GXM_COLOR_SURFACE_SCALE_NONE,
                                SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT, DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                DISPLAY_STRIDE_IN_PIXELS,dbuf[i].data);
        sceGxmSyncObjectCreate(&dbuf[i].sync);
	}

    return;
}

static void gxm_swap(){
    sceGxmPadHeartbeat(&dbuf[backBufferIndex].surf, dbuf[backBufferIndex].sync);
    sceGxmDisplayQueueAddEntry(dbuf[frontBufferIndex].sync, dbuf[backBufferIndex].sync,
                                &dbuf[backBufferIndex].data);

    frontBufferIndex = backBufferIndex;
    backBufferIndex = (backBufferIndex + 1) % DISPLAY_BUFFER_COUNT;

    return;
}

static void gxm_term(){
    sceKernelFreeMemBlock(dbuf[0].uid);
    sceKernelFreeMemBlock(dbuf[1].uid);

    return;
}

char *showImeDialog(const char *initialText) {
    uint16_t initialText_utf16[SCE_IME_DIALOG_MAX_TEXT_LENGTH];
    uint16_t input[10 + 1] = {0};
    SceImeDialogParam param;
    char *ret = NULL;
    int res;

    sceCommonDialogSetConfigParam(&(SceCommonDialogConfigParam){});
    utf8_to_utf16((uint8_t *)initialText, initialText_utf16);

    gxm_init();

    sceImeDialogParamInit(&param);

    param.supportedLanguages = SCE_IME_LANGUAGE_ENGLISH;
    param.languagesForced = SCE_TRUE;
    param.type = SCE_IME_DIALOG_TEXTBOX_MODE_DEFAULT;
    param.option = 0;
    param.textBoxMode = SCE_IME_DIALOG_TEXTBOX_MODE_DEFAULT;
    param.title = u"New Title ID";
    param.maxTextLength = 10;
    param.initialText = initialText_utf16;
    param.inputTextBuffer = input;

    res = sceImeDialogInit(&param);
    if (res < 0)
        return NULL;

    while (1) {
        SceCommonDialogStatus status = sceImeDialogGetStatus();

        memset(dbuf[backBufferIndex].data, 0xff000000, DISPLAY_HEIGHT * DISPLAY_STRIDE_IN_PIXELS * 4);

        if (status == SCE_COMMON_DIALOG_STATUS_FINISHED) {
            SceImeDialogResult result;

            memset(&result, 0, sizeof(SceImeDialogResult));
            sceImeDialogGetResult(&result);

            if (result.button == SCE_IME_DIALOG_BUTTON_ENTER) {
                uint8_t *tmp = malloc(sizeof(uint8_t) * 11);

                utf16_to_utf8(input, tmp);
                ret = (char *)tmp;
            }

            sceImeDialogTerm();
            break;
        }

        sceCommonDialogUpdate(&(SceCommonDialogUpdateParam) {
            {
                NULL, dbuf[backBufferIndex].data, 0, 0,
                DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_STRIDE_IN_PIXELS
            },
            dbuf[backBufferIndex].sync});

        gxm_swap();
        sceDisplayWaitVblankStart();
    }

    gxm_term();

    return ret;
}