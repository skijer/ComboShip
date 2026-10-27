#include <cstdlib>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

static std::vector<std::string> warnings;
static void RecordWarning(const char* message) {
    warnings.emplace_back(message);
}
#define SPDLOG_WARN(...) RecordWarning(__VA_ARGS__)
#include "hint_item_area_production.inc"

static int checks;
static void Require(bool condition, const char* message) {
    ++checks;
    if (!condition) {
        std::cerr << "FAIL " << message << '\n';
        std::exit(1);
    }
}

int main() {
    // This payload crosses the actual combo-seed setter boundary. Multiple
    // independently expected entries catch both unsafe iteration and bad copies.
    std::string payload = R"({
        "Progressive Bow": "Snowhead Temple",
        "Great Fairy's Sword": "Ikana Canyon",
        "Bouteille avec une Fée": "Bourg-Clocher",
        "ignored number": 17,
        "ignored null": null,
        "ignored list": ["Termina Field"],
        "ignored object": {"area": "Woodfall"},
        "ignored boolean": true
    })";
    Combo_SetHintItemAreas(payload);
    const std::unordered_map<std::string, std::string> first{
        { "Progressive Bow", "Snowhead Temple" },
        { "Great Fairy's Sword", "Ikana Canyon" },
        { "Bouteille avec une Fée", "Bourg-Clocher" },
    };
    Require(sComboItemAreas == first, "all string entries must survive parse and non-string values must be ignored");
    Require(warnings.empty(), "valid mixed payload must not warn");
    payload.assign(4096, 'x');
    Require(sComboItemAreas == first, "stored item names and areas must own their copied strings");

    // A later seed replaces stale entries, updates an existing name, and retains
    // the existing empty-area behavior without changing any schema semantics.
    Combo_SetHintItemAreas(R"({"Progressive Bow":"Great Bay Temple","Hookshot":"Pirates' Fortress","Empty Area":""})");
    const std::unordered_map<std::string, std::string> replacement{
        { "Progressive Bow", "Great Bay Temple" },
        { "Hookshot", "Pirates' Fortress" },
        { "Empty Area", "" },
    };
    Require(sComboItemAreas == replacement, "a new seed must replace the prior item-area map");
    Require(!sComboItemAreas.contains("Great Fairy's Sword"), "old seed items must not leak into the next seed");
    Require(warnings.empty(), "valid replacement must not warn");

    // The production clear happens before parse and its catch swallows errors.
    // A malformed new seed therefore clears old areas and emits one warning.
    Combo_SetHintItemAreas(R"({"Hookshot":"Clock Town",)");
    Require(sComboItemAreas.empty(), "invalid replacement must clear stale seed areas");
    Require(warnings.size() == 1, "invalid payload must be caught and warn exactly once");
    Require(warnings[0] == "[ComboShip] Combo_SetHintItemAreas: unparseable payload",
            "warning text must remain unchanged");

    Combo_SetHintItemAreas(R"({"Fire Arrow":"Woodfall Temple","Ice Arrow":"Stone Tower Temple"})");
    const std::unordered_map<std::string, std::string> recovery{
        { "Fire Arrow", "Woodfall Temple" },
        { "Ice Arrow", "Stone Tower Temple" },
    };
    Require(sComboItemAreas == recovery, "valid seed after malformed input must recover all entries");
    Require(warnings.size() == 1, "recovery must not add warnings");
    Combo_SetHintItemAreas("{}");
    Require(sComboItemAreas.empty(), "an empty valid seed must clear old areas");
    Require(warnings.size() == 1, "an empty valid seed must not warn");
    std::cout
        << "PASS " << checks
        << " combo hint-area checks: multiple entries, owned strings, filtering, replacement, catch and recovery\n";
}
