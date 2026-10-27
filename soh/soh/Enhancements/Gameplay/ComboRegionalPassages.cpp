#ifdef COMBO_BUILD
#include "ComboRegionalPassages.h"
#include "ComboGameMode.h"
#include "ComboLakeHyliaBridge.h"
#include "ComboFountainSnowheadBridge.h"
#include "entrances/IkanaGravePlacement.h"
#include "ComboResolve.h"
#include "entrances/RegionalPassages.h"
#include "gameplay/CrossGameTitleCard.h"
#include "soh/ActorDB.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/randomizer/SeedContext.h"
#include "soh/SaveManager.h"
#include "soh/ShipInit.hpp"
#include "overlays/actors/ovl_Bg_Haka/z_bg_haka.h"
#include "overlays/actors/ovl_Door_Ana/z_door_ana.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
extern "C" {
void BgHaka_Init(Actor*, PlayState*);
void BgHaka_Destroy(Actor*, PlayState*);
void BgHaka_Draw(Actor*, PlayState*);
void BgHaka_IdleOpened(BgHaka*, PlayState*);
void DoorAna_Init(Actor*, PlayState*);
void DoorAna_Destroy(Actor*, PlayState*);
void DoorAna_Draw(Actor*, PlayState*);
extern void (*gComboSceneSwitchCallback)(int);
}
namespace {
using namespace ComboRegional;
struct Passage {
    PlayState* play = nullptr;
    Pose entry{}, mouth{}, gravePose{};
    int route = 0, room = -1, slot = -1;
    u32 seed = 0;
    bool ready = false, falling = false, departureArmed = true;
    BgHaka* grave = nullptr;
    Actor* hole = nullptr;
} sRegional;
PlayState* sEntrancePlay = nullptr;
unsigned sEntranceCount = 0;
std::array<Origin, 3> sOrigins{};
std::array<int, 3> sOriginAge{ { -1, -1, -1 } };
std::array<bool, 3> sOriginMq{};
int sOriginSlot = -1, sArrivalRoute = 0;
unsigned sArrivalWaitFrames = 0;
u32 sOriginSeed = 0;
bool sLoadPending = true, sReturnStaged = false, sReturningGrave = false;
RespawnData sPreservedReturn{}, sStagedReturn{};
// Scene-command generation, not PlayState addresses: the arena can reuse the
// source allocation while native fade updates continue before recreation.
u64 sSpawnGeneration = 0, sReturnStageGeneration = 0;
void RestoreStagedReturn() {
    if (!sReturnStaged)
        return;
    // A composed native arrival may replace RETURN while this route is queued.
    // Preserve that newer owner rather than overwriting it with our snapshot.
    if (std::memcmp(&gSaveContext.respawn[RESPAWN_MODE_RETURN], &sStagedReturn, sizeof(sStagedReturn)) == 0)
        gSaveContext.respawn[RESPAWN_MODE_RETURN] = sPreservedReturn;
    sReturnStaged = false;
}
PlayState* sArrivalCoverPlay = nullptr;
void ClearArrivalCover() {
    if (sArrivalCoverPlay && sArrivalCoverPlay == gPlayState)
        sArrivalCoverPlay->envCtx.fillScreen = false;
    sArrivalCoverPlay = nullptr;
}
nlohmann::json sPending;
int sHoleId = -1;
u32 Seed() {
    return IS_RANDO ? Rando::Context::GetInstance()->GetSeed() : 0;
}
int SceneRoute(int scene) {
    if (scene == SCENE_GRAVEYARD)
        return COMBO_PORTAL_IKANA;
    if (scene == SCENE_LAKE_HYLIA)
        return COMBO_PORTAL_GREAT_BAY;
    if (scene == SCENE_ZORAS_FOUNTAIN)
        return COMBO_PORTAL_SNOWHEAD;
    return 0;
}
bool Bound() {
    return Combo_IsCombinedGame() && gSaveContext.gameMode == GAMEMODE_NORMAL &&
           ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) && sOriginSlot == gSaveContext.fileNum &&
           sOriginSeed == Seed();
}
bool Valid(const Origin& origin) {
    return origin.entrance >= 0 && origin.entrance < ENTR_MAX &&
           ValidOrigin(origin, gEntranceTable[origin.entrance].scene);
}
void SetPulled(BgHaka* grave) {
    auto& actor = grave->dyna.actor;
    grave->state = 1;
    grave->actionFunc = BgHaka_IdleOpened;
    grave->dyna.unk_150 = 0;
    actor.minVelocityY = 60;
    actor.world.rot.y = actor.shape.rot.y + 0x8000;
    actor.world.pos.x = actor.home.pos.x + Math_SinS(actor.world.rot.y) * 60;
    actor.world.pos.z = actor.home.pos.z + Math_CosS(actor.world.rot.y) * 60;
}
void HoleInit(Actor* actor, PlayState* play) {
    DoorAna_Init(actor, play);
    Actor_SetScale(actor, 0);
}
void HoleUpdate(Actor* actor, PlayState* play) {
    if (!Bound() || play != sRegional.play || actor != sRegional.hole || !sRegional.grave || !sRegional.grave->state)
        return;
    const bool open = Math_StepToF(&actor->scale.x, .01f, .001f);
    Actor_SetScale(actor, actor->scale.x);
    if (!open)
        return;
    auto* player = GET_PLAYER(play);
    if (!Bound() || !player || !gComboSceneSwitchCallback || !Combo_ResolveSym("2ship", "MM_SetComboPortalRoute") ||
        play->pauseCtx.state != 0 || play->transitionMode != TRANS_MODE_OFF ||
        play->transitionTrigger != TRANS_TRIGGER_OFF || Player_InCsMode(play) ||
        player->stateFlags1 & (PLAYER_STATE1_ON_HORSE | PLAYER_STATE1_IN_WATER))
        return;
    const auto& p = player->actor.world.pos;
    // Native grotto pop-out starts inside the aperture. Require the player to
    // step clear before another departure, including after its cutscene ends.
    if (!sRegional.departureArmed) {
        if (Distance2({ p.x, p.y, p.z }, sRegional.mouth.pos) > 45 * 45)
            sRegional.departureArmed = true;
        return;
    }
    if (Distance2({ p.x, p.y, p.z }, sRegional.mouth.pos) < 15 * 15 && p.y >= sRegional.mouth.pos.y - 50 &&
        p.y <= sRegional.mouth.pos.y + 15) {
        player->stateFlags1 |= PLAYER_STATE1_FLOOR_DISABLED;
        sRegional.falling = true;
    }
}
void RegisterHole() {
    if (sHoleId >= 0)
        return;
    ActorDBInit hole;
    hole.name = "Door_ComboRegionalGrave";
    hole.desc = "Native Ikana grave aperture";
    hole.category = ACTORCAT_ITEMACTION;
    hole.objectId = OBJECT_GAMEPLAY_FIELD_KEEP;
    hole.instanceSize = sizeof(DoorAna);
    hole.flags = ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED;
    hole.init = HoleInit;
    hole.destroy = DoorAna_Destroy;
    hole.update = HoleUpdate;
    hole.draw = DoorAna_Draw;
    sHoleId = ActorDB::Instance->AddEntry(hole).entry.id;
}
bool HolePresent(PlayState* play) {
    for (Actor* actor = play->actorCtx.actorLists[ACTORCAT_ITEMACTION].head; actor; actor = actor->next)
        if (actor == sRegional.hole && actor->id == sHoleId && actor->update)
            return true;
    return false;
}
bool ClaimMarkedGrave(PlayState* play) {
    for (Actor* actor = play->actorCtx.actorLists[ACTORCAT_BG].head; actor; actor = actor->next) {
        if (actor->id != ACTOR_BG_HAKA || actor->init || !actor->update ||
            !IkanaGrave::Matches({ actor->home.pos.x, actor->home.pos.y, actor->home.pos.z }, actor->home.rot.y,
                                 actor->params, actor->room))
            continue;
        // Keep the original native actor, including its init/update/draw,
        // dynamic collision, child/day lock, pull action and vanilla checks.
        sRegional.grave = reinterpret_cast<BgHaka*>(actor);
        sRegional.gravePose = sRegional.mouth = IkanaGrave::kPose;
        if (sReturningGrave) {
            SetPulled(sRegional.grave);
            sRegional.departureArmed = false;
            sReturningGrave = false;
        }
        RegisterHole();
        const auto& p = IkanaGrave::kPose.pos;
        // Actor_Spawn may return an actor whose object bank is still pending.
        // Retain that one aperture while native Actor_UpdateAll initializes it.
        if (!HolePresent(play))
            sRegional.hole = Actor_Spawn(&play->actorCtx, play, sHoleId, p.x, p.y, p.z, 0, IkanaGrave::kPose.yaw, 0, 0);
        return sRegional.hole && sRegional.hole->init == nullptr;
    }
    return false;
}
bool GraveActorsLive(PlayState* play) {
    bool graveLive = false, holeLive = false;
    for (Actor* actor = play->actorCtx.actorLists[ACTORCAT_BG].head; actor; actor = actor->next)
        if (actor == reinterpret_cast<Actor*>(sRegional.grave) && actor->id == ACTOR_BG_HAKA && !actor->init &&
            actor->update)
            graveLive = true;
    for (Actor* actor = play->actorCtx.actorLists[ACTORCAT_ITEMACTION].head; actor; actor = actor->next)
        if (actor == sRegional.hole && actor->id == sHoleId && !actor->init && actor->update)
            holeLive = true;
    return graveLive && holeLive;
}
bool ResolveFixedBridge(PlayState* play) {
    Vec3f mouth{};
    s16 yaw = 0;
    const bool ready = sRegional.route == COMBO_PORTAL_GREAT_BAY ? ComboLakeHyliaBridge_Get(play, &mouth, &yaw)
                                                                 : ComboFountainSnowheadBridge_Get(play, &mouth, &yaw);
    if (ready)
        sRegional.mouth = { { mouth.x, mouth.y, mouth.z }, yaw, Forward(yaw) };
    return ready;
}
void CaptureOrigin() {
    if (!Bound() || !sRegional.ready)
        return;
    SaveManager::Instance->ThreadPoolWait();
    Pose arrival = sRegional.mouth;
    if (sRegional.route != COMBO_PORTAL_IKANA)
        arrival.pos = Add(arrival.pos, arrival.forward, 100);
    if (sRegional.route == COMBO_PORTAL_SNOWHEAD && !SnowheadReturn(&arrival))
        return;
    const auto index = sRegional.route - COMBO_PORTAL_IKANA;
    sOrigins[index] = { sRegional.route, gSaveContext.entranceIndex, sRegional.room, arrival };
    sOriginAge[index] = gSaveContext.linkAge;
    sOriginMq[index] = ResourceMgr_IsSceneMasterQuest(sRegional.play->sceneNum);
}
void StageArrival(const Origin& origin) {
    Pose grounded = origin.pose;
    if (origin.route == COMBO_PORTAL_SNOWHEAD && !SnowheadReturn(&grounded))
        return;
    sPreservedReturn = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    sReturnStaged = true;
    auto& arrival = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    arrival = {};
    arrival.entranceIndex = origin.entrance;
    arrival.roomIndex = origin.room;
    arrival.pos = { grounded.pos.x, grounded.pos.y, grounded.pos.z };
    arrival.yaw = grounded.yaw;
    arrival.playerParams = origin.route == COMBO_PORTAL_IKANA ? 0x04FF : 0x0EFF;
    arrival.data = 1;
    sStagedReturn = arrival;
    sReturnStageGeneration = sSpawnGeneration;
    gSaveContext.entranceIndex = origin.entrance;
    gSaveContext.respawnFlag = 2;
    gSaveContext.cutsceneIndex = 0;
    gSaveContext.cutsceneTrigger = 0;
    sReturningGrave = origin.route == COMBO_PORTAL_IKANA;
}
void PlayerUpdate() {
    auto* play = gPlayState;
    if (!Bound() || !play) {
        ClearArrivalCover();
        sArrivalRoute = 0;
        return;
    }
    if (sReturnStaged && sSpawnGeneration != sReturnStageGeneration && GET_PLAYER(play) &&
        GET_PLAYER(play)->actor.init == nullptr && gSaveContext.respawnFlag == 0)
        RestoreStagedReturn();
    if (!sArrivalRoute || !play->state.running)
        return;
    if (SceneRoute(play->sceneNum) != sArrivalRoute) {
        ClearArrivalCover();
        sArrivalRoute = 0;
        sReturningGrave = false;
        return;
    }
    sArrivalCoverPlay = play;
    play->envCtx.fillScreen = true;
    play->envCtx.screenFillColor[0] = play->envCtx.screenFillColor[1] = play->envCtx.screenFillColor[2] = 0;
    play->envCtx.screenFillColor[3] = 255;
    // Loaded room actors/object banks can lag the first player update. Keep
    // the authored arrival pending until its owner is ready; do not silently
    // turn a required regional arrival into the native scene entrance.
    if (!ComboRegionalPassagesOot_Get(play, nullptr, nullptr, nullptr)) {
        if (++sArrivalWaitFrames >= 300) {
            std::fprintf(stderr,
                         "Combo regional arrival %d failed: authored endpoint unavailable in scene %d room %d\n",
                         sArrivalRoute, play->sceneNum, play->roomCtx.curRoom.num);
            ClearArrivalCover();
            sArrivalRoute = 0;
            sReturningGrave = false;
        }
        return;
    }
    sArrivalWaitFrames = 0;
    if (play->transitionTrigger != TRANS_TRIGGER_OFF || play->transitionMode != TRANS_MODE_OFF ||
        play->pauseCtx.state != 0 || Player_InCsMode(play))
        return;
    // Resolve the authored endpoint under cover, then let Player_Init consume
    // the native RETURN/GROTTO start mode to reveal that passage once.
    CaptureOrigin();
    const auto origin = sOrigins[sArrivalRoute - COMBO_PORTAL_IKANA];
    sArrivalRoute = 0;
    if (!Valid(origin)) {
        ClearArrivalCover();
        return;
    }
    StageArrival(origin);
    play->nextEntranceIndex = origin.entrance;
    play->transitionType = gSaveContext.nextTransitionType = TRANS_TYPE_FADE_BLACK;
    play->transitionTrigger = TRANS_TRIGGER_START;
    Combo_SOH_RequestCrossGameTitleCard();
}
void BindFile(int slot) {
    if (!sLoadPending && sOriginSlot == slot && sOriginSeed == Seed())
        return;
    SaveManager::Instance->ThreadPoolWait();
    sLoadPending = false;
    sOriginSlot = slot;
    sOriginSeed = Seed();
    sOrigins = {};
    sOriginAge = { { -1, -1, -1 } };
    sOriginMq = {};
    sArrivalRoute = 0;
    sArrivalWaitFrames = 0;
    sReturnStaged = false;
    sReturningGrave = false;
    if (!Bound() || !sPending.is_object())
        return;
    try {
        if (sPending.at("version") != 1 || sPending.at("seed") != sOriginSeed || sPending.at("slot") != slot)
            return;
        const auto& records = sPending.at("origins");
        if (!records.is_array() || records.size() != 3)
            return;
        for (size_t i = 0; i < 3; ++i) {
            const auto& p = records[i];
            if (p.is_null())
                continue;
            const int yaw = p.at("yaw");
            const Origin origin{ static_cast<int>(COMBO_PORTAL_IKANA + i),
                                 p.at("entrance"),
                                 p.at("room"),
                                 { { p.at("x"), p.at("y"), p.at("z") }, static_cast<short>(yaw) } };
            const int age = p.at("age");
            const bool mq = p.at("mq");
            if (yaw >= INT16_MIN && yaw <= INT16_MAX && (age == 0 || age == 1) && Valid(origin)) {
                sOrigins[i] = origin;
                sOriginAge[i] = age;
                sOriginMq[i] = mq;
            }
        }
    } catch (const nlohmann::json::exception&) { sOrigins = {}; }
}
void SaveOrigins(const SaveContext& snapshot, int, bool) {
    if (!Combo_IsCombinedGame() || snapshot.fileNum != sOriginSlot)
        return;
    nlohmann::json origins = nlohmann::json::array();
    for (size_t i = 0; i < sOrigins.size(); ++i) {
        const auto& origin = sOrigins[i];
        if (!Valid(origin)) {
            origins.push_back(nullptr);
            continue;
        }
        origins.push_back({ { "entrance", origin.entrance },
                            { "room", origin.room },
                            { "x", origin.pose.pos.x },
                            { "y", origin.pose.pos.y },
                            { "z", origin.pose.pos.z },
                            { "yaw", origin.pose.yaw },
                            { "age", sOriginAge[i] },
                            { "mq", sOriginMq[i] } });
    }
    const nlohmann::json value = {
        { "version", 1 }, { "slot", sOriginSlot }, { "seed", sOriginSeed }, { "origins", origins }
    };
    SaveManager::Instance->SaveData("native", value);
}
void Register() {
    SaveManager::Instance->AddInitFunction([](bool) {
        sPending = nullptr;
        sLoadPending = true;
    });
    SaveManager::Instance->AddLoadFunction("regionalPassages", 1, [] {
        SaveManager::Instance->LoadData("native", sPending);
        sLoadPending = true;
    });
    SaveManager::Instance->AddSaveFunction("regionalPassages", 1, SaveOrigins, true, SECTION_PARENT_NONE);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>(BindFile);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(PlayerUpdate);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>(ComboRegionalPassagesOot_Reset);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnExitGame>([](int32_t) {
        RestoreStagedReturn();
        sArrivalRoute = 0;
        ComboRegionalPassagesOot_Reset();
    });
}
RegisterShipInitFunc regionalPassagesInit(Register);
} // namespace
extern "C" void ComboRegionalPassagesOot_CancelDeparture(void) {
    if (sRegional.falling && sRegional.play && sRegional.play == gPlayState && GET_PLAYER(gPlayState)) {
        auto* player = GET_PLAYER(gPlayState);
        player->stateFlags1 &= ~PLAYER_STATE1_FLOOR_DISABLED;
        const auto safe = Add(sRegional.mouth.pos, sRegional.mouth.forward, 80);
        player->actor.world.pos = { safe.x, safe.y, safe.z };
    }
    sRegional.falling = false;
}
extern "C" void ComboRegionalPassagesOot_Reset(void) {
    ComboRegionalPassagesOot_CancelDeparture();
    ClearArrivalCover();
    sArrivalWaitFrames = 0;
    sRegional = {};
}
extern "C" void ComboRegionalPassagesOot_ReadEntrances(PlayState* play, unsigned count) {
    sEntrancePlay = play;
    sEntranceCount = count;
}
extern "C" void ComboRegionalPassagesOot_ReadSpawns(PlayState* play, const ActorEntry* entries, unsigned count) {
    ++sSpawnGeneration;
    sRegional = {};
    // Farore is composed after the portal boot. Observe its native TOP start
    // mode before Player_Init consumes it, including recalls into this region.
    if (gSaveContext.respawnFlag == 3) {
        sArrivalRoute = 0;
        sReturningGrave = false;
        RestoreStagedReturn();
    }
    if (!Combo_IsCombinedGame() || !play || !SceneRoute(play->sceneNum) || !entries || !play->setupEntranceList ||
        !count)
        return;
    const unsigned spawn = play->sceneNum == SCENE_LAKE_HYLIA ? 4 : 0;
    if (sEntrancePlay != play || spawn >= sEntranceCount)
        return;
    const auto native = play->setupEntranceList[spawn];
    if (native.spawn >= count)
        return;
    const auto& source = entries[native.spawn];
    sRegional.play = play;
    sRegional.route = SceneRoute(play->sceneNum);
    sRegional.room = sRegional.route == COMBO_PORTAL_IKANA ? IkanaGrave::kRoom : native.room;
    sRegional.entry = { { static_cast<float>(source.pos.x), static_cast<float>(source.pos.y),
                          static_cast<float>(source.pos.z) },
                        source.rot.y };
    sRegional.slot = gSaveContext.fileNum;
    sRegional.seed = Seed();
}
extern "C" int ComboRegionalPassagesOot_Get(PlayState* play, int* route, Vec3f* anchor, s16* yaw) {
    if (!Bound() || !play || play != sRegional.play || SceneRoute(play->sceneNum) != sRegional.route ||
        play->roomCtx.curRoom.num != sRegional.room || sRegional.slot != gSaveContext.fileNum ||
        sRegional.seed != Seed() || !play->colCtx.colHeader || !GET_PLAYER(play))
        return false;
    // Actor initialization and bridge registration may occur after the first
    // presentation query; retry the exact owned endpoint, never a search.
    if (sRegional.route == COMBO_PORTAL_IKANA) {
        if (sRegional.ready && !GraveActorsLive(play)) {
            sRegional.ready = false;
            sRegional.grave = nullptr;
            sRegional.hole = nullptr;
            sRegional.falling = false;
        }
        if (!sRegional.ready)
            sRegional.ready = ClaimMarkedGrave(play);
    } else {
        sRegional.ready = ResolveFixedBridge(play);
    }
    if (!sRegional.ready)
        return false;
    if (route)
        *route = sRegional.route;
    if (anchor)
        *anchor = { sRegional.mouth.pos.x, sRegional.mouth.pos.y, sRegional.mouth.pos.z };
    if (yaw)
        *yaw = sRegional.mouth.yaw;
    return true;
}
extern "C" int ComboRegionalPassagesOot_OwnsGraveFall(PlayState* play) {
    return sRegional.falling && ComboRegionalPassagesOot_Get(play, nullptr, nullptr, nullptr) &&
           sRegional.route == COMBO_PORTAL_IKANA;
}
extern "C" int ComboRegionalPassagesOot_AtDeparture(PlayState* play, const Vec3f* position) {
    if (!position || !ComboRegionalPassagesOot_Get(play, nullptr, nullptr, nullptr))
        return 0;
    const Point point{ position->x, position->y, position->z };
    const bool entering = sRegional.route == COMBO_PORTAL_IKANA
                              ? sRegional.falling && point.y < sRegional.mouth.pos.y - 8
                          : sRegional.route == COMBO_PORTAL_SNOWHEAD ? InSnowheadMouth(sRegional.mouth, point)
                                                                     : InMouth(sRegional.mouth, point);
    if (!entering)
        return 0;
    CaptureOrigin();
    return sRegional.route;
}
extern "C" int ComboRegionalPassagesOot_IsArrivalPending(PlayState* play) {
    return Bound() && play && play == gPlayState && sArrivalRoute && SceneRoute(play->sceneNum) == sArrivalRoute;
}
extern "C" int ComboRegionalPassagesOot_ApplyArrival(int route) {
    if (!Bound() || !ComboRegionalPassages_IsRoute(route))
        return 0;
    RestoreStagedReturn();
    // Always resolve the current authored endpoint. Legacy saved coordinates
    // must never relocate a fixed passage after age/layout or resource changes.
    sArrivalRoute = route;
    sArrivalWaitFrames = 0;
    gSaveContext.entranceIndex = route == COMBO_PORTAL_IKANA       ? ENTR_GRAVEYARD_ENTRANCE
                                 : route == COMBO_PORTAL_GREAT_BAY ? ENTR_LAKE_HYLIA_OUTSIDE_LAB
                                                                   : ENTR_ZORAS_FOUNTAIN_OUTSIDE_ICE_CAVERN;
    gSaveContext.respawnFlag = 0;
    gSaveContext.cutsceneIndex = 0;
    gSaveContext.cutsceneTrigger = 0;
    sReturningGrave = route == COMBO_PORTAL_IKANA;
    gSaveContext.nextTransitionType = TRANS_TYPE_FADE_BLACK;
    return 1;
}
#endif
