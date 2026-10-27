#include "global.h"
#include "message_data_static.h"
#include "2s2h/CustomMessage/CustomMessage.h"
#include "2s2h/CustomItem/CustomItem.h"
#include "2s2h/Enhancements/Gameplay/ComboForestActors.h"
#include <nlohmann/json.hpp>
#include "2s2h/Rando/Rando.h"
#include "2s2h/Rando/ItemReceiptText.h"
#include "ComboCapeReceiptChoice.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

SaveContext gSaveContext{};
static RegEditor registers{};
RegEditor* gRegEditor = &registers;
s16 gItemPrices[256]{};
u32 gBitFlags[32]{};
PlayState* gPlayState;
static PlayState play;
static Player player;
static bool customText;
static u16 customId;
static std::string customBody;
static bool sariaReceipt;
static CustomMessage::Entry shownReceipt;
constexpr u16 kText = 0x8F28, kTeachText = 0x8F29, kTaughtText = 0x8F2A;
extern "C" f32 sNESFontWidths[160];
extern "C" const char* fontTbl[156];
extern "C" void Message_SetItemReceiptPresentation(const CwItemReceiptPresentation*);
extern "C" void Message_SetCapeVisibilityChoice(int);
extern "C" void Message_OpenText(PlayState*, u16);
extern "C" void Message_Decode(PlayState*);
extern "C" void SwordElement_MessageAppend(PlayState*, u8) {}

/* CUSTOM_MESSAGE_HELPERS */

static auto sariaHook = /* SARIA_HOOK */;
extern "C" void GameInteractor_ExecuteOnOpenText(u16* textId, bool* fromTable) {
    if (!customText)
        return;
    if (sariaReceipt) {
        assert(*textId == CUSTOM_MESSAGE_ID);
        CustomMessage::LoadCustomMessageIntoFont(shownReceipt);
        *fromTable = false;
        return;
    }
    if (*textId >= kText && *textId <= kTaughtText) {
        sariaHook(textId, fromTable);
    } else {
        assert(*textId == customId);
        CustomMessage::Entry entry;
        entry.msg = customBody;
        CustomMessage::LoadCustomMessageIntoFont(entry);
        *fromTable = false;
    }
}
extern "C" void Interface_SetHudVisibility(u16) {}
extern "C" void Interface_SetBButtonInterfaceDoAction(PlayState*, s16) {}
extern "C" u8 ExtInv_GetItemIconSize(u16) { return 32; }
extern "C" void* ExtInv_GetItemIcon(u16) { return nullptr; }
extern "C" bool GameInteractor_Should(GIVanillaBehavior, uint32_t result, ...) { return result; }
extern "C" s32 Map_CurRoomHasMapI(PlayState*) { return 0; }
extern "C" s32 Flags_GetAllTreasure(PlayState*) { return 0; }
extern "C" s16 Inventory_GetSkullTokenCount(s16) { return 0; }
extern "C" void Audio_PlaySfx(u16) {}
extern "C" int32_t CVarGetInteger(const char*, int32_t defaultValue) { return defaultValue; }

static GameInteractor interactor;
GameInteractor* GameInteractor::Instance = &interactor;
static void GiveSariaSong(Actor*, PlayState*);
static void DrawSariaSong(Actor*, PlayState*) {}
/* SARIA_QUEUE */
struct TalkingState { bool talking, teaching; };
static void TickTalking(PlayState* play, TalkingState& state, bool gift) {
    /* SARIA_CLOSURE */
}

static void TestSariaEventAdvance() {
    customText = true;
    gSaveContext.fileNum = 1;
    for (u16 id : { kText, kTeachText, kTaughtText }) {
        for (u16 button : { u16(BTN_A), u16(BTN_B), u16(BTN_CUP) }) {
            interactor.events.clear();
            interactor.currentEvent = GIEventNone{};
            gSaveContext.save.shipSaveInfo.nei.comboSariaSongClaimed = (id == kTaughtText);
            play.msgCtx = {};
            play.state.input[0] = {};
            Message_OpenText(&play, id);
            Message_Decode(&play);
            assert(play.msgCtx.decodedBuffer.schar[play.msgCtx.decodedTextLen] == '\x19');
            const bool gift = id != kText;
            TalkingState state{ true, id == kTeachText };

            // A finished render of MM control 0x19 leaves an actor-owned event.
            // No graphics are run here; use the real state/input/close functions.
            play.msgCtx.msgMode = MSGMODE_TEXT_DONE;
            play.msgCtx.textboxEndType = TEXTBOX_ENDTYPE_EVENT;
            assert(Message_GetState(&play.msgCtx) == TEXT_STATE_EVENT);
            TickTalking(&play, state, gift);
            assert(state.talking && interactor.events.empty());
            assert(play.msgCtx.msgMode == MSGMODE_TEXT_DONE);

            play.state.input[0].press.button = BTN_CLEFT;
            TickTalking(&play, state, gift);
            assert(state.talking && interactor.events.empty());
            assert(play.msgCtx.msgMode == MSGMODE_TEXT_DONE);

            play.state.input[0].press.button = button;
            TickTalking(&play, state, gift);
            if (play.msgCtx.msgMode != MSGMODE_TEXT_CLOSING) {
                std::fprintf(stderr, "FAIL Saria text 0x%04X ignores advance button 0x%04X at its event arrow\n",
                             id, button);
                std::exit(1);
            }
            assert(state.talking && interactor.events.empty()); // Grant waits for close.
            play.state.input[0].press.button = 0;
            play.msgCtx.stateTimer = 1; // Last native closing tick, rather than forcing a close.
            TickTalking(&play, state, gift);
            assert(!state.talking && !state.teaching);
            assert(interactor.events.size() == (id == kTeachText ? 1 : 0));
            if (id == kTeachText) {
                const auto& item = std::get<GIEventGiveItem>(interactor.events.back());
                assert(item.giveItem == GiveSariaSong && item.showGetItemCutscene);
                assert(item.param == gSaveContext.fileNum);
                TickTalking(&play, state, gift);
                assert(interactor.events.size() == 1);
            }
            std::printf("PASS Saria text 0x%04X advances on 0x%04X; deferred reward count %zu\n",
                        id, button, interactor.events.size());
        }
    }
}

static void TestTeachingClosure() {
    interactor.events.clear();
    interactor.currentEvent = GIEventNone{};
    gSaveContext.fileNum = 1;
    auto& claimed = gSaveContext.save.shipSaveInfo.nei.comboSariaSongClaimed;
    claimed = 0;
    TalkingState state{ true, true };
    customText = true;
    Message_OpenText(&play, kTeachText);
    Message_Decode(&play);
    TickTalking(&play, state, true);
    assert(state.talking && interactor.events.empty());
    Message_CloseTextbox(&play);
    play.msgCtx.stateTimer = 1;
    assert(Message_GetState(&play.msgCtx) == TEXT_STATE_CLOSING);
    TickTalking(&play, state, true);
    assert(!state.talking && !state.teaching && interactor.events.size() == 1);
    const auto& item = std::get<GIEventGiveItem>(interactor.events.back());
    assert(item.param == gSaveContext.fileNum && item.giveItem == GiveSariaSong && item.showGetItemCutscene);
    QueueSariaSong();
    assert(interactor.events.size() == 1); // A queued/current grant cannot duplicate.
    interactor.currentEvent = interactor.events.back();
    interactor.events.clear();
    QueueSariaSong();
    assert(interactor.events.empty());
    interactor.currentEvent = GIEventNone{};
    claimed = 1;
    QueueSariaSong();
    assert(interactor.events.empty());
    claimed = 0;
    for (auto [teaching, gift] : { std::pair{ false, true }, std::pair{ true, false } }) {
        state = { true, teaching };
        TickTalking(&play, state, gift);
        assert(!state.talking && interactor.events.empty());
    }
    puts("PASS actual teaching-close/queue path: no early/duplicate/non-gift grants; item delivery remains a runtime check");
}

static void TestNativeTableRouting() {
    customText = false;
    static const char credits[] = "A native credits control.\x02";
    static const char normal[] = "\0\0\xFE\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF" "An ordinary MM message.\xBF";
    MessageTableEntry creditsTable[] = { { 0x4E20, 0, credits, sizeof(credits) - 1 },
                                        { kText, 0, credits, sizeof(credits) - 1 }, { 0xFFFF, 0, nullptr, 0 } };
    MessageTableEntry normalTable[] = { { 0x4E1F, 0, normal, sizeof(normal) - 1 }, { 0xFFFF, 0, nullptr, 0 } };
    for (bool isCredits : { false, true }) {
        play.msgCtx = {};
        play.msgCtx.messageTableCredits = creditsTable;
        play.msgCtx.messageTableNES = normalTable;
        Message_OpenText(&play, isCredits ? 0x4E20 : 0x4E1F);
        assert(bool(play.msgCtx.textIsCredits) == isCredits);
        Message_Decode(&play);
        assert(u8(play.msgCtx.decodedBuffer.schar[play.msgCtx.decodedTextLen]) == (isCredits ? 0x02 : 0xBF));
    }
    play.msgCtx = {};
    play.msgCtx.messageTableCredits = creditsTable;
    Message_OpenText(&play, kText);
    assert(play.msgCtx.textIsCredits); // Preserve table routing, rather than blacklist private IDs.
    Message_Decode(&play);
    assert(play.msgCtx.decodedBuffer.schar[play.msgCtx.decodedTextLen] == '\x02');
    puts("PASS native MM/credits table boundary and both real decoders");
}

static void TestFontWriteBounds() {
    // Every font write is one 0x80-byte slot in either of two banks.
    auto& font = play.msgCtx.font;
    for (int bank = 0; bank < 2; ++bank) {
        for (int offset : { 0, int(sizeof(font.charBuf[0]) - FONT_CHAR_TEX_SIZE) }) {
            play.msgCtx = {};
            font.unk_11D88 = bank;
            Font_LoadCharNES(&play, 'A', offset);
            assert(std::strncmp(reinterpret_cast<char*>(&font.charBuf[bank][offset]), "__OTR__", 7) == 0);
        }
    }
    for (int bank : { 0, 1, 2 }) {
        for (int offset : { -1, int(sizeof(font.charBuf[0]) - FONT_CHAR_TEX_SIZE + 1),
                            int(sizeof(font.charBuf[0])), 1621 * FONT_CHAR_TEX_SIZE }) {
            play.msgCtx = {};
            font.unk_11D88 = bank;
            const auto before = play;
            Font_LoadCharNES(&play, 'A', offset);
            if (std::memcmp(&before, &play, sizeof(play)) != 0) {
                std::fprintf(stderr, "FAIL font write escaped a glyph slot: bank=%d offset=%d\n", bank, offset);
                std::exit(1);
            }
        }
    }
    play.msgCtx = {};
    font.unk_11D88 = 2;
    const auto before = play;
    Font_LoadCharNES(&play, 'A', 0);
    assert(std::memcmp(&before, &play, sizeof(play)) == 0);
    play.msgCtx = {};
    const auto clean = play;
    Font_LoadCharNES(&play, 0x1F, 0);
    Font_LoadCharNES(&play, 0xFF, 0);
    assert(std::memcmp(&clean, &play, sizeof(play)) == 0);
    const auto* originalPath = fontTbl['A' - 0x20];
    const std::string oversizedPath(FONT_CHAR_TEX_SIZE, 'x');
    fontTbl['A' - 0x20] = oversizedPath.c_str();
    Font_LoadCharNES(&play, 'A', 0);
    assert(std::memcmp(&clean, &play, sizeof(play)) == 0);
    fontTbl['A' - 0x20] = originalPath;
    puts("PASS native font writer: both valid edge slots, negative/oversize slots and invalid banks");
}

/* SARIA_RECEIPT */

int main() {
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    gPlayState = &play;
    for (u16 id : { kText, kTeachText, kTaughtText, u16(0x8F10), u16(0x8F11), u16(0x8F12), u16(0x8F13),
                    u16(0x4E20), u16(0xFFFF) }) {
        play.msgCtx = {};
        customText = true;
        customId = id;
        customBody = "Welcome to the Black Market.\nLeft / Right: browse items\nA: Browse   B: Leave\x19";
        Message_OpenText(&play, id);
        if (play.msgCtx.textIsCredits) {
            std::fprintf(stderr, "FAIL custom MM text 0x%04X selected the credits decoder\n", id);
            return 1;
        }
        Message_Decode(&play);
        assert(play.msgCtx.decodedTextLen < ARRAY_COUNT(play.msgCtx.decodedBuffer.schar));
        assert(play.msgCtx.decodedBuffer.schar[play.msgCtx.decodedTextLen] == '\x19');
        assert(play.msgCtx.msgMode == MSGMODE_TEXT_DISPLAYING);
        assert(play.msgCtx.textCharScale == 0.75f);
        std::printf("PASS custom MM text 0x%04X: normal decoder, event terminates at %u\n", id,
                    play.msgCtx.decodedTextLen);
    }
    TestNativeTableRouting();
    TestFontWriteBounds();
    TestSariaEventAdvance();
    TestTeachingClosure();
    TestSariaRewardReceipt();
    puts("PASS actual Saria hook/formatter and native message opening/decoding; no game execution");
}
