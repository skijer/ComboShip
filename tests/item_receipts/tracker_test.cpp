#include "combo/rando/CrossHints.h"
#include <cassert>
#include <mutex>

using json = nlohmann::json;
static std::mutex g_containerMutex;
static json container;
static json pushedHints, pushedRead;
static int pushedSlot = -1, exportReads = 0;
static std::string exportedMessages = "{}";
static bool exportThrows = false;
static bool exportNull = false;
static bool ComboIsValidSlot(int slot) { return slot >= 0 && slot < 3; }
static json &LoadOrCreateContainer(int slot) {
  assert(slot == 0);
  return container;
}
static void CaptureTracker(int slot, const char *hints, const char *read) {
  pushedSlot = slot;
  pushedHints = json::parse(hints);
  pushedRead = json::parse(read);
}
static const char *ExportRequirements() {
  ++exportReads;
  assert(g_containerMutex
             .try_lock()); // host callbacks must run outside container lock
  g_containerMutex.unlock();
  if (exportThrows)
    throw 1;
  if (exportNull)
    return nullptr;
  return exportedMessages.c_str();
}
static auto ComboUI_SetHintTrackerData = CaptureTracker;
static const char *(*SOH_DumpAltarHintMessages)() = ExportRequirements;
/* TRACKER_PUSH */

static json Messages(const std::string &text) {
  return json::array({{{"en", text}, {"de", text}, {"fr", text}}});
}
static const json *FindHint(const json &hints, const char *key) {
  for (const auto &hint : hints.at("oot"))
    if (hint.at("checkName") == key)
      return &hint;
  return nullptr;
}
int main() {
  const json dump{{"items", json::array()},
                  {"pool", json::array()},
                  {"checks", json::array()}};
  const json seed{{"oot", json::object()},
                  {"mm", json::object()},
                  {"foreign", json::array()}};
  json hintDump{{"options",
                 {{"totAltarHint", 1},
                  {"hintClarity", 2},
                  {"doorOfTimeTemplate", "DOOR"},
                  {"bridgeTemplate", "BRIDGE"},
                  {"bridgeCount", 2},
                  {"gbkTemplate", "KEY"},
                  {"gbkCount", 3},
                  {"soulTemplate", "SOUL"},
                  {"soulCount", 4},
                  {"winconTemplate", "WIN"},
                  {"winconCount", 5}}},
                {"hintTextTable", json::object()}};
  for (const auto &[key, text] : std::map<std::string, std::string>{
           {"RHT_CHILD_ALTAR_STONES", "STONE LOCATIONS [[1]] [[2]] [[3]];"},
           {"RHT_ADULT_ALTAR_MEDALLIONS",
            "MEDALLION LOCATIONS [[1]] [[2]] [[3]] [[4]] [[5]] [[6]];"},
           {"DOOR", "DOOR REQUIREMENT;"},
           {"BRIDGE", "BRIDGE [[d]];"},
           {"KEY", "KEY [[d]];"},
           {"SOUL", "SOUL [[d]];"},
           {"WIN", "WIN [[d]];"},
           {"RHT_ADULT_ALTAR_TEXT_END", "END;"}})
    hintDump["hintTextTable"][key] = {{"clear", Messages(text)[0]}};

  auto generate = [&] {
    return ComboRando::Generate(42, dump.dump(), hintDump.dump(), dump.dump(),
                                json::array(), seed.dump(), {});
  };
  const auto baseline = generate(); // missing option is exactly off
  hintDump["options"]["mapsCompassesGiveInformation"] = 0;
  assert(generate() == baseline);
  hintDump["options"]["mapsCompassesGiveInformation"] = 1;
  for (int altar : {1, 0}) {
    hintDump["options"]["totAltarHint"] = altar;
    const auto enabled = generate();
    const auto *child = FindHint(enabled, "__ALTAR_CHILD__");
    const auto *adult = FindHint(enabled, "__ALTAR_ADULT__");
    assert(child && adult);
    for (const char *language : {"en", "de", "fr"}) {
      assert(child->at("messages")[0][language] == "DOOR REQUIREMENT;");
      assert(adult->at("messages")[0][language] ==
             "BRIDGE 2;KEY 3;SOUL 4;WIN 5;END;");
    }
  }

  // An existing baked seed can still carry its old full altar strings. Only
  // the loaded tracker copy is normalized, based on the slot's saved option.
  json baked = baseline;
  baked["oot"].push_back(
      {{"checkName", "__JUNK__0"}, {"messages", Messages("Unrelated hint")}});
  container = json::object();
  container["combo"]["rando"] = {{"oot", {{"settings", json::object()}}},
                                 {"hints", baked}};
  container["combo"]["hintsRead"] = {
      {"oot", {"__ALTAR_CHILD__", "__ALTAR_ADULT__", "__JUNK__0"}}};
  const auto reads = container["combo"]["hintsRead"];
  // A ready/enabled host cannot override an old or explicitly off slot.
  exportedMessages =
      json{{"__ALTAR_CHILD__", Messages("Loaded door requirement")},
           {"__ALTAR_ADULT__",
            Messages("Loaded bridge/key/soul/win requirements")}}
          .dump();
  Combo_PushHintTrackerData(0);
  assert(pushedHints == baked && pushedRead == reads && exportReads == 0);
  auto &settings = container["combo"]["rando"]["oot"]["settings"];
  settings["gRandoSettings.MapsCompassesGiveInformation"] = 0;
  Combo_PushHintTrackerData(0);
  assert(pushedHints == baked && exportReads == 0);
  settings["gRandoSettings.MapsCompassesGiveInformation"] = 1;
  const auto original = container;
  Combo_PushHintTrackerData(0);
  assert(pushedSlot == 0 && pushedRead == reads && container == original);
  assert(FindHint(pushedHints, "__ALTAR_CHILD__")->at("messages") ==
         Messages("Loaded door requirement"));
  assert(FindHint(pushedHints, "__ALTAR_ADULT__")->at("messages") ==
         Messages("Loaded bridge/key/soul/win requirements"));
  assert(*FindHint(pushedHints, "__JUNK__0") == *FindHint(baked, "__JUNK__0"));
  // No authoritative text yet: omit only the two altar entries, never reveal
  // their stale reward locations. Saved entries and read keys remain intact.
  for (int failure : {0, 1, 2, 3, 4}) {
    SOH_DumpAltarHintMessages = failure == 0 ? nullptr : ExportRequirements;
    exportedMessages = failure == 1 ? "{}" : "invalid JSON";
    exportThrows = failure == 3;
    exportNull = failure == 4;
    Combo_PushHintTrackerData(0);
    assert(!FindHint(pushedHints, "__ALTAR_CHILD__") &&
           !FindHint(pushedHints, "__ALTAR_ADULT__"));
    assert(FindHint(pushedHints, "__JUNK__0") && pushedRead == reads &&
           container == original);
  }
  std::cout
      << "Combo altar composer and loaded tracker preserve requirements, hide "
         "replaced reward locations, and keep off/old slots unchanged\n";
}
