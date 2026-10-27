#!/usr/bin/env python3
"""Execute native shop draw bodies with real host vertices and GBI packets."""
from pathlib import Path
import argparse
import os
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--host", choices=("mm", "soh", "all"), default="all")
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix="market-lock-draw-") as temporary:
    for host in (("mm", "soh") if args.host == "all" else (args.host,)):
        source = (ROOT / host / ("2s2h" if host == "mm" else "soh") /
                  "Enhancements/Gameplay" / ("BlackMarket.cpp" if host == "mm" else "ComboBlackMarketNative.cpp")).read_text()
        bodies = "\n".join(block_from(source, source.index(signature))
                           for signature in ("void DrawSeal(", "void DrawMarket("))
        fixture = (ROOT / "tests/black_market/lock_draw_test.cpp").read_text().replace("/* PRODUCTION */", bodies)
        native_c = None
        if host == "soh":
            arena = (ROOT / "soh/src/code/TwoHeadArena.c").read_text()
            native_c = '#include "global.h"\n' + "\n".join(
                block_from(arena, arena.index(signature))
                for signature in ("void* THA_AllocEnd(", "Gfx* THGA_AllocEnd("))
            allocator = "void* NativeAllocate(GraphicsContext* ctx, size_t bytes) { return THGA_AllocEnd(&ctx->polyOpa, ALIGN16(bytes)); }\n"
        else:
            allocator = "void* NativeAllocate(GraphicsContext* ctx, size_t bytes) { return GRAPH_ALLOC(ctx, bytes); }\n"
        fixture = fixture.replace("/* NATIVE_ALLOCATION */", allocator)
        graph = (ROOT / host / "src/code/graph.c").read_text()
        debug = "\n".join(block_from(graph, graph.index(signature))
                          for signature in ("void Graph_OpenDisps(", "void Graph_CloseDisps("))
        fixture = fixture.replace("/* NATIVE_DEBUG */", debug)
        cpp = Path(temporary) / (host + ".cpp")
        cpp.write_text(fixture)
        binary = Path(temporary) / host
        includes = [host, host + "/include", host + "/include/PR", host + "/src", host + "/assets",
                    host + ("/2s2h" if host == "mm" else "/soh"), "libultraship/include", "combo"]
        objects = []
        if native_c is not None:
            path = Path(temporary) / "native_allocator.c"
            path.write_text(native_c)
            obj = Path(temporary) / "native_allocator.o"
            subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-w", "-DCOMBO_BUILD", "-DF3DEX_GBI_2",
                            "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-pie",
                            "-I" + str(ROOT), *["-I" + str(ROOT / path) for path in includes],
                            "-c", str(path), "-o", str(obj)], check=True)
            objects.append(str(obj))
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-DCOMBO_BUILD", "-DF3DEX_GBI_2",
                        *(["-DOOT_LOCK_TEST"] if host == "soh" else []), "-g", "-fsanitize=address,undefined",
                        "-fno-sanitize-recover=all", "-no-pie", "-I" + str(ROOT),
                        *["-I" + str(ROOT / path) for path in includes], str(cpp), *objects, "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
