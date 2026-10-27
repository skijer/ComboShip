#!/usr/bin/env python3
"""Compile production C attribution sites and execute the real rain draw loop.

Platform matrix/display-list sinks are stubbed; production structs, signatures,
conditions, random calls and draw-loop bodies are retained. Disabled/enabled
captures must produce identical RNG and geometry call counts.
"""
from pathlib import Path
import re
import subprocess
import sys

root = Path(__file__).resolve().parents[2]
out = Path(sys.argv[1]).resolve()
out.mkdir(parents=True, exist_ok=True)
cc = sys.argv[2] if len(sys.argv) > 2 else "cc"
source = root / "soh/src/code/z_kankyo.c"
prefixes = sorted(set(re.findall(r"CVAR_PREFIX_\w+", (root / "soh/soh/cvar_prefixes.h").read_text())))
common = ["-std=gnu11", "-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=6"]
common += ['-D' + p + '="test"' for p in prefixes]
common += ["-I" + p for p in ["soh/assets", "soh/include", "soh/src", "soh/mods", "soh",
                              "libultraship/include", "ZAPDTR/ZAPD/resource/type", "combo",
                              "combo/menu", "combo/title"]]
for name in ["z_play", "z_actor", "z_kankyo"]:
    result = subprocess.run([cc, *common, "-fsyntax-only", "soh/src/code/" + name + ".c"],
                            cwd=root, capture_output=True, text=True)
    (out / (name + "_syntax.log")).write_text(result.stderr)
    if result.returncode:
        print(result.stderr, file=sys.stderr)
        result.check_returncode()
    print("PASS production C syntax:", name)

match = re.search(r"^void Environment_DrawRain\(.*?^}", source.read_text(), re.M | re.S)
assert match, "Environment_DrawRain definition missing"
(out / "rain_draw.inc").write_text(match[0])
fixture = r'''
#include "global.h"
#include "soh/Enhancements/debugger/FrameTimingProbe.h"
#include "soh/frame_interpolation.h"
#include "soh/Enhancements/audio/GlobalOutdoorRainBridge.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include <assert.h>
#include <string.h>
static int active, rngCalls, drops, splashes, phaseEnds;
SaveContext gSaveContext;
FrameTimingSpan FrameTiming_BeginSpan(void) { return (FrameTimingSpan){ active, 1 }; }
void FrameTiming_EndNamedSpan(const char* category, const char* name, FrameTimingSpan span) { if (span.epoch) phaseEnds++; }
void FrameTiming_Count(const char* name, uint64_t n) {
    if (!strcmp(name,"rain_drops_drawn")) drops += n;
    if (!strcmp(name,"rain_splashes_drawn")) splashes += n;
}
void GlobalOutdoorRain_RecordDraw(PlayState* p, int underwater, int snow, float eyeY, float waterY, float viewEyeY) {}
int32_t GlobalOutdoorRain_GetRenderColor(u8* r,u8* g,u8* b) { return 0; }
f32 Rand_ZeroOne(void) { rngCalls++; return 0.5f; }
f32 func_800746DC(void) { rngCalls++; return 0.25f; }
#include "rain_draw.inc"
static int children, displayLists;
void FrameInterpolation_RecordOpenChild(const void* a, int b) { children++; }
void FrameInterpolation_RecordCloseChild(void) { children--; assert(children >= 0); }
void Graph_OpenDisps(Gfx** refs, GraphicsContext* ctx, const char* file, s32 line) {}
void Graph_CloseDisps(Gfx** refs, GraphicsContext* ctx, const char* file, s32 line) {}
Gfx* Gfx_SetupDL(Gfx* gfx, u32 i) { return gfx; }
void Gfx_SetupDL_25Xlu(GraphicsContext* ctx) {}
f32 Math_Atan2F(f32 x, f32 y) { return atan2f(x,y); }
void Matrix_Translate(f32 x, f32 y, f32 z, u8 mode) {}
void Matrix_Scale(f32 x, f32 y, f32 z, u8 mode) {}
void Matrix_RotateX(f32 x, u8 mode) {}
void Matrix_RotateY(f32 x, u8 mode) {}
Mtx* Matrix_NewMtx(GraphicsContext* ctx, char* file, s32 line) { static Mtx matrix; return &matrix; }
void gSPDisplayList(Gfx* pkt, Gfx* dl) { displayLists++; }
static void CheckRain(int enabled, int underwater, int snow, int above, int count) {
    static PlayState play;
    static Player player;
    static Camera camera;
    static GraphicsContext gfx;
    static Gfx commands[4096];
    memset(&play,0,sizeof(play)); memset(&player,0,sizeof(player));
    memset(&camera,0,sizeof(camera)); memset(&gfx,0,sizeof(gfx));
    active=enabled; rngCalls=drops=splashes=phaseEnds=children=displayLists=0;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
    play.cameraPtrs[0]=&camera;
    play.envCtx.unk_EE[1]=count; play.envCtx.unk_EE[2]=snow;
    camera.unk_14C=underwater ? 0x100 : 0;
    play.view.eye.y = above ? 100.0f : -100.0f;
    play.view.lookAt.z=100.0f;
    gfx.polyXlu.p=commands;
    Environment_DrawRain(&play,&play.view,&gfx);
    int drawCount=(!underwater && !snow) ? count : 0;
    int splashCount=above ? drawCount : 0;
    assert(rngCalls==drawCount*4+splashCount*2);
    assert(displayLists==drawCount+splashCount);
    assert(drops==(enabled ? drawCount : 0));
    assert(splashes==(enabled ? splashCount : 0));
    assert(phaseEnds==(enabled ? ((!underwater && !snow) ? 4 : 1) : 0));
    assert(children==0);
}
int main(void) {
    for(int enabled=0;enabled<=1;enabled++) {
        CheckRain(enabled,0,0,1,7);
        CheckRain(enabled,0,0,0,7);
        CheckRain(enabled,1,0,1,7);
        CheckRain(enabled,0,1,1,7);
        CheckRain(enabled,0,0,1,0);
    }
    return 0;
}
'''
(out / "rain_draw_fixture.c").write_text(fixture)
subprocess.run([cc, *common, "-Wno-incompatible-pointer-types", "-I" + str(out),
                str(out / "rain_draw_fixture.c"), "-lm", "-o", str(out / "rain_draw_fixture")],
               cwd=root, check=True)
subprocess.run([str(out / "rain_draw_fixture")], check=True)
print("PASS actual rain draw: drops, splash suppression, underwater, snow, zero particles, disabled parity")
