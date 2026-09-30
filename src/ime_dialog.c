// vitasdk sample (yne)
#include <string.h>
#include <stdlib.h>
#include <psp2/ime_dialog.h>
#include <vita2d.h>

#define ALIGN(x, a)                 (((x) + ((a) - 1)) & ~((a) - 1))
#define DISPLAY_WIDTH               960
#define DISPLAY_HEIGHT              544
#define DISPLAY_STRIDE_IN_PIXELS    1024
#define DISPLAY_BUFFER_COUNT        2
#define DISPLAY_MAX_PENDING_SWAPS   1

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

char *showImeDialog(const char *initialText) {
    uint16_t initialText_utf16[SCE_IME_DIALOG_MAX_TEXT_LENGTH] = {0};
    uint16_t input[10 + 1] = {0};
    SceImeDialogParam param;
    char *ret = NULL;

    utf8_to_utf16((const uint8_t *)initialText, initialText_utf16);

    sceImeDialogParamInit(&param);
    param.supportedLanguages = SCE_IME_LANGUAGE_ENGLISH;
    param.languagesForced = SCE_TRUE;
    param.type = SCE_IME_TYPE_DEFAULT;
    param.option = 0;
    param.textBoxMode = SCE_IME_DIALOG_TEXTBOX_MODE_DEFAULT;
    param.title = u"New Title ID";
    param.maxTextLength = 10;
    param.initialText = initialText_utf16;
    param.inputTextBuffer = input;

    if (sceImeDialogInit(&param) < 0)
        return NULL;

    while (sceImeDialogGetStatus() != SCE_COMMON_DIALOG_STATUS_FINISHED) {
        vita2d_start_drawing();
        vita2d_clear_screen();
        vita2d_end_drawing();
        vita2d_common_dialog_update();
        vita2d_swap_buffers();
    }

    SceImeDialogResult result;
    memset(&result, 0, sizeof(result));
    sceImeDialogGetResult(&result);

    if (result.button == SCE_IME_DIALOG_BUTTON_ENTER) {
        ret = malloc(10 * 3 + 1);          // worst case UTF-8 size
        if (ret) utf16_to_utf8(input, (uint8_t *)ret);
    }

    sceImeDialogTerm();
    return ret;
}