#!/usr/bin/env python3
"""Exercise actual native entrance condition/region guards during foreign exit enumeration."""
import argparse
from pathlib import Path
import re
import subprocess
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import block_from
parser=argparse.ArgumentParser();parser.add_argument('--sanitize',action='store_true');args=parser.parse_args()
adapter=(ROOT/'combo/rando/CrossDungeonOot.inc.h').read_text()
native=(ROOT/'soh/soh/Enhancements/randomizer/entrance.cpp').read_text()
boss=(ROOT/'combo/rando/CrossEntranceOotBoss.inc.h').read_text()
access=(ROOT/'soh/soh/Enhancements/randomizer/location_access.cpp').read_text()
region_header=(ROOT/'soh/soh/Enhancements/randomizer/location_access.h').read_text()
def body(source,name):
    match=re.search(r'^(?:extern "C" )?(?:static )?[\w: *&]+\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*(?:const\s*)?\{',source,re.M)
    assert match,name+' production function is missing'
    return block_from(source,match.start())
source=r'''
#include "ComboGameMode.h"
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
#include "rando/CrossDungeonState.h"
#include "rando/CrossEntranceBoss.h"
#include <cassert>
#include <functional>
#include <iostream>
#include <list>
#define COMBO_EXPORT
using RandomizerRegion=int;
constexpr int RR_NONE=-1, RR_ROOT=0, RSK_NO_LOGIC=0, RO_GENERIC_OFF=0, PT_ENTRANCE_LOGIC=0;
void StartPerformanceTimer(int) {}
void StopPerformanceTimer(int) {}
struct Region;
Region* RegionTable(int);
namespace Rando {
struct Logic { bool IsChild=false,IsAdult=false,AtDay=false,AtNight=false; int CurrentRegionKey=RR_NONE; } logicState;
Logic* logic=&logicState;
struct Option { bool Is(int value) const { return value==RO_GENERIC_OFF; } };
struct Context {
    static Context* GetInstance() { static Context context;return &context; }
    Option GetOption(int) const { return {}; }
    Logic* GetLogic() const { return logic; }
};
enum class EntranceType { All };
struct Entrance {
    int index=100,parentRegion=1;
    RandomizerRegion connectedRegion=900;
    Entrance* replacement=nullptr;
    std::function<bool()> condition_function;
    Entrance(int i,int p,int c,Entrance* r,std::function<bool()> f)
        :index(i),parentRegion(p),connectedRegion(c),replacement(r),condition_function(f) {}
    Entrance(int c,std::function<bool()> f,const char*)
        :connectedRegion(c),condition_function(f) {}
    void SetParentRegion(int region) { parentRegion=region; }
    int GetIndex() const { return index; }
    int GetParentRegionKey() const { return parentRegion; }
    bool GetConditionsMet() const;
    bool ConditionsMet(bool allAgeTimes=false) const;
    bool CheckConditionAtAgeTime(bool& age,bool& time,bool passAnyway=false) const;
    RandomizerRegion GetConnectedRegionKey() const;
};
}
using ConditionFn=std::function<bool()>;
auto& logic=Rando::logic;
struct Region {
    bool childDay=true,childNight=false,adultDay=false,adultNight=false;
    std::list<Rando::Entrance> exits;
    bool AllAccess() const { return childDay&&childNight&&adultDay&&adultNight; }
    bool Child() const { return childDay||childNight; }
    bool Adult() const { return adultDay||adultNight; }
'''+block_from(region_header,region_header.index('    bool AnyAgeTime(ConditionFn condition) const'))+r'''
};
std::map<int,Region> areaTable;
Region* RegionTable(int region) { return &areaTable[region]; }
'''+body(access,'AnyAgeTime')+r'''
namespace ComboCrossOot {
using namespace ComboRando::CrossDungeon;
static HostState state(0);
static bool searching=false,evaluatingExits=false,throwCondition=false;
static BossCompletionState bossCompletion;
static bool runtimeBossEnabled=true;
static std::map<int32_t,int32_t> regions,nativeOverrides;
struct DungeonRuleScope {};
int Resolved(Rando::Entrance* entrance) { return entrance->index; }
void IndexRegions() {}
'''+block_from(adapter,adapter.index('struct SearchScope {'))+';\n'+r'''
'''+body(boss,'BossLogicEnabled')+r'''
}
extern "C" int Combo_SOH_CrossBossEnabled(void) {
    return ComboCrossOot::runtimeBossEnabled;
}
'''+body(boss,'Combo_SOH_CrossBossLogicalClear')+'\n'+body(adapter,'Combo_SOH_CrossDungeonRegion')+r'''
extern "C" int Combo_SOH_CrossDungeonRegion(int,int,int);
namespace Rando {
'''+body(native,'Entrance::GetConditionsMet')+'\n'+body(native,'Entrance::ConditionsMet')+'\n'+body(native,'Entrance::CheckConditionAtAgeTime')+'\n'+body(native,'Entrance::GetConnectedRegionKey')+r'''
}
Rando::Entrance& NativeExit() {
    static Rando::Entrance exit{100,1,900,nullptr,[] {
        if(ComboCrossOot::throwCondition)throw std::runtime_error("native condition interrupted");
        return AnyAgeTime([] {return Rando::logic->IsChild;}) &&
               Combo_SOH_CrossBossLogicalClear(COMBO_BOSS_GOHMA,0)!=0;
    }};
    return exit;
}
std::vector<Rando::Entrance*> GetShuffleableEntrances(Rando::EntranceType,bool) { return {&NativeExit()}; }
'''+body(adapter,'SOH_GetCrossDungeonExits')+r'''
int main() {
    using namespace ComboCrossOot;
    // Native baseline probing still runs while a different playing slot can own boss progress.
    state.Install(nullptr);bossCompletion.SetCompletedMask(1);
    assert(Combo_SOH_CrossBossLogicalClear(COMBO_BOSS_GOHMA,0));
    {
        SearchScope scope;
        assert(!Combo_SOH_CrossBossLogicalClear(COMBO_BOSS_GOHMA,0));
    }
    assert(!searching);
    Dungeon a{"a",{0,100},{0,101},{0,101}}, b{"b",{1,200},{1,201},{1,201}};
    a.pool=b.pool=Pool::Interior;
    Dungeon ba{"ba",{0,10},{0,11},{0,11}},bb{"bb",{1,20},{1,21},{1,21}};
    ba.pool=bb.pool=Pool::Boss;ba.boss=0;bb.boss=8;ba.owner="owner-a";bb.owner="owner-b";
    ba.warpReturn=Endpoint{0,30};bb.warpReturn=Endpoint{1,40};
    state.Install(Encode({a,b,ba,bb},{1,0,3,2}));
    state.logicalClears=1u<<COMBO_BOSS_GOHMA;
    Rando::logic->CurrentRegionKey=1;
    // Verify this fixture executes the actual native connected-region rejection.
    searching=true;
    assert(Combo_SOH_CrossBossLogicalClear(COMBO_BOSS_GOHMA,0));
    assert(NativeExit().GetConnectedRegionKey()==RR_NONE);
    assert(!NativeExit().ConditionsMet());
    searching=false;
    for(bool previousSearching:{false,true})for(bool previousEvaluating:{false,true}) {
        searching=previousSearching;evaluatingExits=previousEvaluating;
        Rando::logic->CurrentRegionKey=RR_NONE;
        bossCompletion.SetCompletedMask(0);state.logicalClears=1;
        auto exits=nlohmann::json::parse(SOH_GetCrossDungeonExits());
        assert(exits.size()==1 && exits[0]["game"]==1 && exits[0]["index"]==200);
        assert(searching==previousSearching && evaluatingExits==previousEvaluating);
        assert(Rando::logic->CurrentRegionKey==RR_NONE);
        // Physical completion cannot substitute for a missing logical source completion.
        bossCompletion.SetCompletedMask(1);state.logicalClears=0;
        assert(nlohmann::json::parse(SOH_GetCrossDungeonExits()).empty());
        assert(searching==previousSearching && evaluatingExits==previousEvaluating);
        assert(Rando::logic->CurrentRegionKey==RR_NONE);
        throwCondition=true;bool threw=false;
        try { SOH_GetCrossDungeonExits(); }catch(const std::runtime_error&) { threw=true; }
        assert(threw && searching==previousSearching && evaluatingExits==previousEvaluating);
        assert(Rando::logic->CurrentRegionKey==RR_NONE);
        throwCondition=false;
    }
    std::cout<<"OoT exit scope: actual native ConditionsMet/connected-region guard, native AnyAgeTime parent context, logical vs physical clears, all state restorations pass\n";
}
'''
with tempfile.TemporaryDirectory(prefix='oot-exit-scope-') as temp:
    path=Path(temp);(path/'test.cpp').write_text(source)
    cmd=['c++','-std=c++20','-DCOMBO_BUILD','-I'+str(ROOT/'combo'),'-I'+str(ROOT.parent/'build-deps/usr/include')]
    if args.sanitize:cmd+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g']
    subprocess.run(cmd+[str(path/'test.cpp'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
