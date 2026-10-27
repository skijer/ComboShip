// Native data and verbatim production serializers are supplied by run_save_tests.py.
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#if HOST_MM
extern "C" {
#include "z64save.h"
#include "z64item.h"
#include "z64animation.h"
#include "macros.h"
}
#else
#include "z64.h"
#include "macros.h"
#endif
#include "mods/extended_inventory.h"
#include "mods/extended_equipment.h"
#if HOST_MM
#include "mods/nei_oot_compat.h"
#endif
#include "combo/rando/RpgStatsJson.h"
#if HOST_MM
#include "2s2h/FleetShipCombo/FleetComboOptions.h"
#else
#include "soh/FleetShipCombo/FleetComboOptions.h"
// Only the current JSON section needs fixture access. The real SaveData and
// LoadData templates remain unchanged; standard headers are already loaded.
#define private public
#include "soh/SaveManager.h"
#undef private
SaveManager* SaveManager::Instance = nullptr;
SaveManager::SaveManager() {}

// Keep the real trade sidecar I/O. Only its application directory is supplied
// by the fixture; it must never touch a user's Save directory.
static std::filesystem::path sFixtureSaveDirectory;
namespace Ship {
struct Context {
    static std::string GetPathRelativeToAppDirectory(const char*) {
        return sFixtureSaveDirectory.string();
    }
};
}

// Unrelated photo I/O and session wheel tracking stay outside this fixture.
static void Bottle_WheelResetTracking() {}
static void Picto_SyncRead() {}
#endif

using json = nlohmann::json;
extern "C" {
SaveContext gSaveContext{};
ExtendedEquipmentState gExtEquipState{};
ExtEquipBehaviorState gExtEquipBehavior{};
// Actor lifetime, icon generation, and CVar/UI activation are outside the
// equipment ownership/persistence boundary exercised below.
void ByrnaOrb_Forget(void) {}
void ExtEquip_OnPlayerSceneInit(void) {}
void ExtEquip_GenerateIcons(void) {}
void CVarSetInteger(const char*, int32_t) {}
// Observe no engine audit service in this synchronous storage fixture. Real
// slot writers and serializer scopes retain their production audit calls.
#ifdef COMBO_BUILD
void ItemGrantAudit_Begin(const char*, int, int, int) {}
void ItemGrantAudit_End(void) {}
#endif
}

static u8 sExtEquipInitInProgress = 0;
static u8 sTransformBackup[4] = {};
static u8 sTransformBackupValid = 0;
#include "nei_save_production.inc"

static_assert(sizeof(NeiSaveData::ownedItems[0]) == sizeof(uint16_t));
#if HOST_MM
static_assert(offsetof(NeiSaveData, hyliasGraceOwned) >=
              offsetof(NeiSaveData, comboRpg) + sizeof(NeiSaveData::comboRpg));
#else
static_assert(offsetof(NeiSaveData, hyliasGraceOwned) > offsetof(NeiSaveData, seasonsOwned));
#endif
static_assert(offsetof(NeiSaveData, phantomHourglassOwned) == offsetof(NeiSaveData, hyliasGraceOwned) + 1);

static void Check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

static void Reset() {
    gSaveContext = {};
#if HOST_MM
    Nei_InitNewSave();
#else
    NeiSave_Init(false);
#endif
}

static json SaveBlob() {
    json section = json::object();
#if HOST_MM
    to_json(section, *Nei_Save());
#else
    auto* manager = SaveManager::Instance;
    manager->currentJsonContext = &section;
    NeiSave_Save(gSaveContext, 0, true);
    Check(manager->currentJsonContext == &section, "SaveManager restored the NEI section after array traversal");
    manager->currentJsonContext = nullptr;
#endif
    // Force real textual serialization/parsing rather than reuse a JSON node.
    return json::parse(section.dump());
}

static void LoadBlob(json section) {
    std::memset(Nei_Save(), 0xA5, sizeof(*Nei_Save()));
#if HOST_MM
    from_json(section, *Nei_Save());
#else
    auto* manager = SaveManager::Instance;
    manager->currentJsonContext = &section;
    NeiSave_Load();
    Check(manager->currentJsonContext == &section, "SaveManager restored the NEI section after loading arrays");
    manager->currentJsonContext = nullptr;
#endif
}

static void CheckEquipmentLayoutInitialization() {
    constexpr uint32_t pendant = 0x00080000;
    constexpr uint32_t moon = 0x00000800;
    constexpr uint32_t modernOwned = 0x0F400000; // all boots + Champion and Sage tunics
    for (uint8_t equippedBoots : {uint8_t(0), uint8_t(2), uint8_t(3)}) {
        Reset();
        for (uint8_t index = 1; index <= 3; ++index)
            ExtEquip_GiveItem(EQUIP_TYPE_BOOTS, index);
        ExtEquip_GiveItem(EQUIP_TYPE_TUNIC, 1);
        ExtEquip_GiveItem(EQUIP_TYPE_TUNIC, 3);
        Nei_Save()->extEquipBoots = equippedBoots;
        Nei_Save()->extEquipTunic = 3;
        Nei_Save()->tradeAdultOwned = moon;
        ExtEquip_Init();
        Check(Nei_Save()->tradeAdultOwned == moon,
              "modern starting Climb Boots must not become a Pendant on first equipment initialization");
        Check(Nei_Save()->extEquipOwnedBits == modernOwned,
              "modern starting boots and tunics must retain all granted ownership");
        Check(Nei_Save()->capeOwned == 0,
              "a modern Champion tunic must not become the legacy Magic Cape");
        Check(Nei_Save()->extEquipBoots == equippedBoots && Nei_Save()->extEquipTunic == 3,
              "current equipment selections must survive first initialization");
        Nei_Save()->tradeAdultOwned |= pendant; // independently earned, preserved across reload
        Nei_Save()->pendantOwned = 1;
        LoadBlob(SaveBlob());
        ExtEquip_Init();
        ExtEquip_Init();
        Check(Nei_Save()->tradeAdultOwned == (moon | pendant) && Nei_Save()->pendantOwned == 1 &&
                  Nei_Save()->extEquipOwnedBits == modernOwned &&
                  Nei_Save()->extEquipBoots == equippedBoots && Nei_Save()->extEquipTunic == 3,
              "current layouts and legitimately earned Pendant must survive reload and repeated initialization");
    }
    // Missing version keys still identify a legacy save, even though the new
    // initializer now selects the current layouts. Owned OR equipped implies owned.
    for (bool owned : {false, true}) {
        Reset();
        LoadBlob({{"extEquipOwnedBits", owned ? 0x0D000000 : 0x08000000},
                  {"extEquipBoots", owned ? 0 : 2}, {"tradeAdultOwned", moon}});
        ExtEquip_Init();
        Check(Nei_Save()->tradeAdultOwned == (moon | pendant),
              "a genuine legacy owned or equipped Pendant slot must migrate to trade ownership");
        Check((Nei_Save()->extEquipOwnedBits & 0x0C000000) == 0 && Nei_Save()->extEquipBoots == 0,
              "legacy Pendant/Dragon Scale slots must be retired during migration");
        const auto migrated = SaveBlob();
        LoadBlob(migrated);
        ExtEquip_Init();
        Check(SaveBlob() == migrated, "legacy equipment migration must persist and run only once");
    }
    Reset();
    LoadBlob({{"extEquipOwnedBits", 0x01400000}, {"extEquipTunic", 3}});
    ExtEquip_Init();
    Check(Nei_Save()->capeOwned == 1 && Nei_Save()->extEquipOwnedBits == 0x00400000 &&
              Nei_Save()->extEquipTunic == 1,
          "legacy Cape/Champion tunics must retain their original migration");
    std::puts("PASS equipment layouts: modern grants, selections, reload, earned Pendant and genuine legacy migration");
}

static void CheckSuppliedRepairedSave() {
    const char* path = std::getenv("NEI_SAVE_TEST_COMBO_SAVE");
    if (!path)
        return;
    std::ifstream file(path);
    Check(bool(file), "the supplied repaired combined save must be readable");
    json combined;
    file >> combined;
#if HOST_MM
    const auto section = combined.at(json::json_pointer("/mm/newCycleSave/save/shipSaveInfo/nei"));
#else
    const auto section = combined.at(json::json_pointer("/oot/sections/nei/data"));
#endif
    LoadBlob(section);
    const auto ownedBefore = Nei_Save()->extEquipOwnedBits;
    for (int round = 0; round < 2; ++round) {
        ExtEquip_Init();
        Check((Nei_Save()->tradeAdultOwned & 0x00080800) == 0 && Nei_Save()->pendantOwned == 0,
              "the supplied repair must keep Moon's Tear and Pendant absent after equipment initialization");
        Check(Nei_Save()->comboObtainedFc[210] == 0 && Nei_Save()->comboAppliedFc[210] == 0 &&
                  Nei_Save()->comboObtainedFc[40] == 0 && Nei_Save()->comboAppliedFc[40] == 0,
              "the supplied repair must not retain a pending Moon or Pendant grant");
        Check(Nei_Save()->extEquipOwnedBits == ownedBefore,
              "the supplied repair must retain its legitimate modern equipment");
        LoadBlob(SaveBlob());
    }
    std::puts("PASS supplied repaired save: native NEI load/init/save/reload retains equipment without phantom items");
}

#if HOST_MM
static void CheckNativeShieldOwnershipRemoval() {
    for (uint8_t importedSkin : {uint8_t(0), uint8_t(1), uint8_t(2)}) {
        Reset();
        gExtEquipState.currentExtShield = 0;
        Nei_Save()->extEquipShield = 0;
        Nei_Save()->vanillaShieldSkin = importedSkin;
        Nei_Save()->shieldOwned = FC_SHIELD_DEKU | FC_SHIELD_MIRROR_OOT;
        // Preserve sword and unused nibbles while releasing only native MM Mirror.
        gSaveContext.save.saveInfo.equips.equipment = 0xAB21;
        ExtEquip_GiveItem(EQUIP_TYPE_SHIELD, 3);
        Check(ExtEquip_HasItem(EQUIP_TYPE_SHIELD, 3) &&
                  Nei_Save()->shieldOwned == (FC_SHIELD_DEKU | FC_SHIELD_MIRROR_OOT | FC_SHIELD_IKANA),
              "canonical Ikana grant must retain unrelated shared shield ownership");
        ExtEquip_RemoveItem(EQUIP_TYPE_SHIELD, 3);
        Check(!ExtEquip_HasItem(EQUIP_TYPE_SHIELD, 3) &&
                  Nei_Save()->shieldOwned == (FC_SHIELD_DEKU | FC_SHIELD_MIRROR_OOT),
              "canonical Ikana removal must clear only its extended and shared ownership");
        Check(gSaveContext.save.saveInfo.equips.equipment == (importedSkin ? 0xAB21 : 0xAB01) &&
                  Nei_Save()->vanillaShieldSkin == importedSkin,
              "native equipment writer must release MM Mirror without erasing imported skins or other nibbles");
    }
    std::puts("PASS native MM shield dependency: real nibble removal and imported skin preservation");
}
#endif

#if !HOST_MM
static void CheckTradeSidecarOwnership() {
    // These are independent trade-wheel entries: Moon's Tear is bit 11,
    // Zelda's Letter is bit 22. A reused file number is not a save identity.
    constexpr uint32_t moon = 0x00000800;
    constexpr uint32_t letter = 0x00400000;
    gSaveContext.fileNum = 0;
    const auto sidecar = sFixtureSaveDirectory / "file1_tradeitems.bin";
    {
        std::ofstream file(sidecar, std::ios::binary);
        file.write(reinterpret_cast<const char*>(&moon), sizeof(moon));
        Check(bool(file), "the fixture created an old slot's Moon's Tear sidecar");
    }
    LoadBlob({{"tradeAdultOwned", letter}});
#ifdef COMBO_BUILD
    Check(Nei_Save()->tradeAdultOwned == letter,
          "a combined save with Zelda's Letter must not acquire Moon's Tear from an old sidecar");
    LoadBlob(json::object());
    Check(Nei_Save()->tradeAdultOwned == 0,
          "a fresh combined save must stay empty despite a reused slot's sidecar");
    LoadBlob({{"tradeAdultOwned", moon | letter}});
    Check(Nei_Save()->tradeAdultOwned == (moon | letter),
          "legitimately saved Moon's Tear and Zelda's Letter must both survive loading");
    LoadBlob({{"tradeAdultOwned", letter}});
    LoadBlob(SaveBlob());
    Check(Nei_Save()->tradeAdultOwned == letter,
          "combined save/load must preserve canonical ownership without reimporting the sidecar");
    {
        uint32_t legacy = 0;
        std::ifstream file(sidecar, std::ios::binary);
        file.read(reinterpret_cast<char*>(&legacy), sizeof(legacy));
        Check(bool(file) && legacy == moon,
              "saving a combined file must leave legacy standalone sidecar bytes untouched");
    }
    std::filesystem::remove(sidecar);
    SaveBlob();
    Check(!std::filesystem::exists(sidecar),
          "a combined save must not create a legacy trade sidecar");
    std::puts("PASS trade ownership: combined JSON is authoritative across empty, letter, Moon and reload cases");
#else
    Check(Nei_Save()->tradeAdultOwned == (moon | letter),
          "standalone SoH must retain its legacy cross-game sidecar import");
    Nei_Save()->tradeAdultOwned = letter;
    SaveBlob();
    {
        uint32_t legacy = 0;
        std::ifstream file(sidecar, std::ios::binary);
        file.read(reinterpret_cast<char*>(&legacy), sizeof(legacy));
        Check(bool(file) && legacy == letter,
              "standalone SoH must retain its legacy cross-game sidecar export");
    }
    std::filesystem::remove(sidecar);
    std::puts("PASS trade ownership: standalone legacy sidecar import/export retained");
#endif
}
#endif

static uint16_t Selected() {
    return ExtInv_GetSlotItem(SLOT_PHANTOM_HOURGLASS);
}

static void CheckFlags(uint8_t grace, uint8_t hourglass, const char* message) {
    Check(Nei_Save()->hyliasGraceOwned == grace && Nei_Save()->phantomHourglassOwned == hourglass, message);
}

static void CheckBothOwnedRoundtrips() {
    for (uint16_t first : {uint16_t(ITEM_HYLIAS_GRACE), uint16_t(EXT_ITEM_PHANTOM_HOURGLASS)}) {
        Reset();
        GraceHourglass_Grant(first);
        GraceHourglass_Grant(first == ITEM_HYLIAS_GRACE ? EXT_ITEM_PHANTOM_HOURGLASS : ITEM_HYLIAS_GRACE);
        CheckFlags(1, 1, "both items are independently owned before save");
        Check(Selected() == first, "granting a sibling retained the selected shared item");
        ExtInv_SetSlotItem(SLOT_PHANTOM_HOURGLASS - 1, 0x0234);
        ExtInv_SetSlotItem(SLOT_PHANTOM_HOURGLASS + 1, 0xF123);
        LoadBlob(SaveBlob());
        CheckFlags(1, 1, "both appended ownership flags survived the JSON roundtrip");
        Check(Selected() == first, "the selected full u16 Grace/hourglass cell survived the JSON roundtrip");
        Check(ExtInv_GetSlotItem(SLOT_PHANTOM_HOURGLASS - 1) == 0x0234 &&
                  ExtInv_GetSlotItem(SLOT_PHANTOM_HOURGLASS + 1) == 0xF123,
              "adjacent custom cells retained their full u16 values");
        GraceHourglass_Heal();
        Check(Selected() == first, "healing retained the saved selection when both flags are owned");
        CheckFlags(1, 1, "healing did not drop a persisted sibling");
    }
    std::puts("PASS native NEI save: both acquisition orders, both flags and selected/adjacent u16 cells");
}

static void CheckLegacySelectedItems() {
    for (uint16_t selected : {uint16_t(ITEM_HYLIAS_GRACE), uint16_t(EXT_ITEM_PHANTOM_HOURGLASS)}) {
        std::array<uint16_t, 48> owned;
        owned.fill(ITEM_NONE);
        owned[SLOT_PHANTOM_HOURGLASS - 24] = selected;
        const json legacy = {{"ownedItems", owned}};
        LoadBlob(json::parse(legacy.dump()));
        CheckFlags(0, 0, "old blobs absent the new keys default both ownership flags to zero");
        Check(Selected() == selected, "legacy load retained its selected full u16 item");
        GraceHourglass_Heal();
        CheckFlags(selected == ITEM_HYLIAS_GRACE, selected == EXT_ITEM_PHANTOM_HOURGLASS,
                   "healing inferred only the selected legacy item's ownership");
        Check(Selected() == selected, "legacy healing retained its original selection");
        LoadBlob(SaveBlob());
        CheckFlags(selected == ITEM_HYLIAS_GRACE, selected == EXT_ITEM_PHANTOM_HOURGLASS,
                   "the recovered legacy flag persisted on the next save");
        Check(Selected() == selected, "the recovered legacy selection persisted on the next save");
    }
    std::puts("PASS native NEI load: absent ownership keys reset stale flags, then heal and resave legacy selection");
}

static void CheckFlagsOnlyRepair() {
    for (uint8_t grace : {uint8_t(0), uint8_t(1)}) {
        for (uint8_t hourglass : {uint8_t(0), uint8_t(1)}) {
            LoadBlob({{"hyliasGraceOwned", grace}, {"phantomHourglassOwned", hourglass}});
            CheckFlags(grace, hourglass, "flags-only JSON retained each ownership combination");
            for (uint16_t item : Nei_Save()->ownedItems) {
                Check(item == ITEM_NONE, "absent ownedItems default to u16 ITEM_NONE, not zero or 0xFFFF");
            }
            GraceHourglass_Heal();
            const uint16_t expected = grace ? ITEM_HYLIAS_GRACE
                                           : hourglass ? EXT_ITEM_PHANTOM_HOURGLASS : ITEM_NONE;
            Check(Selected() == expected, "flags-only repair selected the owned item with Grace first when both owned");
            GraceHourglass_Heal();
            Check(Selected() == expected, "flags-only repair is idempotent");
            CheckFlags(grace, hourglass, "flags-only repair preserved both ownership facts");
            LoadBlob(SaveBlob());
            Check(Selected() == expected, "flags-only repaired selection persisted");
            CheckFlags(grace, hourglass, "flags-only repaired ownership persisted");
        }
    }
    LoadBlob(json::object());
    CheckFlags(0, 0, "an empty old NEI blob defaulted both flags to zero");
    Check(Nei_Save()->slateMode == 0 && Nei_Save()->slateRunesOwned == 0,
          "an empty old NEI blob cleared stale Slate mode and ownership");
    std::puts("PASS native NEI repair: four flags-only combinations, empty old blob, idempotence and resave");
}

static void CheckSlateRoundtrip(uint8_t mask, uint8_t mode) {
    Reset();
    ExtInv_SetSlotItem(SLOT_SHEIKAH_SLATE, EXT_ITEM_SHEIKAH_SLATE);
    Nei_Save()->slateRunesOwned = mask;
    Slate_SetRune(mode);
    Check(Nei_Save()->slateMode == mode, "an owned Slate rune was selectable before save");
    LoadBlob(SaveBlob());
    Check(ExtInv_GetSlotItem(SLOT_SHEIKAH_SLATE) == EXT_ITEM_SHEIKAH_SLATE,
          "the full u16 Slate cell survived the JSON roundtrip");
    Check(Nei_Save()->slateRunesOwned == mask && Nei_Save()->slateMode == mode,
          "Slate owned runes and selected mode survived the JSON roundtrip");
    Check(Slate_GetRune() == mode, "the loaded owned Slate mode remained selected");
    Check(Nei_Save()->slateRunesOwned == mask, "resolving the selected mode preserved its ownership mask");
}

static void CheckSlateModes() {
    const uint8_t all = (1u << SLATE_RUNE_COUNT) - 1u;
    for (uint8_t mode = 0; mode < SLATE_RUNE_COUNT; ++mode) {
        CheckSlateRoundtrip(1u << mode, mode);
        CheckSlateRoundtrip(all, mode);
        if (mode % 2 == 0) {
            CheckSlateRoundtrip(0x15, mode);
        }
    }
    Reset();
    ExtInv_SetSlotItem(SLOT_SHEIKAH_SLATE, EXT_ITEM_SHEIKAH_SLATE);
    Nei_Save()->slateRunesOwned = 1u << SLATE_RUNE_CRYONIS;
    Nei_Save()->slateMode = 255;
    LoadBlob(SaveBlob());
    Check(Nei_Save()->slateMode == 255, "the serializer retained the raw invalid legacy mode for runtime repair");
    Check(Slate_GetRune() == SLATE_RUNE_CRYONIS, "the real Slate resolver repaired to the only owned rune");
    LoadBlob(SaveBlob());
    Check(Nei_Save()->slateMode == SLATE_RUNE_CRYONIS &&
              Nei_Save()->slateRunesOwned == (1u << SLATE_RUNE_CRYONIS),
          "the repaired Slate mode and ownership survived the next save");
    std::puts("PASS native Slate save: all five modes with single/all/sparse ownership, u16 cell and invalid-mode repair");
}

static void CheckFleetOwnershipMerge() {
    Reset();
    Fixture_FCO_APPLY(json{{"hyliasGraceOwned", 1}, {"phantomHourglassOwned", 1}});
    CheckFlags(1, 1, "the real Fleet option table imported both appended ownership flags");
    Fixture_FCO_APPLY(json{{"hyliasGraceOwned", 0}, {"phantomHourglassOwned", 0}});
    CheckFlags(1, 1, "MAX merge rejected incoming ownership downgrades");
    Fixture_FCO_APPLY(json::object());
    CheckFlags(1, 1, "an old shared blob without the flags preserved local ownership");
    json exported = json::object();
    Fixture_FCO_EXTRACT(exported);
    Reset();
    Fixture_FCO_EXTRACT(exported);
    Fixture_FCO_APPLY(json::parse(exported.dump()));
    CheckFlags(1, 1, "MAX export retained previous shared ownership when this host had zero flags");
    GraceHourglass_Heal();
    Check(Selected() == ITEM_HYLIAS_GRACE, "imported flags repaired the shared cell through the real helper");
    LoadBlob(SaveBlob());
    CheckFlags(1, 1, "Fleet-imported ownership persisted through the native serializer");
    std::puts("PASS native Fleet option table: both flags import/export with MAX, missing-key retention and persistence");
}

static void CheckSeasonsRoundtrip() {
    Reset();
    auto* nei = Nei_Save();
    nei->seasonsOwned = 8;
    nei->season = SEASON_WINTER;
    nei->seasonsRodOwned = 1;
    nei->seasonsGates = 5;
    nei->ownedItems[23] = EXT_ITEM_ROD_OF_SEASONS;
    auto saved = SaveBlob();
    Reset();
    LoadBlob(saved);
    Check(nei->seasonsRodOwned == 1 && nei->seasonsGates == 5, "Rod ownership and completion gates survive native save");
    Check(nei->seasonsOwned == 8 && nei->season == SEASON_WINTER && nei->ownedItems[23] == EXT_ITEM_ROD_OF_SEASONS,
          "original season pickup, active selector and full u16 Rod remain intact");
    saved.erase("seasonsRodOwned");
    saved.erase("seasonsGates");
    LoadBlob(saved);
    Check(nei->seasonsRodOwned == 0 && nei->seasonsGates == 0, "older save lacks appended Rod/gate keys");
    Check(nei->seasonsOwned == 8 && nei->ownedItems[23] == EXT_ITEM_ROD_OF_SEASONS, "older save retains original Rod ownership");
    std::puts("PASS native seasons save: Rod/gates/selector roundtrip and old ownership retained");
}

int main() {
#if !HOST_MM
    SaveManager manager;
    SaveManager::Instance = &manager;
    sFixtureSaveDirectory = std::getenv("NEI_SAVE_TEST_DIRECTORY");
    std::filesystem::create_directories(sFixtureSaveDirectory);
    CheckTradeSidecarOwnership();
#endif
    CheckEquipmentLayoutInitialization();
#if HOST_MM
    CheckNativeShieldOwnershipRemoval();
#endif
    CheckBothOwnedRoundtrips();
    CheckSeasonsRoundtrip();
    CheckLegacySelectedItems();
    CheckFlagsOnlyRepair();
    CheckSlateModes();
    CheckFleetOwnershipMerge();
    CheckSuppliedRepairedSave();
    std::puts(HOST_MM ? "PASS host: MM native save" : "PASS host: OoT native save");
    return 0;
}
