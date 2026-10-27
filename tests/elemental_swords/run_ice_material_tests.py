#!/usr/bin/env python3
"""Exercise scoped finish ice material selection, native GBI and mesh preservation."""
import importlib.util
import os
from pathlib import Path
import re
import subprocess
import tempfile
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("finish_native", ROOT / "tests/elemental_sword_finish_native_test.py")
native = importlib.util.module_from_spec(spec)
spec.loader.exec_module(native)


def check_draw(folder):
    impl = (ROOT / "combo/gameplay/elemental_sword_finish_impl.h").read_text()
    core = (ROOT / "combo/gameplay/elemental_sword_finish_core.h").read_text()
    body = native.ice_body(impl)
    prefix = native.RENDER_PREFIX.replace(
        "Gfx* SwordFinishIce_GetDisplayList(uint32_t frame){(void)frame;return NULL;}", "") + r'''
static Gfx customIce[1];
static int materialReady;
Gfx* SwordFinishIce_GetDisplayList(uint32_t frame) { (void)frame; return materialReady ? customIce : NULL; }
'''
    main = r'''
int main(void) {
    PlayState value = {0}; PlayState* play = &value;
    SwordFinishEffect effectValue = {0}; SwordFinishEffect* effect = &effectValue;
    effect->element = SWORD_ELEMENT_ICE;
    SwordFinishPoint pointValue = {1, 2, 3, 4}; SwordFinishPoint* point = &pointValue;
    Mtx matrix; Mtx* allocatedMatrix = &matrix;
    float scale = 1; uint8_t alpha = 173; unsigned slot = 5, i = 0;
    size_t fallbackCommands = 0;
    for (materialReady = 0; materialReady < 2; ++materialReady) {
        dlCount = 0; head = commands;
        BODY
#ifdef SWORD_FINISH_OOT
        CHECK(dlCount == 1);
        CHECK(displayLists[0] == (materialReady ? (const char*)customIce : gEffIceFragment2DL));
#else
        CHECK(dlCount == 2);
        CHECK(displayLists[0] == (materialReady ? (const char*)customIce : gEffIceFragment2MaterialDL));
        CHECK(displayLists[1] == gEffIceFragment2ModelDL);
#endif
        if (!materialReady) fallbackCommands = (size_t)(head - commands);
        else {
            CHECK((size_t)(head - commands) == fallbackCommands);
            CHECK(prim[0] < prim[1] && prim[1] < prim[2]);
            CHECK(env[0] > prim[0] && env[1] > prim[1] && env[2] >= prim[2]);
            CHECK(env[3] == alpha);
        }
    }
    CHECK(scrollChecks == 2);
    puts("PASS scoped custom ice/fallback roots, native MM mesh, blue/frost envelope, fade and unchanged arena commands");
}
'''
    for host in ("OOT", "MM"):
        target = folder / ("draw-" + host)
        source = prefix + core + main.replace("BODY", body)
        command = [os.environ.get("CC", "cc"), "-std=c99", "-w", "-x", "c", "-", "-o", str(target)]
        if host == "OOT":
            command.append("-DSWORD_FINISH_OOT")
        subprocess.run(command, input=source, text=True, check=True)
        subprocess.run([str(target)], check=True)


def check_material(folder):
    target = folder / "material"
    command = [os.environ.get("CXX", "c++"), "-std=c++20", "-O1", "-g", "-DF3DEX_GBI_2",
               "-DFMT_HEADER_ONLY", "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
               "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
               "-I" + str(ROOT / "combo"), "-I" + str(ROOT / "libultraship/include"),
               str(ROOT / "tests/elemental_swords/ice_material_test.cpp"),
               str(ROOT / "libultraship/src/fast/resource/type/DisplayList.cpp"),
               str(ROOT / "libultraship/src/ship/resource/Resource.cpp"), "-o", str(target)]
    subprocess.run(command, check=True)
    subprocess.run([str(target)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})


def check_assets():
    packed = (ROOT / "combo/render/SwordFinishIceData.h").read_text()
    for name, symbol, size, contrast in (("frost.i8.png", "sSwordFinishIceFrostTex", (32, 16), 150),
                                         ("opacity.i8.png", "sSwordFinishIceOpacityTex", (64, 32), 25)):
        image = Image.open(ROOT / "combo/render/sword_finish_ice_assets" / name)
        assert image.mode == "L" and image.size == size
        pixels = image.load()
        width, height = size
        assert all(pixels[0, y] == pixels[width - 1, y] for y in range(height))
        assert all(pixels[x, 0] == pixels[x, height - 1] for x in range(width))
        values = image.tobytes()
        assert max(values) - min(values) >= contrast and len(set(values)) >= 16
        array = re.search(symbol + r"\[\d+\] = \{(.*?)\};", packed, re.S)
        assert array and bytes(int(value, 16) for value in re.findall(r"0x([0-9A-F]{2})", array[1])) == values
    print("PASS native I8 texture dimensions, seamless wrap boundaries and readable frost/opacity contrast", flush=True)


def check_native_effect(folder):
    for host, combo in ((host, combo) for host in ("soh", "mm") for combo in (False, True)):
        target = folder / ("native-effect-" + host + ("-combo" if combo else "-solo"))
        paths = [host, host + "/include", host + "/include/PR", host + "/src", host + "/assets",
                 "combo", "combo/menu", "libultraship/include"]
        if host == "mm": paths.append("mm/2s2h")
        command = [os.environ.get("CC", "cc"), "-std=gnu2x", "-O1", "-g", "-w", "-DF3DEX_GBI_2",
                   "-DCONTROLLERBUTTONS_T=uint32_t", "-DLOG_LEVEL_GAME_PRINTS=0",
                   "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-pie", "-no-pie",
                   *["-I" + str(ROOT / path) for path in paths],
                   *(["-DICE_TEST_MM"] if host == "mm" else []),
                   *(["-DCOMBO_BUILD"] if combo else []),
                   str(ROOT / "tests/elemental_swords/native_ice_material_test.c"),
                   str(ROOT / host / "src/overlays/effects/ovl_Effect_Ss_En_Ice/z_eff_ss_en_ice.c"),
                   "-lm", "-o", str(target)]
        subprocess.run(command, check=True)
        subprocess.run([str(target)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})


def check_frozen_damage(folder):
    actor = (ROOT / "mm/src/code/z_actor.c").read_text()
    start = actor.index("void Actor_DrawDamageEffects(")
    end = actor.index("void Actor_SpawnIceEffects(", start)
    array_start = actor.index("TexturePtr sElectricSparkTextures[]")
    array_end = actor.index("};", array_start) + 2
    source = folder / "native-frozen-draw.c"
    source.write_text('#include "global.h"\n#include "din_fire_sword.h"\n'
                      '#include "render/SwordFinishIceMaterial.h"\n'
                      '#include "objects/gameplay_keep/gameplay_keep.h"\n'
                      '#include "2s2h/GameInteractor/GameInteractor.h"\n'
                      '#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"\n' +
                      actor[array_start:array_end] + "\n" + actor[start:end])
    paths = ["mm", "mm/include", "mm/include/PR", "mm/src", "mm/assets", "mm/2s2h",
             "combo", "combo/menu", "libultraship/include"]
    for combo in (False, True):
        target = folder / ("native-frozen-draw-combo" if combo else "native-frozen-draw-solo")
        command = [os.environ.get("CC", "cc"), "-std=gnu2x", "-O1", "-g", "-w", "-DF3DEX_GBI_2",
                   "-DCONTROLLERBUTTONS_T=uint32_t", "-DICE_TEST_MM", "-DLOG_LEVEL_GAME_PRINTS=0",
                   "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-pie", "-no-pie",
                   *["-I" + str(ROOT / path) for path in paths],
                   *(["-DCOMBO_BUILD"] if combo else []),
                   str(ROOT / "tests/elemental_swords/native_frozen_damage_material_test.c"), str(source),
                   "-lm", "-o", str(target)]
        subprocess.run(command, check=True)
        subprocess.run([str(target)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})


if __name__ == "__main__":
    with tempfile.TemporaryDirectory(prefix="finish-ice-material-") as directory:
        folder = Path(directory)
        check_draw(folder)
        check_material(folder)
        check_assets()
        check_native_effect(folder)
        check_frozen_damage(folder)
