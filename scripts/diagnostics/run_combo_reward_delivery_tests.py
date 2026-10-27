#!/usr/bin/env python3
"""Exercise the actual cross-game magic relay and OoT bottle consume boundary."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
from run_mm_scene_randomization_tests import function

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8-sig")


def statement(source, start):
    brace = source.index("{", start)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def main():
    oot = read("soh/soh/OTRGlobals.cpp")
    mm = read("mm/2s2h/BenPort.cpp")
    relays = [read(f"{prefix}/FleetShipCombo/FleetSharedItems.cpp") for prefix in ("soh/soh", "mm/2s2h")]
    fixture = read("tests/reward_delivery/magic_sharing_test.cpp")
    rg = sorted(set(re.findall(r"\bRG_[A-Z_0-9]+", relays[0])))
    ri = sorted(set(re.findall(r"\bRI_[A-Z_0-9]+", relays[1])))
    fixture = fixture.replace("/* NATIVE_IDS */", "enum RandomizerGet {" + ",".join(rg) + "};\n" +
                              "enum RandoItemId {" + ",".join(ri) + "};")
    fixture = fixture.replace("/* FLOOR_EXPORTS */", function(oot, "SOH_ApplySharedMagicFloor") + "\n" +
                              function(mm, "MM_ApplySharedMagicFloor") + "\n" +
                              function(read("mm/mods/combo_rpg.cpp"), "ComboRpg_NativeMagicTier"))
    getters = ""
    for host, source in (("Oot", oot), ("Mm", mm)):
        source = source[source.index("int SOH_GetSharedTier(" if host == "Oot" else "int MM_GetSharedTier("):]
        start = source.index("case ComboRando::SF_MAGIC:") + len("case ComboRando::SF_MAGIC:")
        getters += f"int {host}NativeTier() {{\n" + source[start:source.index("case ComboRando::SF_WALLET:", start)] + "}\n"
    fixture = fixture.replace("/* NATIVE_TIER_GETTERS */", getters)
    for host, source in zip(("OOT", "MM"), relays):
        body = "typedef void (*FnGrantSharedItem)(const char*);\n"
        if host == "MM":
            start = source.index("static const struct {")
            body += source[start:source.index("typedef void (*FnGrantSharedItem)", start)]
        body += "\n".join(function(source, name) for name in
                           ("ShareNativeMagicFloor", "ResolvePeerGrant", "FleetShared_OnNativeObtained"))
        fixture = fixture.replace(f"/* {host}_RELAY */", body.replace('extern "C" ', ""))
    bottle = read("tests/reward_delivery/bottle_use_test.cpp")
    bottle = bottle.replace("/* CONSUME_BOTTLE */", function(read("soh/src/code/z_parameter.c"),
                                                           "Inventory_UpdateBottleItem"))
    player = read("soh/mods/extended_player.c")
    assert re.search(r"case ITEM_CHATEAU_ROMANI:\s*return PLAYER_IA_BOTTLE_POTION_BLUE;", player)
    notification = read("tests/reward_delivery/notification_test.cpp")
    give = read("mm/2s2h/Rando/GiveItem.cpp")
    start = give.rfind("        if", 0, give.index("!Rando::gComboDormantGive"))
    notification = notification.replace("/* NATIVE_TOAST */", statement(give, start))
    for host, path, name in (("OOT", "soh/soh/Enhancements/randomizer/hook_handlers.cpp", "OOT_DeliverForeign"),
                            ("MM", "mm/2s2h/Rando/MiscBehavior/CheckQueue.cpp", "Rando::MiscBehavior::SendForeignCheck")):
        delivery = function(read(path), name)
        notification = notification.replace(f"/* {host}_FOREIGN_TOAST */",
                                            statement(delivery, delivery.index("Notification::Emit")) + ");")
    flags = ["-std=c++20", "-Wall", "-Wextra", "-I" + str(ROOT), "-I" + str(ROOT / "soh")]
    if "--sanitize" in sys.argv:
        flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-g"]
    with tempfile.TemporaryDirectory(prefix="combo-reward-delivery-") as tmp:
        for name, code, extra in (("magic", fixture, []), ("bottle", bottle,
                                  [str(ROOT / "soh/mods/items/mm_bottles_behavior.cpp")]),
                                 ("notification", notification, ["-DCOMBO_BUILD"]),
                                 ("notification-standalone", notification, [])):
            source = Path(tmp) / (name + ".cpp")
            source.write_text(code)
            binary = Path(tmp) / name
            subprocess.run([os.environ.get("CXX", "c++"), *flags, str(source), *extra, "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
