// The authoritative OoT donor, with native headers/cache/magic/slab code.
#include "z64.h"
#include "functions.h"
#include "variables.h"
#include "macros.h"
#include "mods/items/helpers/equip_helper.h"
#include "mods/items/helpers/fx_helper.h"
#include "mods/extended_inventory.h"
#include "mods/extended_player.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>
#include <algorithm>

SaveContext gSaveContext{};
PlayState* gPlayState;
f32 gSfxDefaultFreqAndVolScale = 1;
s8 gSfxDefaultReverb = 0;
std::vector<Actor*> actors;
CollisionHeader collision{};
bool onSlab = false;
#define WAND_WHEEL_HOLD_FRAMES 8
#define WAND_POSE_IDLE 0
static u8 sWandPoseStage = WAND_POSE_IDLE;
typedef struct {const char* iconPath; u8 iconSize; u8 enabled;} BoxMenuEntry;
typedef void (*BoxMenuConfirmFn)(s32);

s32 CVarGetInteger(const char*, s32) { return 1; }
u8 Sw97_IsBowItem(u8) { return 0; }
u8 Sw97_EffectiveElement(u8) { return 0; }
u8 Pacci_UltrahandModeActive() { return 0; }
u8 MasterCycle_IsRiding() { return 0; }
u8 Cryonis_ModeActive() { return 0; }
u8 ExtEquip_CapeOwned() { return 0; }
s32 Flags_GetRandomizerInf(RandomizerInf) { return 0; }
u8 Wand_ModeCount() { return 1; }
u8 Wand_GetMode() { return WAND_MODE_SAND; }
u8 Wand_ModeAt(u8) { return WAND_MODE_SAND; }
void Wand_SetMode(u8) {}
void* Wand_ModeIcon(u8) { return nullptr; }
void ExtInv_RefreshButtonIconsForItem(PlayState*, u16) {}
u8 BoxMenu_IsOpen() { return 0; }
u8 BoxMenu_Open(PlayState*, const BoxMenuEntry*, s32, s32, u16, BoxMenuConfirmFn) { return 0; }
void WandWater_Forget() {}
void WandShadow_Forget() {}
void WandStorm_Forget() {}
void WandShadow_Tick(PlayState*) {}
void WandStorm_Tick(PlayState*, Player*) {}
void WandWind_Tick(PlayState*, Player*) {}
void WandWind_TickHover(Player*, u8) {}
u8 WandWind_Cast(Player*, PlayState*) { return 0; }
u8 WandWater_Cast(Player*, PlayState*) { return 0; }
u8 WandMeteor_Cast(Player*, PlayState*) { return 0; }
u8 WandStorm_Cast(Player*, PlayState*) { return 0; }
u8 WandShadow_Cast(Player*, PlayState*) { return 0; }
void Wand_PoseStart(PlayState*, Player*, u8) {} // animation scheduling is a boundary
void Audio_PlaySoundGeneral(u16, Vec3f*, u8, f32*, f32*, s8*) {}
f32 Math_SinS(s16 yaw) { return std::sin(yaw * M_PI / 32768.0); }
f32 Math_CosS(s16 yaw) { return std::cos(yaw * M_PI / 32768.0); }
s32 Flags_GetSwitch(PlayState*, s32) { return 0; }
void Flags_SetSwitch(PlayState*, s32) {}
void Flags_UnsetSwitch(PlayState*, s32) {}
s32 Object_GetIndex(ObjectContext*, s16) { return 0; }
s32 Object_Spawn(ObjectContext*, s16) { return 0; }
void Actor_Kill(Actor* actor) { actor->update = nullptr; }
void Actor_SetScale(Actor* actor, f32 scale) { actor->scale = {scale, scale, scale}; }
s32 DynaPolyActor_IsPlayerOnTop(DynaPolyActor*) { return onSlab; }
void FX_SpawnRadialDust(PlayState*, Vec3f*, f32, f32, u8, FX_Color*) {}
void SoundSource_PlaySfxAtFixedWorldPos(PlayState*, Vec3f*, s32, u16) {}
static void WandSand_SlabDraw(Actor*, PlayState*) {}
void LiveActor(Actor*, PlayState*) {}
Actor* Actor_Spawn(ActorContext*, PlayState* play, s16 id, f32 x, f32 y, f32 z, s16, s16 yaw, s16, s16) {
    assert(id == ACTOR_OBJ_LIFT);
    auto* slab = new DynaPolyActor{};
    slab->actor.world.pos = {x, y, z}; slab->actor.update = LiveActor;
    slab->actor.shape.rot.y = yaw;
    while (std::any_of(actors.begin(), actors.end(), [&](Actor* other) {
        return other->update && ((DynaPolyActor*)other)->bgId == slab->bgId;
    })) ++slab->bgId;
    assert(slab->bgId < BG_ACTOR_MAX);
    play->colCtx.dyna.bgActors[slab->bgId].colHeader = &collision;
    actors.push_back(&slab->actor); return &slab->actor;
}

#include "oot_sand.inc"

static int SandParityMagic() { return gSaveContext.magic; }
static void SandParityRefill() { gSaveContext.magic = 48; }
static void ResetSandParity(Player& player, PlayState& play, u16 button, s16 yaw, float halfX) {
    for (Actor* actor : actors) delete (DynaPolyActor*)actor;
    actors.clear(); player = {}; play = {}; gSaveContext = {}; onSlab = false;
    WandSand_Forget(); sSandMeasured = 0; sSandTopOffset = sSandReach = 0;
    gPlayState = &play;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
    player.heldItemAction = PLAYER_IA_ELEMENTAL_WAND;
    player.actor.shape.rot.y = yaw;
    gSaveContext.magicCapacity = gSaveContext.magic = 48;
    std::memset(gSaveContext.equips.buttonItems, ITEM_NONE, sizeof(gSaveContext.equips.buttonItems));
    gSaveContext.equips.buttonItems[button == BTN_CLEFT ? 1 : 4] = ITEM_ELEMENTAL_WAND;
    collision.minBounds = {-(s16)(halfX / .05f), -60, -600};
    collision.maxBounds = {(s16)(halfX / .05f), 20, 600};
    player.heldItemAction = PLAYER_IA_NONE;
    ++play.gameplayFrames; Wand_TickInput(&play, &player);
    player.heldItemAction = PLAYER_IA_ELEMENTAL_WAND;
}

#include "sand_parity_scenario.h"

int main(int argc, char** argv) {
    if (argc > 1) {
        if (std::strcmp(argv[1], "sand-parity")) return 2;
        CheckSandParity();
        for (Actor* actor : actors) delete (DynaPolyActor*)actor;
        return 0;
    }
    Player player{}; PlayState play{};
    gPlayState = &play;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
    collision.minBounds = {-100, -60, -50}; collision.maxBounds = {100, 20, 50};
    for (u16 button : {BTN_CLEFT, BTN_DUP}) {
        for (Actor* actor : actors) delete (DynaPolyActor*)actor;
        actors.clear(); WandSand_Forget(); gSaveContext = {};
        gSaveContext.magicCapacity = gSaveContext.magic = 48;
        std::memset(gSaveContext.equips.buttonItems, ITEM_NONE, sizeof(gSaveContext.equips.buttonItems));
        gSaveContext.equips.buttonItems[button == BTN_CLEFT ? 1 : 4] = ITEM_ELEMENTAL_WAND;
        player.heldItemAction = PLAYER_IA_NONE; play.state.input[0] = {};
        ++play.sceneNum; ++play.gameplayFrames; Wand_TickInput(&play, &player);
        player.heldItemAction = PLAYER_IA_ELEMENTAL_WAND;
        play.state.input[0].press.button = play.state.input[0].cur.button = button;
        ++play.gameplayFrames; Wand_TickInput(&play, &player);
        assert(actors.empty() && gSaveContext.magic == 48 && "first draw must not cast Sand");
        play.state.input[0] = {}; ++play.gameplayFrames; Wand_TickInput(&play, &player);
        play.state.input[0].press.button = play.state.input[0].cur.button = button;
        ++play.gameplayFrames; Wand_TickInput(&play, &player);
        assert(actors.size() == 1 && gSaveContext.magic == 46 && "pressed Sand must be billed once");
        play.state.input[0].press.button = 0; player.stateFlags1 = PLAYER_STATE1_SHIELDING;
        player.actor.world.pos.z += 120;
        ++play.gameplayFrames; Wand_TickInput(&play, &player);
        assert(actors.size() == 1 && gSaveContext.magic == 46 && "held Sand must respect shielding");
        player.stateFlags1 = 0;
        ++play.gameplayFrames; Wand_TickInput(&play, &player);
        assert(actors.size() == 2 && gSaveContext.magic == 44);
        for (int frame = 0; frame < 12; ++frame) { ++play.gameplayFrames; Wand_TickInput(&play, &player); }
        assert(actors.size() == 2 && gSaveContext.magic == 44);
    }
    for (Actor* actor : actors) delete (DynaPolyActor*)actor;
    std::cout << "PASS SoH native Sand C/D first-draw, single press billing, shielding gate and normal held coverage\n";
}
