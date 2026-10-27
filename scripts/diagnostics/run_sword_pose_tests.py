#!/usr/bin/env python3
"""Transform a +X blade through the actual native/producer sword draw bodies."""
import os
from pathlib import Path
import subprocess
import tempfile
from run_time_pedestal_tests import functions

ROOT=Path(__file__).resolve().parents[2]
draw=functions((ROOT/'soh/soh/Enhancements/randomizer/draw.cpp').read_text())
native=functions((ROOT/'soh/soh/Enhancements/randomizer/NeiGiPresentation.cpp').read_text())
owner=functions((ROOT/'combo/menu/ComboItemDrawOOT.h').read_text())
source=r'''
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include "combo/menu/ComboItemDrawABI.h"
#include "soh/soh/Enhancements/randomizer/NeiGiEffectPolicy.h"
#include "soh/soh/Enhancements/randomizer/NeiGiFrameFit.h"
using f32=float;using s16=int16_t;using Gfx=int;
constexpr int MTXMODE_APPLY=1;
struct PlayState{struct{void* gfxCtx;}state;uint32_t gameplayFrames=42;} play;
using Matrix=std::array<std::array<float,3>,3>;
Matrix m;
float lift;
void Reset(){lift=0;m={{{1,0,0},{0,1,0},{0,0,1}}};}
void Multiply(const Matrix& r){auto old=m;for(int i=0;i<3;++i)for(int j=0;j<3;++j){m[i][j]=0;for(int k=0;k<3;++k)m[i][j]+=old[i][k]*r[k][j];}}
void Matrix_Translate(float x,float y,float z,int){lift+=m[1][0]*x+m[1][1]*y+m[1][2]*z;}
void ComboSwordGi_ApplyFit(const char*,const char*,float scale,float tilt,bool shop=false){
    const float c=std::cos(tilt),s=std::sin(tilt);
    NeiGi::FrameBounds bounds{"selected",{0,-670*s-268*c,0},{0,4122*s-268*c,0},2400,NeiGi::Kind::MasterSword,{}};
    auto fit=NeiGi::FrameFit(bounds,scale,shop);Matrix_Translate(0,fit.lift,0,1);
    // Apply the fit without depending on Matrix_Scale's later declaration.
    Multiply({{{fit.scale,0,0},{0,fit.scale,0},{0,0,fit.scale}}});
}
void Matrix_RotateX(float a,int){Multiply({{{1,0,0},{0,std::cos(a),-std::sin(a)},{0,std::sin(a),std::cos(a)}}});}
void Matrix_RotateY(float a,int){Multiply({{{std::cos(a),0,std::sin(a)},{0,1,0},{-std::sin(a),0,std::cos(a)}}});}
void Matrix_RotateZ(float a,int){Multiply({{{std::cos(a),-std::sin(a),0},{std::sin(a),std::cos(a),0},{0,0,1}}});}
void Matrix_Scale(float a,float b,float c,int){assert(a==b&&b==c);Multiply({{{a,0,0},{0,b,0},{0,0,c}}});}
#include "combo/menu/ComboSwordGiEffectFit.h"
#define OPEN_DISPS(...) ((void)0)
#define CLOSE_DISPS(...) ((void)0)
#define Gfx_SetupDL_25Opa(...) ((void)0)
#define gDPSetPrimColor(...) ((void)0)
#define gDPSetEnvColor(...) ((void)0)
#define gSPGrayscale(...) ((void)0)
#define gSPMatrix(...) ((void)0)
#define gSPDisplayList(...) ((void)0)
#define gDma1p(...) ((void)0)
enum RandomizerGet{RG_KOKIRI_SWORD,RG_RAZOR_SWORD,RG_GILDED_SWORD,RG_TRUE_MASTER_SWORD,RG_MASTER_SWORD,RG_BIGGORON_SWORD,RG_GIANTS_KNIFE,RG_GREAT_FAIRY_SWORD};
int32_t OOT_NeiAltAssetsEnabled(){return true;}
int32_t OOT_NeiResourceExists(const char*){return true;}
#define CVAR_ENHANCEMENT(x) x
int CVarGetInteger(const char*,int){return 0;}
void ComboDinSwordGi_DrawLayers(PlayState*,const char*,const char*) {}
void NeiGi_ReportBlackMarketModel(const char*,const char*,const char* = nullptr) {}
''' + draw['DrawMmWeaponGi']+'\n'
source+='\n'.join(owner[name] for name in ('CwSimple','CwCustomGi','CwAltSwordGi'))+'\n'
if 'NeiGi_DrawSelectedSword' in native:
    source+=native['Spin']+'\n'+native['NeiGi_DrawSelectedSword']+'\n'
source+=r'''
void Check(){
    const float length=std::sqrt(m[0][0]*m[0][0]+m[1][0]*m[1][0]+m[2][0]*m[2][0]);
    assert(m[1][0]/length>.99999f && "selected +X sword must point straight up through the full GI spin");
}
int main(){
    for(uint32_t frame:{0u,42u,179u,180u,32767u,65535u}){
        play.gameplayFrames=frame;Reset();DrawMmWeaponGi(&play,nullptr,nullptr,.04f,false);
        Check();
        Reset();DrawMmWeaponGi(&play,nullptr,nullptr,.04f,true);
        Check();
        for(auto id:{RG_KOKIRI_SWORD,RG_RAZOR_SWORD,RG_GILDED_SWORD,RG_MASTER_SWORD,RG_TRUE_MASTER_SWORD,RG_BIGGORON_SWORD,RG_GIANTS_KNIFE,RG_GREAT_FAIRY_SWORD}){
            CwItemDrawInfo info{};assert(CwAltSwordGi(id,&info));
            Reset();Matrix_RotateY(.84f,MTXMODE_APPLY);
            for(int i=0;i<info.opCount;++i){const auto& op=info.ops[i];const float angle=op.a*(6.28318530718f/65536.f);
                if(op.op==CW_OP_ROTATE_X)Matrix_RotateX(angle,1);
                if(op.op==CW_OP_ROTATE_Y)Matrix_RotateY(angle,1);
                if(op.op==CW_OP_ROTATE_Z)Matrix_RotateZ(angle,1);
            }
            Matrix_Scale(info.scale,info.scale,info.scale,1);Check();
        }
'''
if 'NeiGi_DrawSelectedSword' in native:
    source+='Reset();NeiGi_DrawSelectedSword(&play,"selected");Check();\nconst float high=m[1][0]*4122+m[1][1]*-268+m[1][2]*101+lift;\nassert(high<=55.501f && "enlarged selected Din equipment mesh exceeds the GI frame envelope");\n'
    source+='Reset();NeiGi_DrawSelectedSword(&play,"selected",true);Check();\n'
source+='}\n}\n'
with tempfile.TemporaryDirectory(prefix='sword-pose-') as temporary:
    path=Path(temporary); (path/'pose.cpp').write_text(source)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-I'+str(ROOT),str(path/'pose.cpp'),'-o',str(path/'pose')],check=True)
    subprocess.run([str(path/'pose')],check=True)
print('PASS upright native Four Sword/selected sword pickups and shelves across spins')

# The inline GI helpers must use each actual host's matrix/GBI API, not only
# the controlled geometry seam above.
with tempfile.TemporaryDirectory(prefix='sword-gi-headers-') as temporary:
    for game in ('soh','mm'):
        unit='#include "global.h"\n'
        if game=='soh':
            unit+='#include "soh/ResourceManagerHelpers.h"\n'
            includes=['soh','soh/include','soh/src','soh/assets','soh/mods']
        else:
            unit+='#include "2s2h/BenPort.h"\n#define COMBO_DIN_SWORD_GI_HOST_MM\n'
            includes=['mm','mm/include','mm/include/PR','mm/src','mm/assets','mm/2s2h']
        unit+='#include "ComboSwordGiFit.h"\n#include "ComboDinSwordGi.h"\n#include "NeiAssetPriorityResource.h"\n'
        includes+=['libultraship/include','libultraship/src','combo','combo/menu']
        path=Path(temporary)/(game+'.cpp');path.write_text(unit)
        command=[os.environ.get('CXX','c++'),'-std=c++20','-DF3DEX_GBI_2','-DCOMBO_BUILD',
                 '-DLOG_LEVEL_GAME_PRINTS=0','-DCONTROLLERBUTTONS_T=uint32_t','-DNON_EQUIVALENT',
                 '-DNON_MATCHING','-fsyntax-only',*['-I'+str(ROOT/p) for p in includes],str(path)]
        result=subprocess.run(command,capture_output=True,text=True)
        if result.returncode:raise RuntimeError(result.stdout+result.stderr)
        print('PASS real-header sword fit/Din-layer/resource helpers '+game)

# Keep the actual native-MM foreign renderer include order in the gate. A small
# helper unit alone cannot prove this boundary, and OoT compatibility aliases
# hide a missing native MM matrix API.
includes=['mm/include','mm/include/PR','mm/src','mm','mm/assets','mm/2s2h',
          'libultraship/include','libultraship/src','combo','combo/menu']
command=[os.environ.get('CXX','c++'),'-std=gnu++20','-DF3DEX_GBI_2','-DCOMBO_BUILD',
         '-DCONTROLLERBUTTONS_T=uint32_t','-DLOG_LEVEL_GAME_PRINTS=0','-DNON_EQUIVALENT',
         '-DNON_MATCHING','-DMM_BUILD_DLL','-DIMGUI_DEFINE_MATH_OPERATORS',
         *['-I'+str(ROOT/p) for p in includes],
         '-include','libultraship/bridge/consolevariablebridge.h',
         '-include','ship/Context.h','-include','ship/window/Window.h','-fsyntax-only']
for name in ('DrawItem.cpp','NeiGiPresentation.cpp'):
    result=subprocess.run([*command,str(ROOT/'mm/2s2h/Rando'/name)],capture_output=True,text=True)
    if result.returncode:raise RuntimeError(result.stdout+result.stderr)
    print('PASS actual native MM '+name+' sword helper include order')
