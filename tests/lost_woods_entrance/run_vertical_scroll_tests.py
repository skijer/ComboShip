#!/usr/bin/env python3
"""Exercise the actual Lost Woods draw owner and native interpolated GBI.

The source override demonstrates that the fixture fails on the pre-binding
implementation without rewriting a worktree. No scene archive is needed.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SCENE = "soh/src/code/z_scene_table.c"


def function(source, signature):
    start = source.index(signature)
    end = source.index("{", start) + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-ref", help="Read only the scene draw body from this git ref")
    args = parser.parse_args()
    scene = (subprocess.check_output(["git", "show", args.source_ref + ":" + SCENE], cwd=ROOT, text=True)
             if args.source_ref else (ROOT / SCENE).read_text())
    wrapper = (ROOT / "soh/soh/GbiWrap.cpp").read_text()
    rcp = (ROOT / "soh/src/code/z_rcp.c").read_text()
    interpreter = (ROOT / "libultraship/src/fast/interpreter.cpp").read_text()
    bodies = "\n".join((
        function(wrapper, 'extern "C" void gDPSetTileSizeLerp('),
        function(wrapper, 'extern "C" void gSPSegment('),
        function(rcp, "Gfx* Gfx_TwoTexScrollEx("),
        function(scene, "void func_8009EE44("),
    ))
    dispatch = "\n".join((
        function(interpreter, "void* Interpreter::SegAddr("),
        "#ifdef COMBO_BUILD",
        function(interpreter, "static bool ComboIsUnresolvedSegmentTarget("),
        function(interpreter, "bool gfx_dl_handler_common("),
        "#endif",
    ))
    fixture = r'''
#include "global.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <unordered_set>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif
#undef OPEN_DISPS
#undef CLOSE_DISPS
#define OPEN_DISPS(ctx) { GraphicsContext* __gfxCtx=(ctx)
#define CLOSE_DISPS(ctx) }
SaveContext gSaveContext{};
extern "C" { u8 gItemSlots[56]{}; }
static bool shuffleTrade, hasTradeFlag;
static int chickenCries;
extern "C" u8 Randomizer_GetSettingValue(RandomizerSettingKey key) {
    assert(key == RSK_SHUFFLE_ADULT_TRADE); return shuffleTrade;
}
extern "C" s32 Flags_GetRandomizerInf(RandomizerInf flag) {
    assert(flag == RAND_INF_ADULT_TRADES_HAS_COJIRO); return hasTradeFlag;
}
extern "C" void Player_PlaySfx(Actor*, u16 sfx) {
    assert(sfx == NA_SE_EV_CHICKEN_CRY_M); ++chickenCries;
}
extern "C" int ResourceMgr_OTRSigCheck(char*) { return 0; }
extern "C" char* ResourceMgr_LoadIfDListByName(char*) { return nullptr; }
// Match the native frame arena's heap address class. Low no-PIE globals are
// intentionally rejected by the interpreter's unresolved N64-address guard.
static std::vector<Gfx> arena(256);
static size_t used;
extern "C" void* Graph_Alloc(GraphicsContext*, size_t bytes) {
    assert(bytes == 12 * sizeof(Gfx) && used + 12 <= arena.size());
    auto* result = arena.data() + used; used += 12; return result;
}
/* DRAW */
using F3DGfx = Gfx;
struct Interpreter { uintptr_t mSegmentPointers[16]{}; void* SegAddr(uintptr_t); };
struct Stack {
    int calls=0, branches=0; F3DGfx* target=nullptr;
    void call(F3DGfx*, F3DGfx* to) { ++calls; target=to; }
    void branch(F3DGfx*) { ++branches; }
} g_exec_stack;
static F3DGfx* ComboResolveDisplayListTarget(uintptr_t, F3DGfx* target) { return target; }
#define SPDLOG_ERROR(...) ((void)0)
#define C0(pos, width) ((cmd->words.w0 >> (pos)) & ((1U << (width)) - 1))
/* DISPATCH */
static void Same(const Gfx* actual, const Gfx* expected, size_t count) {
    for (size_t i=0; i<count; ++i) {
        assert(actual[i].words.w0 == expected[i].words.w0);
        assert(actual[i].words.w1 == expected[i].words.w1);
    }
}
static void Bound(const Gfx& command, int segment, const Gfx* expected) {
    Gfx binding{};
    gSPSegment(&binding, segment, reinterpret_cast<uintptr_t>(expected));
    assert(command.words.w0 == binding.words.w0);
    Same(reinterpret_cast<const Gfx*>(command.words.w1), expected, 12);
}
int main() {
    GraphicsContext gfx{}; PlayState play{}; Player player{};
    play.state.gfxCtx=&gfx;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head=&player.actor;
    gSaveContext.ship.quest.id=QUEST_NORMAL;
    gSaveContext.inventory.items[0]=ITEM_NONE;
#ifdef COMBO_BUILD
    constexpr int segments=3;
#else
    constexpr int segments=2;
#endif
    for (u32 frame : {0u,1u,15u,16u,31u,32u,63u,64u,65u,127u,128u,2047u,2048u,65535u,0xffffffffu}) {
        Gfx opa[8]{}, xlu[8]{};
        gfx.polyOpa.p=opa; gfx.polyXlu.p=xlu; used=0;
        play.gameplayFrames=frame;
        func_8009EE44(&play);
        assert(gfx.polyOpa.p-opa == 2 && "Lost Woods OPA commands must remain unchanged");
        assert(gfx.polyXlu.p-xlu == segments+2 && "dedicated vertical XLU segment is missing");
        assert(used == size_t(segments)*12);
        // These commands and both original scroll lists are the native control.
        Gfx tail[] = {gsDPPipeSync(), gsDPSetEnvColor(128,128,128,128)};
        Same(opa,tail,2); Same(xlu+segments,tail,2);
        Bound(xlu[0],8,Gfx_TwoTexScrollEx(&gfx,0,frame%128,0,32,16,1,frame%128,0,32,16,1,0,1,0));
        Bound(xlu[1],9,Gfx_TwoTexScrollEx(&gfx,0,127-frame%128,frame%128,32,32,1,frame%128,frame%128,32,32,-1,1,1,1));
#ifdef COMBO_BUILD
        Bound(xlu[2],10,Gfx_TwoTexScrollEx(&gfx,0,0,frame%64,32,16,1,0,frame%64,32,16,0,1,0,1));
        Interpreter interpreter;
        interpreter.mSegmentPointers[10]=xlu[2].words.w1;
        for (bool branch : {false,true}) {
            Gfx call=branch ? Gfx{gsSPBranchList(0x0A000001)} : Gfx{gsSPDisplayList(0x0A000001)};
            Gfx* pc=&call; g_exec_stack={};
            assert(interpreter.SegAddr(call.words.w1)==reinterpret_cast<void*>(xlu[2].words.w1));
            const bool branched=gfx_dl_handler_common(&interpreter,&pc);
            assert(branched==branch);
            if (branch) assert(g_exec_stack.branches==1 && pc==reinterpret_cast<Gfx*>(xlu[2].words.w1));
            else assert(g_exec_stack.calls==1 && g_exec_stack.target==reinterpret_cast<Gfx*>(xlu[2].words.w1));
        }
        for (uintptr_t address : {uintptr_t(0x0A000000),uintptr_t(0x0B000001)}) {
            Gfx call{gsSPDisplayList(address)}; Gfx* pc=&call; g_exec_stack={};
            assert(!gfx_dl_handler_common(&interpreter,&pc));
            assert(g_exec_stack.calls==0 && g_exec_stack.branches==0);
        }
#endif
    }
    auto draw=[&] {
        static Gfx opa[8],xlu[8]; gfx.polyOpa.p=opa;gfx.polyXlu.p=xlu;used=0;func_8009EE44(&play);
    };
    play.roomCtx.unk_74[0]=0;play.roomCtx.unk_74[1]=49;draw();
    assert(play.roomCtx.unk_74[1]==49 && chickenCries==0);
    gSaveContext.inventory.items[0]=ITEM_COJIRO;draw();
    assert(play.roomCtx.unk_74[1]==50 && chickenCries==0);draw();draw();
    assert(play.roomCtx.unk_74[0]==1 && play.roomCtx.unk_74[1]==51 && chickenCries==1);
    gSaveContext.ship.quest.id=QUEST_RANDOMIZER;shuffleTrade=true;hasTradeFlag=false;
    play.roomCtx.unk_74[0]=0;play.roomCtx.unk_74[1]=50;draw();
    assert(play.roomCtx.unk_74[0]==0 && chickenCries==1);
    gSaveContext.inventory.items[0]=ITEM_NONE;hasTradeFlag=true;draw();
    assert(play.roomCtx.unk_74[0]==1 && chickenCries==2);
#ifdef COMBO_BUILD
    puts("PASS Combo Lost Woods vertical XLU0A: both native tiles, wrap/interpolation, flagged dispatch, original08/09/OPA and Cojiro preservation");
#else
    puts("PASS normal Lost Woods: only native08/09, unchanged OPA/XLU and Cojiro behavior");
#endif
}
'''.replace("/* DRAW */", bodies).replace("/* DISPATCH */", dispatch)
    includes = ["soh", "soh/include", "soh/include/PR", "soh/src", "soh/assets", "soh/soh", "combo",
                "libultraship/include"]
    with tempfile.TemporaryDirectory(prefix="lost-woods-vertical-") as temporary:
        source = Path(temporary) / "test.cpp"
        source.write_text(fixture)
        # Check the protected normal build before the new Combo-only behavior.
        for combo in (False, True):
            binary = Path(temporary) / ("combo" if combo else "normal")
            command = [os.environ.get("CXX", "c++"), "-std=c++20", "-fpermissive", "-w", "-DF3DEX_GBI_2",
                       "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie",
                       *(["-DCOMBO_BUILD"] if combo else []), *["-I" + str(ROOT / p) for p in includes],
                       str(source), "-o", str(binary)]
            subprocess.run(command, check=True)
            subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})


if __name__ == "__main__":
    main()
