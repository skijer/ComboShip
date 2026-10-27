#!/usr/bin/env python3
"""Replay the market's real spawn through scene init and player cutscene setup.

The resource command and actor-allocation boundaries are adapters. Scene init,
Actor_SpawnEntry, the cutscene manager and player cutscene assignment use actual
production bodies and engine types. No renderer or game session is claimed.
"""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / "scripts/diagnostics"), str(ROOT / "tools/black_market"),
               str(ROOT / "scripts/assets")]
from run_time_pedestal_tests import block_from, functions
import native_forest
import native_market
import prepare_native_forest


def main():
    resources = native_market.read_bundle(ROOT / "tools/native_market/bundled")
    commands = native_forest.read_scene(resources[native_market.SCENES["mm"]])
    spawn = native_forest.read_actor_list(commands[0])[0]
    assert commands[0x1b] == b"\0\0\0\0", "fixture must replay the actual empty cutscene list"
    startup = (ROOT / "mm/2s2h/z_play_2SH.cpp").read_text()
    eventmgr = (ROOT / "mm/src/code/z_eventmgr.c").read_text()
    play = (ROOT / "mm/src/code/z_play.c").read_text()
    actors = (ROOT / "mm/src/code/z_actor.c").read_text()
    camera = (ROOT / "mm/src/code/z_camera.c").read_text()
    forest = prepare_native_forest.verified_resources(prepare_native_forest.DEFAULT_MANIFEST)
    forest_commands = native_forest.read_scene(forest[native_forest.COMBINED])
    forest_spawn = native_forest.read_actor_list(forest_commands[0])[0]
    declarations = eventmgr[eventmgr.index("typedef enum {"):eventmgr.index("s16 CutsceneManager_SetHudVisibility(")]
    body = declarations + "\n"
    body += block_from(eventmgr, eventmgr.index("ActorCutscene sGlobalCutsceneList[]")) + ";\n"
    for name in ("CutsceneManager_Init", "CutsceneManager_StoreCamera", "CutsceneManager_GetCutsceneEntryImpl", "CutsceneManager_GetCutsceneEntry"):
        body += functions(eventmgr, {name})[name] + "\n"
    body += functions(camera, {"Camera_ChangeStatus"})["Camera_ChangeStatus"] + "\n"
    body += block_from(play, play.index("s16 sPlayerCsIdToCsCamId[]")) + ";\n"
    # Compile any local startup helper with its real COMBO_BUILD guard. The
    # unmodified baseline has no helper and reproduces the supplied null read.
    for helper in ("ComboBlackMarket_PrepareSpawn",):
        if helper in functions(startup):
            body += "#ifdef COMBO_BUILD\n" + functions(startup)[helper] + "\n#endif\n"
    body += block_from(startup, startup.index('extern "C" void OTRPlay_InitScene(')) + "\n"
    body += functions(actors, {"Actor_SpawnEntry"})["Actor_SpawnEntry"] + "\n"
    body += re.sub(r"\bthis\b", "current", functions(play, {"Play_AssignPlayerCsIdsFromScene"})[
        "Play_AssignPlayerCsIdsFromScene"]) + "\n"
    # The allocation boundary retains the real production csId normalization.
    normalization = re.search(r"    actor->csId = csId & 0x7F;.*?\n    }", actors, re.S)[0]
    fixture = (ROOT / "tests/black_market/scene_spawn_test.cpp").read_text()
    fixture = fixture.replace("/* PRODUCTION */", body)
    fixture = fixture.replace("/* CS_ID_NORMALIZATION */", normalization)
    fixture = fixture.replace("/* RESOURCE_SPAWN */", "{" + ",".join(map(str, spawn[:1])) + ",{" +
                              ",".join(map(str, spawn[1:4])) + "},{" + ",".join(map(str, spawn[4:7])) +
                              "}," + str(spawn[7]) + "}")
    fixture = fixture.replace("/* FOREST_SPAWN */", "{" + str(forest_spawn[0]) + ",{" +
                              ",".join(map(str, forest_spawn[1:4])) + "},{" +
                              ",".join(map(str, forest_spawn[4:7])) + "}," + str(forest_spawn[7]) + "}")
    fixture = fixture.replace("/* FOREST_HAS_CUTSCENES */", str(0x1b in forest_commands).lower())
    includes = ("mm", "mm/2s2h", "mm/include", "mm/include/PR", "mm/src", "mm/assets",
                "libultraship/include", "combo")
    with tempfile.TemporaryDirectory(prefix="market-scene-spawn-") as temporary:
        source = Path(temporary) / "test.cpp"
        source.write_text(fixture)
        for combo in (True, False):
            binary = Path(temporary) / ("combo" if combo else "standalone")
            subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-DF3DEX_GBI_2",
                            *(["-DCOMBO_BUILD"] if combo else []), "-g", "-fsanitize=address,undefined",
                            "-fno-sanitize-recover=all", "-no-pie",
                            *["-I" + str(ROOT / path) for path in includes], str(source), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})


if __name__ == "__main__":
    main()
