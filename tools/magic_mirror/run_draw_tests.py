#!/usr/bin/env python3
"""Execute the actual draw against guarded native SoH/MM graphics arenas.

The fixture compiles each engine's real global.h and GBI. Native allocator,
Graph_Open/CloseDisps and setup function bodies are copied verbatim from their
production source files. External dependencies may be supplied through CPATH.
"""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    begin = source.index(signature)
    opening = source.index("{", begin)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[begin:end]


def run():
    failures = []
    with tempfile.TemporaryDirectory(prefix="mirror-native-arenas-") as temp:
        for host in ("soh", "mm"):
            graph = (ROOT / host / "src/code/graph.c").read_text()
            setup = (ROOT / host / "src/code/z_rcp.c").read_text()
            chunks = [function(graph, "void Graph_OpenDisps("), function(graph, "void Graph_CloseDisps(")]
            if host == "soh":
                arena = (ROOT / "soh/src/code/TwoHeadArena.c").read_text()
                chunks += [function(arena, "void* THA_AllocEnd("), function(arena, "Gfx* THGA_AllocEnd("),
                           function(graph, "void* Graph_Alloc("), function(setup, "void Gfx_SetupDL_25Opa("),
                           function(setup, "void Gfx_SetupDL_25Xlu(")]
                prefix = "static Gfx sSetupDL[64][1];\n"
            else:
                chunks += [function(setup, "void Gfx_SetupDL25_Opa("), function(setup, "void Gfx_SetupDL25_Xlu(")]
                prefix = "Gfx gSetupDLs[SETUPDL_MAX][6];\n"
            fixture = (ROOT / "tools/magic_mirror/test_draw.cpp").read_text().replace("/* NATIVE_FUNCTIONS */", prefix + "\n".join(chunks))
            source = Path(temp) / f"{host}.cpp"
            source.write_text(fixture)
            executable = Path(temp) / host
            includes = [host, f"{host}/include", f"{host}/include/PR", f"{host}/src", f"{host}/assets",
                        f"{host}/" + ("2s2h" if host == "mm" else "soh"), "libultraship/include", "combo", "combo/menu"]
            command = [os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-fpermissive", "-DLOG_LEVEL_GAME_PRINTS=0", "-DCOMBO_BUILD", "-DF3DEX_GBI_2",
                       "-I" + str(ROOT), *["-I" + str(ROOT / p) for p in includes]]
            if host == "mm":
                command.append("-DCOMBO_MAGIC_MIRROR_MM")
            command += shlex.split(os.environ.get("MIRROR_TEST_CXXFLAGS", ""))
            command += [str(source), "-o", str(executable)]
            subprocess.run(command, check=True)
            print("Native arena host:", host, flush=True)
            if subprocess.run([str(executable)]).returncode:
                failures.append(host)
    if failures:
        raise SystemExit("Native arena failures: " + ", ".join(failures))


if __name__ == "__main__":
    run()
