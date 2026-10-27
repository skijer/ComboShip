#!/usr/bin/env python3
"""Exercise exclusive shop selection and legacy replay through the production OoT catalogue."""
from pathlib import Path
import os
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[2]
# Use the same production-native descriptor harness, replacing only its assertions.
base = ROOT / 'tests/cross_dungeon/oot_catalogue_behavior_test.py'
env = {'__file__': str(base)}
exec(compile(base.read_text().split('with tempfile.TemporaryDirectory')[0], str(base), 'exec'), env)
source = env['source'].split('int main() {')[0] + r'''
int main() {
    auto options=ParseOptions({{"dungeons",false},{"shops",true}});
    const std::set<int> shops={ENTR_KOKIRI_SHOP_0,ENTR_GORON_SHOP_0,ENTR_ZORA_SHOP_0,
        ENTR_BAZAAR_0,ENTR_BAZAAR_1,ENTR_POTION_SHOP_MARKET_0,ENTR_BOMBCHU_SHOP_1,
        ENTR_POTION_SHOP_GRANNY_0,ENTR_POTION_SHOP_KAKARIKO_FRONT,ENTR_POTION_SHOP_KAKARIKO_BACK};
    auto catalogue=nlohmann::json::parse(CatalogueJson(false,{},options));
    assert(catalogue.size()==shops.size() && "all retail shops must participate, including open entrances");
    for(const auto& raw:catalogue) {
        assert(shops.count(raw.at("entrance").at("index")));
        assert(raw.at("pool")==static_cast<int>(Pool::Shop));
    }
    for(bool broad:{false,true})for(bool doors:{false,true})for(bool shuffleShops:{false,true}) {
        options.interiors=broad;options.doorInteriors=doors;options.shops=shuffleShops;
        catalogue=nlohmann::json::parse(CatalogueJson(false,{},options));
        size_t seen=0;
        for(const auto& raw:catalogue) {
            const auto index=raw.at("entrance").at("index").get<int>();
            if(shops.count(index)) { ++seen;assert(shuffleShops);assert(raw.at("pool")==4); }
            else assert(raw.at("pool")!=4);
            assert(index!=ENTR_HAPPY_MASK_SHOP_0);
        }
        assert(seen==(shuffleShops?shops.size():0));
    }
    // Reusing an older native map must never reclassify a house as a shop.
    options.interiors=false;options.doorInteriors=true;options.shops=true;
    const std::map<int32_t,int32_t> mixedNative={
        {ENTR_BAZAAR_1,ENTR_IMPAS_HOUSE_FRONT},{ENTR_IMPAS_HOUSE_FRONT,ENTR_BAZAAR_1},
        {ENTR_MARKET_DAY_OUTSIDE_BAZAAR,ENTR_KAKARIKO_VILLAGE_OUTSIDE_IMPAS_HOUSE_FRONT},
        {ENTR_KAKARIKO_VILLAGE_OUTSIDE_IMPAS_HOUSE_FRONT,ENTR_MARKET_DAY_OUTSIDE_BAZAAR}};
    for(const auto& raw:nlohmann::json::parse(CatalogueJson(false,mixedNative,options)))
        assert((raw.at("pool")==4)==bool(shops.count(raw.at("entrance").at("index"))) &&
               "a nonretail native target must not inherit retail pool membership");
    // Versioned old records retain the seven-door catalogue and broad precedence.
    const auto oldShops=OptionsFromSeed({{"version",3},{"options",{{"dungeons",false},{"shops",true}}}});
    const auto legacy=nlohmann::json::parse(CatalogueJson(false,{},oldShops));
    assert(legacy.size()==7);
    for(const auto& raw:legacy) {
        const int index=raw.at("entrance").at("index");
        assert(index!=ENTR_KOKIRI_SHOP_0 && index!=ENTR_GORON_SHOP_0 && index!=ENTR_ZORA_SHOP_0);
    }
    for(int version:{2,3}) {
        const auto oldBroad=OptionsFromSeed({{"version",version},{"options",{{"dungeons",false},{"interiors",true}}}});
        const auto broad=nlohmann::json::parse(CatalogueJson(false,{},oldBroad));
        assert(broad.size()==40);
        for(const auto& raw:broad)assert(raw.at("pool")==2);
        assert(CatalogueJson(false,{},ParseOptions(OptionsJson(oldBroad)))==broad.dump());
    }
    std::cout<<"OoT shops: exact ten portals, open shops, exclusive toggle across broad/door combinations PASS\n";
}
'''
with tempfile.TemporaryDirectory(prefix='shop-catalogue-') as tmp:
    path=Path(tmp);(path/'test.cpp').write_text(source)
    cmd=[os.environ.get('CXX','c++'),'-std=c++20','-O1','-DCOMBO_BUILD','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t','-DLOG_LEVEL_GAME_PRINTS=0']
    for p in ('combo','soh','soh/include','soh/include/PR','soh/src','soh/assets','soh/mods','soh/soh','libultraship/include'):
        cmd+=['-I'+str(ROOT/p)]
    cmd+=['-I'+str(ROOT.parent/'build-deps/usr/include')]
    if env['args'].sanitize:cmd+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g']
    subprocess.run(cmd+[str(path/'test.cpp'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
