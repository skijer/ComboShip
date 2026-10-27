#include "combo/ComboSettingsImportConfig.h"

#include <iostream>
#include <string>

using nlohmann::json;

static int failures = 0;

static void Check(bool condition, const char *message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    ++failures;
  }
}

static json ConfigWithMods(const std::string &order) {
  return {{"CVars", {{"gSettings", {{"EnabledMods", order}}}}}};
}

int main() {
  const json noConfig;
  const std::string ootOrder = "Zelda Patch|Djipi 4K|A Base";
  const std::string mmOrder = "Moon Patch|Henriko HD|B Base";
  const auto soh = ConfigWithMods(ootOrder);
  const auto mm = ConfigWithMods(mmOrder);
  const json::json_pointer ootKey("/CVars/gSettings/EnabledMods");
  const json::json_pointer mmKey("/CVars/gSettings/EnabledModsMM");

  auto merged = ComboSettingsImport::Merge(soh, mm);
  Check(merged.value(ootKey, std::string()) == ootOrder,
        "SoH order survives a two-game import");
  Check(merged.value(mmKey, std::string()) == mmOrder,
        "2Ship order goes to its separate ComboShip key");
  Check(soh == ConfigWithMods(ootOrder) && mm == ConfigWithMods(mmOrder),
        "source configs stay unchanged");

  merged = ComboSettingsImport::Merge(noConfig, mm);
  Check(!merged.contains(ootKey),
        "MM-only import never seeds the OoT mod list");
  Check(merged.value(mmKey, std::string()) == mmOrder,
        "MM-only import preserves exact ordering");

  merged = ComboSettingsImport::Merge(soh, noConfig);
  Check(merged.value(ootKey, std::string()) == ootOrder,
        "SoH-only import preserves exact ordering");
  Check(!merged.contains(mmKey), "SoH-only import does not invent an MM list");

  auto explicitMm = ConfigWithMods("old MM order");
  explicitMm[mmKey] = "Explicit MM|Final MM";
  auto comboSoh = soh;
  comboSoh[mmKey] = "Stale MM in SoH file";
  merged = ComboSettingsImport::Merge(comboSoh, explicitMm);
  Check(merged.value(mmKey, std::string()) == "Explicit MM|Final MM",
        "an explicit MM list in the MM input wins over legacy and SoH copies");
  merged = ComboSettingsImport::Merge(comboSoh, noConfig);
  Check(merged.value(mmKey, std::string()) == "Stale MM in SoH file",
        "an already-separated MM list survives when only that config is "
        "imported");

  merged = ComboSettingsImport::Merge(soh, ConfigWithMods(""));
  Check(merged.contains(mmKey) && merged.at(mmKey) == "",
        "an explicitly empty MM order is preserved");
  Check(merged.value(ootKey, std::string()) == ootOrder,
        "empty MM order does not clear OoT order");
  merged = ComboSettingsImport::Merge(ConfigWithMods(""), mm);
  Check(merged.contains(ootKey) && merged.at(ootKey) == "",
        "an explicitly empty OoT order is preserved");

  const auto withoutMods =
      json::parse(R"({"CVars":{"gSettings":{"MenuBar":1}}})");
  merged = ComboSettingsImport::Merge(withoutMods, mm);
  Check(!merged.contains(ootKey),
        "an OoT config without a list cannot inherit the MM list");
  Check(merged.value(mmKey, std::string()) == mmOrder,
        "MM list survives when SoH has no list");
  merged = ComboSettingsImport::Merge(withoutMods, withoutMods);
  Check(!merged.contains(ootKey) && !merged.contains(mmKey),
        "absent mod data creates no order fields");

  auto malformedMm = ConfigWithMods("unused");
  malformedMm[ootKey] = json::array({"not a native order string"});
  merged = ComboSettingsImport::Merge(soh, malformedMm);
  Check(!merged.contains(mmKey),
        "malformed mod data does not create an MM order");
  Check(merged.value(ootKey, std::string()) == ootOrder,
        "malformed MM data cannot affect OoT order");

  const auto settingsSoh =
      json::parse(R"({"ConfigVersion":9,"Window":{"Width":1920},
        "CVars":{"gSettings":{"MenuBar":1},"gRando":{"Logic":2},"Shared":{"Value":1}},
        "Controllers":{"Port1":{"Name":"SoH pad"}}})");
  const auto settingsMm =
      json::parse(R"({"ConfigVersion":99,"Window":{"Width":1280},
        "CVars":{"gSettings":{"MenuBar":0},"gRandoMM":{"Logic":3},"Shared":{"Value":0,"MmOnly":4}},
        "Controllers":{"Port1":{"Name":"MM pad"},"Port2":{"Name":"Second pad"}}})");
  merged = ComboSettingsImport::Merge(settingsSoh, settingsMm);
  Check(merged["CVars"]["Shared"] == json({{"Value", 1}, {"MmOnly", 4}}),
        "shared settings still merge per leaf with SoH winning");
  Check(merged["CVars"]["gRando"]["Logic"] == 2 &&
            merged["CVars"]["gRandoMM"]["Logic"] == 3,
        "both games' randomizer settings survive");
  Check(merged["Controllers"]["Port1"]["Name"] == "SoH pad" &&
            merged["Controllers"]["Port2"]["Name"] == "Second pad",
        "controller blocks retain the existing merge policy");
  Check(!merged.contains("Window"),
        "machine-specific window settings remain excluded");
  Check(merged["ConfigVersion"] == 9,
        "only the SoH config version drives SoH migrations");
  Check(!ComboSettingsImport::Merge(noConfig, settingsMm)
             .contains("ConfigVersion"),
        "a standalone MM version is not treated as a SoH version");
  Check(!ComboSettingsImport::Merge(withoutMods, settingsMm)
             .contains("ConfigVersion"),
        "missing SoH version does not inherit the MM version");
  Check(ComboSettingsImport::Merge(noConfig, noConfig) == json::object(),
        "no configs yields an empty object");

  if (failures) {
    std::cerr << failures << " settings-import checks failed\n";
    return 1;
  }
  std::cout << "PASS: per-game mod order, missing/empty data, source "
               "preservation, and existing settings merge\n";
}
