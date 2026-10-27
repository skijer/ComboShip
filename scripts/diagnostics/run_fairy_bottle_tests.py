"""Execute native/foreign bottled-fairy recipes at the GPU command boundary.

Uses unchanged production bodies with resource, matrix and stream instrumentation.
This verifies a bounded transform, not visual containment inside unsupplied mod art.
"""
import os
import json
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

from run_mm_scene_randomization_tests import function

ROOT = Path(__file__).resolve().parents[2]


def packaged_glass_commands():
    entries = []
    for owner, host in (('oot', 'soh'), ('mm', 'mm')):
        assets = ROOT / host / 'assets/custom'
        pending = ['objects/combo_bottle_gi/EmptyXlu']
        if (assets / 'objects/combo_bottle_gi/FairyXlu').is_file():
            pending.append('objects/combo_bottle_gi/FairyXlu')
        visited = set()
        while pending:
            resource = pending.pop()
            if resource in visited:
                continue
            visited.add(resource)
            path = assets / resource
            blob = path.read_bytes()  # Missing nested resources must fail the test.
            if not blob.startswith(b'<'):
                continue
            root = ET.parse(path).getroot()
            if root.tag != 'DisplayList':
                continue
            commands = []
            for command in root:
                if 'Path' in command.attrib:
                    pending.append(command.get('Path'))
                attributes = ','.join('{' + json.dumps(key) + ',' + json.dumps(value) + '}'
                                      for key, value in command.attrib.items())
                commands.append('{' + json.dumps(command.tag) + ',{' + attributes + '}}')
            key = owner + ':' + path.relative_to(assets).as_posix()
            entries.append('{' + json.dumps(key) + ',{' + ','.join(commands) + '}}')
    return ('const std::map<std::string, std::vector<AssetCommand>> assetCommandLists = {\n' +
            ',\n'.join(entries) + '\n};\n')


def native(namespace, path, draws, member, resource_type, rows, constants):
    source = (ROOT / path).read_text()
    export = function(source, 'GetItem_GetDrawTableEntry')
    names = sorted(set(re.findall(r'GetItem_Draw\w+', export)))
    parts = ['namespace ' + namespace + ' {', constants]
    for name in names:
        parts.append('void ' + name + '(PlayState*,s16)' + (';' if name in draws else ' {}'))
    parts += [f'struct Entry {{void (*drawFunc)(PlayState*,s16);{resource_type} {member}[8];}};',
              'Entry sDrawItemTable[]={' + ','.join(rows) + '};',
              function(source, 'GetItem_EmptyBottleShell'), function(source, 'GetItem_FairyBottleShell'), export]
    parts += [function(source, name) for name in draws]
    if namespace == 'oot':
        shimmer = function(source, 'GetItem_GetShimmerColor')
        # The native shimmer dispatcher delegates collectible eligibility to
        # its shared header. Supply that header's IDs alongside the other
        # out-of-fixture draw IDs instead of replacing the production helper.
        collectible = (ROOT / 'combo/menu/ComboCollectibleShimmerHost.h').read_text()
        ids = sorted(set(re.findall(r'GID_\w+', shimmer + collectible)) - {'GID_FAIRY'})
        parts += ['enum {' + ','.join(name + '=' + str(1000+i) for i, name in enumerate(ids)) + '};',
                  '#include "combo/menu/ComboCollectibleShimmerHost.h"',
                  shimmer, function(source, 'GetItem_Draw')]
    return '\n'.join(parts) + '\n}\n'


def main():
    subprocess.run([sys.executable, '-B', str(ROOT / 'tests/fairy_bottle/run_glass_asset_tests.py')], check=True)
    subprocess.run([sys.executable, '-B', str(ROOT / 'tests/fairy_bottle/run_glow_pose_tests.py')], check=True)
    subprocess.run([sys.executable, '-B', str(ROOT / 'tests/fairy_bottle/run_receipt_tests.py')], check=True)
    bodies = native('oot', 'soh/src/code/z_draw.c', ['GetItem_DrawFairy'], 'dlists', 'Gfx*',
                    ['{GetItem_DrawFairy,{(Gfx*)opaque,(Gfx*)glass,(Gfx*)fairy}}',
                     '{GetItem_DrawOpa0Xlu1,{(Gfx*)genericOpaque,(Gfx*)genericGlass}}',
                     '{GetItem_DrawOpa0Xlu1,{(Gfx*)genericOpaque,(Gfx*)genericGlass}}'],
                    'const char* gGiBottleStopperDL=genericOpaque; const char* gGiBottleDL=genericGlass;\n'
                    'const char* gGiBlueFireChamberstickDL=blueFire;')
    bodies += 'void GetItem_Draw(PlayState* play,int id) { assert(ownerScope==1); ++hostFallbacks; oot::GetItem_Draw(play,id); }\n'
    bodies += native('mm', 'mm/src/code/z_draw.c', ['GetItem_DrawFairyContainer', 'GetItem_DrawFairyBottle'],
                     'drawResources', 'void*',
                     ['{GetItem_DrawFairyContainer,{opaque,glass,fairy,matrixPath}}',
                      '{GetItem_DrawFairyBottle,{opaque,glass,fairy}}',
                      '{GetItem_DrawOpa0Xlu1,{genericOpaque,genericGlass}}'],
                     'const char* gGiEmptyBottleCorkDL=genericOpaque; const char* gGiEmptyBottleGlassDL=genericGlass;\n'
                     'int ResourceMgr_IsModAsset(const char* path) {return ::ResourceMgr_IsModAsset(path);}')
    source = (ROOT / 'combo/menu/ComboForeignDrawOOT.h').read_text()
    bodies += function(source, 'OOT_DrawForeignFairyBottle') + '\n'
    bodies += function(source, 'OOT_DrawForeignFairyContainer') + '\n'
    dispatch = function(source, 'OOT_DrawComboForeign')
    bodies += 'extern "C" void MagicMirror_DrawGI(PlayState*) { assert(false && "fairy dispatch selected Mirror"); }\n'
    bodies += function((ROOT/'combo/menu/ComboMagicMirrorForeign.h').read_text(), 'ComboDrawMagicMirrorForeign') + '\n'
    for name in sorted(set(re.findall(r'\bOOT_DrawForeign\w+', dispatch)) -
                       {'OOT_DrawForeignFairyBottle', 'OOT_DrawForeignFairyContainer'}):
        bodies += 'template<class...T>void ' + name + '(T...) { assert(false); }\n'
    bodies += 'template<class...T>void ComboSwordGi_ApplyEffectFit(T...) { assert(false); }\n'
    bodies += 'template<class...T>void ComboSwordGi_ApplyPresentationSize(T...) { assert(false); }\n'
    bodies += 'template<class...T>float ComboSwordGi_ParticleScale(T...) { assert(false); return 1.f; }\n'
    bodies += dispatch + '\n'
    source = (ROOT / 'combo/menu/ComboForeignDrawMM.h').read_text()
    bodies += function(source, 'MM_DrawForeignFairy') + '\n'
    with tempfile.TemporaryDirectory(prefix='fairy-bottle-test-') as temporary:
        build = Path(temporary)
        (build / 'fairy_production.inc').write_text(bodies)
        (build / 'fairy_asset_commands.inc').write_text(packaged_glass_commands())
        for host in ('oot', 'mm'):
            binary = build / ('test-' + host)
            flags = ['-DCOMBO_FAIRY_HOST_MM'] if host == 'mm' else []
            subprocess.run([*shlex.split(os.environ.get('CXX', 'c++')), '-std=c++20', '-Wall', '-Wextra',
                            '-Wno-unused-parameter', '-Wno-unused-variable', '-Wno-sign-compare',
                            '-Wno-missing-field-initializers', '-DCOMBO_BUILD', *flags, '-I' + str(ROOT), '-I' + str(build),
                            str(ROOT / 'tests/fairy_bottle/draw_test.cpp'), '-o', str(binary)], check=True)
            print('HOST ' + host, flush=True)
            subprocess.run([str(binary)], check=True)
        # This helper is included from C native draw and C++ foreign draw TUs.
        # Check actual C++ engine declarations too, not only the capture fixture.
        for host in ('soh', 'mm'):
            body = '#include "global.h"\n'
            if host == 'soh':
                body += '#include "soh/ResourceManagerHelpers.h"\n'
                includes = ['soh', 'soh/include', 'soh/src', 'soh/assets', 'soh/mods']
            else:
                body += '#include "2s2h/BenPort.h"\n#define COMBO_FAIRY_HOST_MM\n'
                includes = ['mm', 'mm/include', 'mm/include/PR', 'mm/src', 'mm/assets', 'mm/2s2h']
            body += '#include "ComboFairyBottleDraw.h"\n'
            source = build / (host + '-headers.cpp')
            source.write_text(body)
            includes += ['libultraship/include', 'libultraship/src', 'combo', 'combo/menu']
            command = [*shlex.split(os.environ.get('CXX', 'c++')), '-std=c++20', '-DF3DEX_GBI_2',
                       '-DCOMBO_BUILD', '-DLOG_LEVEL_GAME_PRINTS=0', '-DCONTROLLERBUTTONS_T=uint32_t',
                       '-DNON_EQUIVALENT', '-DNON_MATCHING', '-fsyntax-only']
            command += ['-I' + str(ROOT / path) for path in includes] + [str(source)]
            result = subprocess.run(command, capture_output=True, text=True)
            if result.returncode:
                raise RuntimeError(result.stdout + result.stderr)
            print('PASS real-header C++ fairy helper ' + host)


if __name__ == '__main__':
    main()
