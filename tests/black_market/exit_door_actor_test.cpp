#include "global.h"
#include "overlays/actors/ovl_En_Door/z_en_door.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include "objects/gameplay_field_keep/gameplay_field_keep.h"
#include "BlackMarketExitDoor.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>

static PlayState play;
static Player player;
static EnDoor door;
static TransitionActorEntry entry;
SaveContext gSaveContext;
static int spawns, animationStarts;
static AnimationHeader* playedAnimation;
static bool cutscene;
static constexpr bool oot =
#ifdef EXIT_DOOR_OOT
    true;
#else
    false;
#endif
Actor& DoorActor() {
#ifdef EXIT_DOOR_OOT
    return door.actor;
#else
    return door.knobDoor.dyna.actor;
#endif
}
static void ActorCallback(Actor*, PlayState*) {}
extern "C" f32 Math_SinS(s16 angle) { return std::sin(angle * (6.28318530718f / 65536)); }
extern "C" f32 Math_CosS(s16 angle) { return std::cos(angle * (6.28318530718f / 65536)); }
extern "C" s16 Math_Vec3f_Yaw(Vec3f* a, Vec3f* b) {
    return static_cast<s16>(std::atan2(b->x - a->x, b->z - a->z) * (65536 / 6.28318530718f));
}
extern "C" void Animation_PlayOnceSetSpeed(SkelAnime*, AnimationHeader* animation, f32 speed) {
    assert(speed == 1.5f); playedAnimation = animation; ++animationStarts;
}
extern "C" void Flags_SetSwitch(PlayState*, s32) { assert(false && "exit doors cannot spend a key"); }
void EnDoor_Open(EnDoor*, PlayState*) {}
void EnDoor_AjarOpen(EnDoor*, PlayState*) { assert(false); }
void EnDoor_OpenScheduleActor(EnDoor*, PlayState*) { assert(false); }
#define GameInteractor_Should(feature, defaultValue, ...) (defaultValue)
#define DOOR_AJAR_OPEN_RANGE 240.0f

#ifdef EXIT_DOOR_OOT
extern "C" s32 Player_InCsMode(PlayState*) { return cutscene; }
extern "C" void Audio_PlayActorSound2(Actor*, u16) { assert(false); }
void GameInteractor_ExecuteOnDungeonKeyUsedHooks(s16) { assert(false); }
AnimationHeader* D_809FCECC[4];
Gfx* D_809FCEE4[5][2];
#else
extern "C" bool Player_InCsMode(PlayState*) { return cutscene; }
extern "C" s32 Actor_TalkOfferAccepted(Actor*, GameState*) { return false; }
extern "C" void Actor_PlaySfx(Actor*, u16) { assert(false); }
extern "C" void func_80122F28(Player*) {}
extern "C" s32 Schedule_RunScript(PlayState*, u8*, ScheduleOutput*) { assert(false); return false; }
extern "C" int ComboBlackMarket_IsAuthoredDoor(PlayState*, Actor*) { return false; }
AnimationHeader* sAnimations[10];
Gfx* sDoorDLists[14][2];
static bool sDoorIsMilkBarMember;
static u8* sDoorSchedules[32];
/* NATIVE HALF DAYS */
#endif

Actor* Spawn(s16 id, f32 x, f32 y, f32 z, s16 yaw, s16 params) {
    assert(id == ACTOR_EN_DOOR); ++spawns; door = {};
    auto& actor = DoorActor();
    actor.id = id; actor.category = ACTORCAT_DOOR; actor.params = params; actor.room = 0;
    actor.world.pos = actor.home.pos = { x, y, z };
    actor.home.rot.y = actor.shape.rot.y = yaw;
    actor.update = actor.draw = ActorCallback;
#ifdef EXIT_DOOR_OOT
    play.actorCtx.actorLists[ACTORCAT_DOOR].head = &actor;
#else
    play.actorCtx.actorLists[ACTORCAT_DOOR].first = &actor;
    door.doorType = ENDOOR_GET_TYPE(&actor);
    door.typeVar.data = ENDOOR_GET_TYPE_VAR(&actor);
#endif
    return &actor;
}
#ifdef EXIT_DOOR_OOT
extern "C" Actor* Actor_Spawn(ActorContext*, PlayState*, s16 id, f32 x, f32 y, f32 z, s16 rx,
                              s16 yaw, s16 rz, s16 params) {
    assert(rx == 0 && rz == 0); return Spawn(id, x, y, z, yaw, params);
}
#else
extern "C" Actor* Actor_SpawnAsChildAndCutscene(ActorContext*, PlayState*, s16 id, f32 x, f32 y, f32 z,
    s16 rx, s16 yaw, s16 rz, s32 params, u32 cs, u32 halfDays, Actor* parent) {
    assert(rx == 0 && rz == 0 && cs == 127 && halfDays == HALFDAYBIT_ALL && !parent);
    return Spawn(id, x, y, z, yaw, params);
}
extern "C" void DynaPoly_DeleteBgActor(PlayState*, DynaCollisionContext*, s32) { assert(false); }
#endif

/* NATIVE FUNCTIONS */

int main() {
    play.roomCtx.curRoom.num = 0; play.roomCtx.prevRoom.num = -1;
    entry.id = ACTOR_EN_DOOR; entry.pos = { -10, 0, 420 };
    entry.sides[0].room = 0; entry.sides[1].room = -1;
#ifdef EXIT_DOOR_OOT
    entry.params = 3 << 7;
    entry.sides[0].effects = entry.sides[1].effects = -1;
    play.transiActorCtx.list = &entry; play.transiActorCtx.numActors = 1;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
    auto* animations = D_809FCECC;
    auto* lists = D_809FCEE4;
    constexpr int formCount = 2;
#else
    entry.rotY = 127; entry.params = 7;
    entry.sides[0].bgCamIndex = entry.sides[1].bgCamIndex = -1;
    play.transitionActors.list = &entry; play.transitionActors.count = 1;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    auto* animations = sAnimations;
    auto* lists = sDoorDLists;
    constexpr int formCount = PLAYER_FORM_MAX;
#endif
    AnimationHeader animationMarkers[10]{};
    for (int i = 0; i < (oot ? 4 : 10); ++i) animations[i] = &animationMarkers[i];
    lists[0][0] = reinterpret_cast<Gfx*>(const_cast<char*>(gDoorLeftDL));
    lists[0][1] = reinterpret_cast<Gfx*>(const_cast<char*>(gDoorRightDL));
    for (int reload = 0; reload < 3; ++reload) {
        Actor_SpawnTransitionActors(&play, &play.actorCtx);
        assert(spawns == reload + 1 && entry.id == -ACTOR_EN_DOOR);
        assert(BlackMarketExitDoor::Live<oot>(&play) == &DoorActor());
        Actor_SpawnTransitionActors(&play, &play.actorCtx);
        assert(spawns == reload + 1 && "native negative marker prevents duplicate spawning");
        for (int form = 0; form < formCount; ++form) {
            for (int side : { -1, 1 }) {
                player.doorType = PLAYER_DOORTYPE_NONE;
                player.actor.world.pos = { -10, 0, 420.f + side * 39.f };
                player.actor.shape.rot.y = side < 0 ? 0 : -32768;
                cutscene = false;
#ifndef EXIT_DOOR_OOT
                player.transformation = form;
                for (s16 halfDay : { HALFDAYBIT_DAY1_DAWN, HALFDAYBIT_DAY1_NIGHT, HALFDAYBIT_DAY2_DAWN,
                                     HALFDAYBIT_DAY2_NIGHT, HALFDAYBIT_DAY3_DAWN, HALFDAYBIT_DAY3_NIGHT }) {
                    play.actorCtx.halfDaysBit = halfDay;
                    EnDoor_Idle(&door, &play);
                    assert(player.doorType == PLAYER_DOORTYPE_HANDLE);
                }
#else
                gSaveContext.linkAge = form;
#endif
                EnDoor_Idle(&door, &play);
                assert(player.doorActor == &DoorActor() && player.doorType == PLAYER_DOORTYPE_HANDLE &&
                       player.doorDirection == side);
#ifdef EXIT_DOOR_OOT
                door.animStyle = (side < 0 ? 0 : 2) + (LINK_IS_ADULT ? 0 : 1);
                door.playerIsOpening = true;
                const int selectedAnimation = door.animStyle;
#else
                door.knobDoor.animIndex = form + (side > 0 ? PLAYER_FORM_MAX : 0);
                door.knobDoor.requestOpen = true;
                const int selectedAnimation = door.knobDoor.animIndex;
#endif
                const int before = animationStarts;
                EnDoor_Idle(&door, &play);
                assert(animationStarts == before + 1 && playedAnimation == animations[selectedAnimation]);
#ifdef EXIT_DOOR_OOT
                door.playerIsOpening = false;
#else
                door.knobDoor.requestOpen = false;
#endif
            }
        }
        // Actual native limb callback must select a concrete door display list
        // using this valid transition record, without a room-load camera.
        Vec3s jointTable[5]{}; Vec3s rotation{}; Vec3f position{}; Gfx* selected = nullptr;
#ifdef EXIT_DOOR_OOT
        door.skelAnime.jointTable = jointTable;
#else
        door.knobDoor.skelAnime.jointTable = jointTable;
#endif
        EnDoor_OverrideLimbDraw(&play, 4, &selected, &position, &rotation, &DoorActor());
        assert(selected == lists[0][0]);
        EnDoor_Destroy(&DoorActor(), &play);
        assert(entry.id == ACTOR_EN_DOOR && "native destructor re-arms the scene door for reload");
#ifdef EXIT_DOOR_OOT
        play.actorCtx.actorLists[ACTORCAT_DOOR].head = nullptr;
#else
        play.actorCtx.actorLists[ACTORCAT_DOOR].first = nullptr;
#endif
        assert(!BlackMarketExitDoor::Live<oot>(&play));
    }
    std::puts("PASS native door actors: actual scene spawn/proximity, all host forms/ages and both sides, animation dispatch, concrete door limb list, duplicate prevention and three teardown/reload cycles");
}
