// Runs the production Latin textbox color arms with real engine structures.
// Only the ordinary OoT palette setter is a renderer service boundary.
#include "global.h"
#include "ComboBossSoulColor.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#ifdef RECEIPT_MM
#include "message_data_fmt_nes.h"
#else
#include "message_data_fmt.h"
#endif

SaveContext gSaveContext;
static PlayState play;
int32_t CVarGetInteger(const char* key, int32_t fallback) { (void)key; return fallback; }
#ifndef RECEIPT_MM
static void Message_SetTextColor(MessageContext* msgCtx, u16 color) {
    msgCtx->textColorR = color == 2 ? 70 : color == 1 ? 255 : 255;
    msgCtx->textColorG = color == 2 ? 255 : color == 1 ? 60 : 255;
    msgCtx->textColorB = color == 2 ? 80 : color == 1 ? 60 : 255;
}
#endif
/* PRODUCTION_COLOR_ARMS */

static uint32_t RenderName(const char* name, int color, int page) {
    MessageContext* msgCtx = &play.msgCtx;
    const size_t length = strlen(name);
#ifdef RECEIPT_MM
    memset(msgCtx->decodedBuffer.schar, 0xFF, sizeof(msgCtx->decodedBuffer.schar));
    msgCtx->decodedBuffer.schar[0] = color;
    memcpy(msgCtx->decodedBuffer.schar + 1, name, length);
    msgCtx->decodedBuffer.schar[length + 1] = page ? 0x10 : 0;
    msgCtx->decodedBuffer.schar[length + 2] = 0;
    msgCtx->textDrawPos = length + 3;
    RenderColor(&play, color);
#else
    memset(msgCtx->msgBufDecoded, 0xFF, sizeof(msgCtx->msgBufDecoded));
    msgCtx->msgBufDecoded[0] = 5;
    msgCtx->msgBufDecoded[1] = 0x40 | color;
    memcpy(msgCtx->msgBufDecoded + 2, name, length);
    msgCtx->msgBufDecoded[length + 2] = page ? 4 : 5;
    msgCtx->msgBufDecoded[length + 3] = 0x40;
    msgCtx->textDrawPos = length + 4;
    RenderColor(&play, 5);
#endif
    return (uint32_t)msgCtx->textColorR << 16 | (uint32_t)msgCtx->textColorG << 8 | msgCtx->textColorB;
}

int main(void) {
    static const struct { const char* boss; uint32_t rgb; } souls[] = {
        { "Goht", 0x0A8A2E }, { "Gyorg", 0x1363A5 }, { "Majora", 0xE88015 },
        { "Odolwa", 0x911485 }, { "Twinmold", 0xA8B414 }
    };
    static const char* prefixes[] = { "Soul of ", "Seele von ", "\x82" "me de " };
    for (size_t p = 0; p < 3; ++p) {
        for (size_t s = 0; s < 5; ++s) {
            char name[48];
            snprintf(name, sizeof(name), "%s%s", prefixes[p], souls[s].boss);
            assert(RenderName(name, 2, 0) == souls[s].rgb && "soul name lost the exact native flame RGB");
            assert(RenderName(name, 2, 1) == 0x46FF50 && "soul tint crossed a page control");
        }
    }
#ifdef RECEIPT_MM
    assert(RenderName("Soul of\x11Goht", 2, 0) == 0x0A8A2E);
#else
    assert(RenderName("Soul of\x01Goht", 2, 0) == 0x0A8A2E);
#endif
    assert(RenderName("Soul of Goht Jr", 2, 0) == 0x46FF50);
    assert(RenderName("ordinary body", 2, 0) == 0x46FF50);
    assert(RenderName("Goht's Remains", 1, 0) == 0xFF3C3C);
    puts("Exact soul RGB reaches EN/DE/FR name spans only; native palette and Goht Remains stay native");
}
