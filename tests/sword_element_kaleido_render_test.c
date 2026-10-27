#include "global.h"
#include "sword_element.h"
#include "sword_element_kaleido.h"
#include "../combo/render/sword_element_ui_art_data.h"
#include "../combo/render/sword_element_ui_type_data.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static GraphicsContext sGfx;
static PlayState sPlay;
static Gfx sOpa[1024], sOverlay[2048], sWork[32], sXlu[32];
static Gfx sArena[256];
static Vtx sVertices[4];
static const char sSwordIcon[32] = "SwordIcon";
static u8 sFailAllocation;
static u8 sActive = SWORD_ELEMENT_FIRE;
SaveContext gSaveContext;
void gSPVertex(Gfx* packet, uintptr_t vertices, int count, int first) { __gSPVertex(packet, vertices, count, first); }
uint8_t SwordElement_IsAvailable(uint8_t element) { return element != SWORD_ELEMENT_ICE; }
uint8_t SwordElement_ForItem(uint16_t item) { return sActive; }
void FrameInterpolation_RecordOpenChild(const void* key, int id) {}
void FrameInterpolation_RecordCloseChild(void) {}
#ifdef SWORD_KALEIDO_MM
void Gfx_SetupDL42_Opa(GraphicsContext* context) {}
void Gfx_SetupDL39_Overlay(GraphicsContext* context) {}
void Graph_OpenDisps(Gfx** refs, Gfx* vals, GraphicsContext* context, const char* file, s32 line) {}
void Graph_CloseDisps(Gfx** refs, Gfx* vals, GraphicsContext* context, const char* file, s32 line) {}
#else
void Gfx_SetupDL_42Opa(GraphicsContext* context) {}
void Gfx_SetupDL_39Overlay(GraphicsContext* context) {}
void Graph_OpenDisps(Gfx** refs, GraphicsContext* context, const char* file, s32 line) {}
void Graph_CloseDisps(Gfx** refs, GraphicsContext* context, const char* file, s32 line) {}
void* Graph_Alloc(GraphicsContext* context, size_t size) {
    assert(size == sizeof(sVertices));
    return sFailAllocation ? NULL : sVertices;
}
#endif
/* No GfxPrint fixture is provided: any accidental pixel-printer call fails linking. */
static void reset_draw(void) {
    memset(&sGfx, 0, sizeof(sGfx));
    memset(sOpa, 0, sizeof(sOpa));
    memset(sOverlay, 0, sizeof(sOverlay));
    sGfx.polyOpa.p = sOpa;
    sGfx.polyOpa.d = sArena + 256;
    sGfx.polyXlu.p = sXlu;
    sGfx.overlay.p = sOverlay;
    sGfx.work.p = sWork;
    sPlay.state.gfxCtx = &sGfx;
    sPlay.pauseCtx.alpha = 255;
}
static int resource(const char* fragment) {
    Gfx* starts[] = { sOpa, sOverlay };
    Gfx* ends[] = { sGfx.polyOpa.p, sGfx.overlay.p };
    for (int i = 0; i < 2; ++i) for (Gfx* p = starts[i]; p < ends[i]; ++p) {
        if ((p->words.w0 >> 24) == G_SETTIMG) {
            const char* data = (const char*)p->words.w1;
            if ((data == sSwordIcon || !memcmp(data, "__OTR__", 7)) && strstr(data, fragment)) return 1;
        }
    }
    return 0;
}
static int command(int opcode, uintptr_t word1) {
    Gfx* starts[] = { sOpa, sOverlay };
    Gfx* ends[] = { sGfx.polyOpa.p, sGfx.overlay.p };
    for (int i = 0; i < 2; ++i) for (Gfx* p = starts[i]; p < ends[i]; ++p) {
        if ((p->words.w0 >> 24) == opcode && p->words.w1 == word1) return 1;
    }
    return 0;
}
static const SwordElementUiMask* masks[] = {
    &sSwordElementUiTitle, &sSwordElementUiSubtitle,
    &sSwordElementUiNone, &sSwordElementUiFire, &sSwordElementUiIce, &sSwordElementUiLight,
    &sSwordElementUiActive, &sSwordElementUiLocked, &sSwordElementUiSelect, &sSwordElementUiArrows,
    &sSwordElementUiA, &sSwordElementUiB, &sSwordElementUiApply, &sSwordElementUiBack,
    &sSwordElementUiSolid, &sSwordElementUiRound, &sSwordElementUiCircle, &sSwordElementUiLock,
};
static int mask_count(const SwordElementUiMask* mask) {
    const u8* pixels = NULL;
    int format = -1, count = 0;
    for (Gfx* p = sOverlay; p < sGfx.overlay.p; ++p) {
        if ((p->words.w0 >> 24) == G_SETTIMG) {
            pixels = (const u8*)p->words.w1;
            format = (p->words.w0 >> 21) & 7;
        }
        if ((p->words.w0 >> 24) == G_SETTILESIZE && format == G_IM_FMT_IA) {
            int width = ((p->words.w1 >> 12) & 0xfff) / 4 + 1;
            int height = (p->words.w1 & 0xfff) / 4 + 1;
            count += width == mask->width && height == mask->height &&
                     !memcmp(pixels, mask->pixels, width * height);
        }
    }
    return count;
}
static void check_native_load_bounds(void) {
    const u8* pixels = NULL;
    int format = -1, loadBytes = 0, artTiles = 0;
    for (Gfx* p = sOverlay; p < sGfx.overlay.p; ++p) {
        int op = p->words.w0 >> 24;
        if (op == G_SETTIMG) {
            pixels = (const u8*)p->words.w1;
            format = (p->words.w0 >> 21) & 7;
        }
        if (op == G_LOADBLOCK) {
            int texels = ((p->words.w1 >> 12) & 0xfff) + 1;
            /* IA8 blocks are transferred through 16-bit load units. */
            loadBytes = texels * (format == G_IM_FMT_RGBA ? 4 : 2);
            assert(loadBytes <= 4096);
        }
        if (op != G_SETTILESIZE || pixels == (const u8*)sSwordIcon) continue;
        int width = ((p->words.w1 >> 12) & 0xfff) / 4 + 1;
        int height = (p->words.w1 & 0xfff) / 4 + 1;
        if (format == G_IM_FMT_RGBA) {
            const SwordElementUiRgbaTile* tile = (const void*)(pixels - offsetof(SwordElementUiRgbaTile, pixels));
            assert(width == 32 && height == 32 && loadBytes == sizeof(tile->pixels));
            assert(tile->before == SWORD_ELEMENT_UI_CANARY && tile->after == SWORD_ELEMENT_UI_CANARY);
            ++artTiles;
        } else {
            assert(format == G_IM_FMT_IA && loadBytes <= width * height);
            int matched = 0;
            for (unsigned i = 0; i < sizeof(masks) / sizeof(*masks); ++i) {
                matched |= width == masks[i]->width && height == masks[i]->height &&
                           !memcmp(pixels, masks[i]->pixels, width * height);
            }
            assert(matched);
        }
    }
    assert(artTiles == 12);
}
static Vtx* vertices(void) {
#ifdef SWORD_KALEIDO_MM
    return (Vtx*)sGfx.polyOpa.d;
#else
    return sVertices;
#endif
}
#include "elemental_ui_dump.h"
int main(int argc, char** argv) {
    SwordElementMenu menu = { 0 };
    SaveContext saveBefore;
    PauseContext pauseBefore;
    memset(&gSaveContext, 0x5a, sizeof(gSaveContext));
    reset_draw();
    SwordElementKaleido_Draw(&sPlay, &menu, (void*)sSwordIcon);
    assert(sGfx.polyOpa.p == sOpa && sGfx.overlay.p == sOverlay);
    menu.phase = SWORD_ELEMENT_MENU_CHOOSER;
    menu.selection = SWORD_ELEMENT_LIGHT;
    saveBefore = gSaveContext;
    pauseBefore = sPlay.pauseCtx;
    SwordElementKaleido_Draw(&sPlay, &menu, (void*)sSwordIcon);
    assert(sGfx.polyOpa.p == sOpa && sGfx.overlay.p > sOverlay);
    assert(sGfx.overlay.p < sOverlay + 2048);
    assert(resource("SwordIcon") && !resource("EquippedItemOutline"));
    assert(!resource("ArrowFire") && !resource("FireArrow") && !resource("ArrowIce") && !resource("IceArrow"));
    assert(!resource("ArrowLight") && !resource("LightArrow") && !resource("MagicArrowEquip"));
    assert(command(G_SETGRAYSCALE, true) && command(G_SETGRAYSCALE, false));
    assert(mask_count(&sSwordElementUiTitle) == 1 && mask_count(&sSwordElementUiSubtitle) == 1);
    assert(mask_count(&sSwordElementUiNone) == 1 && mask_count(&sSwordElementUiFire) == 1);
    assert(mask_count(&sSwordElementUiIce) == 1 && mask_count(&sSwordElementUiLight) == 1);
    assert(mask_count(&sSwordElementUiActive) == 1 && mask_count(&sSwordElementUiLocked) == 1);
    assert(mask_count(&sSwordElementUiLock) == 1);
    assert(mask_count(&sSwordElementUiA) == 1 && mask_count(&sSwordElementUiB) == 1);
    assert(mask_count(&sSwordElementUiApply) == 1 && mask_count(&sSwordElementUiBack) == 1);
    assert(command(G_SETPRIMCOLOR, 0xf2dc9bd7)); /* Selected Light has its restrained accent. */
    assert(command(G_SETPRIMCOLOR, 0x768397ff)); /* Unavailable Ice has muted text. */
    assert(command(G_SETPRIMCOLOR, 0xc5e5d7ff)); /* Effective Fire stays ACTIVE while selection moves. */
    check_native_load_bounds();
    if (argc > 1) elemental_ui_dump(argv[1]);
    assert(!memcmp(&pauseBefore, &sPlay.pauseCtx, sizeof(pauseBefore)));
    assert(!memcmp(&saveBefore, &gSaveContext, sizeof(saveBefore)));

    reset_draw();
    sPlay.pauseCtx.alpha = 123;
    SwordElementKaleido_Draw(&sPlay, &menu, (void*)sSwordIcon);
    assert(command(G_SETPRIMCOLOR, 0xf2dc9b67));
    for (Gfx* p = sOverlay; p < sGfx.overlay.p; ++p)
        if ((p->words.w0 >> 24) == G_SETPRIMCOLOR) assert((p->words.w1 & 255) <= 123);
    reset_draw();
    sActive = SWORD_ELEMENT_ICE; /* A forced element can be active while arrow selection stays locked. */
    SwordElementKaleido_Draw(&sPlay, &menu, (void*)sSwordIcon);
    assert(mask_count(&sSwordElementUiActive) == 1 && mask_count(&sSwordElementUiLock) == 1);

    reset_draw();
    menu.phase = SWORD_ELEMENT_MENU_HOLD_SWORD;
    menu.selection = SWORD_ELEMENT_ICE;
    menu.alpha = 254;
    menu.x = 120;
    menu.y = 440;
    SwordElementKaleido_Draw(&sPlay, &menu, (void*)sSwordIcon);
    assert(resource("MagicArrowEquipEffectTex"));
    assert(command(G_SETPRIMCOLOR, 0x6464FFFE));
    assert(sGfx.overlay.p == sOverlay);
    Vtx* v = vertices();
    assert(v[0].v.ob[0] == -3 && v[0].v.ob[1] == 59);
    assert(v[3].v.ob[0] == 59 && v[3].v.ob[1] == -3);
    assert(!memcmp(&pauseBefore, &sPlay.pauseCtx, sizeof(pauseBefore)));
    assert(!memcmp(&saveBefore, &gSaveContext, sizeof(saveBefore)));
    reset_draw();
    sPlay.pauseCtx.alpha = 0;
    SwordElementKaleido_Draw(&sPlay, &menu, (void*)sSwordIcon);
    assert(sGfx.polyOpa.p == sOpa && sGfx.overlay.p == sOverlay);
    puts("sword elemental Kaleido render: smooth type, tiled art/load canaries, selected/active/locked, fade and fusion PASS");
}
