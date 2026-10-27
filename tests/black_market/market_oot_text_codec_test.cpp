#include "global.h"
#include "soh/Enhancements/custom-message/CustomMessageManager.h"
#include "soh/Enhancements/randomizer/static_data.h"
#include "soh/Enhancements/Gameplay/ComboBlackMarketItems.h"
#include "soh/util.h"
#include "BlackMarketEntry.h"
#include "rando/BlackMarket.h"
#if __has_include("BlackMarketText.h")
#include "BlackMarketText.h"
#endif
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>

SaveContext gSaveContext{};
static GameInfo registers{};
GameInfo* gGameInfo = &registers;
static PlayState play;
static GraphicsContext graphics;
static Gfx commands[256];
PlayState* gPlayState = &play;
extern "C" const char* fontTbl[140];
extern "C" void Message_Decode(PlayState*);
extern "C" void MarketTest_RenderControls(PlayState*);
extern "C" void MarketTest_CloseTick(PlayState*);
extern "C" void Message_SetItemReceiptPresentation(const CwItemReceiptPresentation*) {}
extern "C" void Message_SetCapeVisibilityChoice(int) {}
extern "C" int32_t CVarGetInteger(const char*, int32_t fallback) { return fallback; }

/* NATIVE_METADATA */
/* NATIVE_COPY */
using namespace ComboRando::BlackMarket;
static Session sSession;
static int sSelected;
static std::string sStatus;
static PurchaseResult purchaseStatus = PurchaseResult::Ready;
constexpr u16 kBrowseText = 0x8F10, kResultText = 0x8F11, kWelcomeText = 0x8F12, kConfirmText = 0x8F13,
              kKeeperTalkText = 0x8F15, kKeeperHelpChoiceText = 0x8F17, kKeeperHelpText = 0x8F18;
static bool Interior() { return true; }
static bool Active() { return sSession.Active(gSaveContext.fileNum); }
static PurchaseResult Status(int) { return purchaseStatus; }
/* MARKET_TEXT */
extern "C" void MarketTest_LoadMarketText(PlayState* current, u16 id) {
    assert(current == &play);
    bool fromTable = true;
    MarketText(&id, &fromTable);
    assert(!fromTable);
}

static std::string Open(u16 id) {
    play.msgCtx = {};
    bool fromTable = true;
    MarketText(&id, &fromTable);
    assert(!fromTable);
    if (id == kBrowseText || id == kConfirmText || id == kResultText)
      assert((play.msgCtx.font.charTexBuf[0] & 0xF) == TEXTBOX_POS_BOTTOM &&
             "focused merchandise view reserves the native bottom textbox");
    return { play.msgCtx.font.msgBuf, play.msgCtx.font.msgLength };
}
static void Decode(u8 expectedEnd, bool choices = false, bool accent = false, bool buttons = false) {
    bool sawAccent = false, sawA = false, sawB = false;
    int pages = 0;
    for (;;) {
        graphics.polyOpa.p = commands;
        Message_Decode(&play);
        auto& context = play.msgCtx;
        assert(++pages < 12);
        assert(context.decodedTextLen < ARRAY_COUNT(context.msgBufDecoded));
        int lines = 1, choiceLine = 0;
        for (int i = 0; i < context.decodedTextLen; ++i) {
            const u8 c = context.msgBufDecoded[i];
            if (c == 0x01) ++lines;
            if (c == 0x1B) choiceLine = lines;
            if (c == 0x05) ++i;
            if (lines > 4) {
                std::fprintf(stderr, "FAIL OoT page lines=%d item=%s status=%d language=%d end=0x%02x page=%d\n",
                             lines, Offer(sSelected)->key, int(purchaseStatus), gSaveContext.language, expectedEnd, pages);
                for (u32 j = 0; j < context.font.msgLength; ++j)
                    std::fprintf(stderr, "%02x ", u8(context.font.msgBuf[j]));
                std::fputc('\n', stderr);
                std::abort();
            }
        }
        for (size_t i = 0; i < sizeof(context.font.charTexBuf); i += FONT_CHAR_TEX_SIZE) {
            const char* glyph = reinterpret_cast<const char*>(&context.font.charTexBuf[i]);
            sawAccent |= std::strcmp(glyph, fontTbl[0x96 - 0x20]) == 0;
            sawA |= std::strcmp(glyph, fontTbl[0x9F - 0x20]) == 0;
            sawB |= std::strcmp(glyph, fontTbl[0xA0 - 0x20]) == 0;
        }
        const u8 end = context.msgBufDecoded[context.decodedTextLen];
        if (end == 0x04) {
            ++context.msgBufPos;
            continue;
        }
        assert(end == expectedEnd);
        assert(context.choiceNum == (choices ? 2 : 0));
        if (choices)
            assert(lines == 4 && choiceLine == 3);
        MarketTest_RenderControls(&play); // Exact native choice/EVENT/END rendering branches.
        assert(Message_GetState(&context) == (choices ? TEXT_STATE_CHOICE :
                                             expectedEnd == 0x0B ? TEXT_STATE_EVENT : TEXT_STATE_DONE));
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
    Select(*Find("nei.cane.statues"), 999);
    play.msgCtx = {};
    for (int conversation = 0; conversation < 3; ++conversation) {
        Message_StartTextbox(&play, kWelcomeText, nullptr);
        R_TEXTBOX_Y = R_TEXTBOX_Y_TARGET =
            38; // Already-open native top welcome box.
        R_TEXTBOX_END_YPOS = 97;
        for (int row = 0; row < 3; ++row)
          R_TEXT_CHOICE_YPOS(row) = 58 + row * 12;
        Decode(0x0B);
        for (u16 id : { kBrowseText, kConfirmText, kBrowseText, kConfirmText, kResultText,
                        kBrowseText, kWelcomeText, kKeeperTalkText, kKeeperHelpChoiceText, kKeeperHelpText, kWelcomeText }) {
            play.msgCtx.choiceIndex = 1;
            sStatus = conversation == 1 ? "You need more rupees." : "Yours: Somaria: Statues.";
            ContinueBrowsingText(&play, id);
            assert(play.msgCtx.msgMode == MSGMODE_TEXT_CONTINUING);
            const bool keeperText = id == kWelcomeText || id == kKeeperTalkText ||
                                    id == kKeeperHelpChoiceText || id == kKeeperHelpText;
            assert(R_TEXTBOX_Y == (keeperText ? 38 : 142) &&
                   R_TEXTBOX_Y_TARGET == R_TEXTBOX_Y &&
                   "merchandise continuation must move an existing top box "
                   "below the focused row, then restore welcome");
            assert(R_TEXTBOX_END_YPOS == (keeperText ? 97 : 201));
            for (int row = 0; row < 3; ++row)
              assert(R_TEXT_CHOICE_YPOS(row) ==
                     (keeperText ? 58 : 162) + row * 12);
            Decode(id == kConfirmText || id == kKeeperHelpChoiceText ? 0x02 : 0x0B,
                   id == kConfirmText || id == kKeeperHelpChoiceText);
            if (id == kConfirmText || id == kKeeperHelpChoiceText)
                assert(play.msgCtx.choiceIndex == 0);
        }
        Message_CloseTextbox(&play);
        assert(play.msgCtx.msgMode == MSGMODE_TEXT_CLOSING && play.msgCtx.stateTimer == 2);
        MarketTest_CloseTick(&play);
        assert(Message_GetState(&play.msgCtx) == TEXT_STATE_CLOSING);
        MarketTest_CloseTick(&play);
        assert(Message_GetState(&play.msgCtx) == TEXT_STATE_NONE && play.msgCtx.msgLength == 0);
        assert(play.msgCtx.textId == 0 && play.msgCtx.stateTimer == 0);
    }
    puts("PASS OoT native Start/Continue/Close with text-loading boundary, ordinary closing reset and repeat-talk contract; actor/camera release is a separate native browsing fixture");
}

int main() {
    const std::string operands = "\x15\x01\x04\x1B" "one\x01two\x01three\x01" "four\x0B\x02";
    assert(BlackMarketText::OotPages(operands) == operands);
    const auto pagedOperands = BlackMarketText::OotPages(
        "\x15\x01\x04\x1B" "one\x01two\x01three\x01" "four\x01" "five\x0B\x02");
    assert(pagedOperands.starts_with(std::string("\x15\x01\x04\x1B", 4)));
    assert(pagedOperands.find("four\x04\x05\x00" "five", 0, 11) != std::string::npos);
    play.state.gfxCtx = &graphics;
    gSaveContext.fileNum = 0;
    gSaveContext.language = LANGUAGE_ENG;
    sSession.Bind(Generate(77), 0);
    InitMetadata();
    Select(*Find("nei.season.spring"));
    Open(kKeeperTalkText); Decode(0x0B);
    auto helpChoice = Open(kKeeperHelpChoiceText);
    assert(helpChoice.find("How does this shop work?") != std::string::npos);
    Decode(0x02, true);
    auto help = Open(kKeeperHelpText);
    assert(help.find("unique dungeon clears") != std::string::npos && help.find("empty bottle") != std::string::npos);
    Decode(0x0B);
    auto body = Open(kBrowseText);
    if (body.find("Season: Spring") == std::string::npos) {
        std::fputs("FAIL OoT merchandise uses catalogue shorthand instead of fixed native item metadata\n", stderr);
        return 1;
    }
    assert(body.find("173 Rupees") != std::string::npos);
    assert(body.find("Tier 1") != std::string::npos);
    assert(body.find('\x9F') != std::string::npos && body.find('\xA0') != std::string::npos);
    Decode(0x0B, false, false, true);
    body = Open(kConfirmText);
    assert(body.find("Buy") != std::string::npos && body.find("Don't buy") != std::string::npos);
    Decode(0x02, true);
    for (const auto& item : Catalog()) {
        Select(item, 999);
        for (auto status : { PurchaseResult::Ready, PurchaseResult::NeedRupees, PurchaseResult::Locked,
                             PurchaseResult::SoldOut, PurchaseResult::Owned, PurchaseResult::Unavailable }) {
            purchaseStatus = status;
            const auto before = sSession.Save(gSaveContext.fileNum);
            body = Open(kBrowseText);
            assert(body.find("Tier " + std::to_string(item.tier)) != std::string::npos);
            Decode(0x0B);
            assert(sSession.Save(gSaveContext.fileNum) == before);
        }
        Open(kConfirmText);
        Decode(0x02, true);
    }
    Select(*Find("wildcard.poke_ball"));
    body = Open(kConfirmText);
    assert(body.find("Pok\x96 Ball") != std::string::npos);
    assert(body.find('\xC3') == std::string::npos);
    Decode(0x02, true, true);
    sStatus = "Yours: Poke Ball.";
    Open(kResultText);
    Decode(0x0B, false, true);

    // The host's actual Text localization and font codec must also survive.
    Select(*Find("nei.season.spring"));
    gSaveContext.language = LANGUAGE_FRA;
    body = Open(kConfirmText);
    assert(body.find("Saison: Printemps") != std::string::npos);
    assert(body.find("Acheter") != std::string::npos && body.find("Ne pas acheter") != std::string::npos);
    Decode(0x02, true);
    gSaveContext.language = LANGUAGE_GER;
    body = Open(kConfirmText);
    assert(body.find("Fr\x9E" "hling") != std::string::npos);
    assert(body.find("Kaufen") != std::string::npos && body.find("Nicht kaufen") != std::string::npos);
    Decode(0x02, true);
    gSaveContext.language = LANGUAGE_ENG;

    for (u8 language : { u8(LANGUAGE_ENG), u8(LANGUAGE_FRA), u8(LANGUAGE_GER) }) {
        gSaveContext.language = language;
        for (const auto& item : Catalog()) {
            Select(item, 999);
            Open(kBrowseText);
            Decode(0x0B);
            Open(kConfirmText);
            Decode(0x02, true);
        }
    }
    gSaveContext.language = LANGUAGE_ENG;

    for (const char* status : { "You need more rupees.", "You need its prerequisite first.",
                               "You already own that power.", "That one has already been sold.",
                               "The seal needs 2 unique dungeon clears.", "That offer cannot be delivered yet." }) {
        sStatus = status;
        const auto before = sSession.Save(gSaveContext.fileNum);
        Open(kResultText);
        Decode(0x0B);
        assert(sSession.Save(gSaveContext.fileNum) == before);
    }
    // A custom unresolved item remains readable instead of adopting another item's identity.
    Select(*Find("nei.season.spring"));
    metadata[RG_SEASON_SPRING] = Rando::Item();
    assert(Open(kBrowseText).find("Spring") != std::string::npos);
    Decode(0x0B);
    Open(kWelcomeText);
    Decode(0x0B, false, false, true);
    Open(BlackMarketEntry::kUnavailableText);
    Decode(0x02);
    TestReleaseContract();
    puts("PASS actual OoT MarketText, fixed localized metadata, prices/status/tier, native choice/event and font glyphs (ASan/UBSan)");
}
