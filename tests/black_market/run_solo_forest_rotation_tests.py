#!/usr/bin/env python3
"""Run converted native forest actor rows through both real actor-spawn bodies.

Only native scene command rows are used here; the absent POC9 scenery is never
substituted and the converter's immutable donor contract is not bypassed.
"""
import argparse
import ast
import hashlib
import importlib.util
import os
import re
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from


def converter_rows(source, payload, engine):
    # Execute the production converter's actor-row statements, rather than
    # recoding their endian, angle, flag or parameter conversion in the test.
    module = ast.parse(source)
    convert = next(node for node in module.body if isinstance(node, ast.FunctionDef) and node.name == "convert")
    loop = next(node for node in ast.walk(convert) if isinstance(node, ast.For) and
                isinstance(node.target, ast.Tuple) and
                [part.id for part in node.target.elts] == ["engine", "prefix"])
    statements = []
    for node in loop.body:
        if isinstance(node, ast.Assign) and any(isinstance(target, ast.Name) and target.id in ("portal", "spawn") for target in node.targets):
            statements.append(node)
        elif isinstance(node, ast.If) and ast.unparse(node.test) == "engine == 'oot'":
            statements.append(node)
    assert len(statements) == 3, "update native fixture when actor-row conversion structure changes"
    context = {"struct": struct, "engine": engine, "transition": bytearray(payload)}
    exec(compile(ast.Module(body=statements, type_ignores=[]), "forest.py actor-row production statements", "exec"), context)
    return struct.unpack("<HhhhhhhH", context["spawn"]), struct.unpack_from("<BBBBhhhhhH", context["portal"], 4)


FIXTURE = r'''
#include "global.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <initializer_list>
#ifndef OOT_NATIVE
#include "z64door.h"
#endif
struct Captured { int id, x, y, z, rx, ry, rz, params; unsigned cs, days; } captured;
static Actor spawned;
static int calls;
#ifdef OOT_NATIVE
int gMapLoading;
#undef osSyncPrintf
#define osSyncPrintf(...) ((void)0)
#define GameInteractor_Should(flag, result, ...) (result)
extern "C" Actor* Actor_Spawn(ActorContext*, PlayState*, s16 id, f32 x, f32 y, f32 z,
                             s16 rx, s16 ry, s16 rz, s16 params) {
    ++calls; captured = {id, (int)x, (int)y, (int)z, rx, ry, rz, params, 0, 0}; return &spawned;
}
#else
extern "C" Actor* Actor_SpawnAsChildAndCutscene(ActorContext*, PlayState*, s16 id, f32 x, f32 y, f32 z,
                             s16 rx, s16 ry, s16 rz, s32 params, u32 cs, u32 days, Actor*) {
    ++calls; captured = {id, (int)x, (int)y, (int)z, rx, ry, rz, params, cs, days}; return &spawned;
}
#endif
/* PRODUCTION */
static void CheckTransition() {
    for (int room : {0, 1, 2}) {
        PlayState play{}; ActorContext actors{}; TransitionActorEntry transition{};
        /* TRANSITION ROW */
        play.roomCtx.curRoom.num = room; play.roomCtx.prevRoom.num = -1;
#ifdef OOT_NATIVE
        play.transiActorCtx.list = &transition; play.transiActorCtx.numActors = 1;
#else
        play.transitionActors.list = &transition; play.transitionActors.count = 1;
#endif
        calls = 0;
        Actor_SpawnTransitionActors(&play, &actors);
        assert(calls == (room != 2));
        if (room == 2) continue;
        assert(captured.id == ACTOR_EN_HOLL);
        assert(captured.x == 800 && captured.y == 0 && captured.z == 0);
        assert(captured.rx == 0 && captured.ry == -16384 && captured.rz == 0);
#ifdef OOT_NATIVE
        assert(captured.params == (4 << 6)); // native horizontal invisible hall
#else
        assert(captured.params == 0x7F && captured.cs == 127 && captured.days == HALFDAYBIT_ALL);
#endif
        assert(transition.id < 0);
        Actor_SpawnTransitionActors(&play, &actors); assert(calls == 1);
    }
}
static void CheckPlayer() {
    PlayState play{}; ActorContext actors{}; ActorEntry entry{};
    /* PLAYER ROW */
    calls = 0; Actor_SpawnEntry(&actors, &entry, &play);
    assert(calls == 1 && captured.id == ACTOR_PLAYER);
    assert(captured.x == -486 && captured.y == 0 && captured.z == -227);
    assert(captured.rx == 0 && captured.rz == 0);
#ifdef OOT_NATIVE
    assert(captured.ry == 12288 && captured.params == 0x0DFF);
#else
    assert(captured.ry == DEG_TO_BINANG(68) && std::abs(captured.ry - 12288) <= 100);
    assert(captured.params == 0x0EFF && captured.cs == 127 && captured.days == HALFDAYBIT_ALL);
#endif
}
int main() {
    /* CASE */
    std::puts("PASS production native forest actor spawn: room hall direction, player pose, day/cutscene bits");
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native", type=Path, default=ROOT.parent / "native-inputs/mm(1).o2r")
    parser.add_argument("--case", choices=("both", "transition", "spawn"), default="both")
    args = parser.parse_args()
    converter = ROOT / "tools/black_market/forest.py"
    spec = importlib.util.spec_from_file_location("forest", converter)
    forest = importlib.util.module_from_spec(spec); spec.loader.exec_module(forest)
    assert hashlib.sha256(args.native.read_bytes()).hexdigest() == forest.NATIVE_SHA
    with zipfile.ZipFile(args.native) as archive:
        payload = forest.native_commands(archive.read(forest.NATIVE + "Z2_LOST_WOODS"))[14]
    assert struct.unpack_from("<I", payload)[0] == 1
    source = converter.read_text()
    with tempfile.TemporaryDirectory(prefix="solo-forest-rotation-") as temporary:
        for engine in ("mm", "soh"):
            player, transition = converter_rows(source, payload, "oot" if engine == "soh" else "mm")
            actor_source = (ROOT / engine / "src/code/z_actor.c").read_text()
            definitions = (r"(?m)^void Actor_SpawnTransitionActors\([^;{}]*\)\s*\{",
                           r"(?m)^Actor\* Actor_SpawnEntry\([^;{}]*\)\s*\{")
            bodies = "\n".join(block_from(actor_source, re.search(pattern, actor_source).start())
                               for pattern in definitions)
            names = ("id", "pos.x", "pos.y", "pos.z", "rot.x", "rot.y", "rot.z", "params")
            player_row = "\n".join(f"entry.{name} = {value};" for name, value in zip(names, player))
            names = ("sides[0].room", "sides[0].effects", "sides[1].room", "sides[1].effects",
                     "id", "pos.x", "pos.y", "pos.z", "rotY", "params")
            if engine == "mm":
                names = tuple(name.replace("effects", "bgCamIndex") for name in names)
            transition_row = "\n".join(f"transition.{name} = {value};" for name, value in zip(names, transition))
            cases = {"both": "CheckTransition(); CheckPlayer();", "transition": "CheckTransition();", "spawn": "CheckPlayer();"}
            fixture = FIXTURE.replace("/* PRODUCTION */", bodies).replace("/* PLAYER ROW */", player_row)
            fixture = fixture.replace("/* TRANSITION ROW */", transition_row).replace("/* CASE */", cases[args.case])
            target = Path(temporary) / (engine + ".cpp"); target.write_text(fixture)
            binary = Path(temporary) / engine
            includes = (engine, engine + "/include", engine + "/include/PR", engine + "/src", engine + "/assets",
                        engine + ("/soh" if engine == "soh" else "/2s2h"), "libultraship/include", "combo")
            subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-DF3DEX_GBI_2", "-DCOMBO_BUILD",
                            *(["-DOOT_NATIVE"] if engine == "soh" else []), "-g", "-fsanitize=address,undefined",
                            "-fno-sanitize-recover=all", "-no-pie", *["-I" + str(ROOT / path) for path in includes],
                            str(target), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})


if __name__ == "__main__":
    main()
