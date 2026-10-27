#!/usr/bin/env python3
"""Execute MM page transitions and the production equipment-preview composite.

The equipment page's two draw dispatch blocks, input handler, transition update,
equipment draw/cursor and framebuffer composite are taken from production. GPU
commands are recorded at their boundary; a small perspective projection observes
the vertices under the production page matrix and transition eye positions.
The inherited ownership fixture keeps optional sword enhancements disabled;
scripts/diagnostics/sword_element_kaleido_tests.py covers the live chooser.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

import run_ownership_tests as ownership
import run_doll_stream_tests as doll_streams

ROOT = Path(__file__).resolve().parents[2]
FOLDER = ROOT / "mm/src/overlays/kaleido_scope/ovl_kaleido_scope"


def gpu_fixture():
    source = (ROOT / "libultraship/src/fast/interpreter.cpp").read_text()
    code = r'''
#define G_IM_SIZ_4b 0
#define G_IM_SIZ_8b 1
#undef G_IM_SIZ_16b
#define G_IM_SIZ_16b 2
#define G_IM_SIZ_32b 3
#define G_IM_SIZ_16b_LINE_BYTES 2
#define G_TEXTURE_IMAGE_FRAC 2
#define G_TX_LOADTILE 7
#define G_TX_WRAP 0
#define G_MDSFT_TEXTFILT 12
#define SUPPORT_CHECK(x) check(x,"production load-tile contract")
struct ResourceStub { struct Init { std::string Path; }; Init* GetInitData() { return nullptr; } };
struct RawTexMetadata { float h_byte_scale=1,v_pixel_scale=1;std::shared_ptr<ResourceStub> resource;uint8_t autumnForestLayer=0,zoraCosmeticLayer=0; };
struct TextureTile { uint32_t palette=0,fmt=0,siz=2,cms=0,cmt=0,masks=0,maskt=0,shifts=0,shiftt=0,line_size_bytes=64,tmem=0,tmem_index=0,uls=0,ult=0,lrs=124,lrt=124; };
struct LoadedTexture { uint32_t line_size_bytes=64,size_bytes=2048,full_image_line_size_bytes=64,orig_size_bytes=2048,tex_flags=0;RawTexMetadata raw_tex_metadata;const uint8_t* addr=nullptr;bool masked=false,blended=false; };
struct Rdp { TextureTile texture_tile[8];LoadedTexture loaded_texture[2];struct { const uint8_t* addr=nullptr;uint32_t siz=2,width=32,tex_flags=0;RawTexMetadata raw_tex_metadata; } texture_to_load;bool textures_changed[2]={true,true};uint32_t other_mode_h=0; };
struct Backend { int binds=0,lastFb=-1;void SelectTextureFb(uint32_t id) { ++binds;lastFb=id; } };
struct F3DGfx { struct { uintptr_t w0=0,w1=0; } words; };
struct MaskedTexture { const void* replacementData=nullptr; };
class Interpreter {
  public:
    Rdp state;Rdp* mRdp=&state;Backend backend;Backend* mRapi=&backend;
    const ResourceStub* mPreludeMaterialBoundResources[2]={};
    std::map<std::string_view,MaskedTexture> mMaskedTextures;
    void Flush() {}
    void GfxDpSetTextureImage(uint32_t,uint32_t,uint32_t,const char*,uint32_t,RawTexMetadata,const void*);
    void GfxDpSetTile(uint8_t,uint32_t,uint32_t,uint32_t,uint8_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t);
    void GfxDpLoadTile(uint8_t,uint32_t,uint32_t,uint32_t,uint32_t);
};
static std::string_view GetBaseTexturePath(const std::string& path) { return path; }
'''
    for name in ("AutumnForestLayer", "ZoraCosmeticLayer", "Interpreter::GfxDpSetTextureImage", "Interpreter::GfxDpSetTile", "Interpreter::GfxDpLoadTile",
                 "gfx_set_timg_fb_handler_custom"):
        code += ownership.production_function(source, name) + "\n"
    dimension_start = source.index("uint32_t loaded_line_size =")
    dimensions = source[dimension_start:source.index("tex_width[i] = line_size;", dimension_start) + len("tex_width[i] = line_size;")]
    uv_start = source.index("            u -= mRdp->texture_tile[uv_tile].uls / 4.0f;")
    uvs = source[uv_start:source.index("            bool clampS", uv_start)]
    code += "static Interpreter interpreter;\nstatic std::array<float,2> NormalizedUV(float u,float v) { auto* mRdp=interpreter.mRdp;int tile=0,i=0,t=0,uv_tile=0;uint32_t tex_width[2]={},tex_height[2]={};bool is_rect=false;float mBufVbo[2]={};int mBufVboLen=0;\n"
    code += dimensions + "\n" + uvs + "\nreturn {mBufVbo[0],mBufVbo[1]}; }\n"
    code += r'''
static int cpuImports;
static void BindFramebuffer(int fb) { F3DGfx command;command.words.w1=fb;auto* pointer=&command;gfx_set_timg_fb_handler_custom(&interpreter,&pointer); }
static void SetPreviewTile(int width,int height,int cms,int cmt) {
    interpreter.GfxDpSetTile(0,2,width*2/8,0,0,0,cmt,0,0,cms,0,0);
    interpreter.GfxDpSetTile(0,2,width*2/8,0,7,0,cmt,0,0,cms,0,0);
    interpreter.mRdp->texture_tile[0].lrs=(width-1)*4;interpreter.mRdp->texture_tile[0].lrt=(height-1)*4;
}
'''
    return code


def block(source, start):
    opening = source.index("{", start)
    cursor, depth = opening + 1, 1
    while depth:
        depth += (source[cursor] == "{") - (source[cursor] == "}")
        cursor += 1
    return source[start:cursor]


def fixture():
    pause = (FOLDER / "z_kaleido_equipment.c").read_text()
    scope = (FOLDER / "z_kaleido_scope_NES.c").read_text()
    code = ownership.fixture().split("int main() {")[0]
    code = code.replace("#include <cstdio>", "#include <cstdio>\n#include <cmath>\n#include <array>\n#include <vector>")
    code = code.replace("struct Gfx {};\nstruct Vtx {};", r'''
using f32 = float;
using u64 = uint64_t;
struct Gfx {};
struct Mtx {};
struct Vec3f { f32 x=0,y=0,z=0; };
struct Vtx { struct { s16 ob[3]; u16 flag; s16 tc[2]; u8 cn[4]; } v; };
''')
    code = code.replace("struct Input { struct { u16 button; } press = {}; };",
                        "struct Input { struct { u16 button; } press = {},cur = {}; };")
    code = code.replace("u8 alpha = 255, itemDescriptionOn = 0;", r'''
    u8 alpha = 255, itemDescriptionOn = 0,debugEditor=0;
    u16 nextPageMode=0,switchPageTimer=0;
    s16 pageSwitchInputTimer=0;
    f32 maskPageRoll=0;
    Vec3f eye;
    Vtx maskPageVtx[60]={};
''')
    code = code.replace("struct PlayState { struct { void* gfxCtx = nullptr; Input input; } state; PauseContext pauseCtx; };",
                        "struct PlayState { struct { void* gfxCtx = nullptr; Input input; } state; PauseContext pauseCtx; struct { int aButtonDoActionDelayed=0; } interfaceCtx; };\nusing InterfaceContext = decltype(PlayState::interfaceCtx);")
    code = code.replace("} gSaveContext;", "u8 buttonStatus[5]={},hudVisibility=0; struct { struct { u8 status[4]={}; } dpad; } shipSaveContext; } gSaveContext;")
    code = code.replace("int BrokenItems_Enabled() { return 0; }", "int BrokenItems_Enabled() { return 1; }")
    code = code.replace("int BrokenItems_FormCount() { return 0; }", "int BrokenItems_FormCount() { return 3; }")
    code = code.replace("void KaleidoEquip_DrawDollImage(PlayState*) {}", "static void KaleidoEquip_DrawDollImage(PlayState*);")
    code = code.replace("static Gfx* POLY_OPA_DISP;", "static Gfx opaCommands[4096],overlayCommands[4096];\nstatic Gfx* POLY_OPA_DISP=opaCommands;\nstatic Gfx* OVERLAY_DISP=overlayCommands;")
    # Install the GPU boundary before the production equipment draw function.
    position = code.index("void KaleidoScope_DrawEquipment(")
    code = code[:position] + r'''
#define PAUSE_ITEM 0
#define PAUSE_MAP 1
#define PAUSE_QUEST 2
#define PAUSE_MAIN_STATE_SWITCHING_PAGE 1
#define BTN_R 64
#define BTN_Z 128
#define BTN_DRIGHT 256
#define BTN_DLEFT 512
#define DEBUG_EDITOR_NONE 0
#define DEBUG_EDITOR_INVENTORY_INIT 1
#define DO_ACTION_DECIDE 1
#define DO_ACTION_INFO 2
#define VB_KALEIDO_SWITCH_PAGE_WITH_DPAD 1
#define BTN_ENABLED 1
#define BTN_DISABLED 0
#define EQUIP_SLOT_A 4
#define EQUIP_SLOT_D_RIGHT 0
#define EQUIP_SLOT_D_LEFT 1
#define EQUIP_SLOT_D_DOWN 2
#define EQUIP_SLOT_D_UP 3
#define HUD_VISIBILITY_IDLE 0
#define HUD_VISIBILITY_ALL 1
#define NA_SE_SY_WIN_SCROLL_LEFT 4
#define NA_SE_SY_WIN_SCROLL_RIGHT 5
#define NA_SE_SY_HP_RECOVER 6
#define TRUNCF_BINANG(x) ((s16)(x))
#define MTXMODE_NEW 0
#define MTXMODE_APPLY 1
#define G_MTX_NOPUSH 0
#define G_MTX_LOAD 1
#define G_MTX_MODELVIEW 2
#define G_SC_NON_INTERLACE 0
#define G_TX_RENDERTILE 0
#define G_IM_FMT_RGBA 0
#define G_IM_SIZ_16b 0
#define G_TX_NOMIRROR 0
#define G_TX_CLAMP 1
#define G_TX_NOMASK 0
#define G_TX_NOLOD 0
#define G_TF_POINT 0
#define G_CC_MODULATEIA_PRIM 0
#define G_AD_NOISE 0
#define G_CD_NOISE 0
#define G_CK_NONE 0
#define G_TC_FILT 0
#define G_TT_NONE 0
#define G_TL_TILE 0
#define G_TD_CLAMP 0
#define G_TP_NONE 0
#define G_CYC_1CYCLE 0
#define G_PM_NPRIMITIVE 0
#define G_AC_NONE 0
#define G_ZS_PRIM 0
#define G_RM_CLD_SURF 0
#define G_RM_CLD_SURF2 0
#define G_CULL_BOTH 0
#define G_FOG 0
#define G_LIGHTING 0
#define G_TEXTURE_GEN 0
#define G_TEXTURE_GEN_LINEAR 0
#define G_ZBUFFER 1
#define G_SHADE 2
#define G_SHADING_SMOOTH 4
Mtx gIdentityMtx;
int gPauseLinkFrameBuffer=7;
static f32 sPauseMenuVerticalOffset=0,sPauseCursorLeftMoveOffsetX=40,sPauseCursorRightMoveOffsetX=40;
static s16 sPauseCursorLeftX=0,sPauseCursorRightX=0;
int CVarGetInteger(const char*,int fallback) { return fallback; }
bool GameInteractor_Should(int,bool fallback,int) { return fallback; }
void Interface_SetAButtonDoAction(PlayState*,int) {}
void Interface_SetHudVisibility(int) {}
void Audio_PlaySfx(int,int) {}
void GameInteractor_ExecuteBeforeKaleidoDrawPage(PauseContext*,int) {}
void GameInteractor_ExecuteAfterKaleidoDrawPage(PauseContext*,int) {}
void* KaleidoNei_GetMaskPageBgTextures() { return nullptr; }
Gfx* KaleidoScope_DrawPageSections(Gfx* gfx,Vtx*,void*) { return gfx; }

using Matrix = std::array<std::array<double,4>,4>;
static Matrix Identity() { Matrix m={}; for(int i=0;i<4;i++)m[i][i]=1;return m; }
static Matrix currentMatrix=Identity(),loadedMatrix=Identity();
static void Apply(const Matrix& b) {
    Matrix result={};for(int r=0;r<4;r++)for(int c=0;c<4;c++)for(int k=0;k<4;k++)result[r][c]+=currentMatrix[r][k]*b[k][c];
    currentMatrix=result;
}
void Matrix_RotateYF(float angle,int mode) { if(mode==MTXMODE_NEW)currentMatrix=Identity();Matrix b=Identity();b[0][0]=b[2][2]=std::cos(angle);b[0][2]=std::sin(angle);b[2][0]=-std::sin(angle);Apply(b); }
void Matrix_Translate(float x,float y,float z,int mode) { if(mode==MTXMODE_NEW)currentMatrix=Identity();Matrix b=Identity();b[0][3]=x;b[1][3]=y;b[2][3]=z;Apply(b); }
void Matrix_Scale(float x,float y,float z,int) { Matrix b=Identity();b[0][0]=x;b[1][1]=y;b[2][2]=z;Apply(b); }
void Matrix_RotateXFApply(float angle) { Matrix b=Identity();b[1][1]=b[2][2]=std::cos(angle);b[1][2]=-std::sin(angle);b[2][1]=std::sin(angle);Apply(b); }
#define MATRIX_FINALIZE_AND_LOAD(gfx,ctx) ((void)(gfx),loadedMatrix=currentMatrix)

struct Composite { bool quad=false,opa=false;int fb=-1,alpha=0,width=0,height=0;Vtx vertices[4]={};double x0=0,y0=0,x1=0,y1=0; };
// GPU_PRODUCTION_BOUNDARY
static Composite composite;
static Vtx* sampledVertices=nullptr;
static u8 previewAlpha;
static int tileWidth,tileHeight,previewFb,changedMatrix,changedScissor;
static PlayState* drawingPlay;
static Vtx previewStorage[4];
static bool IsOpa(Gfx* gfx) { return gfx>=opaCommands&&gfx<opaCommands+4096; }
static std::array<double,2> Project(const Vtx& v) {
    double world[3]={};for(int r=0;r<3;r++){world[r]=loadedMatrix[r][3];for(int c=0;c<3;c++)world[r]+=loadedMatrix[r][c]*v.v.ob[c];}
    auto eye=drawingPlay->pauseCtx.eye;double distance=std::hypot(eye.x,eye.z);
    double forwardX=-eye.x/distance,forwardZ=-eye.z/distance;
    double dx=world[0]-eye.x,dz=world[2]-eye.z;
    double depth=dx*forwardX+dz*forwardZ;
    double right=-forwardZ*dx+forwardX*dz;
    double focal=120.0/std::tan(3.141592653589793/6.0);
    return {160+focal*right/depth,120-focal*world[1]/depth};
}
static void RecordPreviewQuad(Gfx* gfx,int a,int b,int c,int d) {
    composite.quad=true;composite.opa=IsOpa(gfx);composite.fb=previewFb;composite.alpha=previewAlpha;composite.width=tileWidth;composite.height=tileHeight;
    for(int i=0;i<4;i++)composite.vertices[i]=sampledVertices[i];
    auto top=Project(composite.vertices[a]);auto bottom=Project(composite.vertices[c]);
    composite.x0=top[0];composite.y0=top[1];composite.x1=bottom[0];composite.y1=bottom[1];
    if(interpreter.mRdp->textures_changed[0])++cpuImports;
}
static void RecordPreviewRectangle(Gfx* gfx,int x0,int y0,int,int,int x1,int y1,int width,int height,int,int,int) {
    composite.quad=false;composite.opa=IsOpa(gfx);composite.fb=previewFb;composite.alpha=previewAlpha;composite.width=width;composite.height=height;
    composite.x0=x0/4.0;composite.y0=y0/4.0;composite.x1=x1/4.0;composite.y1=y1/4.0;
}
#undef gSPVertex
#undef gDPSetPrimColor
#define gSPVertex(gfx,vertex,count,offset) ((void)(gfx),sampledVertices=(vertex),RecordVertex(vertex))
#define gDPSetPrimColor(gfx,min,lod,r,g,b,a) ((void)(gfx),previewAlpha=(a))
#define gDPSetEnvColor(gfx,r,g,b,a) ((void)(gfx),previewAlpha=(a))
#define gDPSetTextureImageFB(gfx,format,size,width,fb) ((void)(gfx),previewFb=(fb),BindFramebuffer(fb))
#define gDPSetTextureImage(gfx,format,size,width,source) ((void)(gfx),interpreter.GfxDpSetTextureImage(format,size,width,nullptr,0,{},source))
#define gDPLoadTile(gfx,tile,uls,ult,lrs,lrt) ((void)(gfx),interpreter.GfxDpLoadTile(tile,uls,ult,lrs,lrt))
#define gDPLoadSync(...) ((void)0)
#define gDPImageRectangle(gfx,...) RecordPreviewRectangle(gfx,__VA_ARGS__)
#define gSP1Quadrangle(gfx,a,b,c,d,flag) RecordPreviewQuad(gfx,a,b,c,d)
#define gDPSetTileCustom(gfx,format,size,width,height,pal,cms,cmt,...) ((void)(gfx),tileWidth=(width),tileHeight=(height),SetPreviewTile(width,height,cms,cmt))
#define gSPMatrix(gfx,...) ((void)(gfx),++changedMatrix)
#define gDPSetScissor(...) (++changedScissor)
#define gDPSetOtherMode(...) ((void)0)
#define gSPClearGeometryMode(...) ((void)0)
#define gSPSetGeometryMode(...) ((void)0)
#define gDPSetCombineLERP(...) ((void)0)
#define gDPSetTextureFilter(...) ((void)0)
#define gDPPipeSync(...) ((void)0)
#define GRAPH_ALLOC(ctx,size) (previewStorage)
''' + code[position:]
    code = code.replace("// GPU_PRODUCTION_BOUNDARY", gpu_fixture())
    # The external cosmetic draw receiver now wraps the primitive GPU command.
    # Preserve the exact native macro/declaration while recording its RGBA at
    # the same boundary as the fixture's other primitive-color submissions.
    cosmetic = (ROOT / "mm/2s2h/BenGui/CosmeticEditor.h").read_text()
    cosmetic_id = re.search(r"^#define COSMETIC_ID[^\n]*", cosmetic, re.M).group(0)
    declaration = re.search(r"void gDPSetPrimColorOverride\([^;]+;", cosmetic, re.S).group(0)
    receiver = declaration[:-1] + " { (void)m; (void)l; (void)cosmeticId; gDPSetPrimColor(pkt,m,l,r,g,b,a); }\n"
    position = code.index("void KaleidoScope_DrawEquipment(")
    code = code[:position] + cosmetic_id + "\n" + receiver + code[position:]
    for prefix in ("EQUIP_DOLL_",):
        code += ownership.selectors.defines(pause, prefix)
    code += ownership.production_function(pause, "KaleidoEquip_DrawDollImage") + "\n"
    constants = (FOLDER / "z_kaleido_scope.h").read_text()
    for name in ("PAUSE_EYE_DIST", "PAUSE_ITEM_X", "PAUSE_ITEM_Z", "PAUSE_MAP_X", "PAUSE_MAP_Z",
                 "PAUSE_QUEST_X", "PAUSE_QUEST_Z", "PAUSE_MASK_X", "PAUSE_MASK_Z"):
        code += re.search(r"^#define " + name + r"[^\n]*", constants, re.M).group(0) + "\n"
    code += ownership.selectors.defines(scope, "SWITCH_PAGE_")
    for name in ("sPageSwitchEyeDx", "sPageSwitchEyeDz", "sPageSwitchNextPageIndex"):
        code += re.search(r"(?:f32|u16) " + name + r"\[\] = \{.*?\};", scope, re.S).group(0) + "\n"
    for name in ("KaleidoScope_SwitchPage", "KaleidoScope_HandlePageToggles", "KaleidoScope_UpdateSwitchPage"):
        code += ownership.production_function(scope, name) + "\n"
    draw = ownership.production_function(scope, "KaleidoScope_DrawPages")
    inactive = block(draw, draw.index("if ((pauseCtx->pageIndex != PAUSE_MASK) && (pauseCtx->pageIndex != PAUSE_MAP))"))
    active_start = draw.index("case PAUSE_MASK:")
    active = draw[active_start:draw.index("break;", active_start) + len("break;")]
    code += "void DrawPreviewPage(PlayState* play) { auto* pauseCtx=&play->pauseCtx; auto* gfxCtx=play->state.gfxCtx;\n" + inactive
    code += "\nswitch(pauseCtx->pageIndex) {" + active + "}\n}\n"
    code += r'''
static void DrawPreview(PlayState* play) {
    drawingPlay=play;composite={};changedMatrix=changedScissor=0;POLY_OPA_DISP=opaCommands;OVERLAY_DISP=overlayCommands;
    gridBegin=play->pauseCtx.maskVtx;DrawPreviewPage(play);
}
static void CheckComposite(PlayState* play,const char* label) {
    DrawPreview(play);check(composite.quad&&composite.opa&&composite.fb==7,label);
    check(changedMatrix==0&&changedScissor==0,"preview preserves caller modelview and scissor");
    if(composite.quad) {
        check(composite.width==192&&composite.height==336,"full-resolution framebuffer tile");
        check(composite.vertices[0].v.tc[0]==0&&composite.vertices[0].v.tc[1]==0&&
              composite.vertices[1].v.tc[0]==192*32&&composite.vertices[1].v.tc[1]==0&&
              composite.vertices[2].v.tc[0]==0&&composite.vertices[2].v.tc[1]==336*32&&
              composite.vertices[3].v.tc[0]==192*32&&composite.vertices[3].v.tc[1]==336*32,
              "preview UVs cover the complete framebuffer without mirrored layout");
        check(composite.alpha==play->pauseCtx.alpha,"preview follows pause alpha");
        check(composite.vertices[0].v.ob[2]==0&&composite.vertices[3].v.ob[2]==0,"preview lies on equipment page");
    }
}
int main() {
    PlayState play;play.pauseCtx.eye={64,0,0};save.capeOwned=1;
    CheckComposite(&play,"settled equipment preview uses page-owned quad");
    check(std::fabs(composite.x0-86)<1&&std::fabs(composite.y0-68)<1&&std::fabs(composite.x1-150)<1&&std::fabs(composite.y1-180)<1,
          "settled placement preserves the former 64x112 screen rectangle");
    play.pauseCtx.alpha=123;CheckComposite(&play,"partial alpha preview uses page-owned quad");play.pauseCtx.alpha=255;
    auto saved=save;auto equipped=MM_EQ.equipment;
    for(int cycle=0;cycle<3;cycle++) {
        play.state.input.press.button=BTN_L;KaleidoScope_HandlePageToggles(&play,&play.state.input);KaleidoScope_UpdateEquipmentCursor(&play);
        check(play.pauseCtx.mainState==PAUSE_MAIN_STATE_IDLE&&play.pauseCtx.pageIndex==PAUSE_MASK,"L cycles subpages without cube transition");
        check(sEquipSubPage==(cycle+1)%3,"L keeps vanilla/extended/form cycling");
        CheckComposite(&play,"every L subpage shares the page-owned preview");
    }
    check(std::memcmp(&save,&saved,sizeof(save))==0&&MM_EQ.equipment==equipped,"L preview checks preserve save and equipped items");
    for(int width:{16,32,64,128,192}) {
        auto& loaded=interpreter.mRdp->loaded_texture[0];loaded.line_size_bytes=loaded.full_image_line_size_bytes=width*4;
        loaded.size_bytes=loaded.orig_size_bytes=width*32*4;interpreter.mRdp->textures_changed[0]=true;
        ResourceStub precedingMaterial;
        interpreter.mPreludeMaterialBoundResources[0]=&precedingMaterial;
        interpreter.mPreludeMaterialBoundResources[1]=&precedingMaterial;
        DrawPreview(&play);
        auto origin=NormalizedUV(0,0);auto far=NormalizedUV(composite.vertices[3].v.tc[0]/32.f,composite.vertices[3].v.tc[1]/32.f);
        check(origin[0]==0&&origin[1]==0&&far[0]==1&&far[1]==1,"production triangle UV normalization ignores preceding icon dimensions");
        check(cpuImports==0&&!interpreter.mRdp->textures_changed[0]&&interpreter.backend.lastFb==7,"framebuffer bind suppresses placeholder CPU texture imports");
        check(!interpreter.mPreludeMaterialBoundResources[0]&&!interpreter.mPreludeMaterialBoundResources[1],
              "framebuffer binding clears remembered material resources in both slots");
    }
    for(u16 button:{(u16)BTN_Z,(u16)BTN_R}) {
        play.pauseCtx.pageIndex=PAUSE_MASK;play.pauseCtx.mainState=PAUSE_MAIN_STATE_IDLE;play.pauseCtx.eye={64,0,0};
        play.state.input.press.button=button;KaleidoScope_HandlePageToggles(&play,&play.state.input);
        check(play.pauseCtx.mainState==PAUSE_MAIN_STATE_SWITCHING_PAGE,"actual shoulder input starts cube transition");
        play.state.input.press.button=0;
        double previousX=86;int departing=0;
        for(int frame=0;frame<8;frame++) {
            if(play.pauseCtx.pageIndex==PAUSE_MASK) {
                CheckComposite(&play,"departing equipment preview remains attached during every transition frame");
                if(frame>0)check(std::fabs(composite.x0-previousX)>1,"departing preview moves with transition eye");
                previousX=composite.x0;++departing;
            }
            KaleidoScope_UpdateSwitchPage(&play,&play.state.input);
        }
        check(departing==8&&play.pauseCtx.mainState==PAUSE_MAIN_STATE_IDLE,"departure spans the real eight transition frames");
        DrawPreview(&play);
        check(composite.quad,"cached equipment preview is drawn on either adjacent off-axis page");
        play.state.input.press.button=(button==BTN_Z)?BTN_R:BTN_Z;KaleidoScope_HandlePageToggles(&play,&play.state.input);play.state.input.press.button=0;
        for(int frame=0;frame<8;frame++) {
            CheckComposite(&play,"cached incoming preview remains attached before equipment becomes active");
            KaleidoScope_UpdateSwitchPage(&play,&play.state.input);
        }
        check(play.pauseCtx.pageIndex==PAUSE_MASK&&play.pauseCtx.mainState==PAUSE_MAIN_STATE_IDLE,"return transition restores equipment page");
        CheckComposite(&play,"return to equipment restores settled preview");
    }
    check(std::memcmp(&save,&saved,sizeof(save))==0&&MM_EQ.equipment==equipped,"page transitions preserve save and equipped items");
    play.pauseCtx.pageIndex=PAUSE_MAP;DrawPreview(&play);check(composite.fb==-1,"nonadjacent equipment page is not submitted");play.pauseCtx.pageIndex=PAUSE_MASK;
    gPauseLinkFrameBuffer=-1;DrawPreview(&play);check(composite.fb==-1,"unavailable framebuffer emits no composite");
    std::printf("MM equipment preview transition: %d checks, %d failures\n",checks,failures);return failures?1:0;
}
'''
    return code


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="equipment-preview-") as folder:
        cpp, exe = Path(folder) / "preview.cpp", Path(folder) / "preview"
        cpp.write_text(fixture())
        command = [os.environ.get("CXX", "c++"), "-std=c++20", "-I" + str(ROOT), "-I" + str(ROOT / "mm"),
                   "-I" + str(ROOT / "combo"), "-I" + str(ROOT / "combo/menu"), "-I" + str(ROOT / "mm/assets"), str(cpp), "-o", str(exe)]
        if args.sanitize:
            command += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-g"]
        subprocess.run(command, check=True)
        subprocess.run([str(exe)], check=True)
    doll_streams.main()


if __name__ == "__main__":
    main()
