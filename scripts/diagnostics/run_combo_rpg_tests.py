#!/usr/bin/env python3
"""Compile real RPG math, wire merge and MM save conversions without booting a ROM."""
import os
from pathlib import Path
import subprocess
import tempfile
from run_time_pedestal_tests import functions

ROOT = Path(__file__).resolve().parents[2]


def main():
    if not (ROOT / "combo/rando/RpgStatsJson.h").exists():
        raise SystemExit("FAIL: MM has no complete RPG state, saved enable rules or stat sync")
    source = (ROOT / "mm/2s2h/BenJsonConversions.hpp").read_text()
    start = source.index("inline void to_json(json& j, const NeiSaveData&")
    end = source.index("// Spiritual Stones", start)
    with tempfile.TemporaryDirectory(prefix="combo-rpg-") as tmp:
        path = Path(tmp)
        (path / "rpg_nei_json_production.inc").write_text(source[start:end])
        start = source.index("    // Older MM saves always used the native")
        end = source.index("\n}", start)
        migration = "void MigrateMmMagic(const json& j, TestSave& save) {\n" + source[start:end] + "}\n"
        convert = (ROOT / "mm/2s2h/Rando/ConvertItem.cpp").read_text()
        start = convert.index("        case RI_PROGRESSIVE_MAGIC:")
        end = convert.index("        case RI_MAGIC_JAR_SMALL:", start)
        native = "bool NativeMagicObtainable(int item, bool hasObtainedCheck = false) { switch(item) {\n"
        native += convert[start:end] + "default: return false; } }\n"
        start = convert.rindex("            case RI_PROGRESSIVE_MAGIC:")
        end = convert.index("            case RI_PROGRESSIVE_WALLET:", start)
        native += "int NativeMagicNext() { switch(RI_PROGRESSIVE_MAGIC) {\n" + convert[start:end] + "default: return RI_JUNK; } }\n"
        give = (ROOT / "mm/2s2h/Rando/GiveItem.cpp").read_text()
        start = give.index("        case RI_SINGLE_MAGIC:")
        end = give.index("        // Don't love this", start)
        native += "void GiveNativeMagic(int item) { switch(item) {\n" + give[start:end] + "default: break; } }\n"
        (path / "rpg_mm_native_magic_production.inc").write_text(migration + native)
        sync = (ROOT / "mm/2s2h/FleetShipCombo/FleetSync.cpp").read_text()
        start = sync.index("    const uint8_t previousMagicStat")
        end = sync.index("    // Wand rods", start)
        (path / "rpg_mm_sync_production.inc").write_text(
            "void ApplyMmRpg(const json& sh) { NeiSaveData* nei = Nei_Save();\n" + sync[start:end] + "}\n")
        binary = path / "rpg_stats_test"
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-O1", "-g",
                        "-fsanitize=undefined", "-fno-sanitize-recover=undefined",
                        "-Icombo", "-Imm", "-I" + str(ROOT.parent / "deps"), "-I" + tmp,
                        "combo/tests/rpg_stats_test.cpp", "mm/mods/combo_rpg.cpp", "-o", str(binary)], cwd=ROOT, check=True)
        subprocess.run([str(binary)], check=True)
        rando = ROOT / "soh/soh/Enhancements/randomizer"
        helpers = functions((rando / "randostatupgrade.h").read_text())
        bridge = (ROOT / "soh/soh/FleetShipCombo/FleetRpgStats.h").read_text()
        bridge = bridge[bridge.index("namespace FleetRpg {"):]
        give = functions((rando / "randomizer.cpp").read_text().replace('extern "C" ', ''))["Randomizer_Item_Give"]
        native_give = give[give.index("        case RG_MAGIC_SINGLE:"):give.index("        case RG_MAGIC_BEAN_PACK:")]
        give = give[give.index("        case RG_QUARTER_HEART:"):]
        give = give[:give.index("        default:")]
        pools = (rando / "3drando/item_pool.cpp").read_text()
        start = pools.index("    if (ctx->GetOption(RSK_DEFENSE_UPGRADE))")
        end = pools.index("    // MM Masks", start)
        production = helpers["StatUpgradeRequired"] + "\n" + helpers["MagicStatCapacity"] + "\n" + bridge
        production += "\nvoid GiveRpg(PlayState* play, RandomizerGet item) { switch (item) {\n" + give + "default: break; } }\n"
        production += "void GiveNativeOotMagic(PlayState* play, RandomizerGet item) { switch (item) {\n" + native_give + "default: break; } }\n"
        production += "void GenerateRpgPool() { auto ctx = Rando::Context::GetInstance();\n" + pools[start:end] + "}\n"
        (path / "rpg_soh_bridge_production.inc").write_text(production)
        soh_binary = path / "rpg_soh_bridge_test"
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-O1", "-g",
                        "-fsanitize=undefined", "-fno-sanitize-recover=undefined",
                        "-Icombo", "-Isoh", "-I" + str(ROOT.parent / "deps"), "-I" + tmp,
                        "combo/tests/rpg_soh_bridge_test.cpp", "-o", str(soh_binary)], cwd=ROOT, check=True)
        subprocess.run([str(soh_binary)], check=True)


if __name__ == "__main__":
    main()
