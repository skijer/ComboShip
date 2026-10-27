// Built-in native OoT location artwork with native timing and exact decoded
// MM title eligibility. Optional HD packs replace these same native resources.
#include "OotLocationTitleCard.h"
#include "message_data_fmt_nes.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <stdio.h>
#include <string.h>

extern f32 sNESFontWidths[160];

static PlayState* sOwner;
static s16 sScene;
static u16 sTextId;
static TitleCardContext sCard;
static s16 sWidths[3];
static u8 sLines;
static int sUseTexture; // 1: legacy 192px override, 2: built-in 240px native card
static int sUseFont;    // 0: native MM, 1: optional 16px font, 2: native OoT profile
// Native OoT place-card crops retain their source proportions and 24px baseline.
// This versioned profile must not use the native MM dialogue advance table.
static const u8 sOotNativeAdvances[160] = {
    [7] = 3,   // 0x27
    [33] = 13, // 0x41
    [34] = 11, // 0x42
    [35] = 15, // 0x43
    [36] = 13, // 0x44
    [38] = 9,  // 0x46
    [39] = 14, // 0x47
    [40] = 13, // 0x48
    [41] = 5,  // 0x49
    [42] = 7,  // 0x4a
    [43] = 12, // 0x4b
    [44] = 11, // 0x4c
    [45] = 15, // 0x4d
    [48] = 12, // 0x50
    [50] = 11, // 0x52
    [51] = 13, // 0x53
    [52] = 10, // 0x54
    [54] = 13, // 0x56
    [55] = 18, // 0x57
    [58] = 12, // 0x5a
    [65] = 10, // 0x61
    [66] = 7,  // 0x62
    [67] = 10, // 0x63
    [68] = 10, // 0x64
    [69] = 9,  // 0x65
    [70] = 8,  // 0x66
    [71] = 11, // 0x67
    [72] = 11, // 0x68
    [73] = 4,  // 0x69
    [75] = 10, // 0x6b
    [76] = 4,  // 0x6c
    [77] = 14, // 0x6d
    [78] = 10, // 0x6e
    [79] = 10, // 0x6f
    [80] = 10, // 0x70
    [82] = 8,  // 0x72
    [83] = 8,  // 0x73
    [84] = 8,  // 0x74
    [85] = 9,  // 0x75
    [86] = 10, // 0x76
    [87] = 14, // 0x77
    [88] = 9,  // 0x78
    [89] = 11, // 0x79
    [90] = 10, // 0x7a
};

// Sprite texture commands reserve odd pointer values for segmented tags.
static union {
    u16 alignment;
    char path[96];
} sTextureStorage;
#define sTexturePath sTextureStorage.path
// The exact decoded native characters remain authoritative. Optional title-only
// font resources are used only when the complete current message is covered.
// Each row has even stride/alignment and survives all deferred graphics reads.
static union {
    u16 alignment;
    char path[64];
} sGlyphStorage[160];

int OotLocationTitle_HasTexture(const char* path);

static u16 SceneTitleId(s16 scene) {
    switch (scene) {
#define DEFINE_SCENE(name, id, title, draw, restrictions, cycle, entrance, map, human) \
    case id:                                                                           \
        return title;
#define DEFINE_SCENE_UNSET(id)
#include "tables/scene_table.h"
#undef DEFINE_SCENE
#undef DEFINE_SCENE_UNSET
        default:
            return 0;
    }
}

void OotLocationTitle_Reset(void) {
    sOwner = NULL;
}

static int Active(PlayState* play) {
    return play && play == sOwner && play->sceneId == sScene && play->msgCtx.currentTextId == sTextId &&
           play->msgCtx.msgMode >= MSGMODE_SCENE_TITLE_CARD_FADE_IN_BACKGROUND &&
           play->msgCtx.msgMode <= MSGMODE_SCENE_TITLE_CARD_FADE_OUT_BACKGROUND;
}

static int IsIgnoredControl(u8 ch) {
    return ch <= MESSAGE_COLOR_ORANGE || ch == MESSAGE_QUICKTEXT_ENABLE || ch == MESSAGE_QUICKTEXT_DISABLE;
}

// The decoder has already expanded names/shifts and loaded the native glyphs.
// Reject scripting/button controls, overly wide text and excess lines instead
// of interpreting arbitrary dialogue as an image. Native presentation wins.
static int Measure(MessageContext* msg) {
    unsigned glyphs = 0;
    sLines = 1;
    memset(sWidths, 0, sizeof(sWidths));
    // Native decoding excludes a simple terminator from decodedTextLen.
    // Reserve and validate that slot before accepting the glyph stream;
    // event/page/persistent controls must keep the complete native path.
    if (msg->decodedTextLen >= sizeof(msg->decodedBuffer.schar) || msg->font.unk_11D88 > 1 ||
        (u8)msg->decodedBuffer.schar[msg->decodedTextLen] != MESSAGE_END)
        return 0;
    for (unsigned i = 0; i < msg->decodedTextLen; ++i) {
        u8 ch = (u8)msg->decodedBuffer.schar[i];
        if (ch == MESSAGE_END)
            break;
        if (IsIgnoredControl(ch))
            continue;
        if (ch == MESSAGE_NEWLINE || ch == MESSAGE_CARRIAGE_RETURN) {
            if (++sLines > 3)
                return 0;
        } else if (ch == ' ') {
            sWidths[sLines - 1] += 6;
        } else if (ch > ' ' && ch < MESSAGE_BTN_A) {
            if (++glyphs * FONT_CHAR_TEX_SIZE > sizeof(msg->font.charBuf[0]))
                return 0;
            sWidths[sLines - 1] += (s16)sNESFontWidths[ch - ' '];
        } else {
            return 0;
        }
        if (sWidths[sLines - 1] > 300)
            return 0;
    }
    return glyphs != 0;
}

static int PrepareNativeFont(MessageContext* msg) {
    u8 checked[160] = { 0 };
    s16 widths[3] = { 0 };
    unsigned line = 0;
    for (unsigned i = 0; i < msg->decodedTextLen; ++i) {
        u8 ch = (u8)msg->decodedBuffer.schar[i];
        if (ch == MESSAGE_END)
            break;
        if (IsIgnoredControl(ch))
            continue;
        if (ch == MESSAGE_NEWLINE || ch == MESSAGE_CARRIAGE_RETURN) {
            ++line; // Measure already bounded this to three lines.
        } else if (ch == ' ') {
            widths[line] += 5;
        } else {
            unsigned index = ch - ' ';
            if (!sOotNativeAdvances[index])
                return 0;
            if (!checked[index]) {
                snprintf(sGlyphStorage[index].path, sizeof(sGlyphStorage[index].path),
                         "__OTR__custom/mm_location_titles/oot_native_v1/%02x", (unsigned)ch);
                if (!OotLocationTitle_HasTexture(sGlyphStorage[index].path))
                    return 0;
                checked[index] = 1;
            }
            widths[line] += sOotNativeAdvances[index];
        }
        // Preserve the native aspect ratio rather than squeezing long names.
        if (widths[line] > 300)
            return 0;
    }
    memcpy(sWidths, widths, sizeof(sWidths));
    return 2;
}

static int PrepareFont(MessageContext* msg) {
    // An installed native profile is authoritative: missing letters fall back
    // for the whole card rather than mixing in the older optional font.
    if (OotLocationTitle_HasTexture("__OTR__custom/mm_location_titles/oot_native_v1/profile"))
        return PrepareNativeFont(msg);
    u8 checked[160] = { 0 };
    for (unsigned i = 0; i < msg->decodedTextLen; ++i) {
        u8 ch = (u8)msg->decodedBuffer.schar[i];
        if (ch == MESSAGE_END)
            break;
        if (IsIgnoredControl(ch) || ch == ' ' || ch == MESSAGE_NEWLINE || ch == MESSAGE_CARRIAGE_RETURN)
            continue;
        // Measure already rejected scripting/buttons and bounded this index.
        unsigned index = ch - ' ';
        if (!checked[index]) {
            snprintf(sGlyphStorage[index].path, sizeof(sGlyphStorage[index].path),
                     "__OTR__custom/mm_location_titles/font/%02x", (unsigned)ch);
            if (!OotLocationTitle_HasTexture(sGlyphStorage[index].path))
                return 0;
            checked[index] = 1;
        }
    }
    return 1;
}

void OotLocationTitle_Begin(PlayState* play) {
    OotLocationTitle_Reset();
    MessageContext* msg = &play->msgCtx;
    if (!CVarGetInteger("gEnhancements.Graphics.OotLocationTitleCards", 0) || !msg->currentTextId ||
        (msg->currentTextId >= 0x1BB2 && msg->currentTextId <= 0x1BB6) ||
        SceneTitleId(play->sceneId) != msg->currentTextId || !Measure(msg))
        return;
    // The native caller has already decided entrance/cutscene/hide eligibility.
    // Do not arm a second title context or touch any saved arrival/history flags.
    sOwner = play;
    sScene = play->sceneId;
    sTextId = msg->currentTextId;
    memset(&sCard, 0, sizeof(sCard));
    sCard.delayTimer = 20;
    sCard.durationTimer = 80;
    // Each resource is a single 192x24 IA8 card, including its black keyline.
    // The native MM resource manager retains normal pack order and Alt routing.
    snprintf(sTexturePath, sizeof(sTexturePath), "__OTR__custom/mm_location_titles/%u/%04x",
             (unsigned)gSaveContext.options.language, (unsigned)sTextId);
    sUseTexture = OotLocationTitle_HasTexture(sTexturePath);
    if (!sUseTexture) {
        // Native lettering ships in the ordinary port archive. It must not
        // depend on mounting the optional higher-resolution replacement pack.
        snprintf(sTexturePath, sizeof(sTexturePath), "__OTR__custom/oot_location_titles/v1/%u/%04x",
                 (unsigned)gSaveContext.options.language, (unsigned)sTextId);
        if (OotLocationTitle_HasTexture(sTexturePath))
            sUseTexture = 2;
    }
    // A hand-authored whole-card override wins. Missing any optional font
    // character keeps the entire native glyph presentation, without a mix.
    sUseFont = sUseTexture ? 0 : PrepareFont(msg);
}

int OotLocationTitle_Update(PlayState* play) {
    if (!Active(play))
        return 0;
    MessageContext* msg = &play->msgCtx;
    TitleCard_Update(&play->state, &sCard);
    msg->textColorAlpha = sCard.alpha;
    msg->textboxColorAlphaCurrent = 0;
    if (!sCard.durationTimer && !sCard.delayTimer && !sCard.alpha) {
        // Same terminal message cleanup as the native location-title branch.
        msg->msgLength = 0;
        msg->msgMode = MSGMODE_NONE;
        msg->currentTextId = 0;
        msg->stateTimer = 0;
        OotLocationTitle_Reset();
    }
    return 1;
}

static Gfx* GlyphRect(Gfx* gfx, s16 x, s16 y) {
    s16 size = sUseFont == 2 ? 24 : 16;
    gSPTextureRectangle(gfx++, x * 4, y * 4, (x + size) * 4, (y + size) * 4, G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
    return gfx;
}

int OotLocationTitle_Draw(PlayState* play, Gfx** gfxP) {
    if (!Active(play))
        return 0;
    if (!sCard.alpha)
        return 1;
    MessageContext* msg = &play->msgCtx;
    Gfx* gfx = *gfxP;
    Gfx_SetupDL39_Ptr(&gfx);
    gDPSetTextureFilter(gfx++, G_TF_BILERP);
    gDPSetCombineMode(gfx++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gDPSetEnvColor(gfx++, 0, 0, 0, 255);
    if (sUseTexture) {
        const s16 width = sUseTexture == 2 ? 240 : 192;
        const s16 left = 160 - width / 2;
        gDPSetPrimColor(gfx++, 0, 0, sCard.intensity, sCard.intensity, sCard.intensity, sCard.alpha);
        gDPLoadTextureBlock(gfx++, sTexturePath, G_IM_FMT_IA, G_IM_SIZ_8b, width, 24, 0, G_TX_NOMIRROR | G_TX_WRAP,
                            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        gSPTextureRectangle(gfx++, left * 4, 108 * 4, (left + width) * 4 - 4, 132 * 4 - 1, G_TX_RENDERTILE, 0, 0,
                            1 << 10, 1 << 10);
    } else {
        unsigned glyph = 0, line = 0;
        s16 x = 160 - sWidths[0] / 2;
        s16 lineHeight = sUseFont == 2 ? 24 : 18;
        s16 y = 120 - (sUseFont == 2 ? sLines * 24 : sLines * 18 - 2) / 2;
        for (unsigned i = 0; i < msg->decodedTextLen; ++i) {
            u8 ch = (u8)msg->decodedBuffer.schar[i];
            if (ch == MESSAGE_END)
                break;
            if (IsIgnoredControl(ch))
                continue;
            if (ch == MESSAGE_NEWLINE || ch == MESSAGE_CARRIAGE_RETURN) {
                x = 160 - sWidths[++line] / 2;
                y += lineHeight;
            } else if (ch == ' ') {
                x += sUseFont == 2 ? 5 : 6;
            } else {
                gDPPipeSync(gfx++);
                unsigned nativeGlyph = glyph++;
                if (sUseFont) {
                    gDPLoadTextureBlock(gfx++, sGlyphStorage[ch - ' '].path, G_IM_FMT_IA, G_IM_SIZ_8b,
                                        sUseFont == 2 ? 24 : 16, sUseFont == 2 ? 24 : 16, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                                        G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
                } else {
                    gDPLoadTextureBlock_4b(
                        gfx++, &msg->font.charBuf[msg->font.unk_11D88][nativeGlyph * FONT_CHAR_TEX_SIZE], G_IM_FMT_I,
                        16, 16, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK,
                        G_TX_NOLOD, G_TX_NOLOD);
                }
                gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, sCard.alpha);
                gfx = GlyphRect(gfx, x - 1, y);
                gfx = GlyphRect(gfx, x + 1, y);
                gfx = GlyphRect(gfx, x, y - 1);
                gfx = GlyphRect(gfx, x, y + 1);
                gDPPipeSync(gfx++);
                gDPSetPrimColor(gfx++, 0, 0, sCard.intensity, sCard.intensity, sCard.intensity, sCard.alpha);
                gfx = GlyphRect(gfx, x, y);
                x += sUseFont == 2 ? sOotNativeAdvances[ch - ' '] : (s16)sNESFontWidths[ch - ' '];
            }
        }
    }
    *gfxP = gfx;
    return 1;
}
