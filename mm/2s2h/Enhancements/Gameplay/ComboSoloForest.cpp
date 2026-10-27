#ifdef COMBO_BUILD
#include "gameplay/CrossGameTitleCard.h"
#include "ComboSoloForest.h"
#include "entrances/SoloForest.h"
#include "entrances/LostWoodsEntrance.h"
#include "entrances/SoloForestResources.h"
#include "entrances/NativeForest.h"
#include "entrances/NativeForestResourceData.h"
#include "global.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"
#include "2s2h/SaveManager/SaveManager.h"
#include "2s2h/Enhancements/Graphics/ComboPlaceTitleCard.h"
#include "actors/ForestGiParticles.h"
#include "../../../../soh/soh/Enhancements/randomizer/NeiGiRender.h"
#include <algorithm>
#include <cmath>
#include <filesystem>

extern "C" {
extern SceneEntranceTableEntry sSceneEntranceTable[ENTR_SCENE_MAX];
u8 ResourceMgr_FileExists(const char* path);
}
int SaveManager_ReadSaveFile(const std::filesystem::path&, nlohmann::json&);

namespace {
// Display-list macros redeclare these helpers at block scope in this namespace.
extern "C" void FrameInterpolation_RecordOpenChild(const void* a, int b);
extern "C" void FrameInterpolation_RecordCloseChild(void);
struct ForestOrigin {
    int entrance;
    Vec3f pos;
    s16 yaw;
    s8 room;
};
ForestOrigin DefaultForestOrigin() {
    return { ENTRANCE(WOODS_OF_MYSTERY, 0), { 0, 0, 0 }, -8192, 1 };
}
ForestOrigin sForestOrigin = DefaultForestOrigin(), sGroundedOrigin = sForestOrigin;
int sForestSlot = -1;
u32 sForestSeed;
bool sForestResume = false, sForestArmed = false, sOriginStaged = false, sLocalReturning = false, sForestTitle = false;
RespawnData sForestPreservedReturn;
alignas(2) constexpr char kForestTitle[] = "__OTR__custom/oot_location_titles/v1/lost_woods";
u32 ForestSeed() {
    return gSaveContext.save.shipSaveInfo.rando.finalSeed;
}
bool ForestPack() {
    return NativeForest::SceneAvailable(NativeForest::kMmSoloScene, ResourceMgr_FileExists, ResourceMgr_FileExists,
                                        NativeForest::LoadDependencies) ||
           std::all_of(std::begin(kSoloForestResources), std::end(kSoloForestResources),
                       [](const char* path) { return ResourceMgr_FileExists(path); });
}
bool ForestVisit(const PlayState* play) {
    return play &&
           ComboSoloForest_IsMmVisit(play->sceneId, play->roomCtx.curRoom.num, gSaveContext.gameMode,
                                     gSaveContext.fileNum, gSaveContext.sceneLayer, gSaveContext.save.cutsceneIndex);
}
bool NativePortal(const PlayState* play) {
    const auto* shape = play ? play->roomCtx.curRoom.roomShape : nullptr;
    return shape && shape->base.type == ROOM_SHAPE_TYPE_CULLABLE &&
           NativeForest::PortalEntries(shape->cullable.entries, shape->cullable.numEntries);
}
bool OriginScene(const PlayState* play) {
    return play && play->sceneId == SCENE_26SARUNOMORI && play->roomCtx.curRoom.num == 1;
}
bool ForestSafe(const PlayState* play) {
    return gComboGameMode == COMBO_GAME_MM && play && gGameState == &play->state && play->state.running &&
           gSaveContext.gameMode == GAMEMODE_NORMAL && ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) &&
           play->transitionTrigger == TRANS_TRIGGER_OFF && play->transitionMode == TRANS_MODE_OFF &&
           play->pauseCtx.state == PAUSE_STATE_OFF && play->gameOverCtx.state == GAMEOVER_INACTIVE &&
           play->csCtx.state == CS_STATE_IDLE && !gSaveContext.cutsceneTrigger && !play->soaringCsOrSoTCsPlaying;
}
void ForestTransition(PlayState* play, int entrance) {
    Combo_MM_RequestCrossGameTitleCard();
    play->nextEntrance = entrance;
    play->transitionType = gSaveContext.nextTransitionType = TRANS_TYPE_FADE_BLACK;
    play->transitionTrigger = TRANS_TRIGGER_START;
}
void StageForestOrigin() {
    if (sOriginStaged)
        return;
    auto& arrival = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    sForestPreservedReturn = arrival;
    sOriginStaged = true;
    arrival = {};
    arrival.entrance = sForestOrigin.entrance;
    arrival.roomIndex = sForestOrigin.room;
    arrival.pos = sForestOrigin.pos;
    arrival.yaw = sForestOrigin.yaw;
    arrival.playerParams = PLAYER_PARAMS(0xFF, PLAYER_START_MODE_E);
    arrival.data = 1;
    gSaveContext.respawnFlag = 2;
}
void ForestPlayerUpdate() {
    auto* play = gPlayState;
    if (!ForestSafe(play) || Player_InCsMode(play))
        return;
    if (sOriginStaged) {
        gSaveContext.respawn[RESPAWN_MODE_RETURN] = sForestPreservedReturn;
        sOriginStaged = false;
    }
    if (sLocalReturning && OriginScene(play)) {
        sLocalReturning = false;
        sForestArmed = false;
        return;
    }
    auto* player = GET_PLAYER(play);
    if (!player || sForestSlot != gSaveContext.fileNum || sForestSeed != ForestSeed())
        return;
    const auto pos = player->actor.world.pos;
    if (ForestVisit(play)) {
        if (play->roomCtx.curRoom.num != 0)
            return;
        const float dx = pos.x - COMBO_SOLO_FOREST_STUMP_X, dz = pos.z - COMBO_SOLO_FOREST_STUMP_Z;
        if (dx * dx + dz * dz > 14400)
            sForestArmed = true;
        if (sForestArmed && ComboSoloForest_AtStumpWidth(pos.x, pos.y, pos.z, NativePortal(play) ? 42 : 58)) {
            sForestArmed = false;
            sLocalReturning = true;
            ForestTransition(play, sForestOrigin.entrance);
        }
        return;
    }
    if (!OriginScene(play) || !ForestPack())
        return;
    const bool inside = ComboLostWoods_InMmPassage(play->sceneId, play->roomCtx.curRoom.num, pos.x, pos.y, pos.z);
    if (!inside) {
        sForestArmed = true;
        if (player->actor.floorPoly && player->actor.floorPoly->normal.y >= 23170 &&
            std::isfinite(player->actor.floorHeight) && std::abs(pos.y - player->actor.floorHeight) < 35)
            sGroundedOrigin = { gSaveContext.save.entrance, pos, player->actor.shape.rot.y, 1 };
    } else if (sForestArmed) {
        sForestOrigin = sGroundedOrigin;
        sForestArmed = false;
        ForestTransition(play, COMBO_SOLO_FOREST_MM_ENTRANCE);
    }
}
void LoadForestState(const nlohmann::json& value, int slot) {
    sForestSlot = slot;
    sForestSeed = ForestSeed();
    sForestResume = false;
    sOriginStaged = sLocalReturning = sForestArmed = sForestTitle = false;
    sForestOrigin = DefaultForestOrigin();
    if (gComboGameMode != COMBO_GAME_MM || !ComboLostWoods_IsPlayableSlot(slot)) {
        sForestSlot = -1;
        return;
    }
    if (value.is_object() && value.value("seed", nlohmann::json()) == sForestSeed) {
        try {
            const auto point = value.at("origin");
            const int entrance = point.at("entrance"), yaw = point.at("yaw");
            const Vec3f pos = { point.at("x"), point.at("y"), point.at("z") };
            if ((entrance >> 9) == ENTR_SCENE_WOODS_OF_MYSTERY && entrance >= 0 && point.at("room") == 1 &&
                std::isfinite(pos.x) && std::isfinite(pos.y) && std::isfinite(pos.z) && std::abs(pos.x) < 4096 &&
                std::abs(pos.y) < 4096 && std::abs(pos.z) < 4096 && yaw >= INT16_MIN && yaw <= INT16_MAX)
                sForestOrigin = { entrance, pos, static_cast<s16>(yaw), 1 };
        } catch (const nlohmann::json::exception&) {}
        sForestResume = value.value("savedInside", nlohmann::json()) == true;
    }
    sGroundedOrigin = sForestOrigin;
}
void SaveForestState(nlohmann::json& document, int slot) {
    if (gComboGameMode != COMBO_GAME_MM || sForestSlot != slot || slot != gSaveContext.fileNum ||
        sForestSeed != ForestSeed() || !gPlayState || gGameState != &gPlayState->state || !gPlayState->state.running)
        return;
    document["soloForest"] = { { "version", 1 },
                               { "seed", sForestSeed },
                               { "savedInside", ForestVisit(gPlayState) &&
                                                    gSaveContext.save.entrance == COMBO_SOLO_FOREST_MM_ENTRANCE },
                               { "origin",
                                 { { "entrance", sForestOrigin.entrance },
                                   { "room", sForestOrigin.room },
                                   { "x", sForestOrigin.pos.x },
                                   { "y", sForestOrigin.pos.y },
                                   { "z", sForestOrigin.pos.z },
                                   { "yaw", sForestOrigin.yaw } } } };
}
void NormalizeForestEntry() {
    if (gComboGameMode != COMBO_GAME_MM || !ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum))
        return;
    const bool matches = sForestSlot == gSaveContext.fileNum && sForestSeed == ForestSeed();
    if (sForestResume) {
        sForestResume = false;
        if (matches) {
            gSaveContext.save.entrance = COMBO_SOLO_FOREST_MM_ENTRANCE;
            gSaveContext.respawnFlag = 0;
        }
    }
    if (sLocalReturning && matches && gSaveContext.save.entrance == sForestOrigin.entrance) {
        StageForestOrigin();
        return;
    }
    if (gSaveContext.save.entrance != COMBO_SOLO_FOREST_MM_ENTRANCE)
        return;
    gSaveContext.save.cutsceneIndex = 0;
    gSaveContext.nextCutsceneIndex = 0xFFEF;
    gSaveContext.cutsceneTrigger = 0;
    if (!ForestPack()) {
        if (!matches)
            sForestOrigin = DefaultForestOrigin();
        sLocalReturning = true;
        gSaveContext.save.entrance = sForestOrigin.entrance;
        StageForestOrigin();
        return;
    }
    const auto& arrival = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    if (gSaveContext.respawnFlag == 2 && arrival.entrance == COMBO_SOLO_FOREST_MM_ENTRANCE && arrival.roomIndex == 0 &&
        std::isfinite(arrival.pos.x) && std::isfinite(arrival.pos.y) && std::isfinite(arrival.pos.z))
        return;
    gSaveContext.respawnFlag = 0; // void/death/reload starts at the safe private room-zero spawn
}

void DrawForestPortal() {
    auto* play = gPlayState;
    if (!ForestVisit(play) || play->roomCtx.curRoom.num != 0 || !ForestSafe(play))
        return;
    if (sForestTitle && (!ResourceMgr_FileExists("custom/oot_location_titles/v1/lost_woods") ||
                         ComboPlaceTitleCard_Show(play, const_cast<char*>(kForestTitle), 144, 24) != 0))
        sForestTitle = false;
    Matrix_Push();
    Matrix_Translate(COMBO_SOLO_FOREST_STUMP_X, 0, COMBO_SOLO_FOREST_STUMP_Z, MTXMODE_NEW);
    Matrix_RotateYS(COMBO_SOLO_FOREST_STUMP_YAW, MTXMODE_APPLY);
    const bool authored = ResourceMgr_FileExists(NativeForest::kAuthoredPortalMarker);
    const auto mesh =
        ForestGiParticles::Portal(play->gameplayFrames, !NativePortal(play) || authored, NeiGi_CameraBasis(play));
    NeiGi_DrawWorldParticles(play, mesh);
    Matrix_Pop();
}
void RegisterSoloForest() {
    if (gComboGameMode != COMBO_GAME_MM)
        return;
    static EntranceTableEntry entries[4] = { { COMBO_SOLO_FOREST_MM_SCENE, 0, TRANS_TYPE_FADE_BLACK << 7 },
                                             { COMBO_SOLO_FOREST_MM_SCENE, 0, TRANS_TYPE_FADE_BLACK << 7 },
                                             { COMBO_SOLO_FOREST_MM_SCENE, 0, TRANS_TYPE_FADE_BLACK << 7 },
                                             { COMBO_SOLO_FOREST_MM_SCENE, 0, TRANS_TYPE_FADE_BLACK << 7 } };
    static EntranceTableEntry* spawns[] = { entries };
    auto& scene = gSceneTable[COMBO_SOLO_FOREST_MM_SCENE];
    scene.segment.vromStart = scene.segment.vromEnd = 0;
    scene.segment.fileName = (char*)"skiejer_forest";
    scene.titleTextId = 0;
    scene.drawConfig = SCENE_DRAW_CFG_MAT_ANIM;
    scene.unk_A = scene.unk_C = scene.unk_D = 0;
    sSceneEntranceTable[ENTR_SCENE_UNSET_0B] = { 1, spawns, (char*)"SKIEJER_FOREST" };
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSaveLoad>([](s16 slot) {
        nlohmann::json document;
        if (SaveManager_ReadSaveFile(SaveManager_GetFileName(slot + 1), document) != 0)
            document = nullptr;
        LoadForestState(document.is_object() ? document.value("soloForest", nlohmann::json()) : nlohmann::json(), slot);
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSaveInit>([](s16) { LoadForestState(nullptr, -1); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](s8, s8) {
        sForestArmed = false;
        sForestTitle = ForestVisit(gPlayState);
    });
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnActorUpdate>(
        ACTOR_PLAYER, [](Actor*) { ForestPlayerUpdate(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDrawWorldEnd>(DrawForestPortal);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>(
        [] { sForestArmed = sForestTitle = false; });
}
RegisterShipInitFunc init(RegisterSoloForest);
} // namespace
void ComboSoloForestMm_SaveState(nlohmann::json& save, int slot) {
    SaveForestState(save, slot);
}
extern "C" void ComboSoloForestMm_NormalizeEntry(void) {
    NormalizeForestEntry();
}
extern "C" int ComboSoloForestMm_PassageAvailable(void) {
    return gComboGameMode == COMBO_GAME_MM && gSaveContext.gameMode == GAMEMODE_NORMAL &&
           ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) && sForestSlot == gSaveContext.fileNum &&
           sForestSeed == ForestSeed() && ForestPack();
}
#endif
