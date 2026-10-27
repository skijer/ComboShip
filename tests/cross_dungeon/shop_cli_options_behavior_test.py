#!/usr/bin/env python3
"""Exercise actual headless replay/new-map selection with production native catalogue and codec."""
from pathlib import Path
import os
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
base=ROOT/'tests/cross_dungeon/oot_catalogue_behavior_test.py'
env={'__file__':str(base)}
exec(compile(base.read_text().split('with tempfile.TemporaryDirectory')[0],str(base),'exec'),env)
headless=(ROOT/'combo/ComboRandoHeadless.cpp').read_text()
start=headless.index('    ComboRando::CrossDungeon::PoolOptions crossOptions;')
end=headless.index('    // Shared Items: --shared-items wins',start)
selection=headless[start:end]
source=env['source'].split('int main() {')[0]+r'''
#include "rando/CrossDungeonCoordinator.h"
#include <array>
namespace CliFixture {
using namespace ComboRando::CrossDungeon;
static const char* (*SOH_ReadCrossEntranceOptions)()=nullptr;
static int (*SOH_ReadCrossDungeonCVar)()=nullptr;
struct Selection { PoolOptions options;bool enabled,replay; };
Selection Select(const std::optional<nlohmann::json>& settingsCrossSeed,const std::array<int,7>& crossPoolArgs) {
    Coordinator crossDungeons;
'''+selection+r'''
    return {crossDungeons.selectedPools,crossDungeonEnabled,replayCrossMap};
}
}
std::vector<Dungeon> NativeCatalogue(const PoolOptions& options) {
    std::vector<Dungeon> result;
    for(const auto& raw:nlohmann::json::parse(ComboCrossOot::CatalogueJson(false,{},options)))
        result.push_back(ParseDungeon(raw));
    return result;
}
bool Replayable(const nlohmann::json& seed) {
    try {
        ComboRando::CrossDungeon::Coordinator::ValidateCatalogue(seed,NativeCatalogue(OptionsFromSeed(seed)));
        return !Decode(seed).empty();
    } catch(const std::exception&) { return false; }
}
int main() {
    using namespace CliFixture;
    const std::array<int,7> unchanged={-1,-1,-1,-1,-1,-1,-1};
    for(int version:{2,3}) {
        auto oldOptions=ParseOptions({{"dungeons",false},{"interiors",true},{"legacyInteriors",true}});
        auto recorded=GenerateSeed(NativeCatalogue(oldOptions),51);recorded["version"]=version;
        assert(recorded.at("dungeons").size()==40 && Replayable(recorded));
        auto selected=Select(recorded,unchanged);
        assert(selected.replay && selected.enabled && selected.options.legacyInteriors);
        assert(OptionsJson(selected.options)==OptionsJson(OptionsFromSeed(recorded)) &&
               "an untouched old-settings file must replay its exact membership and map");
        auto identical=unchanged;identical[3]=1;
        selected=Select(recorded,identical);
        assert(selected.replay && selected.options.legacyInteriors);
        // These options request a newly generated layout, not exact-map replay.
        for(size_t changed:{size_t(4),size_t(5)}) {
            auto overrides=unchanged;overrides[changed]=1;
            selected=Select(recorded,overrides);
            assert(!selected.replay && selected.enabled);
            const auto fresh=GenerateSeed(NativeCatalogue(selected.options),72);
            assert(fresh.at("version")==4);
            assert(Replayable(fresh) &&
                   "CLI overrides of legacy settings must produce a v4 catalogue that validates on reload");
            assert(!selected.options.legacyInteriors);
            for(const auto& raw:fresh.at("dungeons")) {
                const int entrance=raw.at("entrance").at("index");
                if(IsOotRetailShopEntrance(entrance))assert(changed==5 && raw.at("pool")==4);
            }
        }
        auto disabled=unchanged;disabled[3]=0;
        selected=Select(recorded,disabled);
        assert(!selected.replay && !selected.enabled && !selected.options.legacyInteriors);
    }
    // Version 3's door-only shop setting becomes the full retail set on fresh generation.
    auto oldShops=ParseOptions({{"dungeons",false},{"shops",true},{"legacyInteriors",true}});
    auto recorded=GenerateSeed(NativeCatalogue(oldShops),81);recorded["version"]=3;
    assert(recorded.at("dungeons").size()==7);
    auto overrides=unchanged;overrides[3]=1;
    auto selected=Select(recorded,overrides);
    assert(!selected.replay && !selected.options.legacyInteriors);
    const auto fresh=GenerateSeed(NativeCatalogue(selected.options),82);
    assert(Replayable(fresh) && fresh.at("options").at("shops")==true);
    size_t shops=0;for(const auto& raw:fresh.at("dungeons"))shops+=raw.value("pool",0)==4;
    assert(shops==10);
    std::cout<<"headless legacy settings: unchanged exact replay, changed flags produce reloadable v4 retail policy PASS\n";
}
'''
with tempfile.TemporaryDirectory(prefix='shop-cli-options-') as tmp:
    path=Path(tmp);(path/'test.cpp').write_text(source)
    cmd=[os.environ.get('CXX','c++'),'-std=c++20','-O1','-DCOMBO_BUILD','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t','-DLOG_LEVEL_GAME_PRINTS=0']
    for p in ('combo','soh','soh/include','soh/include/PR','soh/src','soh/assets','soh/mods','soh/soh','libultraship/include'):
        cmd+=['-I'+str(ROOT/p)]
    cmd+=['-I'+str(ROOT.parent/'build-deps/usr/include')]
    if env['args'].sanitize:cmd+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g']
    subprocess.run(cmd+[str(path/'test.cpp'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
