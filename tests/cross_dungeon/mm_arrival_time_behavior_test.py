#!/usr/bin/env python3
"""Run the actual MM cross-root initialization with native time initialization/expansion."""
import argparse
from pathlib import Path
import subprocess
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import block_from
p=argparse.ArgumentParser();p.add_argument('--sanitize',action='store_true');args=p.parse_args()
logic=(ROOT/'mm/2s2h/Rando/Logic/Logic.cpp').read_text()
header=(ROOT/'mm/2s2h/Rando/Logic/Logic.h').read_text()
time=(ROOT/'mm/2s2h/Rando/Logic/TimeLogic.cpp').read_text()
host=(ROOT/'mm/2s2h/BenPort.cpp').read_text()
def body(text,marker):return block_from(text,text.index(marker))
start=host.index('    for (int index : ComboCrossMm::state.arrivals)')
arrivals=block_from(host,start)
types=header[header.index('enum TimeSlice {'):header.index('// Half-day period definitions')]
source=r'''
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <unordered_map>
#define SPDLOG_WARN(...) ((void)0)
using RandoRegionId=int;
constexpr int RR_MAX=-1,SHOP=1,FROZEN=2,RESTRICTED=3;
namespace Rando::Logic {
'''+types+body(header,'struct RegionTimeState {')+r''';
struct RandoRegion {
    bool canStayOverTime=true;
    std::unordered_map<TimeSlice,std::function<bool()>> timeStayRestrictions;
};
std::map<int,RandoRegion> Regions;
bool clocks=false;uint64_t owned=TIME_ALL_SLICES;
bool SettingClocks(){return clocks;}
bool IsTimeSliceOwned(TimeSlice slice){return owned&(TIME_BIT_ONE<<slice);}
namespace TimeLogic { uint64_t GetOwnedTimeSlices(){return owned;}
'''+body(time,'uint64_t ExpandTimeForward(')+r'''
}
int GetRegionIdFromEntrance(int index){return index-100;}
'''+body(logic,'RegionTimeState InitialTimeState()')+'\n'+body(logic,'std::unordered_map<RandoRegionId, RegionTimeState> InitializeRegionTimeStates(')+'\n'+body(header,'inline RegionTimeState MergeTimeStates(')+r'''
}
namespace ComboCrossMm {
struct { std::set<int> arrivals; } state;
}
void AddCrossArrivals(std::set<RandoRegionId>& reachable,
    std::unordered_map<RandoRegionId,Rando::Logic::RegionTimeState>& timeStates) {
'''+arrivals+r'''
}
int main() {
    using namespace Rando::Logic;
    Regions[SHOP].canStayOverTime=true;Regions[FROZEN].canStayOverTime=false;
    Regions[RESTRICTED].timeStayRestrictions[TIME_DAY1_AM_08_00]=[]{return false;};
    std::set<RandoRegionId> reachable;
    std::unordered_map<RandoRegionId,RegionTimeState> states;
    ComboCrossMm::state.arrivals={100+SHOP,100+FROZEN,100+RESTRICTED};
    AddCrossArrivals(reachable,states);
    assert(states.at(SHOP).canStayOverTime && "foreign shop arrival must inherit native waiting capability");
    assert(!states.at(FROZEN).canStayOverTime);
    auto expand=[&](int region){auto s=states.at(region);return s.canStayOverTime?TimeLogic::ExpandTimeForward(s.timeSlices,Regions.at(region)):s.timeSlices;};
    assert(expand(SHOP)==TIME_ALL_SLICES);
    assert(expand(FROZEN)==(TIME_BIT_ONE<<TIME_DAY1_AM_06_00));
    assert(expand(RESTRICTED)==((TIME_BIT_ONE<<TIME_DAY1_AM_06_00)|(TIME_BIT_ONE<<TIME_DAY1_AM_07_00)));
    states[SHOP]={TIME_BIT_ONE<<TIME_DAY3_AM_06_00,false};
    AddCrossArrivals(reachable,states);
    assert(states.at(SHOP).timeSlices&(TIME_BIT_ONE<<TIME_DAY3_AM_06_00));
    assert(states.at(SHOP).canStayOverTime);
    clocks=true;owned=(TIME_BIT_ONE<<TIME_DAY2_AM_06_00)|(TIME_BIT_ONE<<TIME_DAY2_AM_07_00);
    states.clear();AddCrossArrivals(reachable,states);
    assert(expand(SHOP)==owned && expand(FROZEN)==owned);
    assert(InitializeRegionTimeStates(RR_MAX).at(RR_MAX).canStayOverTime==false);
    std::cout<<"MM arrival roots: native waiting, closing restrictions, existing time merge and owned clocks PASS\n";
}
'''
with tempfile.TemporaryDirectory(prefix='mm-arrival-time-') as temp:
    path=Path(temp);(path/'test.cpp').write_text(source)
    cmd=['c++','-std=c++20','-Wall','-Wextra','-Werror',str(path/'test.cpp'),'-o',str(path/'test')]
    if args.sanitize:cmd+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g']
    subprocess.run(cmd,check=True);subprocess.run([str(path/'test')],check=True)
