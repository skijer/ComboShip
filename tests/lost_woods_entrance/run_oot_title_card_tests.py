#!/usr/bin/env python3
"""Execute OoT's real cross-game boot, player card selection and card lifecycle.

Disk loading, hooks and cosmetic settings are fixture boundaries. The native
destination texture, language, timing and one-use arrival eligibility execute.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from, functions


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    boot = (ROOT / "soh/src/code/title_setup.c").read_text()
    portal = (ROOT / "soh/soh/OTRGlobals.cpp").read_text(encoding="utf-8-sig")
    player = (ROOT / "soh/src/overlays/actors/ovl_player_actor/z_player.c").read_text()
    actor = (ROOT / "soh/src/code/z_actor.c").read_text()
    handler = (ROOT / "soh/soh/Enhancements/TimeSavers/timesaver_hook_handlers.cpp").read_text()
    start = player.index("void Player_Init(")
    selection = player.index("        titleFileSize = scene->titleFile", start)
    selection = player[player.rfind("\n\n", start, selection):player.index("    if (func_80845C68(", selection)]
    selection = re.sub(r"\bthis\b", "self", selection)
    consume = "Combo_SOH_ConsumeCrossGameTitleCard"
    # The unmodified baseline must fail on the missing card, not a missing API.
    request = ""
    if consume + "(" in boot:
        request = re.search(r"static s32 sComboCrossGameTitleCardPending[^;]*;", boot)[0]
        request += "\n" + re.search(r"static s32 sComboCustomTitleCardPresentation[^;]*;", boot)[0]
        request += "\n" + block_from(boot, boot.index("int Combo_SOH_SwapCustomTitleCardPresentation("))
        request += "\n" + block_from(boot, boot.index("int Combo_SOH_IsPresentingCustomTitleCard("))
        request += "\n" + block_from(boot, boot.index("int " + consume + "("))
        request += "\n" + block_from(boot, boot.index("int Combo_SOH_HasCrossGameTitleCard("))
        if "void Combo_SOH_RequestCrossGameTitleCard(" in boot:
            request += "\n" + block_from(boot, boot.index("void Combo_SOH_RequestCrossGameTitleCard("))
        request = "#ifdef COMBO_BUILD\n" + request + "\n#endif\n"
    texture_headers = "\n".join(re.findall(r'^#include "textures/place_title_cards/[^\n]+', actor, re.M))
    native = functions(actor, ("TitleCard_InitPlaceName", "TitleCard_Update"))
    library = functions((ROOT / "soh/src/code/z_lib.c").read_text(), ("Math_StepToS",))
    table = (ROOT / "soh/src/code/z_scene_table.c").read_text()
    table = table[table.index("#define DEFINE_ENTRANCE("):table.index("#undef DEFINE_ENTRANCE")]
    fixture = (ROOT / "tests/lost_woods_entrance/oot_title_card_test.cpp").read_text()
    fixture = fixture.replace("/* TEXTURE_HEADERS */", texture_headers)
    fixture = fixture.replace("/* REQUEST_STATE */", request)
    fixture = fixture.replace("/* TITLE_CARD_GUARD */", handler[handler.index("        case VB_SHOW_TITLE_CARD:"):
                                                               handler.index("        case VB_WONDER_TALK:")])
    fixture = fixture.replace("/* BOOT_BODY */", block_from(boot, boot.index("void TitleSetup_InitImpl(")))
    fixture = fixture.replace("/* ARRIVAL_BODY */", block_from(portal, portal.index('extern "C" void SOH_ApplyComboPortalArrival(')))
    fixture = fixture.replace("/* NATIVE_TITLE_CARD */", "\n".join([library["Math_StepToS"], *native.values()]))
    fixture = fixture.replace("/* PLAYER_SELECTION */", selection)
    fixture = fixture.replace("/* CUSTOM_TRANSITIONS */", "\n".join(block_from((ROOT / path).read_text(),
        (ROOT / path).read_text().index(signature)) for path, signature in (
            ("soh/soh/Enhancements/Gameplay/ComboBlackMarketNative.cpp", "void Transition("),
            ("soh/soh/Enhancements/Gameplay/ComboSoloForest.cpp", "void ForestTransition("))))
    fixture = fixture.replace("/* ENTRANCE_TABLE */", table + "\n#undef DEFINE_ENTRANCE\n")
    includes = ("soh", "soh/soh", "soh/include", "soh/src", "soh/assets", "libultraship/include", "combo")
    flags = ["-std=c++20", "-fpermissive", "-w", "-fmax-errors=5", "-DF3DEX_GBI_2", "-DFMT_HEADER_ONLY",
             *["-I" + str(ROOT / path) for path in includes]]
    if args.sanitize:
        flags += ["-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie"]
    with tempfile.TemporaryDirectory(prefix="oot-cross-game-title-") as temporary:
        source = Path(temporary) / "test.cpp"
        source.write_text(fixture)
        for combined in (True, False):
            binary = Path(temporary) / ("combo" if combined else "solo")
            subprocess.run([os.environ.get("CXX", "c++"), *flags,
                            *(["-DCOMBO_BUILD"] if combined else []), str(source), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})


if __name__ == "__main__":
    main()
