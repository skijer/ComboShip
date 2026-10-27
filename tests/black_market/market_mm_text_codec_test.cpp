#include "global.h"
#include "message_data_static.h"
#include "2s2h/CustomMessage/CustomMessage.h"
#include <nlohmann/json.hpp>
#include "2s2h/Rando/Rando.h"
#include "2s2h/Enhancements/Gameplay/BlackMarketItems.h"
#include "BlackMarketEntry.h"
#include "rando/BlackMarket.h"
#include "ComboCapeReceiptChoice.h"
#include "overlays/actors/ovl_En_Ossan/z_en_ossan.h"
#if __has_include("BlackMarketText.h")
#include "BlackMarketText.h"
#endif
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

SaveContext gSaveContext{};
static RegEditor registers{};
RegEditor* gRegEditor = &registers;
s16 gItemPrices[256]{};
u32 gBitFlags[32]{};
static PlayState play;
static Player player;
static char* textboxSegments[TEXTBOX_SEG_MAX];
PlayState* gPlayState = &play;
extern "C" f32 sNESFontWidths[160];
extern "C" const char* fontTbl[156];
extern "C" void Message_OpenText(PlayState*, u16);
extern "C" void Message_Decode(PlayState*);
extern "C" void MarketTest_RenderControls(PlayState*);
extern "C" void MarketTest_CloseTick(PlayState*);
extern "C" void Message_SetItemReceiptPresentation(const CwItemReceiptPresentation*);
extern "C" void Message_SetCapeVisibilityChoice(int);
extern "C" void SwordElement_MessageAppend(PlayState*, u8) {}
extern "C" void Interface_SetHudVisibility(u16) {}
extern "C" void Interface_SetBButtonInterfaceDoAction(PlayState*, s16) {}
extern "C" u8 ExtInv_GetItemIconSize(u16) { std::abort(); }
extern "C" void* ExtInv_GetItemIcon(u16) { std::abort(); }
extern "C" bool GameInteractor_Should(GIVanillaBehavior, uint32_t result, ...) { return result; }
extern "C" s32 Map_CurRoomHasMapI(PlayState*) { return 0; }
extern "C" s32 Flags_GetAllTreasure(PlayState*) { return 0; }
extern "C" s16 Inventory_GetSkullTokenCount(s16) { return 0; }
extern "C" void Audio_PlaySfx(u16) {}
extern "C" void Interface_SetAButtonDoAction(PlayState*, u16) {}
extern "C" int32_t CVarGetInteger(const char*, int32_t fallback) { return fallback; }

/* CUSTOM_MESSAGE_HELPERS */
/* NATIVE_METADATA */

using namespace ComboRando::BlackMarket;
static Session sSession;
static int sSelected;
static std::string sStatus;
static PurchaseResult purchaseStatus = PurchaseResult::Ready;
constexpr u16 kBrowseText = 0x8F10, kResultText = 0x8F11, kWelcomeText = 0x8F12, kConfirmText = 0x8F13;
constexpr u16 kKeeperMenuText = 0x8F14, kKeeperTalkText = 0x8F15, kContinueText = 0x8F16, kKeeperHelpChoiceText = 0x8F17, kKeeperHelpText = 0x8F18;
static EnOssan* sNativeShop;
static bool Interior() { return true; }
static bool Active() { return sSession.Active(gSaveContext.fileNum); }
static PurchaseResult Status(int) { return purchaseStatus; }
/* MARKET_TEXT */
extern "C" void GameInteractor_ExecuteOnOpenText(u16* textId, bool* fromTable) {
    MarketText(textId, fromTable);
    assert(!*fromTable);
}

static std::string Body() {
    return { reinterpret_cast<char*>(play.msgCtx.font.msgBuf.schar + MESSAGE_HEADER_SIZE),
             size_t(play.msgCtx.msgLength - MESSAGE_HEADER_SIZE) };
}
static std::string Open(u16 id) {
    play.msgCtx = {};
    Message_OpenText(&play, id);
    if (id == kBrowseText || id == kConfirmText || id == kResultText ||
        id == kKeeperMenuText || id == kKeeperTalkText || id == kContinueText)
      assert(play.msgCtx.textBoxPos == 3 &&
             "focused merchandise view reserves the native bottom textbox");
    play.msgCtx.textboxY = play.msgCtx.textboxYTarget; // Fully opened native textbox geometry.
    assert(!play.msgCtx.textIsCredits);
    assert(u8(play.msgCtx.font.msgBuf.schar[2]) == 0xFE); // Native merchandise has no receipt icon.
    return Body();
}
static void Decode(u8 expectedEnd, bool choices = false, bool accent = false, bool buttons = false) {
    bool sawAccent = false, sawA = false, sawB = false;
    int pages = 0;
    for (;;) {
        Message_Decode(&play);
        auto& context = play.msgCtx;
        assert(++pages < 12);
        assert(context.decodedTextLen < ARRAY_COUNT(context.decodedBuffer.schar));
        int lines = 1, choiceLine = 0;
        for (int i = 0; i < context.decodedTextLen; ++i) {
            const u8 c = context.decodedBuffer.schar[i];
            if (c == 0x11) ++lines;
            if (c == 0xC2) choiceLine = lines;
            assert(lines <= 3); // MM stores exactly three line offsets.
        }
        for (size_t i = 0; i < sizeof(context.font.charBuf[0]); i += FONT_CHAR_TEX_SIZE) {
            const char* glyph = reinterpret_cast<const char*>(&context.font.charBuf[context.font.unk_11D88][i]);
            sawAccent |= std::strcmp(glyph, fontTbl[0x9D - 0x20]) == 0;
            sawA |= std::strcmp(glyph, fontTbl[0xB0 - 0x20]) == 0;
            sawB |= std::strcmp(glyph, fontTbl[0xB1 - 0x20]) == 0;
        }
        const u8 end = context.decodedBuffer.schar[context.decodedTextLen];
        if (end == 0x10 || end == 0x12) {
            ++context.msgBufPos; // Native page advance consumes the decoded box-break control.
            continue;
        }
        assert(end == expectedEnd);
        assert(context.choiceNum == (choices ? 2 : 0));
        if (choices) {
            assert(lines == 3 && choiceLine == 2);
        }
        MarketTest_RenderControls(&play); // Exact native choice/EVENT/END rendering branches.
        assert(Message_GetState(&context) == (choices ? TEXT_STATE_CHOICE :
                                             expectedEnd == 0x19 ? TEXT_STATE_EVENT : TEXT_STATE_DONE));
        if (choices) {
            assert(context.unk11FFC == 12);
            assert(context.unk11FFE[1] == context.unk11FFA + context.unk11FFC);
            assert(context.unk11FFE[2] == context.unk11FFA + 2 * context.unk11FFC);
        }
        break;
    }
    assert(!accent || sawAccent);
    assert(!buttons || (sawA && sawB));
}
static void Select(const Item& item, int price = 173) {
    sSession.market["stock"][0] = { { "key", item.key }, { "price", price }, { "clears", 2 } };
    purchaseStatus = PurchaseResult::Ready;
}

static void TestReleaseContract() {
    Select(*Find("nei.cane.statues"), 999); // A wrapped confirmation also has to release.
    play.msgCtx = {};
    play.msgCtx.textboxSegment = textboxSegments; // Native Message_Init-owned segment array.
    for (int conversation = 0; conversation < 3; ++conversation) {
        Message_StartTextbox(&play, kWelcomeText, &player.actor);
        // Begin with the authored keeper conversation's already-open native
        // bottom box. ContinueTextbox inherits native layout; no old shared
        // browsing-controller positioning helper participates in the MM path.
        play.msgCtx.textboxY = play.msgCtx.textboxYTarget = 142;
        for (int row = 0; row < 3; ++row)
          play.msgCtx.unk11FFE[row] = 162 + row * 12;
        Decode(0x19);
        for (u16 id : { kKeeperMenuText, kKeeperTalkText, kKeeperHelpChoiceText, kKeeperHelpText, kKeeperMenuText, kBrowseText,
                        kConfirmText, kBrowseText, kConfirmText, kResultText,
                        kBrowseText, kKeeperMenuText, kContinueText, kKeeperMenuText }) {
            play.msgCtx.choiceIndex = 1; // Decline must not leave a stale selection on re-confirm.
            sStatus = conversation == 1 ? "You need more rupees." : "Yours: Somaria: Statues.";
            Message_ContinueTextbox(&play, id);
            assert(play.msgCtx.msgMode == MSGMODE_TEXT_CONTINUING);
            assert(play.msgCtx.textboxY == 142 &&
                   play.msgCtx.textboxYTarget == play.msgCtx.textboxY &&
                   "native shop continuation preserves its bottom textbox");
            const bool choice = id == kConfirmText || id == kKeeperMenuText || id == kContinueText || id == kKeeperHelpChoiceText;
            Decode(choice ? 0xBF : 0x19, choice);
            if (choice)
                assert(play.msgCtx.choiceIndex == 0);
        }
        Message_CloseTextbox(&play);
        assert(play.msgCtx.msgMode == MSGMODE_TEXT_CLOSING && play.msgCtx.stateTimer == 2);
        MarketTest_CloseTick(&play);
        assert(Message_GetState(&play.msgCtx) == TEXT_STATE_CLOSING);
        MarketTest_CloseTick(&play);
        assert(Message_GetState(&play.msgCtx) == TEXT_STATE_NONE && play.msgCtx.msgLength == 0);
        assert(play.msgCtx.currentTextId == 0 && play.msgCtx.stateTimer == 0);
        // No MessageContext reset here: the next iteration re-talks through native StartTextbox.
    }
    puts("PASS MM native Start/Continue/Close, ordinary closing reset and repeat-talk contract; actor/camera release is a separate native browsing fixture");
}

static void TestNativeShopPagesAndSelection() {
    auto welcome = Open(kWelcomeText);
    assert(welcome.find("Continue") != std::string::npos && welcome.find("Talk") == std::string::npos);
    Decode(0x19, false, false, true);
    auto body = Open(kKeeperMenuText);
    assert(body.find("Talk to shopkeeper") != std::string::npos && body.find("Leave") != std::string::npos);
    Decode(0xBF, true);
    body = Open(kKeeperTalkText); assert(body.find("No guessing games") != std::string::npos); Decode(0x19);
    body = Open(kKeeperHelpChoiceText);
    assert(body.find("How does this shop work?") != std::string::npos && body.find("Back") != std::string::npos);
    Decode(0xBF, true);
    body = Open(kKeeperHelpText);
    assert(body.find("unique dungeon clears") != std::string::npos && body.find("empty bottle") != std::string::npos);
    Decode(0x19);
    body = Open(kContinueText);
    assert(body.find("Keep shopping?") != std::string::npos && body.find("Yes") != std::string::npos);
    Decode(0xBF, true);
    Select(*Find("nei.lantern"), 173);
    sSession.market["stock"][1] = {{"key", "nei.whip"}, {"price", 88}, {"clears", 2}};
    EnOssan native{}; native.cursorIndex = 1; sNativeShop = &native; sSelected = 0;
    body = Open(kBrowseText); assert(sSelected == 1 && body.find("88 Rupees") != std::string::npos); Decode(0x19);
    sSelected = 0; Open(kConfirmText); assert(sSelected == 1); Decode(0xBF, true);
    assert(play.msgCtx.unk1206C == 88);
    native.cursorIndex = 0; sSelected = 1; Open(kConfirmText); assert(sSelected == 0); Decode(0xBF, true);
    assert(play.msgCtx.unk1206C == 173); sNativeShop = nullptr;
    puts("PASS MM native keeper/talk/continue message states and actual EnOssan cursor-to-offer/price synchronization");
}

int main() {
    XREG(10) = 26; XREG(11) = 20; XREG(12) = 14; XREG(13) = 0; // Native Setup_InitRegs.
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    gSaveContext.fileNum = 0;
    sSession.Bind(Generate(77), 0);
    const auto* spring = Find("nei.season.spring");
    Select(*spring);
    auto body = Open(kBrowseText);
    if (body.find("Season: Spring") == std::string::npos) {
        std::fputs("FAIL MM merchandise uses catalogue shorthand instead of fixed native item metadata\n", stderr);
        return 1;
    }
    assert(body.find("173 Rupees") != std::string::npos);
    assert(body.find("Tier 1") != std::string::npos);
    assert(body.find('\xB0') != std::string::npos && body.find('\xB1') != std::string::npos);
    Decode(0x19, false, false, true);
    body = Open(kConfirmText);
    assert(body.find("I'll buy it") != std::string::npos && body.find("No thanks") != std::string::npos);
    Decode(0xBF, true);
    assert(play.msgCtx.unk1206C == 173); // Real 11-byte header, real native header decoder.

    for (const auto& item : Catalog()) {
        Select(item, 999);
        for (auto status : { PurchaseResult::Ready, PurchaseResult::NeedRupees, PurchaseResult::Locked,
                             PurchaseResult::SoldOut, PurchaseResult::Owned, PurchaseResult::Unavailable }) {
            purchaseStatus = status;
            const auto before = sSession.Save(gSaveContext.fileNum);
            body = Open(kBrowseText);
            assert(body.find("Tier " + std::to_string(item.tier)) != std::string::npos);
            Decode(0x19);
            assert(sSession.Save(gSaveContext.fileNum) == before);
        }
        Open(kConfirmText);
        Decode(0xBF, true);
        assert(play.msgCtx.unk1206C == 999);
    }

    Select(*Find("wildcard.poke_ball"));
    // Exercise the real native metadata boundary with a supported UTF-8 font glyph.
    auto& metadata = Rando::StaticData::Items[ComboBlackMarketItems::Id(*Find("wildcard.poke_ball"))];
    metadata.name = "Poké Ball";
    body = Open(kConfirmText);
    assert(body.find("Pok\x9D Ball") != std::string::npos);
    assert(body.find('\xC3') == std::string::npos);
    Decode(0xBF, true, true);
    sStatus = "Yours: Poke Ball.";
    Open(kResultText);
    Decode(0x19, false, true);

    // Native metadata unavailable: retain the custom offer's catalogue label.
    Rando::StaticData::Items.erase(ComboBlackMarketItems::Id(*spring));
    Select(*spring);
    assert(Open(kBrowseText).find("Spring") != std::string::npos);
    Decode(0x19);
    for (const char* status : { "You need more rupees.", "Bring an empty bottle for this refill.",
                               "You need its prerequisite first.", "You already own that power.",
                               "That one has already been sold.", "The seal needs 2 unique dungeon clears.",
                               "That offer cannot be delivered yet." }) {
        sStatus = status;
        const auto before = sSession.Save(gSaveContext.fileNum);
        Open(kResultText);
        Decode(0x19);
        assert(sSession.Save(gSaveContext.fileNum) == before);
    }
    Open(kWelcomeText);
    Decode(0x19, false, false, true);
    Open(BlackMarketEntry::kUnavailableText);
    Decode(0xBF);
    TestNativeShopPagesAndSelection();
    TestReleaseContract();
    puts("PASS actual MM MarketText, fixed metadata, prices/status/tier, native header/choice/event and font glyphs (ASan/UBSan)");
}
