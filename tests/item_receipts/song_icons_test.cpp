// Run the real OoT custom-icon load/draw hooks and engine rectangle at the
// renderer boundary. The clef is a native 16x24 IA8 texture, not a square icon.
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include "combo/menu/ComboItemReceiptPresentation.h"
#define RANDO_ENUM_BEGIN(x) enum x {
#define RANDO_ENUM_ITEM(x) x,
#define RANDO_ENUM_END(x) };
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h"
#include "combo/menu/ComboSongDrawOOT.h"
using u16 = uint16_t;
using u8 = uint8_t;
using s16 = int16_t;
using s32 = int32_t;
using Gfx = int;
using TexturePtr = void*;
struct Color_RGB8 { uint8_t r, g, b; };
enum CustomIconSize { ICON_SIZE_32, ICON_SIZE_24 };
enum Language { LANGUAGE_ENG, LANGUAGE_GER, LANGUAGE_FRA };
constexpr int OBJECT_INVALID = -1, MOD_RANDOMIZER = 1, ITEM_CUSTOM = 0x9c,
              MESSAGE_STATIC_TEX_SIZE = 0, MSGMODE_TEXT_DISPLAYING = 1;
constexpr int ITEM_MEDALLION_FOREST = 0x66, ITEM_ROCS_FEATHER_SKIJER = 0xa0,
              ITEM_EXT_BOOTS_3 = 0xe8, ITEM_NET = 0xf4,
              ITEM_BOTTOMLESS_BOTTLE = 0xf5, ITEM_ROCS_FEATHER = 0x9d;
constexpr int G_IM_FMT_RGBA = 0, G_IM_FMT_IA = 3, G_IM_SIZ_8b = 1, G_IM_SIZ_32b = 3;
struct MessageContext {
    char textboxSegment[512]{};
    int msgBufPos = 0, choiceNum = 0, textColorAlpha = 255, textPosX = 0, textPosY = 0, msgMode = 0;
};
struct PlayState { MessageContext msgCtx; } play;
PlayState* gPlayState = &play;
struct Player { struct { int objectId = 0, modIndex = MOD_RANDOMIZER, getItemId = RG_MM_SONG_HEALING; } getItemEntry; } testPlayer;
#define GET_PLAYER(play) (&testPlayer)
struct { int language = LANGUAGE_ENG; struct { int language = LANGUAGE_ENG; } options; } gSaveContext;
int R_TEXTBOX_ICON_XPOS, R_TEXTBOX_ICON_YPOS, R_TEXTBOX_ICON_SIZE;
int R_TEXT_INIT_XPOS = 100, R_TEXTBOX_Y = 20;
const char* iconPath = "__OTR__textures/icon_item_static/gSongNoteTex";
CustomIconSize iconSize = ICON_SIZE_32;
namespace Rando::StaticData {
struct Item {
    const char* GetCustomIcon() const { return iconPath; }
    CustomIconSize GetCustomIconSize() const { return iconSize; }
};
Item RetrieveItem(RandomizerGet) { return {}; }
}
int format, bits, width, height, rectangleWidth, rectangleHeight;
Color_RGB8 color{};
#define gDPPipeSync(p) ((void)(p))
#define gDPSetCombineMode(p, ...) ((void)(p))
#define gDPSetCombineLERP(p, ...) ((void)(p))
#define gSPInvalidateTexCache(p, ...) ((void)(p))
#define gDPSetPrimColor(p, m, l, r, g, b, a) ((void)(p), ::color = Color_RGB8{uint8_t(r), uint8_t(g), uint8_t(b)})
#define gDPLoadTextureBlock(p, tex, fmt, siz, w, h, ...) \
    ((void)(p), format = (fmt), bits = (siz), width = (w), height = (h))
#define gSPTextureRectangle(p, x1, y1, x2, y2, ...) \
    ((void)(p), rectangleWidth = ((x2) - (x1)) / 4, rectangleHeight = ((y2) - (y1)) / 4)
int gSfxDefaultPos, gSfxDefaultFreqAndVolScale, gSfxDefaultReverb;
void Audio_PlaySfxGeneral(int, int*, int, int*, int*, int*) {}
constexpr int VB_DRAW_ITEM_ICON = 0;
void DrawCustomItemIcon(Gfx**);
bool GameInteractor_Should(int, bool, Gfx** gfx) { DrawCustomItemIcon(gfx); return false; }
/* CUSTOM_ICON_FUNCTIONS */
#include "combo/menu/ComboItemReceiptRender.h"
// This song-only fixture leaves the separate dungeon-receipt layout inactive.
CwItemReceiptPresentation sItemReceiptPresentation{};
CwItemReceiptLayout sItemReceiptLayout{};
int R_TEXT_CHAR_SCALE = 75;
int Message_HasItemReceiptIcon() { return false; }
void Message_DrawItemReceiptIcon(PlayState*, Gfx**) {}
/* ENGINE_ICON_DRAW */
// These are the selected native MM branches after the tested message-byte
// lookup. Their original palette, texture format and rectangle are executed.
constexpr int ITEM_SONG_SONATA = 0, ITEM_SONG_HEALING = 7, ITEM_SONG_TIME = 6;
constexpr int TEXTBOX_SEG_ICON = 0;
int D_801CFF88[] = { 60, 60, 60 };
void* gStaticItemIcons[16]{};
struct MmMessageContext {
    int itemId = ITEM_SONG_HEALING, textColorAlpha = 255;
    int unk11FF8 = 100, unk12010 = 0, unk12012 = 0, unk12014 = 0, unk12016 = 0;
    int choiceNum = 0;
    void* textboxSegment[1]{};
};
/* MM_SONG_PALETTE */
/* MM_CUSTOM_STAGE */
int D_801CFF70[] = {18, 18, 18}, D_801CFF7C[] = {14, 14, 14};
void MmCustomIcon(MmMessageContext* msgCtx, Gfx* gfx) {
    struct {
        struct {
            bool bombersNotebookOpen = false;
        } pauseCtx;
    } mmPlay;
    auto* play = &mmPlay;
    const int itemId = MESSAGE_CUSTOM_ICON_ITEM;
    int arg2 = 20, textureStep = 1 << 10;
    (void)textureStep;
    /* MM_CUSTOM_LOAD */
    /* MM_CUSTOM_DRAW */
    /* MM_ICON_RECTANGLE */
}
void MmSongIcon(MmMessageContext* msgCtx, Gfx* gfx) {
    int arg2 = 20, index;
    /* MM_SONG_LOAD */
    /* MM_SONG_DRAW */
    /* MM_ICON_RECTANGLE */
}
int main() {
    Gfx commands[64]{};
    Gfx* p = commands;
    LoadCustomItemIcon(false);
    Message_DrawItemIcon(&play, ITEM_CUSTOM, &p, 0);
    assert(format == G_IM_FMT_IA && bits == G_IM_SIZ_8b);
    assert(width == 16 && height == 24);
    assert(rectangleWidth == 16 && rectangleHeight == 24);
    assert(color.r == 255 && color.g == 150 && color.b == 230);
    testPlayer.getItemEntry.getItemId = RG_MM_SONG_LULLABY;
    p = commands;
    LoadCustomItemIcon(false);
    Message_DrawItemIcon(&play, ITEM_CUSTOM, &p, 0);
    assert(color.r == 255 && color.g == 20 && color.b == 20);
    testPlayer.getItemEntry.getItemId = RG_MM_SONG_DOUBLE_TIME;
    p = commands;
    LoadCustomItemIcon(false);
    Message_DrawItemIcon(&play, ITEM_CUSTOM, &p, 0);
    assert(color.r == 128 && color.g == 216 && color.b == 240);
    testPlayer.getItemEntry.getItemId = RG_MM_SONG_INVERTED_TIME;
    p = commands;
    LoadCustomItemIcon(false);
    Message_DrawItemIcon(&play, ITEM_CUSTOM, &p, 0);
    assert(color.r == 74 && color.g == 112 && color.b == 202);
    iconPath = "__OTR__textures/icon_item_custom/square";
    for (CustomIconSize size : { ICON_SIZE_32, ICON_SIZE_24 }) {
        iconSize = size;
        p = commands;
        LoadCustomItemIcon(false);
        Message_DrawItemIcon(&play, ITEM_CUSTOM, &p, 0);
        const int expected = size == ICON_SIZE_32 ? 32 : 24;
        assert(format == G_IM_FMT_RGBA && bits == G_IM_SIZ_32b);
        assert(width == expected && height == expected);
        assert(rectangleWidth == expected && rectangleHeight == expected);
        assert(color.r == 255 && color.g == 255 && color.b == 255);
    }
    MmMessageContext mm;
    MmSongIcon(&mm, commands);
    assert(format == G_IM_FMT_IA && bits == G_IM_SIZ_8b && width == 16 && height == 24);
    assert(rectangleWidth == 16 && rectangleHeight == 24);
    assert(color.r == 255 && color.g == 150 && color.b == 230);
    mm.itemId = ITEM_SONG_TIME;
    MmSongIcon(&mm, commands);
    assert(color.r == 98 && color.g == 177 && color.b == 211);
    Message_StageCustomItemIconTint((void*)"clef", 16, 24, true, 128, 216, 240);
    MmCustomIcon(&mm, commands);
    assert(format == G_IM_FMT_IA && bits == G_IM_SIZ_8b && width == 16 && height == 24);
    assert(rectangleWidth == 16 && rectangleHeight == 24 && mm.unk12012 == 30);
    assert(color.r == 128 && color.g == 216 && color.b == 240);
    Message_StageCustomItemIcon((void*)"square", 32);
    MmCustomIcon(&mm, commands);
    assert(format == G_IM_FMT_RGBA && width == 32 && height == 32);
    assert(rectangleWidth == 32 && rectangleHeight == 32);
    assert(color.r == 255 && color.g == 255 && color.b == 255);
    std::cout << "Both song receipt renderers retain native IA8 aspect and song colors; square icons preserved\n";
}
