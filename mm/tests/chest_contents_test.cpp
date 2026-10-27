#include <cmath>
#include <cstring>
#include <vector>
#include "tests/test_require.h"
#include "2s2h/Rando/StaticData/StaticData.h"
#include "assets/2s2h_assets.h"
extern "C" {
#include "src/overlays/actors/ovl_En_Box/z_en_box.h"
#include "2s2h/Enhancements/Graphics/ChestContents.h"
void EnBox_Draw(Actor*, PlayState*);
Gfx* EnBox_SetRenderMode1(GraphicsContext*);
Gfx* EnBox_SetRenderMode2(GraphicsContext*);
Gfx* EnBox_SetRenderMode3(GraphicsContext*);
}

#define IS_RANDO (gSaveContext.save.shipSaveInfo.saveType == SAVETYPE_RANDO)
#define RANDO_SAVE_CHECKS gSaveContext.save.shipSaveInfo.rando.randoSaveChecks
#define ENBOX_RC (actor->home.rot.x)

SaveContext gSaveContext;
static Gfx opa[256], xlu[256], loadedResource;
static GraphicsContext gfxContext;
static PlayState play;
PlayState* gPlayState = &play;
static Mtx matrix;
static int matchSize, matchStyle, randoStyle, probes;
static const char* missingResource;
static const char* failedResource;
static float drawScale;
static RandoItemType resolvedType = RITYPE_MAJOR;
static RandoItemId disguise = RI_HOOKSHOT;
static RandoItemId convertedDisguise = RI_QUIVER_40;
static RandoCheckId lastCheck = RC_UNKNOWN;
static RandoItemId lastItem = RI_UNKNOWN;
static RandoCheckId initialCheck = RC_UNKNOWN;

namespace Rando {
namespace StaticData {
std::map<RandoItemId, RandoStaticItem> Items;
RandoStaticCheck GetCheckFromFlag(FlagType type, s32 flag, s16 scene) {
    REQUIRE(type == FLAG_CYCL_SCENE_CHEST && flag == 3 && scene == SCENE_SEA);
    return { .randoCheckId = initialCheck };
}
} // namespace StaticData
RandoItemType GetItemTypeForCheck(RandoItemId item, RandoCheckId check) {
    lastCheck = check;
    lastItem = item;
    return resolvedType;
}
RandoItemId CurrentTrapItem(RandoCheckId check) {
    lastCheck = check;
    return disguise;
}
RandoItemId ConvertItem(RandoItemId item, RandoCheckId check) {
    REQUIRE(item == RI_PROGRESSIVE_BOW && check == RC_GREAT_BAY_TEMPLE_MAP_CHEST);
    return convertedDisguise;
}
} // namespace Rando

extern "C" {
void gSPDisplayList(Gfx* command, Gfx* list) {
    __gSPDisplayList(command, list);
}
void gSPSegment(void* command, int segment, uintptr_t target) {
    __gSPSegment((Gfx*)command, segment, target);
}
s32 Flags_GetSwitch(PlayState*, s32) {
    return 0;
}
int32_t CVarGetInteger(const char* name, int32_t fallback) {
    if (!strcmp(name, "gEnhancements.ChestSizeMatchesContentsMM"))
        return matchSize;
    if (!strcmp(name, "gEnhancements.ChestStyleMatchesContentsMM"))
        return matchStyle;
    if (!strcmp(name, "gRando.CSMC"))
        return randoStyle;
    return fallback;
}
uint8_t ResourceMgr_FileExists(const char* name) {
    ++probes;
    return !missingResource || !strstr(name, missingResource);
}
Gfx* ResourceMgr_LoadGfxByName(const char* name) {
    ++probes;
    return failedResource && strstr(name, failedResource) ? nullptr : &loadedResource;
}
void Matrix_Scale(float x, float y, float z, MatrixMode mode) {
    REQUIRE(x == y && x == z && mode == MTXMODE_APPLY);
    drawScale *= x;
}
Mtx* Matrix_Finalize(GraphicsContext*) {
    return &matrix;
}
void Graph_OpenDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) {
}
void Graph_CloseDisps(Gfx**, Gfx*, GraphicsContext*, const char*, s32) {
}
void FrameInterpolation_RecordOpenChild(const void*, int) {
}
void FrameInterpolation_RecordCloseChild(void) {
}
void Gfx_SetupDL25_Opa(GraphicsContext*) {
}
void Gfx_SetupDL25_Xlu(GraphicsContext*) {
}
void* Graph_Alloc(GraphicsContext*, size_t size) {
    static Gfx buffer[8];
    REQUIRE(size <= sizeof(buffer));
    return buffer;
}
Gfx* SkelAnime_Draw(PlayState* p, void**, Vec3s*, OverrideLimbDraw, PostLimbDraw post, Actor* actor, Gfx* gfx) {
    Gfx* unused = nullptr;
    Vec3s rotation = {};
    post(p, OBJECT_BOX_CHEST_LIMB_01, &unused, &rotation, actor, &gfx);
    post(p, OBJECT_BOX_CHEST_LIMB_03, &unused, &rotation, actor, &gfx);
    return gfx;
}
}

#include "chest_rando_production.inc"

static EnBox Chest(int type, int item) {
    EnBox chest = {};
    chest.type = type;
    chest.getItemId = item;
    chest.alpha = 255;
    chest.dyna.actor.params = ENBOX_PARAMS(type, item, 3);
    chest.dyna.actor.scale = { 0.0075f, 0.0075f, 0.0075f };
    return chest;
}

static std::vector<const void*> Draw(EnBox& chest, bool rando = false, bool translucent = false) {
    const auto before = chest;
    memset(opa, 0, sizeof(opa));
    memset(xlu, 0, sizeof(xlu));
    gfxContext.polyOpa.p = opa;
    gfxContext.polyOpa.d = opa + 256;
    gfxContext.polyXlu.p = xlu;
    play.state.gfxCtx = &gfxContext;
    drawScale = 1.0f;
    (rando ? EnBox_RandoDraw : EnBox_Draw)(&chest.dyna.actor, &play);
    REQUIRE(chest.type == before.type && chest.getItemId == before.getItemId);
    REQUIRE(chest.dyna.actor.params == before.dyna.actor.params);
    REQUIRE(!memcmp(&chest.dyna.actor.scale, &before.dyna.actor.scale, sizeof(Vec3f)));
    REQUIRE(chest.actionFunc == before.actionFunc && chest.unk_1EC == before.unk_1EC);
    std::vector<const void*> lists;
    for (Gfx* command = translucent ? xlu : opa; command < (translucent ? gfxContext.polyXlu.p : gfxContext.polyOpa.p);
         ++command) {
        if (command->words.w0 >> 24 == G_DL)
            lists.push_back((void*)command->words.w1);
    }
    return lists;
}

static void ExpectModel(const std::vector<const void*>& lists, const char* name) {
    REQUIRE(lists.size() == 2);
    const std::string base = "__OTR__objects/object_box/cor_3ds_chests_mm/";
    REQUIRE(!strcmp((const char*)lists[0], (base + "gChestBody" + name + "DL").c_str()));
    REQUIRE(!strcmp((const char*)lists[1], (base + "gChestLid" + name + "DL").c_str()));
}

int main() {
    setbuf(stdout, nullptr);
    play.sceneId = SCENE_SEA;
    EnBox chest = Chest(ENBOX_TYPE_SMALL, GI_HOOKSHOT);
    auto lists = Draw(chest);
    REQUIRE(lists.size() == 2 && !strcmp((const char*)lists[0], gBoxChestBaseDL));
    REQUIRE(drawScale == 1.0f && probes == 0);
    matchStyle = 1;
    ExpectModel(Draw(chest), "Major");
    REQUIRE(drawScale == 1.0f);
    puts("PASS: disabled default and independent optional style selection");

    const struct {
        int item;
        const char* model;
        float scale;
    } categories[] = {
        { GI_HOOKSHOT, "Major", 4.0f / 3.0f },
        { GI_WALLET_ADULT, "Major", 4.0f / 3.0f },
        { GI_POWDER_KEG, "Major", 4.0f / 3.0f },
        { GI_POTION_RED_BOTTLE, "Major", 4.0f / 3.0f }, // NEI's native Longshot slot
        { GI_MAP, "Minor", 4.0f / 3.0f },
        { GI_HEART_PIECE, "Heart", 4.0f / 3.0f },
        { GI_KEY_SMALL, "SmallKey", 1.0f },
        { GI_SKULL_TOKEN, "Token", 1.0f },
        { GI_RUPEE_RED, "Junk", 1.0f },
    };
    matchSize = 1;
    for (const auto& category : categories) {
        chest = Chest(ENBOX_TYPE_SMALL, category.item);
        ExpectModel(Draw(chest), category.model);
        REQUIRE(fabsf(drawScale - category.scale) < 0.00001f);
    }
    for (int type = ENBOX_TYPE_BIG; type <= ENBOX_TYPE_SMALL_SWITCH_FLAG; ++type) {
        chest = Chest(type, GI_RUPEE_GREEN);
        Draw(chest);
        REQUIRE(fabsf(drawScale - (Actor_IsSmallChest(&chest) ? 1.0f : 0.75f)) < 0.00001f);
        matchSize = 0;
        Draw(chest);
        REQUIRE(drawScale == 1.0f);
        matchSize = 1;
    }
    puts("PASS: native categories and all 13 authored types; size changes only the draw matrix");

    chest = Chest(ENBOX_TYPE_SMALL, GI_HOOKSHOT);
    for (const char* part : { "BodyMajor", "LidMajor" }) {
        missingResource = part;
        lists = Draw(chest);
        REQUIRE(!strcmp((const char*)lists[0], gBoxChestBaseDL));
        REQUIRE(!strcmp((const char*)lists[1], gBoxChestLidDL));
        missingResource = nullptr;
        failedResource = part;
        lists = Draw(chest);
        REQUIRE(!strcmp((const char*)lists[0], gBoxChestBaseDL));
        REQUIRE(!strcmp((const char*)lists[1], gBoxChestLidDL));
        failedResource = nullptr;
        ExpectModel(Draw(chest), "Major");
    }
    for (int item : { GI_KEY_BOSS, GI_MASK_DEKU, GI_STRAY_FAIRY, GI_NONE, GI_0B, GI_ICE_TRAP }) {
        chest = Chest(ENBOX_TYPE_BIG_ORNATE, item);
        lists = Draw(chest);
        REQUIRE(!strcmp((const char*)lists[0], gBoxChestBaseOrnateDL));
    }
    chest = Chest(ENBOX_TYPE_SMALL, GI_KEY_BOSS);
    lists = Draw(chest);
    REQUIRE(!strcmp((const char*)lists[0], gBoxChestBaseOrnateDL));
    REQUIRE(!strcmp((const char*)lists[1], gBoxChestLidOrnateDL));
    chest.alpha = 120;
    lists = Draw(chest, false, true);
    REQUIRE(!strcmp((const char*)lists[0], gBoxChestBaseOrnateDL));
    play.sceneId = SCENE_TAKARAYA;
    chest = Chest(ENBOX_TYPE_SMALL, GI_HOOKSHOT);
    lists = Draw(chest);
    REQUIRE(!strcmp((const char*)lists[0], gBoxChestBaseDL) && drawScale == 1.0f);
    play.sceneId = SCENE_SEA;
    puts("PASS: pairwise missing/failed-resource fallback, boss/unknown categories and minigame concealment");

    chest.alpha = 120;
    lists = Draw(chest, false, true);
    REQUIRE(!strcmp((const char*)lists[0], gBoxChestBaseDL));
    chest = Chest(ENBOX_TYPE_SMALL_INVISIBLE, GI_HOOKSHOT);
    chest.dyna.actor.flags |= ACTOR_FLAG_REACT_TO_LENS;
    lists = Draw(chest, false, true);
    REQUIRE(!strcmp((const char*)lists[0], gBoxChestBaseDL));
    chest.dyna.actor.flags &= ~ACTOR_FLAG_REACT_TO_LENS;
    ExpectModel(Draw(chest), "Major");
    puts("PASS: fading and Lens draw passes preserve native transparency rendering");

    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
    chest = Chest(ENBOX_TYPE_SMALL, GI_RECOVERY_HEART);
    chest.dyna.actor.home.rot.x = RC_GREAT_BAY_TEMPLE_MAP_CHEST;
    auto& saved = RANDO_SAVE_CHECKS[chest.dyna.actor.home.rot.x];
    saved.shuffled = true;
    saved.randoItemId = RI_COMBO_FOREIGN;
    EnBox nativeChest = Chest(ENBOX_TYPE_SMALL, GI_HEART_PIECE);
    nativeChest.dyna.actor.home.rot.x = chest.dyna.actor.home.rot.x;
    bool should = true;
    RunRandoShouldActorInit(&nativeChest.dyna.actor, &should);
    ExpectModel(Draw(nativeChest), "Heart"); // authored home X is not a randomizer check identity
    REQUIRE(ENBOX_GET_ITEM(&nativeChest.dyna.actor) == GI_HEART_PIECE);
    REQUIRE(nativeChest.contentsRandoCheck == RC_UNKNOWN);
    initialCheck = RC_GREAT_BAY_TEMPLE_MAP_CHEST;
    RunRandoShouldActorInit(&chest.dyna.actor, &should);
    REQUIRE(ENBOX_GET_ITEM(&chest.dyna.actor) == GI_RECOVERY_HEART);
    REQUIRE(chest.contentsRandoCheck == RC_GREAT_BAY_TEMPLE_MAP_CHEST);
    resolvedType = RITYPE_BOSS_KEY;
    lists = Draw(chest); // new style checkbox works independently of randomizer CSMC
    REQUIRE(!strcmp((const char*)lists[0], gBoxChestBaseOrnateDL));
    REQUIRE(!strcmp((const char*)lists[1], gBoxChestLidOrnateDL));
    matchStyle = 0;
    randoStyle = 1;
    resolvedType = RITYPE_SMALL_KEY;
    ExpectModel(Draw(chest, true), "SmallKey");
    REQUIRE(lastItem == RI_COMBO_FOREIGN && lastCheck == chest.dyna.actor.home.rot.x);
    REQUIRE(saved.randoItemId == RI_COMBO_FOREIGN);
    const struct {
        RandoItemType type;
        const char* model;
    } randoCategories[] = {
        { RITYPE_MAJOR, "Major" },        { RITYPE_LESSER, "Minor" },          { RITYPE_HEALTH, "Heart" },
        { RITYPE_SMALL_KEY, "SmallKey" }, { RITYPE_SKULLTULA_TOKEN, "Token" }, { RITYPE_JUNK, "Junk" },
    };
    for (const auto& category : randoCategories) {
        resolvedType = category.type;
        ExpectModel(Draw(chest, true), category.model);
    }
    resolvedType = RITYPE_MAJOR;
    ExpectModel(Draw(chest), "Major"); // size/style still classify the shuffled reward in native draw
    missingResource = "LidMajor";
    lists = Draw(chest, true);
    REQUIRE(lists.size() == 2 && lists[0] == gBoxChestBaseCopyDL && lists[1] == gBoxChestLidCopyDL);
    missingResource = nullptr;
    chest.alpha = 120;
    lists = Draw(chest, true, true);
    REQUIRE(lists.size() == 2 && lists[0] == gBoxChestBaseCopyDL && lists[1] == gBoxChestLidCopyDL);
    chest.alpha = 255;
    play.sceneId = SCENE_TAKARAYA;
    lists = Draw(chest, true);
    REQUIRE(!strcmp((const char*)lists[0], gBoxChestBaseDL) && drawScale == 1.0f);
    play.sceneId = SCENE_SEA;
    randoStyle = matchSize = 0;
    probes = 0;
    lists = Draw(chest);
    REQUIRE(!strcmp((const char*)lists[0], gBoxChestBaseDL) && drawScale == 1.0f && probes == 0);
    randoStyle = matchSize = 1;
    chest.unk_1EC = 1;
    ExpectModel(Draw(chest, true), "Major");
    resolvedType = RITYPE_JUNK;
    ExpectModel(Draw(chest, true), "Major"); // freeze appearance through reward collection
    chest.unk_1EC = 0;
    chest.dyna.actor.home.rot.z = 0;
    saved.randoItemId = RI_TRAP;
    Rando::StaticData::Items[RI_HOOKSHOT].randoItemType = RITYPE_MAJOR;
    ExpectModel(Draw(chest, true), "Major");
    disguise = RI_PROGRESSIVE_BOW;
    Rando::StaticData::Items[disguise].randoItemType = RITYPE_MAJOR;
    Rando::StaticData::Items[RI_QUIVER_40].randoItemType = RITYPE_LESSER;
    ExpectModel(Draw(chest, true), "Minor");
    convertedDisguise = RI_JUNK;
    Rando::StaticData::Items[RI_JUNK].randoItemType = RITYPE_JUNK;
    ExpectModel(Draw(chest, true), "Junk");
    disguise = RI_HOOKSHOT;
    ExpectModel(Draw(chest, true), "Major"); // concrete disguise is still drawn as a hookshot when owned
    disguise = RI_GREAT_BAY_BOSS_KEY;
    Rando::StaticData::Items[disguise].randoItemType = RITYPE_BOSS_KEY;
    lists = Draw(chest, true);
    REQUIRE(lists.size() == 2 && !strcmp((const char*)lists[0], gBoxChestBaseOrnateDL));
    saved.randoItemId = RI_COMBO_FOREIGN;
    resolvedType = RITYPE_MASK;
    lists = Draw(chest, true);
    REQUIRE(lists.size() == 2 && lists[0] == gBoxChestBaseOrnateCopyDL);
    resolvedType = RITYPE_MAX;
    lists = Draw(chest, true);
    REQUIRE(lists.size() == 2 && lists[0] == gBoxChestBaseCopyDL && drawScale == 1.0f);
    chest.contentsRandoCheck = -1;
    Draw(chest, true); // invalid actor check never indexes the randomizer save
    chest.contentsRandoCheck = RC_MAX;
    Draw(chest, true);
    puts("PASS: randomizer/foreign rewards, opening latch, trap disguise, boss fallback and invalid checks");
}
