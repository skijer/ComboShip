#!/usr/bin/env python3
"""Run native metadata setup with redirected and missing Bottom of the Well returns."""
import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_time_pedestal_tests import block_from

parser = argparse.ArgumentParser()
parser.add_argument('--sanitize', action='store_true')
args = parser.parse_args()
if args.sanitize:
    os.environ['ASAN_OPTIONS'] = 'detect_leaks=0'
native = (ROOT / 'soh/soh/Enhancements/randomizer/entrance.cpp').read_text()
access = (ROOT / 'soh/soh/Enhancements/randomizer/location_access.cpp').read_text()
setup = block_from(native, native.index('void SetAllEntrancesData()'))
get_exit = block_from(access, access.index('Rando::Entrance* Region::GetExit('))
get_original = block_from(access, access.index('Rando::Entrance* GetEntrance('))
source = r'''
#include <cassert>
#include <cstdint>
#include <iostream>
#include <list>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
using RandomizerRegion = int;
constexpr int RR_NONE=0, RR_KAK_WELL=163, RR_BOTW_ENTRYWAY=244;
constexpr int RSK_DECOUPLED_ENTRANCES=0;
#define SPDLOG_ERROR(...) ((void)0)
namespace Rando {
enum class EntranceType { None, Dungeon };
struct Entrance {
    int original, connected, index=-1;
    EntranceType type=EntranceType::None;
    Entrance* reverse=nullptr;
    bool primary=false, decoupled=false;
    int GetConnectedRegionKey() const { return connected; }
    int GetOriginalConnectedRegionKey() const { return original; }
    void SetIndex(int value) { index=value; }
    void SetType(EntranceType value) { type=value; }
    void SetAsPrimary() { primary=true; }
    void SetDecoupled() { decoupled=true; }
    void BindTwoWay(Entrance* other) { reverse=other; other->reverse=this; }
};
struct Context {
    bool decoupled=false;
    static Context* GetInstance() { static Context ctx; return &ctx; }
    bool GetOption(int) const { return decoupled; }
};
struct EntranceLinkInfo { EntranceType type; int parentRegion, connectedRegion; int16_t index; };
std::map<int,Entrance*> entranceMap;
const auto& GetEntranceShuffleTable() {
    static const std::vector<std::pair<EntranceLinkInfo,EntranceLinkInfo>> table = {
        {{EntranceType::Dungeon,RR_KAK_WELL,RR_BOTW_ENTRYWAY,0x98},
         {EntranceType::Dungeon,RR_BOTW_ENTRYWAY,RR_KAK_WELL,0x2a6}}
    };
    return table;
}
}
struct Region {
    std::string regionName="test region";
    std::list<Rando::Entrance> exits;
    Rando::Entrance* GetExit(int);
};
std::map<int,Region> regions;
Region* RegionTable(int key) { return &regions[key]; }
''' + get_exit + '\n' + get_original + '\nnamespace Rando {\n' + setup + r'''
}
int main() {
    using namespace Rando;
    for (bool decoupled : {false,true}) for (bool redirected : {false,true}) {
        regions.clear(); entranceMap.clear(); Context::GetInstance()->decoupled=decoupled;
        regions[RR_KAK_WELL].exits.push_back({RR_BOTW_ENTRYWAY,redirected?RR_NONE:RR_BOTW_ENTRYWAY});
        regions[RR_BOTW_ENTRYWAY].exits.push_back({RR_KAK_WELL,redirected?777:RR_KAK_WELL});
        SetAllEntrancesData();
        auto& forward=regions[RR_KAK_WELL].exits.front();
        auto& reverse=regions[RR_BOTW_ENTRYWAY].exits.front();
        assert(forward.index==0x98 && reverse.index==0x2a6);
        assert(forward.reverse==&reverse && reverse.reverse==&forward);
        assert(forward.primary && forward.decoupled==decoupled && reverse.decoupled==decoupled);
        assert(forward.connected==(redirected?RR_NONE:RR_BOTW_ENTRYWAY));
        assert(reverse.connected==(redirected?777:RR_KAK_WELL));
    }
    // The dump's missing return must reject setup before dereferencing or publishing half a pair.
    regions.clear(); entranceMap.clear();
    regions[RR_KAK_WELL].exits.push_back({RR_BOTW_ENTRYWAY,RR_BOTW_ENTRYWAY});
    bool rejected=false;
    try { SetAllEntrancesData(); } catch (const std::runtime_error& error) {
        rejected=std::string(error.what()).find("678")!=std::string::npos;
    }
    assert(rejected && "missing Bottom of the Well return must report entrance 678");
    assert(entranceMap.empty() && regions[RR_KAK_WELL].exits.front().index==-1);
    std::cout<<"OoT metadata: stable edge identity, preserved routing, checked missing return pass\n";
}
'''
with tempfile.TemporaryDirectory(prefix='oot-metadata-') as tmp:
    path = Path(tmp)
    (path / 'test.cpp').write_text(source)
    cmd = ['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror']
    if args.sanitize:
        cmd += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-g']
    subprocess.run(cmd + [str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
