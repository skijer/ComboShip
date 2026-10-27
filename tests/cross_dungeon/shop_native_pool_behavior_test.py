#!/usr/bin/env python3
"""Exercise native generation pool/target composition using the production bodies."""
from pathlib import Path
import os
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
base=ROOT/'tests/cross_dungeon/oot_catalogue_behavior_test.py'
env={'__file__':str(base)}
exec(compile(base.read_text().split('with tempfile.TemporaryDirectory')[0],str(base),'exec'),env)
native=env['native']
source=env['source'].split('int main() {')[0]
start=native.index('    // Interior entrances',native.index('int EntranceShuffler::ShuffleAllEntrances'))
end=native.index("    // Thieves' Hideout entrances",start)
interior=native[start:end]
start=native.index('static std::vector<Entrance*>\nBuildOneWayTargets')
opening=native.index('{',native.index('/*, target_region_names*/',start));depth=1;end=opening+1
while depth:
    depth+=(native[end]=='{')-(native[end]=='}');end+=1
oneway=native[start:end]
helper=''
if 'static bool IsComboRetailShopEndpoint' in native:
    helper=env['body'](native,'IsComboRetailShopEndpoint')
source+=r'''
namespace Rando {
struct Entrance {
    int index,parent,connected;EntranceType type;Entrance* reverse=nullptr;
    int GetIndex()const{return index;}
    RandomizerRegion GetParentRegionKey()const{return static_cast<RandomizerRegion>(parent);}
    RandomizerRegion GetConnectedRegionKey()const{return static_cast<RandomizerRegion>(connected);}
    Entrance* GetReverse()const{return reverse;}
    Entrance* GetNewTarget(){return this;}
};
using EntrancePools=std::map<EntranceType,std::vector<Entrance*>>;
static int shuffleInteriors=0;static bool decoupled=false;
struct Option {
    int value=0;bool Is(int other)const{return value==other;}
    bool IsNot(int other)const{return value!=other;}
    operator bool()const{return value!=0;}
};
struct Context {
    Option GetOption(int key)const {
        return {key==RSK_SHUFFLE_INTERIOR_ENTRANCES?shuffleInteriors:
                key==RSK_DECOUPLED_ENTRANCES?int(decoupled):key==RSK_EXCLUDE_MASK_SHOP_ENTRANCE?1:0};
    }
};
std::vector<Entrance> entries;
void Init() {
    entries.reserve(GetEntranceShuffleTable().size()*2);
    for(const auto& pair:GetEntranceShuffleTable()) {
        for(const auto& p:{pair.first,pair.second})entries.push_back({p.index,p.parentRegion,p.connectedRegion,p.type});
        auto* first=&entries[entries.size()-2];auto* second=&entries.back();
        first->reverse=second;second->reverse=first;
    }
}
std::vector<Entrance*> GetShuffleableEntrances(EntranceType type,bool primary=true) {
    std::vector<Entrance*> result;
    for(size_t i=0;i<entries.size();++i)
        if(entries[i].index>=0 && entries[i].type==type && (!primary || i%2==0))result.push_back(&entries[i]);
    return result;
}
}
namespace SohUtils {
template<class T>void AppendVector(std::vector<T>& to,const std::vector<T>& from){to.insert(to.end(),from.begin(),from.end());}
template<class T>bool Contains(const T& value,const std::vector<T>& values){return std::find(values.begin(),values.end(),value)!=values.end();}
}
namespace Rando {
'''+helper+'\n'+oneway+r'''
EntrancePools NativeInteriorPools() {
    Context context;auto* ctx=&context;EntrancePools entrancePools;
'''+interior+r'''
    return entrancePools;
}
}
int main() {
    using namespace Rando;Init();
    const std::set<int> shops={ENTR_KOKIRI_SHOP_0,ENTR_GORON_SHOP_0,ENTR_ZORA_SHOP_0,
        ENTR_BAZAAR_0,ENTR_BAZAAR_1,ENTR_POTION_SHOP_MARKET_0,ENTR_BOMBCHU_SHOP_1,
        ENTR_POTION_SHOP_GRANNY_0,ENTR_POTION_SHOP_KAKARIKO_FRONT,ENTR_POTION_SHOP_KAKARIKO_BACK};
    std::set<int> endpoints=shops;
    for(const auto& pair:GetEntranceShuffleTable())if(shops.count(pair.first.index))endpoints.insert(pair.second.index);
    for(int mode:{RO_INTERIOR_ENTRANCE_SHUFFLE_SIMPLE,RO_INTERIOR_ENTRANCE_SHUFFLE_ALL})
        for(bool separate:{false,true}) {
            shuffleInteriors=mode;decoupled=separate;
            const auto pools=NativeInteriorPools();size_t count=0;
            for(const auto& [type,pool]:pools)for(const auto* entry:pool) {
                (void)type;++count;
                assert(!endpoints.count(entry->GetIndex()) &&
                       "native Interior shuffle must not take retail shop entrances or returns");
            }
            assert(count>0 && "ordinary native interiors must still shuffle");
        }
    for(const auto* target:BuildOneWayTargets({EntranceType::Interior,EntranceType::SpecialInterior}))
        assert(!endpoints.count(target->GetIndex()) && "native spawn/warp targets must not inherit shop ownership");
    std::cout<<"native shop exclusion: simple/all, coupled/decoupled and spawn/warp targets PASS\n";
}
'''
with tempfile.TemporaryDirectory(prefix='shop-native-pool-') as tmp:
    path=Path(tmp);(path/'test.cpp').write_text(source)
    cmd=[os.environ.get('CXX','c++'),'-std=c++20','-O1','-DCOMBO_BUILD','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t','-DLOG_LEVEL_GAME_PRINTS=0']
    for p in ('combo','soh','soh/include','soh/include/PR','soh/src','soh/assets','soh/mods','soh/soh','libultraship/include'):
        cmd+=['-I'+str(ROOT/p)]
    cmd+=['-I'+str(ROOT.parent/'build-deps/usr/include')]
    if env['args'].sanitize:cmd+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g']
    subprocess.run(cmd+[str(path/'test.cpp'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
