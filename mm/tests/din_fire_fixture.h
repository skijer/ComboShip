// Engine boundaries for the real MM Din modules. Gameplay structs and GBI are
// production headers; only resources, config, allocation, audio and matrices
// are fixtures. No game or asset archive is required.
#ifndef MM_DIN_FIRE_FIXTURE_H
#define MM_DIN_FIRE_FIXTURE_H
#include "global.h"
#include "din_fire_sword.h"
#include "din_fire_shield.h"
#include "soh/ResourceManagerHelpers.h"
#include "mods/nei_save.h"
#include "mods/forms/custom_forms.h"
#include "mods/items/logic/adult_link_render.h"
#include "2s2h/BenGui/CosmeticEditor.h"
#include "test_require.h"
#include <string.h>
#include "../../tests/fixtures/din_shield_vertices.h"

static GraphicsContext gfx;
static PlayState play;
static Player player;
static NeiSaveData nei;
static Gfx opa[2048], xlu[512], core[1], flame[1], surface[1], rim[1], blade[2], bracer[2], hand[1];
static Mtx matrices[16];
static MtxF currentMatrix, matrixStack[8], captured[16];
static unsigned matrixCount, matrixDepth;
static u8 pixels[4];
static int swordOption, damageOption, shieldOption, soundOption;
static int alt, assets, loadFailure, adult, customForm, hideSword, hideShield, bossOwner, kokiriUpgrade, fairyUpgrade;
static int customColors, audioRequests;
static int fourSwordOwner;
static u16 lastSound;
static const char* missing;
static const char* lastCorePath;
static Actor* soundActor;
// Isolate the selection-state boundary; core tests cover its inventory gates.
static int elementalSelection = -1;
static u8 elementalActive;
static u8 elementalOverride;
static u8 elementalAvailable = 1;
static int auraTrueMaster, resourceQueries;
SaveContext gSaveContext;
PlayState* gPlayState = &play;

void lusprintf(const char* file, int32_t line, int32_t level, const char* fmt, ...) {
}

uint8_t SwordElement_Get(uint16_t item) {
    return elementalSelection < 0 ? 0 : elementalSelection;
}
uint8_t SwordElement_HasSelection(uint16_t item) {
    return elementalSelection >= 0;
}
uint8_t SwordElement_Active(PlayState* state, Player* owner) {
    return elementalOverride ? elementalOverride : elementalActive;
}
uint8_t SwordElement_Override(void) {
    return elementalOverride;
}
uint8_t SwordElement_IsAvailable(uint8_t element) {
    return elementalAvailable;
}

int32_t CVarGetInteger(const char* key, int32_t fallback) {
    int value = -1;
    if (!strcmp(key, "gEnhancements.DinFireSword"))
        value = swordOption;
    if (!strcmp(key, "gEnhancements.DinFireSwordDamage"))
        value = damageOption;
    if (!strcmp(key, "gEnhancements.DinFireShield"))
        value = shieldOption;
    if (!strcmp(key, "gEnhancements.DinFireShieldSfx"))
        value = soundOption;
    return value < 0 ? fallback : value;
}
Color_RGBA8 CosmeticEditor_GetChangedColor(u8 r, u8 g, u8 b, u8 a, const char* id) {
    REQUIRE(!strcmp(id, "Custom.DinFireSwordCore") || !strcmp(id, "Custom.DinFireSwordOuter") ||
            !strcmp(id, "Custom.DinFireShieldCore") || !strcmp(id, "Custom.DinFireShieldOuter"));
    if (!customColors)
        return (Color_RGBA8){ r, g, b, a };
    return strstr(id, "Core") ? (Color_RGBA8){ 41, 241, 199, 255 } : (Color_RGBA8){ 12, 33, 177, 255 };
}
bool ResourceMgr_IsAltAssetsEnabled(void) {
    return alt;
}
uint8_t ResourceMgr_FileExists(const char* path) {
    ++resourceQueries;
    return assets && !(missing && strstr(path, missing));
}
Gfx* ResourceMgr_LoadGfxByName(const char* path) {
    ++resourceQueries;
    if (!ResourceMgr_FileExists(path) || loadFailure)
        return NULL;
    if (strstr(path, "CoreDL")) {
        lastCorePath = path;
        return core;
    }
    if (strstr(path, "FlameDL"))
        return flame;
    if (strstr(path, "SurfaceDL"))
        return surface;
    if (strstr(path, "RimDL"))
        return rim;
    if (strstr(path, "SwordDL"))
        return &blade[adult ? 0 : 1];
    if (strstr(path, "BracerDL"))
        return &bracer[adult ? 0 : 1];
    REQUIRE(false);
    return NULL;
}
void* ResourceGetDataByName(const char* path) {
    ++resourceQueries;
    if (!loadFailure && ResourceMgr_FileExists(path) && strstr(path, "/SurfaceVertices"))
        return shieldSurfaceVertices;
    if (!loadFailure && ResourceMgr_FileExists(path) && strstr(path, "/RimVertices"))
        return shieldRimVertices;
    return loadFailure || !ResourceMgr_FileExists(path) ? NULL : pixels;
}
size_t ResourceGetSizeByName(const char* path) {
    return strstr(path, "/RimVertices") ? sizeof(shieldRimVertices) : sizeof(shieldSurfaceVertices);
}
NeiSaveData* Nei_Save(void) {
    return &nei;
}
// Effective render ownership intentionally differs from the saved fields in
// the forced-form/forced-adult cases, catching accidental raw-save queries.
s32 CustomForms_ActiveForm(void) {
    return customForm;
}
s32 AdultLink_IsActive(void) {
    return adult;
}
s32 AdultLink_UsesAdultPresentation(const Player* owner) {
    return adult;
}
u8 ExtEquip_ShouldHideSwordDL(void) {
    return hideSword;
}
u8 FourSword_IsEquipped(void) {
    return fourSwordOwner;
}
const char* ExtEquip_GetShieldDLOverride(void) {
    return hideShield ? "HIDE" : NULL;
}
s32 BossRemains_IsOdolwaWorn(void) {
    return bossOwner == 1;
}
s32 BossRemains_IsGohtWorn(void) {
    return bossOwner == 2;
}
u8 WeaponUpgrade_KokiriLevel(void) {
    return kokiriUpgrade;
}
u8 WeaponUpgrade_HasGreatFairy(void) {
    return fairyUpgrade;
}
u8 WeaponUpgrade_HasTrueMaster(void) {
    return auraTrueMaster;
}
u8 WeaponUpgrade_HasHammerAxe(void) {
    return 0;
}
void Actor_PlaySfx(Actor* actor, u16 sound) {
    ++audioRequests;
    soundActor = actor;
    lastSound = sound;
}
void Gfx_SetupDL25_Opa(GraphicsContext* context) {
    (void)context;
}
void Gfx_SetupDL25_Xlu(GraphicsContext* context) {
    (void)context;
}
void Graph_OpenDisps(Gfx** refs, Gfx* vals, GraphicsContext* context, const char* file, s32 line) {
    (void)refs;
    (void)vals;
    (void)context;
    (void)file;
    (void)line;
}
void Graph_CloseDisps(Gfx** refs, Gfx* vals, GraphicsContext* context, const char* file, s32 line) {
    (void)refs;
    (void)vals;
    (void)context;
    (void)file;
    (void)line;
}
void FrameInterpolation_RecordOpenChild(const void* key, int id) {
    (void)key;
    (void)id;
}
void FrameInterpolation_RecordCloseChild(void) {
}
void gSPDisplayList(Gfx* packet, Gfx* list) {
    __gSPDisplayList(packet, list);
}
void Matrix_Push(void) {
    REQUIRE(matrixDepth < ARRAY_COUNT(matrixStack));
    matrixStack[matrixDepth++] = currentMatrix;
}
void Matrix_Pop(void) {
    REQUIRE(matrixDepth > 0);
    currentMatrix = matrixStack[--matrixDepth];
}
void Matrix_Translate(f32 x, f32 y, f32 z, MatrixMode mode) {
    (void)mode;
    currentMatrix.xw += currentMatrix.xx * x + currentMatrix.xy * y + currentMatrix.xz * z;
    currentMatrix.yw += currentMatrix.yx * x + currentMatrix.yy * y + currentMatrix.yz * z;
    currentMatrix.zw += currentMatrix.zx * x + currentMatrix.zy * y + currentMatrix.zz * z;
}
void Matrix_Scale(f32 x, f32 y, f32 z, MatrixMode mode) {
    (void)mode;
    currentMatrix.xx *= x;
    currentMatrix.yx *= x;
    currentMatrix.zx *= x;
    currentMatrix.xy *= y;
    currentMatrix.yy *= y;
    currentMatrix.zy *= y;
    currentMatrix.xz *= z;
    currentMatrix.yz *= z;
    currentMatrix.zz *= z;
}
void Matrix_Get(MtxF* dest) {
    *dest = currentMatrix;
}
void Matrix_Put(MtxF* src) {
    currentMatrix = *src;
}
Mtx* Matrix_Finalize(GraphicsContext* context) {
    (void)context;
    REQUIRE(matrixCount < ARRAY_COUNT(matrices));
    captured[matrixCount] = currentMatrix;
    return &matrices[matrixCount++];
}
static void ResetBuffers(void) {
    memset(opa, 0, sizeof(opa));
    memset(xlu, 0, sizeof(xlu));
    gfx.polyOpa.p = opa;
    gfx.polyOpa.d = opa + ARRAY_COUNT(opa);
    gfx.polyXlu.p = xlu;
    matrixCount = matrixDepth = 0;
}
static void Setup(void) {
    static s16 scene;
    memset(&play, 0, sizeof(play));
    memset(&player, 0, sizeof(player));
    memset(&gfx, 0, sizeof(gfx));
    memset(&nei, 0, sizeof(nei));
    memset(&gSaveContext, 0, sizeof(gSaveContext));
    memset(&currentMatrix, 0, sizeof(currentMatrix));
    currentMatrix.xx = currentMatrix.yy = currentMatrix.zz = currentMatrix.ww = 1.0f;
    play.state.gfxCtx = &gfx;
    play.sceneId = ++scene;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    player.actor.id = ACTOR_PLAYER;
    player.actor.category = ACTORCAT_PLAYER;
    player.actor.scale.y = .01f;
    player.transformation = PLAYER_FORM_HUMAN;
    player.leftHandType = PLAYER_MODELTYPE_LH_ONE_HAND_SWORD;
    player.itemAction = player.heldItemAction = PLAYER_IA_SWORD_KOKIRI;
    player.heldItemId = ITEM_SWORD_KOKIRI;
    player.currentShield = PLAYER_SHIELD_HEROS_SHIELD;
    player.rightHandType = PLAYER_MODELTYPE_RH_SHIELD;
    player.stateFlags1 = PLAYER_STATE1_400000;
    swordOption = damageOption = shieldOption = soundOption = -1;
    alt = assets = 1;
    loadFailure = adult = hideSword = hideShield = bossOwner = kokiriUpgrade = fairyUpgrade = 0;
    fourSwordOwner = 0;
    elementalSelection = -1;
    elementalActive = 0;
    elementalOverride = 0;
    elementalAvailable = 1;
    auraTrueMaster = resourceQueries = 0;
    customForm = CUSTOM_FORM_NONE;
    customColors = audioRequests = 0;
    soundActor = NULL;
    missing = lastCorePath = NULL;
    ResetBuffers();
}
static int HasLayer(Gfx* begin, Gfx* end, Gfx* list) {
    for (Gfx* p = begin; p < end; ++p)
        if ((p->words.w0 >> 24) == G_DL && p->words.w1 == (uintptr_t)list)
            return true;
    return false;
}
static int HasColor(Gfx* begin, Gfx* end, int opcode, u32 rgb) {
    for (Gfx* p = begin; p < end; ++p)
        if ((p->words.w0 >> 24) == opcode && ((p->words.w1 >> 8) & 0xFFFFFF) == rgb)
            return true;
    return false;
}
static void RequireNamedTexture(Gfx* begin, Gfx* end, const char* path) {
    int count = 0;
    for (Gfx* p = begin; p < end; ++p) {
        if ((p->words.w0 >> 24) == G_SETTIMG) {
            REQUIRE(p->words.w1 != 0 && !(p->words.w1 & 1));
            if (!strcmp((const char*)p->words.w1, path))
                ++count;
        }
    }
    REQUIRE(count == 1);
}
#endif
