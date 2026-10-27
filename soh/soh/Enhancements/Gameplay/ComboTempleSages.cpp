#ifdef COMBO_BUILD
#include "global.h"
#include "ComboTempleSages.h"
#include "actors/TempleSagesZeldaAura.h"
#include "soh/Enhancements/randomizer/NeiGiRender.h"
#include "ComboGameMode.h"
#include "gameplay/CrossGameTitleCard.h"
#include "textures/place_title_cards/g_pn_14.h"
#include "entrances/TempleSagesPolicy.h"
#include "entrances/TempleSagesReturnPlacements.h"
#include "entrances/TempleSagesConfirmationNative.h"
#include "soh/resource/type/Scene.h"
#include "soh/resource/type/scenecommand/SetActorList.h"
#include "soh/resource/type/scenecommand/SetAlternateHeaders.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/ShipInit.hpp"
#include "soh/frame_interpolation.h"
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManager.h>
#include <string>

extern "C" bool NeiGi_CanDrawLayers(PlayState*, size_t, size_t, size_t);

namespace {
// OPEN/CLOSE_DISPS redeclare these at block scope; retain the native C ABI.
extern "C" void FrameInterpolation_RecordOpenChild(const void*, int);
extern "C" void FrameInterpolation_RecordCloseChild(void);

static_assert(QUEST_MEDALLION_FOREST == 0 && QUEST_MEDALLION_FIRE == 1 && QUEST_MEDALLION_WATER == 2 &&
              QUEST_MEDALLION_SPIRIT == 3 && QUEST_MEDALLION_SHADOW == 4 && QUEST_MEDALLION_LIGHT == 5);
static_assert(QUEST_SONG_MINUET == 6 && QUEST_SONG_BOLERO == 7 && QUEST_SONG_SERENADE == 8 && QUEST_SONG_REQUIEM == 9 &&
              QUEST_SONG_NOCTURNE == 10);
static_assert(TempleSages::Entrance(0) == ENTR_SACRED_FOREST_MEADOW_WARP_PAD &&
              TempleSages::Entrance(1) == ENTR_DEATH_MOUNTAIN_CRATER_WARP_PAD &&
              TempleSages::Entrance(2) == ENTR_LAKE_HYLIA_WARP_PAD &&
              TempleSages::Entrance(3) == ENTR_DESERT_COLOSSUS_WARP_PAD &&
              TempleSages::Entrance(4) == ENTR_GRAVEYARD_WARP_PAD && TempleSages::Entrance(5) == -1);
static_assert(TempleSages::kScene == SCENE_TEMPLE_OF_TIME && TempleSages::kMarkerActor == ACTOR_EN_OKARINA_TAG);

TempleSages::PadCache sPads;
unsigned sCaptureGeneration = 0;
bool sReturnVisit = false;
bool sWarpStarted = false;
struct ChamberTitle {
    int file = -1;
    bool pending = false;
    bool tunnelArmed = false;
    bool fromTunnel = false;
};
ChamberTitle sTitle;
std::optional<int> sSelection;
std::array<TempleSages::Pad, TempleSages::kPadCount> sHubPads{};
struct PendingReturn {
    bool active = false;
    int file = -1;
    RespawnData previous{}, owned{};
};
PendingReturn sPending;

bool OwnsReturn() {
    const auto& current = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    const auto& owned = sPending.owned;
    return current.entranceIndex == owned.entranceIndex && current.roomIndex == owned.roomIndex &&
           current.pos.x == owned.pos.x && current.pos.y == owned.pos.y && current.pos.z == owned.pos.z &&
           current.yaw == owned.yaw && current.playerParams == owned.playerParams && current.data == owned.data;
}
void FinishPending(bool abort) {
    if (sPending.active && sPending.file == gSaveContext.fileNum && (!abort || gSaveContext.respawnFlag == 2) &&
        OwnsReturn()) {
        gSaveContext.respawn[RESPAWN_MODE_RETURN] = sPending.previous;
        if (abort && gSaveContext.respawnFlag == 2)
            gSaveContext.respawnFlag = 0;
        if (!abort) {
            sTitle = {};
            sTitle.file = gSaveContext.fileNum;
            sTitle.pending = true;
        }
    }
    sPending = {};
}

// Native marker yaw points from each outer pillar toward the central bridge.
TempleSages::Pad HubArrival(TempleSages::Pad marker) {
    const float angle = marker.yaw * (6.28318530718f / 65536.0f);
    marker.position.x += std::sin(angle) * 100.0f;
    marker.position.z += std::cos(angle) * 100.0f;
    return marker;
}

// Read the active room header, retaining no scene-owned pointers across visits.
// Mirror Scene_CommandAlternateHeaderList, including adult-night fallback.
bool LoadHubPads(const TempleSages::Visit& destination) {
    Ship::OwnRMScope owner("oot");
    auto room = std::dynamic_pointer_cast<SOH::Scene>(
        ResourceMgr_GetResourceByNameHandlingMQ("scenes/shared/tokinoma_scene/tokinoma_room_0"));
    if (!room)
        return false;
    const auto baseRoom = room;
    for (const auto& command : baseRoom->commands) {
        auto alternate = std::dynamic_pointer_cast<SOH::SetAlternateHeaders>(command);
        if (!alternate || destination.setup == 0)
            continue;
        const size_t index = destination.setup - 1;
        if (index >= alternate->headers.size())
            return false;
        auto selected = alternate->headers[index];
        if (!selected && destination.setup == 3 && alternate->headers.size() > 1)
            selected = alternate->headers[1];
        if (selected)
            room = selected;
        break;
    }
    TempleSages::PadCache hub;
    auto visit = destination;
    visit.scene = TempleSages::kScene;
    visit.room = TempleSages::kRoom;
    bool found = false;
    for (const auto& command : room->commands) {
        auto actors = std::dynamic_pointer_cast<SOH::SetActorList>(command);
        if (!actors)
            continue;
        if (found || actors->numActors != actors->actorList.size())
            return false;
        hub.Capture(visit, actors->actorList.data(), actors->actorList.size());
        found = true;
    }
    if (!found || !hub.Ready(visit))
        return false;
    sHubPads = hub.Pads();
    return true;
}
// Private portal geometry is local-space and caller-tinted. Markers are at the
// landing floor; no native raised pad mesh or Light travel route is used.
// Stable, separate route bindings allow texture overrides after compilation.
// Fixed even row stride retains the native resource signature alignment.
alignas(2) constexpr char kPortalEffects[TempleSages::kPadCount][96] = {
    "__OTR__custom/temple_sages/portals/forest/effect", "__OTR__custom/temple_sages/portals/fire/effect",
    "__OTR__custom/temple_sages/portals/water/effect",  "__OTR__custom/temple_sages/portals/spirit/effect",
    "__OTR__custom/temple_sages/portals/shadow/effect",
};

bool PortalResourcesReady() {
    // Normal port archives own these resources too. Mod provenance rejects
    // soh.o2r, and host extension caches can belong to MM in a combined game.
    // Inspect only the registered OoT owner's selected archive namespace.
    const auto owner = Ship::CrossRMRegistry::Get("oot");
    if (!owner)
        return false;
    const auto archives = owner->GetArchiveManager();
    if (!archives)
        return false;
    const bool alt = owner->IsAltAssetsEnabled();
    const auto available = [&](const std::string& path) {
        if (alt && (archives->HasFile("alt/" + path) || archives->HasFile("alt/" + path + ".meta")))
            return true;
        return archives->HasFile(path) || archives->HasFile(path + ".meta");
    };
    constexpr const char* dependencies[] = { "material", "texture_flow", "texture_detail", "geometry", "geometry_vtx" };
    for (const auto& effect : kPortalEffects) {
        if (!available(effect + 7))
            return false;
        const std::string path(effect + 7, std::char_traits<char>::length(effect + 7) - 6);
        for (const char* suffix : dependencies) {
            if (!available(path + suffix))
                return false;
        }
    }
    return true;
}
constexpr u8 kPortalColors[TempleSages::kPadCount][3] = {
    { 105, 255, 145 }, { 255, 105, 40 }, { 90, 180, 255 }, { 255, 195, 85 }, { 190, 105, 255 }
};

bool OotSession() {
    return gComboGameMode == COMBO_GAME_BOTH || gComboGameMode == COMBO_GAME_OOT;
}

TempleSages::Visit CurrentVisit(const PlayState* play) {
    if (!play)
        return {};
    return { play,
             play->sceneSegment,
             play->roomCtx.curRoom.segment,
             play->sceneNum,
             play->roomCtx.curRoom.num,
             gSaveContext.sceneLayer,
             play->roomCtx.status == 0 && play->roomCtx.curRoom.segment != nullptr };
}

void Reset() {
    TempleSages::ConfirmationNative::Reset(gPlayState);
    sSelection.reset();
    sWarpStarted = false;
    sPads.Reset();
    sReturnVisit = false;
    sTitle = {};
    ++sCaptureGeneration;
}

void Capture(s16 scene) {
    auto* play = gPlayState;
    Reset();
    if (!OotSession() || !play || scene != play->sceneNum)
        return;
    const auto visit = CurrentVisit(play);
    if (scene == SCENE_TEMPLE_OF_TIME) {
        sPads.Capture(visit, play->setupActorList, play->numSetupActors);
        if (sPending.active && !sPads.Ready(visit) && visit.roomReady)
            FinishPending(true);
        return;
    }
    if (!TempleSages::LoadedVisit(visit))
        return;
    for (int route = 0; route < int(TempleSages::kPadCount); ++route) {
        const auto& placement = TempleSages::kReturnPlacements[route];
        if (visit.scene == placement.scene && visit.room == placement.room && LoadHubPads(visit)) {
            sPads.CaptureReturn(visit, route, placement.effect);
            sReturnVisit = true;
            return;
        }
    }
}

bool Safe(PlayState* play, bool ownConfirmation = false) {
    const bool owned = ownConfirmation && TempleSages::ConfirmationNative::Active() &&
                       TempleSages::ConfirmationNative::SameVisit(play);
    const bool fading = owned && TempleSages::ConfirmationNative::Holding() && sWarpStarted;
    return OotSession() && play && gGameState == &play->state && play->state.running &&
           gSaveContext.gameMode == GAMEMODE_NORMAL && gSaveContext.fileNum >= 0 && gSaveContext.fileNum < 3 &&
           sPads.Ready(CurrentVisit(play)) && gSaveContext.cutsceneIndex < 0xFFF0 &&
           gSaveContext.nextCutsceneIndex == 0xFFEF && !gSaveContext.cutsceneTrigger &&
           play->csCtx.state == CS_STATE_IDLE && play->pauseCtx.state == 0 &&
           play->gameOverCtx.state == GAMEOVER_INACTIVE &&
           (fading || (play->transitionMode == TRANS_MODE_OFF && play->transitionTrigger == TRANS_TRIGGER_OFF &&
                       gSaveContext.respawnFlag == 0)) &&
           (owned || GameInteractor::IsPlayerInControl());
}

TempleSages::TravelMode TravelModeSetting() {
    return TempleSages::NormalizeTravelMode(CVarGetInteger("gEnhancements.TempleSagesTravel", 1));
}

void UpdateTitle(PlayState* play) {
    const auto visit = CurrentVisit(play);
    if (!OotSession() || !TempleSages::NormalVisit(visit) || !sPads.Ready(visit) || sReturnVisit ||
        gSaveContext.fileNum < 0 || gSaveContext.fileNum >= 3 ||
        (sTitle.file >= 0 && sTitle.file != gSaveContext.fileNum)) {
        sTitle = {};
        return;
    }
    sTitle.file = gSaveContext.fileNum;
    if (!Safe(play) || TempleSages::ConfirmationNative::Active())
        return;
    const auto* player = GET_PLAYER(play);
    if (!player)
        return;
    const auto& pos = player->actor.world.pos;
    if (!TempleSages::Finite({ pos.x, pos.y, pos.z }))
        return;

    // The chamber enclosure opens at z=-1200. Require a tunnel-side approach
    // before crossing its mouth, with hysteresis to avoid edge jitter/repeats.
    const bool opening = std::fabs(pos.x) < 100.f && pos.y >= -150.f && pos.y <= 130.f;
    if (sTitle.fromTunnel && pos.z >= -1160.f)
        sTitle.pending = false;
    if (opening && pos.z >= -1160.f && pos.z <= -720.f) {
        sTitle.tunnelArmed = true;
    } else if (opening && pos.z <= -1240.f && sTitle.tunnelArmed) {
        sTitle.pending = sTitle.fromTunnel = true;
        sTitle.tunnelArmed = false;
    } else if (!opening || pos.z > -720.f) {
        sTitle.tunnelArmed = false;
    }

    auto& card = play->actorCtx.titleCtx;
    if (!sTitle.pending || card.durationTimer || card.delayTimer || card.alpha)
        return;
    struct PresentationScope {
        int previous = Combo_SOH_SwapCustomTitleCardPresentation(true);
        ~PresentationScope() {
            Combo_SOH_SwapCustomTitleCardPresentation(previous);
        }
    } scope;
    sTitle.pending = false;
    if (!GameInteractor_Should(VB_SHOW_TITLE_CARD, true))
        return;

    // This annex still belongs to SCENE_TEMPLE_OF_TIME. InitPlaceName would
    // replace our texture with the temple card, so initialize its ordinary
    // place-card context with the chamber's native resource instead. Native
    // update/draw, HUD cosmetics and HD/Alt replacements retain ownership.
    const char* texture = gChamberOfTheSagesTitleCardENGTex;
    if (gSaveContext.language == LANGUAGE_GER)
        texture = gChamberOfTheSagesTitleCardGERTex;
    else if (gSaveContext.language == LANGUAGE_FRA)
        texture = gChamberOfTheSagesTitleCardFRATex;
    else if (gSaveContext.language == LANGUAGE_JPN) {
        // Match the suffix selection in native TitleCard_InitPlaceName.
        alignas(2) static constexpr char japanese[] = "__OTR__textures/g_pn_14/gChamberOfTheSagesTitleCardJPNTex";
        texture = japanese;
    }
    card = {};
    card.texture = const_cast<char*>(texture);
    card.isBossCard = false;
    card.hasTranslation = false;
    card.x = 160;
    card.y = 120;
    card.width = 144;
    card.height = 24;
    card.durationTimer = 80;
    card.delayTimer = 20;
}

void PlayerUpdate() {
    sPads.ObserveMode(TravelModeSetting());
    auto* play = gPlayState;
    if (sPending.active) {
        if (!OotSession() || !play || sPending.file != gSaveContext.fileNum ||
            (play->sceneNum != SCENE_TEMPLE_OF_TIME && gSaveContext.respawnFlag != 2))
            FinishPending(true);
        else if (play->sceneNum == SCENE_TEMPLE_OF_TIME && gSaveContext.respawnFlag == 0 &&
                 CurrentVisit(play).roomReady)
            FinishPending(!sPads.Ready(CurrentVisit(play)));
    }
    UpdateTitle(play);
    bool confirmed = false;
    if (TempleSages::ConfirmationNative::Active()) {
        const auto decision = TempleSages::ConfirmationNative::Update(play);
        if (decision != TempleSages::ConfirmationNative::Result::Yes) {
            if (decision != TempleSages::ConfirmationNative::Result::Pending)
                sSelection.reset();
            return;
        }
        confirmed = true;
    }
    if (!Safe(play, confirmed)) {
        if (confirmed)
            TempleSages::ConfirmationNative::Reset(play);
        sSelection.reset();
        return;
    }
    auto* player = GET_PLAYER(play);
    if (!player)
        return;
    constexpr u32 blocked = PLAYER_STATE1_INPUT_DISABLED | PLAYER_STATE1_TALKING | PLAYER_STATE1_DEAD |
                            PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_HANGING_OFF_LEDGE |
                            PLAYER_STATE1_CLIMBING_LEDGE | PLAYER_STATE1_CLIMBING_LADDER | PLAYER_STATE1_ON_HORSE |
                            PLAYER_STATE1_IN_WATER | PLAYER_STATE1_IN_ITEM_CS | PLAYER_STATE1_FLOOR_DISABLED;
    const auto& actor = player->actor;
    const auto route = confirmed ? sSelection
                                 : sPads.Tick({ CurrentVisit(play),
                                                { actor.world.pos.x, actor.world.pos.y, actor.world.pos.z },
                                                actor.floorHeight,
                                                actor.floorPoly ? actor.floorPoly->normal.y : 0,
                                                gSaveContext.inventory.questItems,
                                                (player->stateFlags1 & blocked) == 0,
                                                (actor.bgCheckFlags & BGCHECKFLAG_GROUND) != 0,
                                                actor.floorPoly != nullptr,
                                                TravelModeSetting() });
    if (!route)
        return;
    if (!confirmed) {
        if (!PortalResourcesReady())
            return;
        if (TempleSages::ConfirmationNative::Start(play, *route, sReturnVisit, sPads.Pads()[*route]))
            sSelection = route;
        return;
    }
    sSelection.reset();
    if (!TempleSages::Unlocked(*route, gSaveContext.inventory.questItems, TravelModeSetting()) ||
        (player->stateFlags1 & blocked) != 0 || !sPads.Ready(CurrentVisit(play)) || !actor.floorPoly ||
        actor.floorPoly->normal.y < 23170 || !(actor.bgCheckFlags & BGCHECKFLAG_GROUND) ||
        !TempleSages::Finite({ actor.world.pos.x, actor.world.pos.y, actor.world.pos.z }) ||
        !std::isfinite(actor.floorHeight) ||
        std::fabs(actor.world.pos.y - sPads.Pads()[*route].position.y) >= TempleSages::kHeightTolerance ||
        std::fabs(actor.floorHeight - sPads.Pads()[*route].position.y) >= TempleSages::kHeightTolerance) {
        TempleSages::ConfirmationNative::Reset(play);
        return;
    }
    if (sReturnVisit && !LoadHubPads(CurrentVisit(play))) {
        TempleSages::ConfirmationNative::Reset(play);
        return;
    }
    sWarpStarted = true;

    if (sReturnVisit) {
        const auto target = HubArrival(sHubPads[*route]);
        auto& arrival = gSaveContext.respawn[RESPAWN_MODE_RETURN];
        sPending = { true, gSaveContext.fileNum, arrival, {} };
        arrival = {};
        arrival.entranceIndex = ENTR_TEMPLE_OF_TIME_ENTRANCE;
        arrival.roomIndex = TempleSages::kRoom;
        arrival.pos = { target.position.x, target.position.y, target.position.z };
        arrival.yaw = target.yaw;
        arrival.playerParams = 0x0DFF; // Native idle arrival, no song/cutscene ownership.
        arrival.data = *route;
        sPending.owned = arrival;
        gSaveContext.respawnFlag = 2;
        gSaveContext.cutsceneIndex = 0;
        gWeatherMode = 0;
        play->nextEntranceIndex = arrival.entranceIndex;
        play->transitionType = gSaveContext.nextTransitionType = TRANS_TYPE_FADE_WHITE;
        play->transitionTrigger = TRANS_TRIGGER_START;
        return;
    }

    // Native -3 respawn changes the start mode, retaining the target scene's
    // authored spawn position. Its data field selects the warp-in effect color.
    // Do not call Environment_WarpSongLeave: song randomization owns that hook.
    const int entrance = TempleSages::Entrance(*route);
    auto& arrival = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    arrival.entranceIndex = entrance;
    arrival.playerParams = 0x5FF;
    arrival.data = *route;
    gWeatherMode = 0;
    gSaveContext.cutsceneIndex = 0;
    gSaveContext.respawnFlag = -3;
    play->nextEntranceIndex = entrance;
    play->transitionType = gSaveContext.nextTransitionType = TRANS_TYPE_FADE_WHITE;
    play->transitionTrigger = TRANS_TRIGGER_START;
}

void Draw() {
    auto* play = gPlayState;
    if (!Safe(play, true) || !play->state.gfxCtx || !PortalResourcesReady() ||
        (sReturnVisit && TravelModeSetting() == TempleSages::TravelMode::Off))
        return;
    // Dormant portals identify every destination before its reward/song is earned.
    // The selected mode gates travel in PlayerUpdate/PadCache.
    const size_t count = sReturnVisit ? 1 : TempleSages::kPadCount;
    const size_t commands = 16 + count * 4;
    if (!NeiGi_CanDrawLayers(play, count, 0, commands))
        return;

    // gSPDisplayList resolves ordinary __OTR__ signatures while building the
    // stream; the renderer's RM push then owns nested texture/resource calls.
    Ship::OwnRMScope owner("oot");
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    gSPComboRMPush(POLY_XLU_DISP++, "oot");
    const auto travelMode = TravelModeSetting();
    for (int route = 0; route < int(TempleSages::kPadCount); ++route) {
        if (!sPads.Active(route))
            continue;
        const bool earned = TempleSages::Unlocked(route, gSaveContext.inventory.questItems, travelMode);
        const unsigned period = earned ? 240u : 480u;
        const float wave = std::sin((play->gameplayFrames % period) * (6.28318530718f / period));
        const u8 alpha = earned ? u8(155.0f + 35.0f * wave) : u8(52.0f + 10.0f * wave);
        const auto& pad = sPads.Pads()[route];
        FrameInterpolation_RecordOpenChild(&pad, int(sCaptureGeneration & 0x7FFFFFFFu));
        Matrix_Push();
        Matrix_Translate(pad.position.x, pad.position.y + 1.5f, pad.position.z, MTXMODE_NEW);
        Matrix_RotateY(pad.yaw * (6.28318530718f / 65536.0f), MTXMODE_APPLY);
        gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gDPPipeSync(POLY_XLU_DISP++);
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, kPortalColors[route][0], kPortalColors[route][1],
                        kPortalColors[route][2], alpha);
        gSPDisplayList(POLY_XLU_DISP++, reinterpret_cast<Gfx*>(const_cast<char*>(kPortalEffects[route])));
        Matrix_Pop();
        FrameInterpolation_RecordCloseChild();
    }
    gSPComboRMPop(POLY_XLU_DISP++);
    gDPPipeSync(POLY_XLU_DISP++);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    CLOSE_DISPS(play->state.gfxCtx);
}

void Register() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::AfterSceneCommands>(Capture);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](s16 scene) {
        Reset();
        if (sPending.active && (scene != SCENE_TEMPLE_OF_TIME || !OotSession()))
            FinishPending(true);
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>([](int32_t) {
        FinishPending(true);
        Reset();
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnExitGame>([](int32_t) {
        FinishPending(true);
        Reset();
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>([] {
        TempleSages::ConfirmationNative::Reset(gPlayState, true);
        Reset();
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(PlayerUpdate);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDrawEnd>(Draw);
}
RegisterShipInitFunc init(Register);
} // namespace

extern "C" void ComboTempleSages_DrawZeldaAuraAt(Actor* actor, PlayState* play, float x, float y, float z) {
    if (!actor || !play || !TempleSagesZeldaAura::Enabled(actor->id, actor->params) || !OotSession() ||
        !play->state.running || !play->state.gfxCtx || !play->sceneSegment || !play->roomCtx.curRoom.segment ||
        play->roomCtx.status != 0 || (actor->room != -1 && actor->room != play->roomCtx.curRoom.num) ||
        play->pauseCtx.state != 0 || play->csCtx.state != CS_STATE_IDLE ||
        play->gameOverCtx.state != GAMEOVER_INACTIVE || play->transitionTrigger != TRANS_TRIGGER_OFF ||
        play->transitionMode != TRANS_MODE_OFF || gSaveContext.cutsceneTrigger ||
        gSaveContext.gameMode != GAMEMODE_NORMAL || gSaveContext.sceneLayer >= 4 ||
        !TempleSagesZeldaAura::Finite(actor->xzDistToPlayer) || !TempleSagesZeldaAura::Finite(actor->yDistToPlayer) ||
        actor->xzDistToPlayer > 1800.f || std::fabs(actor->yDistToPlayer) > 400.f ||
        !TempleSagesZeldaAura::Finite({ actor->world.pos.x, actor->world.pos.y, actor->world.pos.z }) ||
        !TempleSagesZeldaAura::Finite({ x, y, z }))
        return;
    Matrix_Push();
    Matrix_Translate(x, y, z, MTXMODE_NEW);
    const auto mesh = TempleSagesZeldaAura::Sample(actor->params, play->gameplayFrames, NeiGi_CameraBasis(play));
    NeiGi_DrawWorldParticles(play, mesh);
    Matrix_Pop();
}

extern "C" void ComboTempleSages_DrawZeldaAura(Actor* actor, PlayState* play) {
    if (actor)
        ComboTempleSages_DrawZeldaAuraAt(actor, play, actor->world.pos.x, actor->world.pos.y, actor->world.pos.z);
}
#endif
