#!/usr/bin/env python3
"""Compile production MM snapshot code and exercise real MM resource classes.
Archive lookup is controlled; no game executable or runtime proof is implied.
CPATH may supply public dependencies; --sanitize enables address/UB checks.
Optional --native-oot, --native-mm and repeated --saria inputs add real archive
regressions using production MM binary skeleton/limb factories. The RM/archive
boundary is controlled; normal clips/textures decode real payload fields and
DL owning classes preserve presence/size without invoking the renderer.
"""
import argparse
import os
import re
import subprocess
import tempfile
import zipfile
import hashlib
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
p = argparse.ArgumentParser()
p.add_argument('--sanitize', action='store_true')
p.add_argument('--native-oot', type=Path)
p.add_argument('--native-mm', type=Path)
p.add_argument('--saria', type=Path, action='append', default=[])
args = p.parse_args()
def method(source, name):
    match = re.search(r'^[^\n;{}]*\b' + re.escape(name) + r'\([^;{}]*\)\s*\{', source, re.M)
    if not match: raise RuntimeError(name)
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'
def run(*args):
    subprocess.run(list(map(str, args)), cwd=ROOT, check=True)
flags = ['-std=c++20', '-DCOMBO_BUILD', '-DF3DEX_GBI_2', '-DNDEBUG', '-DFMT_HEADER_ONLY', '-DCONTROLLERBUTTONS_T=uint32_t',
         '-Ilibultraship/include', '-Imm/include', '-Imm/include/PR', '-Imm', '-Icombo']
if args.sanitize:
    flags += ['-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
with tempfile.TemporaryDirectory(prefix='mm-saria-resources-') as temp:
    work = Path(temp)
    # The accessors are real methods; skeleton patcher depends on the game host.
    source = (ROOT/'mm/2s2h/resource/type/Skeleton.cpp').read_text()
    skeleton = work/'skeleton.cpp'
    skeleton.write_text('#include "2s2h/resource/type/Skeleton.h"\nnamespace SOH {\n' +
                        method(source, 'Skeleton::GetPointer') + method(source, 'Skeleton::GetPointerSize') + '}\n')
    sources = ['mm/tests/placed_saria_local_resources_test.cpp',
               'mm/2s2h/Enhancements/Gameplay/MmPlacedSariaResources.cpp', skeleton,
               'mm/2s2h/resource/type/SkeletonLimb.cpp', 'mm/2s2h/resource/type/Animation.cpp',
               'mm/2s2h/resource/type/Array.cpp',
               'libultraship/src/ship/resource/Resource.cpp', 'libultraship/src/fast/resource/type/Texture.cpp',
               'libultraship/src/fast/resource/type/DisplayList.cpp']
    run(os.environ.get('CXX', 'c++'), *flags, *sources, '-o', work/'resources')
    subprocess.run([str(work/'resources')], cwd=ROOT, check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
    # Original standalone 2Ship compilation has no Combo ABI dependency.
    run(os.environ.get('CXX', 'c++'), '-std=c++20', '-c',
        'mm/2s2h/Enhancements/Gameplay/MmPlacedSariaResources.cpp', '-o', work/'original.o')

    if args.saria:
        if not args.native_oot: p.error('--saria requires --native-oot (MM has no native Saria donor)')
        factory = (ROOT/'mm/2s2h/resource/importer/SkeletonFactory.cpp').read_text()
        skeletonFactory = method(factory, 'InitializeSkeletonData') + "std::shared_ptr<Ship::IResource>\n" + method(factory, 'ResourceFactoryBinarySkeletonV0::ReadResource')
        original = 'Ship::Context::GetRawInstance()->GetResourceManager()->LoadResourceProcess(limbStr.c_str())'
        assert skeletonFactory.count(original) == 1
        # Replace only archive/RM lookup. Keep production binary readers, limb
        # table initialization, factory-created pointers and Alt=false default.
        skeletonFactory = skeletonFactory.replace(original, '::FactoryLoad(limbStr)')
        limbFactory = "std::shared_ptr<Ship::IResource>\n" + method((ROOT/'mm/2s2h/resource/importer/SkeletonLimbFactory.cpp').read_text(), 'ResourceFactoryBinarySkeletonLimbV0::ReadResource')
        fixture = (ROOT/'mm/tests/placed_saria_local_resources_test.cpp').read_text()
        packFixture = work/'pack.cpp'
        packFixture.write_text(fixture.replace('/* PRODUCTION_MM_BINARY_FACTORIES */', 'namespace SOH {\n' + skeletonFactory + limbFactory + '}\n'))
        factoryBase = work/'factory_base.cpp'
        factoryBase.write_text('#include <ship/resource/ResourceFactoryBinary.h>\nnamespace Ship {\n' + method((ROOT/'libultraship/src/ship/resource/ResourceFactoryBinary.cpp').read_text(), 'ResourceFactoryBinary::FileHasValidFormatAndReader').replace('SPDLOG_ERROR("resource file format does not match factory format.");', '').replace('SPDLOG_ERROR("Failed to load resource: File has Reader ({} - {})", initData->Type, initData->Path);', '') + '}\n')
        packSources = [packFixture, *sources[1:], factoryBase, 'mm/2s2h/resource/importer/ArrayFactory.cpp', *[ROOT/'libultraship/src/ship/utils'/name for name in ['Utils.cpp', 'binarytools/Stream.cpp', 'binarytools/MemoryStream.cpp', 'binarytools/BinaryReader.cpp']]]
        run(os.environ.get('CXX', 'c++'), *flags, '-DMM_SARIA_PACK_TEST', *packSources, '-o', work/'pack')
        for number, pack in enumerate(args.saria):
            folder = work/('archives-' + str(number))
            folder.mkdir()
            for archive in [args.native_oot, *([args.native_mm] if args.native_mm else []), pack]:
                print('Archive:', archive, hashlib.sha256(archive.read_bytes()).hexdigest(), flush=True)
                with zipfile.ZipFile(archive) as z:
                    for name in z.namelist():
                        if not name.startswith(('objects/object_sa/', 'alt/objects/object_sa/')) or name.endswith('/'): continue
                        path = folder/name.replace('/', '!')
                        path.write_bytes(z.read(name))
            subprocess.run([str(work/'pack'), str(folder)], cwd=ROOT, check=True,
                           env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
