#!/usr/bin/env python3
"""Exercise both real native-pair policies with actual Fast resource types."""
from pathlib import Path
import os
import subprocess
import tempfile
from run_native_priority_tests import function

ROOT = Path(__file__).resolve().parents[2]


def main():
    oot=(ROOT/'soh/soh/ResourceManagerHelpers.cpp').read_text()
    mm=(ROOT/'mm/2s2h/BenPort.cpp').read_text()
    fixture=(ROOT/'tests/nei_asset_priority/tunic_model_available_test.cpp').read_text()
    fixture=fixture[:fixture.index('int main() {')]
    scope=(ROOT/'libultraship/include/ship/resource/ResourceManagerScope.h').read_text()
    fixture=fixture.replace('/* PRODUCTION_SCOPE */',scope[scope.index('namespace Ship {'):])
    fixture=fixture.replace('/* PRODUCTION_QUERY */',function(oot,'ResourceMgr_IsGiModelAvailableForGame'))
    fixture += r'''
#include "combo/ComboGameMode.h"
#include "combo/menu/ComboIkanaShieldResources.h"
#include <set>
#ifdef COMBO_BUILD
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
#endif
static bool optionalAvailable;
static std::set<std::string> optionalPasses;
int MmAssets_IsAvailable() { return optionalAvailable; }
void* MmAssets_LoadDisplayListGraphStrict(const char* path) {
    static int token;
    return optionalPasses.contains(path) ? &token : nullptr;
}
'''
    fixture += function(oot,'ResourceMgr_IsIkanaShieldGiAvailable')+'\n'
    fixture += function(mm,'ResourceMgr_IsIkanaShieldGiAvailable').replace(
        'ResourceMgr_IsIkanaShieldGiAvailable','MmNativePairAvailable')+'\n'
    fixture += r'''
int main() {
    auto oot=std::make_shared<Ship::ResourceManager>();
    auto mm=std::make_shared<Ship::ResourceManager>();
    Ship::CrossRMRegistry::owners={{"oot",oot},{"mm",mm}};
    Ship::context.active=oot;
    auto model=std::make_shared<Fast::DisplayList>();
    model->Instructions={gsSPEndDisplayList()};
    auto empty=std::make_shared<Fast::DisplayList>();
    auto texture=std::make_shared<Fast::Texture>();
    const std::string opa="objects/object_gi_shield_3/gGiMirrorShieldEmptyDL";
    const std::string xlu="objects/object_gi_shield_3/gGiMirrorShieldDL";
    oot->files[opa]=model; oot->files[xlu]=model; // wrong-owner distractors
    mm->files[opa]=model; mm->files[xlu]=model;
    for(bool hostAlt:{false,true})for(bool ownerAlt:{false,true}) {
        oot->alt=hostAlt; mm->alt=ownerAlt;
        assert(ResourceMgr_IsIkanaShieldGiAvailable() && MmNativePairAvailable());
        assert(Ship::context.active==oot);
    }
    mm->alt=true;
    for(const auto& bad:{std::static_pointer_cast<Ship::IResource>(texture),
                         std::static_pointer_cast<Ship::IResource>(empty)}) {
        mm->files["alt/"+xlu]=bad;
        assert(!ResourceMgr_IsIkanaShieldGiAvailable() && !MmNativePairAvailable());
        assert(Ship::context.active==oot);
    }
    mm->files.erase("alt/"+xlu); mm->files.erase(xlu);
    optionalAvailable=true; optionalPasses={COMBO_IKANA_GI_OPA,COMBO_IKANA_GI_XLU};
    assert(!ResourceMgr_IsIkanaShieldGiAvailable() && !MmNativePairAvailable());
    mm->files[xlu]=model;
    assert(ResourceMgr_IsIkanaShieldGiAvailable() && MmNativePairAvailable());
    Ship::CrossRMRegistry::owners.erase("mm");
#ifdef COMBO_BUILD
    assert(!ResourceMgr_IsIkanaShieldGiAvailable() && "combined mode must not queue an absent MM owner");
    assert(!MmNativePairAvailable());
    gComboGameMode=COMBO_GAME_OOT;
#endif
    assert(ResourceMgr_IsIkanaShieldGiAvailable());
    optionalPasses.erase(COMBO_IKANA_GI_XLU);
    assert(!ResourceMgr_IsIkanaShieldGiAvailable());
    optionalPasses.insert(COMBO_IKANA_GI_XLU);
    assert(ResourceMgr_IsIkanaShieldGiAvailable() && "late optional availability must retry");
    optionalAvailable=false;
    assert(!ResourceMgr_IsIkanaShieldGiAvailable());
    assert(Ship::context.active==oot);
    std::cout<<"PASS real native GI pair: correct owner, typed/nonempty passes, independent Alt, missing/late resources, combined guard and strict Solo fallback\n";
}
'''
    with tempfile.TemporaryDirectory(prefix='ikana-native-availability-') as temporary:
        path=Path(temporary);source=path/'test.cpp';source.write_text(fixture)
        (path/'spdlog').mkdir()
        (path/'spdlog/spdlog.h').write_text('#pragma once\n#define SPDLOG_TRACE(...) ((void)0)\n')
        for name,mode in [('combo',['-DCOMBO_BUILD']),('standalone',[])]:
            binary=path/name
            subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-DF3DEX_GBI_2','-DFMT_HEADER_ONLY',
                '-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_OFF','-Wall','-Wextra','-Werror',
                '-I'+str(path),'-I'+str(ROOT),'-I'+str(ROOT/'libultraship/include'),*mode,str(source),
                str(ROOT/'libultraship/src/ship/resource/Resource.cpp'),
                str(ROOT/'libultraship/src/fast/resource/type/DisplayList.cpp'),
                str(ROOT/'libultraship/src/fast/resource/type/Texture.cpp'),'-o',str(binary)],check=True)
            subprocess.run([str(binary)],check=True)


if __name__ == '__main__':
    main()
