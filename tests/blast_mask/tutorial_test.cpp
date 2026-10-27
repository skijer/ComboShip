#include "global.h"
#include "soh/Enhancements/custom-message/CustomMessageManager.h"
#include "soh/util.h"
#include <cassert>
#include <cstdio>
#include <cstring>

SaveContext gSaveContext{};
static GameInfo registers{};
GameInfo *gGameInfo = &registers;
static PlayState play;
static GraphicsContext graphics;
static Gfx commands[256];
PlayState *gPlayState = &play;
extern "C" void Message_Decode(PlayState *);
extern "C" void MarketTest_RenderControls(PlayState *);
extern "C" void
Message_SetItemReceiptPresentation(const CwItemReceiptPresentation *) {}
extern "C" void Message_SetCapeVisibilityChoice(int) {}
extern "C" int32_t CVarGetInteger(const char *, int32_t fallback) {
  return fallback;
}

/* TUTORIAL_TEXT */
/* NATIVE_COPY */

static std::string Decode() {
  std::string visible;
  int pages = 0;
  bool sawA = false, sawB = false;
  for (;;) {
    graphics.polyOpa.p = commands;
    Message_Decode(&play);
    auto &message = play.msgCtx;
    assert(++pages < 15);
    assert(message.decodedTextLen < ARRAY_COUNT(message.msgBufDecoded));
    unsigned lines = 1;
    for (unsigned i = 0; i < message.decodedTextLen; ++i) {
      const u8 glyph = message.msgBufDecoded[i];
      if (glyph == 0x01) {
        ++lines;
        visible += ' ';
      } else if (glyph == 0x05 || glyph == 0x13) {
        assert(++i < message.decodedTextLen);
      } else {
        // Unsupported raw UTF-8/control bytes must never reach the font.
        assert(glyph >= 0x20 && glyph <= 0xAA);
        visible += char(glyph);
      }
    }
    assert(lines <= 4);
    for (size_t offset = 0; offset < sizeof(message.font.charTexBuf);
         offset += FONT_CHAR_TEX_SIZE) {
      const char *texture =
          reinterpret_cast<const char *>(&message.font.charTexBuf[offset]);
      sawA |= std::strcmp(
                  texture,
                  "__OTR__textures/nes_font_static/gMsgChar9FButtonATex") == 0;
      sawB |= std::strcmp(
                  texture,
                  "__OTR__textures/nes_font_static/gMsgCharA0ButtonBTex") == 0;
    }
    const u8 end = message.msgBufDecoded[message.decodedTextLen];
    if (end == 0x04) {
      ++message.msgBufPos;
      visible += ' ';
      continue;
    }
    assert(end == 0x02);
    assert(message.msgBufPos < message.font.msgLength);
    MarketTest_RenderControls(&play);
    assert(Message_GetState(&message) == TEXT_STATE_DONE);
    assert(sawA && sawB);
    return visible;
  }
}

int main() {
  play.state.gfxCtx = &graphics;
  const u8 locales[] = {LANGUAGE_ENG, LANGUAGE_FRA, LANGUAGE_GER};
  const char *endings[] = {"1 frame.", "1 frame.", "1 Frame."};
  bool readable = true;
  for (unsigned language = 0; language < ARRAY_COUNT(locales); ++language) {
    gSaveContext.language = locales[language];
    play.msgCtx = {};
    // Registry rows are English/French/German; CustomMessage's ABI is E/G/F.
    CustomMessage message(tutorials[0], tutorials[2], tutorials[1],
                          TEXTBOX_TYPE_BLUE);
    message.AutoFormat(ITEM_MM_MASK_BLAST);
    message.LoadIntoFont();
    const auto visible = Decode();
    assert(visible.ends_with(endings[language]));
    // A setting's UI label is readable; its internal CVar is developer text.
    readable &= visible.find("gMods.") == std::string::npos;
    assert(visible.find("Instant Blast Mask") != std::string::npos);
  }
  assert(readable &&
         "the rendered lesson must use the setting's readable UI name");
  puts("PASS Blast Mask receipt: native formatter/font/decoder, complete "
       "localized ending, pages and DONE state");
}
