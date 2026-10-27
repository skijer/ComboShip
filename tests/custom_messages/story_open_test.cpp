// Actual story registration, GameInteractor registry, formatter and native
// message open/decode. Graphics, audio and unrelated receipt services are the
// boundaries; no game or save is run.
#include "global.h"
#include "message_data_static.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"
#include "soh/util.h"
extern "C" {
#include "src/overlays/actors/ovl_En_Viewer/static_story_dialogue.h"
void Message_OpenText(PlayState *, u16);
void Message_Decode(PlayState *);
}
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>

SaveContext gSaveContext{};
u32 gBitFlags[32]{};
u8 gEquipShifts[4]{};
static GameInfo registers{};
GameInfo *gGameInfo = &registers;
static PlayState play;
PlayState *gPlayState = &play;
static GraphicsContext graphics;
static Gfx commands[512];
static char textbox[4096];
static GameInteractor interactor;
GameInteractor *GameInteractor::Instance = &interactor;
static const char vanilla[] = "Native table.\x02";
static MessageTableEntry table[] = {
    {0x1234, 0x02, vanilla, sizeof(vanilla) - 1},
    {0xFFFF, 0x02, vanilla + sizeof(vanilla) - 1, 0},
};
extern "C" {
MessageTableEntry *sNesMessageEntryTablePtr = table;
MessageTableEntry *sGerMessageEntryTablePtr = table;
MessageTableEntry *sFraMessageEntryTablePtr = table;
MessageTableEntry *sJpnMessageEntryTablePtr = table;
MessageTableEntry *sStaffMessageEntryTablePtr = table;
void Interface_ChangeHudVisibilityMode(u16) {}
void Interface_SetDoAction(PlayState *, u16) {}
s32 Flags_GetEventChkInf(s32) { return 0; }
size_t GetEquipNowMessage(char *, char *, size_t) {
  assert(false);
  return 0;
}
int32_t CVarGetInteger(const char *, int32_t fallback) { return fallback; }
bool GameInteractor_Should(GIVanillaBehavior, uint32_t result, ...) {
  return result;
}
}
#ifdef COMBO_BUILD
// ShipInit's RM guard is outside the message/registration boundary.
Ship::OwnRMScope::OwnRMScope(const char *) {}
Ship::OwnRMScope::~OwnRMScope() = default;
#endif

/* NATIVE_COPY */
extern "C" {
/* NATIVE_OPEN_HOOK */
}

static void ResetMessage() {
  play.msgCtx = {};
  play.msgCtx.textboxSegment = textbox;
  graphics.polyOpa.p = commands;
  play.state.gfxCtx = &graphics;
}

static std::string Decode() {
  graphics.polyOpa.p = commands;
  Message_Decode(&play);
  const auto &context = play.msgCtx;
  assert(context.decodedTextLen < ARRAY_COUNT(context.msgBufDecoded));
  assert(context.msgBufDecoded[context.decodedTextLen] == '\x02');
  std::string body;
  for (unsigned i = 0; i < context.decodedTextLen; ++i) {
    const char c = context.msgBufDecoded[i];
    body += c == '\x01' ? ' ' : c;
  }
  return body;
}

static void CheckCustom(u16 id, const char* expectedText = nullptr) {
  ResetMessage();
  bool fromTable = true;
  GameInteractor_ExecuteOnOpenText(&id, &fromTable);
  if (fromTable) {
    std::fprintf(stderr,
                 "FAIL registered story text 0x%04X falls through to native "
                 "message table\n",
                 id);
    std::abort();
  }
  // Exercise the entire native opening path with poisoned table metadata.
  // A missing hook must never reach its absent-ID/null-offset fallback.
  play.msgCtx.font.msgOffset = 0;
  play.msgCtx.font.msgLength = 777;
  play.msgCtx.msgLength = 777;
  graphics.polyOpa.p = commands;
  Message_StartTextbox(&play, id, nullptr);
  assert(play.msgCtx.textId == id && play.msgCtx.msgMode == MSGMODE_TEXT_START);
  assert(play.msgCtx.msgLength > 0 &&
         play.msgCtx.msgLength < sizeof(play.msgCtx.font.msgBuf));
  assert(play.msgCtx.font.msgLength == play.msgCtx.msgLength);
  const std::string expected =
      expectedText != nullptr ? expectedText : StaticStoryDialogue_GetText(id, gSaveContext.language);
  assert(Decode() == expected);
  Message_CloseTextbox(&play);
  assert(play.msgCtx.msgMode == MSGMODE_TEXT_CLOSING &&
         play.msgCtx.stateTimer == 2);
  graphics.polyOpa.p = commands;
  Message_ContinueTextbox(&play, id);
  assert(play.msgCtx.msgMode == MSGMODE_TEXT_CONTINUING &&
         play.msgCtx.stateTimer == 3);
  assert(Decode() == expected);
}

int main() {
  ShipInit::InitAll();
  // Explicit IDs/copy ensure missing Zelda catalog or registration fails decisively.
  for (u8 language : {LANGUAGE_ENG, LANGUAGE_GER, LANGUAGE_FRA, LANGUAGE_JPN}) {
    gSaveContext.language = language;
    CheckCustom(0x8F2B, "Do you hear it? The stones are humming... as if they remember your footsteps.");
    CheckCustom(0x8F2C,
                "Every path leaves an echo in this chamber. Some wake to a song... others to a promise fulfilled.");
    CheckCustom(0x8F2D, "A fairy boy... I thought the forest kept its secrets. Did your fairy guide you here?");
    for (size_t entry = 0; const u16 id = StaticStoryDialogue_GetTextIdAt(entry); ++entry)
      CheckCustom(id);
    bool fromTable = true;
    u16 ordinary = 0x1234;
    GameInteractor_ExecuteOnOpenText(&ordinary, &fromTable);
    assert(fromTable && ordinary == 0x1234);
  }
  // Re-running initialization must retain one delivery per ID.
  ShipInit::InitAll();
  gSaveContext.language = LANGUAGE_ENG;
  for (size_t entry = 0; const u16 id = StaticStoryDialogue_GetTextIdAt(entry); ++entry) {
    CheckCustom(id);
    assert(GameInteractor::RegisteredGameHooks<
               GameInteractor::OnOpenText>::functionsForID[id]
               .size() == 1);
  }
  ResetMessage();
  Message_StartTextbox(&play, 0x1234, nullptr);
  assert(Decode() == "Native table.");
  puts("PASS story registration -> native opening/decoding/closing/reopening: "
       "plain child/chamber Zelda and all catalog IDs, "
       "four languages, repeat registration and native-table passthrough "
       "(ASan/UBSan)");
}
