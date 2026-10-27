#!/usr/bin/env python3
"""Exercise both games' real shield/light functions without the renderer.

Production declarations and function bodies are compiled unchanged. Only the
play-state accessor and random generator are supplied by the small fixture.
"""
import os
from pathlib import Path
import re
import resource
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def declaration(source, name):
    end = re.search(r"}\s*" + re.escape(name) + r"\s*;", source)
    if end is None:
        raise ValueError(name)
    start = source.rfind("typedef ", 0, end.start())
    return source[start:end.end()]


def function(source, name):
    match = re.search(r"^[\w* ]+\b" + name + r"\([^;\n]*\)\s*\{", source, re.M)
    if match is None:
        raise ValueError(name)
    depth = 1
    end = match.end()
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


def main():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    failures = []
    cases = ["null_light", "allocation_failure", "head_middle_tail", "repeat_destroy", "next_scene"]
    with tempfile.TemporaryDirectory(prefix="shield-lifetime-") as directory:
        work = Path(directory)
        for game in ("soh", "mm"):
            light_header = (ROOT / game / "include/z64light.h").read_text()
            effect_header = (ROOT / game / ("include/z64effect.h" if game == "soh" else
                                           "include/z64eff_shield_particle.h")).read_text()
            lights = (ROOT / game / "src/code/z_lights.c").read_text()
            effect = (ROOT / game / "src/code/z_eff_shield_particle.c").read_text()
            decls = [declaration(light_header, name) for name in
                     ("LightPoint", "LightDirectional", "LightParams", "LightInfo", "LightNode", "LightContext")]
            decls += [declaration(effect_header, name) for name in
                      ("EffectShieldParticleElement", "EffectShieldParticleInit", "EffectShieldParticle")]
            decls += [declaration(lights if game == "soh" else light_header, "LightsBuffer")]
            bodies = [function(lights, name) for name in
                      ("Lights_FindBufSlot", "Lights_FreeNode", "LightContext_InsertLight", "LightContext_RemoveLight")]
            bodies += [function(effect, name) for name in ("EffectShieldParticle_Init", "EffectShieldParticle_Destroy")]
            unit = """
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8; typedef int8_t s8; typedef int16_t s16;
typedef int32_t s32; typedef uint32_t u32; typedef float f32;
typedef struct { s16 x, y, z; } Vec3s;
typedef struct { u8 r, g, b, a; } Color_RGBA8;
#define LIGHTS_BUFFER_SIZE 32
#define LIGHT_POINT_NOGLOW 0
#define ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))
#define osSyncPrintf(...) ((void)0)
""" + "\n".join(decls) + """
typedef struct PlayState { LightContext lightCtx; } PlayState;
static PlayState play;
static LightsBuffer sLightsBuffer;
static PlayState* Effect_GetPlayState(void) { return &play; }
static float Rand_ZeroOne(void) { return 0.5f; }
""" + "\n".join(bodies) + "\n" + (ROOT / "tests/effect_lifetime/shield_particle_test.c").read_text()
            source = work / (game + ".c")
            source.write_text(unit)
            binary = work / game
            subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-g", "-O0", "-fsanitize=undefined",
                            "-fno-sanitize-recover=all", str(source), "-o", str(binary)], check=True)
            for case in cases:
                result = subprocess.run([str(binary), case], capture_output=True, text=True)
                print(f"{'PASS' if result.returncode == 0 else 'FAIL'} {game}: {case}", flush=True)
                if result.returncode:
                    print((result.stdout + result.stderr).strip())
                    failures.append((game, case))
    if failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
