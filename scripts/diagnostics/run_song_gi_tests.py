"""Execute the production native OoT note palette and MM song export.

GPU submission is captured at the command boundary; no runtime rasterization claim.
"""
import os
from pathlib import Path
import subprocess
import tempfile
from run_mm_scene_randomization_tests import function
from run_time_pedestal_tests import functions

ROOT = Path(__file__).resolve().parents[2]
oot = (ROOT / 'soh/src/code/z_draw.c').read_text()
mm = (ROOT / 'combo/menu/ComboItemDrawMM.h').read_text()
native = function(oot, 'GetItem_DrawGenericMusicNote')
native_shimmer = function(oot, 'GetItem_GetShimmerColor')
recipe = function(mm, 'MM_FillSongDrawInfo')
with tempfile.TemporaryDirectory(prefix='song-gi-') as temp:
    temp = Path(temp)
    source = r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "soh/include/z64item.h"
#include "combo/menu/ComboSongDraw.h"
#include "combo/menu/ComboBottleShimmer.h"
typedef int16_t s16; typedef int32_t s32; typedef uint8_t u8;
typedef struct PlayState { struct {void* gfxCtx;} state; } PlayState;
typedef struct {uint8_t r,g,b;} Color_RGB8;
static int notes, stormOverlay;
#define CVAR_COSMETIC(x) x
int CVarGetInteger(const char* key,int value) {return value;}
void Matrix_Push(void);
void Matrix_Pop(void);
void Matrix_Scale(float,float,float,int);
enum { MTXMODE_APPLY = 1 };
#include "combo/menu/ComboCollectibleShimmerHost.h"
void OOT_SampleCollectibleShimmerColor(int32_t profile,uint8_t color[4]) {
    assert(false && "song fixture unexpectedly sampled collectible colors");
}
Color_RGB8 CVarGetColor24(const char* key,Color_RGB8 value) {return value;}
int ComboOotMaskShimmerColor(int id,uint8_t* color) {return false;}
void ComboMaskShimmerColor(int id,uint8_t* color) {}
void NeiGi_DrawSeasonOverlay(PlayState* play,int profile,const char* owner) {(void)play;(void)profile;(void)owner;}
void NeiGi_DrawSongOverlay(PlayState* play,int song,const char* owner) {assert(song==CW_SONG_STORMS);++stormOverlay;}
static struct {void* dlists[3];} sDrawItemTable[256];
static uintptr_t tint[3];
#define OPEN_DISPS(x) ((void)0)
#define CLOSE_DISPS(x) ((void)0)
#define gSPMatrix(...) ((void)0)
#define gDPSetGrayscaleColor(p,r,g,b,a) (tint[0]=(uintptr_t)(r),tint[1]=(uintptr_t)(g),tint[2]=(uintptr_t)(b))
#define gSPGrayscale(...) ((void)0)
#define Gfx_SetupDL_25Opa(...) ((void)0)
#define gSPDisplayList(...) (++notes)
''' + native_shimmer + '\n' + native + r'''
int main(void) {
    PlayState play={0};
    const int ids[]={GID_SONG_MINUET,GID_SONG_BOLERO,GID_SONG_SERENADE,GID_SONG_REQUIEM,
                     GID_SONG_NOCTURNE,GID_SONG_PRELUDE,GID_SONG_EPONA,GID_SONG_SUN};
    const uint32_t hues[]={0x62FF62,0xFF3C00,0x55B4DF,0xDE9E2F,0xA028D2,0xEDE73E,0xD96E30,0xEDE73E};
    for(size_t i=0;i<sizeof(ids)/sizeof(ids[0]);++i) {
        uint8_t rgba[4];
        assert(GetItem_GetShimmerColor(ids[i],rgba) && "native table fallback must always shimmer for warp songs, Epona and Sun");
        assert(rgba[0]==(hues[i]>>16) && rgba[1]==((hues[i]>>8)&255) && rgba[2]==(hues[i]&255) && rgba[3]==255);
    }
    GetItem_DrawGenericMusicNote(&play,GID_SONG_TIME);
    assert(tint[0]==98 && tint[1]==177 && tint[2]==211);
    GetItem_DrawGenericMusicNote(&play,GID_SONG_GENERIC);
    assert(tint[0]==255 && tint[1]==255 && tint[2]==255);
    notes=stormOverlay=0;
    GetItem_DrawGenericMusicNote(&play,GID_SONG_STORM);
    assert(notes==1 && stormOverlay==1 && tint[0]==146 && tint[1]==146 && tint[2]==146 &&
           "Storms must restore its original clef and submit local rain");
}
'''
    path = temp / 'note.c'; path.write_text(source)
    subprocess.run([os.environ.get('CC', 'cc'), '-std=c11', '-Wno-int-conversion', '-I'+str(ROOT), str(path), '-o', str(temp/'note')], check=True)
    subprocess.run([str(temp/'note')], check=True)
    source = r'''
#include <cassert>
#include <cstring>
#include "combo/menu/ComboItemDrawABI.h"
#include "combo/menu/ComboSongDraw.h"
#include "mm/2s2h/Rando/Types.h"
#include "combo/menu/ComboSongDrawMM.h"
const char* gGiSongNoteDL="__OTR__objects/object_gi_melody/gGiSongNoteDL";
''' + recipe + r'''
int main() {
    for(int id=RI_UNKNOWN;id<RI_MAX;++id) {
        const int song=ComboSongForMmItem(id);
        if(song<0)continue;
        CwItemDrawInfo info{};
        assert(MM_FillSongDrawInfo(static_cast<RandoItemId>(id),&info)==1);
        const bool overlay=(song>=CW_SONG_OOT_MINUET && song<=CW_SONG_OOT_SARIA) ||
                           song==CW_SONG_EPONA || song==CW_SONG_SARIA || song==CW_SONG_STORMS || song==CW_SONG_SUN || song==CW_SONG_SOARING ||
                           song==CW_SONG_HEALING || song==CW_SONG_SONATA || song==CW_SONG_LULLABY_INTRO || song==CW_SONG_LULLABY ||
                           song==CW_SONG_NOVA || song==CW_SONG_ELEGY || song==CW_SONG_OATH || song==CW_SONG_INVERTED_TIME || song==CW_SONG_DOUBLE_TIME;
        assert(info.drawKind==CW_DRAW_KIND_SONG_GI && info.neiEffect==song && info.dlistCount==1 && info.hasEnvColor);
        assert(bool(info.itemShimmer)==overlay && "song shimmer eligibility must follow its intrinsic profile");
        assert(!memcmp(info.itemShimmerColor,info.envColor,4));
        if(song==CW_SONG_EPONA)assert(info.envColor[0]==217 && info.envColor[1]==110 && info.envColor[2]==48);
    }
}
'''
    path = temp / 'song.cpp'; path.write_text(source)
    subprocess.run([os.environ.get('CXX','c++'), '-std=c++20', '-I'+str(ROOT), str(path), '-o', str(temp/'song')], check=True)
    subprocess.run([str(temp/'song')], check=True)
    draw_song = functions((ROOT/'mm/2s2h/Rando/DrawItem.cpp').read_text())["DrawSong"]
    dispatcher = functions((ROOT/'mm/2s2h/Rando/NeiGiPresentation.cpp').read_text())["MM_TryDrawNeiGi"]
    source = r'''
#include <cassert>
#include <array>
#include <cstring>
#include <cstdint>
#include "combo/menu/ComboItemDrawABI.h"
#include "mm/2s2h/Rando/Types.h"
#include "combo/menu/ComboSongDrawMM.h"
#include "soh/soh/Enhancements/randomizer/NeiGiSongEffectPolicy.h"
#include "combo/menu/ComboElementalArrowGi.h"
using NeiGi::Kind;
bool ResourceMgr_IsAltAssetsEnabled() {return false;}
struct PlayState { struct {void* gfxCtx;} state; uint32_t gameplayFrames; } play{};
PlayState* gPlayState=&play;
static int overlay=-1, season=-1, notes=0, genericShimmer=0, descriptions=0;
static bool gray=true, grayAtNote=false;
static std::array<uint8_t,4> env{};
using Gfx=int;
const char* gGiSongNoteDL="note";
#define OPEN_DISPS(...) ((void)0)
#define CLOSE_DISPS(...) ((void)0)
#define Gfx_SetupDL25_Xlu(...) ((void)0)
#define MATRIX_FINALIZE_AND_LOAD(...) ((void)0)
#define gDPSetEnvColor(p,r,g,b,a) (env={uint8_t(r),uint8_t(g),uint8_t(b),uint8_t(a)})
#define gSPDisplayList(...) (++notes,grayAtNote=gray)
#define gSPGrayscale(p,state) (gray=state)
void NeiGi_DrawSeasonOverlay(PlayState*,int profile,const char*) {season=profile;}
void NeiGi_DrawSongOverlay(PlayState*,int song,const char*) {overlay=song;}
void ComboDrawMaskShimmer(PlayState*,void*,const uint8_t*,const char*) {++genericShimmer;}
bool MM_DescribeNeiGi(RandoItemId,CwItemDrawInfo*) {++descriptions;return false;}
void MM_DrawNeiGi(const CwItemDrawInfo&,bool=false,int=0) {assert(false);}
extern "C" int NeiGi_DrawElementalArrowShop(PlayState*,int) {
    assert(false && "song fixture must not dispatch elemental arrows");return 0;
}
extern "C" int NeiGi_DrawElementalSpell(PlayState*,int,int,int) {
    assert(false && "song fixture must not dispatch elemental spells");return 0;
}
bool MM_TryDrawNeiGi(RandoItemId,bool shop=false,int mmPickup=0);
''' + draw_song + '\n' + dispatcher + r'''
// Hand-recorded accepted samples from 9eff802c, quantized exactly as the
// renderer packs positions. Protect the approved particles, not private source text.
uint64_t ApprovedParticleHash(const NeiGi::Mesh& mesh) {
    uint64_t hash=1469598103934665603ull;
    const auto add=[&](uint32_t value) {
        for(int byte=0;byte<4;++byte) {hash^=(value>>(8*byte))&255;hash*=1099511628211ull;}
    };
    for(size_t i=0;i<mesh.count;++i) {
        const auto& v=mesh.vertices[i];
        add(int32_t(std::lround(v.p.x*16)));add(int32_t(std::lround(v.p.y*16)));add(int32_t(std::lround(v.p.z*16)));
        add(v.rgb);add(v.alpha);
    }
    return hash;
}
int main() {
    for(int id=RI_UNKNOWN;id<RI_MAX;++id) {
        const int song=ComboSongForMmItem(id);
        if(song<0)continue;
        overlay=season=-1; notes=genericShimmer=descriptions=0; env={};gray=true;
        assert(MM_TryDrawNeiGi(static_cast<RandoItemId>(id)) &&
               "early NEI dispatcher must claim every native/imported song before shared GI table IDs");
        assert(descriptions==0);
        const bool hasOverlay=(song>=CW_SONG_OOT_MINUET && song<=CW_SONG_OOT_SARIA) ||
                              song==CW_SONG_EPONA || song==CW_SONG_SARIA || song==CW_SONG_STORMS || song==CW_SONG_SUN || song==CW_SONG_SOARING ||
                              song==CW_SONG_HEALING || song==CW_SONG_SONATA || song==CW_SONG_LULLABY_INTRO || song==CW_SONG_LULLABY ||
                              song==CW_SONG_NOVA || song==CW_SONG_ELEGY || song==CW_SONG_OATH || song==CW_SONG_INVERTED_TIME || song==CW_SONG_DOUBLE_TIME;
        uint8_t color[4];assert(ComboSongShimmerColor(song,color));
        assert(notes==1 && overlay==(hasOverlay?song:-1) && genericShimmer==0 && season==-1);
        assert(!grayAtNote && "native MM notes must clear inherited grayscale before their native color is submitted");
        assert(!std::memcmp(env.data(),color,4));
    }
    assert(NeiGi::SampleSong(CW_SONG_OOT_BOLERO,42).count==0 && "Bolero retains its red shimmer without fire particles");
    const auto soaring=NeiGi::SampleSong(CW_SONG_SOARING,42);
    assert(soaring.count>400 && "Soaring lost its feather geometry");
    bool rachis=false;
    for(size_t i=0;i<soaring.count;++i)rachis|=soaring.vertices[i].rgb==0xFFF9FF && soaring.vertices[i].alpha>0;
    assert(rachis && "Soaring needs the feather's white spine, not generic motes");
    const auto prelude=NeiGi::SampleSong(CW_SONG_OOT_PRELUDE,42);
    assert(prelude.count>0);
    for(size_t i=0;i<prelude.count;i+=3)for(size_t j=0;j<3;++j) {
        const auto edge=prelude.vertices[i+j].p-prelude.vertices[i+(j+1)%3].p;
        assert(edge.x*edge.x+edge.y*edge.y+edge.z*edge.z<144 && "Prelude needs small light particles, not a circle or central disc");
    }
    assert(NeiGi::SampleSong(CW_SONG_OOT_SERENADE,42).count==0 &&
           "Serenade must keep its note/shimmer without a circle ring");
    for(int song=0;song<CW_SONG_COUNT;++song) {
        for(uint32_t frame:{0u,42u,179u,180u,719u,720u,0xffffffffu}) {
            const auto mesh=NeiGi::SampleSong(song,frame);
            assert(mesh.count<=mesh.vertices.size() && mesh.count%3==0);
            const bool themed=((song>=CW_SONG_OOT_MINUET && song<=CW_SONG_OOT_ZELDA) &&
                              song!=CW_SONG_OOT_SERENADE && song!=CW_SONG_OOT_BOLERO) || song==CW_SONG_SARIA || song==CW_SONG_OOT_SARIA || song==CW_SONG_SOARING ||
                              song==CW_SONG_HEALING || song==CW_SONG_SONATA || song==CW_SONG_LULLABY_INTRO || song==CW_SONG_LULLABY ||
                              song==CW_SONG_NOVA || song==CW_SONG_ELEGY || song==CW_SONG_OATH || song==CW_SONG_INVERTED_TIME || song==CW_SONG_DOUBLE_TIME;
            if(song==CW_SONG_OOT_SERENADE)assert(mesh.count==0 && "Serenade must keep its note/shimmer without a circle ring");
            if(!themed)assert(mesh.count==0 && "plain and shimmer-only songs must not submit themed shapes");
            else {
                assert(mesh.count>0);
                bool hue=false;
                for(size_t i=0;i<mesh.count;++i) {
                    const auto& v=mesh.vertices[i];
                    assert(std::isfinite(v.p.x)&&std::isfinite(v.p.y)&&std::isfinite(v.p.z));
                    assert(std::abs(v.p.x)<60&&std::abs(v.p.y)<60&&std::abs(v.p.z)<60);
                    hue |= v.rgb==ComboSongColorHex(song);
                }
                assert(hue);
            }
        }
    }
    const int newSongs[]={CW_SONG_HEALING,CW_SONG_SONATA,CW_SONG_LULLABY_INTRO,CW_SONG_LULLABY,CW_SONG_NOVA,
                          CW_SONG_ELEGY,CW_SONG_OATH,CW_SONG_INVERTED_TIME,CW_SONG_DOUBLE_TIME};
    uint64_t silhouettes[9]{};
    for(size_t i=0;i<9;++i) {
        const int song=newSongs[i];
        assert(ComboSongAlwaysShimmers(song) && "requested MM song shimmer must not depend on the generic item-effects toggle");
        const auto a=NeiGi::SampleSong(song,42), b=NeiGi::SampleSong(song,57);
        assert(a.count<a.vertices.size() && b.count<b.vertices.size() && "song geometry silently exhausted the arena before all particles were emitted");
        if(song==CW_SONG_ELEGY || song==CW_SONG_OATH) {
            uint8_t color[4];assert(ComboSongShimmerColor(song,color));
            const uint32_t hue=song==CW_SONG_ELEGY?0xFF6200:0x620062;
            assert(color[0]==(hue>>16) && color[1]==((hue>>8)&255) && color[2]==(hue&255) && color[3]==255);
        }
        assert(a.count && b.count && ApprovedParticleHash(a)!=ApprovedParticleHash(b) && "song particles must animate on the active host clock");
        silhouettes[i]=ApprovedParticleHash(a);
        for(size_t j=0;j<i;++j)assert(silhouettes[i]!=silhouettes[j] && "requested songs need distinct particle presentations");
    }
    const auto elegy40=NeiGi::SampleSong(CW_SONG_ELEGY,40), elegy80=NeiGi::SampleSong(CW_SONG_ELEGY,80);
    assert(elegy40.count && elegy80.count && elegy40.vertices[0].p.y>elegy80.vertices[0].p.y &&
           "Elegy's dust must fall slowly rather than form an effigy");
    for(uint32_t frame:{479u,719u,1439u}) {
        const auto a=NeiGi::SampleSong(CW_SONG_ELEGY,frame), b=NeiGi::SampleSong(CW_SONG_ELEGY,frame+1);
        assert(a.count==b.count);
        for(size_t i=0;i<a.count;++i)if(a.vertices[i].alpha>32 && b.vertices[i].alpha>32) {
            const auto delta=b.vertices[i].p-a.vertices[i].p;
            assert(delta.x*delta.x+delta.y*delta.y+delta.z*delta.z<1.f &&
                   "Elegy dust jumped visibly across an animation clock wrap");
        }
    }
    const auto oath40=NeiGi::SampleSong(CW_SONG_OATH,40), oath80=NeiGi::SampleSong(CW_SONG_OATH,80);
    assert(oath40.count && oath80.count && oath40.vertices[0].alpha!=oath80.vertices[0].alpha &&
           "Oath's sparse glints must twinkle");
    const auto clock0=NeiGi::SampleSong(CW_SONG_INVERTED_TIME,0);
    const auto clock1=NeiGi::SampleSong(CW_SONG_INVERTED_TIME,1);
    assert(clock1.vertices[0].p.y>clock0.vertices[0].p.y && "inverted dial must move counterclockwise from right toward up");
    for(uint32_t frame:{719u,1439u}) {
        const auto a=NeiGi::SampleSong(CW_SONG_INVERTED_TIME,frame), b=NeiGi::SampleSong(CW_SONG_INVERTED_TIME,frame+1);
        const auto delta=b.vertices[0].p-a.vertices[0].p;
        assert(std::sqrt(delta.x*delta.x+delta.y*delta.y+delta.z*delta.z)<.2f && "inverted dial jumped at a clock wrap");
    }
    struct AcceptedSample {int song;uint32_t frame;size_t count;uint64_t hash;};
    const AcceptedSample approved[]={
        {CW_SONG_OOT_ZELDA,0,432,0x5ef3848d5e204d77},{CW_SONG_OOT_ZELDA,42,432,0x1075dc4d5aee8647},
        {CW_SONG_OOT_ZELDA,179,432,0x1c1932cfb2609263},{CW_SONG_OOT_ZELDA,719,432,0xc3e507ee86d04727},
        {CW_SONG_OOT_SARIA,0,54,0x14c65f6bb3c545a3},{CW_SONG_OOT_SARIA,42,54,0xadf3702b117af522},
        {CW_SONG_OOT_SARIA,179,54,0x3fde501240cdd859},{CW_SONG_OOT_SARIA,719,54,0x309b942a98a1ff6c},
        {CW_SONG_SARIA,0,54,0x22e1f32bba3b68d7},{CW_SONG_SARIA,42,54,0xc1d8b9ef7734896e},
        {CW_SONG_SARIA,179,54,0xcbd3ded5c13dbf41},{CW_SONG_SARIA,719,54,0xad2442503b0a47d0}};
    for(const auto& sample:approved) {
        const auto mesh=NeiGi::SampleSong(sample.song,sample.frame);
        assert(mesh.count==sample.count && ApprovedParticleHash(mesh)==sample.hash && "approved Zelda/Saria particles changed");
    }
}
'''
    path=temp/'dispatch.cpp';path.write_text(source)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-I'+str(ROOT),'-I'+str(ROOT/'combo/menu'),str(path),'-o',str(temp/'dispatch')],check=True)
    subprocess.run([str(temp/'dispatch')],check=True)
print('PASS native notes/shimmer and MM exports/dispatcher; nine animated MM particle profiles fit the mesh budget; Elegy falling dust and Oath twinkling glints retain icon hues; counterclockwise continuous time dial; accepted Zelda/Saria hashes, Prelude motes, Bolero shimmer and Soaring feathers preserved')
