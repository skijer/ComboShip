#ifdef COMBO_BUILD
#include "BlackMarket.h"
#include "BlackMarketItems.h"
#include "BlackMarketResources.h"
#include "BlackMarketNativeResources.h"
#include "BlackMarketEntry.h"
#include "BlackMarketExitDoor.h"
#include "BlackMarketDoorCollision.h"
extern "C" {
#include "overlays/actors/ovl_En_Door/z_en_door.h"
void EnDoor_Idle(EnDoor*, PlayState*);
}
#include "ComboExport.h"
#include "ComboOpeningForest.h"
#include "BlackMarketDoorNative.h"
#include "entrances/BlackMarketEntrance.h"
#include "entrances/SoloForest.h"
#include "BlackMarketLock.h"
#include "BlackMarketGraphicsArena.h"
#include "BlackMarketKeeper.h"
#include "BlackMarketGrannyResources.h"
#include "BlackMarketRoomMagicResources.h"
#include "BlackMarketMmShopScene.h"
#include "BlackMarketReceipt.h"
#include "2s2h/CustomItem/CustomItem.h"
#include "2s2h/Rando/ItemReceiptText.h"
#include "sword_element_message.h"
#include "BlackMarketText.h"
#include "2s2h/BenPort.h"
#include "mods/nei_save.h"
#include "2s2h/CustomMessage/CustomMessage.h"
#include "2s2h/Enhancements/Graphics/ComboPlaceTitleCard.h"
#include "gameplay/CrossGameTitleCard.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/Rando/Rando.h"
#include "2s2h/SaveManager/SaveManager.h"
#include "2s2h/ShipInit.hpp"
#include <array>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <libultraship/bridge/resourcebridge.h>
#include <libultraship/bridge/consolevariablebridge.h>
#include <ship/resource/CrossRMRegistry.h>
#include "2s2h/resource/type/Scene.h"
#include "2s2h/resource/type/scenecommand/SetCsCamera.h"
#include "2s2h/resource/type/scenecommand/SetActorCutsceneList.h"
#include "2s2h/resource/type/scenecommand/SetMesh.h"
int SaveManager_ReadSaveFile(const std::filesystem::path&, nlohmann::json&);
extern "C" {
#include "overlays/actors/ovl_En_Takaraya/z_en_takaraya.h"
#include "overlays/actors/ovl_En_Ossan/z_en_ossan.h"
#include "black_market_shop_adapter.h"
#include "z64shrink_window.h"
void EnOssan_Init(Actor*, PlayState*);
void EnOssan_Update(Actor*, PlayState*);
void EnOssan_Idle(EnOssan*, PlayState*);
void EnOssan_InitShop(EnOssan*, PlayState*);
void EnOssan_TalkToShopkeeper(EnOssan*, PlayState*);
void EnOssan_SelectItem(EnOssan*, PlayState*);
void EnOssan_CannotBuy(EnOssan*, PlayState*);
void EnOssan_BuyItemWithFanfare(EnOssan*, PlayState*);
void EnOssan_SetupItemPurchased(EnOssan*, PlayState*);
void EnOssan_DrawCursor(PlayState*, EnOssan*, f32, f32, f32, u8);
void EnOssan_DrawStickDirectionPrompts(PlayState*, EnOssan*);
void EnGirlA_DoNothing(EnGirlA*, PlayState*);
void EnGirlA_Update(Actor*, PlayState*);
void EnGirlA_Update2(EnGirlA*, PlayState*);
extern SceneEntranceTableEntry sSceneEntranceTable[ENTR_SCENE_MAX];
extern f32 sNESFontWidths[160];
u8 ResourceMgr_FileExists(const char* path);
u8 ResourceMgr_FileAltExists(const char* path);
bool ResourceMgr_IsAltAssetsEnabled(void);
void Player_Action_35(Player* player, PlayState* play);
void Player_Action_36(Player* player, PlayState* play);
}
#include "BlackMarketShelvesNative.h"
#include "BlackMarketMmShopLayout.h"
#include "BlackMarketCounterBarrier.h"
#include "BlackMarketInteriorEffects.h"
#include "BlackMarketKeeperAssets.h"

void CustomItem00_Destroy(Actor* actor, PlayState* play);

namespace {
// Display-list macros redeclare these helpers at block scope in this namespace.
extern "C" void FrameInterpolation_RecordOpenChild(const void* a, int b);
extern "C" void FrameInterpolation_RecordCloseChild(void);
using namespace ComboRando::BlackMarket;
Session sSession;
uint16_t sClears = 0;
void (*sReportClear)(int, int) = nullptr;
bool sEntering = false, sReturnPending = false, sDoorArmed = false, sTitlePending = false;
bool sTalking = false;
bool sCustomKeeper = false;
bool sResumeFromMarket = false;
int sTravelSlot = -1;
u32 sTravelSeed = 0;
int sSelected = 0;
float sDoorFloor = 0;
std::string sStatus;
constexpr u16 kBrowseText = 0x8F10, kResultText = 0x8F11, kWelcomeText = 0x8F12, kConfirmText = 0x8F13;
constexpr u16 kKeeperMenuText = 0x8F14, kKeeperTalkText = 0x8F15, kContinueText = 0x8F16,
              kKeeperHelpChoiceText = 0x8F17, kKeeperHelpText = 0x8F18;
constexpr char kScene[] = "scenes/nonmq/skiejer_market/skiejer_market";
constexpr char kRoom[] = "scenes/nonmq/skiejer_market/skiejer_market_room_0";
constexpr char kKeeper[] = "__OTR__custom/skiejer_market/keeper/gTreasureChestShopGalSkel";
alignas(2) constexpr char kCard[] = "__OTR__custom/oot_location_titles/v1/market";
alignas(2) constexpr char kExterior[] = "__OTR__custom/skiejer_market/entrance";
constexpr Vec3f kShelves[8] = {
    // Native +/-30-degree bank planes; row changes keep one bank camera.
    { 44.84456f, 52, 40.75f },  { 44.84456f, 76, 40.75f },  { 75.15544f, 52, 58.25f },  { 75.15544f, 76, 58.25f },
    { -44.84456f, 52, 40.75f }, { -44.84456f, 76, 40.75f }, { -75.15544f, 52, 58.25f }, { -75.15544f, 76, 58.25f }
};
BlackMarketShelvesNative::Fixture sShelfFixture{ BlackMarketMmShopLayout::Vertices(false),
                                                 BlackMarketMmShopLayout::Vertices(true),
                                                 BlackMarketMmShopLayout::CollisionCorners() };
BlackMarketCounterBarrier::Fixture sCounterBarrier;
Vec3f sSoloReturn = { 184, 0, -300 };
s16 sSoloReturnYaw = -16384;
u16 sSoloReturnEntrance = COMBO_SOLO_FOREST_MM_ENTRANCE;
RespawnData sPreservedReturn;
bool sReturnStaged = false;

bool SoloExterior(const PlayState* play) {
    return play && play->roomCtx.curRoom.num == 0 &&
           ComboSoloForest_IsMmVisit(play->sceneId, play->roomCtx.curRoom.num, gSaveContext.gameMode,
                                     gSaveContext.fileNum, gSaveContext.sceneLayer, gSaveContext.save.cutsceneIndex);
}
bool Exterior(const PlayState* play) {
    return (ComboOpeningForest_IsExploration(play) && play->roomCtx.curRoom.num == 0) || SoloExterior(play);
}
Vec3f Door() {
    return { COMBO_BLACK_MARKET_DOOR_X, 0, COMBO_BLACK_MARKET_DOOR_Z };
}
u16 ReturnEntrance() {
    return gComboGameMode == COMBO_GAME_MM ? sSoloReturnEntrance : ENTRANCE(LOST_WOODS, 0);
}
void StageSoloReturn() {
    if (gComboGameMode != COMBO_GAME_MM || sReturnStaged)
        return;
    // Local forest arrivals require RETURN's position fields. Preserve
    // that disposable point across this one load; TOP/Farore's Wind is untouched.
    auto& arrival = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    sPreservedReturn = arrival;
    sReturnStaged = true;
    arrival = {};
    arrival.entrance = sSoloReturnEntrance;
    arrival.roomIndex = 0;
    arrival.pos = sSoloReturn;
    arrival.yaw = sSoloReturnYaw;
    arrival.playerParams = PLAYER_PARAMS(0xFF, PLAYER_START_MODE_E);
    arrival.data = 1;
    gSaveContext.respawnFlag = 2;
}

bool PackAvailable() {
    if (!std::all_of(std::begin(BlackMarketMmShopScene::kPresentationResources),
                     std::end(BlackMarketMmShopScene::kPresentationResources),
                     [](const char* path) { return ResourceMgr_FileExists(path); }))
        return false;
    // The base scene needs its own host's geometry and native keeper. Visual
    // packs remain optional; complete legacy packs retain compatibility.
    return std::all_of(std::begin(kNativeBlackMarketMmResources), std::end(kNativeBlackMarketMmResources),
                       [](const char* path) { return ResourceMgr_FileExists(path); }) ||
           std::all_of(std::begin(kBlackMarketResources), std::end(kBlackMarketResources), [](const char* path) {
               return BlackMarketEntry::LegacyResourceAvailable(path, ResourceMgr_FileExists);
           });
}
bool DestinationAvailable() {
    return std::all_of(BlackMarketEntry::kMmDestination.begin(), BlackMarketEntry::kMmDestination.end(),
                       [](const char* path) { return ResourceMgr_FileExists(path); });
}
bool Interior() {
    return gComboGameMode != COMBO_GAME_OOT && gPlayState && gPlayState->sceneId == COMBO_BLACK_MARKET_SCENE;
}
bool Active() {
    return Interior() && sSession.Active(gSaveContext.fileNum);
}

BlackMarketDoorCollision::Fixture<false> sDoorCollision;
void SyncShopDoorCollision() {
    auto* play = gPlayState;
    if (!play || gGameState != &play->state || !play->state.running)
        return;
    // Custom native Open starts its transition immediately. Keep its explicit
    // release for the whole handoff even if the actor is still idle this frame.
    if (play->transitionTrigger != TRANS_TRIGGER_OFF || play->transitionMode != TRANS_MODE_OFF) {
        sDoorCollision.Release(play);
        return;
    }
    Actor* closed = nullptr;
    if (Interior())
        closed = const_cast<Actor*>(BlackMarketExitDoor::Live<false>(play));
    else if (Exterior(play)) {
        BlackMarketDoorNative::Pose pose;
        if (BlackMarketDoorNative::Find(play, pose))
            for (auto* actor = play->actorCtx.actorLists[ACTORCAT_DOOR].first; actor; actor = actor->next)
                if (BlackMarketDoorNative::Matches(actor, pose) && actor->update && actor->draw) {
                    closed = actor;
                    break;
                }
    }
    if (closed) {
        const auto* native = reinterpret_cast<const EnDoor*>(closed);
        if (native->actionFunc != EnDoor_Idle || native->knobDoor.requestOpen || native->openTimer != 0)
            closed = nullptr;
    }
    sDoorCollision.Ensure(play, closed);
}
BlackMarketGranny::Resources sGrannyResources;
bool sGrannyPending = false, sGrannyActive = false;
RoomShape sGrannyShape{};
RoomShape* sGrannyOriginal = nullptr;
RoomShapeDListsEntry sGrannyEntry{};
std::vector<EnvLightSettings> sGrannyLights;
void GrannySceneBegin() {
    sGrannyActive = false;
    sGrannyOriginal = nullptr;
    sGrannyPending = Interior() && CVarGetInteger(BlackMarketGranny::kCvar, 0) != 0;
}
void ApplyGrannyInterior() {
    auto* play = gPlayState;
    if (!sGrannyPending || !Interior() || play->roomCtx.curRoom.num != 0 || !play->roomCtx.curRoom.roomShape)
        return;
    sGrannyPending = false;
    if (!sGrannyResources.Prepare("mm"))
        return;
    std::array<Actor*, 2> flames{};
    size_t index = 0;
    for (const auto& actor : sGrannyResources.Decorations()) {
        auto* spawned = Actor_Spawn(&play->actorCtx, play, ACTOR_EN_LIGHT, actor.x, actor.y, actor.z, actor.rx,
                                    actor.ry, actor.rz, BlackMarketGranny::MmFlameParams(actor.params));
        if (!spawned) {
            for (auto* previous : flames)
                if (previous)
                    Actor_Kill(previous);
            CVarSetString(BlackMarketGranny::kErrorCvar, "Unable to create Granny's decorative flames.");
            return;
        }
        flames[index++] = spawned;
    }
    static_assert(sizeof(EnvLightSettings) == 22);
    sGrannyLights.resize(sGrannyResources.Lights().size());
    std::memcpy(sGrannyLights.data(), sGrannyResources.Lights().data(),
                sGrannyLights.size() * sizeof(EnvLightSettings));
    play->envCtx.numLightSettings = sGrannyLights.size();
    play->envCtx.lightSettingsList = sGrannyLights.data();
    play->envCtx.lightMode = LIGHT_MODE_SETTINGS;
    play->envCtx.lightSetting = 0;
    std::memcpy(&play->envCtx.lightSettings, sGrannyLights.data(), sizeof(EnvLightSettings));
    sGrannyShape.normal = { { ROOM_SHAPE_TYPE_NORMAL }, 1, &sGrannyEntry, &sGrannyEntry + 1 };
    sGrannyOriginal = play->roomCtx.curRoom.roomShape;
    play->roomCtx.curRoom.roomShape = &sGrannyShape;
    sGrannyActive = true;
}
void DrawGrannyRoom() {
    ApplyGrannyInterior();
    if (!sGrannyActive || !Interior())
        return;
    auto* play = gPlayState;
    auto* frame = static_cast<Gfx*>(GRAPH_ALLOC(play->state.gfxCtx, 12 * sizeof(Gfx)));
    const auto tick = play->gameplayFrames;
    BlackMarketGranny::Wrap(frame, frame + 5, sGrannyResources.Opaque(), sGrannyResources.Translucent(),
                            Gfx_TexScrollEx(play->state.gfxCtx, 0, (tick * 3) % 128, 32, 32, 0, 3),
                            Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, 0, 1023 - (tick * 3) % 1024, 16, 256, 1, 0,
                                               1023 - (tick * 6) % 1024, 16, 256, 0, -3, 0, -6),
                            sGrannyResources.Route());
    sGrannyEntry = { frame, frame + 5 };
}
void TraceRoomMagic(const char* result, RoomShape* source = nullptr, bool prepared = false);
BlackMarketRoomMagic::Resources sRoomMagicResources;
bool sRoomMagicTraceValid = false;
RoomShape sRoomMagicShape{};
RoomShapeDListsEntry sRoomMagicEntry{};
RoomShape* sRoomMagicOriginal = nullptr;
PlayState* sRoomMagicPlay = nullptr;
void RestoreRoomMagic() {
    // Restore only the live room we attached. A scene load may have reused the
    // PlayState address or installed another shape since the previous draw.
    auto* play = gPlayState;
    if (play && play == sRoomMagicPlay && play->roomCtx.curRoom.roomShape == &sRoomMagicShape)
        play->roomCtx.curRoom.roomShape = sRoomMagicOriginal;
    sRoomMagicPlay = nullptr;
    sRoomMagicOriginal = nullptr;
    sRoomMagicEntry = {};
}
void ClearRoomMagic() {
    sRoomMagicTraceValid = false;
    RestoreRoomMagic();
    sRoomMagicResources.Clear();
}
void BeginRoomMagic() {
    RestoreRoomMagic();
    auto* play = gPlayState;
    if (!Interior())
        return;
    if (sGrannyActive) {
        TraceRoomMagic("granny");
        return;
    }
    if (play->roomCtx.curRoom.num != 0) {
        TraceRoomMagic("room");
        return;
    }
    if (!CVarGetInteger(BlackMarketRoomMagic::kCvar, 0)) {
        TraceRoomMagic("disabled");
        return;
    }
    auto* shape = play->roomCtx.curRoom.roomShape;
    if (!shape) {
        TraceRoomMagic("shape-null");
        return;
    }
    if (shape->base.type != 0 || shape->normal.numEntries != 1 || !shape->normal.entries) {
        TraceRoomMagic("shape-layout", shape);
        return;
    }
    if (!BlackMarketRoomMagic::MatchesSource(shape->normal.entries->opa, false)) {
        TraceRoomMagic("source-mismatch", shape);
        return;
    }
    if (!sRoomMagicResources.Prepare("mm", shape->normal.entries->opa)) {
        TraceRoomMagic("resource-unavailable", shape, true);
        return;
    }
    sRoomMagicResources.SetPalette(BlackMarketRoomMagic::ReadPalette(false));
    sRoomMagicEntry = *shape->normal.entries;
    sRoomMagicEntry.opa = sRoomMagicResources.Opaque();
    sRoomMagicShape = *shape;
    sRoomMagicShape.normal.entries = &sRoomMagicEntry;
    sRoomMagicShape.normal.entriesEnd = &sRoomMagicEntry + 1;
    sRoomMagicOriginal = shape;
    sRoomMagicPlay = play;
    play->roomCtx.curRoom.roomShape = &sRoomMagicShape;
    TraceRoomMagic("attached", shape, true);
}
void TraceRoomMagic(const char* result, RoomShape* source, bool prepared) {
    // Compare canonical source identities, never the per-draw replacement/Granny
    // wrapper lists. This emits on entry or changed decisions, not every frame.
    struct Snapshot {
        const char* result;
        const char* preparation;
        const void* play;
        const void* shape;
        const void* opaque;
        const void* owner;
        const void* active;
        const void* root;
        int scene, room, enabled, granny, grannyActive, adult, forceAdult, altCvar, type, entries;
        bool alt;
        bool operator==(const Snapshot&) const = default;
    };
    static Snapshot previous{};
    auto* play = gPlayState;
    if (!Interior())
        return;
    if (!source)
        source = play->roomCtx.curRoom.roomShape;
    const auto owner = Ship::CrossRMRegistry::Get("mm");
    auto* context = Ship::Context::GetRawInstance();
    const auto active = context ? context->GetResourceManager() : nullptr;
    const auto& preparation = sRoomMagicResources.LastPreparation();
    const bool normal = source && source->base.type == ROOM_SHAPE_TYPE_NORMAL;
    const bool single = normal && source->normal.numEntries == 1 && source->normal.entries;
    const Snapshot current{ result,
                            prepared ? preparation.reason : "skipped",
                            play,
                            source,
                            single && source != &sGrannyShape ? source->normal.entries->opa : nullptr,
                            owner.get(),
                            active.get(),
                            prepared ? preparation.root : nullptr,
                            play->sceneId,
                            play->roomCtx.curRoom.num,
                            CVarGetInteger(BlackMarketRoomMagic::kCvar, 0),
                            CVarGetInteger(BlackMarketGranny::kCvar, 0),
                            sGrannyActive,
                            Nei_Save()->timeGateAdultMode,
                            CVarGetInteger("gAdultLink.ForceOn", 0),
                            CVarGetInteger("gEnhancements.Mods.AlternateAssets", 0),
                            source ? source->base.type : -1,
                            normal ? source->normal.numEntries : -1,
                            owner && owner->IsAltAssetsEnabled() };
    if (sRoomMagicTraceValid && current == previous)
        return;
    previous = current;
    sRoomMagicTraceValid = true;
    SPDLOG_INFO("BlackMarketMaterials: result={} prepare={} scene={} room={} enabled={} granny={} grannyActive={} "
                "adult={} forceAdult={} altCVar={} ownerAlt={} mmRM={} activeRM={} sourceShape={} type={} entries={} "
                "sourceOpaque={} root={}",
                current.result, current.preparation, current.scene, current.room, current.enabled, current.granny,
                current.grannyActive, current.adult, current.forceAdult, current.altCvar, current.alt, current.owner,
                current.active, current.shape, current.type, current.entries, current.opaque, current.root);
}
void RememberTravel() {
    sTravelSlot = gSaveContext.fileNum;
    sTravelSeed = gSaveContext.save.shipSaveInfo.rando.finalSeed;
}
bool MatchesTravel() {
    return sTravelSlot == gSaveContext.fileNum && sTravelSeed == gSaveContext.save.shipSaveInfo.rando.finalSeed;
}
bool Safe(const PlayState* play) {
    return play && gGameState == &play->state && play->state.running && gSaveContext.gameMode == GAMEMODE_NORMAL &&
           ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) && play->transitionTrigger == TRANS_TRIGGER_OFF &&
           play->transitionMode == TRANS_MODE_OFF && play->pauseCtx.state == PAUSE_STATE_OFF &&
           play->gameOverCtx.state == GAMEOVER_INACTIVE && play->csCtx.state == CS_STATE_IDLE &&
           !gSaveContext.cutsceneTrigger && !play->soaringCsOrSoTCsPlaying;
}
void Transition(PlayState* play, u16 entrance) {
    Combo_MM_RequestCrossGameTitleCard();
    play->nextEntrance = entrance;
    play->transitionType = TRANS_TYPE_FADE_BLACK;
    gSaveContext.nextTransitionType = TRANS_TYPE_FADE_BLACK;
    play->transitionTrigger = TRANS_TRIGGER_START;
}
const Item* Offer(int slot) {
    return sSession.Active(gSaveContext.fileNum) && slot >= 0 && slot < 8
               ? Find(sSession.market["stock"][slot]["key"].get<std::string>())
               : nullptr;
}
PurchaseResult Status(int slot) {
    const Item* item = Offer(slot);
    if (!item)
        return PurchaseResult::Unavailable;
    return CanPurchase(sSession.market, sSession.state, slot,
                       gSaveContext.save.saveInfo.playerData.rupees + std::min<int>(0, gSaveContext.rupeeAccumulator),
                       ClearCount(sClears), ComboBlackMarketItems::Owned(*item),
                       ComboBlackMarketItems::Available(*item));
}
BlackMarketReceipt::State sReceipt;
nlohmann::json sReceiptMarket;
Actor* sReceiptActor = nullptr;
int sReceiptActorTicket = 0;
bool ReceiptContext() {
    return Active() && gPlayState && gGameState == &gPlayState->state && gPlayState->state.running &&
           sReceipt.Matches(gSaveContext.fileNum, gSaveContext.save.shipSaveInfo.rando.finalSeed, sReceipt.ticket) &&
           sReceiptMarket == sSession.market && gPlayState->transitionTrigger == TRANS_TRIGGER_OFF &&
           gPlayState->transitionMode == TRANS_MODE_OFF && gPlayState->gameOverCtx.state == GAMEOVER_INACTIVE;
}
void AbortMarketReceipt(Actor* actor) {
    CustomMessage::Entry entry;
    entry.textboxType = 2;
    entry.msg = "That offer is no longer available.";
    if (CUSTOM_ITEM_FLAGS & CustomItem::GIVE_ITEM_CUTSCENE)
        CustomMessage::SetActiveCustomMessage(entry.msg, entry);
    else
        CustomMessage::StartTextbox(entry.msg, entry);
    // The queue's callback still owns currentEvent until this call returns.
    sReceipt.Cancel();
    sReceiptMarket = {};
}
void GiveMarketReceipt(Actor* actor, PlayState* play) {
    if (actor != sReceiptActor || !ReceiptContext() ||
        !sReceipt.Matches(gSaveContext.fileNum, gSaveContext.save.shipSaveInfo.rando.finalSeed, CUSTOM_ITEM_PARAM) ||
        sReceipt.delivered)
        return;
    const auto* item = Offer(sReceipt.slot);
    if (!item || Status(sReceipt.slot) != PurchaseResult::Ready) {
        AbortMarketReceipt(actor);
        return;
    }
    const auto id = ComboBlackMarketItems::Id(*item);
    CustomMessage::Entry entry;
    entry.textboxType = 2;
    entry.icon = Rando::StaticData::GetIconForZMessage(id);
    entry.msg = "You got " + std::string(item->label) + "!";
    // Capture the actual receipt and its native follow-ups before granting.
    Rando::ApplyItemReceiptText(id, entry);
    Rando::AppendReceiptSource(entry, "");
    entry.swordElementTutorial = SwordElement_MessageElementForPickup(Rando::StaticData::Items.at(id).itemId);
    if (!ComboBlackMarketItems::Grant(*item)) {
        AbortMarketReceipt(actor);
        return;
    }
    if (!sReceipt.Acknowledge(gSaveContext.fileNum, gSaveContext.save.shipSaveInfo.rando.finalSeed, CUSTOM_ITEM_PARAM))
        return;
    // Reward, balance and sold bit enter one synchronous native snapshot.
    gSaveContext.save.saveInfo.playerData.rupees -= sReceipt.price;
    CommitPurchase(sSession.state, sReceipt.slot);
    SaveManager_SaveCurrentForCombo(true);
    if (CUSTOM_ITEM_FLAGS & CustomItem::GIVE_ITEM_CUTSCENE) {
        CustomMessage::SetActiveCustomMessage(entry.msg, entry);
    } else {
        CustomMessage::StartTextbox(entry.msg, entry);
        SwordElement_MessageAppend(play, entry.swordElementTutorial);
    }
}
void DrawMarketReceipt(Actor* actor, PlayState* play) {
    if (!ReceiptContext())
        return;
    const auto* item = Offer(sReceipt.slot);
    if (!item)
        return;
    Matrix_Scale(30.0f, 30.0f, 30.0f, MTXMODE_APPLY);
    Rando::DrawResolvedItem(ComboBlackMarketItems::Id(*item), RC_UNKNOWN, actor);
}
void ReleaseReceiptEvent(int ticket) {
    auto& current = GameInteractor::Instance->currentEvent;
    const auto* give = std::get_if<GIEventGiveItem>(&current);
    if (ticket && give && give->giveItem == GiveMarketReceipt && give->param == ticket)
        current = GIEventNone{};
}
void MarketReceiptDestroyed(Actor* actor, PlayState* play) {
    if (actor == sReceiptActor) {
        sReceiptActor = nullptr;
        sReceiptActorTicket = 0;
    }
    ReleaseReceiptEvent(CUSTOM_ITEM_PARAM);
    // Retain native item00 collider destruction; there is no scheduler closure.
    CustomItem00_Destroy(actor, play);
}
void CancelReceipt(bool destroying = false) {
    auto* actor = sReceiptActor;
    ReleaseReceiptEvent(sReceiptActorTicket);
    sReceiptActorTicket = 0;
    sReceiptActor = nullptr; // retire ownership before deferred actor destruction
    sReceipt.Cancel();
    sReceiptMarket = {};
    if (!destroying && actor)
        Actor_Kill(actor);
}
bool ReceiptPlacementIdle() {
    const auto& current = GameInteractor::Instance->currentEvent;
    const auto* receipt = std::get_if<GIEventGiveItem>(&current);
    const bool ownReceipt = receipt && receipt->giveItem == GiveMarketReceipt && receipt->param == sReceipt.ticket;
    return GameInteractor::Instance->events.empty() && (std::holds_alternative<GIEventNone>(current) || ownReceipt);
}
void OfferMarketReceipt(PlayState* play, Player* player) {
    // Yield to the ordinary event queue without ever changing its ownership.
    if (sReceiptActor || !GameInteractor::Instance->events.empty() ||
        !std::holds_alternative<GIEventNone>(GameInteractor::Instance->currentEvent))
        return;
    s16 flags = CustomItem::HIDE_TILL_OVERHEAD | CustomItem::KEEP_ON_PLAYER;
    if ((player->stateFlags1 &
         (PLAYER_STATE1_CHARGING_SPIN_ATTACK | PLAYER_STATE1_2000 | PLAYER_STATE1_4000 | PLAYER_STATE1_40000 |
          PLAYER_STATE1_80000 | PLAYER_STATE1_100000 | PLAYER_STATE1_200000 | PLAYER_STATE1_8000000)) ||
        Player_GetExplosiveHeld(player) > PLAYER_EXPLOSIVE_NONE)
        flags |= CustomItem::GIVE_OVERHEAD;
    else
        flags |= CustomItem::GIVE_ITEM_CUTSCENE;
    // Same native custom-item path as GIEventGiveItem, with an actor-owned
    // action/destructor instead of closures that read mutable currentEvent.
    auto* actor =
        Actor_Spawn(&play->actorCtx, play, ACTOR_EN_ITEM00, player->actor.world.pos.x, player->actor.world.pos.y,
                    player->actor.world.pos.z, flags, 0, static_cast<s16>(sReceipt.ticket), ITEM00_NOTHING);
    if (!actor)
        return;
    auto* item = reinterpret_cast<EnItem00*>(actor);
    item->actionFunc = reinterpret_cast<EnItem00ActionFunc>(GiveMarketReceipt);
    actor->draw = DrawMarketReceipt;
    actor->destroy = MarketReceiptDestroyed;
    sReceiptActor = actor;
    sReceiptActorTicket = sReceipt.ticket;
    // Reserve scheduler ownership while this independent native actor is live.
    // No action or destructor dispatches through this mutable marker.
    GameInteractor::Instance->currentEvent = GIEventGiveItem{
        .showGetItemCutscene = true,
        .param = static_cast<s16>(sReceipt.ticket),
        .giveItem = GiveMarketReceipt,
        .drawItem = DrawMarketReceipt,
    };
}
bool Purchase() {
    switch (Status(sSelected)) {
        case PurchaseResult::Ready: {
            const Item* item = Offer(sSelected);
            const int price = sSession.market["stock"][sSelected]["price"];
            if (!item || !sReceipt.Begin(sSelected, gSaveContext.fileNum,
                                         gSaveContext.save.shipSaveInfo.rando.finalSeed, price)) {
                sStatus = "That offer cannot be delivered yet.";
                break;
            }
            sReceiptMarket = sSession.market;
            return true;
        }
        case PurchaseResult::SoldOut:
            sStatus = "That one has already been sold.";
            break;
        case PurchaseResult::Owned:
            sStatus = "You already own that power.";
            break;
        case PurchaseResult::Locked: {
            const int clears = sSession.market["stock"][sSelected]["clears"];
            sStatus = "The seal needs " + std::to_string(clears) + " unique dungeon clear" + (clears == 1 ? "." : "s.");
            break;
        }
        case PurchaseResult::NeedRupees:
            sStatus = "You need more rupees.";
            break;
        case PurchaseResult::Unavailable:
            sStatus = Offer(sSelected) && Offer(sSelected)->renewable ? "Bring an empty bottle for this refill."
                                                                      : "You need its prerequisite first.";
            break;
    }
    return false;
}

#include "BlackMarketMmNativeShop.h"
void MonitorReceipt() {
    if (!sReceipt.pending)
        return;
    if (!ReceiptContext()) {
        // Once the play arena is gone only drop ownership; its destroy hooks
        // already retired actors. Never dereference the old player/actor.
        CancelReceipt(!gPlayState || gGameState != &gPlayState->state);
        return;
    }
    auto* play = gPlayState;
    auto* player = GET_PLAYER(play);
    if (!player || play->msgCtx.msgMode != 0 || (player->stateFlags1 & PLAYER_STATE1_400) ||
        Player_InBlockingCsMode(play, player) || !Safe(play) || Play_GetActiveCamId(play) != CAM_ID_MAIN)
        return;
    if (!sReceipt.delivered) {
        if (Status(sReceipt.slot) != PurchaseResult::Ready) {
            CancelReceipt();
            return;
        }
        OfferMarketReceipt(play, player);
        return;
    }
    // Delivery acknowledgement and full receipt-chain completion are separate
    // native adapter predicates. EnOssan resumes its keeper/continue action.
}

bool CustomKeeperAvailable() {
    return BlackMarketKeeperAssets::CustomAvailable(kBlackMarketResources, [](const char* path) {
        return ResourceMgr_FileExists(path) || (ResourceMgr_IsAltAssetsEnabled() && ResourceMgr_FileAltExists(path));
    });
}

void KeeperUpdate(Actor* actor, PlayState* play) {
    sCounterBarrier.Ensure(play);
    MonitorBrowsing();
    auto* shop = reinterpret_cast<EnOssan*>(actor);
    if (sNativeShop != shop || sNativeShopPlay != play)
        return;
    if (!Active() && shop->actionFunc != EnOssan_InitShop) {
        EndBrowsing(play);
        actor->textId = BlackMarketEntry::kUnavailableText;
        if (Actor_TalkOfferAccepted(actor, &play->state))
            Message_ContinueTextbox(play, BlackMarketEntry::kUnavailableText);
        else if (Safe(play))
            Actor_OfferTalk(actor, play, 100.0f);
        NativeKeeperBlink(shop);
        SkelAnime_Update(&shop->skelAnime);
    } else {
        EnOssan_Update(actor, play);
    }
    Actor_SetFocus(actor, 55.0f);
    sSelected = shop->cursorIndex;
    sTalking = NativeShopDialog(shop);
    UpdateNativeInspection(shop, play);
}
s32 KeeperLimb(PlayState*, s32 limb, Gfx** display, Vec3f*, Vec3s*, Actor* actor) {
    if (sCustomKeeper && limb == TREASURE_CHEST_SHOP_GAL_LIMB_HEAD) {
        static const char* heads[] = { "__OTR__custom/skiejer_market/keeper/ShopGalMMDBlinkHead0DL",
                                       "__OTR__custom/skiejer_market/keeper/ShopGalMMDBlinkHead1DL",
                                       "__OTR__custom/skiejer_market/keeper/ShopGalMMDBlinkHead2DL",
                                       "__OTR__custom/skiejer_market/keeper/ShopGalMMDBlinkHead1DL" };
        *display = reinterpret_cast<Gfx*>(const_cast<char*>(heads[reinterpret_cast<EnOssan*>(actor)->eyeTexIndex]));
    }
    return false;
}
void KeeperDraw(Actor* actor, PlayState* play) {
    auto* gal = reinterpret_cast<EnOssan*>(actor);
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    if (!sCustomKeeper)
        gSPSegment(POLY_OPA_DISP++, 0x08,
                   reinterpret_cast<uintptr_t>(BlackMarketKeeperAssets::kNativeEyes[gal->eyeTexIndex & 3]));
    SkelAnime_DrawFlexOpa(play, gal->skelAnime.skeleton, gal->skelAnime.jointTable, gal->skelAnime.dListCount,
                          KeeperLimb, nullptr, actor);
    CLOSE_DISPS(play->state.gfxCtx);
    if (sTalking && NativeShopOwnCamera(play) && Play_GetActiveCamId(play) == sBrowseCamera &&
        BlackMarketGraphicsArena::HasRoom(play->state.gfxCtx, 0, 64, 0, 64)) {
        EnOssan_DrawCursor(play, gal, gal->cursorPos.x, gal->cursorPos.y, gal->cursorPos.z, gal->drawCursor);
        EnOssan_DrawStickDirectionPrompts(play, gal);
    }
}
void InitKeeper(Actor* actor) {
    if (!Interior() || !PackAvailable()) {
        Actor_Kill(actor);
        return;
    }
    auto* gal = reinterpret_cast<EnOssan*>(actor);
    sCustomKeeper = CustomKeeperAvailable();
    ActorShape_Init(&actor->shape, 0, nullptr, 0);
    Actor_SetScale(actor, BlackMarketKeeper::kScale);
    SkelAnime_InitFlex(
        gPlayState, &gal->skelAnime,
        (FlexSkeletonHeader*)(sCustomKeeper ? kKeeper : BlackMarketKeeperAssets::kNativeSkeleton),
        (AnimationHeader*)(sCustomKeeper ? BlackMarketKeeper::kAnimation : BlackMarketKeeperAssets::kNativeAnimation),
        sKeeperJoints.data(), sKeeperMorphs.data(), TREASURE_CHEST_SHOP_GAL_LIMB_MAX);
    actor->update = KeeperUpdate;
    actor->draw = KeeperDraw;
    // Native collider was never initialized; the private destructor owns only
    // this controller reference, not the ordinary shop or minigame state.
    actor->flags |= ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_FRIENDLY | ACTOR_FLAG_UPDATE_CULLING_DISABLED;
    actor->shape.yOffset = 0;
    gal->eyeTexIndex = 0;
    gal->blinkTimer = 90;
    gal->blinkFunc = NativeKeeperBlink;
    sTalking = false;
}

std::shared_ptr<SOH::Scene> sNativeShopSceneResource;
bool NativeShopSceneReady(PlayState* play) {
    // GetCutscenes follows linked IDs without bounds checks. Validate the
    // actually selected native/Alt scene and its loaded pointers before the
    // first native actor action, rather than replacing the manager at SceneInit.
    Ship::OwnRMScope scope("mm");
    auto scene = std::dynamic_pointer_cast<SOH::Scene>(ResourceLoad(kScene));
    if (!scene)
        return false;
    std::shared_ptr<SOH::SetCsCamera> cameras;
    std::shared_ptr<SOH::SetActorCutsceneList> cutscenes;
    for (const auto& command : scene->commands) {
        if (!command)
            return false;
        if (command->cmdId == SOH::SceneCommandID::SetCsCamera)
            cameras = std::dynamic_pointer_cast<SOH::SetCsCamera>(command);
        if (command->cmdId == SOH::SceneCommandID::SetActorCutsceneList)
            cutscenes = std::dynamic_pointer_cast<SOH::SetActorCutsceneList>(command);
    }
    if (!cameras || !cutscenes || cameras->csCamera.size() != 4 || cutscenes->entries.size() != 4 ||
        static_cast<void*>(play->actorCsCamList) != cameras->GetPointer() ||
        static_cast<void*>(CutsceneManager_GetCutsceneEntry(0)) != cutscenes->GetPointer())
        return false;
    static_assert(sizeof(Vec3s) == sizeof(SOH::z64Vec3s));
    static_assert(sizeof(ActorCutscene) == sizeof(SOH::CutsceneEntry));
    for (int i = 0; i < 4; ++i) {
        const auto& cam = cameras->csCamera[i];
        const auto& cs = cutscenes->entries[i];
        if (cam.setting != CAM_SET_BLACK_MARKET_OOT || cam.count != 3 || !cam.actorCsCamFuncData ||
            std::memcmp(cam.actorCsCamFuncData, BlackMarketMmShopScene::kCameraRecords[i], 9 * sizeof(s16)) ||
            cs.priority != BlackMarketMmShopScene::kFirstPriority + i || cs.length != -1 ||
            cs.csCamId != BlackMarketMmShopScene::kCutsceneCameras[i] || cs.scriptIndex != CS_SCRIPT_ID_NONE ||
            cs.additionalCsId != (i == 3 ? CS_ID_NONE : i + 1) || cs.endSfx != CS_END_SFX_NONE ||
            cs.customValue != 255 || cs.hudVisibility != BlackMarketMmShopScene::kHudVisibility ||
            cs.endCam != CS_END_CAM_0 || cs.letterboxSize != BlackMarketMmShopScene::kLetterboxSize)
            return false;
    }
    // A legacy/Alt room can retain the same scene cameras while overriding
    // the mesh with the old basket positions. Validate the actual selected
    // room and loaded mesh pointer before native actions can begin.
    auto room = std::dynamic_pointer_cast<SOH::Scene>(ResourceLoad(kRoom));
    std::shared_ptr<SOH::SetMesh> mesh;
    if (!room)
        return false;
    for (const auto& command : room->commands)
        if (command && command->cmdId == SOH::SceneCommandID::SetMesh)
            mesh = std::dynamic_pointer_cast<SOH::SetMesh>(command);
    if (!mesh || mesh->meshHeader.base.type != 0 || mesh->opaPaths.size() != 1 ||
        mesh->opaPaths[0] != std::string("__OTR__") + BlackMarketMmShopScene::kPresentationResources[0] ||
        !mesh->xluPaths.empty())
        return false;
    const auto* loadedShape = play->roomCtx.curRoom.roomShape;
    if (static_cast<const void*>(loadedShape) != mesh->GetPointer() &&
        !(sGrannyActive && loadedShape == &sGrannyShape && static_cast<void*>(sGrannyOriginal) == mesh->GetPointer()))
        return false;
    sNativeShopSceneResource = std::move(scene);
    return true;
}
void NativeKeeperDestroyed(Actor* actor, PlayState* play) {
    if (sNativeShop && &sNativeShop->actor == actor) {
        const bool live = play && play == gPlayState && gGameState == &play->state && play->state.running;
        if (live) {
            const bool owns = CutsceneManager_IsCurrentActor(actor);
            const s16 activeCamera = Play_GetActiveCamId(play);
            const bool noActiveCamera = activeCamera < 0 || activeCamera >= NUM_CAMS || !play->cameraPtrs[activeCamera];
            const bool ownsPresentation =
                owns && ((NativeShopOwnCamera(play) && activeCamera == sBrowseCamera) || noActiveCamera);
            auto* returnCamera = play->cameraPtrs[CAM_ID_MAIN];
            const s16 returnUid = returnCamera ? returnCamera->uid : -1;
            if ((owns || Play_GetActiveCamId(play) == CAM_ID_MAIN) && NativeShopMessage(play))
                Message_CloseTextbox(play);
            if (auto* player = GET_PLAYER(play); player && sTalking)
                player->stateFlags2 &= ~PLAYER_STATE2_20000000;
            const bool ended = CutsceneManager_EndForActor(play, actor);
            // Synchronous retirement bypasses Update's completed-cutscene UI
            // step. Restore it only if our unchanged return view survived.
            if (ended && ownsPresentation && CutsceneManager_GetCurrentCsId() == CS_ID_NONE && returnCamera &&
                Play_GetActiveCamId(play) == CAM_ID_MAIN && play->cameraPtrs[CAM_ID_MAIN] == returnCamera &&
                returnCamera->uid == returnUid) {
                ShrinkWindow_Letterbox_SetSizeTarget(0);
                Interface_SetHudVisibility(HUD_VISIBILITY_ALL);
            }
        }
        CancelReceipt(!live);
        EndBrowsing(play, true);
        sNativeShopSceneResource.reset();
    }
}
void InitNativeKeeper(Actor* actor, PlayState* play) {
    actor->destroy = NativeKeeperDestroyed;
    if (!Interior() || actor->params != BlackMarketMmShopScene::kKeeperParams || !PackAvailable() ||
        !NativeShopSceneReady(play) || sNativeShop) {
        Actor_Kill(actor);
        return;
    }
    sNativeShop = reinterpret_cast<EnOssan*>(actor);
    sNativeShopPlay = play;
    actor->csId = BlackMarketMmShopScene::kFirstCutscene;
    actor->update = KeeperUpdate;
    CutsceneManager_SetCameraOwnershipProtection(true);
    EnOssan_Init(actor, play);
}
void PresentTitle() {
    if (!sTitlePending || !Interior())
        return;
    if (!ResourceMgr_FileExists(kCard) && !(ResourceMgr_IsAltAssetsEnabled() && ResourceMgr_FileAltExists(kCard))) {
        sTitlePending = false;
        return;
    }
    if (ComboPlaceTitleCard_Show(gPlayState, (TexturePtr)kCard, 240, 24) != 0)
        sTitlePending = false;
}
void MarketText(u16* textId, bool* fromTable) {
    using BlackMarketText::Page;
    if (Interior() && *textId == BlackMarketEntry::kUnavailableText) {
        CustomMessage::Entry entry;
        entry.autoFormat = false;
        entry.msg = BlackMarketText::Mm(Page::Unavailable, {}, {}, sNESFontWidths);
        CustomMessage::LoadCustomMessageIntoFont(entry);
        *fromTable = false;
        return;
    }
    if (Active() && (*textId == kKeeperMenuText || *textId == kKeeperTalkText || *textId == kContinueText ||
                     *textId == kKeeperHelpChoiceText || *textId == kKeeperHelpText)) {
        CustomMessage::Entry entry;
        entry.autoFormat = false;
        entry.textboxYPos = 0x30;
        if (*textId == kKeeperTalkText || *textId == kKeeperHelpChoiceText || *textId == kKeeperHelpText) {
            const Page page = *textId == kKeeperTalkText         ? Page::KeeperTalk
                              : *textId == kKeeperHelpChoiceText ? Page::KeeperHelpChoice
                                                                 : Page::KeeperHelp;
            entry.msg = BlackMarketText::Mm(page, {}, {}, sNESFontWidths);
        } else {
            entry.msg = ComboItemReceiptText::FromNeiMarkup(*textId == kContinueText ? "Keep shopping?"
                                                                                     : "Left / Right: browse items");
            ComboItemReceiptText::Wrap(entry.msg, sNESFontWidths, 160, 300.f);
            entry.msg += "\x11\x02\xC2";
            entry.msg += *textId == kContinueText ? "Yes\x11No\xBF" : "Talk to shopkeeper\x11Leave\xBF";
        }
        CustomMessage::LoadCustomMessageIntoFont(entry);
        *fromTable = false;
        return;
    }
    if (!Active() ||
        (*textId != kBrowseText && *textId != kResultText && *textId != kWelcomeText && *textId != kConfirmText))
        return;
    if (sNativeShop && (*textId == kBrowseText || *textId == kConfirmText))
        sSelected = sNativeShop->cursorIndex;
    const Page page = *textId == kWelcomeText   ? Page::Welcome
                      : *textId == kConfirmText ? Page::Confirm
                      : *textId == kResultText  ? Page::Result
                                                : Page::Browse;
    BlackMarketText::Offer offer;
    if (page != Page::Welcome) {
        offer.item = Offer(sSelected);
        if (!offer.item && page != Page::Result)
            return;
        if (offer.item) {
            offer.name = offer.item->label;
            // Resolve the fixed item we deliver, without progressive/check conversion.
            const auto id = ComboBlackMarketItems::Id(*offer.item);
            const auto metadata = Rando::StaticData::Items.find(id);
            if (id != RI_UNKNOWN && metadata != Rando::StaticData::Items.end() && metadata->second.name &&
                metadata->second.name[0])
                offer.name = metadata->second.name;
            const auto& stock = sSession.market["stock"][sSelected];
            offer.price = stock["price"].get<int>();
            offer.clears = stock["clears"].get<int>();
            offer.slot = sSelected;
            if (page == Page::Browse)
                offer.status = Status(sSelected);
        }
    }
    CustomMessage::Entry entry;
    entry.autoFormat = false; // The native MM codec/wrapper runs before raw choice controls are appended.
    entry.textboxYPos = 0x30; // Initial native Start fixes all later Continue pages below merchandise.
    if (page == Page::Browse || page == Page::Confirm)
        entry.firstItemCost = offer.price;
    entry.msg = BlackMarketText::Mm(page, offer, sStatus, sNESFontWidths);
    CustomMessage::LoadCustomMessageIntoFont(entry);
    *fromTable = false;
}

void DrawSeal(PlayState* play, const BlackMarketVisual::Frame& frame, bool selected) {
    using namespace BlackMarketVisual;
    const auto mesh = BuildLock(frame, play->gameplayFrames, selected, BlackMarketMmShopLayout::kSealPaddingScale);
    if (!mesh.valid ||
        !BlackMarketGraphicsArena::HasRoom(play->state.gfxCtx, kLockVertices * sizeof(Vtx) + sizeof(Mtx), 8, 64))
        return;
    auto* vertices = (Vtx*)GRAPH_ALLOC(play->state.gfxCtx, kLockVertices * sizeof(Vtx));
    if (!vertices)
        return;
    for (int i = 0; i < kLockVertices; ++i) {
        const auto& source = mesh.vertices[i];
        auto& v = vertices[i];
        v = {};
        v.v.ob[0] = source.x;
        v.v.ob[1] = source.y;
        v.v.ob[2] = source.z;
        v.v.tc[0] = source.s;
        v.v.tc[1] = source.t;
        v.v.cn[0] = v.v.cn[1] = v.v.cn[2] = v.v.cn[3] = 255;
    }
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
    Matrix_Push();
    Matrix_Translate(mesh.center.x, mesh.center.y, mesh.center.z, MTXMODE_NEW);
    Matrix_Scale(1.f / kLockVertexScale, 1.f / kLockVertexScale, 1.f / kLockVertexScale, MTXMODE_APPLY);
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
    gSPClearGeometryMode(POLY_XLU_DISP++, G_LIGHTING | G_FOG | G_CULL_BOTH);
    // SETUPDL_25 uses opaque blending even on the translucent command stream.
    gDPSetRenderMode(POLY_XLU_DISP++, G_RM_PASS, G_RM_AA_ZB_XLU_SURF2);
    gDPSetTextureLUT(POLY_XLU_DISP++, G_TT_NONE);
    gDPSetCombineMode(POLY_XLU_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, mesh.color[0], mesh.color[1], mesh.color[2], mesh.color[3]);
    gSPTexture(POLY_XLU_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPLoadTextureBlock(POLY_XLU_DISP++, kLockTexture.data(), G_IM_FMT_IA, G_IM_SIZ_8b, kLockTextureWidth,
                        kLockTextureHeight, 0, G_TX_WRAP, G_TX_CLAMP, 5, 3, 0, 0);
    for (int batch = 0; batch < kLockVertices; batch += 32) {
        gSPVertex(POLY_XLU_DISP++, (uintptr_t)(vertices + batch), 32, 0);
        for (int i = 0; i < 8; ++i)
            gSP2Triangles(POLY_XLU_DISP++, i * 4, i * 4 + 1, i * 4 + 2, 0, i * 4, i * 4 + 2, i * 4 + 3, 0);
    }
    gSPTexture(POLY_XLU_DISP++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
    Matrix_Pop();
    gDPPipeSync(POLY_XLU_DISP++);
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
    CLOSE_DISPS(play->state.gfxCtx);
}

void DrawShelves(PlayState* play) {
    sCounterBarrier.Ensure(play);
    if (!sShelfFixture.Ensure(play))
        return;
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    Matrix_Push();
    Matrix_Translate(0, 0, 0, MTXMODE_NEW);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    const auto color =
        CVarGetColor("gCosmetic.World.BlackMarketShelves.Color",
                     Color_RGBA8{ BlackMarketShelves::kDefaultTint[0], BlackMarketShelves::kDefaultTint[1],
                                  BlackMarketShelves::kDefaultTint[2], 255 });
    sShelfFixture.Draw(POLY_OPA_DISP, sGrannyActive, sGrannyResources.ShelfTextures(),
                       BlackMarketShelves::Tint(CVarGetInteger("gCosmetic.World.BlackMarketShelves.Changed", 0) != 0,
                                                { color.r, color.g, color.b, 255 }));
    Matrix_Pop();
    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    CLOSE_DISPS(play->state.gfxCtx);
}
void DrawMarket() {
    PlayState* play = gPlayState;
    if (!Interior() || !play->state.running)
        return;
    DrawShelves(play);
    BlackMarketInteriorEffects::Draw(play, !sGrannyActive, false);
    if (!Active())
        return;
    for (int slot = 0; slot < 8; ++slot) {
        const Item* item = Offer(slot);
        if (!item)
            continue;
        const auto status = Status(slot);
        if (status == PurchaseResult::SoldOut && !item->renewable)
            continue;
        const bool locked = status == PurchaseResult::Locked;
        BlackMarketVisual::Capture capture(locked);
        auto* stock = sNativeShop && sNativeShopPlay == play ? sNativeShop->items[slot] : nullptr;
        if (!stock || !stock->actor.update)
            continue;
        Actor& display = stock->actor;
        Matrix_Push();
        Matrix_Translate(display.world.pos.x, display.world.pos.y, display.world.pos.z, MTXMODE_NEW);
        Matrix_RotateYS(stock->rotY + play->gameplayFrames * 100 + slot * 4096, MTXMODE_APPLY);
        Matrix_Scale(display.scale.x, display.scale.y, display.scale.z, MTXMODE_APPLY);
        Rando::DrawResolvedItem(ComboBlackMarketItems::Id(*item), RC_UNKNOWN, &display);
        Matrix_Pop();
        if (locked && CVarGetInteger(BlackMarketVisual::kSealsCvar, 1))
            DrawSeal(play, capture.frame, sTalking && slot == sSelected);
    }
}

void PlayerUpdate(Actor* actor) {
    PlayState* play = gPlayState;
    if (!Safe(play) || Player_InCsMode(play))
        return;
    if (Interior()) {
        if (BlackMarketExitDoor::WalkExit<false>(play, actor->world.pos)) {
            RememberTravel();
            sReturnPending = true;
            Transition(play, ReturnEntrance());
        }
        return;
    }
    if (!Exterior(play) || !DestinationAvailable())
        return;
    if (BlackMarketDoorNative::HasDoor(play))
        return; // The native Open action owns the authored door, including unavailable/ambiguous markers.
    const auto door = Door();
    Vec3f probe = { door.x, 120.0f, door.z };
    CollisionPoly* floor = nullptr;
    s32 bg = BGCHECK_SCENE;
    const float height = BgCheck_EntityRaycastFloor5_3(play, &play->colCtx, &floor, &bg, actor, &probe);
    if (height == BGCHECK_Y_MIN || !floor || floor->normal.y < 23170)
        return;
    sDoorFloor = height;
    const float dx = actor->world.pos.x - door.x;
    const float dz = actor->world.pos.z - door.z;
    if (dx * dx + dz * dz > 14400) {
        sDoorArmed = true;
        if (SoloExterior(play) && actor->floorPoly && actor->floorPoly->normal.y >= 23170 &&
            std::isfinite(actor->floorHeight) && std::abs(actor->world.pos.y - actor->floorHeight) < 35.0f) {
            sSoloReturn = actor->world.pos;
            sSoloReturnYaw = actor->shape.rot.y;
            sSoloReturnEntrance = gSaveContext.save.entrance;
        }
    }
    const bool atDoor = ComboBlackMarket_InDoor(actor->world.pos.x, actor->world.pos.y, actor->world.pos.z, height);
    if (sDoorArmed && atDoor) {
        sDoorArmed = false;
        sEntering = true;
        RememberTravel();
        Transition(play, COMBO_BLACK_MARKET_ENTRANCE);
    }
}
void SceneInit(s8 scene, s8 spawn) {
    ClearRoomMagic();
    CancelReceipt(true);
    sShelfFixture.Reset();
    sDoorCollision.Reset();
    sCounterBarrier.Reset();
    if (sNativeShopPlay != gPlayState || scene != COMBO_BLACK_MARKET_SCENE)
        EndBrowsing(gPlayState, true);
    GrannySceneBegin();
    ApplyGrannyInterior();
    if (sReturnStaged) {
        gSaveContext.respawn[RESPAWN_MODE_RETURN] = sPreservedReturn;
        sReturnStaged = false;
    }
    sEntering = false;
    sDoorArmed = false;
    sTalking = false;
    sTitlePending = false;
    if (scene == COMBO_BLACK_MARKET_SCENE) {
        sSelected = 0;
        sTitlePending = PackAvailable();
    } else if (sReturnPending && MatchesTravel() && Exterior(gPlayState)) {
        sReturnPending = false;
        Player* player = GET_PLAYER(gPlayState);
        if (!player)
            return;
        BlackMarketDoorNative::Pose placed;
        const bool authoredDoor = BlackMarketDoorNative::Find(gPlayState, placed);
        Vec3f pos = authoredDoor ? BlackMarketDoorNative::Outside(placed)
                    : SoloExterior(gPlayState)
                        ? sSoloReturn
                        : Vec3f{ COMBO_BLACK_MARKET_DOOR_X - 226.0f, 0, COMBO_BLACK_MARKET_DOOR_Z };
        pos.y += 120.0f;
        CollisionPoly* floor = nullptr;
        s32 bg = BGCHECK_SCENE;
        const float y =
            BgCheck_EntityRaycastFloor5_3(gPlayState, &gPlayState->colCtx, &floor, &bg, &player->actor, &pos);
        if (y == BGCHECK_Y_MIN || !floor || floor->normal.y < 23170)
            return;
        pos.y = y;
        const s16 yaw = authoredDoor ? placed.yaw : SoloExterior(gPlayState) ? sSoloReturnYaw : -16384;
        if (player->actionFunc == Player_Action_35) {
            const float turn = (s16)(yaw - player->actor.shape.rot.y) * (6.28318530718f / 65536.0f);
            const Vec3f delta = { player->unk_3A0.x - player->actor.world.pos.x,
                                  player->unk_3A0.y - player->actor.world.pos.y,
                                  player->unk_3A0.z - player->actor.world.pos.z };
            player->unk_3A0 = { pos.x + delta.x * std::cos(turn) + delta.z * std::sin(turn), pos.y + delta.y,
                                pos.z - delta.x * std::sin(turn) + delta.z * std::cos(turn) };
        }
        player->actor.world.pos = player->actor.prevPos = player->actor.home.pos = pos;
        player->yaw = player->actor.world.rot.y = player->actor.shape.rot.y = yaw;
        player->actor.velocity = {};
        player->actor.speed = 0;
        player->actor.floorHeight = y;
        player->actor.floorPoly = floor;
        player->actor.floorBgId = bg;
        Camera_InitFocalActorSettings(&gPlayState->mainCamera, &player->actor);
    } else {
        sReturnPending = false;
    }
}
void DrawExterior() {
    PlayState* play = gPlayState;
    if (!Exterior(play) || !PackAvailable())
        return;
    if (BlackMarketDoorNative::HasDoor(play))
        return; // The finished scene already draws its building and door.
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    Matrix_Push();
    const auto door = Door();
    Matrix_Translate(door.x, sDoorFloor, door.z, MTXMODE_NEW);
    Matrix_RotateYS(COMBO_BLACK_MARKET_DOOR_YAW, MTXMODE_APPLY);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)kExterior);
    Matrix_Pop();
    CLOSE_DISPS(play->state.gfxCtx);
}
void LoadSolo(s16 slot) {
    if (gComboGameMode != COMBO_GAME_MM || !ComboLostWoods_IsPlayableSlot(slot))
        return;
    nlohmann::json saved;
    if (SaveManager_ReadSaveFile(SaveManager_GetFileName(slot + 1), saved) != 0) {
        sSession.Bind(nullptr, -1);
        return;
    }
    const bool create = !saved.contains("blackMarket");
    if (create) {
        Options options;
        options.games = Games::Mm;
        options.dungeonUnlocks = CVarGetInteger("gCombo.Rando.BlackMarketDungeonUnlocks", 1) != 0;
        saved["blackMarket"] = { { "market", Generate(gSaveContext.save.shipSaveInfo.rando.finalSeed, options) } };
    }
    ComboBlackMarket_LoadState(saved, slot);
    if (create && sSession.Active(slot))
        SaveManager_WriteSaveFile(SaveManager_GetFileName(slot + 1), saved);
    // Native load hooks may still replace entrance/respawns. Play_Init's
    // normalizer consumes the seed/slot-bound saved-inside marker last.
}

void RegisterMarket() {
    static EntranceTableEntry entries[4] = { { COMBO_BLACK_MARKET_SCENE, 0, TRANS_TYPE_FADE_BLACK << 7 },
                                             { COMBO_BLACK_MARKET_SCENE, 0, TRANS_TYPE_FADE_BLACK << 7 },
                                             { COMBO_BLACK_MARKET_SCENE, 0, TRANS_TYPE_FADE_BLACK << 7 },
                                             { COMBO_BLACK_MARKET_SCENE, 0, TRANS_TYPE_FADE_BLACK << 7 } };
    static EntranceTableEntry* spawns[] = { entries };
    auto* table = &gSceneTable[COMBO_BLACK_MARKET_SCENE];
    table->segment.vromStart = table->segment.vromEnd = 0;
    table->segment.fileName = (char*)"skiejer_market";
    table->titleTextId = 0;
    table->drawConfig = SCENE_DRAW_CFG_DEFAULT;
    table->unk_A = table->unk_C = table->unk_D = 0;
    sSceneEntranceTable[ENTR_SCENE_UNSET_09].tableCount = 1;
    sSceneEntranceTable[ENTR_SCENE_UNSET_09].table = spawns;
    sSceneEntranceTable[ENTR_SCENE_UNSET_09].name = (char*)"SKIEJER_MARKET";
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>(SceneInit);
    Ship::CrossRMRegistry::RegisterTeardownListener([] {
        ClearRoomMagic();
        sGrannyResources.Clear();
    });
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnActorUpdate>(
        ACTOR_EN_DOOR, [](Actor*) { SyncShopDoorCollision(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDrawWorldStart>([] {
        DrawGrannyRoom();
        BeginRoomMagic();
    });
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::ShouldVanillaBehavior>(
        VB_DISABLE_LETTERBOX, [](GIVanillaBehavior, bool* should, va_list) {
            // Native view scissoring and black rectangles share this flag.
            // Preserve other hooks/user preferences and native bar state.
            if (NativeInspectionOwnsView(gPlayState))
                *should = true;
        });
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::ShouldActorDraw>(
        ACTOR_PLAYER, [](Actor* actor, bool* should) {
            auto* play = gPlayState;
            if (!play || gGameState != &play->state || !NativeShopContext(play) || !sTalking || sReceipt.pending)
                return;
            const auto* player = GET_PLAYER(play);
            // Native EnOssan owns the hide bit and releases it for receipts.
            // Filter before Player_Draw so adult/custom replacement rigs and
            // their attached models cannot bypass the vanilla body guard.
            if (player && actor == &player->actor && (player->stateFlags2 & PLAYER_STATE2_20000000))
                *should = false;
        });
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::ShouldActorInit>(
        ACTOR_EN_OSSAN, [](Actor* actor, bool* should) {
            if (!Interior() || actor->params != BlackMarketMmShopScene::kKeeperParams)
                return;
            actor->init = InitNativeKeeper;
            *should = true;
        });
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::ShouldActorInit>(
        ACTOR_EN_GIRLA, [](Actor* actor, bool* should) {
            if (!Interior() || actor->params < BlackMarketMmShopScene::kItemParamBase ||
                actor->params >= BlackMarketMmShopScene::kItemParamBase + 8)
                return;
            actor->init = InitNativeStock;
            *should = true;
        });
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::ShouldActorInit>(ACTOR_EN_TAKARAYA,
                                                                                     [](Actor* actor, bool* should) {
                                                                                         if (!Interior())
                                                                                             return;
                                                                                         // Legacy visual packs carry a
                                                                                         // smaller minigame actor and
                                                                                         // no shop cutscenes. They need
                                                                                         // the prepared MM scene; never
                                                                                         // reinterpret it.
                                                                                         actor->destroy = nullptr;
                                                                                         *should = false;
                                                                                     });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSaveLoad>(LoadSolo);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSaveInit>([](s16) {
        if (gComboGameMode != COMBO_GAME_MM)
            return;
        sSession.Bind(nullptr, -1);
        sClears = 0;
        sEntering = sReturnPending = sReturnStaged = sResumeFromMarket = false;
    });
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnActorUpdate>(ACTOR_PLAYER, PlayerUpdate);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnOpenText>(MarketText);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDrawWorldEnd>([] {
        RestoreRoomMagic();
        DrawMarket();
        DrawExterior();
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameStateUpdate>([] {
        SyncShopDoorCollision();
        MonitorBrowsing();
        MonitorReceipt();
        PresentTitle();
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>([] {
        ClearRoomMagic();
        sShelfFixture.Reset();
        sDoorCollision.Reset();
        sCounterBarrier.Reset();
        EndBrowsing(gPlayState, true);
        sNativeShopSceneResource.reset();
        CancelReceipt(true);
        sGrannyActive = sGrannyPending = false;
        sGrannyOriginal = nullptr;
        sTitlePending = sTalking = sDoorArmed = false;
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnBossDefeated>([](s16 actor) {
        if (!gPlayState || !sSession.Active(gSaveContext.fileNum) || gSaveContext.gameMode != GAMEMODE_NORMAL)
            return;
        int boss = actor == ACTOR_BOSS_01        ? 8
                   : actor == ACTOR_BOSS_HAKUGIN ? 9
                   : actor == ACTOR_BOSS_03      ? 10
                   : actor == ACTOR_BOSS_02      ? 11
                                                 : -1;
        static constexpr int scenes[] = { SCENE_MITURIN_BS, SCENE_HAKUGIN_BS, SCENE_SEA_BS, SCENE_INISIE_BS };
        if (boss >= 0 && gPlayState->sceneId != scenes[boss - 8])
            return;
        if (boss < 0)
            return;
        if (gComboGameMode == COMBO_GAME_MM) {
            const auto next = static_cast<uint16_t>(sClears | (1u << boss));
            if (next != sClears) {
                sClears = next;
                SaveManager_SaveCurrentForCombo(true);
            }
        } else if (Combo_IsCombinedGame() && sReportClear)
            sReportClear(boss, gSaveContext.fileNum);
    });
}
RegisterShipInitFunc init(RegisterMarket);
} // namespace

extern "C" void ComboBlackMarket_RestoreRoomAfterDraw(PlayState* play) {
    if (play && play == gPlayState && play == sRoomMagicPlay)
        RestoreRoomMagic();
}

extern "C" const ComboBlackMarketShopAdapter* ComboBlackMarketShop_GetAdapter(EnOssan* shop, PlayState* play) {
    return shop && shop == sNativeShop && play == sNativeShopPlay && Interior() && shop->actor.id == ACTOR_EN_OSSAN &&
                   shop->actor.params == BlackMarketMmShopScene::kKeeperParams
               ? &kNativeShopAdapter
               : nullptr;
}

extern "C" COMBO_EXPORT void MM_SetBlackMarketSeed(const char* text, int slot, uint16_t clears) try {
    auto value = nlohmann::json::parse(text ? text : "null");
    sSession.Bind(value, slot);
    sClears = clears & 0xFFF;
} catch (...) {
    sSession.Bind(nullptr, -1);
    sClears = 0;
}
extern "C" COMBO_EXPORT void MM_SetBlackMarketClearCallback(void (*callback)(int, int)) {
    sReportClear = callback;
}
extern "C" int ComboBlackMarket_IsInterior(void) {
    return Interior();
}
extern "C" int ComboBlackMarket_IsCounterBarrier(const CollisionContext* context, int bgId) {
    return sCounterBarrier.Owns(context, bgId);
}
extern "C" void ComboBlackMarket_ResetState(void) {
    sSession.state = {};
    sResumeFromMarket = false;
}
void ComboBlackMarket_SaveState(nlohmann::json& save, int slot) {
    // Native file-select copies/imports write full documents without loading
    // their market session. Preserve their existing stock and bits verbatim.
    if (gComboGameMode == COMBO_GAME_MM &&
        (!gPlayState || gGameState != &gPlayState->state || !gPlayState->state.running))
        return;
    if (sSession.Active(slot) && slot == gSaveContext.fileNum) {
        save["blackMarket"] = sSession.Save(slot);
        if (gComboGameMode == COMBO_GAME_MM) {
            save["blackMarket"]["clears"] = sClears;
            save["blackMarket"]["return"] = { { "entrance", sSoloReturnEntrance },
                                              { "x", sSoloReturn.x },
                                              { "y", sSoloReturn.y },
                                              { "z", sSoloReturn.z },
                                              { "yaw", sSoloReturnYaw } };
        }
        save["blackMarket"]["savedInside"] = Interior() && gSaveContext.save.entrance == COMBO_BLACK_MARKET_ENTRANCE;
    }
}
void ComboBlackMarket_LoadState(const nlohmann::json& save, int slot) {
    CancelReceipt();
    const auto value = save.is_object() ? save.value("blackMarket", nlohmann::json()) : nlohmann::json();
    if (gComboGameMode == COMBO_GAME_MM) {
        sEntering = sReturnPending = sReturnStaged = sResumeFromMarket = false;
        const auto market = value.is_object() ? value.value("market", nlohmann::json()) : nlohmann::json();
        // A native file owns its frozen stock; never accept a combined or OoT
        // payload in solo MM, even if it happens to use the same seed number.
        const bool valid = Validate(market) && MatchesGames(market, Games::Mm) &&
                           market["seed"] == gSaveContext.save.shipSaveInfo.rando.finalSeed;
        sSession.Bind(valid ? market : nlohmann::json(), slot);
        const auto clears = value.is_object() ? value.value("clears", nlohmann::json(0)) : nlohmann::json();
        sClears = valid && UnsignedInRange(clears, 0xF00) ? clears.get<uint16_t>() & 0xF00 : 0;
        sSoloReturn = { 184, 0, -300 };
        sSoloReturnYaw = -16384;
        sSoloReturnEntrance = COMBO_SOLO_FOREST_MM_ENTRANCE;
        const auto point = value.is_object() ? value.value("return", nlohmann::json()) : nlohmann::json();
        if (point.is_object())
            try {
                const auto entrance = point.at("entrance").get<int>();
                const Vec3f pos = { point.at("x").get<float>(), point.at("y").get<float>(),
                                    point.at("z").get<float>() };
                const auto yaw = point.at("yaw").get<int>();
                if (entrance >= 0 && entrance == COMBO_SOLO_FOREST_MM_ENTRANCE && std::isfinite(pos.x) &&
                    std::isfinite(pos.y) && std::isfinite(pos.z) && std::abs(pos.x) < 4096 && std::abs(pos.y) < 4096 &&
                    std::abs(pos.z) < 4096 && yaw >= INT16_MIN && yaw <= INT16_MAX) {
                    sSoloReturn = pos;
                    sSoloReturnYaw = yaw;
                    sSoloReturnEntrance = entrance;
                }
            } catch (const nlohmann::json::exception&) {}
    }
    sSession.Load(value, slot);
    sResumeFromMarket = sSession.Active(slot) && value.is_object() &&
                        value.value("market", nlohmann::json()) == sSession.market &&
                        value.value("savedInside", nlohmann::json()) == true;
    if (gComboGameMode == COMBO_GAME_MM && sResumeFromMarket)
        RememberTravel();
}
extern "C" int ComboBlackMarket_IsAuthoredDoor(PlayState* play, Actor* door) {
    BlackMarketDoorNative::Pose placed;
    return play == gPlayState && Exterior(play) && BlackMarketDoorNative::Find(play, placed) &&
           BlackMarketDoorNative::Matches(door, placed);
}
namespace {
void LogDoorAttempt(PlayState* play, Actor* door, int result, const char* reason) {
    if (!play || (play->sceneId != SCENE_LOST_WOODS && play->sceneId != COMBO_SOLO_FOREST_MM_SCENE))
        return;
    BlackMarketDoorNative::Pose placed;
    const bool found = BlackMarketDoorNative::Find(play, placed);
    const auto* player = GET_PLAYER(play);
    const auto missing = BlackMarketEntry::Missing(BlackMarketEntry::kMmDestination, ResourceMgr_FileExists);
    nlohmann::json record = {
        { "host", "mm" },
        { "result", BlackMarketEntry::ResultName(result) },
        { "reason", reason },
        { "location",
          { { "mode", gComboGameMode },
            { "scene", play->sceneId },
            { "room", play->roomCtx.curRoom.num },
            { "layer", gSaveContext.sceneLayer },
            { "entrance", gSaveContext.save.entrance },
            { "cutscene_index", gSaveContext.save.cutsceneIndex } } },
        { "recognition",
          { { "current_play", play == gPlayState },
            { "exterior", Exterior(play) },
            { "marker_found", found },
            { "marker_matches", found && BlackMarketDoorNative::Matches(door, placed) },
            { "transition_actors", play->transitionActors.count },
            { "door_id", door ? door->id : -1 },
            { "door_params", door ? door->params : -1 } } },
        { "gates",
          { { "transition_idle",
              play->transitionTrigger == TRANS_TRIGGER_OFF && play->transitionMode == TRANS_MODE_OFF },
            { "native_state_ready", Safe(play) && player && player->doorActor == door &&
                                        player->actionFunc == Player_Action_36 &&
                                        gSaveContext.save.saveInfo.playerData.health > 0 },
            { "destination_ready", missing.empty() } } },
        { "state",
          { { "current_game_state", gGameState == &play->state },
            { "running", play->state.running },
            { "game_mode", gSaveContext.gameMode },
            { "file", gSaveContext.fileNum },
            { "transition_trigger", play->transitionTrigger },
            { "transition_mode", play->transitionMode },
            { "pause", play->pauseCtx.state },
            { "game_over", play->gameOverCtx.state },
            { "scene_cutscene", play->csCtx.state },
            { "cutscene_trigger", gSaveContext.cutsceneTrigger },
            { "soaring_or_time_song", play->soaringCsOrSoTCsPlaying },
            { "player_present", player != nullptr },
            { "player_owns_door", player && player->doorActor == door },
            { "player_action_expected", player && player->actionFunc == Player_Action_36 },
            { "player_door_type", player ? player->doorType : -1 },
            { "player_form", player ? player->transformation : -1 },
            { "health", gSaveContext.save.saveInfo.playerData.health } } },
        { "market", BlackMarketEntry::MarketState(sSession, gSaveContext.fileNum) },
        { "travel",
          { { "entering", sEntering },
            { "return_pending", sReturnPending },
            { "door_armed", sDoorArmed },
            { "slot", sTravelSlot },
            { "seed", sTravelSeed },
            { "respawn_flag", gSaveContext.respawnFlag } } },
        { "resources",
          { { "destination_missing", missing },
            { "manifest_raw", ResourceMgr_FileExists(BlackMarketEntry::kManifest) != 0 },
            { "manifest_cache_key", ResourceMgr_FileExists(BlackMarketEntry::kManifestKey) != 0 } } }
    };
    record["checks"] = { { "door_recognized", play == gPlayState && Exterior(play) && found &&
                                                  BlackMarketDoorNative::Matches(door, placed) },
                         { "game_state_current", gGameState == &play->state },
                         { "running", play->state.running != 0 },
                         { "normal_mode", gSaveContext.gameMode == GAMEMODE_NORMAL },
                         { "playable_slot", ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) != 0 },
                         { "transition_idle",
                           play->transitionTrigger == TRANS_TRIGGER_OFF && play->transitionMode == TRANS_MODE_OFF },
                         { "pause_closed", play->pauseCtx.state == PAUSE_STATE_OFF },
                         { "game_over_inactive", play->gameOverCtx.state == GAMEOVER_INACTIVE },
                         { "scene_cutscene_idle", play->csCtx.state == CS_STATE_IDLE },
                         { "cutscene_not_pending", !gSaveContext.cutsceneTrigger },
                         { "time_song_idle", !play->soaringCsOrSoTCsPlaying },
                         { "player_present", player != nullptr },
                         { "player_owns_door", player && player->doorActor == door },
                         { "player_action_expected", player && player->actionFunc == Player_Action_36 },
                         { "alive", gSaveContext.save.saveInfo.playerData.health > 0 },
                         { "destination_ready", missing.empty() } };
    record["failed_checks"] = BlackMarketEntry::FailedChecks(record["checks"]);
    if (!missing.empty()) {
        record["resources"]["native_missing"] =
            BlackMarketEntry::Missing(kNativeBlackMarketMmResources, ResourceMgr_FileExists);
        record["resources"]["legacy_missing"] = BlackMarketEntry::Missing(kBlackMarketResources, [](const char* path) {
            return BlackMarketEntry::LegacyResourceAvailable(path, ResourceMgr_FileExists);
        });
    }
    SPDLOG_INFO("[BlackMarketDoor] {}", record.dump());
}
void LogEntryRecovery() {
    const nlohmann::json record = { { "host", "mm" },
                                    { "file", gSaveContext.fileNum },
                                    { "entering", sEntering },
                                    { "travel_matches", MatchesTravel() },
                                    { "travel_slot", sTravelSlot },
                                    { "travel_seed", sTravelSeed },
                                    { "seed", gSaveContext.save.shipSaveInfo.rando.finalSeed },
                                    { "respawn_flag", gSaveContext.respawnFlag },
                                    { "destination_ready", DestinationAvailable() },
                                    { "market", BlackMarketEntry::MarketState(sSession, gSaveContext.fileNum) } };
    SPDLOG_INFO("[BlackMarketReturn] {}", record.dump());
}
} // namespace
extern "C" int ComboBlackMarket_TryDoorEntrance(PlayState* play, Actor* door) {
    if (play == gPlayState && Interior() && BlackMarketExitDoor::Matches<false>(play, door)) {
        const auto* player = GET_PLAYER(play);
        if (!Safe(play) || sTalking || !player || player->doorActor != door ||
            player->doorType != PLAYER_DOORTYPE_HANDLE || player->actionFunc != Player_Action_36 ||
            gSaveContext.save.saveInfo.playerData.health <= 0)
            return COMBO_BLACK_MARKET_DOOR_BLOCKED;
        sDoorCollision.Release(play);
        RememberTravel();
        sReturnPending = true;
        Transition(play, ReturnEntrance());
        return COMBO_BLACK_MARKET_DOOR_ENTERED;
    }
    BlackMarketDoorNative::Pose placed;
    if (play != gPlayState || !Exterior(play) || !BlackMarketDoorNative::Find(play, placed) ||
        !BlackMarketDoorNative::Matches(door, placed)) {
        LogDoorAttempt(play, door, COMBO_BLACK_MARKET_DOOR_UNHANDLED, "door_not_recognized");
        return COMBO_BLACK_MARKET_DOOR_UNHANDLED;
    }
    // Preserve native transition/player safety. Stock and optional presentation
    // resources are independent of whether this physical destination is accessible.
    if (play->transitionTrigger != TRANS_TRIGGER_OFF || play->transitionMode != TRANS_MODE_OFF) {
        LogDoorAttempt(play, door, COMBO_BLACK_MARKET_DOOR_BLOCKED, "transition_active");
        return COMBO_BLACK_MARKET_DOOR_BLOCKED;
    }
    const auto* player = GET_PLAYER(play);
    if (!Safe(play) || !player || player->doorActor != door || player->actionFunc != Player_Action_36 ||
        gSaveContext.save.saveInfo.playerData.health <= 0) {
        LogDoorAttempt(play, door, COMBO_BLACK_MARKET_DOOR_BLOCKED, "native_state_not_ready");
        return COMBO_BLACK_MARKET_DOOR_BLOCKED;
    }
    if (!DestinationAvailable()) {
        LogDoorAttempt(play, door, COMBO_BLACK_MARKET_DOOR_BLOCKED, "destination_missing");
        return COMBO_BLACK_MARKET_DOOR_BLOCKED;
    }
    LogDoorAttempt(play, door, COMBO_BLACK_MARKET_DOOR_ENTERED, "native_open");
    sDoorCollision.Release(play);
    if (SoloExterior(play)) {
        sSoloReturn = BlackMarketDoorNative::Outside(placed);
        sSoloReturnYaw = placed.yaw;
        sSoloReturnEntrance = gSaveContext.save.entrance;
    }
    sDoorArmed = sReturnPending = false;
    sEntering = true;
    RememberTravel();
    Transition(play, COMBO_BLACK_MARKET_ENTRANCE);
    return COMBO_BLACK_MARKET_DOOR_ENTERED;
}
extern "C" void ComboBlackMarket_ApplyResume(int resume) {
    const bool fromMarket = sResumeFromMarket;
    sResumeFromMarket = false;
    if (!resume || !fromMarket || !sSession.Active(gSaveContext.fileNum))
        return;
    sReturnPending = true;
    RememberTravel();
    gSaveContext.save.entrance = ReturnEntrance();
    gSaveContext.save.cutsceneIndex = 0;
    gSaveContext.respawnFlag = 0;
    StageSoloReturn();
}
extern "C" void ComboBlackMarket_NormalizeEntry(void) {
    if (gComboGameMode == COMBO_GAME_OOT)
        return;
    if (gComboGameMode == COMBO_GAME_MM && sResumeFromMarket) {
        sResumeFromMarket = false;
        if (MatchesTravel() && sSession.Active(gSaveContext.fileNum)) {
            sEntering = false;
            sReturnPending = true;
            gSaveContext.save.entrance = ReturnEntrance();
            gSaveContext.save.cutsceneIndex = 0;
            gSaveContext.respawnFlag = 0;
            StageSoloReturn();
            return;
        }
    }
    if (sReturnPending && MatchesTravel() && gSaveContext.save.entrance == ReturnEntrance()) {
        StageSoloReturn();
        return;
    }
    if (gSaveContext.save.entrance != COMBO_BLACK_MARKET_ENTRANCE)
        return;
    if (sEntering && MatchesTravel() && DestinationAvailable() && gSaveContext.respawnFlag == 0)
        return;
    // Save reload, void/death and missing-pack recovery use the ordinary MM
    // forest spawn. No portal callback, shared arrival flag or TOP respawn write.
    LogEntryRecovery();
    sEntering = false;
    sReturnPending = true;
    RememberTravel();
    gSaveContext.save.entrance = ReturnEntrance();
    gSaveContext.save.cutsceneIndex = 0;
    gSaveContext.respawnFlag = 0;
    StageSoloReturn();
}
#endif
