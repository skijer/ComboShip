#include "global.h"
#include "assets/objects/object_gi_hearts/object_gi_hearts.h"
#include "assets/objects/object_gi_magicpot/object_gi_magicpot.h"
#include "assets/objects/object_gi_bomb_1/object_gi_bomb_1.h"
#include "assets/objects/object_gi_heart/object_gi_heart.h"
#include "assets/objects/gameplay_keep/gameplay_keep.h"
#include "overlays/actors/ovl_Item_B_Heart/z_item_b_heart.h"
#include "overlays/actors/ovl_En_Slime/z_en_slime.h"
#include <stdio.h>
#include <string.h>
#include "2s2h/BenGui/CosmeticEditor.h"

// The table is the resource-service boundary; functions below are unmodified
// production bodies. Distinct names expose wrong-item and border tinting.
static struct {
    const char* drawResources[2];
} sDrawItemTable[256] = {
    [GID_HEART_PIECE] = { { gGiHeartBorderDL, gGiHeartPieceDL } },
    [GID_HEART_CONTAINER] = { { gGiHeartBorderDL, gGiHeartContainerDL } },
    [GID_MAGIC_JAR_SMALL] = { { gGiMagicJarSmallDL } },
    [GID_MAGIC_JAR_BIG] = { { gGiMagicJarLargeDL } },
    [GID_RECOVERY_HEART] = { { gGiRecoveryHeartDL } },
    [GID_BOMB] = { { gGiBombDL } },
};
static int sCollectibleShimmer, sShimmerDraws, sSampledProfile, sMatrixDepth;
static float sDropScale = 1.0f, sSavedScale, sShimmerScale;
static int sCustom, sHeartsChanged, sMagicChanged, sFailures, sCases;
static const char* sFirstAltDL;
static Color_RGB8 sHeartsColor = { 53, 167, 225 }, sMagicColor = { 230, 93, 171 };
static Gfx sOpa[128], sXlu[128];
static GraphicsContext sGfx;
static PlayState sPlay;
PlayState* gPlayState = &sPlay;
static Mtx sMatrix;
static EnItem00 sPickup;
static ItemBHeart sBossHeart;
static Actor sWarp;
static Player sPlayer;
static EnSlime sSlime;

int32_t CVarGetInteger(const char* name, int32_t fallback) {
    if (!strcmp(name, "gEnhancements.CollectibleShimmer"))
        return sCollectibleShimmer;
    if (!strcmp(name, "gCosmetic.HUD.Hearts.Changed"))
        return sHeartsChanged;
    if (!strcmp(name, "gCosmetic.HUD.Magic.Changed"))
        return sMagicChanged;
    return fallback;
}
Color_RGBA8 CosmeticEditor_GetChangedColor(u8 r, u8 g, u8 b, u8 a, const char* id) {
    Color_RGB8 c = !strcmp(id, "HUD.Hearts") ? sHeartsColor : sMagicColor;
    return (Color_RGBA8){ c.r, c.g, c.b, a };
}
uint8_t ResourceGetIsCustomByName(const char* name) {
    return sCustom;
}
void gSPDisplayList(Gfx* pkt, Gfx* dl) {
    // The real GBI bridge resolves an uncached Alt before submitting its DL,
    // even when an earlier resource query still sees a cached native asset.
    if (sFirstAltDL && !strcmp((const char*)dl, sFirstAltDL))
        sCustom = 1;
    __gSPDisplayList(pkt, dl);
}
void Graph_OpenDisps(Gfx** refs, Gfx* vals, GraphicsContext* gfx, const char* file, s32 line) {
}
void Graph_CloseDisps(Gfx** refs, Gfx* vals, GraphicsContext* gfx, const char* file, s32 line) {
}
void gSPSegment(void* pkt, int segment, uintptr_t target) {
    __gSPSegment((Gfx*)pkt, segment, target);
}
void Gfx_SetupDL25_Opa(GraphicsContext* gfx) {
}
void Gfx_SetupDL25_Xlu(GraphicsContext* gfx) {
}
Mtx* Matrix_Finalize(GraphicsContext* gfx) {
    return &sMatrix;
}
void Matrix_Scale(f32 x, f32 y, f32 z, MatrixMode mode) {
    sDropScale *= x;
}
void Matrix_RotateYS(s16 angle, MatrixMode mode) {
}
void FrameInterpolation_IgnoreActorMtx(void) {
}
// Passive capture reporting is outside this color/GBI fixture.
void NeiGi_ReportBlackMarketModel(const char* owner, const char* opaque, const char* translucent) {
}
void GetItem_DrawRecoveryHeart(PlayState* play, s16 drawId);
void GetItem_DrawOpa0(PlayState* play, s16 drawId);
void GetItem_Draw(PlayState* play, s16 drawId) {
    // Dispatch boundary for the three production 3D drop entry points below.
    // Their draw bodies and custom/native classification remain production code.
    if (drawId == GID_RECOVERY_HEART) {
        GetItem_DrawRecoveryHeart(play, drawId);
    } else if (drawId == GID_MAGIC_JAR_SMALL || drawId == GID_MAGIC_JAR_BIG) {
        GetItem_DrawOpa0(play, drawId);
    } else {
        fprintf(stderr, "Unexpected drop draw id %d\n", drawId);
        ++sFailures;
    }
}
s32 Object_GetSlot(ObjectContext* objectCtx, s16 objectId) {
    return 0;
}
Gfx* Gfx_TwoTexScrollEx(GraphicsContext* gfx, s32 tile1, u32 x1, u32 y1, s32 width1, s32 height1, s32 tile2, u32 x2,
                        u32 y2, s32 width2, s32 height2, s32 sx1, s32 sy1, s32 sx2, s32 sy2) {
    return sXlu + 100;
}
void FrameInterpolation_RecordOpenChild(const void* a, int b) {
}
void FrameInterpolation_RecordCloseChild(void) {
}
void func_800B8118(Actor* actor, PlayState* play, s32 flag) {
}
void UnusedActorSetup(Actor* actor, PlayState* play, s32 flag) {
}

// Shared drop policy is production code. Only palette service, matrix storage
// and GPU overlay submission are fixture boundaries.
void Matrix_Push(void) {
    sSavedScale = sDropScale;
    ++sMatrixDepth;
}
void Matrix_Pop(void) {
    sDropScale = sSavedScale;
    --sMatrixDepth;
}
#define COMBO_COLLECTIBLE_SHIMMER_HOST_MM
#include "ComboCollectibleShimmerHost.h"
void MM_SampleCollectibleShimmerColor(int32_t profile, uint8_t color[4]) {
    sSampledProfile = profile;
    color[0] = sHeartsColor.r;
    color[1] = sHeartsColor.g;
    color[2] = sHeartsColor.b;
    color[3] = 255;
}
void NeiGi_DrawTintedShimmerOverlay(PlayState* play, const uint8_t color[4], const char* owner) {
    ++sShimmerDraws;
    sShimmerScale = sDropScale;
    if (play != &sPlay || owner != NULL || sMatrixDepth != 1 || color[0] != sHeartsColor.r ||
        color[1] != sHeartsColor.g || color[2] != sHeartsColor.b || color[3] != 255)
        ++sFailures;
}

#include "custom_item_color_production.inc"

static void Reset(void) {
    sFailures += sMatrixDepth != 0;
    sShimmerDraws = 0;
    sSampledProfile = -1;
    sDropScale = 1.0f;
    memset(&sGfx, 0, sizeof(sGfx));
    memset(&sPlay, 0, sizeof(sPlay));
    memset(sOpa, 0, sizeof(sOpa));
    memset(sXlu, 0, sizeof(sXlu));
    sGfx.polyOpa.p = sOpa;
    sGfx.polyXlu.p = sXlu;
    sPlay.state.gfxCtx = &sGfx;
    sPlay.actorCtx.actorLists[ACTORCAT_PLAYER].first = &sPlayer.actor;
}
static void CheckStreamWithBorder(const char* label, Gfx* begin, Gfx* end, const char* body, int tint, Color_RGB8 color,
                                  int whiteBorder) {
    int tintHeartsOverride = sCustom && sHeartsChanged;
    int gray = 0, bodySeen = 0, borderSeen = 0, error = 0;
    uint32_t rgba = 0;
    for (Gfx* g = begin; g < end; ++g) {
        unsigned op = (g->words.w0 >> 24) & 255;
        if (op == G_SETINTENSITY)
            rgba = g->words.w1;
        if (op == G_SETGRAYSCALE)
            gray = g->words.w1;
        if (op == G_DL) {
            const char* path = (const char*)g->words.w1;
            if (!strcmp(path, body)) {
                ++bodySeen;
                error |= gray != tint;
                if (tint)
                    error |= rgba != ((uint32_t)color.r << 24 | (uint32_t)color.g << 16 | (uint32_t)color.b << 8 |
                                      ((whiteBorder && !tintHeartsOverride) ? 100 : 255));
            } else if (whiteBorder && !strcmp(path, gGiHeartBorderDL)) {
                ++borderSeen;
                error |= gray != 1 || rgba != 0xFFFFFFFFu;
            } else {
                error |= gray != 0; // Frame/exterior/next item must not inherit tint.
            }
        }
    }
    error |= bodySeen != 1 || gray != 0 || borderSeen != whiteBorder;
    ++sCases;
    if (error) {
        ++sFailures;
        fprintf(stderr, "FAIL %s (expected tint=%d, body draws=%d, final grayscale=%d)\n", label, tint, bodySeen, gray);
    }
}
static void CheckStream(const char* label, Gfx* begin, Gfx* end, const char* body, int tint, Color_RGB8 color) {
    CheckStreamWithBorder(label, begin, end, body, tint, color, 0);
}
static void DrawCases(int tintHearts, int tintMagic) {
    Reset();
    GetItem_DrawXlu01(&sPlay, GID_HEART_PIECE);
    CheckStream("held/3D heart piece", sXlu, sGfx.polyXlu.p, gGiHeartPieceDL, tintHearts, sHeartsColor);
    Reset();
    GetItem_DrawXlu01(&sPlay, GID_HEART_CONTAINER);
    CheckStream("held heart container", sXlu, sGfx.polyXlu.p, gGiHeartContainerDL, tintHearts, sHeartsColor);
    Reset();
    GetItem_DrawRecoveryHeart(&sPlay, GID_RECOVERY_HEART);
    CheckStream("3D recovery heart", sXlu, sGfx.polyXlu.p, gGiRecoveryHeartDL, tintHearts, sHeartsColor);
    Reset();
    GetItem_DrawOpa0(&sPlay, GID_MAGIC_JAR_SMALL);
    GetItem_DrawOpa0(&sPlay, GID_BOMB);
    CheckStream("small magic jar then bomb", sOpa, sGfx.polyOpa.p, gGiMagicJarSmallDL, tintMagic, sMagicColor);
    Reset();
    GetItem_DrawOpa0(&sPlay, GID_MAGIC_JAR_BIG);
    CheckStream("large magic jar", sOpa, sGfx.polyOpa.p, gGiMagicJarLargeDL, tintMagic, sMagicColor);
    Reset();
    EnItem00_DrawHeartContainer(&sPickup, &sPlay);
    CheckStream("placed container interior", sXlu, sGfx.polyXlu.p, gGiHeartContainerDL, tintHearts, sHeartsColor);
    Reset();
    EnItem00_DrawHeartPiece(&sPickup, &sPlay);
    CheckStream("placed piece interior", sXlu, sGfx.polyXlu.p, gHeartPieceInteriorDL, tintHearts, sHeartsColor);
    Reset();
    ItemBHeart_Draw(&sBossHeart.actor, &sPlay);
    CheckStream("boss container opaque", sOpa, sGfx.polyOpa.p, gGiHeartContainerDL, tintHearts, sHeartsColor);
    Reset();
    sWarp.id = ACTOR_DOOR_WARP1;
    sWarp.projectedPos.z = 100;
    sWarp.next = NULL;
    sPlay.actorCtx.actorLists[ACTORCAT_ITEMACTION].first = &sWarp;
    ItemBHeart_Draw(&sBossHeart.actor, &sPlay);
    CheckStream("boss container translucent", sXlu, sGfx.polyXlu.p, gGiHeartContainerDL, tintHearts, sHeartsColor);
    Reset();
    DrawDoubleDefense();
    CheckStreamWithBorder("Double Defense white border and body", sXlu, sGfx.polyXlu.p, gGiHeartContainerDL, 1,
                          tintHearts ? sHeartsColor : (Color_RGB8){ 255, 0, 0 }, 1);

    Reset();
    sPickup.actor.params = ITEM00_RECOVERY_HEART;
    EnItem00_3DItemsDraw(&sPickup.actor, &sPlay);
    CheckStream("3D ground heart", sXlu, sGfx.polyXlu.p, gGiRecoveryHeartDL, tintHearts, sHeartsColor);
    Reset();
    sPickup.actor.params = ITEM00_MAGIC_JAR_SMALL;
    EnItem00_3DItemsDraw(&sPickup.actor, &sPlay);
    CheckStream("3D ground small magic", sOpa, sGfx.polyOpa.p, gGiMagicJarSmallDL, tintMagic, sMagicColor);
    Reset();
    sPickup.actor.params = ITEM00_MAGIC_JAR_BIG;
    EnItem00_3DItemsDraw(&sPickup.actor, &sPlay);
    CheckStream("3D ground large magic", sOpa, sGfx.polyOpa.p, gGiMagicJarLargeDL, tintMagic, sMagicColor);
    Reset();
    bool should = true;
    sSlime.actor.params = EN_SLIME_TYPE_RED;
    DrawSlime3DItem(&sSlime.actor, &should);
    sFailures += should; // The 3D hook must replace the sprite draw.
    CheckStream("red ChuChu 3D heart", sXlu, sGfx.polyXlu.p, gGiRecoveryHeartDL, tintHearts, sHeartsColor);
    Reset();
    should = true;
    sSlime.actor.params = EN_SLIME_TYPE_GREEN;
    DrawSlime3DItem(&sSlime.actor, &should);
    sFailures += should;
    CheckStream("green ChuChu 3D magic", sOpa, sGfx.polyOpa.p, gGiMagicJarSmallDL, tintMagic, sMagicColor);
}
static void CheckDropShimmer(void) {
    for (int enabled = 0; enabled <= 1; ++enabled) {
        sCollectibleShimmer = enabled;
        Reset();
        EnItem00_DrawHeartPiece(&sPickup, &sPlay);
        sFailures += sShimmerDraws != enabled || sSampledProfile != (enabled ? CW_COLLECTIBLE_HEART : -1) ||
                     sDropScale != 1.0f || sMatrixDepth != 0 || (enabled && sShimmerScale != 21.0f);
        ++sCases;
        Reset();
        EnItem00_DrawHeartContainer(&sPickup, &sPlay);
        sFailures += sShimmerDraws != enabled || sSampledProfile != (enabled ? CW_COLLECTIBLE_HEART : -1) ||
                     sDropScale != 20.0f || sMatrixDepth != 0 || (enabled && sShimmerScale != 20.0f);
        ++sCases;
    }
    sCollectibleShimmer = 0;
}
int main(void) {
    sCustom = 1;
    sHeartsChanged = 1;
    sMagicChanged = 1;
    DrawCases(1, 1);
    sHeartsColor = (Color_RGB8){ 245, 41, 79 };
    sMagicColor = (Color_RGB8){ 12, 219, 234 };
    DrawCases(1, 1); // Fresh CVar values, including rainbow updates, without resource reload.
    sHeartsChanged = 0;
    DrawCases(0, 1);
    sHeartsChanged = 1;
    sMagicChanged = 0;
    DrawCases(1, 0);
    sHeartsChanged = 0;
    DrawCases(0, 0); // Reset preserves original materials.
    sCustom = 0;
    sHeartsChanged = 1;
    sMagicChanged = 1;
    DrawCases(0, 0);
    sFirstAltDL = gGiHeartPieceDL;
    Reset();
    GetItem_DrawXlu01(&sPlay, GID_HEART_PIECE);
    CheckStream("first Alt heart after warm native", sXlu, sGfx.polyXlu.p, gGiHeartPieceDL, 1, sHeartsColor);
    sCustom = 0;
    sFirstAltDL = gGiMagicJarSmallDL;
    Reset();
    GetItem_DrawOpa0(&sPlay, GID_MAGIC_JAR_SMALL);
    CheckStream("first Alt magic jar after warm native", sOpa, sGfx.polyOpa.p, gGiMagicJarSmallDL, 1, sMagicColor);
    CheckDropShimmer();
    printf("%s custom item color draw state: %d cases, %d failures\n", sFailures ? "FAIL" : "PASS", sCases, sFailures);
    return sFailures != 0;
}
