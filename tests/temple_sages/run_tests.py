#!/usr/bin/env python3
"""Focused Temple Sages native policy checks; these do not prove in-game behavior."""
from pathlib import Path
import argparse
import os
import subprocess
import tempfile
import sys
import re

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--json-include", type=Path)
    parser.add_argument("--spdlog-include", type=Path)
    parser.add_argument("--adapter-source", type=Path, help="Optional mutation/source candidate for regression checks")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="temple-sages-") as temporary:
        binary = Path(temporary) / "policy"
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie",
                        "-I" + str(ROOT / "combo"), str(ROOT / "tests/temple_sages/policy_test.cpp"),
                        "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
        fixture = (ROOT / "tests/temple_sages/native_adapter_test.cpp").read_text()
        interactor = (ROOT / "soh/soh/Enhancements/game-interactor/GameInteractor.cpp").read_text()
        player = (ROOT / "soh/src/code/z_player_lib.c").read_text()
        bodies = block_from(interactor, interactor.index("bool GameInteractor::IsPlayerInControl("))
        bodies += "\n" + re.sub(r"\bthis\b", "self", block_from(player, player.index("s32 Player_InBlockingCsMode(")))
        player_actor = (ROOT / "soh/src/overlays/actors/ovl_player_actor/z_player.c").read_text()
        start = player_actor.index("            if (respawnFlag < 0)", player_actor.index("respawnFlag = gSaveContext.respawnFlag;"))
        end = player_actor.index("\n            play->actorCtx.flags.tempSwch", start)
        respawn = re.sub(r"\bthis\b", "self", player_actor[start:end])
        room = (ROOT / "soh/src/code/z_room.c").read_text()
        room_start = room.index("    frontRoom = gSaveContext.respawnFlag > 0")
        room_end = room.index(";", room_start) + 1
        room = "s32 frontRoom;\n" + room[room_start:room_end] + "\nassert(frontRoom == 0);"
        fixture = fixture.replace("/* ACTUAL RESPAWN */", respawn).replace("/* ACTUAL ROOM */", room)
        message = (ROOT / "soh/src/code/z_message_PAL.c").read_text()
        fixture = fixture.replace("/* ACTUAL MESSAGE STATE */", block_from(message, message.index("u8 Message_GetState(")))
        boot = (ROOT / "soh/src/code/title_setup.c").read_text()
        title = "\n".join(re.findall(r"static s32 sCombo(?:CrossGameTitleCardPending|CustomTitleCardPresentation)[^;]*;", boot))
        for signature in ("int Combo_SOH_SwapCustomTitleCardPresentation(", "int Combo_SOH_IsPresentingCustomTitleCard(",
                          "int Combo_SOH_HasCrossGameTitleCard(", "int Combo_SOH_ConsumeCrossGameTitleCard(",
                          "void Combo_SOH_RequestCrossGameTitleCard("):
            title += "\n" + block_from(boot, boot.index(signature))
        actor = (ROOT / "soh/src/code/z_actor.c").read_text()
        library = (ROOT / "soh/src/code/z_lib.c").read_text()
        title += "\n" + block_from(library, library.index("s32 Math_StepToS("))
        title += "\n" + block_from(actor, actor.index("void TitleCard_Update("))
        fixture = fixture.replace("/* ACTUAL TITLE FUNCTIONS */", title)
        handlers = (ROOT / "soh/soh/Enhancements/TimeSavers/timesaver_hook_handlers.cpp").read_text()
        policy = handlers[handlers.index("        case VB_SHOW_TITLE_CARD:"):handlers.index("        case VB_WONDER_TALK:")]
        fixture = fixture.replace("/* ACTUAL TITLE POLICY */", policy.replace("IS_RANDO", "randomized"))
        gbi = (ROOT / "soh/soh/GbiWrap.cpp").read_text()
        gbi = block_from(gbi, gbi.index('extern "C" void gSPDisplayList('))
        cpp = Path(temporary) / "adapter.cpp"
        # Keep the production adapter include absolute when moving this fixture.
        fixture = fixture.replace("../../soh/soh/Enhancements/Gameplay/ComboTempleSages.cpp",
                                  str((args.adapter_source or ROOT / "soh/soh/Enhancements/Gameplay/ComboTempleSages.cpp").resolve()))
        cpp.write_text(fixture.replace("/* ACTUAL CONTROL */", bodies).replace("/* ACTUAL GBI */", gbi))
        include_paths = [ROOT / item for item in (
            ".", "soh", "soh/include", "soh/include/PR", "soh/src", "soh/assets", "soh/mods", "soh/soh",
            "libultraship/include", "combo", "combo/menu")]
        extra = os.environ.get("COMBO_NEI_FIXTURE_CPATH", "")
        include_paths += [path for path in (args.json_include, args.spdlog_include) if path is not None]
        env = {**os.environ, "ASAN_OPTIONS": "detect_leaks=0"}
        if extra:
            env["CPATH"] = os.pathsep.join((extra, env.get("CPATH", "")))
        native = Path(temporary) / "adapter"
        flags = ["-std=c++20", "-DCOMBO_BUILD", "-DF3DEX_GBI_2", "-DFMT_HEADER_ONLY",
                 "-DCONTROLLERBUTTONS_T=uint32_t", "-w", "-g", "-ffunction-sections", "-fdata-sections",
                 "-Wl,--gc-sections", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie"]
        subprocess.run([os.environ.get("CXX", "c++"), *flags, *["-I" + str(p) for p in include_paths],
                        str(cpp), str(ROOT / "libultraship/src/ship/resource/Resource.cpp"),
                        str(ROOT / "soh/soh/resource/type/Scene.cpp"),
                        str(ROOT / "soh/soh/resource/type/scenecommand/SetActorList.cpp"),
                        str(ROOT / "soh/soh/resource/type/scenecommand/SetAlternateHeaders.cpp"), "-o", str(native)], check=True, env=env)
        subprocess.run([str(native)], check=True, env=env)
        source = ROOT / "soh/soh/Enhancements/Gameplay/ComboTempleSages.cpp"
        subprocess.run([os.environ.get("CXX", "c++"), *flags, *["-I" + str(p) for p in include_paths],
                        "-c", str(source), "-o", str(Path(temporary) / "native_adapter.o")], check=True, env=env)
        print("PASS complete Temple Sages production translation unit with native headers (COMBO_BUILD)", flush=True)
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-fsyntax-only", str(source)], check=True)
        print("PASS original non-Combo compilation guard", flush=True)


if __name__ == "__main__":
    main()
