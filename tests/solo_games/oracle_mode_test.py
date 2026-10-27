#!/usr/bin/env python3
"""Keep native logic unchanged even when a stale cross-world oracle map exists."""
from pathlib import Path
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[2]
def block(text,marker):
    start=text.index(marker);pos=text.index('{',start);depth=1;end=pos+1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[start:end]
oot=(ROOT/'combo/rando/CrossDungeonOot.inc.h').read_text()
mm=(ROOT/'combo/rando/CrossDungeonMm.inc.h').read_text()
source=r'''
#include <cassert>
#include <map>
#define COMBO_BUILD
#define COMBO_EXPORT
#include "combo/ComboGameMode.h"
#include "combo/rando/CrossDungeonState.h"
extern "C" {int gComboGameMode=COMBO_GAME_BOTH;}
constexpr int RR_NONE=-1;
using RandoRegionId=int;
namespace ComboCrossOot {
using namespace ComboRando::CrossDungeon;
HostState state(0);bool searching=true;
std::map<int32_t,int32_t> nativeOverrides;
std::map<int32_t,int> regions;
}
namespace ComboCrossMm {
using namespace ComboRando::CrossDungeon;
HostState state(1);bool searching=true;
}
namespace ComboCrossMmCatalogue {int GrottoExitForRegion(int){return -1;}}
int CVarGetInteger(const char*,int){return 1;}
'''
for text,marker in [(oot,'int Combo_SOH_CrossDungeonRegion('),
                    (mm,'int Combo_MM_CrossDungeonEntranceFromRegion('),
                    (oot,'const char* SOH_ReadCrossEntranceOptions(')]:
    source+='\n'+block(text,marker)+'\n'
source+=r'''
int main(){
    using namespace ComboRando::CrossDungeon;
    Dungeon a{"oot",{0,100},{0,101},{0,101}},b{"mm",{1,200},{1,201},{1,201}};
    const auto seed=Encode({a,b},{1,0});
    ComboCrossOot::state.Install(seed);ComboCrossMm::state.Install(seed);
    assert(Combo_SOH_CrossDungeonRegion(100,100,42)==RR_NONE);
    assert(Combo_MM_CrossDungeonEntranceFromRegion(200,0)==-1);
    auto options=ParseOptions(nlohmann::json::parse(SOH_ReadCrossEntranceOptions()));
    assert(options.Any() && options.expanded);
    for(int mode:{COMBO_GAME_OOT,COMBO_GAME_MM}){
        gComboGameMode=mode;
        assert(Combo_SOH_CrossDungeonRegion(100,100,42)==42);
        assert(Combo_MM_CrossDungeonEntranceFromRegion(200,0)==200);
        options=ParseOptions(nlohmann::json::parse(SOH_ReadCrossEntranceOptions()));
        assert(!options.Any() && !options.expanded);
    }
}
'''
with tempfile.TemporaryDirectory(prefix='solo-oracle-') as temp:
    path=Path(temp);(path/'test.cpp').write_text(source)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-I'+str(ROOT),str(path/'test.cpp'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
print('Native logic fallback and stale cross-entrance option isolation PASS')
