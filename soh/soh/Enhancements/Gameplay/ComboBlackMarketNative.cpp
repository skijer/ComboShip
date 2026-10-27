#ifdef COMBO_BUILD
#include "gameplay/CrossGameTitleCard.h"
#include "ComboBlackMarketNative.h"
#include "ComboBlackMarketItems.h"
#include "ComboGameMode.h"
#include "entrances/BlackMarketEntrance.h"
#include "entrances/SoloForest.h"
#include "BlackMarketLock.h"
#include "BlackMarketGraphicsArena.h"
#include "BlackMarketKeeper.h"
#include "BlackMarketNativeResources.h"
#include "BlackMarketEntry.h"
#include "BlackMarketExitDoor.h"
#include "BlackMarketDoorCollision.h"
extern "C" {
#include "overlays/actors/ovl_En_Door/z_en_door.h"
void EnDoor_Idle(EnDoor*, PlayState*);
}
#include "BlackMarketGrannyResources.h"
#include "BlackMarketRoomMagicResources.h"
#include "BlackMarketBrowsing.h"
#include "BlackMarketReceipt.h"
#include "BlackMarketText.h"
#include "entrances/LostWoodsEntrance.h"
#include "soh/SaveManager.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/ActorDB.h"
#include "soh/ShipInit.hpp"
#include "soh/frame_interpolation.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/Enhancements/custom-message/CustomMessageManager.h"
#include "soh/Enhancements/randomizer/SeedContext.h"
#include "soh/Enhancements/randomizer/static_data.h"
#include "soh/Enhancements/randomizer/NeiGiPresentation.h"
#include "mods/transformation_masks/assets/mm_asset_loader.h"
#include "../../../../mm/2s2h/Enhancements/Gameplay/BlackMarketResources.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <spdlog/spdlog.h>
extern "C" {
#include "global.h"
u8 ResourceMgr_FileExists(const char*);
const char* SOH_ReadBlackMarketOptions();
}
#include "BlackMarketShelvesNative.h"
#include "BlackMarketInteriorEffects.h"
#include "BlackMarketKeeperAssets.h"
#include "OotBlackMarketDoorNative.h"

extern "C" void Player_Action_80845EF8(Player*, PlayState*);

namespace {
// Display-list macros redeclare these helpers at block scope in this namespace.
extern "C" void FrameInterpolation_RecordOpenChild(const void* a, int b);
extern "C" void FrameInterpolation_RecordCloseChild(void);
using namespace ComboRando::BlackMarket;
Session sSession;
nlohmann::json sPendingSave;
bool sFileLoadPending = true;
u16 sClears = 0;
bool sEntering = false, sReturnPending = false, sReturnStaged = false, sResumeFromMarket = false;
bool sDoorArmed = false, sTalking = false, sTitlePending = false;
bool sKeeperSpawned = false;
bool sCustomKeeper = false;
int sTravelSlot = -1, sSelected = 0;
float sDoorFloor = 0;
u32 sTravelSeed = 0;
Vec3f sReturnPos = { 184, 0, -300 };
s16 sReturnYaw = -16384;
u16 sReturnEntrance = ENTR_SKIEJER_FOREST;
// Movement updates only this main-thread candidate. The serialized return
// point is published once at entry, after prior native saves have finished.
Vec3f sGroundedReturnPos = { 184, 0, -300 };
s16 sGroundedReturnYaw = 0;
u16 sGroundedReturnEntrance = ENTR_SKIEJER_FOREST;
RespawnData sPreservedReturn;
std::string sStatus;
constexpr u16 kBrowseText = 0x8F10, kResultText = 0x8F11, kWelcomeText = 0x8F12, kConfirmText = 0x8F13,
              kKeeperTalkText = 0x8F15, kKeeperHelpChoiceText = 0x8F17, kKeeperHelpText = 0x8F18;
int sKeeperActorId = -1;
constexpr int kLimbCount = 24, kHeadLimb = 5;
alignas(2) constexpr char kKeeper[] = "__OTR__custom/skiejer_market/keeper/gTreasureChestShopGalSkel";
alignas(2) constexpr char kCard[] = "__OTR__custom/oot_location_titles/v1/market";
alignas(2) constexpr char kExterior[] = "__OTR__custom/skiejer_market/entrance";
struct KeeperState {
    SkelAnime skel;
    Vec3s joints[kLimbCount], morphs[kLimbCount];
    int eye, timer;
} sKeeper;
BlackMarketKeeperAssets::NativeModel sNativeKeeper;
bool sNativeKeeperReady = false;
BlackMarketShelvesNative::Fixture sShelfFixture;
constexpr Vec3f kShelves[8] = {
    // Native +/-30-degree bank planes; row changes keep one bank camera.
    { 44.84456f, 58, 98.75f },  { 44.84456f, 110, 98.75f },  { 75.15544f, 58, 116.25f },  { 75.15544f, 110, 116.25f },
    { -44.84456f, 58, 98.75f }, { -44.84456f, 110, 98.75f }, { -75.15544f, 58, 116.25f }, { -75.15544f, 110, 116.25f }
};

u32 Seed() {
    return IS_RANDO ? Rando::Context::GetInstance()->GetSeed() : 0;
}
bool CustomKeeperAvailable();
bool LoadNativeKeeper();
bool PackAvailable() {
    if (ResourceMgr_FileExists("custom/skiejer_market/native_geometry_v1")) {
        MmAssets_Init();
        if (std::all_of(std::begin(kNativeBlackMarketOotResources), std::end(kNativeBlackMarketOotResources),
                        [](const char* path) {
                            const bool native =
                                std::strncmp(path, "objects/object_bg/", 18) == 0 ||
                                std::strcmp(path, "scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORITex_00BAC0") == 0;
                            return native ? MmAssets_BaseResourceExists(path) : ResourceMgr_FileExists(path);
                        }) &&
            (CustomKeeperAvailable() || LoadNativeKeeper()))
            return true;
    }
    return std::all_of(std::begin(kBlackMarketResources), std::end(kBlackMarketResources), [](const char* path) {
        return BlackMarketEntry::LegacyResourceAvailable(path, ResourceMgr_FileExists);
    });
}
bool DestinationAvailable() {
    return std::all_of(BlackMarketEntry::kOotDestination.begin(), BlackMarketEntry::kOotDestination.end(),
                       [](const char* path) { return ResourceMgr_FileExists(path); });
}
bool Interior() {
    return gComboGameMode == COMBO_GAME_OOT && gPlayState && gPlayState->sceneNum == SCENE_SKIEJER_MARKET;
}
bool Exterior(const PlayState* play) {
    return play && play->roomCtx.curRoom.num == 0 &&
           ComboSoloForest_IsOotVisit(play->sceneNum, play->roomCtx.curRoom.num, gSaveContext.gameMode,
                                      gSaveContext.fileNum, gSaveContext.sceneLayer, gSaveContext.cutsceneIndex);
}
bool Active() {
    return Interior() && sSession.Active(gSaveContext.fileNum);
}

BlackMarketDoorCollision::Fixture<true> sDoorCollision;
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
        closed = const_cast<Actor*>(BlackMarketExitDoor::Live<true>(play));
    else if (Exterior(play)) {
        OotBlackMarketDoorNative::Pose pose;
        if (OotBlackMarketDoorNative::Find(play, pose))
            for (auto* actor = play->actorCtx.actorLists[ACTORCAT_DOOR].head; actor; actor = actor->next)
                if (OotBlackMarketDoorNative::Matches(actor, pose) && actor->update && actor->draw) {
                    closed = actor;
                    break;
                }
    }
    if (closed) {
        const auto* native = reinterpret_cast<const EnDoor*>(closed);
        if (native->actionFunc != EnDoor_Idle || native->playerIsOpening)
            closed = nullptr;
    }
    sDoorCollision.Ensure(play, closed);
}
BlackMarketGranny::Resources sGrannyResources;
bool sGrannyPending = false, sGrannyActive = false;
MeshHeader sGrannyShape{};
PolygonDlist sGrannyEntry{};
std::vector<EnvLightSettings> sGrannyLights;
void GrannySceneBegin() {
    sGrannyActive = false;
    sGrannyPending = Interior() && CVarGetInteger(BlackMarketGranny::kCvar, 0) != 0;
}
void ApplyGrannyInterior() {
    auto* play = gPlayState;
    if (!sGrannyPending || !Interior() || play->roomCtx.curRoom.num != 0 || !play->roomCtx.curRoom.meshHeader)
        return;
    sGrannyPending = false;
    if (!sGrannyResources.Prepare("oot"))
        return;
    std::array<Actor*, 2> flames{};
    size_t index = 0;
    for (const auto& actor : sGrannyResources.Decorations()) {
        auto* spawned = Actor_Spawn(&play->actorCtx, play, ACTOR_EN_LIGHT, actor.x, actor.y, actor.z, actor.rx,
                                    actor.ry, actor.rz, actor.params);
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
    play->envCtx.blendIndoorLights = 0;
    play->envCtx.unk_BD = play->envCtx.unk_BE = 0;
    std::memcpy(&play->envCtx.lightSettings, sGrannyLights.data(), sizeof(EnvLightSettings));
    sGrannyShape.polygon0 = { { 0 }, 1, &sGrannyEntry, &sGrannyEntry + 1 };
    play->roomCtx.curRoom.meshHeader = &sGrannyShape;
    sGrannyActive = true;
}
void DrawGrannyRoom() {
    ApplyGrannyInterior();
    if (!sGrannyActive || !Interior())
        return;
    auto* play = gPlayState;
    auto* frame = static_cast<Gfx*>(Graph_Alloc(play->state.gfxCtx, 12 * sizeof(Gfx)));
    const auto tick = play->gameplayFrames;
    BlackMarketGranny::Wrap(frame, frame + 5, sGrannyResources.Opaque(), sGrannyResources.Translucent(),
                            Gfx_TexScrollEx(play->state.gfxCtx, 0, (tick * 3) % 128, 32, 32, 0, 3),
                            Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, 0, 1023 - (tick * 3) % 1024, 16, 256, 1, 0,
                                               1023 - (tick * 6) % 1024, 16, 256, 0, -3, 0, -6),
                            sGrannyResources.Route());
    sGrannyEntry = { frame, frame + 5 };
}
BlackMarketRoomMagic::Resources sRoomMagicResources;
MeshHeader sRoomMagicShape{};
PolygonDlist sRoomMagicEntry{};
MeshHeader* sRoomMagicOriginal = nullptr;
PlayState* sRoomMagicPlay = nullptr;
void RestoreRoomMagic() {
    // Restore only the live room we attached. A scene load may have reused the
    // PlayState address or installed another shape since the previous draw.
    auto* play = gPlayState;
    if (play && play == sRoomMagicPlay && play->roomCtx.curRoom.meshHeader == &sRoomMagicShape)
        play->roomCtx.curRoom.meshHeader = sRoomMagicOriginal;
    sRoomMagicPlay = nullptr;
    sRoomMagicOriginal = nullptr;
    sRoomMagicEntry = {};
}
void ClearRoomMagic() {
    RestoreRoomMagic();
    sRoomMagicResources.Clear();
}
void BeginRoomMagic() {
    RestoreRoomMagic();
    auto* play = gPlayState;
    if (!Interior() || sGrannyActive || play->roomCtx.curRoom.num != 0 ||
        !CVarGetInteger(BlackMarketRoomMagic::kCvar, 0))
        return;
    auto* shape = play->roomCtx.curRoom.meshHeader;
    if (!shape || shape->base.type != 0 || shape->polygon0.num != 1 || !shape->polygon0.start ||
        !BlackMarketRoomMagic::MatchesSource(static_cast<PolygonDlist*>(shape->polygon0.start)->opa, true) ||
        !sRoomMagicResources.Prepare("oot", static_cast<PolygonDlist*>(shape->polygon0.start)->opa))
        return;
    sRoomMagicResources.SetPalette(BlackMarketRoomMagic::ReadPalette(true));
    sRoomMagicEntry = *static_cast<PolygonDlist*>(shape->polygon0.start);
    sRoomMagicEntry.opa = sRoomMagicResources.Opaque();
    sRoomMagicShape = *shape;
    sRoomMagicShape.polygon0.start = &sRoomMagicEntry;
    sRoomMagicShape.polygon0.end = &sRoomMagicEntry + 1;
    sRoomMagicOriginal = shape;
    sRoomMagicPlay = play;
    play->roomCtx.curRoom.meshHeader = &sRoomMagicShape;
}
bool Safe(PlayState* play) {
    return play && play->state.running && gSaveContext.gameMode == GAMEMODE_NORMAL &&
           ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) && play->transitionTrigger == TRANS_TRIGGER_OFF &&
           play->transitionMode == TRANS_MODE_OFF && play->pauseCtx.state == 0 &&
           play->gameOverCtx.state == GAMEOVER_INACTIVE && play->csCtx.state == CS_STATE_IDLE &&
           !gSaveContext.cutsceneTrigger;
}
void RememberTravel() {
    sTravelSlot = gSaveContext.fileNum;
    sTravelSeed = Seed();
}
bool MatchesTravel() {
    return sTravelSlot == gSaveContext.fileNum && sTravelSeed == Seed();
}
void StageReturn() {
    if (sReturnStaged)
        return;
    auto& arrival = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    sPreservedReturn = arrival;
    sReturnStaged = true;
    arrival = {};
    arrival.entranceIndex = sReturnEntrance;
    arrival.roomIndex = 0;
    arrival.pos = sReturnPos;
    arrival.yaw = sReturnYaw;
    arrival.playerParams = (PLAYER_START_MODE_IDLE << 8) | 0xFF;
    gSaveContext.respawnFlag = 2;
}
void Transition(PlayState* play, u16 entrance) {
    Combo_SOH_RequestCrossGameTitleCard();
    // Direct native routes never call the entrance randomizer or the launcher.
    play->nextEntranceIndex = entrance;
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
                       gSaveContext.rupees + std::min<int>(0, gSaveContext.rupeeAccumulator), ClearCount(sClears),
                       ComboBlackMarketOotItems::Owned(*item), ComboBlackMarketOotItems::Available(*item));
}
BlackMarketReceipt::State sReceipt;
nlohmann::json sReceiptMarket;
GetItemEntry sReceiptEntry{};
bool ReceiptContext() {
    return Active() && gPlayState && gPlayState->state.running &&
           sReceipt.Matches(gSaveContext.fileNum, Seed(), sReceipt.ticket) && sReceiptMarket == sSession.market &&
           gPlayState->transitionTrigger == TRANS_TRIGGER_OFF && gPlayState->transitionMode == TRANS_MODE_OFF &&
           gPlayState->gameOverCtx.state == GAMEOVER_INACTIVE;
}
void CancelReceipt(bool destroying = false) {
    // Withdraw only the entry issued by this transaction. Native item actions
    // already test GI_NONE/OBJECT_INVALID and finish through normal cleanup.
    if (!destroying && gPlayState && !sReceipt.delivered) {
        auto* player = GET_PLAYER(gPlayState);
        if (player && sReceipt.pending && player->getItemEntry.comboMarketReceipt == sReceipt.ticket) {
            player->getItemEntry = GET_ITEM_NONE;
            player->getItemId = GI_NONE;
            if (player->interactRangeActor == &player->actor)
                player->interactRangeActor = nullptr;
        }
    }
    sReceipt.Cancel();
    sReceiptMarket = {};
    sReceiptEntry = {};
}
bool PendingReceiptEntry(PlayState* play, const GetItemEntry& entry) {
    const auto* player = play == gPlayState && play ? GET_PLAYER(play) : nullptr;
    return ReceiptContext() && sReceipt.offered && !sReceipt.delivered && player &&
           (player->stateFlags1 & PLAYER_STATE1_GETTING_ITEM) && entry.comboMarketReceipt == sReceipt.ticket &&
           player->getItemEntry.comboMarketReceipt == sReceipt.ticket && entry.modIndex == sReceiptEntry.modIndex &&
           entry.getItemId == sReceiptEntry.getItemId && player->getItemEntry.modIndex == entry.modIndex &&
           player->getItemEntry.getItemId == entry.getItemId;
}
bool FixedMarketGrant(PlayState* play, const GetItemEntry& entry) {
    if (!entry.comboMarketReceipt)
        return false; // Every ordinary native entry remains untouched.
    if (!PendingReceiptEntry(play, entry))
        return true; // A cancelled/stale market offer must never fall through.
    const auto* item = Offer(sReceipt.slot);
    if (!item)
        return true;
    SaveManager::Instance->ThreadPoolWait();
    // Preserve physically owned tools before any native shared-slot write.
    ComboBlackMarketOotItems::LatchToolOwnership();
    const auto key = std::string_view(item->key);
    // Native acquisition still owns the presentation and OnItemReceive. These
    // few catalogue rows need their exact shop grant instead of native
    // progressive cane/feather or incomplete bundle behavior.
    if ((key.starts_with("nei.cane.") && key != "nei.cane.byrna") || key.starts_with("nei.bundle.") ||
        key == "nei.rocs_feather") {
        ComboBlackMarketOotItems::Grant(*item);
        return true;
    }
    return false;
}
void ReceiveMarketItem(GetItemEntry entry) {
    if (!PendingReceiptEntry(gPlayState, entry))
        return;
    // Latch a newly granted tool before ownership checks and the atomic save.
    ComboBlackMarketOotItems::LatchToolOwnership();
    const auto* item = Offer(sReceipt.slot);
    if (!item || !ComboBlackMarketOotItems::Owned(*item))
        return;
    SaveManager::Instance->ThreadPoolWait();
    if (!sReceipt.Acknowledge(gSaveContext.fileNum, Seed(), sReceipt.ticket))
        return;
    gSaveContext.rupees -= sReceipt.price;
    CommitPurchase(sSession.state, sReceipt.slot);
    SaveManager::Instance->SaveSection(gSaveContext.fileNum, SECTION_ID_BASE, false);
}
bool Purchase() {
    if (Status(sSelected) != PurchaseResult::Ready) {
        switch (Status(sSelected)) {
            case PurchaseResult::SoldOut:
                sStatus = "That one has already been sold.";
                break;
            case PurchaseResult::Owned:
                sStatus = "You already own that power.";
                break;
            case PurchaseResult::Locked:
                sStatus = "The seal needs more unique dungeon clears.";
                break;
            case PurchaseResult::NeedRupees:
                sStatus = "You need more rupees.";
                break;
            default:
                sStatus = "You need its prerequisite first.";
                break;
        }
        return false;
    }
    const auto* item = Offer(sSelected);
    const auto entry =
        item ? Rando::StaticData::RetrieveItem(ComboBlackMarketOotItems::Id(*item)).GetGIEntryUnresolved() : nullptr;
    if (!entry || entry->modIndex != MOD_RANDOMIZER ||
        !sReceipt.Begin(sSelected, gSaveContext.fileNum, Seed(),
                        sSession.market["stock"][sSelected]["price"].get<int>())) {
        sStatus = "That offer cannot be delivered yet.";
        return false;
    }
    sReceiptMarket = sSession.market;
    sReceiptEntry = *entry; // exact GI row, never GetGIEntry()/GetFinalGIEntry()
    sReceiptEntry.comboMarketReceipt = sReceipt.ticket;
    return true;
}

#define BLACK_MARKET_BROWSING_OOT
#include "BlackMarketBrowsingNative.h"
#undef BLACK_MARKET_BROWSING_OOT
void MonitorReceipt() {
    if (!sReceipt.pending)
        return;
    if (!ReceiptContext()) {
        CancelReceipt();
        return;
    }
    auto* play = gPlayState;
    auto* player = GET_PLAYER(play);
    if (!player || play->msgCtx.msgMode != 0 || Player_InBlockingCsMode(play, player) ||
        (player->stateFlags1 & (PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_IN_ITEM_CS)) || !Safe(play))
        return;
    if (!sReceipt.delivered) {
        if (Status(sReceipt.slot) != PurchaseResult::Ready) {
            CancelReceipt();
            return;
        }
        sReceipt.offered = GiveItemEntryWithoutActor(play, sReceiptEntry) != 0;
        return;
    }
    if (Play_GetActiveCamId(play) != CAM_ID_MAIN)
        return;
    const int shelf = sReceipt.slot;
    CancelReceipt();
    if (BeginBrowsing(play)) {
        sSelected = sBrowser.slot = shelf;
        Message_StartTextbox(play, kBrowseText, nullptr);
    }
}

bool CustomKeeperAvailable() {
    return BlackMarketKeeperAssets::CustomAvailable(kBlackMarketResources, [](const char* path) {
        return ResourceMgr_FileExists(path) || (ResourceMgr_IsAltAssetsEnabled() && ResourceMgr_FileAltExists(path));
    });
}
bool LoadNativeKeeper() {
    if (!sNativeKeeperReady) {
        // The archive-scoped importers pin this private model for renderer use.
        // Keep a successful preflight alive and retry only incomplete imports.
        sNativeKeeperReady = sNativeKeeper.Load(MmAssets_LoadSkeleton, MmAssets_LoadAnimation,
                                                MmAssets_LoadResourceStrict, MmAssets_LoadDisplayListGraphStrict);
    }
    return sNativeKeeperReady;
}

void KeeperUpdate(Actor* actor, PlayState* play) {
    sBrowseKeeper = actor;
    SkelAnime_Update(&sKeeper.skel);
    if (sKeeper.timer > 0)
        --sKeeper.timer;
    if (!sKeeper.timer) {
        sKeeper.eye = (sKeeper.eye + 1) % 4;
        sKeeper.timer = sKeeper.eye ? 2 : 80 + play->gameplayFrames % 53;
    }
    Actor_SetFocus(actor, 55);
    if (!Active()) {
        EndBrowsing(play);
        actor->textId = BlackMarketEntry::kUnavailableText;
        if (Actor_ProcessTalkRequest(actor, play))
            Message_ContinueTextbox(play, BlackMarketEntry::kUnavailableText);
        else
            Actor_OfferTalk(actor, play, 150);
        return;
    }
    if (sReceipt.pending)
        return;
    if (Actor_ProcessTalkRequest(actor, play)) {
        if (!BeginBrowsing(play)) {
            Message_CloseTextbox(play);
            return;
        }
        Message_ContinueTextbox(play, kWelcomeText);
    } else if (!sTalking) {
        actor->textId = kWelcomeText;
        Actor_OfferTalk(actor, play, 150);
    }
    if (!sTalking)
        return;
    BrowseInput(play);
}
s32 KeeperLimb(PlayState*, s32 limb, Gfx** display, Vec3f*, Vec3s*, void*) {
    if (sCustomKeeper && limb == kHeadLimb) {
        static const char* heads[] = { "__OTR__custom/skiejer_market/keeper/ShopGalMMDBlinkHead0DL",
                                       "__OTR__custom/skiejer_market/keeper/ShopGalMMDBlinkHead1DL",
                                       "__OTR__custom/skiejer_market/keeper/ShopGalMMDBlinkHead2DL",
                                       "__OTR__custom/skiejer_market/keeper/ShopGalMMDBlinkHead1DL" };
        *display = (Gfx*)heads[sKeeper.eye];
    }
    return false;
}
void KeeperDraw(Actor* actor, PlayState* play) {
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    if (!sCustomKeeper) {
        MmAssets_EnsureStrictTextureBindings();
        constexpr int eyes[] = { 0, 1, 2, 1 };
        gSPSegment(POLY_OPA_DISP++, 0x08, reinterpret_cast<uintptr_t>(sNativeKeeper.eyes[eyes[sKeeper.eye & 3]]));
    }
    SkelAnime_DrawFlexOpa(play, sKeeper.skel.skeleton, sKeeper.skel.jointTable, sKeeper.skel.dListCount, KeeperLimb,
                          nullptr, actor);
    CLOSE_DISPS(play->state.gfxCtx);
}
void KeeperInit(Actor* actor, PlayState* play) {
    if (!Interior() || !PackAvailable()) {
        Actor_Kill(actor);
        return;
    }
    sKeeper = {};
    sCustomKeeper = CustomKeeperAvailable();
    if (!sCustomKeeper && !LoadNativeKeeper()) {
        Actor_Kill(actor);
        return;
    }
    ActorShape_Init(&actor->shape, 0, nullptr, 0);
    Actor_SetScale(actor, BlackMarketKeeper::kScale);
    actor->flags = ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_FRIENDLY | ACTOR_FLAG_UPDATE_CULLING_DISABLED;
    SkelAnime_InitFlex(play, &sKeeper.skel, sCustomKeeper ? (FlexSkeletonHeader*)kKeeper : &sNativeKeeper.skeleton,
                       sCustomKeeper ? (AnimationHeader*)BlackMarketKeeper::kAnimation : sNativeKeeper.animation,
                       sKeeper.joints, sKeeper.morphs, kLimbCount);
    actor->update = KeeperUpdate;
    actor->draw = KeeperDraw;
    actor->destroy = nullptr;
    sKeeper.timer = 90;
}
void RegisterKeeperActor() {
    if (sKeeperActorId >= 0)
        return;
    ActorDBInit profile;
    profile.name = "En_SkiejerMarketKeeper";
    profile.desc = "Skiejer's Black Market Goth Shop Girl";
    profile.category = ACTORCAT_NPC;
    profile.flags = ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_FRIENDLY | ACTOR_FLAG_UPDATE_CULLING_DISABLED |
                    ACTOR_FLAG_DRAW_CULLING_DISABLED;
    profile.objectId = OBJECT_GAMEPLAY_KEEP;
    profile.instanceSize = sizeof(Actor);
    profile.init = KeeperInit;
    profile.update = KeeperUpdate;
    profile.draw = KeeperDraw;
    sKeeperActorId = ActorDB::Instance->AddEntry(profile).entry.id;
}
void MarketText(u16* textId, bool* fromTable) {
    using BlackMarketText::Page;
    if (Interior() && *textId == BlackMarketEntry::kUnavailableText) {
        CustomMessage entry(BlackMarketText::Oot(Page::Unavailable, {}, {}, gSaveContext.language));
        entry.AutoFormat();
        entry.LoadIntoFont();
        *fromTable = false;
        return;
    }
    if (!Active() ||
        (*textId != kBrowseText && *textId != kResultText && *textId != kWelcomeText && *textId != kConfirmText &&
         *textId != kKeeperTalkText && *textId != kKeeperHelpChoiceText && *textId != kKeeperHelpText))
        return;
    const Page page = *textId == kKeeperTalkText         ? Page::KeeperTalk
                      : *textId == kKeeperHelpChoiceText ? Page::KeeperHelpChoice
                      : *textId == kKeeperHelpText       ? Page::KeeperHelp
                      : *textId == kWelcomeText          ? Page::Welcome
                      : *textId == kConfirmText          ? Page::Confirm
                      : *textId == kResultText           ? Page::Result
                                                         : Page::Browse;
    BlackMarketText::Offer offer;
    if (page == Page::Browse || page == Page::Confirm || page == Page::Result) {
        offer.item = Offer(sSelected);
        if (!offer.item && page != Page::Result)
            return;
        if (offer.item) {
            offer.name = offer.item->label;
            // Use the same fixed item ID as delivery; progressive resolution
            // would advertise a different power than this saved offer grants.
            const auto id = ComboBlackMarketOotItems::Id(*offer.item);
            if (id != RG_NONE) {
                const auto& metadata = Rando::StaticData::RetrieveItem(id);
                const auto& name = metadata.GetName().GetForLanguage(gSaveContext.language);
                if (!name.empty())
                    offer.name = name;
                if (!metadata.GetColor().empty())
                    offer.color = metadata.GetColor();
            }
            const auto& stock = sSession.market["stock"][sSelected];
            offer.price = stock["price"].get<int>();
            offer.clears = stock["clears"].get<int>();
            offer.slot = sSelected;
            if (page == Page::Browse)
                offer.status = Status(sSelected);
        }
    }
    CustomMessage entry(BlackMarketText::Oot(page, offer, sStatus, gSaveContext.language));
    entry.AutoFormat();
    entry = CustomMessage(BlackMarketText::OotPages(entry.GetForCurrentLanguage(MF_RAW)));
    if (page == Page::Browse || page == Page::Confirm || page == Page::Result)
        entry.SetTextBoxPosition(TEXTBOX_POS_BOTTOM);
    entry.LoadIntoFont();
    *fromTable = false;
}
void DrawSeal(PlayState* play, const BlackMarketVisual::Frame& frame, bool selected) {
    using namespace BlackMarketVisual;
    const auto mesh = BuildLock(frame, play->gameplayFrames, selected);
    if (!mesh.valid ||
        !BlackMarketGraphicsArena::HasRoom(play->state.gfxCtx, kLockVertices * sizeof(Vtx) + sizeof(Mtx), 8, 64))
        return;
    auto* vertices = (Vtx*)Graph_Alloc(play->state.gfxCtx, kLockVertices * sizeof(Vtx));
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
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    Matrix_Push();
    Matrix_Translate(mesh.center.x, mesh.center.y, mesh.center.z, MTXMODE_NEW);
    Matrix_Scale(1.f / kLockVertexScale, 1.f / kLockVertexScale, 1.f / kLockVertexScale, MTXMODE_APPLY);
    gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
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
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    CLOSE_DISPS(play->state.gfxCtx);
}

void PresentTitle() {
    if (!sTitlePending || !Interior() || !Safe(gPlayState) || Player_InCsMode(gPlayState))
        return;
    if (!ResourceMgr_FileExists(kCard) && !(ResourceMgr_IsAltAssetsEnabled() && ResourceMgr_FileAltExists(kCard))) {
        sTitlePending = false;
        return;
    }
    // TitleCard_InitPlaceName rejects new scene IDs and rewrites language
    // suffixes. Initialize its ordinary native context with our private card;
    // native timing, HUD cosmetics and OnPresentTitleCard still own display.
    auto& card = gPlayState->actorCtx.titleCtx;
    if (card.durationTimer || card.delayTimer || card.alpha)
        return;
    struct PresentationScope {
        int previous = Combo_SOH_SwapCustomTitleCardPresentation(true);
        ~PresentationScope() {
            Combo_SOH_SwapCustomTitleCardPresentation(previous);
        }
    } scope;
    if (!GameInteractor_Should(VB_SHOW_TITLE_CARD, true)) {
        sTitlePending = false;
        return;
    }
    card = {};
    card.texture = (void*)kCard;
    card.isBossCard = false;
    card.hasTranslation = false;
    card.x = 160;
    card.y = 120;
    card.width = 240;
    card.height = 24;
    card.durationTimer = 80;
    card.delayTimer = 20;
    sTitlePending = false;
}

void DrawOffer(PlayState* play, const Item& item) {
    const auto entry = Rando::StaticData::RetrieveItem(ComboBlackMarketOotItems::Id(item)).GetGIEntryUnresolved();
    if (entry && !NeiGi_DrawShop(play, entry.get()))
        GetItemEntry_Draw(play, *entry);
}

void DrawShelves(PlayState* play) {
    if (!sShelfFixture.Ensure(play))
        return;
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    Matrix_Push();
    Matrix_Translate(0, 0, 0, MTXMODE_NEW);
    gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    const auto color =
        CVarGetColor("gCosmetics.World.BlackMarketShelves.Value",
                     Color_RGBA8{ BlackMarketShelves::kDefaultTint[0], BlackMarketShelves::kDefaultTint[1],
                                  BlackMarketShelves::kDefaultTint[2], 255 });
    sShelfFixture.Draw(POLY_OPA_DISP, sGrannyActive, sGrannyResources.ShelfTextures(),
                       BlackMarketShelves::Tint(CVarGetInteger("gCosmetics.World.BlackMarketShelves.Changed", 0) != 0,
                                                { color.r, color.g, color.b, 255 }));
    Matrix_Pop();
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    CLOSE_DISPS(play->state.gfxCtx);
}
void DrawMarket() {
    auto* play = gPlayState;
    if (!Interior() || !play->state.running)
        return;
    DrawShelves(play);
    BlackMarketInteriorEffects::Draw(play, !sGrannyActive, true);
    if (!Active())
        return;
    for (int slot = 0; slot < 8; ++slot) {
        const auto* item = Offer(slot);
        if (!item)
            continue;
        const auto status = Status(slot);
        if (status == PurchaseResult::SoldOut && !item->renewable)
            continue;
        const bool locked = status == PurchaseResult::Locked;
        BlackMarketVisual::Capture capture(locked);
        Matrix_Push();
        const auto pos = kShelves[slot];
        Matrix_Translate(pos.x, pos.y + 2 * std::sin((play->gameplayFrames + slot * 9) * .035f), pos.z, MTXMODE_NEW);
        Matrix_RotateY(play->gameplayFrames * .018f, MTXMODE_APPLY);
        const float scale = sTalking && slot == sSelected ? .32f : .25f;
        Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
        DrawOffer(play, *item);
        Matrix_Pop();
        if (locked && CVarGetInteger(BlackMarketVisual::kSealsCvar, 1))
            DrawSeal(play, capture.frame, sTalking && slot == sSelected);
    }
}
void DrawExterior() {
    auto* play = gPlayState;
    if (!Exterior(play) || !PackAvailable())
        return;
    if (OotBlackMarketDoorNative::HasDoor(play))
        return; // The finished forest supplies the building and native door.
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    Matrix_Push();
    Matrix_Translate(COMBO_BLACK_MARKET_DOOR_X, sDoorFloor, COMBO_BLACK_MARKET_DOOR_Z, MTXMODE_NEW);
    Matrix_RotateY(COMBO_BLACK_MARKET_DOOR_YAW * (6.28318530718f / 65536.0f), MTXMODE_APPLY);
    gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)kExterior);
    Matrix_Pop();
    CLOSE_DISPS(play->state.gfxCtx);
}
void PlayerUpdate() {
    auto* play = gPlayState;
    if (gComboGameMode != COMBO_GAME_OOT || !Safe(play) || Player_InCsMode(play))
        return;
    auto* player = GET_PLAYER(play);
    if (!player)
        return;
    if (sReturnStaged) {
        gSaveContext.respawn[RESPAWN_MODE_RETURN] = sPreservedReturn;
        sReturnStaged = false;
    }
    if (sReturnPending && MatchesTravel() && Exterior(play)) {
        sReturnPending = false;
        OotBlackMarketDoorNative::Pose placed;
        const bool authoredDoor = OotBlackMarketDoorNative::Find(play, placed);
        Vec3f pos = authoredDoor ? OotBlackMarketDoorNative::Outside(placed) : sReturnPos;
        const s16 yaw = authoredDoor ? placed.yaw : sReturnYaw;
        pos.y += 120;
        CollisionPoly* floor = nullptr;
        s32 bg = BGCHECK_SCENE;
        const auto y = BgCheck_EntityRaycastFloor5(play, &play->colCtx, &floor, &bg, &player->actor, &pos);
        if (y != BGCHECK_Y_MIN && floor && floor->normal.y >= 23170) {
            pos.y = y;
            player->actor.world.pos = player->actor.prevPos = player->actor.home.pos = pos;
            player->actor.world.rot.y = player->actor.shape.rot.y = player->yaw = yaw;
            player->actor.velocity = {};
            player->actor.speedXZ = 0;
            player->actor.floorHeight = y;
            player->actor.floorPoly = floor;
            player->actor.floorBgId = bg;
            if (authoredDoor)
                Camera_InitPlayerSettings(&play->mainCamera, player);
        }
        sDoorArmed = false;
        return;
    }
    if (Interior()) {
        // Empty custom rooms do not fire OnSceneSpawnActors in SoH. Wait for
        // the native player and object context, then create the one keeper.
        if (!sKeeperSpawned && PackAvailable())
            sKeeperSpawned =
                Actor_Spawn(&play->actorCtx, play, sKeeperActorId, BlackMarketKeeper::kX, BlackMarketKeeper::kY,
                            BlackMarketKeeper::kZ, 0, BlackMarketKeeper::kYaw, 0, 0) != nullptr;
        if (BlackMarketExitDoor::WalkExit<true>(play, player->actor.world.pos)) {
            RememberTravel();
            sReturnPending = true;
            Transition(play, sReturnEntrance);
        }
        return;
    }
    if (!Exterior(play) || !DestinationAvailable())
        return;
    if (OotBlackMarketDoorNative::HasDoor(play))
        return; // Authored Open owns entry, including ambiguous/unavailable markers.
    const auto pos = player->actor.world.pos;
    Vec3f probe = { COMBO_BLACK_MARKET_DOOR_X, 120, COMBO_BLACK_MARKET_DOOR_Z };
    CollisionPoly* floor = nullptr;
    s32 bg = BGCHECK_SCENE;
    const float height = BgCheck_EntityRaycastFloor5(play, &play->colCtx, &floor, &bg, &player->actor, &probe);
    if (height == BGCHECK_Y_MIN || !floor || floor->normal.y < 23170)
        return;
    sDoorFloor = height;
    const float dx = pos.x - COMBO_BLACK_MARKET_DOOR_X, dz = pos.z - COMBO_BLACK_MARKET_DOOR_Z;
    if (dx * dx + dz * dz > 14400) {
        sDoorArmed = true;
        if (player->actor.floorPoly && player->actor.floorPoly->normal.y >= 23170 &&
            std::isfinite(player->actor.floorHeight) && std::abs(pos.y - player->actor.floorHeight) < 35.0f) {
            sGroundedReturnPos = pos;
            sGroundedReturnYaw = player->actor.shape.rot.y;
            sGroundedReturnEntrance = gSaveContext.entranceIndex;
        }
    }
    if (sDoorArmed && ComboBlackMarket_InDoor(pos.x, pos.y, pos.z, height)) {
        SaveManager::Instance->ThreadPoolWait();
        sReturnPos = sGroundedReturnPos;
        sReturnYaw = sGroundedReturnYaw;
        sReturnEntrance = sGroundedReturnEntrance;
        sDoorArmed = false;
        sEntering = true;
        RememberTravel();
        Transition(play, ENTR_SKIEJER_MARKET);
    }
}
void BindSolo(int slot) {
    CancelReceipt();
    if (gComboGameMode != COMBO_GAME_OOT || !ComboLostWoods_IsPlayableSlot(slot))
        return;
    if (!sFileLoadPending && sSession.Active(slot) && sSession.market["seed"] == Seed())
        return;
    sFileLoadPending = false;
    sResumeFromMarket = false;
    const bool create = sPendingSave.is_null();
    auto market = sPendingSave.is_object() ? sPendingSave.value("market", nlohmann::json()) : nlohmann::json();
    if (sPendingSave.is_null()) {
        auto options = OptionsFromJson(nlohmann::json::parse(SOH_ReadBlackMarketOptions()));
        options.games = Games::Oot;
        market = Generate(Seed(), options);
    }
    if (!Validate(market) || !MatchesGames(market, Games::Oot) || market["seed"] != Seed()) {
        sSession.Bind(nullptr, -1);
        sClears = 0;
        return;
    }
    sSession.Bind(market, slot);
    sSession.Load(sPendingSave, slot);
    const auto clears = sPendingSave.is_object() ? sPendingSave.value("clears", nlohmann::json(0)) : nlohmann::json();
    sClears = UnsignedInRange(clears, 0xFF) ? clears.get<u16>() : 0;
    sReturnEntrance = ENTR_SKIEJER_FOREST;
    sReturnPos = { 184, 0, -300 };
    sReturnYaw = -16384;
    const auto point = sPendingSave.is_object() ? sPendingSave.value("return", nlohmann::json()) : nlohmann::json();
    if (point.is_object())
        try {
            const int entrance = point.at("entrance").get<int>(), yaw = point.at("yaw").get<int>();
            const Vec3f pos = { point.at("x").get<float>(), point.at("y").get<float>(), point.at("z").get<float>() };
            if (entrance >= 0 && entrance < ENTR_MAX && gEntranceTable[entrance].scene == SCENE_SKIEJER_FOREST &&
                std::isfinite(pos.x) && std::isfinite(pos.y) && std::isfinite(pos.z) && std::abs(pos.x) < 4096 &&
                std::abs(pos.y) < 4096 && std::abs(pos.z) < 4096 && yaw >= INT16_MIN && yaw <= INT16_MAX) {
                sReturnEntrance = entrance;
                sReturnPos = pos;
                sReturnYaw = yaw;
            }
        } catch (const nlohmann::json::exception&) {}
    sGroundedReturnPos = sReturnPos;
    sGroundedReturnYaw = sReturnYaw;
    sGroundedReturnEntrance = sReturnEntrance;
    sResumeFromMarket = sPendingSave.is_object() && sPendingSave.value("savedInside", nlohmann::json()) == true;
    if (sResumeFromMarket) {
        // OnLoadGame is unordered; native entrance-rando hooks may still
        // choose a savewarp. Apply our local recovery only at Play_Init.
        RememberTravel();
    }
    if (create) {
        SaveManager::Instance->ThreadPoolWait();
        SaveManager::Instance->SaveSection(slot, SECTION_ID_BASE, false);
    }
}
void SaveMarket(const SaveContext& snapshot, int, bool) {
    if (gComboGameMode != COMBO_GAME_OOT || !sSession.Active(snapshot.fileNum))
        return;
    auto value = sSession.Save(snapshot.fileNum);
    value["clears"] = sClears;
    value["savedInside"] =
        snapshot.entranceIndex >= ENTR_SKIEJER_MARKET && snapshot.entranceIndex <= ENTR_SKIEJER_MARKET_3;
    value["return"] = { { "entrance", sReturnEntrance },
                        { "x", sReturnPos.x },
                        { "y", sReturnPos.y },
                        { "z", sReturnPos.z },
                        { "yaw", sReturnYaw } };
    SaveManager::Instance->SaveData("native", value);
}
void RegisterNativeMarket() {
    RegisterKeeperActor();
    SaveManager::Instance->AddInitFunction([](bool) {
        sPendingSave = nullptr;
        sSession = {};
        sClears = 0;
        sFileLoadPending = true;
        sEntering = sReturnPending = sReturnStaged = sResumeFromMarket = false;
    });
    SaveManager::Instance->AddLoadFunction("blackMarket", 1, [] {
        SaveManager::Instance->LoadData("native", sPendingSave);
        sFileLoadPending = true;
    });
    SaveManager::Instance->AddSaveFunction("blackMarket", 1, SaveMarket, true, SECTION_PARENT_NONE);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>(BindSolo);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](s16 scene) {
        ClearRoomMagic();
        sShelfFixture.Reset();
        sDoorCollision.Reset();
        EndBrowsing(gPlayState, true);
        InitBrowsingCameraScene(nullptr);
        CancelReceipt(true);
        GrannySceneBegin();
        sEntering = sDoorArmed = sTalking = false;
        sKeeperSpawned = false;
        sTitlePending = Interior() && PackAvailable();
        if (scene == SCENE_SKIEJER_MARKET)
            sSelected = 0;
    });
    Ship::CrossRMRegistry::RegisterTeardownListener([] {
        ClearRoomMagic();
        sGrannyResources.Clear();
    });
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnActorUpdate>(
        ACTOR_EN_DOOR, [](void*) { SyncShopDoorCollision(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>(SyncShopDoorCollision);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDrawBegin>([] {
        DrawGrannyRoom();
        BeginRoomMagic();
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(ApplyGrannyInterior);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(PlayerUpdate);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnOpenText>(MarketText);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnItemReceive>(ReceiveMarketItem);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>([] {
        MonitorBrowsing();
        MonitorReceipt();
        PresentTitle();
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDrawEnd>([] {
        RestoreRoomMagic();
        DrawMarket();
        DrawBrowseCursor();
        DrawExterior();
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>([] {
        ClearRoomMagic();
        sShelfFixture.Reset();
        sDoorCollision.Reset();
        EndBrowsing(gPlayState, true);
        CancelReceipt();
        sTitlePending = sTalking = sDoorArmed = sGrannyActive = sGrannyPending = false;
    });
}
RegisterShipInitFunc init(RegisterNativeMarket);
} // namespace

extern "C" void ComboBlackMarketOot_RestoreRoomAfterDraw(PlayState* play) {
    if (play && play == gPlayState && play == sRoomMagicPlay)
        RestoreRoomMagic();
}

extern "C" bool ComboBlackMarketOot_TryFixedReceiptGrant(PlayState* play, GetItemEntry entry) {
    return FixedMarketGrant(play, entry);
}

namespace {
void LogDoorAttempt(PlayState* play, Actor* door, int result, const char* reason) {
    if (!play || play->sceneNum != SCENE_SKIEJER_FOREST)
        return;
    OotBlackMarketDoorNative::Pose placed;
    const bool found = OotBlackMarketDoorNative::Find(play, placed);
    const auto* player = GET_PLAYER(play);
    const auto missing = BlackMarketEntry::Missing(BlackMarketEntry::kOotDestination, ResourceMgr_FileExists);
    nlohmann::json record = {
        { "host", "oot" },
        { "result", BlackMarketEntry::ResultName(result) },
        { "reason", reason },
        { "location",
          { { "mode", gComboGameMode },
            { "scene", play->sceneNum },
            { "room", play->roomCtx.curRoom.num },
            { "layer", gSaveContext.sceneLayer },
            { "entrance", gSaveContext.entranceIndex },
            { "cutscene_index", gSaveContext.cutsceneIndex } } },
        { "recognition",
          { { "current_play", play == gPlayState },
            { "exterior", Exterior(play) },
            { "marker_actor", OotBlackMarketDoorNative::MarkerActor(play, door) },
            { "marker_found", found },
            { "marker_matches", found && OotBlackMarketDoorNative::Matches(door, placed) },
            { "transition_actors", play->transiActorCtx.numActors },
            { "door_id", door ? door->id : -1 },
            { "door_params", door ? door->params : -1 } } },
        { "gates",
          { { "transition_idle",
              play->transitionTrigger == TRANS_TRIGGER_OFF && play->transitionMode == TRANS_MODE_OFF },
            { "native_state_ready", found && Safe(play) && player && player->doorActor == door &&
                                        player->doorType == PLAYER_DOORTYPE_HANDLE &&
                                        player->actionFunc == Player_Action_80845EF8 && gSaveContext.health > 0 },
            { "destination_ready", missing.empty() } } },
        { "state",
          { { "running", play->state.running },
            { "game_mode", gSaveContext.gameMode },
            { "file", gSaveContext.fileNum },
            { "transition_trigger", play->transitionTrigger },
            { "transition_mode", play->transitionMode },
            { "pause", play->pauseCtx.state },
            { "game_over", play->gameOverCtx.state },
            { "scene_cutscene", play->csCtx.state },
            { "cutscene_trigger", gSaveContext.cutsceneTrigger },
            { "player_present", player != nullptr },
            { "player_owns_door", player && player->doorActor == door },
            { "player_action_expected", player && player->actionFunc == Player_Action_80845EF8 },
            { "player_door_type", player ? player->doorType : -1 },
            { "health", gSaveContext.health },
            { "native_keeper_ready", sNativeKeeperReady } } },
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
    record["checks"] = { { "door_recognized",
                           play == gPlayState && Exterior(play) && OotBlackMarketDoorNative::MarkerActor(play, door) },
                         { "unique_marker", found },
                         { "running", play->state.running != 0 },
                         { "normal_mode", gSaveContext.gameMode == GAMEMODE_NORMAL },
                         { "playable_slot", ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) != 0 },
                         { "transition_idle",
                           play->transitionTrigger == TRANS_TRIGGER_OFF && play->transitionMode == TRANS_MODE_OFF },
                         { "pause_closed", play->pauseCtx.state == 0 },
                         { "game_over_inactive", play->gameOverCtx.state == GAMEOVER_INACTIVE },
                         { "scene_cutscene_idle", play->csCtx.state == CS_STATE_IDLE },
                         { "cutscene_not_pending", !gSaveContext.cutsceneTrigger },
                         { "player_present", player != nullptr },
                         { "player_owns_door", player && player->doorActor == door },
                         { "door_type_handle", player && player->doorType == PLAYER_DOORTYPE_HANDLE },
                         { "player_action_expected", player && player->actionFunc == Player_Action_80845EF8 },
                         { "alive", gSaveContext.health > 0 },
                         { "destination_ready", missing.empty() } };
    record["failed_checks"] = BlackMarketEntry::FailedChecks(record["checks"]);
    if (!missing.empty()) {
        record["resources"]["native_missing"] =
            BlackMarketEntry::Missing(kNativeBlackMarketOotResources, [](const char* path) {
                const bool native = std::strncmp(path, "objects/object_bg/", 18) == 0 ||
                                    std::strcmp(path, "scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORITex_00BAC0") == 0;
                return native ? MmAssets_BaseResourceExists(path) != 0 : ResourceMgr_FileExists(path) != 0;
            });
        record["resources"]["legacy_missing"] = BlackMarketEntry::Missing(kBlackMarketResources, [](const char* path) {
            return BlackMarketEntry::LegacyResourceAvailable(path, ResourceMgr_FileExists);
        });
    }
    SPDLOG_INFO("[BlackMarketDoor] {}", record.dump());
}
void LogEntryRecovery() {
    const nlohmann::json record = { { "host", "oot" },
                                    { "file", gSaveContext.fileNum },
                                    { "entering", sEntering },
                                    { "travel_matches", MatchesTravel() },
                                    { "travel_slot", sTravelSlot },
                                    { "travel_seed", sTravelSeed },
                                    { "seed", Seed() },
                                    { "respawn_flag", gSaveContext.respawnFlag },
                                    { "destination_ready", DestinationAvailable() },
                                    { "market", BlackMarketEntry::MarketState(sSession, gSaveContext.fileNum) } };
    SPDLOG_INFO("[BlackMarketReturn] {}", record.dump());
}
} // namespace
extern "C" int ComboBlackMarketOot_TryDoorEntrance(PlayState* play, Actor* door) {
    if (play == gPlayState && Interior() && BlackMarketExitDoor::Matches<true>(play, door)) {
        const auto* player = GET_PLAYER(play);
        if (!Safe(play) || sTalking || !player || player->doorActor != door ||
            player->doorType != PLAYER_DOORTYPE_HANDLE || player->actionFunc != Player_Action_80845EF8 ||
            gSaveContext.health <= 0)
            return COMBO_BLACK_MARKET_OOT_DOOR_BLOCKED;
        sDoorCollision.Release(play);
        RememberTravel();
        sReturnPending = true;
        Transition(play, sReturnEntrance);
        return COMBO_BLACK_MARKET_OOT_DOOR_ENTERED;
    }
    OotBlackMarketDoorNative::Pose placed;
    if (play != gPlayState || !Exterior(play) || !OotBlackMarketDoorNative::MarkerActor(play, door)) {
        LogDoorAttempt(play, door, COMBO_BLACK_MARKET_OOT_DOOR_UNHANDLED, "door_not_recognized");
        return COMBO_BLACK_MARKET_OOT_DOOR_UNHANDLED;
    }
    if (!OotBlackMarketDoorNative::Find(play, placed)) {
        LogDoorAttempt(play, door, COMBO_BLACK_MARKET_OOT_DOOR_BLOCKED, "door_marker_ambiguous");
        return COMBO_BLACK_MARKET_OOT_DOOR_BLOCKED;
    }
    // Native Open already selected its action. Keep player/transition safety,
    // while stock, optional art and keeper preflight remain independent of entry.
    if (play->transitionTrigger != TRANS_TRIGGER_OFF || play->transitionMode != TRANS_MODE_OFF) {
        LogDoorAttempt(play, door, COMBO_BLACK_MARKET_OOT_DOOR_BLOCKED, "transition_active");
        return COMBO_BLACK_MARKET_OOT_DOOR_BLOCKED;
    }
    const auto* player = GET_PLAYER(play);
    if (!Safe(play) || !player || player->doorActor != door || player->doorType != PLAYER_DOORTYPE_HANDLE ||
        player->actionFunc != Player_Action_80845EF8 || gSaveContext.health <= 0) {
        LogDoorAttempt(play, door, COMBO_BLACK_MARKET_OOT_DOOR_BLOCKED, "native_state_not_ready");
        return COMBO_BLACK_MARKET_OOT_DOOR_BLOCKED;
    }
    if (!DestinationAvailable()) {
        LogDoorAttempt(play, door, COMBO_BLACK_MARKET_OOT_DOOR_BLOCKED, "destination_missing");
        return COMBO_BLACK_MARKET_OOT_DOOR_BLOCKED;
    }
    LogDoorAttempt(play, door, COMBO_BLACK_MARKET_OOT_DOOR_ENTERED, "native_open");
    sDoorCollision.Release(play);
    // The native save worker must finish before publishing a new return point.
    SaveManager::Instance->ThreadPoolWait();
    sReturnPos = OotBlackMarketDoorNative::Outside(placed);
    sReturnYaw = placed.yaw;
    sReturnEntrance = gSaveContext.entranceIndex;
    sDoorArmed = sReturnPending = false;
    sEntering = true;
    RememberTravel();
    Transition(play, ENTR_SKIEJER_MARKET);
    return COMBO_BLACK_MARKET_OOT_DOOR_ENTERED;
}

extern "C" int ComboBlackMarketNative_PrepareViewpointCamera(PlayState* play) {
    if (!sTalking || play != gPlayState || OwnNativeBrowsingCamera(play))
        return 1;
    if (BrowsingMessage(play))
        Message_CloseTextbox(play);
    EndBrowsing(play);
    // Skipping this dispatch is necessary even after restoring viewpoint 1:
    // applying its original BG0 record would also mutate the foreign camera.
    return 0;
}

extern "C" void ComboBlackMarketOot_NormalizeEntry(void) {
    if (gComboGameMode != COMBO_GAME_OOT)
        return;
    if (sResumeFromMarket) {
        sResumeFromMarket = false;
        if (MatchesTravel() && sSession.Active(gSaveContext.fileNum)) {
            sEntering = false;
            sReturnPending = true;
            gSaveContext.entranceIndex = sReturnEntrance;
            gSaveContext.cutsceneIndex = 0;
            gSaveContext.respawnFlag = 0;
            StageReturn();
            return;
        }
    }
    if (sReturnPending && MatchesTravel() && gSaveContext.entranceIndex == sReturnEntrance) {
        StageReturn();
        return;
    }
    if (gSaveContext.entranceIndex < ENTR_SKIEJER_MARKET || gSaveContext.entranceIndex > ENTR_SKIEJER_MARKET_3)
        return;
    if (sEntering && MatchesTravel() && DestinationAvailable() && gSaveContext.respawnFlag == 0)
        return;
    LogEntryRecovery();
    sEntering = false;
    sReturnPending = true;
    RememberTravel();
    gSaveContext.entranceIndex = sReturnEntrance;
    gSaveContext.cutsceneIndex = 0;
    gSaveContext.respawnFlag = 0;
    StageReturn();
}
extern "C" void ComboBlackMarketOot_ReportClear(int boss, int slot) {
    if (gComboGameMode != COMBO_GAME_OOT || boss < 0 || boss >= 8 || slot != gSaveContext.fileNum ||
        !sSession.Active(slot))
        return;
    const u16 next = sClears | (1u << boss);
    if (next == sClears)
        return;
    SaveManager::Instance->ThreadPoolWait();
    sClears = next;
    SaveManager::Instance->SaveSection(slot, SECTION_ID_BASE, false);
}
#endif
