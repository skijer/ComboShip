#!/usr/bin/env python3
"""Check the exact supplied pack, active native mesh contract and return bounds."""
import argparse
from pathlib import Path
import os
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_time_pedestal_tests import block_from

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--baseline', type=Path)
parser.add_argument('--candidate', type=Path)
parser.add_argument('--sanitize', action='store_true')
args = parser.parse_args()
if bool(args.baseline) != bool(args.candidate):
    parser.error('supply both --baseline and --candidate')

prefix = 'custom/cor_mm3d/lost_woods_poc1/room_0/'
wrapper = prefix + 'opa_combo_return_v1'
if args.baseline:
    with zipfile.ZipFile(args.baseline) as before, zipfile.ZipFile(args.candidate) as after:
        assert after.testzip() is None
        assert set(after.namelist()) == set(before.namelist()) | {'alt/' + wrapper}
        changed = {n for n in before.namelist() if before.read(n) != after.read(n)}
        headers = {n for n in before.namelist() if n.startswith(
            'alt/scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS_room_00')}
        glow = 'alt/' + prefix + 'ReturnTunnelPinkLight_vtx'
        assert len(headers) == 4 and changed == headers | {glow}, changed
        old_vertices, new_vertices = [ET.fromstring(z.read(glow)) for z in (before, after)]
        assert len(old_vertices) == len(new_vertices) == 60
        for old, new in zip(old_vertices, new_vertices):
            assert tuple(new.get(c) for c in ('R', 'G', 'B')) == ('96', '32', '176'), 'glow must be deep purple'
            assert {k: v for k, v in old.attrib.items() if k not in ('R', 'G', 'B')} == {
                k: v for k, v in new.attrib.items() if k not in ('R', 'G', 'B')}, 'glow geometry/UV/alpha changed'
        root = ET.fromstring(after.read('alt/' + wrapper))
        assert [(c.tag, c.get('Path')) for c in root] == [('CallDisplayList', prefix + 'opa'), ('EndDisplayList', None)]
        print('PASS supplied archive: exactly four mesh bindings and purple glow RGB; geometry/textures/alpha unchanged')

drawing = (ROOT / 'mm/2s2h/Enhancements/Graphics/ComboLostWoodsPassage.cpp').read_text()
body = block_from(drawing, drawing.index('static void DrawPassage('))
# Display-list submission is the boundary. Execute the actual draw selection.
body = body[:body.index('    static const bool built')] + '\n++draws;\n}\n'
source = r'''
#include <cassert>
#include <cstdio>
#include <initializer_list>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <fast/resource/type/Texture.h>
#include "global.h"
#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#include "2s2h/Enhancements/Graphics/WoodsMysteryMakeover.h"
#include "ComboGameMode.h"
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
SaveContext gSaveContext{};
PlayState* gPlayState;
static int draws;
static bool soloPassageAvailable;
extern "C" bool ResourceMgr_IsAltAssetsEnabled() { return true; }
extern "C" uint8_t ResourceMgr_FileAltExists(const char*) { return 0; }
extern "C" uint8_t ResourceMgr_FileExists(const char*) { return 0; }
extern "C" int ComboSoloForestMm_PassageAvailable(void) { return soloPassageAvailable; }
// These fixtures exercise opening-forest ownership and the legacy fallback;
// the verified baked-wall lifecycle has its own native resource harness.
namespace WoodsMysteryNativeBridge { inline bool Active(const PlayState*) { return false; } }
'''
foliage = (ROOT / 'mm/2s2h/Enhancements/Graphics/AutumnSceneFoliage.cpp').read_text()
source += block_from(foliage, foliage.index('std::shared_ptr<Fast::Texture> BuildSurfaceTexture('))
source += body + r'''
static void CheckPackedPixels(const char* path) {
    std::ifstream file(path,std::ios::binary);
    std::vector<char> bytes((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
    assert(bytes.size()>92);
    uint32_t values[4]; float scales[2]; uint32_t size;
    memcpy(values,bytes.data()+64,sizeof(values)); memcpy(scales,bytes.data()+80,sizeof(scales));
    memcpy(&size,bytes.data()+88,4);
    assert(values[0]==2 && values[3]==TEX_FLAG_LOAD_AS_RAW && size==values[1]*values[2]*4 && bytes.size()==size+92);
    auto source=std::make_shared<Fast::Texture>(std::make_shared<Ship::ResourceInitData>());
    source->Type=(Fast::TextureType)values[0]; source->Width=values[1]; source->Height=values[2]; source->Flags=values[3];
    source->HByteScale=scales[0]; source->VPixelScale=scales[1]; source->ImageDataSize=size;
    source->mImageBuffer=std::make_shared<std::vector<char>>(bytes.begin()+92,bytes.end());
    source->ImageData=(uint8_t*)source->mImageBuffer->data();
    for (uint32_t color : {0xB96848FFu,0xF6C1D9E0u,0xCCDCEEFFu}) {
        auto copy=BuildSurfaceTexture(source,color,color==0xCCDCEEFFu?1.8f:1.0f);
        assert(copy && copy->Width==source->Width && copy->Height==source->Height && copy->Flags==source->Flags &&
               copy->Type==source->Type && copy->HByteScale==source->HByteScale && copy->VPixelScale==source->VPixelScale);
        unsigned changed=0;
        for (size_t i=0;i<size;i+=4) {
            const auto* original=source->ImageData+i; const auto* shown=copy->ImageData+i;
            assert(shown[3]==original[3]);
            const bool green=original[1]>original[0] && original[1]>original[2];
            if (!green) assert(memcmp(original,shown,4)==0);
            changed+=memcmp(original,shown,3)!=0;
        }
        assert(changed>0 && memcmp(source->ImageData,bytes.data()+92,size)==0);
    }
}
int main(int argc,char** argv) {
    PlayState play{}; gPlayState=&play;
    gSaveContext.gameMode=GAMEMODE_NORMAL; gSaveContext.fileNum=0;
    gSaveContext.save.entrance=ENTRANCE(LOST_WOODS,0);
    play.sceneId=SCENE_LOST_WOODS; play.roomCtx.curRoom.num=0;
    ActorEntry spawn{}; spawn.pos={-417,0,-122}; spawn.rot.y=11520;
    play.linkActorEntry=&spawn;
    RoomShape shape{}; RoomShapeDListsEntry entry{};
    shape.normal.base.type=ROOM_SHAPE_TYPE_NORMAL; shape.normal.numEntries=1;
    shape.normal.entries=&entry; play.roomCtx.curRoom.roomShape=&shape;
    alignas(Gfx) static const char bound[]="__OTR__custom/cor_mm3d/lost_woods_poc1/room_0/opa_combo_return_v1";
    alignas(Gfx) static const char unbound[]="__OTR__custom/cor_mm3d/lost_woods_poc1/room_0/opa";
    entry.opa=(Gfx*)bound;
    assert(ComboOpeningForest_UsesPackTunnel(&play));
    DrawPassage(); assert(draws==0); // No doubled arch over the authored tunnel.
    const auto progress=gSaveContext.save;
    auto position=[&](float lateral,float y,float depth) {
        const float a=11520.0f*6.28318530718f/65536.0f;
        return Vec3f{-417+cosf(a)*lateral+sinf(a)*depth,y,-122-sinf(a)*lateral+cosf(a)*depth};
    };
    auto center=position(0,0,0); assert(ComboOpeningForest_AtReturn(&play,&center));
    for (const auto& p : {position(65,0,0),position(0,0,80),position(0,0,-80),position(0,140,0),position(0,-50,0)})
        assert(!ComboOpeningForest_AtReturn(&play,&p));
    assert(memcmp(&progress,&gSaveContext.save,sizeof(progress))==0);
    entry.opa=(Gfx*)unbound;
    assert(!ComboOpeningForest_UsesPackTunnel(&play)); DrawPassage(); assert(draws==1);
    entry.opa=(Gfx*)bound; gSaveContext.sceneLayer=1;
    assert(!ComboOpeningForest_UsesPackTunnel(&play)); assert(!ComboOpeningForest_AtReturn(&play,&center));
    gSaveContext.sceneLayer=0; shape.normal.numEntries=0;
    assert(!ComboOpeningForest_UsesPackTunnel(&play));
    shape.normal.numEntries=1; play.sceneId=SCENE_26SARUNOMORI; play.roomCtx.curRoom.num=1;
    DrawPassage(); assert(draws==2); // Existing Mystery arch remains available.
    gComboGameMode=COMBO_GAME_MM;
    DrawPassage(); assert(draws==2); // A missing native addon must not advertise a route.
    soloPassageAvailable=true;
    DrawPassage(); assert(draws==3); // The native solo Mystery route has its visible bark arch.
    play.roomCtx.curRoom.num=0; DrawPassage(); assert(draws==3);
    play.sceneId=SCENE_LOST_WOODS; DrawPassage(); assert(draws==3); // Original intro stays guarded.
    play.sceneId=SCENE_26SARUNOMORI; play.roomCtx.curRoom.num=1;
    gSaveContext.gameMode=GAMEMODE_TITLE_SCREEN; DrawPassage(); assert(draws==3);
    gSaveContext.gameMode=GAMEMODE_NORMAL; gComboGameMode=COMBO_GAME_OOT;
    soloPassageAvailable=false; DrawPassage(); assert(draws==3);
    std::puts("PASS active native room mesh: exact pack selection, oriented return bounds, fallback/Mystery arch and untouched save");
    for (int i=1;i<argc;++i) CheckPackedPixels(argv[i]);
    if (argc>1) std::puts("PASS actual packed HD images: autumn/spring/frost copy, exact alpha/non-green pixels/scales and immutable source");
}
'''
with tempfile.TemporaryDirectory(prefix='mm3d-forest-pack-') as directory:
    cpp=Path(directory)/'test.cpp'; binary=Path(directory)/'test'; cpp.write_text(source)
    command=[os.environ.get('CXX','c++'),'-std=c++20','-O1','-w','-DCOMBO_BUILD','-DF3DEX_GBI_2','-DFMT_HEADER_ONLY']
    command+=['-I'+str(ROOT/p) for p in ['mm','mm/2s2h','mm/include','mm/include/PR','mm/src','mm/assets','libultraship/include','combo']]
    if args.sanitize:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie']
    subprocess.run([*command,str(cpp),str(ROOT/'libultraship/src/ship/resource/Resource.cpp'),
                    str(ROOT/'libultraship/src/fast/resource/type/Texture.cpp'),'-o',str(binary)],check=True)
    samples=[]
    if args.candidate:
        with zipfile.ZipFile(args.candidate) as z:
            for name in ('lw_enkei_01_0','lw_kabe_01_5400','lw_kusa_01_12600','lw_wood_02_2CA00',
                         'lw_wood_03_31E00','lw_wood_04_37200','lw_yuka_02_39C00','lw_yuka_03_44400'):
                sample=Path(directory)/name
                sample.write_bytes(z.read('alt/custom/cor_mm3d/lost_woods_poc1/textures/'+name))
                samples.append(str(sample))
    subprocess.run([str(binary),*samples],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
