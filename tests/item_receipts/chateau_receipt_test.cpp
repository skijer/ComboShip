// The runner inserts the real receipt callback, constructors, formatters,
// registration expressions, and hook registration/dispatch bodies. Only engine
// font staging and hook metadata are replaced; this does not run either game.
#include "combo/menu/ComboItemReceiptPresentation.h"
#include "soh/include/message_data_textbox_types.h"
#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std::literals::string_literals;
enum { LANGUAGE_ENG, LANGUAGE_GER, LANGUAGE_FRA };
/* MESSAGE_MACROS */
#define SPDLOG_DEBUG(...) ((void)0)

static int language = LANGUAGE_ENG;
static std::string loadedBody;
static unsigned loadCalls;

struct CustomMessage {
  std::vector<std::string> messages{"", TODO_TRANSLATE, TODO_TRANSLATE};
  std::vector<std::string> colors;
  TextBoxType type = TEXTBOX_TYPE_BLACK;
  TextBoxPosition position = TEXTBOX_POS_BOTTOM;
  CustomMessage(std::string english_, std::string german_, std::string french_,
                TextBoxType type_ = TEXTBOX_TYPE_BLACK,
                TextBoxPosition position_ = TEXTBOX_POS_BOTTOM);
  CustomMessage(std::string english_, TextBoxType type_ = TEXTBOX_TYPE_BLACK,
                TextBoxPosition position_ = TEXTBOX_POS_BOTTOM);
  void Format();
  void FormatString(std::string &) const;
  void ReplaceSpecialCharacters(std::string &) const;
  void ReplaceColors(std::string &) const;
  void ReplaceAltarIcons(std::string &) const;
  void EncodeColors(std::string &) const;
  static std::string MESSAGE_END();
  static std::string NEWLINE();
  static std::string WAIT_FOR_INPUT();
  static std::string PLAYER_NAME();
  static std::string TWO_WAY_CHOICE();
  static std::string COLOR(std::string);
  static std::string ITEM_OBTAINED(uint8_t);

  // Capture the locale bytes that LoadIntoFont would stage. Neither target
  // message has receipt metadata, cape choices, or altar-icon markup.
  void LoadIntoFont() {
    loadedBody = messages[language].starts_with(TODO_TRANSLATE)
                     ? messages[LANGUAGE_ENG]
                     : messages[language];
    ++loadCalls;
  }
};
static const std::unordered_map<std::string, uint8_t> altarIcons;
/* MESSAGE_DATA */
/* MESSAGE_METHODS */
/* CHATEAU_CALLBACK */
/* CONTINUE_MESSAGE */

using HOOK_ID = uint32_t;
#define GET_CURRENT_REGISTERING_INFO(...) 0
#define HOOK_TYPE_ID 0
struct HookInfo {
  unsigned calls;
  int registeringInfo;
};

class GameInteractor {
  uint32_t nextHookId = 1;

public:
  inline static GameInteractor *Instance;
  struct OnOpenText {
    using fn = std::function<void(uint16_t *, bool *)>;
  };
  template <typename H> struct RegisteredGameHooks {
    inline static std::unordered_map<
        int32_t, std::unordered_map<HOOK_ID, typename H::fn>>
        functionsForID;
    inline static std::map<HOOK_ID, HookInfo> hookData;
  };
  template <typename H> struct HooksToUnregister {
    inline static std::vector<HOOK_ID> hooksForID;
  };

  // Source-location bookkeeping is omitted; the production method bodies
  // below still own ID allocation, deferred removal, and execution order.
  template <typename H>
  HOOK_ID RegisterGameHookForID(int32_t id, typename H::fn h) {
    /* REGISTER_HOOK_BODY */
  }
  template <typename H> void UnregisterGameHookForID(HOOK_ID hookId) {
    /* UNREGISTER_HOOK_BODY */
  }
  template <typename H, typename... Args>
  void ExecuteHooksForID(int32_t id, Args &&...args) {
    /* DISPATCH_HOOK_BODY */
  }

  void ClearForTest() {
    RegisteredGameHooks<OnOpenText>::functionsForID.clear();
    RegisteredGameHooks<OnOpenText>::hookData.clear();
    HooksToUnregister<OnOpenText>::hooksForID.clear();
    // Keep IDs monotonic: production registration functions retain their
    // previous static hook ID when the next scenario re-registers them.
  }
};

/* RECEIPT_IDS */
/* CONDITIONAL_ID_HOOK */
#define CVAR_BETTERSAVE_VALUE 1
/* CHATEAU_REGISTRATION */
void RegisterContinueMessage() { /* CONTINUE_REGISTRATION */
}

static unsigned failures;
static void Require(bool condition, const std::string &reason) {
  if (!condition) {
    if (++failures <= 12)
      std::cerr << "FAIL: " << reason << '\n';
  }
}

static void CheckGlyphRange(const std::string &body, int locale) {
  size_t decoded = 0, glyphs = 0;
  bool ended = false;
  for (size_t offset = 0; offset < body.size();) {
    const uint8_t c = body[offset];
    const size_t command = ComboReceipt_CommandSize(c, false);
    if (command > body.size() - offset) {
      Require(false, "truncated receipt command");
      return;
    }
    decoded += command;
    if (command == 1 && c >= 0x20 && c != ' ') {
      ++glyphs;
      // Guard before accessing the actual width table. An invalid glyph
      // fails deterministically without reproducing the game's OOB read.
      const size_t index = c - ' ';
      Require(index < std::size(sFontWidths),
              "Chateau locale " + std::to_string(locale) + " emits glyph " +
                  std::to_string(c) + " outside the native font width table");
      if (index < std::size(sFontWidths))
        Require(sFontWidths[index] >= 0, "native glyph width is invalid");
    }
    if (c == 0x04 || c == 0x02) {
      Require(decoded <= 200 && glyphs <= 256,
              "receipt exceeds OoT's decoded/font page buffers");
      decoded = glyphs = 0;
    }
    if (c == 0x02) {
      Require(offset + command == body.size(), "receipt has bytes after END");
      ended = true;
    }
    offset += command;
  }
  Require(ended, "receipt has no END");
}

static std::string OpenText(uint16_t id) {
  loadedBody.clear();
  loadCalls = 0;
  bool loadFromTable = true;
  GameInteractor::Instance->ExecuteHooksForID<GameInteractor::OnOpenText>(
      id, &id, &loadFromTable);
  Require(!loadFromTable && loadCalls == 1,
          "a receipt ID must select exactly one message handler");
  return loadedBody;
}

static void CheckBothMessages(const std::string &scenario) {
  for (language = LANGUAGE_ENG; language <= LANGUAGE_FRA; ++language) {
    const auto continuation = OpenText(kContinueId);
    Require(continuation.find("Continue?") != std::string::npos &&
                continuation.find(CustomMessage::TWO_WAY_CHOICE()) !=
                    std::string::npos,
            scenario +
                ": overworld save continuation must retain its choice prompt");
    Require(continuation.find("Chateau Romani") == std::string::npos,
            scenario + ": Chateau receipt overwrote the save continuation");
    const auto receipt = OpenText(kCowReceiptId);
    Require(receipt.find("Chateau Romani") != std::string::npos &&
                receipt.find(CustomMessage::TWO_WAY_CHOICE()) ==
                    std::string::npos &&
                receipt.find("Continue?") == std::string::npos,
            scenario + ": the cow pickup must retain its own receipt");
  }
}

int main() {
  Require(kChateauRegistrationId == kCowReceiptId,
          "cow entry and Chateau callback must use the same text ID");
  Require(kChateauRegistrationId != kContinueId,
          "Chateau and overworld save continuation must use distinct IDs");

  for (language = LANGUAGE_ENG; language <= LANGUAGE_FRA; ++language) {
    bool loadFromTable = true;
    uint16_t id = kCowReceiptId;
    BuildChateauRomaniMessage(&id, &loadFromTable);
    Require(!loadFromTable, "Chateau callback must stage its receipt");
    CheckGlyphRange(loadedBody, language);
  }

  continueOverworldMsg.Format();
  GameInteractor hooks;
  GameInteractor::Instance = &hooks;
  for (bool chateauFirst : {true, false}) {
    hooks.ClearForTest();
    if (chateauFirst) {
      RegisterChateauRomaniMessage();
      RegisterContinueMessage();
    } else {
      RegisterContinueMessage();
      RegisterChateauRomaniMessage();
    }
    CheckBothMessages(chateauFirst ? "Chateau registered first"
                                   : "Continue registered first");
    RegisterChateauRomaniMessage();
    CheckBothMessages("Chateau re-registered");
    RegisterContinueMessage();
    CheckBothMessages("Continue re-registered");
  }
  if (failures) {
    std::cerr << failures << " Chateau receipt regression checks failed\n";
    return 1;
  }
  std::cout << "PASS localized Chateau glyphs, distinct cow/save IDs, both "
               "hook orders and re-registration\n";
}
