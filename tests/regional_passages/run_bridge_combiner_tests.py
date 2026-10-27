#!/usr/bin/env python3
"""Execute regional single-texture materials through production GenerateCC.

The two-cycle native setup swaps TEXEL0/1 in cycle2. Repeating modulation
there must fail: an authored surface cannot sample the scene's inherited tile1.
--source-ref provides a read-only baseline for a reproducible regression check.
"""
from pathlib import Path
import argparse
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sanitize', action='store_true')
parser.add_argument('--source-ref', help='Read provider sources at this git ref; leave the worktree untouched')
args = parser.parse_args()
renderer = (ROOT / 'libultraship/src/fast/interpreter.cpp').read_text()
header = (ROOT / 'libultraship/include/fast/interpreter.h').read_text()

def block(text, start):
    begin = text.index(start)
    opening = text.index('{', begin)
    depth = 1
    end = opening + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[begin:end]

generator = block(renderer, 'void Interpreter::GenerateCC').replace('Interpreter::GenerateCC', 'GenerateCC')
definitions = block(header, 'enum {\n    SHADER_0') + ';\n'
definitions += block(header, 'enum class ShaderOpts') + ';\n'
definitions += '#define SHADER_OPT(opt) ((uint64_t)(1 << static_cast<int>(ShaderOpts::opt)))\n'
definitions += block(header, 'struct ColorCombinerKey') + ';\n'
definitions += block(header, 'struct ColorCombiner {') + ';\n'

providers = ('ComboLakeHyliaBridge.cpp', 'ComboFountainSnowheadBridge.cpp', 'ComboFountainValley.cpp')
materials = []
for name in providers:
    path = 'soh/soh/Enhancements/Gameplay/' + name
    provider = (subprocess.check_output(['git', 'show', args.source_ref + ':' + path], cwd=ROOT, text=True)
                if args.source_ref else (ROOT / path).read_text())
    calls = list(re.finditer(r'gDPSetCombineMode\(POLY_(?:OPA|XLU)_DISP\+\+,\s*(.*?)\);', provider, re.S))
    assert calls, 'No production materials found in ' + path
    for call in calls:
        label = name + ':' + str(provider.count('\n', 0, call.start()) + 1)
        materials.append('{"' + label + '", EXPAND(' + call.group(1) + ')}')

source = r'''
#include <cstdint>
#include <cstring>
#include <cstdio>
#include "fast/lus_gbi.h"
using namespace Fast;
''' + definitions + generator + r'''
static uint32_t rgb(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    return (a & 15) | ((b & 15) << 4) | ((c & 31) << 8) | ((d & 7) << 13);
}
static uint32_t alpha(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    return (a & 7) | ((b & 7) << 3) | ((c & 7) << 6) | ((d & 7) << 9);
}
#define MAKE(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) \
    (uint64_t(rgb(G_CCMUX_##a,G_CCMUX_##b,G_CCMUX_##c,G_CCMUX_##d)) | \
    (uint64_t(alpha(G_ACMUX_##e,G_ACMUX_##f,G_ACMUX_##g,G_ACMUX_##h)) << 16) | \
    (uint64_t(rgb(G_CCMUX_##i,G_CCMUX_##j,G_CCMUX_##k,G_CCMUX_##l)) << 28) | \
    (uint64_t(alpha(G_ACMUX_##m,G_ACMUX_##n,G_ACMUX_##o,G_ACMUX_##p)) << 44))
#define EXPAND(a,b) MAKE(a,b)
int main() {
    struct Material { const char* label; uint64_t combine; };
    const Material materials[] = {''' + ',\n'.join(materials) + r'''};
    bool failed = false;
    for (const auto& material : materials) {
        ColorCombinerKey key{};
        key.options = SHADER_OPT(_2CYC) | SHADER_OPT(ALPHA) | SHADER_OPT(FOG);
        key.combine_mode = material.combine;
        ColorCombiner comb{};
        GenerateCC(&comb, key);
        const bool owned = comb.usedTextures[0] && !comb.usedTextures[1];
        printf("%s %s: texture0=%d texture1=%d\n", owned ? "PASS" : "FAIL", material.label,
               comb.usedTextures[0], comb.usedTextures[1]);
        failed |= !owned;
    }
    return failed ? 1 : 0;
}
'''
with tempfile.TemporaryDirectory(prefix='bridge-combiner-') as directory:
    cpp = Path(directory) / 'test.cpp'
    binary = Path(directory) / 'test'
    cpp.write_text(source)
    command = ['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-I' + str(ROOT / 'libultraship/include')]
    if args.sanitize:
        command += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer', '-no-pie']
    subprocess.run([*command, str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
