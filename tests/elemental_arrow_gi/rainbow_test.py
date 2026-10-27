"""Compare MM's actual donor sampler and native editor tick without a game frame."""
from pathlib import Path
import os,re,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import functions
source=(ROOT/'mm/2s2h/BenGui/CosmeticEditor.cpp').read_text()
start=source.index('// Resource-only donor clock.')
sampler=source[start:source.index('#endif',start)].replace('COMBO_EXPORT ','')
native=functions(source)['CosmeticEditorUpdateTick']
table=source[source.index('std::map<std::string, CosmeticOption> cosmeticOptions'):]
table=table[:table.index('\n};')]
entries=re.findall(r'COSMETIC_OPTION\("([^"]+)",\s*"[^"]*",\s*\w+,\s*ColorRGBA8\(([^)]+)\),\s*(true|false),\s*(true|false),\s*(true|false)\)',table)
assert len(entries)>40
prefix=r'''
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <iostream>
struct Color_RGB8 {uint8_t r,g,b;};
struct Color_RGBA8 {uint8_t r,g,b,a;};
struct ImVec4 {float x,y,z,w; ImVec4(float x=0,float y=0,float z=0,float w=1):x(x),y(y),z(z),w(w){}};
struct CosmeticOption {const char* valuesCvar;const char* rainbowCvar;const char* changedCvar;bool supportsAlpha,supportsRainbow;ImVec4 currentColor;};
std::map<std::string,CosmeticOption> cosmeticOptions;
std::map<std::string,int> flags;
std::map<std::string,Color_RGBA8> colors;
float rainbowSpeed=.6f;
const char* kCosmeticRainbowSyncCvar="gCosmetics.RainbowSync";
const char* kCosmeticRainbowSpeedCvar="gCosmetics.RainbowSpeed";
int sCosmeticRainbowHue=0;
int patches=0,notifications=0;
int CVarGetInteger(const char* key,int value){auto it=flags.find(key);return it==flags.end()?value:it->second;}
float CVarGetFloat(const char*,float){return rainbowSpeed;}
Color_RGB8 CVarGetColor24(const char* key,Color_RGB8 value){auto it=colors.find(key);return it==colors.end()?value:Color_RGB8{it->second.r,it->second.g,it->second.b};}
void CVarSetInteger(const char* key,int value){flags[key]=value;}
void CVarSetColor(const char* key,Color_RGBA8 value){colors[key]=value;}
bool CosmeticEditorIsSuppressed(const CosmeticOption&){return false;}
void RefreshDynamicCosmeticsStateIfNeeded(){}
bool UpdateCustomCosmeticsRainbow(int,float,int){return false;}
void ApplyDynamicCosmetics(){++patches;}
namespace ShipInit {void Init(const char*){++notifications;}}
'''
setup=''
for id,rgba,alpha,rainbow,advanced in entries:
 setup+=f' cosmeticOptions["{id}"]={{"gCosmetic.{id}.Color","gCosmetic.{id}.Rainbow","gCosmetic.{id}.Changed",{alpha},{rainbow}}};\n'
checks=r'''
 for(const auto& [id,option]:cosmeticOptions) flags[option.rainbowCvar]=1;
 const char* rows[]={"Effects.FireArrowPrim","Effects.FireArrowSec","Effects.IceArrowPrim","Effects.IceArrowSec","Effects.LightArrowPrim","Effects.LightArrowSec","Magic.DinsPrimary","Magic.DinsSecondary","Magic.FaroresPrimary","Magic.FaroresSecondary","Magic.NayrusPrimary","Magic.NayrusSecondary"};
 for(bool sync:{false,true}) for(float speed:{.3f,.6f,1.2f}) {
  rainbowSpeed=speed;flags[kCosmeticRainbowSyncCvar]=sync;
  sGiCosmeticFrameSet=false;sCosmeticRainbowHue=17;
  for(uint32_t frame=100;frame<130;++frame) {
   MM_SetGiCosmeticFrame(frame);
   std::map<std::string,Color_RGB8> expected;
   for(const char* row:rows) {
    const auto& option=cosmeticOptions.at(row);uint8_t sampled[3];
    MM_SampleGiCosmeticColor(option.valuesCvar,1,2,3,sampled);
    expected[row]={sampled[0],sampled[1],sampled[2]};
   }
   const auto phase=sGiCosmeticHue;MM_SetGiCosmeticFrame(frame);assert(sGiCosmeticHue==phase);
   assert(patches==0 && "foreign sampling must not patch dormant assets");
   CosmeticEditorUpdateTick();
   for(const char* row:rows) {
    const auto actual=colors.at(cosmeticOptions.at(row).valuesCvar);
    const auto want=expected.at(row);
    assert(actual.r==want.r && actual.g==want.g && actual.b==want.b);
   }
  }
 }
 rainbowSpeed=.6f;sCosmeticRainbowHue=17;sGiCosmeticFrameSet=false;
 MM_SetGiCosmeticFrame(100);const int notified=notifications;
 MM_SetGiCosmeticFrame(137);assert(sGiCosmeticHue==54 && sCosmeticRainbowHue==17 && notifications==notified);
 MM_SetGiCosmeticFrame(1);assert(sGiCosmeticHue==55);
 const auto& option=cosmeticOptions.at("Magic.FaroresPrimary");flags[option.rainbowCvar]=0;colors[option.valuesCvar]={12,34,56,255};
 uint8_t sampled[3];MM_SampleGiCosmeticColor(option.valuesCvar,1,2,3,sampled);
 assert(sampled[0]==12 && sampled[1]==34 && sampled[2]==56);
 colors[option.valuesCvar]={210,120,30,255};MM_SampleGiCosmeticColor(option.valuesCvar,1,2,3,sampled);
 assert(sampled[0]==210 && sampled[1]==120 && sampled[2]==30);
 assert(patches==0);
 std::cout<<"PASS actual MM editor tick parity for all six GI pairs: live colors, rainbow speed/sync, same-frame idempotence, dormant advancement, frame reset, no dormant patches\n";
}
'''
with tempfile.TemporaryDirectory(prefix='elemental-rainbow-') as temp:
 path=Path(temp)/'test.cpp';binary=Path(temp)/'test'
 path.write_text(prefix+sampler+'\n'+native+'\nint main(){\n'+setup+checks)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-O2',str(path),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
