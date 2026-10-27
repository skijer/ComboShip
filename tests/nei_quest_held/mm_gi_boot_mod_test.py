"""Exercise MM's actual GI recolor helpers after their stock copies exist."""
from pathlib import Path
import os, subprocess, sys, tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import functions
f=functions((ROOT/'mm/2s2h/Rando/DrawItem.cpp').read_text())
prefix=r'''
#include <cassert>
#include <cstdint>
#include <vector>
#include <iostream>
using u8=uint8_t;using u32=uint32_t;using f32=float;
struct Gfx{struct{uint32_t w0;uintptr_t w1;}words;};
constexpr int G_SETPRIMCOLOR=0xfa,G_SETENVCOLOR=0xfb;
Gfx stock[]={{{0xfa000000,0xff1234ff}},{{0xdf000000,0}}};
Gfx baseMod[]={{{0xfa000000,0xab1234ff}},{{0xdf000000,0}}};
Gfx altMod[]={{{0xfa000000,0xcd5678ff}},{{0xdf000000,0}}};
bool custom=false,alt=false;
int ResourceMgr_IsModAsset(const char*){return custom;}
Gfx* ResourceMgr_LoadGfxByName(const char*){return custom?(alt?altMod:baseMod):stock;}
void* OotAssets_LoadGfx(const char* p){return ResourceMgr_LoadGfxByName(p);}
u32 remap(u32){return 0xeeeeeeff;}
'''
checks=r'''
int main(){
 const char* root="__OTR__objects/object_gi_boots_2/gGiIronBootsDL";
 Gfx copy[512];bool built=false;
 auto* pegasus=Pegasus_GetRecoloredBootsDL();assert(pegasus!=stock);
 auto* climb=BuildRecoloredOotGiDL(root,remap,copy,&built);assert(climb==copy&&built);
 assert(copy[0].words.w1==0xeeeeeeff);
 custom=true;
 assert(Pegasus_GetRecoloredBootsDL()==baseMod);
 assert(BuildRecoloredOotGiDL(root,remap,copy,&built)==baseMod);
 alt=true;
 assert(Pegasus_GetRecoloredBootsDL()==altMod);
 assert(BuildRecoloredOotGiDL(root,remap,copy,&built)==altMod);
 assert(baseMod[0].words.w1==0xab1234ff&&altMod[0].words.w1==0xcd5678ff);
 custom=false;assert(Pegasus_GetRecoloredBootsDL()==pegasus);
 assert(BuildRecoloredOotGiDL(root,remap,copy,&built)==climb);
 std::cout<<"PASS MM GI boot helpers: stock recolor retained, current base/Alt mod bypasses populated copy and preserves mod colors\n";
}
'''
with tempfile.TemporaryDirectory(prefix='mm-gi-boot-') as td:
 p=Path(td)/'test.cpp';p.write_text(prefix+'\n'.join(f[n] for n in ['Pegasus_CrimsonRamp','Pegasus_GetRecoloredBootsDL','BuildRecoloredOotGiDL'])+checks)
 out=Path(td)/'test';subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',str(p),'-o',str(out)],check=True)
 subprocess.run([str(out)],check=True)
