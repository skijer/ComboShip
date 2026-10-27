// Link the complete production NES decoder. Only font upload, item-icon upload
// and unrelated inventory/engine services are boundaries; header/page/row
// decoding uses the real MessageContext and its three-entry row array.
#include "global.h"
#include "message_data_fmt_nes.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "ComboSwordElementReceipt.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

SaveContext gSaveContext;
static RegEditor registers;
RegEditor* gRegEditor = &registers;
s16 gItemPrices[256];
s16 D_801CFF94[250];
static s16 sMsgStrayFairyIndex = -1;
static PlayState play;
static Player player;
static unsigned glyphUploads, iconUploads;

void Font_LoadCharNES(PlayState* state, u8 character, s32 offset) {
    assert(state == &play && character >= 0x20 && character < 0xC0);
    assert(offset >= 0 && offset + FONT_CHAR_TEX_SIZE <= (s32)sizeof(state->msgCtx.font.charBuf[0]));
    ++glyphUploads;
}
void Message_LoadItemIcon(PlayState* state, u16 itemId, s16 y) {
    (void)state; (void)itemId; (void)y;
    ++iconUploads;
}
s32 Flags_GetAllTreasure(PlayState* state) { (void)state; assert(0); return 0; }
s16 Inventory_GetSkullTokenCount(s16 scene) { (void)scene; assert(0); return 0; }
s32 Map_CurRoomHasMapI(PlayState* state) { (void)state; assert(0); return 0; }
bool GameInteractor_Should(GIVanillaBehavior flag, uint32_t result, ...) { (void)flag; return result; }
void Audio_PlaySfx(u16 id) { (void)id; assert(0); }
int Message_ItemReceiptSpaceWidth(int width) { return width; }

#include "sword_element_native_header.inc"

static void DecodeReceipt(uint8_t element, int custom, int fade, int negative) {
    memset(&play, 0, sizeof(play));
    memset(&player, 0, sizeof(player));
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    MessageContext* ctx = &play.msgCtx;
    ctx->textCharScale = 0.75f;
    ctx->textBoxType = TEXTBOX_TYPE_2;
    ctx->unk11F20[0] = 0x5A;
    ctx->unk11F20[1] = 0x6B;
    uint8_t* buffer = ctx->font.msgBuf.schar;
    const uint8_t header[] = { 2, 0, 7, 255, 255, 0, 37, 255, 255, 255, 255 };
    memcpy(buffer, header, sizeof(header));
    const char* receipt = custom ? "Custom arrow receipt preserved." : "You got the elemental arrows!";
    const size_t receiptLength = strlen(receipt);
    memcpy(buffer + sizeof(header), receipt, receiptLength);
    size_t length = sizeof(header) + receiptLength;
    if (fade) {
        const uint8_t trailer[] = { 0x1C, 2, 0x10 };
        memcpy(buffer + length, trailer, sizeof(trailer));
        length += sizeof(trailer);
    }
    buffer[length++] = 0xBF;
    ctx->msgLength = CwSwordElementReceipt_Append(buffer, length, sizeof(ctx->font.msgBuf.schar), 1, element);
    assert(ctx->msgLength > (s32)length);
    if (negative) {
        // Reproduce the reviewed four-line tutorial in the actual decoder.
        // Its END/FADE row write must be rejected by the bounds sanitizer.
        const size_t at = ctx->msgLength - (fade ? 4 : 1);
        memmove(buffer + at + 1, buffer + at, ctx->msgLength - at);
        buffer[at] = 0x11;
        ++ctx->msgLength;
    }
    glyphUploads = iconUploads = 0;
    Message_DecodeNES(&play);
    assert(ctx->decodedBuffer.schar[ctx->decodedTextLen] == MESSAGE_BOX_BREAK);
    assert(ctx->msgBufPos == sizeof(header) + receiptLength);
    assert(iconUploads == 1 && ctx->unk1206C == 37);
    ++ctx->msgBufPos; // The native next-page transition advances past BOX_BREAK.
    glyphUploads = 0;
    Message_DecodeNES(&play);
    assert(ctx->unk120D8 == 2 && glyphUploads <= 120);
    assert(ctx->unk11F20[0] == 0x5A && ctx->unk11F20[1] == 0x6B);
    const uint8_t terminal = ctx->decodedBuffer.schar[ctx->decodedTextLen - (fade ? 2 : 0)];
    assert(terminal == (fade ? MESSAGE_FADE : MESSAGE_END));
    assert(ctx->msgMode == MSGMODE_TEXT_DISPLAYING && iconUploads == 1);
}

int main(int argc, char** argv) {
    (void)argv;
    gSaveContext.options.language = LANGUAGE_ENG;
    for (uint8_t element = 1; element <= 3; ++element)
        for (int custom = 0; custom <= 1; ++custom)
            for (int fade = 0; fade <= 1; ++fade)
                DecodeReceipt(element, custom, fade, argc > 1);
    puts("PASS complete native MM header + NES decoder: three-line tutorial, END/FADE, native/custom, row/glyph bounds");
}
