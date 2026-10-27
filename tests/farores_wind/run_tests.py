"""Execute production Farore's Wind scene policy and cross-game save/bridge bodies."""
import argparse
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("--sanitize", action="store_true")
args = parser.parse_args()

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from


def body(path, name):
    source = (ROOT / path).read_text()
    match = re.search(r'^(?:extern "C" )?(?:static )?[\w: *&]+\b' + re.escape(name)
                      + r"\s*\([^;{}]*\)\s*\{", source, re.M)
    if not match:
        raise RuntimeError("Production function not found: " + name)
    return block_from(source, match.start())


def run(name, source, flags=()):
    with tempfile.TemporaryDirectory(prefix="farores-wind-") as directory:
        source_path = Path(directory) / "test.cpp"
        source_path.write_text(source)
        binary = Path(directory) / "test"
        sanitizer = ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-g"] if args.sanitize else []
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-O1", "-I" + str(ROOT / "combo"),
                        *flags, *sanitizer, str(source_path), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, cwd=directory)
        print("PASS " + name, flush=True)


scenes = "SCENE_MITURIN SCENE_MITURIN_BS SCENE_HAKUGIN SCENE_HAKUGIN_BS SCENE_SEA SCENE_SEA_BS SCENE_INISIE_N SCENE_INISIE_R SCENE_INISIE_BS SCENE_LAST_DEKU SCENE_LAST_GORON SCENE_LAST_ZORA SCENE_LAST_LINK SCENE_LAST_BS".split()
policy = """
#include <cassert>
typedef int s32;
struct PlayState { int sceneId; };
int Play_GetOriginalSceneId(int scene) { return scene; }
enum { """ + ",".join(scenes) + """, SCENE_CLOCK_TOWN, SCENE_TERMINA_FIELD, SCENE_KAKUSIANA };
""" + body("mm/mods/items/logic/item_oot_spells.c", "OotSpells_SceneAllowsFarores")
run("MM temples retain casting", policy + "int main(){PlayState p{SCENE_MITURIN};assert(OotSpells_SceneAllowsFarores(&p));}")
run("standalone MM keeps its original scene policy", policy + "int main(){PlayState p{SCENE_CLOCK_TOWN};assert(!OotSpells_SceneAllowsFarores(&p));}")
run("ComboShip Farore's Wind activates outdoors", policy + "int main(){PlayState p{SCENE_CLOCK_TOWN};assert(OotSpells_SceneAllowsFarores(&p));p.sceneId=SCENE_TERMINA_FIELD;assert(OotSpells_SceneAllowsFarores(&p));}", ["-DCOMBO_BUILD"])
run("MM grottos cannot capture a stale parent-scene point", policy + "int main(){PlayState p{SCENE_KAKUSIANA};assert(!OotSpells_SceneAllowsFarores(&p));}", ["-DCOMBO_BUILD"])

run("shared point JSON", (ROOT / "tests/farores_wind/json_test.cpp").read_text())
container_names = ["ComboContainerPath", "ComboIsValidSlot", "ComboReleaseMajorMinor", "LoadOrCreateContainer",
                   "FlushContainer", "Combo_CopyContainer", "Combo_ReadGameSave", "Combo_WriteGameSave",
                   "Combo_ReadFwPoint", "Combo_WriteFwPoint", "Combo_RequestFwReturn"]
container = (ROOT / "tests/farores_wind/container_test.cpp").read_text().replace(
    "// PRODUCTION_CONTAINER_FUNCTIONS", "\n".join(body("combo/ComboShip.cpp", name) for name in container_names))
run("launcher save persistence and both recall handoffs", container)
for game in ("soh", "mm"):
    flags = ["-DCOMBO_BUILD", "-DF3DEX_GBI_2", "-DCONTROLLERBUTTONS_T=uint32_t", "-DLOG_LEVEL_GAME_PRINTS=0"]
    flags += ["-I" + str(ROOT / path) for path in (game, game + "/include", game + "/include/PR", game + "/src",
                                                game + "/assets", game + "/mods", "libultraship/include")]
    if game == "mm":
        flags += ["-DTEST_MM", "-DMM_BUILD_DLL", "-I" + str(ROOT / "mm/2s2h")]
    else:
        flags += ["-DSOH_BUILD_DLL", "-I" + str(ROOT / "soh/soh")]
    capture = body(game + "/src/code/z_play.c", "Play_SaveCycleSceneFlags" if game == "mm" else "Play_SaveSceneFlags")
    if game == "mm":
        capture = re.sub(r"\bthis\b", "play", capture)
    source = (ROOT / "tests/farores_wind/native_test.cpp").read_text().replace("// NATIVE_FLAG_CAPTURE", capture)
    run(game + " native point bridge", source.replace(
        '"../../mm/', '"' + str(ROOT) + '/mm/').replace('"../../soh/', '"' + str(ROOT) + '/soh/'), flags)
