#!/usr/bin/env python3
"""Compile real interpreter methods and its complete triangle texture loop.

This partial checkout cannot build the engine. Resource ownership and rendering
API boundaries are supplied by native_test.cpp; production cache types, guard,
lookup/import/invalidation methods, tile changes, and texture preparation loop
are compiled unchanged. Decoder bodies are sentinels: uploaded pixel conversion,
shader selection, non-UV vertex packing, and a real GPU are outside this fixture.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile


def block(source, marker):
    start = source.index(marker)
    opening = source.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


def run(root, sanitize):
    source = (root / 'libultraship/src/fast/interpreter.cpp').read_text()
    header = (root / 'libultraship/include/fast/interpreter.h').read_text()
    fixture = (Path(__file__).parent / 'native_test.cpp').read_text()
    types = header[header.index('struct TextureCacheKey {'):header.index('struct RGBA {')]
    for name in ('RGBA', 'LoadedVertex', 'XYWidthHeight', 'RawTexMetadata', 'RDP', 'GfxTextureCache'):
        types += '\n' + block(header, 'struct ' + name + ' {') + ';\n'
    methods = '\n'.join(block(source, marker) for marker in (
        'bool Interpreter::TextureCacheLookup(',
        'std::string_view Interpreter::GetBaseTexturePath(',
        'void Interpreter::TextureCacheClear(',
        'void Interpreter::TextureCacheDelete(',
        'void Interpreter::TextureCacheDeleteByPalette(',
        'void Interpreter::ImportTexture(',
        'void Interpreter::GfxDpSetTile(',
        'void Interpreter::GfxDpSetTileSize(',
        'bool gfx_set_timg_fb_handler_custom(',
        'static uint32_t GetTileSizeFromCoordinates(',
        'static bool IsPyramidLike('))
    marker = 'bool Interpreter::TryReusePreludeMaterialBinding('
    if marker in source:
        methods += '\n' + block(source, marker)
    else:
        # The baseline still executes its original native texture loop. This
        # fallback lets the behavioral regression fail before implementation.
        methods += '\nbool Interpreter::TryReusePreludeMaterialBinding(int, uint32_t, const bool[2]) { return false; }'
    start = source.index('comb = LookupOrCreateColorCombiner(key);', source.index('void Interpreter::GfxSpTri1Impl'))
    loop = block(source[start:], 'for (int i = 0; i < 2; i++) {')
    uv_loop = block(source[start:], 'for (int t = 0; t < 2; t++) {')
    run_start = source.index('void Interpreter::Run(Gfx* commands,')
    run_prefix = source[source.index('{', run_start) + 1:source.index('const bool sampled = ', run_start)]
    translation_unit = fixture.replace('/* PRODUCTION TYPES */', types).replace(
        '/* PRODUCTION METHODS */', methods).replace('/* PRODUCTION TEXTURE LOOP */', loop).replace(
        '/* PRODUCTION UV LOOP */', uv_loop).replace('/* PRODUCTION RUN PREFIX */', run_prefix)
    with tempfile.TemporaryDirectory(prefix='caustic-binding-native-') as work:
        cpp, binary = Path(work) / 'native.cpp', Path(work) / 'native'
        cpp.write_text(translation_unit)
        command = ['c++', '-std=c++20', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                   '-Wno-parentheses', '-Wno-missing-field-initializers']
        if sanitize:
            command += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie']
        subprocess.run(command + [str(cpp), '-o', str(binary)], check=True)
        env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0')
        return subprocess.run([str(binary)], env=env).returncode


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    raise SystemExit(run(args.root, args.sanitize))
