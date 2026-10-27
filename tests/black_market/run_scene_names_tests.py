#!/usr/bin/env python3
"""Exercise native scene names and resolver bounds in both build modes."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from

stats = (ROOT / "soh/soh/Enhancements/gameplaystats.cpp").read_text()
util = (ROOT / "soh/soh/util.cpp").read_text()
arrays = block_from(stats, stats.index("const char* const sceneMappings[]")) + ";\n"
arrays += block_from(util, util.index("std::vector<std::string> sceneNames")) + ";\n"
resolver = block_from(stats, stats.index("const char* ResolveSceneID("))
fixture = '''#include <cassert>
#include <cstring>
#include <string>
#include <vector>
#define ARRAY_COUNT(a) (sizeof(a)/sizeof((a)[0]))
enum SceneId {
#define DEFINE_SCENE(segment, title, id, config, unknown, unused) id,
#include "tables/scene_table.h"
#undef DEFINE_SCENE
SCENE_ID_MAX
};
''' + arrays + resolver + '''
int main() {
    assert(!strcmp(ResolveSceneID(SCENE_LOST_WOODS, 0), "Lost Woods"));
    assert(!strcmp(ResolveSceneID(SCENE_TESTROOM, 0), "Chest Room"));
#ifdef COMBO_BUILD
    assert(SCENE_SKIEJER_MARKET == 0x6E);
    assert(!strcmp(ResolveSceneID(SCENE_SKIEJER_MARKET, 0), "Skiejer's Black Market"));
    assert(sceneNames[SCENE_SKIEJER_MARKET] == "Skiejer's Black Market");
    assert(SCENE_SKIEJER_FOREST == 0x6F);
    assert(!strcmp(ResolveSceneID(SCENE_SKIEJER_FOREST, 0), "Lost Woods (Shared Forest)"));
    assert(sceneNames[SCENE_SKIEJER_FOREST] == "Lost Woods (Shared Forest)");
#else
    assert(SCENE_ID_MAX == 0x6E);
#endif
    assert(!strcmp(ResolveSceneID(-1, 0), "???"));
    assert(!strcmp(ResolveSceneID(SCENE_ID_MAX, 0), "???"));
    assert(!strcmp(ResolveSceneID(32767, 0), "???"));
    assert(sceneNames.size() == SCENE_ID_MAX + 1);
    assert(sceneNames.back() == "Unknown");
}
'''
with tempfile.TemporaryDirectory(prefix="market-scene-names-") as temp:
    source = Path(temp) / "names.cpp"
    source.write_text(fixture)
    for combo in (True, False):
        binary = Path(temp) / ("combo" if combo else "standalone")
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-g",
                        "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie",
                        *(["-DCOMBO_BUILD"] if combo else []),
                        "-I" + str(ROOT / "soh/include"), str(source), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
        print("PASS production scene resolver bounds and names: " + ("ComboShip" if combo else "standalone SoH"))
