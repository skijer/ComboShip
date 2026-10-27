#!/usr/bin/env python3
"""Production-path probes for native MM foreign OoT scale presentation and saves."""
import os
from pathlib import Path
import re
import subprocess
import tempfile
import sys

ROOT=Path(__file__).resolve().parents[2]

def function(source,name):
    match=re.search(r'^(?:extern "C" )?(?:(?:static|inline) )?(?:void\*|bool|int32_t|int|void|std::string|std::shared_ptr<Fast::Texture>)\s+'+re.escape(name)+r'\([^;{}]*\)\s*\{',source,re.M)
    if not match:
        raise RuntimeError('Missing production function: '+name)
    index,depth=match.end(),1
    while depth:
        depth+=(source[index]=='{')-(source[index]=='}')
        index+=1
    return source[match.start():index]+'\n'

def run_texture():
    source=(ROOT/'libultraship/src/fast/interpreter.cpp').read_text()
    manager=(ROOT/'libultraship/src/ship/resource/ResourceManager.cpp').read_text()
    header=(ROOT/'libultraship/include/fast/interpreter.h').read_text()
    names=['Interpreter::SegAddr','IsValidResolvedAddress','gfx_check_image_signature']
    if 'ComboLoadTextureResource(' in source:
        names.append('ComboLoadTextureResource')
    names+=['ReportTextureLoadFailure','gfx_set_timg_handler_rdp','gfx_set_timg_otr_filepath_handler_custom']
    with tempfile.TemporaryDirectory(prefix='foreign-texture-') as temporary:
        build=Path(temporary)
        (build/'texture_signature.inc').write_text(function(manager,'ResourceManager::OtrSignatureCheck'))
        (build/'texture_metadata.inc').write_text(re.search(r'struct RawTexMetadata \{.*?\n\};',header,re.S).group(0))
        (build/'foreign_texture_production.inc').write_text(''.join(function(source,name) for name in names))
        (build/'spdlog').mkdir()
        (build/'spdlog/spdlog.h').write_text('#pragma once\n#define SPDLOG_TRACE(...) ((void)0)\n')
        binary=build/'foreign_texture'
        flags=['-std=c++20','-Wall','-Wextra','-I'+str(build),'-I'+str(ROOT/'libultraship/include')]
        if '--sanitize' in sys.argv:
            flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g']
        subprocess.run([os.environ.get('CXX','c++'),*flags,
            str(ROOT/'tests/mm_presentation/foreign_texture_test.cpp'),
            str(ROOT/'libultraship/src/ship/resource/Resource.cpp'),
            str(ROOT/'libultraship/src/fast/resource/type/Texture.cpp'),'-ldl','-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)

def run_lifecycle():
    saves=(ROOT/'mm/2s2h/SaveManager/SaveManager.cpp').read_text()
    queue=(ROOT/'mm/2s2h/Rando/MiscBehavior/CheckQueue.cpp').read_text()
    item=(ROOT/'soh/soh/Enhancements/randomizer/item.cpp').read_text()
    start=item.index('        case RG_PROGRESSIVE_SCALE:',item.index('std::shared_ptr<GetItemEntry> Item::GetGIEntry'))
    scale=item[start:item.index('        case RG_PROGRESSIVE_MAGIC_METER:',start)]
    start=queue.index('                        if (randoSaveCheck.randoItemId == RI_COMBO_FOREIGN) {')
    end,depth=queue.index('{',start)+1,1
    while depth:
        depth+=(queue[end]=='{')-(queue[end]=='}');end+=1
    fixture=(ROOT/'tests/mm_presentation/scale_lifecycle_test.cpp').read_text()
    save_body=function(saves,'SaveManager_SaveCurrentForCombo')
    if 'bool preservePortalSession' in save_body:
        # Portal capture/restore is an engine-state boundary for this narrow
        # foreign-reward fixture. Native SaveContext coverage lives in portal_save_test.py.
        save_body = ('static bool SaveManager_CapturePortalSession(nlohmann::json&,int) { assert(false); return false; }\n'
                     'static void SaveManager_RestorePortalRespawns(const nlohmann::json& j) { assert(!j.contains("comboPortalSession")); }\n'
                     + save_body + function(saves[saves.index('void SaveManager_SaveCurrentForCombo()'):],
                                            'SaveManager_SaveCurrentForCombo'))
    fixture=fixture.replace('/* MM_SAVE */',save_body+function(saves,'SaveManager_LoadSaveFile'))
    fixture=fixture.replace('/* SCALE_CASE */',scale).replace('/* FOREIGN_QUEUE */',queue[start:end])
    receipts=(ROOT/'mm/2s2h/Rando/ItemReceiptText.cpp').read_text()
    fixture=fixture.replace('/* RECEIPT_SOURCE */',function(receipts,'Rando::AppendReceiptSource'))
    fixture=fixture.replace('/* SEND_FOREIGN */',function(queue,'Rando::MiscBehavior::SendForeignCheck'))
    check_source=(ROOT/'mm/2s2h/Rando/MiscBehavior/MiscBehavior.h').read_text()
    fixture=fixture.replace('/* BANK_SOURCE */',function(check_source,'BankRewardSourceSuffix'))
    sharing=(ROOT/'mm/2s2h/FleetShipCombo/FleetSharedItems.cpp').read_text()
    shared=function(sharing,'ShareNativeMagicFloor')+function(sharing,'FleetShared_OnNativeObtained')
    fixture=fixture.replace('/* SHARE_SWIM */',shared)
    json_include=Path('/usr/include')
    if '--json-include' in sys.argv:
        json_include=Path(sys.argv[sys.argv.index('--json-include')+1])
    with tempfile.TemporaryDirectory(prefix='scale-lifecycle-') as temporary:
        build=Path(temporary);test=build/'lifecycle.cpp';test.write_text(fixture)
        binary=build/'lifecycle'
        flags=['-std=c++20','-Wall','-Wextra','-I'+str(json_include),'-I'+str(ROOT)]
        if '--sanitize' in sys.argv:
            flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g']
        subprocess.run([os.environ.get('CXX','c++'),*flags,str(test),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)

if __name__=='__main__':
    run_texture()
    run_lifecycle()
