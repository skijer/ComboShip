#include <nlohmann/json.hpp>
#include "2s2h/Rando/Rando.h"
#include "2s2h/Rando/ItemReceiptText.h"
#include "2s2h/CustomMessage/CustomMessage.h"
#include "sword_element_message.h"
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#ifdef COMBO_BUILD
#include "rando/CrossForeign.h"
#endif
using namespace std::string_literals;

extern "C" {
SaveContext gSaveContext{};
PlayState* gPlayState = nullptr;
#ifdef COMBO_BUILD
int gComboGameMode = COMBO_GAME_BOTH;
#endif
}
static PlayState play{};
static Player player{};
static int options[4], flags, param = RC_UNKNOWN, param2, grants, opens, notices;
static bool denyGrant, showReceipt = true, customReceipt = false;
static CwItemReceiptPresentation loadedPresentation{};
#ifdef COMBO_BUILD
int gMMComboGoalRequired;
int (*gMMComboOtherTriforceCount)() = nullptr;
void MMAnchor_BroadcastCrossItem(int, const char*, const char*) {}
void SaveManager_SaveCurrentForCombo() {}
// Arrow receipts must not consume a Mirror stump reward token.
extern "C" bool MagicMirror_CommitStumpRewardToken(void) { assert(false); return false; }
#endif
static const char* kCustom = "Custom arrow receipt preserved.\x11Progressive description preserved.";
extern CustomMessage::Entry activeCustomMessage;
#define CUSTOM_ITEM_FLAGS flags
#define CUSTOM_ITEM_PARAM param
#define CUSTOM_ITEM_PARAM2 param2
namespace CustomItem { constexpr int GIVE_ITEM_CUTSCENE = 1; }

extern "C" int32_t CVarGetInteger(const char* key, int32_t fallback) {
    if (!strcmp(key, "gEnhancements.DinFireSwordDamage")) return options[1];
    if (!strcmp(key, "gEnhancements.IceSwordDamage")) return options[2];
    if (!strcmp(key, "gEnhancements.LightSwordDamage")) return options[3];
    return fallback;
}
extern "C" void Message_SetItemReceiptPresentation(const CwItemReceiptPresentation* value) {
    loadedPresentation = value ? *value : CwItemReceiptPresentation{};
}
extern "C" void Message_SetCapeVisibilityChoice(int value) { assert(!value); }
extern "C" void Message_StartTextbox(PlayState* state, u16 textId, Actor*) {
    assert(textId == CUSTOM_MESSAGE_ID && grants == 0);
    assert(state == &play);
    ++opens;
    CustomMessage::LoadCustomMessageIntoFont(activeCustomMessage);
}

#include "sword_element_queue_catalog.inc"
namespace Rando::StaticData {
#ifdef COMBO_BUILD
std::string GetItemName(RandoItemId id, bool, RandoCheckId, bool) {
#else
std::string GetItemName(RandoItemId id, bool, RandoCheckId) {
#endif
    return Items.at(id).name;
}
uint8_t GetIconForZMessage(RandoItemId id) { return static_cast<uint8_t>(Items.at(id).getItemId); }
bool ShouldShowGetItemCutscene(RandoItemId) { return showReceipt; }
const char* GetIconTexturePath(RandoItemId) { return "arrow-icon"; }
const std::string& GetCheckDisplayName(RandoCheckId) { static std::string name = "arrow check"; return name; }
}
namespace Notification {
struct Info { const char* itemIcon; std::string message, suffix; };
void Emit(Info) { ++notices; }
}
namespace Rando {
RandoItemId ConvertItem(RandoItemId id, RandoCheckId) { return id; }
RandoItemId CurrentJunkItem(RandoCheckId) { return RI_RUPEE_GREEN; }
bool ApplyItemReceiptText(RandoItemId, CustomMessage::Entry& entry) {
    if (!customReceipt) return false;
    entry.msg = std::string(kCustom) + '\277';
    entry.autoFormat = false;
    entry.firstItemCost = 37;
    entry.receiptPresentation.singleBox = 1;
    entry.receiptPresentation.rewardLine = 1;
    return true;
}
void AppendReceiptSource(CustomMessage::Entry& entry, const std::string& source) {
    if (source.empty()) return;
    if (!entry.msg.empty() && uint8_t(entry.msg.back()) == 0xBF) entry.msg.pop_back();
    entry.msg += source;
    if (!entry.autoFormat) entry.msg += '\277';
}
void GiveItem(RandoItemId id, RandoCheckId) {
    ++grants;
    if (denyGrant) return;
    const auto item = StaticData::Items.at(id).itemId;
    auto& inventory = gSaveContext.save.saveInfo.inventory;
    if (item == ITEM_ARROW_FIRE) inventory.items[SLOT_ARROW_FIRE] = item;
    if (item == ITEM_ARROW_ICE) inventory.items[SLOT_ARROW_ICE] = item;
    if (item == ITEM_ARROW_LIGHT) inventory.items[SLOT_ARROW_LIGHT] = item;
}
#ifdef COMBO_BUILD
void LatchComboForeign(RandoCheckId) {}
const char* ComboForeignLatchedName(RandoCheckId) { return "Fire Arrows"; }
uint8_t ComboForeignMessageIcon(RandoCheckId) { return 7; }
bool ApplyForeignItemReceiptText(const char*, CustomMessage::Entry&, RandoCheckId) { return false; }
#endif
namespace MiscBehavior {
static bool queued = true;
static RandoCheckId queuedCheck = RC_UNKNOWN;
std::string BankRewardSourceSuffix(RandoCheckId) { return " (Bank reward)"; }
std::string GetTrapMessage() { return "Trap"; }
#ifdef COMBO_BUILD
const ComboRando::ForeignItem* MM_LookupForeign(RandoCheckId) { return nullptr; }
bool ShouldShowForeignCutscene(RandoCheckId) { return true; }
void SendForeignCheck(RandoCheckId) { assert(false); }
void BroadcastCheckObtainedIfFirst(RandoCheckId, RandoItemId, bool) {}
void OfferTrapItem() { assert(false); }
#endif
/* QUEUE_RECEIVE */
}
}

static std::string Loaded() {
    return std::string(reinterpret_cast<const char*>(play.msgCtx.font.msgBuf.schar), play.msgCtx.msgLength);
}

static void Reset(RandoItemId id, int element, bool enabled, bool owned, bool overhead) {
    std::memset(&gSaveContext, 0, sizeof(gSaveContext));
    std::memset(&play, 0, sizeof(play));
    std::memset(options, 0, sizeof(options));
    std::memset(gSaveContext.save.saveInfo.inventory.items, ITEM_NONE, 24);
    gPlayState = &play;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    options[element] = enabled;
    flags = overhead ? 0 : CustomItem::GIVE_ITEM_CUTSCENE;
    param = RC_UNKNOWN;
    param2 = 0;
    grants = opens = notices = 0;
    denyGrant = false;
    showReceipt = true;
    gSaveContext.options.language = LANGUAGE_ENG;
    if (owned) {
        const int slots[] = { 0, SLOT_ARROW_FIRE, SLOT_ARROW_ICE, SLOT_ARROW_LIGHT };
        gSaveContext.save.saveInfo.inventory.items[slots[element]] = Rando::StaticData::Items.at(id).itemId;
    }
    auto& check = RANDO_SAVE_CHECKS[param];
    check.randoItemId = id;
    check.eligible = true;
    activeCustomMessage = {};
    play.msgCtx.msgLength = 4;
    std::memcpy(play.msgCtx.font.msgBuf.schar, "old\277", 4);
}

int main() {
    const RandoItemId ids[] = { RI_UNKNOWN, RI_ARROW_FIRE, RI_ARROW_ICE, RI_ARROW_LIGHT };
    for (int element = 1; element <= 3; ++element)
        for (bool enabled : { false, true })
            for (bool owned : { false, true })
                for (bool overhead : { false, true })
                    for (bool authored : { false, true })
                        for (bool denied : { false, true }) {
                            customReceipt = authored;
                            Reset(ids[element], element, enabled, owned, overhead);
                            denyGrant = denied;
                            Rando::MiscBehavior::Receive(nullptr, &play);
                            assert(grants == 1 && notices == 0);
                            assert(RANDO_SAVE_CHECKS[RC_UNKNOWN].obtained && !RANDO_SAVE_CHECKS[RC_UNKNOWN].eligible);
                            assert(activeCustomMessage.swordElementTutorial == (!owned ? element : 0));
                            if (!overhead) CustomMessage::LoadCustomMessageIntoFont(activeCustomMessage);
                            assert(opens == (overhead ? 1 : 0));
                            const auto body = Loaded();
                            assert((body.find("cost no magic.") != std::string::npos) == (!owned && !denied));
                            auto flat = body;
                            for (char& c : flat) if (c == '\021') c = ' ';
                            assert(flat.find("Bank reward") != std::string::npos);
                            assert(uint8_t(body[2]) == Rando::StaticData::Items.at(ids[element]).getItemId);
                            if (authored) {
                                assert(body.find(kCustom) == 11);
                                assert(uint8_t(body[5]) == 0 && uint8_t(body[6]) == 37);
                                assert(loadedPresentation.singleBox == 1 && loadedPresentation.rewardLine == 1);
                            } else if (overhead) {
                                assert(body.compare(body.size() - 4, 4, "\034\002\020\277", 4) == 0);
                            }
                        }
    // Notification-only delivery must never mutate a stale native text buffer.
    customReceipt = false;
    Reset(RI_ARROW_FIRE, 1, true, false, true);
    showReceipt = false;
    const auto before = Loaded();
    Rando::MiscBehavior::Receive(nullptr, &play);
    assert(opens == 0 && notices == 1 && grants == 1 && Loaded() == before);
    // Save imports, sync and ordinary custom messages do not carry pickup eligibility.
    CustomMessage::Entry ordinary;
    ordinary.msg = "Ordinary message";
    CustomMessage::LoadCustomMessageIntoFont(ordinary);
    assert(Loaded().find("cost no magic.") == std::string::npos);
    std::cout << "PASS real queued receipt callback + message loader: captured first pickup, metadata, overhead fade and no extra grant\n";
}
