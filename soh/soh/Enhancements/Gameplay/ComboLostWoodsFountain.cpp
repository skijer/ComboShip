#ifdef COMBO_BUILD
#include "ComboLostWoodsFountain.h"
#include "ComboGameMode.h"
#include "global.h"
#include "entrances/LostWoodsEntrance.h"
#include "gameplay/CrossGameTitleCard.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/randomizer/randomizer.h"
#include "soh/Enhancements/randomizer/SeedContext.h"
#include "soh/SaveManager.h"
#include "soh/ShipInit.hpp"
#include <array>
#include <cmath>

extern "C" s32 gComboReturnFileNum;

namespace {
struct FountainOrigin {
    int entrance = -1;
    Vec3f pos{};
    s16 yaw = 0;
    int room = -1;
};
FountainOrigin sFountainOrigin;
std::array<FountainOrigin, 8> sFountainGround;
size_t sFountainGroundCount = 0, sFountainGroundNext = 0;
int sFountainSlot = -1, sFountainEntrance = -1;
u32 sFountainSeed = 0;
bool sFountainActive = false, sFountainResume = false, sFountainReturnStaged = false;
bool sFountainLoadPending = true;
RespawnData sFountainPreservedReturn;
nlohmann::json sFountainPending;

u32 FountainSeed() {
    return IS_RANDO ? Rando::Context::GetInstance()->GetSeed() : 0;
}
bool FountainBound() {
    return gComboGameMode != COMBO_GAME_MM && gSaveContext.gameMode == GAMEMODE_NORMAL &&
           ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) && sFountainSlot == gSaveContext.fileNum &&
           sFountainSeed == FountainSeed();
}
int FountainScene(int entrance) {
    return entrance >= 0 && entrance < ENTR_MAX ? gEntranceTable[entrance].scene : -1;
}
bool IsGreatFairyScene(int scene) {
    return scene == SCENE_GREAT_FAIRYS_FOUNTAIN_MAGIC || scene == SCENE_GREAT_FAIRYS_FOUNTAIN_SPELLS;
}
bool FountainVisit(const PlayState* play) {
    return play && FountainBound() && sFountainActive && play->sceneNum == FountainScene(sFountainEntrance) &&
           play->curSpawn == gEntranceTable[sFountainEntrance].spawn;
}
bool ValidFountainOrigin(const FountainOrigin& point) {
    return FountainScene(point.entrance) == SCENE_LOST_WOODS && point.room >= 0 && point.room <= 10 &&
           std::isfinite(point.pos.x) && std::isfinite(point.pos.y) && std::isfinite(point.pos.z) &&
           std::abs(point.pos.x) < 32768 && std::abs(point.pos.y) < 32768 && std::abs(point.pos.z) < 32768;
}
float FountainDistance(const Vec3f& a, const Vec3f& b) {
    const float x = a.x - b.x, z = a.z - b.z;
    return x * x + z * z;
}
void FountainPlayerUpdate() {
    auto* play = gPlayState;
    if (!FountainBound() || !play)
        return;
    if (sFountainReturnStaged && play->sceneNum == SCENE_LOST_WOODS) {
        gSaveContext.respawn[RESPAWN_MODE_RETURN] = sFountainPreservedReturn;
        sFountainReturnStaged = false;
    }
    if (play->sceneNum != SCENE_LOST_WOODS || !play->state.running || play->transitionTrigger != TRANS_TRIGGER_OFF ||
        play->transitionMode != TRANS_MODE_OFF || play->pauseCtx.state != 0 ||
        play->gameOverCtx.state != GAMEOVER_INACTIVE || Player_InCsMode(play))
        return;
    auto* player = GET_PLAYER(play);
    if (!player || !player->actor.floorPoly || !(player->actor.bgCheckFlags & BGCHECKFLAG_GROUND) ||
        player->actor.floorPoly->normal.y < 23170 || !std::isfinite(player->actor.floorHeight) ||
        std::abs(player->actor.world.pos.y - player->actor.floorHeight) >= 35 ||
        SurfaceType_GetSceneExitIndex(&play->colCtx, player->actor.floorPoly, player->actor.floorBgId) != 0)
        return;
    FountainOrigin point{ gSaveContext.entranceIndex, player->actor.world.pos,
                          static_cast<s16>(player->actor.shape.rot.y + 0x8000), play->roomCtx.curRoom.num };
    point.pos.y = player->actor.floorHeight;
    if (!ValidFountainOrigin(point))
        return;
    if (sFountainGroundCount) {
        const auto& last = sFountainGround[(sFountainGroundNext + sFountainGround.size() - 1) % sFountainGround.size()];
        if (last.room == point.room && FountainDistance(last.pos, point.pos) < 16 * 16)
            return;
    }
    sFountainGround[sFountainGroundNext] = point;
    sFountainGroundNext = (sFountainGroundNext + 1) % sFountainGround.size();
    if (sFountainGroundCount < sFountainGround.size())
        ++sFountainGroundCount;
}
void FountainSceneInit(s16) {
    sFountainGroundCount = sFountainGroundNext = 0;
    if (sFountainActive && !FountainVisit(gPlayState)) {
        SaveManager::Instance->ThreadPoolWait();
        sFountainActive = false;
    }
}
void ResetFountainFile(bool) {
    // InitFile/LoadFile hold saveMtx. Publish new worker-facing state only
    // from OnLoadGame after waiting outside that mutex.
    sFountainPending = nullptr;
    sFountainLoadPending = true;
    sFountainGroundCount = sFountainGroundNext = 0;
}
void BindFountainFile(int slot) {
    if (!sFountainLoadPending && sFountainSlot == slot && sFountainSeed == FountainSeed())
        return; // Native warps/age changes also emit OnLoadGame.
    SaveManager::Instance->ThreadPoolWait();
    sFountainLoadPending = false;
    sFountainSlot = slot;
    sFountainSeed = FountainSeed();
    sFountainActive = sFountainResume = sFountainReturnStaged = false;
    sFountainEntrance = -1;
    sFountainGroundCount = sFountainGroundNext = 0;
    // A paired-game return loads the old OoT save before composing its new
    // portal/dungeon/Farore arrival. That explicit trip owns the destination;
    // only an ordinary file resume may restore the older fountain visit.
    if (!FountainBound() || gComboReturnFileNum >= 0 || !sFountainPending.is_object())
        return;
    try {
        const auto& value = sFountainPending;
        if (value.at("version") != 1 || value.at("seed") != sFountainSeed || value.at("savedInside") != true)
            return;
        const int entrance = value.at("fountain");
        const auto& point = value.at("origin");
        const int yaw = point.at("yaw");
        const FountainOrigin origin{ point.at("entrance"),
                                     { point.at("x"), point.at("y"), point.at("z") },
                                     static_cast<s16>(yaw),
                                     point.at("room") };
        if (!IsGreatFairyScene(FountainScene(entrance)) || !ValidFountainOrigin(origin) || yaw < INT16_MIN ||
            yaw > INT16_MAX)
            return;
        sFountainOrigin = origin;
        sFountainEntrance = entrance;
        sFountainActive = sFountainResume = true;
    } catch (const nlohmann::json::exception&) {}
}
void SaveFountainState(const SaveContext& snapshot, int, bool) {
    if (sFountainSlot != snapshot.fileNum || gComboGameMode == COMBO_GAME_MM)
        return;
    const bool inside = sFountainActive && FountainScene(snapshot.entranceIndex) == FountainScene(sFountainEntrance) &&
                        gEntranceTable[snapshot.entranceIndex].spawn == gEntranceTable[sFountainEntrance].spawn;
    const nlohmann::json value = { { "version", 1 },
                                   { "seed", sFountainSeed },
                                   { "savedInside", inside },
                                   { "fountain", sFountainEntrance },
                                   { "origin",
                                     { { "entrance", sFountainOrigin.entrance },
                                       { "room", sFountainOrigin.room },
                                       { "x", sFountainOrigin.pos.x },
                                       { "y", sFountainOrigin.pos.y },
                                       { "z", sFountainOrigin.pos.z },
                                       { "yaw", sFountainOrigin.yaw } } } };
    SaveManager::Instance->SaveData("native", value);
}
void RegisterFountainReturn() {
    SaveManager::Instance->AddInitFunction(ResetFountainFile);
    SaveManager::Instance->AddLoadFunction("lostWoodsFountain", 1, [] {
        SaveManager::Instance->LoadData("native", sFountainPending);
        sFountainLoadPending = true;
    });
    SaveManager::Instance->AddSaveFunction("lostWoodsFountain", 1, SaveFountainState, true, SECTION_PARENT_NONE);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>(BindFountainFile);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>(FountainSceneInit);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(FountainPlayerUpdate);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>(
        [] { sFountainGroundCount = sFountainGroundNext = 0; });
}
RegisterShipInitFunc fountainReturnInit(RegisterFountainReturn);
} // namespace

extern "C" void ComboLostWoodsFountain_NormalizeEntry(void) {
    if (!sFountainResume)
        return;
    sFountainResume = false;
    if (FountainBound() && sFountainActive) {
        gSaveContext.entranceIndex = sFountainEntrance;
        gSaveContext.respawnFlag = 0;
        gSaveContext.cutsceneIndex = 0;
        gSaveContext.cutsceneTrigger = 0;
    }
}

extern "C" int ComboLostWoodsFountain_TryExit(PlayState* play, Player* player, int entrance) {
    if (!FountainBound() || !play || !player)
        return 0;
    const auto& pos = player->actor.world.pos;
    // The unmodified room-7 north tunnel shares Kokiri's exit-list slot with
    // other dead ends. Claim only its own native exit floor, before shuffle.
    const bool nativeTunnel = play->sceneNum == SCENE_LOST_WOODS && play->roomCtx.curRoom.num == 7 &&
                              entrance == ENTR_KOKIRI_FOREST_UPPER_EXIT && pos.x >= 1540 && pos.x <= 1660 &&
                              pos.y > -30 && pos.y < 120 && pos.z <= -2800 && pos.z >= -3200;
    if (nativeTunnel)
        entrance = ENTR_GREAT_FAIRYS_FOUNTAIN_MAGIC_DMT;
    if (play->sceneNum == SCENE_LOST_WOODS && IsGreatFairyScene(FountainScene(entrance))) {
        // Authored Fairy exits retain their chosen destination and approach.
        const FountainOrigin* origin = nullptr;
        for (size_t i = 0; i < sFountainGroundCount; ++i) {
            const auto& point =
                sFountainGround[(sFountainGroundNext + sFountainGround.size() - i - 1) % sFountainGround.size()];
            const auto distance = FountainDistance(point.pos, player->actor.world.pos);
            if (point.room != play->roomCtx.curRoom.num || distance > 256 * 256)
                continue;
            if (!origin)
                origin = &point;
            if (distance >= 48 * 48) {
                origin = &point;
                break;
            }
        }
        // Debug warps can arrive directly on the native exit floor. Its known
        // dry approach is outside that floor and faces south into room 7.
        const FountainOrigin nativeOrigin{ ENTR_LOST_WOODS_SOUTH_EXIT, { 1600, 0, -2760 }, 0, 7 };
        if (!origin && nativeTunnel)
            origin = &nativeOrigin;
        if (!origin)
            return 0;
        SaveManager::Instance->ThreadPoolWait();
        sFountainOrigin = *origin;
        sFountainEntrance = entrance;
        sFountainActive = true;
        play->nextEntranceIndex = entrance;
        Combo_SOH_RequestCrossGameTitleCard();
        return 1;
    }
    const bool exit =
        (play->sceneNum == SCENE_GREAT_FAIRYS_FOUNTAIN_MAGIC && entrance == ENTR_RETURN_DAIYOUSEI_IZUMI) ||
        (play->sceneNum == SCENE_GREAT_FAIRYS_FOUNTAIN_SPELLS && entrance == ENTR_RETURN_YOUSEI_IZUMI_YOKO);
    if (!exit || !FountainVisit(play))
        return 0;
    SaveManager::Instance->ThreadPoolWait();
    sFountainActive = false; // Consume this visit; later native visits keep their native/shuffled exits.
    auto& arrival = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    sFountainPreservedReturn = arrival;
    sFountainReturnStaged = true;
    arrival = {};
    arrival.entranceIndex = sFountainOrigin.entrance;
    arrival.roomIndex = sFountainOrigin.room;
    arrival.pos = sFountainOrigin.pos;
    arrival.yaw = sFountainOrigin.yaw;
    arrival.playerParams = (PLAYER_START_MODE_IDLE << 8) | 0xFF;
    arrival.data = 1;
    gSaveContext.respawnFlag = 2;
    gSaveContext.entranceSpeed = 0;
    play->nextEntranceIndex = sFountainOrigin.entrance;
    Combo_SOH_RequestCrossGameTitleCard();
    return 1;
}
#endif
