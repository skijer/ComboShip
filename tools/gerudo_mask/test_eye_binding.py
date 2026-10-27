#!/usr/bin/env python3
"""Compile the production MmForm eye binding with a small display-command adapter.

Run with Python 3 and a C++17 compiler (CXX, default c++). No game assets needed.
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "soh/mods/transformation_masks/mm_player_form.cpp"


def check_binding(source):
    table = re.search(
        r"static const char\* sFormEyeTextures\[MM_PLAYER_FORM_MAX\]\[4\] = \{.*?\n\};",
        source, re.S
    ).group()
    symbols = set(re.findall(r"\bs(?:Goron|Zora)\w+", table))
    symbols.add("sZoraMouthClosed")
    declarations = "\n".join(
        re.search(r"static const char " + name + r"\[\] = [^;]+;", source).group()
        for name in sorted(symbols)
    )
    gerudo = re.search(
        r"alignas\(2\) static const char sGerudoEyeTextures\[2\]\[3\]\[128\] = \{.*?\n\};",
        source, re.S
    )
    # Absence is allowed here so the baseline fails on the actual missing command.
    declarations += "\n" + (gerudo.group() if gerudo else "")
    block = re.search(
        r"            s32 form = \(s32\)gFormState.currentForm;.*?"
        r"            // FD mouth:[^\n]*", source, re.S
    ).group()
    program = r'''
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <initializer_list>
using s32=int;
using u8=uint8_t;
enum { MM_PLAYER_FORM_GORON=1, MM_PLAYER_FORM_ZORA=2,
       MM_PLAYER_FORM_GERUDO=7, MM_PLAYER_FORM_MAX=11 };
static bool adult;
#define LINK_IS_ADULT adult
struct { int currentForm; u8 eyeIndex; } gFormState;
struct Cmd { int segment; uintptr_t path; } commands[10];
static Cmd* POLY_OPA_DISP;
#define gSPSegment(p,s,v) (*(p)=Cmd{s,v})
DECLARATIONS
TABLE
void bind() {
BLOCK
}
int main() {
    for (bool age : {false, true}) {
        adult=age;
        for (int eye : {0,1,2,3,4,5,255}) {
            POLY_OPA_DISP=commands;
            gFormState={MM_PLAYER_FORM_GERUDO,(u8)eye};
            bind();
            if (POLY_OPA_DISP==commands) {
                std::puts("FAIL: Gerudo segment 08 absent");
                return 1;
            }
            assert(POLY_OPA_DISP==commands+1);
            assert(commands[0].segment==8);
            assert((commands[0].path & 1)==0);
            auto path=(const char*)commands[0].path;
            assert(std::strncmp(path,"__OTR__",7)==0);
            assert(std::strstr(path,age ?
                "object_link_boy/gLinkAdultEyes" : "object_link_child/gLinkChildEyes"));
            assert(std::strstr(path,eye==1 ? "HalfTex" :
                eye==2 ? "ClosedfTex" : "OpenTex"));
        }
    }
    for (int form=0; form<MM_PLAYER_FORM_MAX; ++form) {
        if (form==MM_PLAYER_FORM_GERUDO) continue;
        for (int eye : {0,1,2,3,4,5,255}) {
            POLY_OPA_DISP=commands;
            gFormState={form,(u8)eye};
            bind();
            auto expected=sFormEyeTextures[form][eye>3 ? 0 : eye];
            int count=0;
            if (expected) {
                assert(commands[count].segment==8);
                assert((const char*)commands[count++].path==expected);
            }
            if (form==MM_PLAYER_FORM_ZORA) {
                assert(commands[count].segment==9);
                assert((const char*)commands[count++].path==sZoraMouthClosed);
            }
            assert(POLY_OPA_DISP==commands+count);
        }
    }
    std::puts("PASS: both ages, blink/fallback states, and other form bindings");
}
'''
    program = program.replace("DECLARATIONS", declarations).replace(
        "TABLE", table).replace("BLOCK", block)
    with tempfile.TemporaryDirectory(prefix="gerudo-eye-binding-") as directory:
        cpp = Path(directory) / "test.cpp"
        executable = Path(directory) / "test"
        cpp.write_text(program)
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17",
                        "-Wall", "-Wextra", "-Werror", str(cpp),
                        "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    check_binding(SOURCE.read_text())
