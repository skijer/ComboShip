#include "global.h"
#include "ComboGameMode.h"
#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#include "2s2h/Enhancements/Gameplay/BlackMarket.h"
#include "2s2h/Enhancements/Gameplay/BlackMarketDoorNative.h"
#include "entrances/BlackMarketEntrance.h"
#include "BlackMarketExitDoor.h"
#include "entrances/SoloForest.h"
#include "rando/BlackMarket.h"
#include "BlackMarketText.h"
#include "BlackMarketReceipt.h"
#include "BlackMarketRoomMagicResources.h"
#include "2s2h/CustomItem/CustomItem.h"
#include "2s2h/Rando/ItemReceiptText.h"
#include "sword_element_message.h"
#include "2s2h/CustomMessage/CustomMessage.h"
#include "2s2h/Rando/StaticData/StaticData.h"
#include "overlays/actors/ovl_En_Door/z_en_door.h"
#include "overlays/actors/ovl_En_Ossan/z_en_ossan.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include "entry_diagnostics_fixture.h"
extern "C" u8 ResourceMgr_FileExists(const char*) { return destination; }
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
SaveContext gSaveContext{};
// Native title policy is exercised by lost_woods_entrance/run_mm_title_card_tests.py.
extern "C" void Combo_MM_RequestCrossGameTitleCard(void) {}
PlayState* gPlayState;
GameState* gGameState;
using namespace ComboRando::BlackMarket;
Session sSession;
// Native collision ownership is exercised by run_door_collision_native_tests.py.
struct DoorCollisionBoundary { int releases=0; void Release(PlayState*) { ++releases; } void Reset() {} } sDoorCollision;
uint16_t sClears;
bool sEntering, sReturnPending, sDoorArmed, sTitlePending, sTalking;
bool sResumeFromMarket;
int sSelected;
float sDoorFloor;
int sTravelSlot = -1;
u32 sTravelSeed;
Vec3f sSoloReturn = { 0, 0, 0 };
s16 sSoloReturnYaw = -8192;
u16 sSoloReturnEntrance = ENTRANCE(WOODS_OF_MYSTERY, 0);
RespawnData sPreservedReturn;
bool sReturnStaged = false;
EnOssan* sNativeShop = nullptr; // Native actor actions run in the dedicated shop fixture.
PlayState* sNativeShopPlay = nullptr;
std::string sStatus;
bool pack = true, owned = false, usable = true, grantWorks = true, inCs = false;
int grants, writes;
static int receiptTextCalls, activeReceiptMessages, ordinaryReceiptMessages;
static int receiptSpawns, receiptKills, receiptDestroys;
static bool receiptCapeChoice, receiptCapturedBeforeGrant, receiptSpawnWorks = true;
static std::array<EnItem00, 8> receiptActors;
static GameInteractor receiptInteractor;
GameInteractor* GameInteractor::Instance = &receiptInteractor;
nlohmann::json persisted;
nlohmann::json nativeSavedDisk;
int CVarGetInteger(const char*, int fallback) { return fallback; }
std::string SaveManager_GetFileName(int slot) { return "file" + std::to_string(slot) + ".json"; }
int SaveManager_ReadSaveFile(const std::filesystem::path&, nlohmann::json& saved) { saved = nativeSavedDisk; return 0; }
void SaveManager_WriteSaveFile(const std::filesystem::path&, nlohmann::json saved) { nativeSavedDisk = std::move(saved); }
static PlayState play;
static Player player;
bool PackAvailable() { return pack; }
bool Interior();
bool Active() { return Interior() && sSession.Active(gSaveContext.fileNum); }
extern "C" bool Player_InCsMode(PlayState*) { return inCs; }
extern "C" void Player_Action_35(Player*, PlayState*) {}
extern "C" void Player_Action_36(Player*, PlayState*) {}
static int vanillaDoors, stoppedDoors, doorErrors, doorCutscenes;
extern "C" s16 CutsceneManager_Start(s16, Actor*) { ++doorCutscenes; return 1; }
static void NativeIdle(Player*, PlayState*) {}
extern "C" void Player_StopCutscene(Player*) { ++stoppedDoors; }
void func_80839E74(Player* current, PlayState*) { current->actionFunc = NativeIdle; }
extern "C" void Audio_PlaySfx(u16) { ++doorErrors; }
CollisionPoly flatFloor;
bool floorExists = true;
extern "C" f32 BgCheck_EntityRaycastFloor5_3(PlayState*, CollisionContext*, CollisionPoly** poly,
    s32* bg, Actor*, Vec3f*) {
    *poly = floorExists ? &flatFloor : nullptr; *bg = BGCHECK_SCENE;
    return floorExists ? 0 : BGCHECK_Y_MIN;
}
extern "C" void Camera_InitFocalActorSettings(Camera*, Actor*) {}
namespace ComboBlackMarketItems {
RandoItemId Id(const Item&) { return RI_UNKNOWN; }
bool Owned(const Item&) { return owned; }
bool Available(const Item&) { return usable; }
bool Grant(const Item&) {
    if (!grantWorks) return false;
    owned = true;
    ++grants;
    const auto item = Rando::StaticData::Items.at(RI_UNKNOWN).itemId;
    if (item == ITEM_ARROW_FIRE) gSaveContext.save.saveInfo.inventory.items[SLOT_ARROW_FIRE] = item;
    return true;
}
}
void SaveManager_SaveCurrentForCombo(bool) {
    ++writes;
    persisted = { { "market", sSession.Save(gSaveContext.fileNum) },
                  { "rupees", gSaveContext.save.saveInfo.playerData.rupees } };
}
constexpr u16 kBrowseText = 0x8F10, kResultText = 0x8F11, kWelcomeText = 0x8F12, kConfirmText = 0x8F13;
constexpr u16 kKeeperMenuText = 0x8F14, kKeeperTalkText = 0x8F15, kContinueText = 0x8F16;
constexpr u16 kKeeperHelpChoiceText = 0x8F17, kKeeperHelpText = 0x8F18;
namespace CustomMessage {
Entry last;
void LoadCustomMessageIntoFont(Entry entry) { last = std::move(entry); }
void SetActiveCustomMessage(std::string msg, Entry entry) {
    ++activeReceiptMessages; entry.msg = std::move(msg); last = std::move(entry);
}
void StartTextbox(std::string msg, Entry entry) {
    ++ordinaryReceiptMessages; entry.msg = std::move(msg); last = std::move(entry);
}
}
namespace Rando::StaticData {
std::map<RandoItemId, RandoStaticItem> Items;
u8 GetIconForZMessage(RandoItemId id) { assert(id == RI_UNKNOWN); return ITEM_MASK_BUNNY; }
}
namespace Rando {
bool ApplyItemReceiptText(RandoItemId id, CustomMessage::Entry& entry) {
    assert(id == RI_UNKNOWN);
    ++receiptTextCalls;
    receiptCapturedBeforeGrant = !owned;
    entry.msg = "Native receipt and tutorial.";
    entry.capeVisibilityChoice = receiptCapeChoice;
    return true;
}
void AppendReceiptSource(CustomMessage::Entry&, const std::string&) {}
}
// Actor allocation, native item drawing and collider destruction are explicit
// boundaries. All receipt ownership, flags, callbacks and transactions below
// execute the production market bodies against the real MM actor/event types.
extern "C" Actor* Actor_Spawn(ActorContext*, PlayState*, s16 id, f32 x, f32 y, f32 z,
                              s16 rotX, s16 rotY, s16 rotZ, s32 params) {
    assert(id == ACTOR_EN_ITEM00);
    if (!receiptSpawnWorks) return nullptr;
    assert(receiptSpawns < int(receiptActors.size()));
    auto& item = receiptActors[receiptSpawns++];
    item = {};
    item.actor.id = id; item.actor.params = params;
    item.actor.world.pos = {x, y, z};
    item.actor.home.rot = {rotX, rotY, rotZ};
    return &item.actor;
}
extern "C" void Actor_Kill(Actor* actor) { ++receiptKills; actor->update = nullptr; }
extern "C" PlayerExplosive Player_GetExplosiveHeld(Player*) { return PLAYER_EXPLOSIVE_NONE; }
void CustomItem00_Destroy(Actor*, PlayState*) { ++receiptDestroys; }
void DrawMarketReceipt(Actor*, PlayState*) {}

float sNESFontWidths[160]{}; // Actual native widths/decoder are exercised by run_text_codec_tests.py.
// Room rendering is exercised separately by run_granny_native_tests.py.
// Native camera setup has its own real-engine integration fixture.
void GrannySceneBegin() {}
void ApplyGrannyInterior() {}
void EndBrowsing(PlayState*,bool = false) { sTalking = false; }
struct ShelfFixtureStub { void Reset() {} } sShelfFixture, sCounterBarrier;
/* NATIVE_HALF_DAY_TABLE */
/* PRODUCTION */

void NativeOffer(EnDoor* current, PlayState* play) {
    Player* player = GET_PLAYER(play);
    player->doorType = PLAYER_DOORTYPE_HANDLE;
    /* NATIVE_DOOR_OFFER */
}

s32 NativeOpen(Player* current, PlayState* play, KnobDoorActor* knobDoor) {
    knobDoor->requestOpen = true;
    /* NATIVE_DOOR_OPEN */
    ++vanillaDoors;
    return true;
}
s32 NativeCaller(Player* current, PlayState* play, KnobDoorActor* knobDoor) {
    /* NATIVE_DOOR_CALLER */
    return true;
}

void Reset() {
    CancelReceipt(true);
    sReceipt = {};
    receiptInteractor.events.clear(); receiptInteractor.currentEvent = GIEventNone{};
    receiptActors = {};
    receiptTextCalls = activeReceiptMessages = ordinaryReceiptMessages = 0;
    receiptSpawns = receiptKills = receiptDestroys = 0;
    receiptCapeChoice = receiptCapturedBeforeGrant = false; receiptSpawnWorks = true;
    Rando::StaticData::Items.clear();
    Rando::StaticData::Items[RI_UNKNOWN] = {};
    Rando::StaticData::Items[RI_UNKNOWN].itemId = ITEM_NONE;
    CustomMessage::last = {};
    destination = true;
    diagnostics.clear();
    gComboGameMode = COMBO_GAME_BOTH;
    gSaveContext = {}; play = {}; player = {};
    gPlayState = &play; gGameState = &play.state;
    play.state.running = true;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    player.actor.category = ACTORCAT_PLAYER;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    gSaveContext.fileNum = 0;
    gSaveContext.save.shipSaveInfo.rando.finalSeed = 77;
    gSaveContext.save.saveInfo.playerData.health = 0x50;
    play.sceneId = COMBO_BLACK_MARKET_SCENE;
    sSession.Bind(Generate(77), 0); sSession.state = {};
    sClears = 3; sEntering = sReturnPending = false;
    sTravelSlot = -1; sTravelSeed = 0;
    sResumeFromMarket = false;
    sSoloReturn = {}; sSoloReturnYaw = -8192;
    sSoloReturnEntrance = ENTRANCE(WOODS_OF_MYSTERY, 0); sReturnStaged = false;
    owned = false; usable = grantWorks = pack = true;
    inCs = false; floorExists = true; flatFloor = {}; flatFloor.normal.y = 32767;
    player.actor.floorPoly = &flatFloor; player.actor.floorHeight = 0;
    grants = writes = 0; gSaveContext.save.saveInfo.playerData.rupees = 500;
    vanillaDoors = stoppedDoors = doorErrors = doorCutscenes = 0;
    sSelected = 0;
}

void TestPlacedDoor() {
    TransitionActorEntry transitions[2]{};
    KnobDoorActor door{};
    auto prepare = [&] {
        Reset();
        play.sceneId = SCENE_LOST_WOODS;
        play.roomCtx.curRoom.num = 0;
        gSaveContext.save.entrance = ENTRANCE(LOST_WOODS, 0);
        transitions[0] = {}; transitions[0].id = -ACTOR_EN_HOLL;
        transitions[0].sides[0].room = 1; transitions[0].sides[1].room = 0;
        auto& entry = transitions[1]; entry = {};
        entry.id = -ACTOR_EN_DOOR; entry.pos = {-680, 0, 72};
        entry.rotY = (106 << 7) | 127;
        entry.sides[0].room = entry.sides[1].room = 0;
        play.transitionActors.count = 2; play.transitionActors.list = transitions;
        door = {}; door.dyna.actor.id = ACTOR_EN_DOOR;
        door.dyna.actor.params = TRANSITION_ACTOR_PARAMS(1, 0);
        door.dyna.actor.world.pos = door.dyna.actor.home.pos = {-680, 0, 72};
        player.doorActor = &door.dyna.actor; player.actionFunc = Player_Action_36;
    };
    const s16 halfDays[] = {HALFDAYBIT_DAY0_DAWN, HALFDAYBIT_DAY0_NIGHT,
        HALFDAYBIT_DAY1_DAWN, HALFDAYBIT_DAY1_NIGHT, HALFDAYBIT_DAY2_DAWN, HALFDAYBIT_DAY2_NIGHT,
        HALFDAYBIT_DAY3_DAWN, HALFDAYBIT_DAY3_NIGHT, HALFDAYBIT_DAY4_DAWN, HALFDAYBIT_DAY4_NIGHT};
    // Execute the native offer branch, not just a manually preselected Open action.
    for (s16 halfDay : halfDays) {
        prepare();
        EnDoor offered{}; offered.knobDoor = door;
        offered.doorType = ENDOOR_TYPE_WHOLE_DAY; offered.typeVar.halfDayCheck = 0;
        play.actorCtx.halfDaysBit = halfDay;
        NativeOffer(&offered, &play);
        assert(player.doorType == PLAYER_DOORTYPE_HANDLE);
        // A different scene, transition index, or room pair retains its schedule.
        for (int other = 0; other < 3; ++other) {
            prepare(); offered.knobDoor = door;
            play.actorCtx.halfDaysBit = halfDay;
            if (other == 0) play.sceneId = SCENE_TOWN;
            if (other == 1) offered.knobDoor.dyna.actor.params = TRANSITION_ACTOR_PARAMS(0, 0);
            if (other == 2) transitions[1].sides[1].room = 1;
            NativeOffer(&offered, &play);
            const s16 nightBit = ((halfDay & HALFDAYBIT_DAWNS) >> 1) | (halfDay & HALFDAYBIT_NIGHTS);
            assert(player.doorType == ((nightBit & D_801AED48[0]) ?
                PLAYER_DOORTYPE_HANDLE : PLAYER_DOORTYPE_TALKING));
        }
    }
    prepare();
    const auto original = transitions[1];
    const auto respawn = gSaveContext;
    NativeCaller(&player, &play, &door);
    assert(sEntering && sTravelSlot == 0 && sTravelSeed == 77);
    assert(play.transitionTrigger == TRANS_TRIGGER_START && play.nextEntrance == COMBO_BLACK_MARKET_ENTRANCE);
    assert(play.transitionType == TRANS_TYPE_FADE_BLACK && door.requestOpen && vanillaDoors == 0);
    assert(doorCutscenes == 1 && (player.stateFlags1 & PLAYER_STATE1_20000000));
    assert(std::memcmp(&transitions[1], &original, sizeof(original)) == 0);
    assert(std::memcmp(&gSaveContext.respawn, &respawn.respawn, sizeof(respawn.respawn)) == 0);
    // Returning uses the authored door's outward direction, not the old shell.
    play.transitionTrigger = TRANS_TRIGGER_OFF;
    sReturnPending = true;
    SceneInit(SCENE_LOST_WOODS, 0);
    const float angle = 106 * (3.14159265359f / 180);
    assert(std::abs(player.actor.world.pos.x - (-680 + std::sin(angle) * 226)) < 0.1f);
    assert(std::abs(player.actor.world.pos.z - (72 + std::cos(angle) * 226)) < 0.1f);
    assert(player.actor.shape.rot.y == DEG_TO_BINANG(106));
    // A future Prelude translation/rotation drives the same native callback.
    prepare();
    transitions[1].id = ACTOR_EN_DOOR; // also valid before native spawning
    transitions[1].pos = {350, 10, -250}; transitions[1].rotY = (270 << 7) | 127;
    door.dyna.actor.home.pos = door.dyna.actor.world.pos = {350, 10, -250};
    BlackMarketDoorNative::Pose moved;
    assert(BlackMarketDoorNative::Find(&play, moved) && moved.yaw == DEG_TO_BINANG(270));
    NativeCaller(&player, &play, &door);
    assert(sEntering && vanillaDoors == 0);
    for (int reason = 0; reason < 3; ++reason) {
        prepare();
        if (reason == 0) destination = false;
        if (reason == 1) play.transitionTrigger = TRANS_TRIGGER_START;
        if (reason == 2) play.transitionMode = TRANS_MODE_SETUP;
        const auto previousTrigger = play.transitionTrigger;
        NativeCaller(&player, &play, &door);
        assert(!sEntering && play.transitionTrigger == previousTrigger && !door.requestOpen);
        assert(vanillaDoors == 0 && stoppedDoors == 1 && doorErrors == 1 && player.actionFunc == NativeIdle);
        assert(doorCutscenes == 0); // Rejected entry must not restart the infinite global door cutscene.
    }
    // Ordinary doors, other scenes and prologue visits retain the native path.
    for (int reason = 0; reason < 4; ++reason) {
        prepare();
        if (reason == 0) play.sceneId = SCENE_TOWN;
        if (reason == 1) transitions[1].sides[1].room = 1;
        if (reason == 2) gSaveContext.save.cutsceneIndex = 0xFFF0;
        if (reason == 3) door.dyna.actor.params = TRANSITION_ACTOR_PARAMS(0, 0);
        NativeCaller(&player, &play, &door);
        assert(!sEntering && vanillaDoors == 1 && door.requestOpen && stoppedDoors == 0);
    }
    puts("PASS authored native Open action, moved door/return, blocked-shop cleanup, ordinary-door and respawn preservation");
}

static Actor* OfferFixtureReceipt() {
    assert(sReceipt.pending && !sReceipt.delivered);
    const int before = receiptSpawns;
    OfferMarketReceipt(&play, &player);
    assert(receiptSpawns == before + 1 && sReceiptActor);
    auto* actor = sReceiptActor;
    assert(actor->home.rot.z == sReceipt.ticket && sReceiptActorTicket == sReceipt.ticket);
    assert(actor->draw == DrawMarketReceipt && actor->destroy == MarketReceiptDestroyed);
    assert(reinterpret_cast<ActorFunc>(reinterpret_cast<EnItem00*>(actor)->actionFunc) == GiveMarketReceipt);
    const auto* event = std::get_if<GIEventGiveItem>(&receiptInteractor.currentEvent);
    assert(event && event->giveItem == GiveMarketReceipt && event->drawItem == DrawMarketReceipt);
    assert(event->param == sReceipt.ticket);
    return actor;
}
static void OtherReceipt(Actor*, PlayState*) {}
static void TestDeferredPurchases() {
    Reset();
    const int price = sSession.market["stock"][0]["price"];
    receiptCapeChoice = true;
    assert(Purchase());
    const auto ticket = sReceipt.ticket;
    assert(sReceipt.pending && !sReceipt.delivered && sReceipt.slot == 0);
    assert(grants == 0 && writes == 0 && sSession.state.purchased == 0);
    assert(gSaveContext.save.saveInfo.playerData.rupees == 500 && receiptTextCalls == 0);
    assert(!Purchase() && sReceipt.ticket == ticket); // A second confirmation cannot replace an outstanding receipt.
    // The ordinary scheduler retains its own pending/current events.
    receiptInteractor.events.emplace_back(GIEventTrap{});
    OfferMarketReceipt(&play, &player);
    assert(!sReceiptActor && receiptSpawns == 0 && receiptInteractor.events.size() == 1);
    receiptInteractor.events.clear();
    receiptInteractor.currentEvent = GIEventTrap{};
    OfferMarketReceipt(&play, &player);
    assert(!sReceiptActor && receiptSpawns == 0 && std::holds_alternative<GIEventTrap>(receiptInteractor.currentEvent));
    receiptInteractor.currentEvent = GIEventNone{};
    receiptSpawnWorks = false;
    OfferMarketReceipt(&play, &player);
    assert(!sReceiptActor && sReceipt.pending && grants == 0 && writes == 0);
    receiptSpawnWorks = true;
    Actor* actor = OfferFixtureReceipt();
    assert(actor->home.rot.x & CustomItem::GIVE_ITEM_CUTSCENE);
    assert(!(actor->home.rot.x & CustomItem::GIVE_OVERHEAD));
    assert(grants == 0 && writes == 0 && activeReceiptMessages == 0);
    // Wrong actor and wrong ticket must not acknowledge the queued purchase.
    Actor unrelated{};
    GiveMarketReceipt(&unrelated, &play);
    actor->home.rot.z = ticket == 32767 ? 1 : ticket + 1;
    GiveMarketReceipt(actor, &play);
    assert(grants == 0 && writes == 0 && !sReceipt.delivered);
    actor->home.rot.z = ticket;
    sSelected = 1; // The acknowledged receipt retains its original shelf.
    GiveMarketReceipt(actor, &play);
    assert(grants == 1 && writes == 1 && sReceipt.delivered);
    assert(persisted["rupees"] == 500 - price && persisted["market"]["state"]["purchased"] == 1);
    assert(activeReceiptMessages == 1 && ordinaryReceiptMessages == 0 && receiptTextCalls == 1);
    assert(receiptCapturedBeforeGrant && CustomMessage::last.capeVisibilityChoice);
    GiveMarketReceipt(actor, &play);
    assert(grants == 1 && writes == 1 && activeReceiptMessages == 1);
    MarketReceiptDestroyed(actor, &play);
    assert(!sReceiptActor && receiptDestroys == 1 && std::holds_alternative<GIEventNone>(receiptInteractor.currentEvent));
    CancelReceipt(true);
    sSelected = 0;
    assert(!Purchase() && grants == 1 && writes == 1);

    Reset(); grantWorks = false;
    assert(Purchase()); actor = OfferFixtureReceipt();
    GiveMarketReceipt(actor, &play);
    assert(grants == 0 && writes == 0 && !sReceipt.pending && sSession.state.purchased == 0);
    assert(gSaveContext.save.saveInfo.playerData.rupees == 500);
    assert(std::holds_alternative<GIEventGiveItem>(receiptInteractor.currentEvent)); // Callback still owns the event.
    MarketReceiptDestroyed(actor, &play);
    assert(std::holds_alternative<GIEventNone>(receiptInteractor.currentEvent));

    // Native receipt fields must survive both GI modes; arrow eligibility is captured before the grant.
    for (bool alreadyOwned : {false, true}) {
        Reset();
        Rando::StaticData::Items[RI_UNKNOWN].itemId = ITEM_ARROW_FIRE;
        gSaveContext.save.saveInfo.inventory.items[SLOT_ARROW_FIRE] = alreadyOwned ? ITEM_ARROW_FIRE : ITEM_NONE;
        assert(Purchase()); actor = OfferFixtureReceipt();
        GiveMarketReceipt(actor, &play);
        assert(grants == 1 && writes == 1 && activeReceiptMessages == 1);
        assert(CustomMessage::last.swordElementTutorial == (alreadyOwned ? SWORD_ELEMENT_NONE : SWORD_ELEMENT_FIRE));
    }
    Reset(); receiptCapeChoice = true; player.stateFlags1 = PLAYER_STATE1_CHARGING_SPIN_ATTACK;
    assert(Purchase()); actor = OfferFixtureReceipt();
    assert((actor->home.rot.x & CustomItem::GIVE_OVERHEAD) && !(actor->home.rot.x & CustomItem::GIVE_ITEM_CUTSCENE));
    GiveMarketReceipt(actor, &play);
    assert(grants == 1 && writes == 1 && ordinaryReceiptMessages == 1 && activeReceiptMessages == 0);
    assert(CustomMessage::last.capeVisibilityChoice);

    Reset(); gSaveContext.rupeeAccumulator = -450;
    const bool affordable = Purchase();
    if (price > 50) assert(!affordable && !sReceipt.pending && grants == 0 && writes == 0);
    else assert(affordable && grants == 0 && writes == 0);
    Reset(); sClears = 0;
    for (int i = 0; i < 8; ++i) if (sSession.market["stock"][i]["clears"] != 0) {
        sSelected = i; assert(!Purchase()); assert(grants == 0 && writes == 0 && !sReceipt.pending);
    }
    // Revalidate availability immediately before the GI acknowledgment.
    for (int unavailable = 0; unavailable < 3; ++unavailable) {
        Reset(); assert(Purchase()); actor = OfferFixtureReceipt();
        if (unavailable == 0) owned = true;
        if (unavailable == 1) usable = false;
        if (unavailable == 2) gSaveContext.rupeeAccumulator = -500;
        GiveMarketReceipt(actor, &play);
        assert(grants == 0 && writes == 0 && !sReceipt.pending && sSession.state.purchased == 0);
        assert(gSaveContext.save.saveInfo.playerData.rupees == 500);
    }
    // Context changes retire the receipt without touching another save/market/transition.
    for (int context = 0; context < 8; ++context) {
        Reset(); assert(Purchase()); actor = OfferFixtureReceipt();
        if (context == 0) gSaveContext.fileNum = 1;
        if (context == 1) gSaveContext.save.shipSaveInfo.rando.finalSeed = 78;
        if (context == 2) sSession.market = Generate(78);
        if (context == 3) play.transitionTrigger = TRANS_TRIGGER_START;
        if (context == 4) play.transitionMode = TRANS_MODE_SETUP;
        if (context == 5) play.gameOverCtx.state = 1;
        if (context == 6) gGameState = nullptr;
        if (context == 7) play.state.running = false;
        GiveMarketReceipt(actor, &play);
        assert(grants == 0 && writes == 0 && sSession.state.purchased == 0);
        assert(gSaveContext.save.saveInfo.playerData.rupees == 500);
        CancelReceipt();
        assert(!sReceipt.pending && !sReceiptActor && receiptKills == 1);
    }
    // A deferred destructor from a cancelled actor cannot release a newer ticket.
    Reset(); assert(Purchase()); Actor* stale = OfferFixtureReceipt();
    const auto staleTicket = sReceipt.ticket;
    CancelReceipt();
    assert(!sReceipt.pending && !sReceiptActor && receiptKills == 1);
    assert(Purchase() && sReceipt.ticket != staleTicket);
    actor = OfferFixtureReceipt();
    GiveMarketReceipt(stale, &play);
    MarketReceiptDestroyed(stale, &play);
    assert(grants == 0 && writes == 0 && sReceiptActor == actor);
    const auto* current = std::get_if<GIEventGiveItem>(&receiptInteractor.currentEvent);
    assert(current && current->param == sReceipt.ticket);
    GiveMarketReceipt(actor, &play);
    assert(grants == 1 && writes == 1);
    // Cancellation must not clear an unrelated scheduler owner, even with the same numeric ticket.
    receiptInteractor.currentEvent = GIEventGiveItem{
        .showGetItemCutscene = true, .param = static_cast<s16>(sReceipt.ticket),
        .giveItem = OtherReceipt, .drawItem = nullptr,
    };
    CancelReceipt();
    current = std::get_if<GIEventGiveItem>(&receiptInteractor.currentEvent);
    assert(current && current->giveItem == OtherReceipt && !sReceiptActor && !sReceipt.pending);
    MarketReceiptDestroyed(actor, &play);
    current = std::get_if<GIEventGiveItem>(&receiptInteractor.currentEvent);
    assert(current && current->giveItem == OtherReceipt);
    puts("PASS deferred native GI acknowledgment, receipt tutorials, exactly-once debit/save, revalidation and stale actor ownership");
}

#include "interior_exit_cases.inc"
int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "door-exit") { TestInteriorExitDoor(); return 0; }
    TestPlacedDoor();
    Reset();
    u16 textId = kBrowseText; bool fromTable = true;
    MarketText(&textId, &fromTable);
    assert(!fromTable && CustomMessage::last.msg.find("\xB0 Select   \xB1 Leave") != std::string::npos);
    textId = kConfirmText; fromTable = true; MarketText(&textId,&fromTable);
    assert(!fromTable && CustomMessage::last.msg.find("\xC2I'll buy it\x11No thanks") != std::string::npos);
    textId = kWelcomeText; fromTable = true; MarketText(&textId,&fromTable);
    assert(!fromTable && CustomMessage::last.msg.find("Continue") != std::string::npos &&
           CustomMessage::last.msg.find("Talk") == std::string::npos);
    textId = kKeeperHelpChoiceText; fromTable = true; MarketText(&textId,&fromTable);
    assert(!fromTable && CustomMessage::last.msg.find("How does this shop work?") != std::string::npos &&
           CustomMessage::last.msg.find("Back") != std::string::npos);
    textId = kKeeperHelpText; fromTable = true; MarketText(&textId,&fromTable);
    assert(!fromTable && CustomMessage::last.msg.find("unique dungeon clears") != std::string::npos &&
           CustomMessage::last.msg.find("empty bottle") != std::string::npos);
    TestDeferredPurchases();
    Reset();
    gSaveContext.save.entrance = COMBO_BLACK_MARKET_ENTRANCE;
    const auto protectedSave = gSaveContext.save.saveInfo;
    const auto protectedTop = gSaveContext.respawn[RESPAWN_MODE_TOP];
    ComboBlackMarket_NormalizeEntry();
    assert(gSaveContext.save.entrance == ENTRANCE(LOST_WOODS, 0) && sReturnPending);
    assert(!std::memcmp(&protectedSave, &gSaveContext.save.saveInfo, sizeof(protectedSave)));
    assert(!std::memcmp(&protectedTop, &gSaveContext.respawn[RESPAWN_MODE_TOP], sizeof(protectedTop)));
    Reset(); sEntering = true; sTravelSlot = 0; sTravelSeed = 77;
    gSaveContext.save.entrance = COMBO_BLACK_MARKET_ENTRANCE;
    ComboBlackMarket_NormalizeEntry(); assert(gSaveContext.save.entrance == COMBO_BLACK_MARKET_ENTRANCE);
    gSaveContext.respawnFlag = 1;
    ComboBlackMarket_NormalizeEntry(); assert(gSaveContext.save.entrance == ENTRANCE(LOST_WOODS, 0));
    Reset(); sEntering = true; sTravelSlot = 1; sTravelSeed = 77;
    gSaveContext.save.entrance = COMBO_BLACK_MARKET_ENTRANCE;
    ComboBlackMarket_NormalizeEntry();
    assert(gSaveContext.save.entrance == ENTRANCE(LOST_WOODS, 0));
    Reset(); sEntering = true; sTravelSlot = 0; sTravelSeed = 78;
    gSaveContext.save.entrance = COMBO_BLACK_MARKET_ENTRANCE;
    ComboBlackMarket_NormalizeEntry();
    assert(gSaveContext.save.entrance == ENTRANCE(LOST_WOODS, 0));
    for (int i = 0; i < 4; ++i) {
        Reset(); sEntering = true; sTravelSlot = 0; sTravelSeed = 77;
        gSaveContext.save.entrance = COMBO_BLACK_MARKET_ENTRANCE;
        if (i == 0) destination = false;
        if (i == 1) sTravelSlot = 1;
        if (i == 2) gSaveContext.respawnFlag = -1;
        if (i == 3) gSaveContext.fileNum = 2;
        ComboBlackMarket_NormalizeEntry();
        assert(gSaveContext.save.entrance == ENTRANCE(LOST_WOODS, 0));
    }
    assert(ComboBlackMarket_InDoor(COMBO_BLACK_MARKET_DOOR_X, 0, COMBO_BLACK_MARKET_DOOR_Z, 0));
    assert(!ComboBlackMarket_InDoor(COMBO_BLACK_MARKET_DOOR_X - 226, 0, COMBO_BLACK_MARKET_DOOR_Z, 0));
    assert(ComboBlackMarket_AtExit(-11, 0, 410)); // native customer doorway, Z420
    assert(!ComboBlackMarket_AtExit(-11, 0, 381)); // safe native entry
    assert(!ComboBlackMarket_AtExit(0, 0, -260)); // staff area is not an exit
    assert(!ComboBlackMarket_AtExit(60, 0, 410)); // beside the door
    // The final authored scene owns the new door. The retired placeholder
    // trigger on the far side of the clearing must no longer enter the shop.
    Reset();
    play.sceneId = SCENE_LOST_WOODS;
    play.roomCtx.curRoom.num = 0;
    gSaveContext.save.entrance = ENTRANCE(LOST_WOODS, 0);
    TransitionActorEntry authored{};
    authored.id = -ACTOR_EN_DOOR; // native spawn marks transition entries negative
    authored.pos = {-680, 0, 72};
    authored.rotY = (106 << 7) | 127;
    authored.sides[0].room = authored.sides[1].room = 0;
    play.transitionActors.count = 1;
    play.transitionActors.list = &authored;
    sDoorArmed = true;
    player.actor.world.pos = {410, 0, -300};
    PlayerUpdate(&player.actor);
    assert(!sEntering && play.transitionTrigger == TRANS_TRIGGER_OFF);
    Reset();
    gSaveContext.save.entrance = COMBO_BLACK_MARKET_ENTRANCE;
    sSession.state.purchased = 0xA5;
    nlohmann::json saved;
    ComboBlackMarket_SaveState(saved,0);
    assert(saved["blackMarket"]["savedInside"] == true);
    sSession.state = {};
    ComboBlackMarket_LoadState(saved,0);
    assert(sSession.state.purchased == 0xA5);
    gSaveContext.save.entrance = ENTRANCE(SOUTH_CLOCK_TOWN,0);
    ComboBlackMarket_ApplyResume(1);
    assert(gSaveContext.save.entrance == ENTRANCE(LOST_WOODS,0) && sReturnPending);
    assert(sTravelSlot == 0 && sTravelSeed == 77);
    Reset();
    ComboBlackMarket_LoadState(saved,0);
    gSaveContext.save.entrance = ENTRANCE(SOUTH_CLOCK_TOWN,0);
    ComboBlackMarket_ApplyResume(0);
    assert(gSaveContext.save.entrance == ENTRANCE(SOUTH_CLOCK_TOWN,0) && !sReturnPending);
    Reset();
    nlohmann::json copied = saved;
    ComboBlackMarket_SaveState(copied,1);
    assert(copied == saved); // another slot's copy/migration must not erase purchases
    ComboBlackMarket_LoadState(saved,1);
    ComboBlackMarket_ApplyResume(1);
    assert(!sReturnPending && sSession.state.purchased == 0);
    Reset();
    gSaveContext.save.entrance = ENTRANCE(CLOCK_TOWER_INTERIOR,0);
    nlohmann::json cycle;
    ComboBlackMarket_SaveState(cycle,0);
    assert(cycle["blackMarket"]["savedInside"] == false);
    Reset();
    gComboGameMode = COMBO_GAME_MM;
    const auto soloMarket = Generate(77, OptionsFromJson({ { "games", "mm" } }));
    nlohmann::json soloSave = { { "blackMarket", { { "market", soloMarket },
        { "state", { { "purchased", 0x5A } } }, { "clears", 0x500 } } } };
    sSession.Bind(nullptr, -1);
    ComboBlackMarket_LoadState(soloSave, 0);
    assert(sSession.Active(0) && sSession.market == soloMarket);
    assert(sSession.state.purchased == 0x5A && sClears == 0x500);
    nlohmann::json soloRoundTrip;
    ComboBlackMarket_SaveState(soloRoundTrip, 0);
    assert(soloRoundTrip["blackMarket"]["clears"] == 0x500);
    nativeSavedDisk = soloSave;
    nativeSavedDisk["blackMarket"]["savedInside"] = true;
    gSaveContext.save.entrance = ENTRANCE(CLOCK_TOWER_INTERIOR, 0);
    LoadSolo(0);
    assert(sResumeFromMarket && !sReturnStaged);
    // Remember Save Location runs later and loads all three native respawns.
    gSaveContext.respawn[RESPAWN_MODE_RETURN].pos = { 44, 55, 66 };
    const auto loadedReturn = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    ComboBlackMarket_NormalizeEntry();
    assert(gSaveContext.save.entrance == COMBO_SOLO_FOREST_MM_ENTRANCE);
    assert(sReturnStaged && gSaveContext.respawn[RESPAWN_MODE_RETURN].roomIndex == 0);
    assert(!std::memcmp(&sPreservedReturn, &loadedReturn, sizeof(loadedReturn)));
    play.sceneId = COMBO_SOLO_FOREST_MM_SCENE; play.roomCtx.curRoom.num = 0;
    SceneInit(COMBO_SOLO_FOREST_MM_SCENE, 0);
    assert(!std::memcmp(&gSaveContext.respawn[RESPAWN_MODE_RETURN], &loadedReturn, sizeof(loadedReturn)));
    play.sceneId = COMBO_BLACK_MARKET_SCENE;
    nlohmann::json nativeCopy = soloSave;
    gGameState = nullptr;
    ComboBlackMarket_SaveState(nativeCopy, 0);
    assert(nativeCopy == soloSave);
    gGameState = &play.state;
    assert(Interior());
    play.sceneId = COMBO_SOLO_FOREST_MM_SCENE;
    play.roomCtx.curRoom.num = 0;
    assert(SoloExterior(&play) && Exterior(&play));
    play.sceneId = SCENE_26SARUNOMORI; assert(!SoloExterior(&play));
    play.sceneId = COMBO_SOLO_FOREST_MM_SCENE;
    assert(!ComboOpeningForest_IsExploration(&play));
    assert(Door().x == 410 && Door().z == -300);
    const auto soloTop = gSaveContext.respawn[RESPAWN_MODE_TOP];
    gSaveContext.respawn[RESPAWN_MODE_RETURN].pos = { 44, 55, 66 };
    const auto soloReturn = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    gSaveContext.save.entrance = COMBO_BLACK_MARKET_ENTRANCE;
    ComboBlackMarket_NormalizeEntry();
    assert(gSaveContext.save.entrance == COMBO_SOLO_FOREST_MM_ENTRANCE);
    assert(gSaveContext.respawnFlag == 2 && sReturnStaged);
    assert(gSaveContext.respawn[RESPAWN_MODE_RETURN].roomIndex == 0);
    assert(!std::memcmp(&soloTop, &gSaveContext.respawn[RESPAWN_MODE_TOP], sizeof(soloTop)));
    assert(!std::memcmp(&soloReturn, &sPreservedReturn, sizeof(soloReturn)));
    gComboGameMode = COMBO_GAME_OOT;
    play.sceneId = COMBO_BLACK_MARKET_SCENE;
    assert(!Interior() && !SoloExterior(&play));
    gComboGameMode = COMBO_GAME_MM;
    // Binding a native solo save cannot accept another game's frozen stock.
    soloSave["blackMarket"]["market"] = Generate(77, OptionsFromJson({ { "games", "oot" } }));
    ComboBlackMarket_LoadState(soloSave, 0);
    assert(!sSession.Active(0));
    Reset(); gComboGameMode = COMBO_GAME_MM;
    play.sceneId = COMBO_SOLO_FOREST_MM_SCENE; play.roomCtx.curRoom.num = 0;
    player.actor.world.pos = { 0, 500, 0 }; PlayerUpdate(&player.actor);
    assert(sSoloReturn.y == 0);
    Reset();
    gComboGameMode = COMBO_GAME_MM;
    play.sceneId = COMBO_SOLO_FOREST_MM_SCENE; play.roomCtx.curRoom.num = 0;
    gSaveContext.save.entrance = COMBO_SOLO_FOREST_MM_ENTRANCE;
    player.actor.world.pos = { 0, 0, 0 };
    PlayerUpdate(&player.actor);
    assert(sDoorArmed && play.transitionTrigger == TRANS_TRIGGER_OFF);
    const auto captured = player.actor.world.pos;
    player.actor.world.pos = { 410, 0, -300 };
    PlayerUpdate(&player.actor);
    assert(sEntering && play.nextEntrance == COMBO_BLACK_MARKET_ENTRANCE);
    assert(play.transitionTrigger == TRANS_TRIGGER_START && gComboGameMode == COMBO_GAME_MM);
    play.transitionTrigger = TRANS_TRIGGER_OFF;
    play.sceneId = COMBO_BLACK_MARKET_SCENE;
    player.actor.world.pos = { -11, 0, 410 };
    PlayerUpdate(&player.actor);
    assert(sReturnPending && play.nextEntrance == COMBO_SOLO_FOREST_MM_ENTRANCE);
    gSaveContext.save.entrance = play.nextEntrance;
    ComboBlackMarket_NormalizeEntry();
    play.sceneId = COMBO_SOLO_FOREST_MM_SCENE;
    SceneInit(COMBO_SOLO_FOREST_MM_SCENE, 0);
    assert(!sReturnPending && !sReturnStaged);
    assert(player.actor.world.pos.x == captured.x && player.actor.world.pos.z == captured.z);
    for (int gate = 0; gate < 8; ++gate) {
        Reset(); gComboGameMode = COMBO_GAME_MM;
        play.sceneId = COMBO_SOLO_FOREST_MM_SCENE; play.roomCtx.curRoom.num = 0;
        player.actor.world.pos = { 410, 0, -300 }; sDoorArmed = true;
        if (gate == 0) destination = false;
        if (gate == 1) play.transitionMode = TRANS_MODE_SETUP;
        if (gate == 2) play.pauseCtx.state = PAUSE_STATE_MAIN;
        if (gate == 3) inCs = true;
        if (gate == 4) play.roomCtx.curRoom.num = 1;
        if (gate == 5) gComboGameMode = COMBO_GAME_OOT;
        if (gate == 6) floorExists = false;
        if (gate == 7) gSaveContext.fileNum = 0xFF;
        PlayerUpdate(&player.actor);
        assert(!sEntering && play.transitionTrigger == TRANS_TRIGGER_OFF);
    }
    std::puts("PASS market native transactions and local entry/reload/void recovery; slot/seed and TOP isolation");
}
