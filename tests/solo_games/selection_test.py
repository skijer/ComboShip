#!/usr/bin/env python3
"""Run the production game picker at a scripted renderer boundary."""
from pathlib import Path
import argparse
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--sanitize', action='store_true')
parser.add_argument('--windows', action='store_true', help='exercise Windows SDK macro behavior on the host compiler')
parser.add_argument('--msvc', action='store_true', help='compile against the real Windows SDK with MSVC')
parser.add_argument('--compiler')
args = parser.parse_args()
screen = (ROOT/'combo/gui/ComboExtractScreen.cpp').read_text()
screen = '\n'.join(line for line in screen.splitlines() if not (
    line.startswith('#include "') or
    line.startswith(('#include <libultraship/', '#include <ship/', '#include <fast/', '#include <imgui.h>'))))
if 'int ComboUI_SelectGame(' not in screen:
    screen += '\nint ComboUI_SelectGame(int) { return COMBO_GAME_BOTH; }\n'
native_picker = args.windows or args.msvc
source = ('#define COMBO_TEST_NATIVE_PICKER\n' if native_picker else '')
source += '#include "tests/rom_setup/screen_stubs.h"\n'
if native_picker:
    # Keep the production dialog header in this fixture. It precedes the game
    # selector and must not let Windows SDK macros rewrite C++ standard calls.
    source += '#ifndef _WIN32\n#define _WIN32 1\n#endif\n#include "combo/gui/ComboFilePicker.h"\n'
source += screen + r'''
int main(int argc, char** argv) {
    assert(argc==2);
    const int scenario=std::stoi(argv[1]);
    Test::selectGame=scenario<3 ? scenario : -2;
    Test::selectQuit=scenario==3;
    assert(ComboUI_SelectGame(COMBO_GAME_MM)==(scenario<3 ? scenario : -1));
    assert(Test::disabled.empty());
}
'''
with tempfile.TemporaryDirectory(prefix='solo-game-picker-') as tmp:
    directory = Path(tmp)
    cpp=directory/'screen.cpp';binary=directory/('screen.exe' if args.msvc else 'screen');cpp.write_text(source, encoding='utf-8')
    if args.windows and not args.msvc:
        # Only the OS boundary is modeled; both the dialog header and selector
        # are production code. MSVC mode uses the actual Windows SDK instead.
        (directory/'windows.h').write_text('''#pragma once
#define MAX_PATH 260
#ifndef NOMINMAX
#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))
#endif
''')
        (directory/'commdlg.h').write_text('''#pragma once
struct OPENFILENAMEA {
    unsigned long lStructSize;
    const char* lpstrFilter;
    char* lpstrFile;
    unsigned long nMaxFile;
    const char* lpstrTitle;
    unsigned long Flags;
};
#define OFN_FILEMUSTEXIST 0x1000
#define OFN_PATHMUSTEXIST 0x800
#define OFN_NOCHANGEDIR 8
inline int GetOpenFileNameA(OPENFILENAMEA*) { return 0; }
''')
    flags=['-std=c++20','-Wall','-Wextra','-Werror','-I',str(ROOT)]
    if args.sanitize: flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g']
    if args.windows:
        flags+=['-I',str(directory),'-Wno-unknown-pragmas','-Wno-missing-field-initializers']
    command = ([args.compiler or 'cl', '/nologo', '/std:c++20', '/EHsc', '/utf-8', '/DCOMBO_BUILD', '/I'+str(ROOT),
                str(cpp), '/Fe:'+str(binary)] if args.msvc else
               [args.compiler or 'g++',*flags,str(cpp),'-o',str(binary)])
    subprocess.run(command,check=True,cwd=tmp)
    for scenario in range(5):
        subprocess.run([str(binary),str(scenario)],check=True,cwd=tmp)
print('Game picker: Combo/OoT/MM, Quit and window close PASS' + (' (Windows dialog header)' if native_picker else ''))
