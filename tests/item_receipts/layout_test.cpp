// Execute both engines' per-message stage/apply/draw boundary with the real
// native font widths. GBI recording replaces only the GPU command sink.
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include "combo/menu/ComboItemReceiptText.h"
#include "combo/menu/ComboItemReceiptPresentation.h"
using Gfx = int;
constexpr int G_IM_FMT_IA = 1, G_IM_FMT_RGBA = 2, G_IM_SIZ_8b = 1, G_IM_SIZ_32b = 4;
static int format, bits, sourceWidth, sourceHeight, x1, y1, x2, y2, red, green, blue, alpha, draws;
static std::string path;
#define gDPPipeSync(p) ((void)(p))
#define gDPSetCombineMode(p, ...) ((void)(p))
#define gDPSetCombineLERP(p, ...) ((void)(p))
#define gDPSetPrimColor(p, a, b, r, g, bl, al) \
    ((void)(p), red = (r), green = (g), blue = (bl), ::alpha = (al))
#define gDPLoadTextureBlock(p, tex, fmt, siz, w, h, ...) \
    ((void)(p), path = (tex), format = (fmt), bits = (siz), sourceWidth = (w), sourceHeight = (h))
#define gSPTextureRectangle(p, ax, ay, bx, by, ...) \
    ((void)(p), x1 = (ax) / 4, y1 = (ay) / 4, x2 = (bx) / 4, y2 = (by) / 4, ++draws)
#include "combo/menu/ComboItemReceiptRender.h"

namespace Oot {
struct MessageContext {
    struct { char msgBuf[1280]{}; unsigned msgLength = 0; } font;
    int msgLength = 0, msgBufPos = 0, textPosX = 0, textPosY = 0, textColorAlpha = 192, textBoxType = 2;
};
struct PlayState { MessageContext msgCtx; } play;
using u16 = uint16_t;
constexpr int TEXTBOX_TYPE_NONE_BOTTOM = 4;
int R_TEXT_CHAR_SCALE = 75, R_TEXT_LINE_SPACING = 12, R_TEXT_INIT_XPOS = 65;
int R_TEXT_INIT_YPOS = 28, R_TEXTBOX_Y = 12;
/* OOT_FONT_WIDTHS */
/* OOT_RECEIPT_RENDERER */
/* OOT_DECODE_POSITION */
}
namespace Mm {
using u16 = uint16_t;
using s16 = int16_t;
struct MessageContext {
    struct { struct { char schar[1280]{}; } msgBuf; } font;
    int msgLength = 0, msgBufPos = 0, unk11FFC = 12, unk11F18 = 0, unk11F1A[3]{};
    int unk11FF8 = 65, unk11FFA = 28, textColorAlpha = 192;
    int textPosX = 0, textPosY = 0, itemId = 0xFE;
    int textboxY = 12, textBoxType = 2;
    bool textIsCredits = false;
    float textCharScale = 0.75f;
};
struct PlayState { MessageContext msgCtx; } play;
int sCharTexSize = 12, sCharTexScale = 1024 / 0.75f;
/* MM_FONT_WIDTHS */
/* MM_RECEIPT_RENDERER */
constexpr int LANGUAGE_ENG = 1, LANGUAGE_JPN = 0, VB_DRAW_OCARINA_STAFF = 1;
struct { struct { int language = LANGUAGE_ENG; } options; } gSaveContext;
int nativeIconDraws = 0, japaneseDraws = 0, creditsDraws = 0;
bool GameInteractor_Should(int, bool enabled) { return enabled; }
void Message_DrawItemIcon(PlayState*, Gfx**) { ++nativeIconDraws; }
void Message_DrawTextDefault(PlayState*, Gfx**) { ++japaneseDraws; }
void Message_DrawTextCredits(PlayState*, Gfx**) { ++creditsDraws; }
/* MM_TEXT_DISPATCH */
/* MM_DECODE_POSITION */
}

static std::string VisibleWords(const char* body, size_t size, int mm) {
    std::string result;
    for (size_t i = 0; i < size;) {
        const uint8_t c = body[i];
        const size_t command = ComboReceipt_CommandSize(c, mm);
        if (c == (mm ? 0xBF : 0x02)) break;
        if (c == (mm ? 0x11 : 0x01) || c == (mm ? 0x10 : 0x04) || c == ' ') {
            if (!result.empty() && result.back() != ' ') result += ' ';
        } else if (command == 1 && c >= 0x20) result += c;
        i += command;
    }
    if (!result.empty() && result.back() == ' ') result.pop_back();
    return result;
}

static void CheckReflow(const std::string& original, const char* body, size_t size, int mm,
                        const float* widths, size_t count, int scale = 75) {
    assert(VisibleWords(body, size, mm) == VisibleWords(original.data(), original.size(), mm));
    unsigned lines = 1;
    unsigned glyphs = 0;
    int width = 0;
    for (size_t i = 0; i < size;) {
        const uint8_t c = body[i];
        const size_t command = ComboReceipt_CommandSize(c, mm);
        if (c == (mm ? 0xBF : 0x02)) break;
        if (c == (mm ? 0x11 : 0x01)) {
            assert(++lines <= 3 && "native line-offset storage must never receive a fourth line");
            width = 0;
        } else if (c == (mm ? 0x10 : 0x04)) {
            lines = 1;
            glyphs = 0;
            width = 0;
        } else if (command == 1) {
            if (mm && c >= 0x21) {
                assert(++glyphs <= 120 && "MM's native character texture bank has only 120 slots per page");
            }
            // Use the engine's submitted pen advance, independently of the fitter.
            width += mm ? Mm::NativeLineWidth(std::string(1, static_cast<char>(c)))
                        : c == ' ' ? 6 * scale / 75
                                   : c >= 0x20 && static_cast<size_t>(c - 0x20) < count
                                         ? static_cast<int>(widths[c - 0x20] * (scale / 100.0f)) : 0;
            assert(width <= 220 && "word wrapping must fit the actual native pen, including spaces");
        }
        i += command;
    }
}

static void CheckLongMmMapPages() {
  // Real entrance-source names: normal and inverted Stone Tower can each
  // point to a different source. A visually fitting map still needs enough
  // character texture slots in MM, independently of its chosen text scale.
  const std::string sources[] = {"Woodfall",
                                 "Snowhead",
                                 "Zora Cape's turtle",
                                 "Stone Tower (normal)",
                                 "Stone Tower (inverted)",
                                 "Southern Swamp's Spider House",
                                 "Great Bay's Spider House",
                                 "Ikana Castle",
                                 "Ikana Canyon's Secret Shrine",
                                 "Beneath the Well's front entrance",
                                 "Beneath the Well's castle entrance",
                                 "Great Bay's Pirates' Fortress",
                                 "Deku Tree",
                                 "Dodongo's Cavern",
                                 "Jabu-Jabu's Belly",
                                 "Forest Temple",
                                 "Fire Temple",
                                 "Water Temple",
                                 "Spirit Temple",
                                 "Shadow Temple",
                                 "Bottom of the Well",
                                 "Ice Cavern"};
  CwItemReceiptPresentation p{};
  p.singleBox = 1;
  p.rewardLine = 2;
  for (const auto &normal : sources) {
    for (const auto &inverted : sources) {
      if (normal == inverted)
        continue;
      const auto original = ComboItemReceiptText::FromNeiMarkup(
                                "You found the %gStone Tower Map%w!&It seems "
                                "the entrance is at %cnormal: " +
                                normal + "; inverted: " + inverted + "%w.") +
                            '\xBF';
      char *body = Mm::play.msgCtx.font.msgBuf.schar + 11;
      std::memcpy(body, original.data(), original.size());
      Mm::play.msgCtx.msgLength = original.size() + 11;
      Mm::play.msgCtx.msgBufPos = 0;
      Mm::Message_SetItemReceiptPresentation(&p);
      Mm::Message_ApplyItemReceiptLayout(&Mm::play);
      const auto &layout = Mm::sItemReceiptLayout;
      CheckReflow(original, body, layout.bodySize, true, Mm::sNESFontWidths,
                  160, layout.textScale);
      assert(body[layout.bodySize - 1] == '\xBF');
    }
  }
  for (const unsigned length : {120u, 121u, 241u}) {
    std::string original;
    for (unsigned i = 0; i < length; ++i) {
      if (i && i % 10 == 0)
        original += ' ';
      original += 'i';
    }
    original += '\xBF';
    char *body = Mm::play.msgCtx.font.msgBuf.schar + 11;
    std::memcpy(body, original.data(), original.size());
    Mm::play.msgCtx.msgLength = original.size() + 11;
    Mm::play.msgCtx.msgBufPos = 0;
    Mm::Message_SetItemReceiptPresentation(&p);
    Mm::Message_ApplyItemReceiptLayout(&Mm::play);
    const auto &layout = Mm::sItemReceiptLayout;
    CheckReflow(original, body, layout.bodySize, true, Mm::sNESFontWidths, 160,
                layout.textScale);
    assert(length != 120 || !std::memchr(body, '\x10', layout.bodySize));
  }
}


int main() {
    CheckLongMmMapPages();
    (void)&Mm::Message_DrawItemReceiptIcon;
    CwItemReceiptPresentation p{};
    p.singleBox = 1;
    p.rewardLine = 2;
    const CwItemIconInfo fire{ "__OTR__textures/icon_item_24_static/gQuestIconMedallionFireTex", 24, 24, 0, 0, {} };
    assert(ComboReceipt_CopyIcon(&p, &fire, "oot"));
    const std::string body = "You found the Deku Tree Compass!\x01It points to Volvagia!\x01"
                             "Defeating the boss grants the Fire Medallion!\x13\x9C\x02";
    std::memcpy(Oot::play.msgCtx.font.msgBuf, body.data(), body.size());
    Oot::play.msgCtx.msgLength = body.size();
    Oot::Message_SetItemReceiptPresentation(&p);
    // Staging owns its own copy, including the resource path.
    p.iconPath[0] = '\0';
    Oot::Message_ApplyItemReceiptLayout(&Oot::play);
    for (int lines = 0; lines <= 2; ++lines) {
        Oot::FinishReceiptDecodePosition(lines);
        assert(Oot::R_TEXT_INIT_YPOS == Oot::R_TEXTBOX_Y + 16 && "page quantity changed receipt baseline");
    }
    assert(Oot::R_TEXT_CHAR_SCALE == 75 && "long reward hints must wrap at the native readable font size");
    CheckReflow(body, Oot::play.msgCtx.font.msgBuf, Oot::play.msgCtx.msgLength, false, Oot::sFontWidths, 144);
    assert(!Oot::Message_HasItemReceiptIcon() && "a reward on a later page cannot appear beside the title");
    Oot::play.msgCtx.msgBufPos = Oot::sItemReceiptLayout.iconPageStart;
    const auto ootLength = Oot::play.msgCtx.msgLength;
    Oot::Message_ApplyItemReceiptLayout(&Oot::play);
    assert(Oot::play.msgCtx.msgLength == ootLength && "changing pages must not reflow the message twice");
    assert(Oot::R_TEXT_LINE_SPACING == 12 && Oot::Message_HasItemReceiptIcon());
    assert(Oot::sItemReceiptLayout.iconX + Oot::sItemReceiptLayout.iconWidth <= 220);
    Oot::play.msgCtx.textPosX = 65 + Oot::sItemReceiptLayout.iconX - 4;
    Oot::play.msgCtx.textPosY = 52;
    Gfx commands[64]{};
    Gfx* gfx = commands;
    Oot::Message_DrawItemReceiptIcon(&Oot::play, &gfx);
    assert(draws == 1 && path == "__OTR__@oot:textures/icon_item_24_static/gQuestIconMedallionFireTex");
    assert(format == G_IM_FMT_RGBA && bits == G_IM_SIZ_32b && sourceWidth == 24 && sourceHeight == 24);
    assert(x1 > 65 && x2 <= 285 && y1 > 28 && y2 <= 76 && x2 - x1 == 24 && y2 - y1 == 24);
    assert(red == 255 && green == 255 && blue == 255 && alpha == 192);
    Oot::Message_SetItemReceiptPresentation(nullptr);
    Oot::FinishReceiptDecodePosition(0);
    assert(Oot::R_TEXT_INIT_YPOS == Oot::R_TEXTBOX_Y + 26 && "ordinary native receipt centering changed");
    Oot::Message_DrawItemReceiptIcon(&Oot::play, &gfx);
    assert(draws == 1 && !Oot::Message_HasItemReceiptIcon());

    p = {};
    p.singleBox = 1;
    p.rewardLine = 2;
    const CwItemIconInfo clef{ "__OTR__icon_item_static_yar/gItemIconSongNoteTex", 16, 24, 1, 1,
                              { 128, 216, 240, 255 } };
    assert(ComboReceipt_CopyIcon(&p, &clef, "mm"));
    const std::string text = ComboItemReceiptText::FromNeiMarkup(
        "You found the Snowhead Compass!&It points to Goht!&Defeating the boss grants the Progressive Goron Lullaby!");
    std::memcpy(Mm::play.msgCtx.font.msgBuf.schar + 11, text.data(), text.size());
    Mm::play.msgCtx.font.msgBuf.schar[text.size() + 11] = '\xBF';
    Mm::play.msgCtx.msgLength = text.size() + 12;
    Mm::Message_SetItemReceiptPresentation(&p);
    Mm::Message_ApplyItemReceiptLayout(&Mm::play);
    for (int lines = 0; lines <= 2; ++lines) {
        Mm::FinishReceiptDecodePosition(lines);
        assert(Mm::play.msgCtx.unk11FFA == Mm::play.msgCtx.textboxY + 16 && "page quantity changed receipt baseline");
    }
    assert(Mm::play.msgCtx.textCharScale == 0.75f && "long reward hints must not change glyph/space proportions");
    CheckReflow(text, Mm::play.msgCtx.font.msgBuf.schar + 11, Mm::play.msgCtx.msgLength - 11,
                true, Mm::sNESFontWidths, 160);
    for (int line = 0; line < 3; ++line) {
        Mm::DecodeReceiptLine(line, 120.0f + line * 10.0f);
        assert(Mm::play.msgCtx.unk11F1A[line] == 0 && "native English decode shifted the fitted receipt away from its sprite");
    }
    assert(Mm::sItemReceiptLayout.iconX + Mm::sItemReceiptLayout.iconWidth <= 220);
    Mm::Message_DrawText(&Mm::play, &gfx);
    assert(draws == 1 && "later-page reward icon appeared on the title page");
    Mm::play.msgCtx.msgBufPos = Mm::sItemReceiptLayout.iconPageStart + 11;
    Mm::Message_ApplyItemReceiptLayout(&Mm::play);
    Mm::Message_DrawText(&Mm::play, &gfx);
    assert(draws == 2 && path == "__OTR__@mm:icon_item_static_yar/gItemIconSongNoteTex");
    assert(format == G_IM_FMT_IA && bits == G_IM_SIZ_8b && sourceWidth == 16 && sourceHeight == 24);
    assert(x2 - x1 == 16 && y2 - y1 == 24 && x2 <= 285 && y1 >= 22 && y2 <= 76);
    assert(red == 128 && green == 216 && blue == 240 && alpha == 192);
    Mm::play.msgCtx.msgBufPos = Mm::sItemReceiptLayout.firstPageEnd + 12;
    Mm::Message_ApplyItemReceiptLayout(&Mm::play);
    Mm::Message_DrawText(&Mm::play, &gfx);
    assert(draws == 2 && Mm::play.msgCtx.textCharScale == 0.75f); // attribution's next page is ordinary
    Mm::Message_SetItemReceiptPresentation(nullptr);
    Mm::FinishReceiptDecodePosition(0);
    assert(Mm::play.msgCtx.unk11FFA == Mm::play.msgCtx.textboxY + 26 && "ordinary native receipt centering changed");
    Mm::play.msgCtx.unk11FFA = 28;
    Mm::play.msgCtx.itemId = 1;
    Mm::Message_DrawText(&Mm::play, &gfx);
    assert(draws == 2 && Mm::nativeIconDraws == 1);
    p.iconWidth = 65;
    Mm::Message_SetItemReceiptPresentation(&p);
    Mm::Message_ApplyItemReceiptLayout(&Mm::play);
    Mm::Message_DrawText(&Mm::play, &gfx);
    assert(draws == 2 && Mm::nativeIconDraws == 1 && !ComboReceipt_HasIcon(&Mm::sItemReceiptPresentation));
    Mm::gSaveContext.options.language = Mm::LANGUAGE_JPN;
    Mm::Message_DrawText(&Mm::play, &gfx);
    assert(Mm::japaneseDraws == 1 && draws == 2);
    Mm::play.msgCtx.textIsCredits = true;
    Mm::Message_DrawText(&Mm::play, &gfx);
    assert(Mm::creditsDraws == 1 && draws == 2);
    // NES spaces have a fixed six-pixel advance even when glyphs shrink.
    p = {};
    p.singleBox = 1;
    p.rewardLine = 2;
    const CwItemIconInfo key{ "__OTR__icon_item_static_yar/gItemIconBossKeyTex", 32, 32, 0, 0, {} };
    assert(ComboReceipt_CopyIcon(&p, &key, "mm"));
    const std::string reward = "Defeating the boss grants the Stone Tower Boss Key!";
    const std::string compact = ComboItemReceiptText::FromNeiMarkup(
        "You found the Stone Tower Compass!&It points to Twinmold!&" + reward);
    std::memcpy(Mm::play.msgCtx.font.msgBuf.schar + 11, compact.data(), compact.size());
    Mm::play.msgCtx.font.msgBuf.schar[compact.size() + 11] = '\xBF';
    Mm::play.msgCtx.msgLength = compact.size() + 12;
    Mm::play.msgCtx.msgBufPos = 0;
    Mm::play.msgCtx.textIsCredits = false;
    Mm::gSaveContext.options.language = Mm::LANGUAGE_ENG;
    Mm::Message_SetItemReceiptPresentation(&p);
    Mm::Message_ApplyItemReceiptLayout(&Mm::play);
    CheckReflow(compact, Mm::play.msgCtx.font.msgBuf.schar + 11, Mm::play.msgCtx.msgLength - 11,
                true, Mm::sNESFontWidths, 160);
    const std::string wrapped(Mm::play.msgCtx.font.msgBuf.schar + 11, Mm::play.msgCtx.msgLength - 11);
    assert(Mm::sItemReceiptLayout.firstPageEnd < wrapped.size() &&
           static_cast<uint8_t>(wrapped[Mm::sItemReceiptLayout.firstPageEnd]) == 0xBF &&
           "reward endpoint must still address its native terminator after reflow");
    const size_t lastBreak = wrapped.find_last_of("\x10\x11", Mm::sItemReceiptLayout.firstPageEnd);
    const std::string lastLine = wrapped.substr(lastBreak + 1, Mm::sItemReceiptLayout.firstPageEnd - lastBreak - 1);
    const int nativeEnd = ComboReceipt_LineWidth(lastLine.data(), lastLine.size(), true, Mm::sNESFontWidths, 160);
    assert(Mm::sItemReceiptLayout.iconX >= nativeEnd + 4 && "reward text overlaps its final-line sprite");
    size_t lineBegin = Mm::sItemReceiptLayout.iconPageStart;
    for (size_t i = lineBegin; i <= Mm::sItemReceiptLayout.firstPageEnd; ++i) {
        if (i == Mm::sItemReceiptLayout.firstPageEnd || wrapped[i] == '\x11') {
            assert(ComboReceipt_LineWidth(wrapped.data() + lineBegin, i - lineBegin, true,
                                          Mm::sNESFontWidths, 160) + 4 <= Mm::sItemReceiptLayout.iconX &&
                   "tall sprite must clear the wrapped reward's preceding row");
            lineBegin = i + 1;
        }
    }
    assert(Mm::sItemReceiptLayout.iconX + Mm::sItemReceiptLayout.iconWidth <= 220);
    (void)Mm::NativeLineWidth(reward);
    // The reported Spirit/Shadow entrance must never strand its final words
    // on a second page. Multiple inverse entrances retain every name too.
    for (const std::string entrance : {"Deku Tree", "Shadow Temple", "Shadow Temple Entryway", "GV Behind Tent Grotto Entry",
            "GV Behind Tent Grotto Entry, Bottom of the Well, Dodongo's Cavern"}) {
        const std::string map = ComboItemReceiptText::FromNeiMarkup(
            "You found the Ice Cavern Map!&It's %gordinary%w.&It seems the entrance is at %c" + entrance + "%w.");
        p = {};
        p.singleBox = 1;
        std::memcpy(Mm::play.msgCtx.font.msgBuf.schar + 11, map.data(), map.size());
        Mm::play.msgCtx.font.msgBuf.schar[map.size() + 11] = '\xBF';
        Mm::play.msgCtx.msgLength = map.size() + 12;
        Mm::play.msgCtx.msgBufPos = 0;
        Mm::Message_SetItemReceiptPresentation(&p);
        Mm::Message_ApplyItemReceiptLayout(&Mm::play);
        assert(Mm::play.msgCtx.textCharScale >= 0.55f && Mm::play.msgCtx.textCharScale <= 0.75f);
        assert(entrance != "Deku Tree" || Mm::play.msgCtx.textCharScale == 0.75f);
        CheckReflow(map, Mm::play.msgCtx.font.msgBuf.schar + 11, Mm::play.msgCtx.msgLength - 11,
                    true, Mm::sNESFontWidths, 160, Mm::sItemReceiptLayout.textScale);
        assert(!std::memchr(Mm::play.msgCtx.font.msgBuf.schar + 11, '\x10', Mm::play.msgCtx.msgLength - 11) &&
               "a short map hint must fit one native textbox without truncation");
        assert(Mm::sCharTexSize == static_cast<int>(16 * Mm::play.msgCtx.textCharScale));
        // The same receipt also needs to fit in OoT, retaining all words.
        const std::string ootMap = "You found the Ice Cavern Map!\x01It's \x05\x42ordinary\x05\x40.\x01"
                                   "It seems the entrance is at \x05\x44" + entrance + "\x05\x40.\x02";
        std::memcpy(Oot::play.msgCtx.font.msgBuf, ootMap.data(), ootMap.size());
        Oot::play.msgCtx.msgLength = ootMap.size();
        Oot::play.msgCtx.msgBufPos = 0;
        Oot::Message_SetItemReceiptPresentation(&p);
        Oot::Message_ApplyItemReceiptLayout(&Oot::play);
        assert(!std::memchr(Oot::play.msgCtx.font.msgBuf, '\x04', Oot::play.msgCtx.msgLength));
        assert(Oot::R_TEXT_CHAR_SCALE >= 55 && Oot::R_TEXT_CHAR_SCALE <= 75);
        CheckReflow(ootMap, Oot::play.msgCtx.font.msgBuf, Oot::play.msgCtx.msgLength,
                    false, Oot::sFontWidths, 144, Oot::R_TEXT_CHAR_SCALE);
    }
    // Shrinking a map must not shrink a later attribution page or the next
    // ordinary dialogue. Explicit authored page breaks remain intact.
    const std::string attributedMap = ComboItemReceiptText::FromNeiMarkup(
        "You found the Spirit Temple Map!&It's %gordinary%w.&It seems the entrance is at %cShadow Temple Entryway%w.") +
        "\x10" "Bank reward attribution." + '\xBF';
    p = {};p.singleBox=1;
    std::memcpy(Mm::play.msgCtx.font.msgBuf.schar+11, attributedMap.data(), attributedMap.size());
    Mm::play.msgCtx.msgLength=attributedMap.size()+11;Mm::play.msgCtx.msgBufPos=0;
    Mm::Message_SetItemReceiptPresentation(&p);Mm::Message_ApplyItemReceiptLayout(&Mm::play);
    assert(Mm::play.msgCtx.textCharScale<.75f);
    const std::string attributed(Mm::play.msgCtx.font.msgBuf.schar+11,Mm::play.msgCtx.msgLength-11);
    assert(std::count(attributed.begin(),attributed.end(),'\x10')==1 && attributed.find("Bank reward attribution.")!=std::string::npos);
    Mm::play.msgCtx.msgBufPos=Mm::sItemReceiptLayout.firstPageEnd+12;
    Mm::Message_ApplyItemReceiptLayout(&Mm::play);
    assert(Mm::play.msgCtx.textCharScale==.75f && Mm::Message_ItemReceiptSpaceWidth(6)==6);
    Mm::Message_SetItemReceiptPresentation(nullptr);
    assert(Mm::Message_ItemReceiptSpaceWidth(9)==9);
    char shortBuffer[32]="WWWWWWWWWWWWWWWWWWWWWWWWWWWWWW";
    const std::string before(shortBuffer,30);
    const auto noCapacity=ComboReceipt_Layout(&p,shortBuffer,30,30,true,Mm::sNESFontWidths,160);
    assert(noCapacity.textScale==75 && noCapacity.bodySize==30 && std::string(shortBuffer,30)==before);
    // MM's END byte also indexes a ten-pixel font slot. It is a command, so
    // these final lines must retain their exact three-line body at 218-220px.
    for (const std::string entrance : {"Zora Shop", "LLR Tower", "MK Bazaar"}) {
        Mm::Message_SetItemReceiptPresentation(nullptr);
        Mm::play.msgCtx.textCharScale = 0.75f;
        const std::string finalLine = "It seems the entrance is at " + entrance + ".";
        const int nativeWidth = Mm::NativeLineWidth(finalLine);
        assert(nativeWidth >= 211 && nativeWidth <= 220);
        const std::string map = ComboItemReceiptText::FromNeiMarkup(
            "You found the Ice Cavern Map!&It's %gordinary%w.&It seems the entrance is at %c" + entrance + "%w.") + '\xBF';
        p = {};
        p.singleBox = 1;
        std::memcpy(Mm::play.msgCtx.font.msgBuf.schar + 11, map.data(), map.size());
        Mm::play.msgCtx.msgLength = map.size() + 11;
        Mm::play.msgCtx.msgBufPos = 0;
        Mm::Message_SetItemReceiptPresentation(&p);
        Mm::Message_ApplyItemReceiptLayout(&Mm::play);
        assert(static_cast<size_t>(Mm::play.msgCtx.msgLength) == map.size() + 11 &&
               !std::memcmp(Mm::play.msgCtx.font.msgBuf.schar + 11, map.data(), map.size()) &&
               "zero-width END must not move a fitting final word onto another page");
        Mm::FinishReceiptDecodePosition(2);
        assert(Mm::play.msgCtx.unk11FFA == Mm::play.msgCtx.textboxY + 16);
        assert(ComboReceipt_LineWidth("\x10\xBF", 2, true, Mm::sNESFontWidths, 160) == 0);
        CheckReflow(map, Mm::play.msgCtx.font.msgBuf.schar + 11, Mm::play.msgCtx.msgLength - 11,
                    true, Mm::sNESFontWidths, 160);
    }
    // The requested receipt uses a real reward sprite beside its second line.
    p = {};
    p.singleBox = 1;
    p.rewardLine = 1;
    const CwItemIconInfo emerald{ "__OTR__textures/icon_item_24_static/gQuestIconKokiriEmeraldTex", 24, 24, 0, 0, {} };
    assert(ComboReceipt_CopyIcon(&p, &emerald, "oot"));
    const std::string bossLine = "It points to Queen Gohma";
    const std::string ootReceipt = "You received a Deku Tree Compass!\x01" + bossLine + '\x02';
    Oot::play.msgCtx.msgBufPos = 0;
    std::memcpy(Oot::play.msgCtx.font.msgBuf, ootReceipt.data(), ootReceipt.size());
    Oot::play.msgCtx.msgLength = ootReceipt.size();
    Oot::Message_SetItemReceiptPresentation(&p);
    Oot::Message_ApplyItemReceiptLayout(&Oot::play);
    Oot::FinishReceiptDecodePosition(1);
    const std::string ootWrapped(Oot::play.msgCtx.font.msgBuf, Oot::play.msgCtx.msgLength);
    assert(std::count(ootWrapped.begin(), ootWrapped.end(), '\x01') == 1 && ootWrapped.find('\x04') == std::string::npos);
    assert(Oot::sItemReceiptLayout.iconPageStart == 0 && Oot::sItemReceiptLayout.iconY == 12);
    assert(Oot::sItemReceiptLayout.iconX >= ComboReceipt_LineWidth(bossLine.data(), bossLine.size(), false, Oot::sFontWidths, 144) + 4);
    Oot::play.msgCtx.textPosY = Oot::R_TEXT_INIT_YPOS + 12;
    const int beforeOot = draws;
    Oot::Message_DrawItemReceiptIcon(&Oot::play, &gfx);
    assert(draws == beforeOot + 1 && sourceWidth == 24 && sourceHeight == 24);
    assert(path == "__OTR__@oot:textures/icon_item_24_static/gQuestIconKokiriEmeraldTex");
    assert(y1 == Oot::R_TEXT_INIT_YPOS + 12 && x2 <= 285 && y2 <= 76);

    const std::string mmReceipt = ComboItemReceiptText::FromNeiMarkup("You received a Deku Tree Compass!&" + bossLine) + '\xBF';
    Mm::play.msgCtx.msgBufPos = 0;
    std::memcpy(Mm::play.msgCtx.font.msgBuf.schar + 11, mmReceipt.data(), mmReceipt.size());
    Mm::play.msgCtx.msgLength = mmReceipt.size() + 11;
    Mm::Message_SetItemReceiptPresentation(&p);
    Mm::Message_ApplyItemReceiptLayout(&Mm::play);
    Mm::FinishReceiptDecodePosition(1);
    const std::string mmWrapped(Mm::play.msgCtx.font.msgBuf.schar + 11, Mm::play.msgCtx.msgLength - 11);
    assert(std::count(mmWrapped.begin(), mmWrapped.end(), '\x11') == 1 && mmWrapped.find('\x10') == std::string::npos);
    assert(Mm::sItemReceiptLayout.iconPageStart == 0 && Mm::sItemReceiptLayout.iconY == 12);
    assert(Mm::sItemReceiptLayout.iconX >= Mm::NativeLineWidth(bossLine) + 4);
    const int beforeMm = draws;
    assert(Mm::Message_DrawItemReceiptIcon(&Mm::play, &gfx));
    assert(draws == beforeMm + 1 && sourceWidth == 24 && sourceHeight == 24);
    assert(path == "__OTR__@oot:textures/icon_item_24_static/gQuestIconKokiriEmeraldTex");
    assert(y1 == Mm::play.msgCtx.unk11FFA + 12 && x2 <= 285 && y2 <= 76);
    std::cout << "Real OoT/MM native typography, two-line boss/reward sprites, wrapping/pages, English dispatch and reset passed\n";
}
