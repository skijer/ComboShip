#!/usr/bin/env python3
"""Compile native NEI bridge declarations and verify per-DLL export ownership."""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--compiler', default='c++')
parser.add_argument('--msvc', action='store_true')
args = parser.parse_args()

sources = {
    'OOT_SetGiCosmeticFrame': 'soh/soh/Enhancements/cosmetics/CosmeticsEditor.cpp',
    'OOT_SampleGiCosmeticColor': 'soh/soh/Enhancements/cosmetics/CosmeticsEditor.cpp',
    'MM_SetGiCosmeticFrame': 'mm/2s2h/BenGui/CosmeticEditor.cpp',
    'MM_SampleGiCosmeticColor': 'mm/2s2h/BenGui/CosmeticEditor.cpp',
    'OOT_NeiResourceExists': 'soh/soh/ResourceManagerHelpers.cpp',
    'OOT_NeiEnsureGiBaseOwner': 'soh/soh/ResourceManagerHelpers.cpp',
    'OOT_GetNeiGiDrawInfo': 'soh/soh/Enhancements/randomizer/NeiGiPresentation.cpp',
    'OOT_GetNeiGiDrawInfoForAssets': 'soh/soh/Enhancements/randomizer/NeiGiPresentation.cpp',
    'OOT_NeiAltAssetsEnabled': 'soh/soh/ResourceManagerHelpers.cpp',
    'OOT_CopyWolfLinkResource': 'soh/soh/ResourceManagerHelpers.cpp',
    'MM_CopyWolfLinkResource': 'mm/2s2h/BenPort.cpp',
}

signatures = {}
for name, path in sources.items():
    production = (ROOT / path).read_text(encoding='utf-8')
    match = re.search(r'extern "C"\s+(?:(?:#ifdef COMBO_BUILD\s+)?COMBO_EXPORT\s+(?:#endif\s+)?)?'
                      r'(?:void|int|int32_t)\s+' + name + r'\([^)]*\)\s*\{', production)
    assert match, f'Missing production definition: {name}'
    signatures[name] = match[0][:-1].strip()

includes = '#include "combo/menu/ComboItemDrawABI.h"\n#include "combo/NeiWolfAsset.h"\n'

with tempfile.TemporaryDirectory(prefix='nei-abi-') as temporary:
    directory = Path(temporary)
    for owner, native_flag in [('OOT', 'SOH_BUILD_DLL'), ('MM', 'MM_BUILD_DLL')]:
        source = directory / (owner + '.cpp')
        definitions = []
        for name, signature in signatures.items():
            # On Windows use the exact native definition decoration. On ELF,
            # omit it to verify the shared declaration itself exports only its owner.
            if args.msvc and not name.startswith(owner + '_'):
                continue
            if not args.msvc:
                signature = re.sub(r'#ifdef COMBO_BUILD\s+|#endif\s*|COMBO_EXPORT\s+', '', signature)
            body = '{}' if re.search(r'\bvoid\s+' + name, signature) else '{ return 0; }'
            definitions.append(signature + ' ' + body)
        source.write_text(includes + '\n'.join(definitions), encoding='utf-8')
        if args.msvc:
            command = [args.compiler, '/nologo', '/std:c++20', '/EHsc', '/W3', '/wd4100',
                       '/DCOMBO_BUILD', '/D' + native_flag, '/I' + str(ROOT), '/c', str(source),
                       '/Fo:' + str(directory / (owner + '.obj'))]
        else:
            library = directory / (owner + '.so')
            command = [args.compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter',
                       '-DCOMBO_BUILD', '-D' + native_flag, '-I' + str(ROOT), '-shared', '-fPIC',
                       '-fvisibility=hidden', str(source), '-o', str(library)]
        subprocess.run(command, cwd=directory, check=True)
        if not args.msvc:
            symbols = subprocess.check_output(['nm', '-D', '--defined-only', str(library)], text=True)
            exported = {line.split()[-1] for line in symbols.splitlines()} & sources.keys()
            expected = {name for name in sources if name.startswith(owner + '_')}
            assert exported == expected, f'{owner} bridge exports: {exported}; expected {expected}'
        print(f'PASS: {owner} native NEI bridge declarations and export ownership')

    # Shared consumer headers must not define the native implementation macro.
    # The existing dungeon-key cache fixture supplies its own empty decoration.
    consumer = directory / 'consumer.cpp'
    consumer.write_text(includes + '#define COMBO_EXPORT\n'
                        'extern "C" COMBO_EXPORT void LegacyFixtureExport(void) {}\n', encoding='utf-8')
    command = ([args.compiler, '/nologo', '/std:c++20', '/EHsc', '/W3', '/we4005', '/DCOMBO_BUILD',
                '/I' + str(ROOT), '/c', str(consumer), '/Fo:' + str(directory / 'consumer.obj')]
               if args.msvc else
               [args.compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', '-DCOMBO_BUILD',
                '-I' + str(ROOT), '-c', str(consumer), '-o', str(directory / 'consumer.o')])
    subprocess.run(command, cwd=directory, check=True)
    print('PASS: consumer headers preserve independent implementation export macros')
