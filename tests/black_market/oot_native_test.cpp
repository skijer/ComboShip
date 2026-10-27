#include "global.h"
#include "ComboGameMode.h"
#include "entrances/LostWoodsEntrance.h"
#include "entrances/BlackMarketEntrance.h"
#include "BlackMarketExitDoor.h"
#include "entrances/SoloForest.h"
#include "rando/BlackMarket.h"
#include "BlackMarketKeeper.h"
#include "BlackMarketReceipt.h"
#include "BlackMarketRoomMagicResources.h"
#include "BlackMarketBrowsing.h"
#include "soh/ActorDB.h"
#include "Enhancements/Gameplay/ComboBlackMarketNative.h"
#include "Enhancements/Gameplay/OotBlackMarketDoorNative.h"
#include "overlays/actors/ovl_En_Door/z_en_door.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <memory>
#include <string_view>
#include "entry_diagnostics_fixture.h"
#include "soh/Enhancements/game-interactor/vanilla-behavior/GIVanillaBehavior.h"
bool sNativeKeeperReady = false;
extern "C" u8 MmAssets_BaseResourceExists(const char*) { return destination; }
extern "C" { int gComboGameMode = COMBO_GAME_OOT; }
SaveContext gSaveContext{};
// Native title policy is exercised by lost_woods_entrance/run_oot_title_card_tests.py.
extern "C" void Combo_SOH_RequestCrossGameTitleCard(void) {}
extern "C" int Combo_SOH_SwapCustomTitleCardPresentation(int) { return 0; }
extern "C" bool GameInteractor_Should(GIVanillaBehavior, uint32_t result, ...) { return result; }
PlayState* gPlayState;
using namespace ComboRando::BlackMarket;
Session sSession;
// Native collision ownership is exercised by run_door_collision_native_tests.py.
struct DoorCollisionBoundary { int releases=0; void Release(PlayState*) { ++releases; } void Reset() {} } sDoorCollision;
nlohmann::json sPendingSave;
bool sFileLoadPending = true;
u16 sClears;
bool sEntering, sReturnPending, sReturnStaged, sResumeFromMarket;
bool sDoorArmed, sTalking, sTitlePending, sKeeperSpawned;
int sKeeperActorId = 1000;
/* CARD_RESOURCE */
int sTravelSlot = -1, sSelected;
float sDoorFloor;
u32 sTravelSeed;
Vec3f sReturnPos = { 184, 0, -300 };
s16 sReturnYaw = -16384;
u16 sReturnEntrance = ENTR_SKIEJER_FOREST;
Vec3f sGroundedReturnPos = { 184, 0, -300 };
s16 sGroundedReturnYaw = 0;
u16 sGroundedReturnEntrance = ENTR_SKIEJER_FOREST;
RespawnData sPreservedReturn;
std::string sStatus;
BlackMarketReceipt::State sReceipt;
nlohmann::json sReceiptMarket;
GetItemEntry sReceiptEntry{};
BlackMarketBrowsing::State sBrowser;
constexpr u16 kBrowseText = 0x8F10;
int receiptOffers = 0, browseStarts = 0, browseMessages = 0;
bool offerWorks = true, rowAvailable = true;
u16 rowMod = MOD_RANDOMIZER;
s16 activeCamera = CAM_ID_MAIN;
u32 nativeSeed = 77;
GetItemEntry lastOfferedEntry{};
bool pack = true, owned = false, usable = true, grantWorks = true;
int grants = 0, writes = 0;
u16 clearsAtWait;
Vec3f returnPosAtWait;
nlohmann::json persisted;
static PlayState play;
static Player player;
static EnDoor nativeDoor;
static TransitionActorEntry placedDoors[3];
static Vec3s playerJoints[2];
static int cameraResets, vanillaDoorCameras, doorErrors;
static bool floorExists = true;
extern "C" void Player_Action_80845EF8(Player*, PlayState*) {}
void Player_Action_Idle(Player*, PlayState*) {}
void Player_Action_8084E3C4(Player*, PlayState*) {}
void Player_Action_808507F4(Player*, PlayState*) {}
extern "C" void AudioOcarina_SetInstrument(u8) {}
void func_80832340(PlayState*, Player*) {}
extern "C" void func_8008EC70(Player*) {}
void func_80834644(PlayState*, Player*) {}
void func_808326F0(Player*) {}
LinkAnimationHeader* Player_GetIdleAnim(Player*) { return nullptr; }
void Player_AnimPlayOnce(PlayState*, Player*, LinkAnimationHeader*) {}
extern "C" void Audio_PlaySfxGeneral(u16 sfx, Vec3f*, u8, f32*, f32*, s8*) {
    assert(sfx == NA_SE_SY_ERROR); ++doorErrors;
}
Vec3f gSfxDefaultPos;
f32 gSfxDefaultFreqAndVolScale;
s8 gSfxDefaultReverb;
extern "C" void Camera_InitPlayerSettings(Camera* camera, Player* current) {
    assert(camera == &play.mainCamera && current == &player); ++cameraResets;
}
EntranceInfo gEntranceTable[ENTR_MAX];
int keeperSpawns, nativeInits, customInits;
ActorDBInit lastProfile;
int profileRegistrations;
ActorDB::ActorDB() {}
ActorDB::Entry::Entry() {}
ActorDB actorDatabase;
ActorDB* ActorDB::Instance = &actorDatabase;
ActorDB::Entry& ActorDB::AddEntry(const ActorDBInit& profile) {
    ++profileRegistrations;
    lastProfile = profile;
    static Entry entry;
    entry.entry.id = 1000;
    return entry;
}
bool inCs = false;
extern "C" s32 Player_InCsMode(PlayState*) { return inCs; }
extern "C" s32 Player_InBlockingCsMode(PlayState*, Player*) { return inCs; }
extern "C" s16 Play_GetActiveCamId(PlayState*) { return activeCamera; }
extern "C" s32 GiveItemEntryWithoutActor(PlayState* current, GetItemEntry entry) {
    assert(current == gPlayState);
    ++receiptOffers;
    lastOfferedEntry = entry;
    if (!offerWorks) return false;
    player.getItemEntry = entry;
    player.getItemId = entry.getItemId;
    player.interactRangeActor = &player.actor;
    return true;
}
bool BeginBrowsing(PlayState* current) {
    assert(current == gPlayState && !sReceipt.pending);
    ++browseStarts; sBrowser = {}; sSelected = -1; sTalking = true;
    return true;
}
extern "C" void Message_StartTextbox(PlayState* current, u16 text, Actor* actor) {
    assert(current == gPlayState && text == kBrowseText && !actor);
    ++browseMessages;
}
extern "C" Actor* Actor_Spawn(ActorContext*, PlayState*, s16 actor, f32 x, f32 y, f32 z, s16, s16 yaw, s16, s16 marker) {
    assert(actor > ACTOR_ID_MAX); // dedicated profile, independent of vanilla actor controllers
    assert(actor == sKeeperActorId && marker == 0); ++keeperSpawns;
    assert(x == BlackMarketKeeper::kX && y == BlackMarketKeeper::kY && z == BlackMarketKeeper::kZ &&
           yaw == BlackMarketKeeper::kYaw);
    return &player.actor;
}
CollisionPoly flatFloor;
extern "C" f32 BgCheck_EntityRaycastFloor5(PlayState*, CollisionContext*, CollisionPoly** poly, s32* bg, Actor*, Vec3f*) {
    *poly = floorExists ? &flatFloor : nullptr; *bg = BGCHECK_SCENE;
    return floorExists ? 0 : BGCHECK_Y_MIN;
}
void KeeperInit(Actor*, PlayState*) { ++customInits; }
void KeeperUpdate(Actor*, PlayState*) {}
void KeeperDraw(Actor*, PlayState*) {}
void VanillaInit(Actor*, PlayState*) { ++nativeInits; }
const char* SOH_ReadBlackMarketOptions() { return "{}"; }
u32 Seed() { return nativeSeed; }
bool PackAvailable() { return pack; }
extern "C" u8 ResourceMgr_FileExists(const char*) { return destination; }
extern "C" bool ResourceMgr_IsAltAssetsEnabled() { return false; }
extern "C" u8 ResourceMgr_FileAltExists(const char*) { return false; }
namespace ComboBlackMarketOotItems {
void LatchToolOwnership() {}
RandomizerGet Id(const Item&) { return RG_LANTERN; }
bool Owned(const Item&) { return owned; }
bool Available(const Item&) { return usable; }
bool Grant(const Item&) { if (!grantWorks) return false; owned = true; ++grants; return true; }
}
namespace Rando::StaticData {
struct NativeRow {
    std::shared_ptr<GetItemEntry> entry;
    std::shared_ptr<GetItemEntry> GetGIEntryUnresolved() const { return entry; }
};
NativeRow RetrieveItem(RandomizerGet id) {
    if (!rowAvailable) return {};
    auto entry = std::make_shared<GetItemEntry>();
    entry->getItemId = id; entry->modIndex = rowMod;
    entry->textId = 0x1357; entry->objectId = OBJECT_GAMEPLAY_KEEP; entry->gi = 17;
    return { entry };
}
}
int shopDraws, fallbackDraws;
bool shopDrawAvailable = true;
extern "C" bool NeiGi_DrawShop(PlayState*, GetItemEntry* entry) {
    assert(entry && entry->getItemId == RG_LANTERN); ++shopDraws; return shopDrawAvailable;
}
extern "C" void GetItemEntry_Draw(PlayState*, GetItemEntry) { ++fallbackDraws; }
struct SaveManager {
    static SaveManager* Instance;
    void ThreadPoolWait() { clearsAtWait = sClears; returnPosAtWait = sReturnPos; }
    void SaveData(const std::string&, const nlohmann::json& value) { persisted = value; }
    void SaveSection(int, int, bool) {
        ++writes;
        persisted = { { "market", sSession.Save(gSaveContext.fileNum) }, { "rupees", gSaveContext.rupees } };
    }
};
SaveManager saveManager;
SaveManager* SaveManager::Instance = &saveManager;
/* NATIVE_DOOR_CLEANUP */
/* PRODUCTION */

static int NativeOpen(Player* current, PlayState* play, EnDoor* door) {
    Actor* doorActor = &door->actor;
    Player_SetupAction(play, current, Player_Action_80845EF8, 0);
    current->doorActor = doorActor;
    current->doorType = PLAYER_DOORTYPE_HANDLE;
    current->skelAnime.movementFlags = 0x28F;
    current->skelAnime.jointTable[0] = { 55, 66, 77 };
    current->skelAnime.jointTable[1].y = 15;
    door->playerIsOpening = true;
    /* NATIVE_DOOR_OPEN */
    ++vanillaDoorCameras;
    return 1;
}

void Reset() {
    sReceipt = {}; sReceiptMarket = {}; sReceiptEntry = {}; sBrowser = {};
    receiptOffers = browseStarts = browseMessages = 0;
    offerWorks = rowAvailable = true; rowMod = MOD_RANDOMIZER; nativeSeed = 77;
    activeCamera = CAM_ID_MAIN; lastOfferedEntry = {};
    destination = true;
    diagnostics.clear();
    gComboGameMode = COMBO_GAME_OOT;
    gSaveContext = {};
    play = {}; player = {}; gPlayState = &play; play.state.running = true;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
    gSaveContext.fileNum = 0;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    Options options; options.games = Games::Oot;
    sSession.Bind(Generate(77, options), 0); sSession.state = {};
    sClears = 3;
    sEntering = sReturnPending = sReturnStaged = sResumeFromMarket = false;
    sDoorArmed = sTalking = sTitlePending = sKeeperSpawned = false;
    sTravelSlot = -1; sTravelSeed = 0; sSelected = 0;
    sReturnEntrance = ENTR_SKIEJER_FOREST; sReturnPos = { 184, 0, -300 }; sReturnYaw = -16384;
    sGroundedReturnPos = sReturnPos; sGroundedReturnYaw = sReturnYaw; sGroundedReturnEntrance = sReturnEntrance;
    owned = false; usable = grantWorks = pack = true; grants = writes = 0;
    keeperSpawns = nativeInits = customInits = 0; inCs = false;
    sKeeperActorId = 1000;
    flatFloor = {}; flatFloor.normal.y = 32767;
    placedDoors[0] = placedDoors[1] = placedDoors[2] = {};
    nativeDoor = {};
    cameraResets = vanillaDoorCameras = doorErrors = 0;
    floorExists = true;
    playerJoints[0] = playerJoints[1] = {};
    player.skelAnime.jointTable = playerJoints;
    player.skelAnime.baseTransl = { 1, 2, 3 };
    player.actor.category = ACTORCAT_PLAYER;
    gSaveContext.health = 0x30;
    player.actor.floorPoly = &flatFloor; player.actor.floorHeight = 0;
    gEntranceTable[ENTR_SKIEJER_FOREST].scene = SCENE_SKIEJER_FOREST;
    sPendingSave = nullptr;
    sFileLoadPending = true;
    gSaveContext.rupees = 500;
    clearsAtWait = 0;
    shopDraws = fallbackDraws = 0; shopDrawAvailable = true;
}
static void PlaceDoor() {
    play.sceneNum = SCENE_SKIEJER_FOREST;
    gSaveContext.entranceIndex = ENTR_SKIEJER_FOREST;
    play.transiActorCtx.list = placedDoors;
    play.transiActorCtx.numActors = 2;
    placedDoors[0].id = ACTOR_DOOR_SHUTTER;
    auto& placed = placedDoors[1];
    placed.id = -ACTOR_EN_DOOR; // native spawn marks the entry negative
    placed.sides[0].room = placed.sides[1].room = 0;
    placed.sides[0].effects = placed.sides[1].effects = -1;
    placed.pos = { -680, 0, 72 };
    placed.rotY = static_cast<s16>(106 * 65536 / 360);
    nativeDoor.actor.id = ACTOR_EN_DOOR;
    nativeDoor.actor.category = ACTORCAT_DOOR;
    nativeDoor.actor.params = 1 << 10;
    nativeDoor.actor.home.pos = nativeDoor.actor.world.pos = {
        static_cast<float>(placed.pos.x), static_cast<float>(placed.pos.y), static_cast<float>(placed.pos.z) };
    nativeDoor.actor.home.rot.y = nativeDoor.actor.shape.rot.y = placed.rotY;
    player.doorActor = &nativeDoor.actor;
    player.doorType = PLAYER_DOORTYPE_HANDLE;
    player.actionFunc = Player_Action_80845EF8;
}
static void TestDoorMarker() {
    for (int count : { 2, 3 }) {
        Reset(); PlaceDoor();
        play.transiActorCtx.numActors = count;
        placedDoors[2] = placedDoors[1];
        sDoorArmed = true;
        player.actor.world.pos = { 410, 0, -300 };
        PlayerUpdate();
        assert(!sEntering && play.transitionTrigger == TRANS_TRIGGER_OFF);
        if (count == 3) {
            assert(ComboBlackMarketOot_TryDoorEntrance(&play, &nativeDoor.actor) ==
                   COMBO_BLACK_MARKET_OOT_DOOR_BLOCKED);
            assert(diagnostics.back()["reason"] == "door_marker_ambiguous" &&
                   diagnostics.back()["checks"]["unique_marker"] == false);
        }
    }
    Reset(); PlaceDoor();
    placedDoors[1].sides[1].room = 1; // ordinary native room-to-room door
    sDoorArmed = true; player.actor.world.pos = { 410, 0, -300 }; PlayerUpdate();
    assert(sEntering && play.nextEntranceIndex == ENTR_SKIEJER_MARKET);
    std::puts("PASS OoT authored marker suppresses obsolete coordinate trigger; legacy forest preserved");
}
static void TestDoorEntry() {
    Reset(); PlaceDoor();
    const auto priorReturn = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    const auto priorTop = gSaveContext.respawn[RESPAWN_MODE_TOP];
    inCs = true; // native door action owns the player; its action is the explicit gate
    assert(ComboBlackMarketOot_TryDoorEntrance(&play, &nativeDoor.actor) == COMBO_BLACK_MARKET_OOT_DOOR_ENTERED);
    assert(sEntering && play.nextEntranceIndex == ENTR_SKIEJER_MARKET &&
           play.transitionTrigger == TRANS_TRIGGER_START && MatchesTravel());
    const float yaw = placedDoors[1].rotY * (6.28318530718f / 65536.0f);
    assert(std::abs(sReturnPos.x - (-680 + std::sin(yaw) * 226)) < .001f);
    assert(std::abs(sReturnPos.z - (72 + std::cos(yaw) * 226)) < .001f);
    assert(sReturnYaw == placedDoors[1].rotY && sReturnEntrance == ENTR_SKIEJER_FOREST);
    assert(returnPosAtWait.x == 184 && returnPosAtWait.z == -300 && writes == 0);
    assert(!std::memcmp(&priorReturn, &gSaveContext.respawn[RESPAWN_MODE_RETURN], sizeof(priorReturn)));
    assert(!std::memcmp(&priorTop, &gSaveContext.respawn[RESPAWN_MODE_TOP], sizeof(priorTop)));
    for (int gate = 0; gate < 3; ++gate) {
        Reset(); PlaceDoor();
        if (gate == 0) destination = false;
        if (gate == 1) play.transitionTrigger = TRANS_TRIGGER_START;
        if (gate == 2) play.transitionMode = TRANS_MODE_SETUP;
        assert(ComboBlackMarketOot_TryDoorEntrance(&play, &nativeDoor.actor) == COMBO_BLACK_MARKET_OOT_DOOR_BLOCKED);
        assert(!sEntering);
        assert(writes == 0 && sReturnPos.x == 184);
    }
    for (int unrelated = 0; unrelated < 9; ++unrelated) {
        Reset(); PlaceDoor();
        if (unrelated == 0) gComboGameMode = COMBO_GAME_BOTH;
        if (unrelated == 1) gComboGameMode = COMBO_GAME_MM;
        if (unrelated == 2) play.sceneNum = SCENE_KOKIRI_FOREST;
        if (unrelated == 3) play.roomCtx.curRoom.num = 1;
        if (unrelated == 4) placedDoors[1].sides[1].room = 1;
        if (unrelated == 5) nativeDoor.actor.id = ACTOR_DOOR_SHUTTER;
        if (unrelated == 6) nativeDoor.actor.home.pos.x += 2;
        if (unrelated == 7) nativeDoor.actor.params = 0;
        if (unrelated == 8) placedDoors[1].params = 0x80;
        assert(ComboBlackMarketOot_TryDoorEntrance(&play, &nativeDoor.actor) == COMBO_BLACK_MARKET_OOT_DOOR_UNHANDLED);
        assert(!sEntering && play.transitionTrigger == TRANS_TRIGGER_OFF);
    }
    Reset(); PlaceDoor(); PlayState another = play;
    assert(ComboBlackMarketOot_TryDoorEntrance(&another, &nativeDoor.actor) == COMBO_BLACK_MARKET_OOT_DOOR_UNHANDLED);
    assert(ComboBlackMarketOot_TryDoorEntrance(nullptr, nullptr) == COMBO_BLACK_MARKET_OOT_DOOR_UNHANDLED);
    std::puts("PASS OoT direct native door entry, destination/transition protection and mode isolation");
}
static void TestDoorReturn() {
    Reset(); PlaceDoor();
    // Re-entry uses the loaded transition pose, including changes since entry/save.
    placedDoors[1].pos = { 150, 25, 330 };
    placedDoors[1].rotY = -16384;
    RememberTravel(); sReturnPending = true;
    gSaveContext.respawn[RESPAWN_MODE_RETURN].pos = { 44, 55, 66 };
    const auto preserved = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    StageReturn();
    player.actor.world.pos = { -900, 40, -600 };
    player.actor.velocity = { 3, 4, 5 }; player.actor.speedXZ = 7;
    PlayerUpdate();
    assert(std::abs(player.actor.world.pos.x - (-76)) < .001f &&
           std::abs(player.actor.world.pos.z - 330) < .001f && player.actor.world.pos.y == 0);
    assert(player.actor.shape.rot.y == -16384 && player.actor.world.rot.y == -16384 && player.yaw == -16384);
    assert(player.actor.velocity.x == 0 && player.actor.velocity.y == 0 && player.actor.speedXZ == 0);
    assert(cameraResets == 1 && !sReturnPending && !sReturnStaged);
    assert(!std::memcmp(&preserved, &gSaveContext.respawn[RESPAWN_MODE_RETURN], sizeof(preserved)));
    Reset(); PlaceDoor(); RememberTravel(); sReturnPending = true; floorExists = false;
    player.actor.world.pos = { 91, 92, 93 }; PlayerUpdate();
    assert(player.actor.world.pos.x == 91 && player.actor.world.pos.y == 92 && cameraResets == 0);
    std::puts("PASS OoT loaded door return pose, ground check, camera initialization and respawn lifetime");
}
static void TestDoorOpen() {
    Reset(); PlaceDoor();
    assert(NativeOpen(&player, &play, &nativeDoor));
    assert(sEntering && play.nextEntranceIndex == ENTR_SKIEJER_MARKET && vanillaDoorCameras == 0);
    assert(player.stateFlags1 & PLAYER_STATE1_IN_CUTSCENE);
    Reset(); PlaceDoor(); destination = false;
    assert(NativeOpen(&player, &play, &nativeDoor));
    assert(player.actionFunc == Player_Action_Idle && !nativeDoor.playerIsOpening);
    assert(!(player.stateFlags1 & (PLAYER_STATE1_IN_CUTSCENE | PLAYER_STATE1_LOADING)));
    assert(player.skelAnime.movementFlags == 0 && player.skelAnime.jointTable[1].y == 0);
    assert(player.skelAnime.jointTable[0].x == 1 && player.skelAnime.jointTable[0].z == 3);
    assert(doorErrors == 1 && vanillaDoorCameras == 0 && !sEntering);
    for (int mode : { COMBO_GAME_BOTH, COMBO_GAME_MM }) {
        Reset(); PlaceDoor(); gComboGameMode = mode;
        assert(NativeOpen(&player, &play, &nativeDoor));
        assert(vanillaDoorCameras == 1 && doorErrors == 0 && !sEntering);
    }
    std::puts("PASS OoT native Open hook, actual idle/action animation cleanup and ordinary-door fallback");
}
static void PrepareReceipt() {
    Reset();
    play.sceneNum = SCENE_SKIEJER_MARKET;
    assert(Status(0) == PurchaseResult::Ready);
}
static void NativeAcquire(const GetItemEntry& entry) {
    // Observe the native GI boundary: the engine owns acquisition, grants the
    // unresolved row, then reports OnItemReceive. Offering alone does neither.
    player.stateFlags1 |= PLAYER_STATE1_GETTING_ITEM;
    if (!FixedMarketGrant(&play, entry)) {
        const auto* item = Offer(sReceipt.slot);
        if (item) ComboBlackMarketOotItems::Grant(*item);
    }
    ReceiveMarketItem(entry);
}
static void TestReceipt() {
    PrepareReceipt();
    const int price = sSession.market["stock"][0]["price"];
    assert(Purchase() && sReceipt.pending && !sReceipt.delivered);
    assert(grants == 0 && writes == 0 && gSaveContext.rupees == 500 && !sSession.state.purchased);
    assert(!Purchase()); // An outstanding offer cannot create a second receipt.
    play.msgCtx.msgMode = 1;
    MonitorReceipt(); assert(receiptOffers == 0);
    play.msgCtx.msgMode = 0; inCs = true;
    MonitorReceipt(); assert(receiptOffers == 0);
    inCs = false; offerWorks = false;
    MonitorReceipt(); assert(receiptOffers == 1 && !sReceipt.offered);
    offerWorks = true;
    MonitorReceipt();
    const auto entry = lastOfferedEntry;
    assert(receiptOffers == 2 && sReceipt.offered && entry.comboMarketReceipt == sReceipt.ticket);
    assert(entry.modIndex == MOD_RANDOMIZER && entry.getItemId == RG_LANTERN &&
           entry.textId == 0x1357 && entry.objectId == OBJECT_GAMEPLAY_KEEP && entry.gi == 17);
    assert(grants == 0 && writes == 0 && gSaveContext.rupees == 500);
    ReceiveMarketItem(entry); // The native acquisition flag is also required.
    assert(grants == 0 && writes == 0);
    NativeAcquire(entry);
    assert(grants == 1 && writes == 1 && sReceipt.delivered && gSaveContext.rupees == 500 - price);
    assert(persisted["rupees"] == 500 - price && persisted["market"]["state"]["purchased"] == 1);
    NativeAcquire(entry);
    assert(grants == 1 && writes == 1 && gSaveContext.rupees == 500 - price);
    assert(!Purchase());
    MonitorReceipt(); assert(browseStarts == 0);
    player.stateFlags1 = 0; play.msgCtx.msgMode = 1;
    MonitorReceipt(); assert(browseStarts == 0);
    play.msgCtx.msgMode = 0; inCs = true;
    MonitorReceipt(); assert(browseStarts == 0);
    inCs = false; activeCamera = CAM_ID_MAIN + 1;
    MonitorReceipt(); assert(browseStarts == 0 && sReceipt.pending);
    activeCamera = CAM_ID_MAIN; sSelected = -1;
    MonitorReceipt();
    assert(!sReceipt.pending && browseStarts == 1 && browseMessages == 1 && sSelected == 0 && sBrowser.slot == 0);
    MonitorReceipt(); assert(browseStarts == 1 && browseMessages == 1);

    PrepareReceipt(); grantWorks = false;
    assert(Purchase()); MonitorReceipt(); NativeAcquire(lastOfferedEntry);
    assert(!sReceipt.delivered && grants == 0 && writes == 0 &&
           gSaveContext.rupees == 500 && !sSession.state.purchased);
    CancelReceipt();
    for (int invalid = 0; invalid < 2; ++invalid) {
        PrepareReceipt();
        if (invalid == 0) rowAvailable = false;
        else rowMod = MOD_RANDOMIZER + 1;
        assert(!Purchase() && !sReceipt.pending && grants == 0 && writes == 0);
    }
    PrepareReceipt(); gSaveContext.rupeeAccumulator = -450;
    if (price > 50) assert(!Purchase() && grants == 0 && writes == 0);

    // A market marker never authorizes an unrelated GI row or player entry.
    for (int mismatch = 0; mismatch < 4; ++mismatch) {
        PrepareReceipt(); assert(Purchase()); MonitorReceipt();
        auto wrong = lastOfferedEntry;
        if (mismatch == 0) ++wrong.comboMarketReceipt;
        if (mismatch == 1) ++wrong.modIndex;
        if (mismatch == 2) ++wrong.getItemId;
        if (mismatch == 3) ++player.getItemEntry.comboMarketReceipt;
        NativeAcquire(wrong);
        assert(grants == 0 && writes == 0 && !sReceipt.delivered && gSaveContext.rupees == 500);
    }
    GetItemEntry ordinary{};
    assert(!FixedMarketGrant(&play, ordinary)); // Ordinary native grants pass through.

    PrepareReceipt(); assert(Purchase()); MonitorReceipt();
    const auto cancelled = lastOfferedEntry;
    CancelReceipt();
    assert(!sReceipt.pending && player.getItemEntry.comboMarketReceipt == 0 &&
           player.getItemId == GI_NONE && !player.interactRangeActor);
    assert(Purchase() && sReceipt.ticket != cancelled.comboMarketReceipt);
    MonitorReceipt();
    NativeAcquire(cancelled);
    assert(grants == 0 && writes == 0 && !sReceipt.delivered);
    NativeAcquire(lastOfferedEntry);
    assert(grants == 1 && writes == 1);
    PrepareReceipt(); assert(Purchase()); MonitorReceipt();
    ++player.getItemEntry.comboMarketReceipt;
    const int unrelated = player.getItemEntry.comboMarketReceipt;
    CancelReceipt();
    assert(player.getItemEntry.comboMarketReceipt == unrelated); // Do not withdraw another native action.

    for (int stale = 0; stale < 7; ++stale) {
        PrepareReceipt(); assert(Purchase()); MonitorReceipt();
        const auto old = lastOfferedEntry;
        if (stale == 0) gSaveContext.fileNum = 1;
        if (stale == 1) ++nativeSeed;
        if (stale == 2) sSession.market["stock"][0]["price"] = price + 1;
        if (stale == 3) play.sceneNum = SCENE_SKIEJER_FOREST;
        if (stale == 4) gComboGameMode = COMBO_GAME_BOTH;
        if (stale == 5) play.transitionTrigger = TRANS_TRIGGER_START;
        if (stale == 6) play.gameOverCtx.state = GAMEOVER_DEATH_START;
        NativeAcquire(old);
        assert(grants == 0 && writes == 0 && !sReceipt.delivered && gSaveContext.rupees == 500);
        MonitorReceipt();
        assert(!sReceipt.pending && browseStarts == 0);
    }
    std::puts("PASS OoT unresolved native GI offer, acknowledgment-only purchase/save, cancellation and browse recovery");
}

#define OOT_INTERIOR_EXIT_TEST
#include "interior_exit_cases.inc"
int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "door-exit") { TestInteriorExitDoor(); return 0; }
    const std::string only = argc > 1 ? argv[1] : "all";
    if (only != "all") {
        if (only == "receipt") TestReceipt();
        if (only == "door-marker") TestDoorMarker();
        if (only == "door-entry") TestDoorEntry();
        if (only == "door-return") TestDoorReturn();
        if (only == "door-open") TestDoorOpen();
        return 0;
    }
    Reset(); DrawOffer(&play, *Find("nei.lantern"));
    assert(shopDraws == 1 && fallbackDraws == 0);
    shopDrawAvailable = false; DrawOffer(&play, *Find("nei.lantern"));
    assert(shopDraws == 2 && fallbackDraws == 1);
    Reset();
    gSaveContext.entranceIndex = ENTR_SKIEJER_MARKET;
    const auto top = gSaveContext.respawn[RESPAWN_MODE_TOP];
    gSaveContext.respawn[RESPAWN_MODE_RETURN].pos = { 44, 55, 66 };
    const auto local = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    ComboBlackMarketOot_NormalizeEntry();
    assert(gSaveContext.entranceIndex == ENTR_SKIEJER_FOREST);
    assert(sReturnPending && sReturnStaged && gSaveContext.respawnFlag == 2);
    assert(!std::memcmp(&top, &gSaveContext.respawn[RESPAWN_MODE_TOP], sizeof(top)));
    assert(!std::memcmp(&local, &sPreservedReturn, sizeof(local)));
    Reset(); sEntering = true; RememberTravel();
    gSaveContext.entranceIndex = ENTR_SKIEJER_MARKET;
    ComboBlackMarketOot_NormalizeEntry();
    assert(gSaveContext.entranceIndex == ENTR_SKIEJER_MARKET && !sReturnPending);
    gSaveContext.respawnFlag = 1;
    ComboBlackMarketOot_NormalizeEntry();
    assert(gSaveContext.entranceIndex == ENTR_SKIEJER_FOREST);
    Reset(); gComboGameMode = COMBO_GAME_BOTH;
    gSaveContext.entranceIndex = ENTR_SKIEJER_MARKET;
    ComboBlackMarketOot_NormalizeEntry();
    assert(gSaveContext.entranceIndex == ENTR_SKIEJER_MARKET && !sReturnPending);
    Reset(); play.sceneNum = SCENE_LOST_WOODS;
    assert(!Exterior(&play)); // native LW now approaches the forest, never the shop directly
    TestReceipt();
    Reset(); play.sceneNum = SCENE_SKIEJER_FOREST;
    player.actor.world.pos = { 184, 500, -300 }; PlayerUpdate();
    assert(sReturnPos.y == 0 && sReturnPos.x == 184);
    Reset(); play.sceneNum = SCENE_SKIEJER_FOREST; play.roomCtx.curRoom.num = 0;
    gSaveContext.entranceIndex = ENTR_SKIEJER_FOREST;
    player.actor.world.pos = { 184, 0, -300 }; PlayerUpdate();
    assert(sDoorArmed);
    assert(sReturnPos.x == 184); // save-worker-facing return point stays frozen while walking
    player.actor.world.pos = { 410, 80, -300 }; PlayerUpdate(); // airborne entry keeps last grounded candidate
    assert(sEntering && play.nextEntranceIndex == ENTR_SKIEJER_MARKET);
    assert(returnPosAtWait.x == 184 && sReturnPos.x == 184 && sReturnPos.y == 0);
    assert(gComboGameMode == COMBO_GAME_OOT && play.transitionTrigger == TRANS_TRIGGER_START);
    play.transitionTrigger = TRANS_TRIGGER_OFF; play.sceneNum = SCENE_SKIEJER_MARKET;
    player.actor.world.pos = { -11, 0, 381 }; PlayerUpdate(); PlayerUpdate();
    assert(keeperSpawns == 1);
    sKeeperActorId = -1; profileRegistrations = 0;
    RegisterKeeperActor(); RegisterKeeperActor();
    assert(profileRegistrations == 1 && sKeeperActorId == 1000);
    assert(lastProfile.category == ACTORCAT_NPC && lastProfile.objectId == OBJECT_GAMEPLAY_KEEP);
    assert(lastProfile.instanceSize == sizeof(Actor) && lastProfile.destroy == nullptr);
    assert(lastProfile.init == KeeperInit && lastProfile.update == KeeperUpdate && lastProfile.draw == KeeperDraw);
    lastProfile.init(&player.actor, &play); assert(customInits == 1 && nativeInits == 0);
    sTitlePending = true;
    PresentTitle();
    const auto& title = play.actorCtx.titleCtx;
    assert(title.texture == kCard && title.width == 240 && title.height == 24);
    assert(std::string_view(static_cast<const char*>(title.texture)) == "__OTR__custom/oot_location_titles/v1/market");
    assert(!title.isBossCard && !title.hasTranslation && title.durationTimer == 80 && title.delayTimer == 20);
    player.actor.world.pos = { -11, 0, 410 }; PlayerUpdate();
    assert(sReturnPending && play.nextEntranceIndex == ENTR_SKIEJER_FOREST);
    gSaveContext.entranceIndex = play.nextEntranceIndex; ComboBlackMarketOot_NormalizeEntry();
    play.sceneNum = SCENE_SKIEJER_FOREST; play.transitionTrigger = TRANS_TRIGGER_OFF; PlayerUpdate();
    assert(!sReturnStaged && !sReturnPending && player.actor.world.pos.x == 184);
    Reset();
    sPendingSave = { { "market", sSession.market }, { "state", { { "purchased", 0x5A } } },
        { "clears", 5 }, { "savedInside", true } };
    BindSolo(0);
    assert(sSession.Active(0) && sSession.state.purchased == 0x5A && sClears == 5);
    assert(sResumeFromMarket && !sReturnStaged);
    // A later unordered native randomizer hook changes the savewarp entrance.
    gSaveContext.entranceIndex = ENTR_TEMPLE_OF_TIME_WARP_PAD;
    gSaveContext.respawn[RESPAWN_MODE_RETURN].pos = { 44, 55, 66 };
    const auto loadedReturn = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    ComboBlackMarketOot_NormalizeEntry();
    assert(sReturnPending && sReturnStaged && gSaveContext.entranceIndex == ENTR_SKIEJER_FOREST);
    assert(!std::memcmp(&sPreservedReturn, &loadedReturn, sizeof(loadedReturn)));
    SaveMarket(gSaveContext, 0, true);
    assert(persisted["state"]["purchased"] == 0x5A && persisted["clears"] == 5);
    gSaveContext.entranceIndex = ENTR_SKIEJER_MARKET_3; gPlayState = nullptr;
    SaveMarket(gSaveContext, 0, true);
    assert(persisted["savedInside"] == true); // native snapshot only, no live scene access on worker
    gPlayState = &play;
    sPendingSave["market"] = Generate(77, OptionsFromJson({ { "games", "mm" } }));
    sFileLoadPending = true;
    BindSolo(0); assert(!sSession.Active(0));
    Reset(); BindSolo(0); assert(sSession.Active(0) && sSession.market["games"] == "oot");
    assert(writes == 1);
    CommitPurchase(sSession.state, 4);
    BindSolo(0); // native age/warp notifications are not another file load
    assert(sSession.state.purchased == 16 && writes == 1);
    for (int gate = 0; gate < 6; ++gate) {
        Reset(); play.sceneNum = SCENE_SKIEJER_FOREST; play.roomCtx.curRoom.num = 0;
        player.actor.world.pos = { 410, 0, -300 }; sDoorArmed = true;
        if (gate == 0) destination = false;
        if (gate == 1) floorExists = false;
        if (gate == 2) play.pauseCtx.state = 1;
        if (gate == 3) inCs = true;
        if (gate == 4) gComboGameMode = COMBO_GAME_BOTH;
        if (gate == 5) play.roomCtx.curRoom.num = 1;
        PlayerUpdate(); assert(!sEntering && play.transitionTrigger == TRANS_TRIGGER_OFF);
    }
    Reset(); ComboBlackMarketOot_ReportClear(3, 0);
    assert(clearsAtWait == 3 && sClears == 11 && writes == 1);
    ComboBlackMarketOot_ReportClear(3, 0); assert(writes == 1);
    ComboBlackMarketOot_ReportClear(4, 1); assert(writes == 1 && sClears == 11);
    gComboGameMode = COMBO_GAME_BOTH;
    ComboBlackMarketOot_ReportClear(4, 0); assert(writes == 1 && sClears == 11);
    std::puts("PASS OoT native transactions, dedicated entry, solo isolation and local RETURN/TOP recovery");
    TestDoorMarker();
    TestDoorEntry();
    TestDoorReturn();
    TestDoorOpen();
}
