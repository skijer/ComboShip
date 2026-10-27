#!/usr/bin/env python3
"""Execute the production pause-doll command graph for opaque/translucent limbs."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    start = re.search(r"(?:static )?(?:void|Gfx\*) " + name + r"\([^;]*?\)\s*\{", source).start()
    opening = source.index("{", start)
    end, depth = opening + 1, 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def fixture():
    source = (ROOT / "mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_equipment.c").read_text()
    defines = "\n".join(re.findall(r"^#define EQUIP_DOLL_[^\n]*", source, re.M))
    rcp = (ROOT / "mm/src/code/z_rcp.c").read_text()
    setups = [re.search(r"/\* SETUPDL_" + str(index) + r" \*/.*?gsSPEndDisplayList\(\),", rcp, re.S).group()
              for index in (25, 26)]
    code = r'''
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cstring>
using u16=uint16_t;using f32=float;
struct Vec3s { int x,y,z; };
struct Vp { int values[8]; };
struct Lights1 {};
struct Mtx { int id; };
enum Op { State,SetFb,ResetFb,Call,Branch,End,Projection,Viewport,Scissor,Marker,Render,Combine,Geometry };
struct Gfx { Op op=State;Gfx* target=nullptr;int value=0; };
struct GraphicsContext {};
struct Player { struct { void** skeleton;void* jointTable;int dListCount; } skelAnime;
    int transformation;struct { struct { int face; } shape; } actor; };
struct PlayState { struct { GraphicsContext* gfxCtx; } state; };
static Player player;static Player* livePlayer=&player;
static Gfx opa[1024],xlu[1024],work[1024],*POLY_OPA_DISP=opa,*POLY_XLU_DISP=xlu,*WORK_DISP=work;
static Mtx matrices[8];static int matrixIndex,matrixDepth,creates;static bool mario,emitOpaque=true,emitXlu=true;
static int gPauseLinkFrameBuffer=7;
static Gfx gCullBackDList[1];
#define GET_PLAYER(play) livePlayer
#define OPEN_DISPS(ctx) ((void)(ctx))
#define CLOSE_DISPS(ctx) ((void)(ctx))
#define GRAPH_ALLOC(ctx,size) (&matrices[matrixIndex++])
#define gdSPDefLights1(...) {}
#define G_MAXZ 65535
#define G_SC_NON_INTERLACE 0
#define G_CYC_FILL 0
#define G_CYC_2CYCLE 0
#define G_RM_NOOP 0
#define G_RM_NOOP2 0
#define GPACK_RGBA5551(...) 0
#define G_ZBUFFER 1
#define G_SHADE 2
#define G_CULL_BACK 4
#define G_LIGHTING 8
#define G_SHADING_SMOOTH 16
#define G_FOG 32
#define G_TX_RENDERTILE 0
#define G_ON 1
#define G_MTX_NOPUSH 0
#define G_MTX_LOAD 1
#define G_MTX_MUL 2
#define G_MTX_PROJECTION 4
#define MTXMODE_APPLY 1
#define PLAYER_FORM_GORON 0
#define PLAYER_FORM_ZORA 1
#define PLAYER_FORM_DEKU 2
static void Command(Gfx* p,Op op,int value=0,Gfx* target=nullptr) { *p={op,target,value}; }
#define gsSPSetFB(p,fb) Command(p,SetFb,fb)
#define gsSPResetFB(p) Command(p,ResetFb)
#define gSPDisplayList(p,list) Command(p,Call,0,list)
#define gSPBranchList(p,list) Command(p,Branch,0,list)
#define gSPEndDisplayList(p) Command(p,End)
#define gSPMatrix(p,mtx,flags) Command(p,Projection,(mtx)->id)
#define gSPViewport(p,vp) Command(p,Viewport)
#define gDPSetScissor(p,...) Command(p,Scissor)
#define gDPPipeSync(p) Command(p,State)
#define gDPSetCycleType(p,...) Command(p,State)
#define gDPSetRenderMode(p,a,b) Command(p,Render,(a)|(b))
#define gDPSetFillColor(p,...) Command(p,State)
#define gDPFillRectangle(p,...) Command(p,State)
#define gSPLoadGeometryMode(p,mode) Command(p,Geometry,mode)
#define gSPTexture(p,...) Command(p,State)
#define gSPPerspNormalize(p,...) Command(p,State)
#define gSPSetLights1(p,...) Command(p,State)
#define gSPSegment(p,...) Command(p,State)
int gfx_create_framebuffer(int,int,int,int,bool) { ++creates;return 7; }
bool Sm64Kaleido_DrawForm(PlayState*) { return mario; }
void guPerspective(Mtx* m,u16*,...) { m->id=1; }
void guLookAt(Mtx* m,...) { m->id=2; }
void Matrix_Push() { ++matrixDepth; }void Matrix_Pop() { --matrixDepth; }
void Matrix_SetTranslateRotateYXZ(float,float,float,Vec3s*) {}
void Matrix_Scale(float,float,float,int) {}
void Player_OverrideLimbDrawGameplayDefault() {}void Player_PostLimbDrawGameplay() {}
void PlayerTunic_BindLocalColor(PlayState*) {
    Command(POLY_OPA_DISP++,State);Command(POLY_XLU_DISP++,State);
}
// Only the loaded skeleton boundary is replaced. Its real callbacks can emit
// on both streams (notably Divine/Kite shields and replacement-model effects).
void Player_DrawImpl(PlayState*,...) {
    if(emitOpaque)Command(POLY_OPA_DISP++,Marker,1);
    if(emitXlu)Command(POLY_XLU_DISP++,Marker,2);
}
'''
    # Run the native player's real setup display list and wrappers. Only GPU
    # encoding is replaced; a missing setup leaves the clear's NOOP mode active.
    known = set(re.findall(r"^#define (G_\w+)", code, re.M))
    values = {"G_RM_AA_ZB_OPA_SURF2": 64, "G_RM_FOG_SHADE_A": 128}
    for token in sorted(set(re.findall(r"\bG_\w+", "\n".join(setups))) - known):
        code += f"#define {token} {values.get(token, 0)}\n"
    code += r'''
#define SETUPDL_25 0
#define SETUPDL_26 1
#define gsDPPipeSync() Gfx{State,nullptr,0}
#define gsSPTexture(...) Gfx{State,nullptr,0}
#define gsDPSetCombineMode(...) Gfx{Combine,nullptr,1}
#define gsDPSetOtherMode(a,b) Gfx{Render,nullptr,(b)}
#define gsSPLoadGeometryMode(mode) Gfx{Geometry,nullptr,mode}
#define gsSPEndDisplayList() Gfx{End,nullptr,0}
static Gfx gSetupDLs[][6] = {{
''' + "\n},{\n".join(setups) + "\n}};\n"
    code += function(rcp, "Gfx_SetupDL25_Opa") + "\n" + function(rcp, "Gfx_SetupDL25_Xlu") + "\n"
    code += function(rcp, "Gfx_SetupDL26") + "\n"
    return code + defines + "\n" + function(source, "KaleidoEquip_RenderDollFB") + r'''
struct Draw { int marker,fb,projection,render,geometry;bool viewport,combine; };
static std::vector<Draw> draws;static int activeFb,projection,renderMode,geometryMode;static bool viewport,combine;
static void Execute(Gfx* commands,int depth=0) {
    if(depth>8) { std::fprintf(stderr,"command recursion exceeded\n");std::abort(); }
    for(int count=0;count<4096;++count,++commands) {
        switch(commands->op) {
        case End:return;
        case SetFb:activeFb=commands->value;break;
        case ResetFb:activeFb=0;break;
        case Call:Execute(commands->target,depth+1);break;
        case Branch:commands=commands->target-1;break;
        case Projection:projection=commands->value;break;
        case Viewport:viewport=true;break;
        case Render:renderMode=commands->value;break;
        case Combine:combine=true;break;
        case Geometry:geometryMode=commands->value;break;
        case Marker:draws.push_back({commands->value,activeFb,projection,renderMode,geometryMode,viewport,combine});break;
        default:break;
        }
    }
    std::fprintf(stderr,"unterminated command graph\n");std::abort();
}
static int checks,failures;
static void check(bool ok,const char* message) { ++checks;if(!ok){++failures;std::fprintf(stderr,"FAIL: %s\n",message);} }
static void Reset() {
    for(auto& c:opa)c={};for(auto& c:xlu)c={};for(auto& c:work)c={};
    POLY_OPA_DISP=opa;POLY_XLU_DISP=xlu;WORK_DISP=work;matrixIndex=matrixDepth=0;
    draws.clear();activeFb=projection=renderMode=geometryMode=0;viewport=combine=false;livePlayer=&player;mario=false;
    player.skelAnime.skeleton=reinterpret_cast<void**>(&player);gPauseLinkFrameBuffer=7;
}
int main() {
    GraphicsContext context;PlayState play;play.state.gfxCtx=&context;
    for(int form=0;form<5;++form)for(int channels=1;channels<4;++channels) {
        Reset();player.transformation=form;emitOpaque=channels&1;emitXlu=channels&2;
        KaleidoEquip_RenderDollFB(&play);
        Command(WORK_DISP,End);Command(POLY_OPA_DISP,End);Command(POLY_XLU_DISP,End);
        Execute(work);
        const int expected=bool(emitOpaque)+bool(emitXlu);
        check((int)draws.size()==expected,"every model channel executes exactly once inside preview");
        for(const auto& draw:draws) {
            check(draw.fb==7&&draw.projection==2&&draw.viewport,"model/transparent equipment draws inside preview camera and framebuffer");
            check((draw.render&64)&&draw.combine,"model draw restores native player render/combine state after framebuffer clear");
            check(!(draw.render&128)&&!(draw.geometry&32),"preview uses matched no-fog blender and geometry state");
        }
        check(activeFb==0,"preview restores main framebuffer");
        const auto before=draws.size();projection=99;viewport=false;
        Execute(opa);Execute(xlu);
        check(draws.size()==before,"main OPA/XLU passes skip all preview commands");
        check(matrixDepth==0,"preview balances CPU matrix stack");
    }
    Reset();livePlayer=nullptr;KaleidoEquip_RenderDollFB(&play);
    check(WORK_DISP==work&&POLY_OPA_DISP==opa&&POLY_XLU_DISP==xlu,"missing player emits no preview commands");
    Reset();player.skelAnime.skeleton=nullptr;KaleidoEquip_RenderDollFB(&play);
    check(WORK_DISP==work&&POLY_OPA_DISP==opa&&POLY_XLU_DISP==xlu,"missing skeleton emits no preview commands");
    Reset();gPauseLinkFrameBuffer=-1;mario=true;KaleidoEquip_RenderDollFB(&play);
    check(creates==1&&gPauseLinkFrameBuffer==7,"framebuffer created once before delegated Mario preview");
    check(WORK_DISP==work&&POLY_OPA_DISP==opa&&POLY_XLU_DISP==xlu,"delegated Mario preview emits no Link commands");
    std::printf("MM production doll stream checks: %d, failures: %d\n",checks,failures);
    return failures?1:0;
}
'''


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="doll-streams-") as folder:
        cpp, binary = Path(folder) / "fixture.cpp", Path(folder) / "fixture"
        cpp.write_text(fixture())
        command = [os.environ.get("CXX", "c++"), "-std=c++20", str(cpp), "-o", str(binary)]
        if args.sanitize:
            command += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-g"]
        subprocess.run(command, check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
