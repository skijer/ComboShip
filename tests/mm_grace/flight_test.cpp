#include "mods/items/logic/item_hylias_grace.h"
#include "mods/items/helpers/equip_helper.h"
#include "tests/test_require.h"
#include "macros.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/Rando/Rando.h"
#include "NeiGracePolicy.h"
#include "overlays/actors/ovl_En_Holl/z_en_holl.h"
#include <cmath>
#include <cstring>

#define linearVelocity speedXZ
CustomItemState gCustomItemState{};
SaveContext gSaveContext{};
static s8 sHGracePrevInvinc;
static s32 sHGPhaseEnd;
static u8 sHGraceRoomPending;
static s8 sHGraceForm = -1;
static Actor* sIvanActor;
u8 gIvanPossessActive;
static Vec3f sFairyPos, sFairyVelocity;
static u8 sFairyPosValid, sPinkFairySkelInited;
static float sFairyDimLevel;
static Camera camera{};
static ItemInputState nextInput{};
static int sparkleCount, spent, errorSounds, magic = 48;
static int roomRequests, roomFinishes, roomAccept = 1, roomReady;
static int floorExit, fadeCalls, voidCalls;
static FloorEffect floorEffect = FLOOR_EFFECT_0;
static CollisionPoly exitPoly{};

extern "C" {
void Player_Draw(Actor*, PlayState*) {}
void HGrace_DrawFairy(Actor*, PlayState*) {}
void HGrace_ResetLighting(PlayState*) {}
void HGrace_SpawnFairySparkles(Player*, PlayState*) { ++sparkleCount; }
void HGrace_SpawnTrailSparkles(Player*, PlayState*, f32) {}
void HGrace_StateCasting(Player*, PlayState*) {}
void HGrace_StateWarpEnter(Player*, PlayState*) {}
void HGrace_StateWarpExit(Player*, PlayState*) {}
void HGrace_StateIvan(Player*, PlayState*) {}
int HGrace_CanActivateMM(void);
s32 ItemMagic_HasEnough(PlayState*, s16 amount) { return magic >= amount; }
void ItemMagic_Consume(PlayState*, s16 amount) { spent += amount; magic -= amount; }
void Audio_PlaySoundGeneral(u16 sound, Vec3f*, u8, f32*, f32*, s8*) {
    REQUIRE(sound == NA_SE_SY_ERROR); ++errorSounds;
}
void ItemInput_Update(ItemInputState* out, u8, Player*, PlayState*) { *out = nextInput; }
u8 ItemInput_CheckDamage(Player*, s8*) { return 0; }
u8 ItemInput_IsBlocked(Player*, PlayState*) { return 0; }
void Actor_Kill(Actor* actor) { actor->update = nullptr; }
int func_8005B1A4(...) { return 0; }
Camera* Play_GetCamera(PlayState*, s16) { return &camera; }
s16 Camera_GetInputDirYaw(Camera*) { return camera.inputDir.y; }
s32 Camera_ChangeSetting(Camera*, s16) { return 1; }
s32 GameInteractor_InvertControl(GIInvertType) { return 1; }
f32 Math_SinS(s16 angle) { return std::sin(angle * (3.141592653589793 / 32768.0)); }
f32 Math_CosS(s16 angle) { return std::cos(angle * (3.141592653589793 / 32768.0)); }
s16 Math_Atan2S(f32 x, f32 y) { return std::atan2(x, y) * (32768.0 / 3.141592653589793); }
s16 Math_Atan2S_XY(f32 x, f32 y) { return std::atan2(y, x) * (32768.0 / 3.141592653589793); }
f32 BgCheck_EntityRaycastFloor5(CollisionContext*, CollisionPoly** out, s32* id, Actor*, Vec3f*) {
    *out = floorExit ? &exitPoly : nullptr; *id = BGCHECK_SCENE;
    return floorExit ? 0 : BGCHECK_Y_MIN;
}
u32 SurfaceType_GetSceneExitIndex(CollisionContext*, CollisionPoly*, s32) { return floorExit; }
int Entrance_OverrideNextIndex(...) { return 0; }
int Scene_SetTransitionForNextEntrance(...) { return 0; }
FloorEffect SurfaceType_GetFloorEffect(CollisionContext*, CollisionPoly*, s32) { return floorEffect; }
void Scene_SetExitFade(PlayState* play) { ++fadeCalls; play->transitionType = TRANS_TYPE_FADE_BLACK; }
void func_80169EFC(PlayState*) { ++voidCalls; }
s32 Room_RequestNewRoom(PlayState*, RoomContext* context, s32 room) {
    if (context->status || !roomAccept) return 0;
    ++roomRequests; context->prevRoom = context->curRoom;
    context->curRoom.num = room; context->status = 1; return 1;
}
s32 Room_ProcessRoomRequest(PlayState*, RoomContext* context) {
    if (!roomReady) return 0;
    context->status = 0; return 1;
}
void Room_FinishRoomChange(PlayState*, RoomContext* context) {
    REQUIRE(context->status == 0); ++roomFinishes; context->prevRoom.num = -1;
}
}

f32 gSfxDefaultFreqAndVolScale = 1;
s8 gSfxDefaultReverb = 0;

#include "flight_production.inc"

static void SeedPolicy(Player* player, PlayState* play) {
    auto& options = gSaveContext.save.shipSaveInfo.rando.randoSaveOptions;
    auto& oot = gSaveContext.save.shipSaveInfo.nei.ootQuestItems;
    auto& mm = gSaveContext.save.saveInfo.inventory.questItems;
    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
    player->actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    options[RO_HYLIAS_GRACE] = RO_GRACE_OFF;
    HGrace_Start(player, play);
    REQUIRE(!hgActive && spent == 0 && magic == 48 && errorSounds == 1);
    options[RO_HYLIAS_GRACE] = RO_GRACE_GATED;
    for (unsigned required = 0; required <= 13; ++required) {
        options[RO_HYLIAS_GRACE_REWARDS] = required;
        oot = 0xFFFFFFFFu; mm = 0xFFFFFFFFu;
        REQUIRE(HGrace_CanActivateMM());
        oot = mm = 0;
        REQUIRE(HGrace_CanActivateMM() == (required == 0));
    }
    REQUIRE(NeiGrace_RewardCount(0xFFFFFFFFu, 0xFFFFFFFFu) == 13);
    options[RO_HYLIAS_GRACE_REWARDS] = 1;
    oot = ~0x001C003Fu; mm = ~0xFu; // Songs and every non-reward flag.
    REQUIRE(!HGrace_CanActivateMM());
    options[RO_HYLIAS_GRACE_REWARDS] = 4;
    oot = 3; mm = 1; // Two medallions and one remains: still locked.
    HGrace_Start(player, play);
    REQUIRE(!hgActive && spent == 0 && magic == 48 && errorSounds == 2);
    mm = 3;
    HGrace_Start(player, play);
    REQUIRE(hgActive && hgState == HGRACE_STATE_CASTING && spent == 24 && magic == 24);
    HGrace_Stop(player, play);
    options[RO_HYLIAS_GRACE_REWARDS] = 14;
    oot = mm = 0xFFFFFFFFu;
    REQUIRE(!HGrace_CanActivateMM());
    options[RO_HYLIAS_GRACE] = 99;
    REQUIRE(!HGrace_CanActivateMM());
    options[RO_HYLIAS_GRACE] = RO_GRACE_ON; // Legacy zero-valued saved option.
    oot = mm = 0;
    REQUIRE(HGrace_CanActivateMM());
    options[RO_HYLIAS_GRACE] = RO_GRACE_OFF;
    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_VANILLA;
    REQUIRE(HGrace_CanActivateMM());
    puts("PASS seed-saved Grace policy, 0..13 distinct rewards, legacy/non-rando and locked casts cost no magic");
}

static void PreloadedHall(Player* player, PlayState* play) {
    TransitionActorEntry entry{};
    entry.id = -ACTOR_EN_HOLL;
    entry.sides[0].room = 0; entry.sides[1].room = 1;
    play->transiActorCtx.list = &entry; play->transiActorCtx.count = 1;
    play->roomList.count = 2; play->roomCtx.status = 0;
    play->roomCtx.curRoom.num = 1; play->roomCtx.prevRoom.num = 0;
    player->actor.world.pos = {0, 20, -20};
    const Vec3f position = player->actor.world.pos;
    REQUIRE(!HGrace_CheckDoorTransition(player, play));
    REQUIRE(roomRequests == 0 && roomFinishes == 0);
    REQUIRE(play->roomCtx.curRoom.num == 1 && play->roomCtx.prevRoom.num == 0);
    REQUIRE(!std::memcmp(&position, &player->actor.world.pos, sizeof(position)));
    REQUIRE(!HGrace_UpdateRoomChange(play));
    play->transiActorCtx.list = nullptr; play->transiActorCtx.count = 0;
    play->roomCtx.curRoom.num = 0; play->roomCtx.prevRoom.num = -1;
    player->actor.world.pos = {};
    puts("PASS Grace leaves native preloaded hall room ownership intact without duplicate requests");
}

static void VerticalHalls(Player* player, PlayState* play) {
    for (s16 type : {EN_HOLL_TYPE_VERTICAL, EN_HOLL_TYPE_VERTICAL_BG_COVER}) {
        for (s16 id : {static_cast<s16>(ACTOR_EN_HOLL), static_cast<s16>(-ACTOR_EN_HOLL)}) {
            for (int side : {0, 1}) {
                TransitionActorEntry entry{};
                entry.id = id; entry.params = type << 7;
                entry.sides[0].room = 0; entry.sides[1].room = 1;
                play->transiActorCtx.list = &entry; play->transiActorCtx.count = 1;
                play->roomList.count = 2; play->roomCtx.status = 0;
                play->roomCtx.curRoom.num = side; play->roomCtx.prevRoom.num = -1;
                player->actor.world.pos = side == 0 ? Vec3f{0, 70, 20} : Vec3f{0, -70, -20};
                const Vec3f position = player->actor.world.pos;
                REQUIRE(!HGrace_CheckDoorTransition(player, play));
                REQUIRE(roomRequests == 0 && roomFinishes == 0 && play->roomCtx.curRoom.num == side);
                REQUIRE(!std::memcmp(&position, &player->actor.world.pos, sizeof(position)));
                REQUIRE(!HGrace_UpdateRoomChange(play));
            }
        }
    }
    play->transiActorCtx.list = nullptr; play->transiActorCtx.count = 0;
    play->roomCtx.curRoom.num = 0; player->actor.world.pos = {};
    puts("PASS Grace defers both vertical hall types and directions without horizontal teleport or wrong room");
}

static void Rooms(Player* player, PlayState* play) {
    TransitionActorEntry entry{};
    entry.id = -ACTOR_EN_DOOR; // Native engine negates IDs of spawned doors.
    entry.rotY = (90 << 7) | 12; // Degrees packed above the cutscene ID.
    entry.sides[0].room = 0; entry.sides[1].room = 1;
    play->transiActorCtx.list = &entry; play->transiActorCtx.count = 1;
    play->roomList.count = 2; play->roomCtx.curRoom.num = 0;
    player->actor.world.pos = {20, 10, 0};
    play->roomCtx.status = 1;
    REQUIRE(!HGrace_CheckDoorTransition(player, play));
    REQUIRE(roomRequests == 0 && roomFinishes == 0 && player->actor.world.pos.x == 20);
    play->roomCtx.status = 0; roomAccept = 0;
    REQUIRE(!HGrace_CheckDoorTransition(player, play));
    REQUIRE(roomRequests == 0 && roomFinishes == 0 && player->actor.world.pos.x == 20);
    roomAccept = 1;
    entry.sides[1].room = 2; // Invalid target must not reach native loader.
    REQUIRE(!HGrace_CheckDoorTransition(player, play) && roomRequests == 0);
    entry.sides[1].room = 1;
    REQUIRE(HGrace_CheckDoorTransition(player, play));
    REQUIRE(roomRequests == 1 && roomFinishes == 0 && play->roomCtx.curRoom.num == 1);
    REQUIRE(player->actor.world.pos.x > 79 && std::fabs(player->actor.world.pos.z) < 1);
    REQUIRE(HGrace_CheckDoorTransition(player, play) && roomFinishes == 0);
    roomReady = 1;
    REQUIRE(HGrace_CheckDoorTransition(player, play) && roomFinishes == 1);
    REQUIRE(!HGrace_CheckDoorTransition(player, play) && roomRequests == 1 && roomFinishes == 1);
    play->transiActorCtx.list = nullptr; play->transiActorCtx.count = 0;
    sFairyPosValid = 0; player->actor.world.pos = {};
    puts("PASS real Grace door helper: live MM IDs/yaw, rejected/busy/bounds, async completion and no room bounce");
}

static void Exits(Player* player, PlayState* play) {
    u16 entrances[22]{};
    entrances[0] = 0x1234;
    play->setupExitList = entrances;
    nextInput.wasEquipped = 1;
    player->stateFlags1 = 0;
    sFairyPosValid = 0;
    hgActive = 1; hgState = HGRACE_STATE_FAIRY;
    floorExit = 1; play->sceneId = SCENE_TOWN;
    Handle_HyliasGrace(player, play);
    REQUIRE(!hgActive && fadeCalls == 1 && play->nextEntrance == 0x1234);
    REQUIRE(play->transitionTrigger == TRANS_TRIGGER_START);
    entrances[0] = 0xFFFF;
    gSaveContext.respawn[RESPAWN_MODE_UNK_3].entrance = 0x4321;
    play->transitionTrigger = TRANS_TRIGGER_OFF;
    hgActive = 1; hgState = HGRACE_STATE_FAIRY;
    Handle_HyliasGrace(player, play);
    REQUIRE(!hgActive && gSaveContext.respawnFlag == 4 && play->nextEntrance == 0x4321);
    REQUIRE(play->transitionType == TRANS_TYPE_FADE_WHITE && gSaveContext.nextTransitionType == TRANS_TYPE_FADE_WHITE);
    entrances[0] = 0x1234; floorEffect = FLOOR_EFFECT_2;
    play->transitionTrigger = TRANS_TRIGGER_OFF;
    hgActive = 1; hgState = HGRACE_STATE_FAIRY;
    Handle_HyliasGrace(player, play);
    REQUIRE(!hgActive && voidCalls == 1 && gSaveContext.respawnFlag == -2);
    REQUIRE(gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance == 0x1234);
    floorEffect = FLOOR_EFFECT_0;
    const struct { s16 scene; int eventIndex; } events[] = {
        {SCENE_GORONRACE, 3}, {SCENE_DEKU_KING, 3}, {SCENE_20SICHITAI, 21},
        {SCENE_20SICHITAI2, 21}, {SCENE_11GORONNOSATO, 6},
    };
    for (auto event : events) {
        play->sceneId = event.scene; floorExit = event.eventIndex;
        play->transitionTrigger = TRANS_TRIGGER_OFF;
        hgActive = 1; hgState = HGRACE_STATE_FAIRY;
        Handle_HyliasGrace(player, play);
        REQUIRE(hgActive && play->transitionTrigger == TRANS_TRIGGER_OFF);
        HGrace_Stop(player, play);
    }
    floorExit = 0; play->sceneId = SCENE_TOWN; play->setupExitList = nullptr;
    play->transitionTrigger = TRANS_TRIGGER_OFF;
    puts("PASS native MM fairy exits, 0xFFFF grotto return, floor effect and event-index exclusions");
}

static void Cancellation(Player* player, PlayState* play) {
    for (int reason = 0; reason < 5; ++reason) {
        player->stateFlags1 = PLAYER_STATE1_INPUT_DISABLED;
        player->actor.draw = HGrace_DrawFairy;
        hgActive = 1; hgState = HGRACE_STATE_FAIRY;
        sHGraceForm = player->transformation;
        nextInput.wasEquipped = 1; nextInput.isPressed = 1;
        switch (reason) {
            case 0: player->stateFlags1 |= PLAYER_STATE1_IN_WATER; break;
            case 1: player->stateFlags1 |= PLAYER_STATE1_DEAD; break;
            case 2: play->csCtx.state = CS_STATE_RUN; break;
            case 3: play->transitionTrigger = TRANS_TRIGGER_START; break;
            case 4: player->transformation = PLAYER_FORM_GORON; break;
        }
        Handle_HyliasGrace(player, play);
        REQUIRE(!hgActive && !HGrace_WantsNoClip() && player->actor.draw == Player_Draw);
        REQUIRE(!(player->stateFlags1 & PLAYER_STATE1_INPUT_DISABLED));
        player->transformation = PLAYER_FORM_HUMAN;
        player->stateFlags1 = 0; play->csCtx.state = CS_STATE_IDLE;
        play->transitionTrigger = TRANS_TRIGGER_OFF;
    }
    hgActive = 1; sFairyPos = {400, 500, 600}; sFairyPosValid = 1;
    sIvanActor = (Actor*)(uintptr_t)1; // Stale heap pointer must never be dereferenced.
    sPinkFairySkelInited = gIvanPossessActive = 1;
    const auto position = player->actor.world.pos;
    HGrace_ResetTransient();
    REQUIRE(!hgActive && !sFairyPosValid && !sPinkFairySkelInited && !sIvanActor && !gIvanPossessActive);
    REQUIRE(!std::memcmp(&position, &player->actor.world.pos, sizeof(position)));
    puts("PASS water/death/cutscene/transition/form cancellation and heap-safe fairy lifecycle reset");
}

int main(int argc, char** argv) {
    Player player{};
    PlayState play{};
    play.cameraPtrs[CAM_ID_MAIN] = &camera;
    camera.play = &play;
    player.transformation = PLAYER_FORM_HUMAN;
    if (argc > 1 && !std::strcmp(argv[1], "--preloaded-hall")) { PreloadedHall(&player, &play); return 0; }
    if (argc > 1 && !std::strcmp(argv[1], "--vertical-halls")) { VerticalHalls(&player, &play); return 0; }
    if (argc > 1 && !std::strcmp(argv[1], "--exits")) { Exits(&player, &play); return 0; }
    if (argc > 1 && !std::strcmp(argv[1], "--water")) { Cancellation(&player, &play); return 0; }
    SeedPolicy(&player, &play);
    PreloadedHall(&player, &play);
    VerticalHalls(&player, &play);
    Rooms(&player, &play);
    hgActive = 1;
    hgState = HGRACE_STATE_FAIRY;
    player.actor.draw = HGrace_DrawFairy;
    nextInput.wasEquipped = 1;
    play.state.input[0].cur.stick_y = play.state.input[0].rel.stick_y = 60;
    for (int i = 0; i < 10; ++i) Handle_HyliasGrace(&player, &play);
    REQUIRE(player.actor.world.pos.z > 30 && std::fabs(player.actor.world.pos.x) < 0.1f);
    REQUIRE(hgActive && hgState == HGRACE_STATE_FAIRY && HGrace_WantsNoClip());
    auto first = player.actor.world.pos;
    play.state.input[0].cur.button = BTN_A;
    for (int i = 0; i < 8; ++i) Handle_HyliasGrace(&player, &play);
    REQUIRE(player.actor.world.pos.y > first.y + 20 && hgActive);
    first = player.actor.world.pos;
    play.state.input[0].cur.button = BTN_B;
    for (int i = 0; i < 12; ++i) Handle_HyliasGrace(&player, &play);
    REQUIRE(player.actor.world.pos.y < first.y - 15 && hgActive);
    first = player.actor.world.pos;
    camera.inputDir.y = 0x4000;
    play.state.input[0].cur.button = BTN_L;
    for (int i = 0; i < 10; ++i) Handle_HyliasGrace(&player, &play);
    REQUIRE(player.actor.world.pos.x > first.x + 55 && hgActive);
    nextInput.isPressed = 1;
    Handle_HyliasGrace(&player, &play);
    REQUIRE(hgState == HGRACE_STATE_WARP_OUT);
    nextInput.isPressed = 0;
    HGrace_Stop(&player, &play);
    REQUIRE(!hgActive && !HGrace_WantsNoClip() && player.actor.draw == Player_Draw);
    REQUIRE(!(player.stateFlags1 & PLAYER_STATE1_INPUT_DISABLED));
    REQUIRE(player.cylinder.base.atFlags & AT_ON);
    REQUIRE(player.cylinder.base.acFlags & AC_ON);
    REQUIRE(player.cylinder.base.ocFlags1 & OC1_ON);
    puts("PASS real MM Grace flight: analog, camera yaw, A/B, sprint, toggle and restore");
    Exits(&player, &play);
    Cancellation(&player, &play);
}
