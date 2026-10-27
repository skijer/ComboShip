#!/usr/bin/env python3
"""Exercise Moon's Tear ownership through real native grant/sync/save boundaries.

The affected OoT switch arms, FC materializer, native trade writers, initializers,
serializers and MM computed starting list are copied verbatim from production.
Engine presentation and unrelated item effects remain outside this headless probe.
Requires the normal nlohmann/json.hpp build dependency (or CPATH).
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/nei_shared_slot"))
from run_save_tests import build_native_equipment_dependency, extract, production_for, source_at
from run_time_pedestal_tests import block_from


def between(source, start, end):
    return source[source.index(start):source.index(end, source.index(start))]


def block(source, marker):
    return block_from(source, source.index(marker))


def array(source, marker):
    return between(source, marker, "\n};") + "\n};"


def production(host, grant_ref=None):
    fleet_dir = "mm/2s2h/FleetShipCombo" if host == "mm" else "soh/soh/FleetShipCombo"
    fleet = source_at(f"{fleet_dir}/FleetSync.cpp")
    glue = source_at(f"{fleet_dir}/FleetComboItemsGlue.cpp")
    trade = source_at(f"{host}/mods/items/logic/trade_items.c")
    parts = [array(trade, "static const u8 sTradeAdultItems[")]
    parts += [extract(trade, name) for name in
              ("TradeAdult_IndexOfItem", "TradeAdult_IsOwnedIndex", "TradeAdult_GiveItem", "TradeAdult_FoldCurrent")]
    parts.append(between(glue, "const int sFcNative[]", "// Tokens stringificados"))
    parts.append(block(glue, "std::unordered_map<int, int>& ReverseMap()"))
    parts += [extract(glue, name) for name in ("FcCombo_NativeForItem", "FcCombo_ItemForNative")]
    parts.append("#define SPDLOG_INFO(...) ((void)0)\n#define SPDLOG_WARN(...) ((void)0)\n")
    # Relevant ExtractShared assignments and ApplyShared blocks remain production
    # code. Other shared vitals, UI settings and engine refresh are not exercised.
    assignments = []
    for key in ("tradeAdultOwned", "comboObtainedFc"):
        assignments.append(re.search(r'^    sh\["' + key + r'"\] = [^\n]+', fleet, re.M).group())
    parts.append("static json ExportTrade() { auto* nei = Nei_Save(); json sh;\n" +
                 "\n".join(assignments) + "\nreturn json::parse(sh.dump()); }")
    if host == "mm":
        trade_merge = between(
            fleet, '    if (sh.contains("tradeAdultOwned"))', '    if (sh.contains("ootQuestItems"))')
    else:
        trade_merge = between(fleet, '    if (sh.contains("tradeAdultOwned"))',
                              '    // ootMasksOwned round-trip')
    receipts = between(fleet, '    if (sh.contains("comboObtainedFc") && sh["comboObtainedFc"].is_array())',
                       '    if (sh.contains("comboGoalFlags"))')
    parts.append("static void ImportTrade(const json& sh) { auto* nei = Nei_Save();\n" +
                 trade_merge + receipts + "\n}")
    if host == "soh":
        grant = source_at("soh/soh/Enhancements/randomizer/randomizer.cpp", grant_ref)
        cases = between(grant, "        case RG_MM_SMALL_KEY_WOODFALL:", "        case RG_MM_PICTOGRAPH_BOX:")
        noops = re.findall(r"case (RG_MM_\w+):", cases[:cases.index("case RG_MM_MOONS_TEAR:")])
        parts.append("static constexpr RandomizerGet kNoopItems[] = {" + ",".join(noops) + "};")
        # These IDs reach the main switch: none uses the preceding fast-path
        # randomizerInf mapping. Assert that routing premise against production.
        mapped = between(source_at("soh/soh/Enhancements/randomizer/logic.cpp"),
                         "std::map<RandomizerGet, uint32_t> StaticData::RandoGetToRandInf =", "\n};")
        assert not set(noops) & set(re.findall(r"\bRG_\w+", mapped))
        record = block(grant, "if (!FleetSync_IsApplyingFc())")
        parts.append("static bool sApplyingFc = false;\nstatic bool FleetSync_IsApplyingFc() { return sApplyingFc; }\n"
                     "static void FleetShared_OnNativeObtained(int) {}\n"
                     "static void GrantNative(RandomizerGet item) {\n" + record +
                     "\nswitch (item) {\n" + cases + "\ndefault: break; }\n}\n")
        parts.append("namespace Rando::StaticData {\n"
                     "struct Item { RandomizerGet item; GetItemEntry GetGIEntry_Copy() const {"
                     "GetItemEntry e{}; e.getItemId = item; e.modIndex = MOD_RANDOMIZER; return e; } };\n"
                     "static Item RetrieveItem(RandomizerGet item) { return {item}; }\n}\n"
                     "static PlayState* gPlayState = nullptr;\n"
                     "static u16 Randomizer_Item_Give(PlayState*, GetItemEntry e) {"
                     "GrantNative(static_cast<RandomizerGet>(e.getItemId)); return 0; }\n")
        parts.append(extract(fleet, "ApplyFcRegistryToNatives"))
    else:
        grant = source_at("mm/2s2h/Rando/GiveItem.cpp")
        native = source_at("mm/src/code/z_parameter.c")
        flags = source_at("mm/src/code/z_actor.c")
        inventory = source_at("mm/src/code/z_inventory.c")
        for marker in ("u8 gItemSlots[77]", "u32 gUpgradeMasks[]", "u8 gUpgradeShifts[]",
                       "u16 gUpgradeCapacities[][4]"):
            parts.append(array(inventory, marker))
        parts.append("static void GameInteractor_ExecuteOnFlagSet(int, int) {}\n"
                     "#define FLAG_RANDO_INF 0\n")
        parts += [extract(flags, name) for name in ("Flags_GetRandoInf", "Flags_SetRandoInf")]
        parts.append("#define SOUL_RI_TO_RANDO_INF(randoItemId) "
                     "((randoItemId - RI_SOUL_BOSS_GOHT) + RANDO_INF_OBTAINED_SOUL_OF_BOSS_GOHT)\n"
                     "static PlayState* gPlayState = nullptr;\n"
                     "void Interface_LoadItemIconImpl(PlayState*, u8) {}\n"
                     "void Interface_Dpad_LoadItemIconImpl(PlayState*, u8) {}\n")
        # Execute MM's native trade-slot writer, including equipped-button updates.
        native_trade = block(native, "if ((item >= ITEM_MOONS_TEAR) && (item <= ITEM_MASK_GIANT))")
        parts.append("u8 Item_Give(PlayState* play, u8 item) { u8 i, temp;\n" +
                     native_trade + "\nassert(false); return ITEM_NONE; }\n")
        rows = source_at("mm/2s2h/Rando/StaticData/Items.cpp")
        metadata = []
        for ri in ("MOONS_TEAR", "DEED_LAND", "DEED_SWAMP", "DEED_MOUNTAIN", "DEED_OCEAN",
                   "ROOM_KEY", "LETTER_TO_MAMA", "LETTER_TO_KAFEI", "PENDANT_OF_MEMORIES"):
            row = re.search(r"RI\(RI_" + ri + r",[^\n]+", rows).group()
            item = re.search(r"\bITEM_[A-Z0-9_]+\b", row).group()
            metadata.append("{RI_" + ri + ", {" + item + "}}")
        parts.append("namespace Rando::StaticData { struct Item { u8 itemId; };\n"
                     "static std::map<RandoItemId, Item> Items = {" + ",".join(metadata) + "}; }\n")
        record = between(grant, "    if (sGiveDepth == 1 && gFcCombo_SuppressRecord == 0)",
                         "        FleetShared_OnNativeObtained((int)recordId)") + "\n}\n"
        cases = between(grant, "        case RI_SOUL_BOSS_GOHT:", "        case RI_FROG_BLUE:")
        cases += between(grant, "        case RI_MOONS_TEAR:", "        case RI_POWDER_KEG:")
        parts.append("static int gFcCombo_SuppressRecord = 0;\n"
                     "namespace Rando {\nstatic void GiveItem(RandoItemId randoItemId) { int sGiveDepth = 1;\n" +
                     record + "\nswitch (randoItemId) {\n" + cases + "\ndefault: break; }\n}\n"
                     "static RandoItemId ConvertItem(RandoItemId item) { return item; }\n}\n")
        parts.append("#define FS_TRACE(...) ((void)0)\n"
                     "namespace spdlog { struct Logger { void flush() {} };\n"
                     "static Logger* default_logger() { static Logger logger; return &logger; } }\n")
        parts.append(extract(fleet, "ApplyFcRegistryToNatives"))
        starting = source_at("mm/2s2h/Rando/StartingItems.cpp")
        candidates = between(rows, "std::map<StartingItemCategory, std::vector<RandoItemId>> StartingItemsMap =", "\n};")
        candidate_ids = list(dict.fromkeys(re.findall(r"\bRI_\w+", candidates)))
        parts.append("static constexpr RandoItemId kStartingChoices[] = {" + ",".join(candidate_ids) + "};")
        parts.append("#define RANDO_SAVE_OPTIONS gSaveContext.save.shipSaveInfo.rando.randoSaveOptions\n"
                     "static void Ship_Random_Seed(uint32_t) {}\n"
                     "static int Ship_Random(int lower, int) { return lower; }\n"
                     "namespace Rando {\n")
        parts += [block(starting, "std::vector<RandoItemId> " + name + "(") for name in
                  ("GetStartingItemsFromSave", "GetComputedStartingItems")]
        parts += [extract(starting, name) for name in ("SetStartingItemsInSave", "GrantStartingItems")]
        parts.append("}\n")
    return "\n\n".join(parts)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--grant-ref", help="git revision supplying the OoT grant switch (negative control)")
    parser.add_argument("--sanitizers", action="store_true")
    parser.add_argument("--paired-only", action="store_true", help="skip direct grants to isolate the fresh-pair reproduction")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="moons-tear-") as td:
        build = Path(td)
        binaries = {}
        for host in ("mm", "soh"):
            host_build = build / host
            host_build.mkdir()
            fleet_dir = "mm/2s2h/FleetShipCombo" if host == "mm" else "soh/soh/FleetShipCombo"
            # Reuse the existing native save fixture's helpers and declarations.
            common = source_at("tests/nei_shared_slot/save_test.cpp").split("static void CheckEquipmentLayoutInitialization()")[0]
            common = common.replace('#include "z64animation.h"', '#include "z64animation.h"\n'
                                    '#include "z64interface.h"\n#include "z64player.h"')
            cpp = host_build / "test.cpp"
            cpp.write_text(common + '\n#include <unordered_map>\n#include "' + fleet_dir + '/FleetComboItems.h"\n' +
                           ('#include "soh/Enhancements/randomizer/randomizerEnums.h"\n' if host == "soh" else "") +
                           '#include "moons_production.inc"\n' + source_at("tests/moons_tear/test.cpp"))
            (host_build / "nei_save_production.inc").write_text(production_for(host, None))
            (host_build / "moons_production.inc").write_text(production(host, args.grant_ref))
            (host_build / "BS_thread_pool.hpp").write_text("#pragma once\nnamespace BS { class thread_pool; }\n")
            includes = ["-I" + str(p) for p in (host_build, ROOT, ROOT / host, ROOT / host / "include",
                        ROOT / host / "include/PR", ROOT / host / "2s2h", ROOT / host / "assets",
                        ROOT / "libultraship/include", ROOT / "combo")]
            flags = ["-std=c++20", "-O1", "-g", "-DF3DEX_GBI_2", "-DCOMBO_BUILD",
                     "-DLOG_LEVEL_GAME_PRINTS=0", "-DHOST_MM=" + str(int(host == "mm"))]
            if args.sanitizers:
                flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-no-pie"]
            native_objects = build_native_equipment_dependency(host, host_build, includes, flags)
            binary = host_build / "test"
            subprocess.run([os.environ.get("CXX", "c++"), *flags, *includes, str(cpp), *native_objects, "-o", str(binary)],
                           cwd=ROOT, check=True)
            binaries[host] = binary
        pair = build / "pair.json"
        for host, mode in (("mm", "start"), ("soh", "receive"), ("mm", "receive"),
                           ("soh", "receive"), ("mm", "receive")):
            subprocess.run([str(binaries[host]), mode, str(pair)], cwd=ROOT, check=True,
                           env={**os.environ, "NEI_SAVE_TEST_DIRECTORY": str(build / "Save"),
                                "ASAN_OPTIONS": "detect_leaks=0",
                                **({"NEI_MOON_TEST_PAIR_ONLY": "1"} if args.paired_only else {})})


if __name__ == "__main__":
    main()
