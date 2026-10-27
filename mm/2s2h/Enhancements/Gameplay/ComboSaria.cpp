#ifdef COMBO_BUILD
#include "ComboSaria.h"
#include "ComboOpeningForest.h"
#include "ComboForestActors.h"
#include "MmPlacedSariaResources.h"
#include "ComboForestEpona.h"
#include "entrances/ForestActorPlacements.h"
#include "ComboResolve.h"
#include "actors/PlacedSariaABI.h"
#include "actors/ForestGiParticles.h"
#include "../../../../soh/soh/Enhancements/randomizer/NeiGiRender.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include "2s2h/CustomMessage/CustomMessage.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/CustomItem/CustomItem.h"
#include "2s2h/SaveManager/SaveManager.h"
#include "2s2h/Rando/Rando.h"
#include "2s2h/Rando/ItemReceiptText.h"
#include "2s2h/Rando/MiscBehavior/MiscBehavior.h"
#include "2s2h/ShipInit.hpp"
#include <ship/resource/CrossRMRegistry.h>
#include <cmath>
#include <memory>
#include <unordered_map>

static void DrawShimmer(Actor* actor, PlayState* play, bool seated) {
    if (!actor || !play || !play->state.gfxCtx || play->pauseCtx.state != PAUSE_STATE_OFF ||
        actor->xzDistToPlayer > 700.0f || std::fabs(actor->playerHeightRel) > 300.0f)
        return;
    Matrix_Push();
    Matrix_Translate(actor->world.pos.x, actor->world.pos.y, actor->world.pos.z, MTXMODE_NEW);
    const auto mesh = ForestGiParticles::Saria(play->gameplayFrames, seated, NeiGi_CameraBasis(play));
    NeiGi_DrawWorldParticles(play, mesh);
    Matrix_Pop();
}

namespace {
constexpr u16 kText = 0x8F28;
constexpr u16 kTeachText = 0x8F29;
constexpr u16 kTaughtText = 0x8F2A;
constexpr s16 kWaitingParams = 0x7F13;
constexpr s16 kSongGiftParams = 0x7E33;
constexpr s16 kSeatedRootOffset = 7003;

bool IsSeated(const Actor* actor) {
    return (((u16)actor->params >> 4) & 15) == 3;
}

bool IsSongGiftParam(s16 params) {
    return params == 0x7E13 || params == 0x7E23 || params == kSongGiftParams;
}

bool IsSongGift(const Actor* actor, const PlayState* play) {
    return IsSongGiftParam(actor->params) && ComboForestActors_IsMmGame() && play->sceneId == SCENE_00KEIKOKU &&
           gSaveContext.gameMode == GAMEMODE_NORMAL && ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) &&
           gSaveContext.save.cutsceneIndex == 0;
}

void GiveSariaSong(Actor* actor, PlayState*) {
    // Item events outlive their NPC. Bind to the originating file, never an actor pointer.
    auto& claimed = gSaveContext.save.shipSaveInfo.nei.comboSariaSongClaimed;
    if (!ComboForestActors_IsMmGame() || !ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) ||
        CUSTOM_ITEM_PARAM != gSaveContext.fileNum || claimed)
        return;
    Rando::GiveItem(RI_SONG_SARIA);
    claimed = 1;
    Rando::MiscBehavior::SariasSongHint();
    CustomMessage::Entry entry;
    entry.textboxType = TEXTBOX_TYPE_2;
    entry.icon = Rando::StaticData::GetIconForZMessage(RI_SONG_SARIA);
    Rando::ApplyItemReceiptText(RI_SONG_SARIA, entry);
    Rando::AppendReceiptSource(entry, "");
    if (CUSTOM_ITEM_FLAGS & CustomItem::GIVE_ITEM_CUTSCENE)
        CustomMessage::SetActiveCustomMessage(entry.msg, entry);
    else
        CustomMessage::StartTextbox(entry.msg, entry);
    SaveManager_SaveCurrentForCombo();
}

void DrawSariaSong(Actor* actor, PlayState*) {
    Matrix_Scale(30.0f, 30.0f, 30.0f, MTXMODE_APPLY);
    Rando::DrawItem(RI_SONG_SARIA, RC_UNKNOWN, actor);
}

void QueueSariaSong() {
    if (gSaveContext.save.shipSaveInfo.nei.comboSariaSongClaimed)
        return;
    auto* interactor = GameInteractor::Instance;
    auto pending = [](const GIEvent& event) {
        const auto* item = std::get_if<GIEventGiveItem>(&event);
        return item && item->giveItem == GiveSariaSong && item->param == gSaveContext.fileNum;
    };
    if (pending(interactor->currentEvent))
        return;
    for (const auto& event : interactor->events)
        if (pending(event))
            return;
    interactor->events.emplace_back(GIEventGiveItem{
        .showGetItemCutscene = true,
        .param = (s16)gSaveContext.fileNum,
        .giveItem = GiveSariaSong,
        .drawItem = DrawSariaSong,
    });
}
struct Bridge {
    ComboPlacedSariaLoadFunc load = nullptr;
    ComboPlacedSariaLoadV2Func loadV2 = nullptr;
    ComboPlacedSariaReleaseFunc release = nullptr;
    ComboPlacedSariaAltFunc alt = nullptr;
    bool Resolve() {
        if (gComboGameMode == COMBO_GAME_MM) {
            load = MM_PlacedSariaLoad;
            loadV2 = MM_PlacedSariaLoadV2;
            release = MM_PlacedSariaRelease;
            alt = MM_PlacedSariaAlt;
            return true;
        }
        if (!Combo_IsCombinedGame())
            return false;
        if (!load)
            load = reinterpret_cast<ComboPlacedSariaLoadFunc>(Combo_ResolveSym("soh", "OOT_PlacedSariaLoad"));
        if (!loadV2)
            loadV2 = reinterpret_cast<ComboPlacedSariaLoadV2Func>(Combo_ResolveSym("soh", "OOT_PlacedSariaLoadV2"));
        if (!release)
            release = reinterpret_cast<ComboPlacedSariaReleaseFunc>(Combo_ResolveSym("soh", "OOT_PlacedSariaRelease"));
        if (!alt)
            alt = reinterpret_cast<ComboPlacedSariaAltFunc>(Combo_ResolveSym("soh", "OOT_PlacedSariaAltEnabled"));
        return (loadV2 || load) && release && alt;
    }
    int Load(unsigned pose, ComboPlacedSariaResourcesV2* output) const {
        *output = {};
        output->drawScale = 1.f;
        return loadV2 ? loadV2(pose, output, sizeof(*output)) : load ? load(pose, &output->resources) : 0;
    }
} sBridge;
struct State {
    ComboPlacedSariaResources resources{};
    float drawScale = 1.f;
    SkelAnime animation{};
    Vec3s joints[17]{}, morph[17]{};
    ColliderCylinder collider{};
    s16 blinkTimer = 40;
    u8 eye = 0;
    bool talking = false;
    bool teaching = false;
    ~State() {
        if (resources.owner)
            sBridge.release(resources.owner);
    }
};
std::unordered_map<Actor*, std::unique_ptr<State>> sStates;
bool sSpawnAttempted = false;
bool sHorseSpawnAttempted = false;

u8 EyeIndex(const State& state, const Actor* actor) {
    return (((u16)actor->params >> 4) & 15) == 2 ? (state.talking ? 0 : 2) : state.eye;
}

void ResetSariaResources() {
    sStates.clear();
    sSpawnAttempted = false;
    sHorseSpawnAttempted = false;
}

bool Bind(State& state, Actor* actor, PlayState* play) {
    ComboPlacedSariaResourcesV2 next{};
    if (!sBridge.Load(((u16)actor->params >> 4) & 15, &next))
        return false;
    const float frame = state.animation.curFrame;
    auto previous = state.resources;
    state.resources = next.resources;
    state.drawScale = next.drawScale;
    // The normal headers share a C layout. MM owns all animation state and
    // fixed joint arrays; no OoT actor, player or animation context is invoked.
    SkelAnime_InitFlex(play, &state.animation, static_cast<FlexSkeletonHeader*>(next.resources.skeleton),
                       static_cast<AnimationHeader*>(next.resources.animation), state.joints, state.morph, 17);
    state.animation.curFrame = std::fmod(frame, state.animation.endFrame + 1.0f);
    // This cutscene loop carries an elevated stump position. Ground the small
    // placed NPC, preserving the native breathing motion and authored XYZ.
    if (IsSeated(actor))
        state.joints[0].y -= kSeatedRootOffset;
    if (previous.owner)
        sBridge.release(previous.owner);
    return true;
}

s32 Override(PlayState*, s32 limb, Gfx** display, Vec3f*, Vec3s*, Actor* actor) {
    auto& state = *sStates.at(actor);
    if (limb > 0 && limb < 17 && state.resources.poseLimbs[limb])
        *display = reinterpret_cast<Gfx*>(const_cast<char*>(state.resources.poseLimbs[limb]));
    const u8 eye = EyeIndex(state, actor);
    if (limb == 16 && state.resources.heads[eye])
        *display = reinterpret_cast<Gfx*>(const_cast<char*>(state.resources.heads[eye]));
    return false;
}

void SpawnForestSaria() {
    auto* play = gPlayState;
    const bool field = ComboForestActors_IsMmGame() && play && play->sceneId == SCENE_00KEIKOKU &&
                       gSaveContext.gameMode == GAMEMODE_NORMAL &&
                       ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) && gSaveContext.save.cutsceneIndex == 0;
    if (sSpawnAttempted || !play || gGameState != &play->state || !play->state.running ||
        (!field && !ComboForestActors_IsExploration(play)) || play->roomCtx.curRoom.num != 0 ||
        play->roomCtx.status != 0 || !play->roomCtx.curRoom.segment || play->transitionTrigger != TRANS_TRIGGER_OFF ||
        play->transitionMode != TRANS_MODE_OFF || play->pauseCtx.state != PAUSE_STATE_OFF ||
        play->csCtx.state != CS_STATE_IDLE || gSaveContext.cutsceneTrigger || play->soaringCsOrSoTCsPlaying ||
        Player_InCsMode(play))
        return;
    // An authored placement wins; room visits never duplicate a placed Saria.
    for (const auto& list : play->actorCtx.actorLists)
        for (Actor* actor = list.first; actor; actor = actor->next)
            if (actor->id == ACTOR_EN_VIEWER &&
                (field ? IsSongGiftParam(actor->params) : ComboSaria_IsParam(actor->params))) {
                sSpawnAttempted = true;
                return;
            }
    sSpawnAttempted = true;
    if (!sBridge.Resolve())
        return;
    const Vec3f anchor = ComboOpeningForest_ReturnAnchor(play);
    const auto side = ForestActorPlacements::SariaOffset(ComboOpeningForest_ReturnYaw(play));
    // cor's authored Termina Field song-giver export; MM's packed yaw is 272 degrees.
    Vec3f pos =
        field ? Vec3f{ 975.0f, -99.0f, 2101.0f } : Vec3f{ anchor.x + side.x, anchor.y + 120.0f, anchor.z + side.z };
    Vec3f floorProbe = pos;
    if (field)
        floorProbe.y += 120.0f;
    CollisionPoly* poly = nullptr;
    s32 bgId;
    const float floor = BgCheck_EntityRaycastFloor5(&play->colCtx, &poly, &bgId, nullptr, &floorProbe);
    if (!poly || !std::isfinite(floor) || floor <= BGCHECK_Y_MIN)
        return;
    Actor_Spawn(&play->actorCtx, play, ACTOR_EN_VIEWER, pos.x, field ? pos.y : floor, pos.z, 0,
                field ? DEG_TO_BINANG(272) : ComboOpeningForest_ReturnYaw(play), 0,
                field ? kSongGiftParams : kWaitingParams);
}

void SpawnForestEpona() {
    auto* play = gPlayState;
    if (sHorseSpawnAttempted || !play || gGameState != &play->state || !play->state.running ||
        !ComboForestActors_IsExploration(play) || play->roomCtx.curRoom.num != 0 || play->roomCtx.status != 0 ||
        !play->roomCtx.curRoom.segment || play->transitionTrigger != TRANS_TRIGGER_OFF ||
        play->transitionMode != TRANS_MODE_OFF || play->pauseCtx.state != PAUSE_STATE_OFF ||
        play->csCtx.state != CS_STATE_IDLE || gSaveContext.cutsceneTrigger || play->soaringCsOrSoTCsPlaying ||
        Player_InCsMode(play))
        return;
    const Vec3f anchor = ComboOpeningForest_ReturnAnchor(play);
    const s16 yaw = ComboOpeningForest_ReturnYaw(play);
    const float angle = yaw * (6.28318530718f / 65536.0f);
    const float sine = std::sin(angle), cosine = std::cos(angle);
    float sariaLateral = ForestActorPlacements::kSariaLateral;
    bool foundSaria = false;
    for (const auto& list : play->actorCtx.actorLists) {
        for (Actor* actor = list.first; actor; actor = actor->next) {
            if (actor->id == ACTOR_EN_HORSE && actor->params == COMBO_FOREST_EPONA_PARAMS) {
                sHorseSpawnAttempted = true;
                return;
            }
            if (!foundSaria && actor->id == ACTOR_EN_VIEWER && ComboSaria_IsParam(actor->params)) {
                sariaLateral = (actor->world.pos.x - anchor.x) * cosine - (actor->world.pos.z - anchor.z) * sine;
                foundSaria = true;
            }
        }
    }
    sHorseSpawnAttempted = true;
    // Use portal coordinates so angled/older portal packs keep the horse on
    // the opposite flank. Leave the doorway and its forward approach clear.
    const float lateral =
        sariaLateral <= 0.0f ? ForestActorPlacements::kHorseLateral : -ForestActorPlacements::kHorseLateral;
    constexpr float depth = ForestActorPlacements::kFlankDepth;
    Vec3f pos = { anchor.x + lateral * cosine + depth * sine, anchor.y + 120.0f,
                  anchor.z - lateral * sine + depth * cosine };
    CollisionPoly* poly = nullptr;
    s32 bgId = BGCHECK_SCENE;
    const float floor = BgCheck_EntityRaycastFloor5(&play->colCtx, &poly, &bgId, nullptr, &pos);
    if (!poly || !std::isfinite(floor) || floor <= BGCHECK_Y_MIN)
        return;
    Actor_Spawn(&play->actorCtx, play, ACTOR_EN_HORSE, pos.x, floor, pos.z, 0, yaw, 0, COMBO_FOREST_EPONA_PARAMS);
}

void RegisterSaria() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>(
        [](s16, s8) { sSpawnAttempted = sHorseSpawnAttempted = false; });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnRoomInit>(
        [](s16, s8) { sSpawnAttempted = sHorseSpawnAttempted = false; });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameStateUpdate>(SpawnForestSaria);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameStateUpdate>(SpawnForestEpona);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>(ResetSariaResources);
    // Window-close shutdown can bypass Play_Destroy. Release foreign objects
    // while both game modules and the owner ResourceManager still exist.
    Ship::CrossRMRegistry::RegisterTeardownListener(ResetSariaResources);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnOpenText>([](u16* textId, bool* fromTable) {
        if (*textId != kText && *textId != kTeachText && *textId != kTaughtText)
            return;
        CustomMessage::Entry entry;
        if (*textId == kTeachText)
            entry.msg = "Even here, I can hear\nthe forest calling...\nLet me teach you its song.\x19";
        else if (*textId == kTaughtText)
            entry.msg = "Wherever you wander,\nour song will carry the\nforest with you.\x19";
        else
            entry.msg = "These woods are a bridge\nbetween worlds...\nThe forest will guide you home.\x19";
        CustomMessage::LoadCustomMessageIntoFont(entry);
        *fromTable = false;
    });
}
static RegisterShipInitFunc init(RegisterSaria, {});
} // namespace

extern "C" int ComboSaria_IsParam(s16 params) {
    return ((u16)params & 0xFF0F) == 0x7F03 || IsSongGiftParam(params);
}

extern "C" void ComboSaria_Init(Actor* actor, PlayState* play) {
    if (!sBridge.Resolve()) {
        Actor_Kill(actor);
        return;
    }
    auto state = std::make_unique<State>();
    if (!Bind(*state, actor, play)) {
        Actor_Kill(actor);
        return;
    }
    actor->flags = ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_FRIENDLY | ACTOR_FLAG_UPDATE_CULLING_DISABLED;
    Actor_ChangeCategory(play, &play->actorCtx, actor, ACTORCAT_NPC);
    Actor_SetScale(actor, 0.01f);
    actor->world.rot.x = actor->world.rot.z = 0;
    actor->shape.rot = actor->world.rot;
    ActorShape_Init(&actor->shape, 0.0f, ActorShadow_DrawCircle, 20.0f);
    actor->attentionRangeType = ATTENTION_RANGE_6;
    actor->colChkInfo.mass = MASS_IMMOVABLE;
    ColliderCylinderInit cylinder = {
        { COL_MATERIAL_NONE, AT_NONE, AC_NONE, OC1_ON | OC1_TYPE_ALL, OC2_TYPE_1, COLSHAPE_CYLINDER },
        { ELEM_MATERIAL_UNK0, { 0, 0, 0 }, { 0, 0, 0 }, ATELEM_NONE, ACELEM_NONE, OCELEM_ON },
        { 8, 46, 0, { 0, 0, 0 } },
    };
    if (IsSeated(actor)) {
        cylinder.dim.radius = 12;
        cylinder.dim.height = 24;
    }
    Collider_InitCylinder(play, &state->collider);
    Collider_SetCylinder(play, &state->collider, actor, &cylinder);
    Vec3f floorProbe = actor->world.pos;
    floorProbe.y += 10.0f;
    s32 floorBg = BGCHECK_SCENE;
    actor->floorHeight = BgCheck_EntityRaycastFloor5(&play->colCtx, &actor->floorPoly, &floorBg, actor, &floorProbe);
    actor->floorBgId = floorBg;
    Actor_SetFocus(actor, IsSeated(actor) ? 20.0f : 40.0f);
    sStates.emplace(actor, std::move(state));
}

extern "C" void ComboSaria_Destroy(Actor* actor, PlayState* play) {
    auto found = sStates.find(actor);
    if (found == sStates.end())
        return;
    Collider_DestroyCylinder(play, &found->second->collider);
    sStates.erase(found);
}

extern "C" void ComboSaria_Update(Actor* actor, PlayState* play) {
    auto found = sStates.find(actor);
    if (found == sStates.end())
        return;
    auto& state = *found->second;
    if (state.resources.altEnabled != sBridge.alt() && !Bind(state, actor, play)) {
        Actor_Kill(actor);
        return;
    }
    SkelAnime_Update(&state.animation);
    if (IsSeated(actor))
        state.joints[0].y -= kSeatedRootOffset;
    if (state.blinkTimer > 0)
        --state.blinkTimer;
    if (state.blinkTimer == 0) {
        state.eye = (state.eye + 1) % 3;
        state.blinkTimer = state.eye ? 1 : Rand_S16Offset(30, 30);
    }
    Actor_SetFocus(actor, IsSeated(actor) ? 20.0f : 40.0f);
    Collider_UpdateCylinder(actor, &state.collider);
    CollisionCheck_SetOC(play, &play->colChkCtx, &state.collider.base);
    const bool gift = IsSongGift(actor, play);
    if (state.talking) {
        const u8 textState = Message_GetState(&play->msgCtx);
        if (textState == TEXT_STATE_EVENT && Message_ShouldAdvance(play)) {
            // Event-ended dialogue is closed by its actor, not the message engine.
            Message_CloseTextbox(play);
        } else if (textState == TEXT_STATE_CLOSING || textState == TEXT_STATE_NONE) {
            state.talking = false;
            if (state.teaching && gift)
                QueueSariaSong();
            state.teaching = false;
        }
        return;
    }
    actor->textId =
        gift ? (gSaveContext.save.shipSaveInfo.nei.comboSariaSongClaimed ? kTaughtText : kTeachText) : kText;
    if (Actor_TalkOfferAccepted(actor, &play->state)) {
        state.talking = true;
        state.teaching = gift && !gSaveContext.save.shipSaveInfo.nei.comboSariaSongClaimed;
    } else {
        Actor_OfferTalk(actor, play, 70.0f);
    }
}

extern "C" void ComboSaria_Draw(Actor* actor, PlayState* play) {
    auto found = sStates.find(actor);
    if (found == sStates.end())
        return;
    auto& state = *found->second;
    const u8 eye = EyeIndex(state, actor);
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    // Native OoT limbs branch through segment C, including the fallback when
    // no private model pack is installed. Match the native opaque NPC setup.
    static Gfx endDisplayList[] = { gsSPEndDisplayList() };
    gDPPipeSync(POLY_OPA_DISP++);
    gDPSetEnvColor(POLY_OPA_DISP++, 0, 0, 0, 255);
    gDPPipeSync(POLY_OPA_DISP++);
    gSPSegment(POLY_OPA_DISP++, 12, reinterpret_cast<uintptr_t>(endDisplayList));
    gSPSegment(POLY_OPA_DISP++, 8, reinterpret_cast<uintptr_t>(state.resources.eyes[eye]));
    gSPSegment(POLY_OPA_DISP++, 9, reinterpret_cast<uintptr_t>(state.resources.eyes[eye]));
    gSPSegment(POLY_OPA_DISP++, 10, reinterpret_cast<uintptr_t>(state.resources.mouth));
    Matrix_Push();
    const float drawScale = state.drawScale;
    if (state.resources.custom && std::isfinite(drawScale) && drawScale >= .5f && drawScale <= 2.f && drawScale != 1.f)
        Matrix_Scale(drawScale, drawScale, drawScale, MTXMODE_APPLY);
    SkelAnime_DrawFlexOpa(play, state.animation.skeleton, state.animation.jointTable, state.animation.dListCount,
                          Override, nullptr, actor);
    Matrix_Pop();
    CLOSE_DISPS(play->state.gfxCtx);
    DrawShimmer(actor, play, IsSeated(actor));
}
#endif
