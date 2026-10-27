#!/usr/bin/env python3
"""Compile the actual file-select query with native GameState and SDL headers."""
import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--compiler", default=os.environ.get("CXX", "c++"))
parser.add_argument("--msvc", action="store_true")
parser.add_argument("--include", type=Path, action="append", default=[])
args = parser.parse_args()

production = (ROOT / "soh/soh/OTRGlobals.cpp").read_text(encoding="utf-8")
preamble = production[:production.index("#include")]
query = block_from(production, production.index('extern "C" COMBO_EXPORT uint8_t SOH_IsOnFileSelect('))
sdl = r'''
// Prime SDL's ordinary compiler configuration before selecting its Windows
// entry-point policy on a non-Windows test host. Use the real SDL_main header.
#if !defined(__WIN32__)
#define __WIN32__
#define PROBE_UNDEFINE_WIN32
#endif
#include <SDL2/SDL_main.h>
#ifdef PROBE_UNDEFINE_WIN32
#undef __WIN32__
#undef PROBE_UNDEFINE_WIN32
#endif
'''
body = r'''
extern "C" {
PlayState* gPlayState = nullptr;
GameState* gGameState = nullptr;
void FileChoose_Main(GameState*) {}
void Other_Main(GameState*) {}
}
/* ACTUAL_QUERY */
// The fixture owns its ordinary entry point; production was compiled while
// SDL's macro was active unless the production preamble correctly prevented it.
#undef main
int main() {
    static GameState fileSelect{};
    static PlayState play{};
    assert(!SOH_IsOnFileSelect());
    gGameState = &fileSelect;
    fileSelect.main = FileChoose_Main;
    assert(SOH_IsOnFileSelect());
    gPlayState = &play;
    assert(!SOH_IsOnFileSelect());
    gPlayState = nullptr;
    fileSelect.main = Other_Main;
    assert(!SOH_IsOnFileSelect());
    fileSelect.main = nullptr;
    assert(!SOH_IsOnFileSelect());
    gGameState = nullptr;
    assert(!SOH_IsOnFileSelect());
}
'''.replace("/* ACTUAL_QUERY */", query)

includes = [ROOT / p for p in ("soh", "soh/include", "soh/include/PR", "soh/assets",
                               "libultraship/include", "combo")]
includes += args.include
with tempfile.TemporaryDirectory(prefix="file-select-sdl-main-") as directory:
    build = Path(directory)
    for engine_first in (True, False):
        name = "engine-first" if engine_first else "sdl-first"
        source = preamble + '#include <cassert>\n#include <SDL2/SDL_stdinc.h>\n'
        if engine_first:
            source += '#include "z64.h"\n'
        source += sdl
        if not engine_first:
            source += '#include "z64.h"\n'
        source += '#include "ComboExport.h"\n' + body
        cpp = build / (name + ".cpp")
        cpp.write_text(source, encoding="utf-8")
        binary = build / (name + (".exe" if args.msvc else ""))
        if args.msvc:
            # Match the engine's UTF-8 and C++20 X-macro preprocessor settings.
            command = [args.compiler, "/nologo", "/std:c++20", "/EHsc", "/utf-8", "/Zc:preprocessor",
                       "/DF3DEX_GBI_2", "/DCOMBO_BUILD",
                       *["/I" + str(p) for p in includes], str(cpp), "/Fo" + str(build / (name + ".obj")),
                       "/Fe" + str(binary)]
        else:
            command = [args.compiler, "-std=c++20", "-DF3DEX_GBI_2", "-DCOMBO_BUILD",
                       *["-I" + str(p) for p in includes], str(cpp), "-o", str(binary)]
        subprocess.run(command, cwd=build, check=True)
        subprocess.run([str(binary)], cwd=build, check=True)
        print(f"PASS native file-select query: SDL Windows main policy, {name}, all screen/null controls", flush=True)
