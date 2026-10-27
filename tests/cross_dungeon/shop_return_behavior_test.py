#!/usr/bin/env python3
"""Run shop returns through actual OoT native functions and the production cross runtime."""
from pathlib import Path
import re
import os
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[2]
base = ROOT / 'tests/cross_dungeon/oot_pool_runtime_behavior_test.py'
env = {'__file__': str(base)}
exec(compile(base.read_text().split('with tempfile.TemporaryDirectory')[0], str(base), 'exec'), env)
source = env['source'].split('int main() {')[0]
catalogue_base=ROOT/'tests/cross_dungeon/oot_catalogue_behavior_test.py'
catalogue_env={'__file__':str(catalogue_base)}
exec(compile(catalogue_base.read_text().split('with tempfile.TemporaryDirectory')[0],str(catalogue_base),'exec'),catalogue_env)
catalogue_source=catalogue_env['source'].split('int main() {')[0]
catalogue_source=catalogue_source.removesuffix('using namespace ComboCrossOot;\nusing namespace ComboRando::CrossDungeon;\n')
# Both reusable fixtures now provide solo-mode state. Keep one definition when
# compiling their catalogue and runtime production bodies in the same unit.
source=catalogue_source+source.replace('extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }\n', '', 1)
native = (ROOT / 'soh/soh/Enhancements/randomizer/randomizer_entrance.c').read_text()
def body(name):
    start = re.search(r'^s16 '+name+r'\(', native, re.M).start()
    opening = native.index('{', start); depth = 1; end = opening + 1
    while depth:
        depth += (native[end] == '{') - (native[end] == '}'); end += 1
    return native[start:end]
source += '\nstatic s16 entranceOverrideTable[ENTRANCE_TABLE_SIZE];\n'
source += re.search(r's16 dynamicExitList\[\] = \{.*?\};', native, re.S).group() + '\n'
source += '\n'.join(body(name) for name in ('Entrance_GetOverride', 'Entrance_OverrideNextIndex', 'Entrance_OverrideDynamicExit'))
scene=(ROOT/'mm/include/z64scene.h').read_text()
mm_scenes={name:int(value,16) for value,name in re.findall(r'/\*\s*(0x[0-9A-Fa-f]+)\s*\*/\s*(ENTR_SCENE_\w+)',scene)}
source += f'\nconstexpr int MM_BOMB_SHOP={mm_scenes["ENTR_SCENE_BOMB_SHOP"]<<9};\n'
source += f'constexpr int MM_BOMB_RETURN={(mm_scenes["ENTR_SCENE_WEST_CLOCK_TOWN"]<<9)|(6<<4)};\n'
source += r'''
int main() {
    PlayState play{};gPlayState=&play;gSaveContext.fileNum=1;gSaveContext.linkAge=LINK_AGE_ADULT;
    Grotto_InitExitAndLoadLists();
    for(int i=0;i<ENTRANCE_TABLE_SIZE;++i)entranceOverrideTable[i]=i;
    PoolOptions shopOptions;shopOptions.dungeons=false;shopOptions.shops=true;
    const auto catalogue=nlohmann::json::parse(ComboCrossOot::CatalogueJson(false,{},shopOptions));
    const auto portal=[&](int entrance) {
        for(const auto& raw:catalogue)if(raw.at("entrance").at("index")==entrance)return ParseDungeon(raw);
        throw std::runtime_error("production shop catalogue omitted a required portal");
    };
    Dungeon bazaar=portal(ENTR_BAZAAR_1),front=portal(ENTR_POTION_SHOP_KAKARIKO_FRONT),back=portal(ENTR_POTION_SHOP_KAKARIKO_BACK);
    // Actual MM ENTRANCE(BOMB_SHOP,0) / ENTRANCE(WEST_CLOCK_TOWN,6) identities.
    Dungeon bomb{"mm:interior:bomb_shop:0",{1,MM_BOMB_SHOP},{1,MM_BOMB_RETURN},{1,MM_BOMB_RETURN}};
    for(auto* entry:{&bazaar,&front,&back,&bomb})entry->pool=Pool::Shop;
    for(auto* entry:{&front,&back})
        for(int layer=0;layer<4;++layer)gEntranceTable[entry->entrance.index+layer].scene=SCENE_POTION_SHOP_KAKARIKO;
    gEntranceTable[bazaar.entrance.index].scene=SCENE_BAZAAR;
    const auto seed=Encode({bazaar,front,back,bomb},{2,0,3,1});
    assert(SOH_SetCrossDungeonRuntimeSeed(seed.dump().c_str()));
    // Market Bazaar enters the rear potion door; the front door independently belongs to Bomb Shop.
    assert(Entrance_OverrideNextIndex(ENTR_BAZAAR_1)==ENTR_POTION_SHOP_KAKARIKO_BACK);
    gSaveContext.entranceIndex=ENTR_POTION_SHOP_KAKARIKO_BACK;play.sceneNum=SCENE_POTION_SHOP_KAKARIKO;play.curSpawn=2;
    ObserveScene(play.sceneNum);
    assert(Combo_SOH_SaveCrossDungeonCheckpoint()==ENTR_POTION_SHOP_KAKARIKO_BACK);
    // Returning through the entry's own rear exit must retain Market Bazaar.
    assert(Entrance_OverrideNextIndex(ENTR_KAKARIKO_VILLAGE_OUTSIDE_POTION_SHOP_BACK)==ENTR_MARKET_DAY_OUTSIDE_BAZAAR);
    assert(!Combo_SOH_CrossDungeonTransitionPending());
    // The other physical door keeps its own route; traversing the shop must remain possible.
    assert(Entrance_OverrideDynamicExit(3)==ENTR_KAKARIKO_VILLAGE_OUTSIDE_POTION_SHOP_FRONT);
    assert(pending && pending->to==bomb.returnTo);
    Combo_SOH_CancelCrossDungeonTransition();
    // A different visit through the front still owns its own peer return.
    assert(SOH_QueueCrossDungeonArrival(ENTR_POTION_SHOP_KAKARIKO_FRONT));
    assert(Combo_SOH_ConsumeCrossDungeonArrival()==ENTR_POTION_SHOP_KAKARIKO_FRONT);
    gSaveContext.entranceIndex=ENTR_POTION_SHOP_KAKARIKO_FRONT;play.curSpawn=0;
    ObserveScene(play.sceneNum);
    assert(Entrance_OverrideDynamicExit(3)==ENTR_KAKARIKO_VILLAGE_OUTSIDE_POTION_SHOP_FRONT);
    assert(pending && pending->to==bomb.returnTo);
    Combo_SOH_CancelCrossDungeonTransition();
    assert(Entrance_OverrideNextIndex(ENTR_KAKARIKO_VILLAGE_OUTSIDE_POTION_SHOP_BACK)==ENTR_MARKET_DAY_OUTSIDE_BAZAAR);
    // Sweep real catalogue permutations through the native pre/post lookup bodies.
    // Legacy v3 also composes a coupled native shop cycle before the cross overlay.
    for(bool legacy:{false,true}) {
        auto options=shopOptions;options.legacyInteriors=legacy;
        std::map<int32_t,int32_t> nativeMap;
        if(legacy) {
            const std::vector<Dungeon> cycle={bazaar,front,back};
            for(size_t i=0;i<cycle.size();++i) {
                nativeMap[cycle[i].entrance.index]=cycle[(i+1)%cycle.size()].entrance.index;
                nativeMap[cycle[(i+1)%cycle.size()].exit.index]=cycle[i].exit.index;
            }
        }
        for(int i=0;i<ENTRANCE_TABLE_SIZE;++i)entranceOverrideTable[i]=i;
        for(const auto& [from,to]:nativeMap)entranceOverrideTable[from]=to;
        std::vector<Dungeon> entries;
        for(const auto& raw:nlohmann::json::parse(ComboCrossOot::CatalogueJson(false,nativeMap,options)))
            entries.push_back(ParseDungeon(raw));
        entries.push_back(bomb);
        const auto nativeExit=[](int index) {
            for(size_t i=0;i<sizeof(dynamicExitList)/sizeof(*dynamicExitList);++i)
                if(dynamicExitList[i]==index)return Entrance_OverrideDynamicExit(i);
            return Entrance_OverrideNextIndex(index);
        };
        for(uint64_t roll=0;roll<32;++roll)for(int age:{LINK_AGE_CHILD,LINK_AGE_ADULT}) {
            gSaveContext.linkAge=age;
            auto recorded=GenerateSeed(entries,roll);if(legacy)recorded["version"]=3;
            assert(SOH_SetCrossDungeonRuntimeSeed(recorded.dump().c_str()));
            RuntimeState mm(1);mm.Install(recorded);
            const auto routes=Decode(recorded);
            for(const auto& physical:entries) {
                if(physical.entrance.game!=0)continue;
                const int resolved=ResolveNativeIndex(physical.entrance.index,physical.entrance.index,nativeMap);
                const auto target=Resolve(routes,Phase::AfterNative,{0,resolved});
                const auto slot=std::find_if(entries.begin(),entries.end(),[&](const Dungeon& d){return d.entrance==Endpoint{0,resolved};});
                assert(slot!=entries.end());
                const auto destination=std::find_if(entries.begin(),entries.end(),[&](const Dungeon& d){return d.entrance==target;});
                assert(destination!=entries.end());
                const int result=Entrance_OverrideNextIndex(physical.entrance.index);
                if(target.game==0) {
                    assert(result==target.index && !pending);
                    assert(nativeExit(destination->exit.index)==slot->returnTo.index && !pending &&
                           "same physical exit must return to its actual production source after native composition");
                } else {
                    assert(pending && pending->to==target);
                    Combo_SOH_CancelCrossDungeonTransition();
                    const auto reverse=mm.Lookup(Phase::BeforeNative,destination->exit.index);
                    assert(reverse && reverse->to==slot->returnTo);
                }
            }
        }
    }
    std::cout<<"shop returns: production catalogue, 128 age/native layouts, exact front/back returns and independent other door PASS\n";
}
'''
with tempfile.TemporaryDirectory(prefix='shop-return-') as tmp:
    path=Path(tmp);(path/'test.cpp').write_text(source)
    cmd=[os.environ.get('CXX','c++'),'-std=c++20','-O1','-Wno-narrowing','-DCOMBO_BUILD','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t','-DLOG_LEVEL_GAME_PRINTS=0']
    for p in ('combo','soh','soh/include','soh/include/PR','soh/src','soh/assets','soh/mods','soh/soh','libultraship/include'):
        cmd+=['-I'+str(ROOT/p)]
    cmd+=['-I'+str(ROOT.parent/'build-deps/usr/include')]
    if env['args'].sanitize:cmd+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g']
    subprocess.run(cmd+[str(path/'test.cpp'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
