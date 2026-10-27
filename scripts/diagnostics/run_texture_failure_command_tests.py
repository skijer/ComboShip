#!/usr/bin/env python3
"""Check production texture failure handlers without a GPU or game archives.

The fixture preserves a following vertex-load sentinel, checks successful
binding/recovery, and captures bounded diagnostics. --baseline-ref compiles the
same handlers from an older commit to demonstrate the command-skip defect.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    match = re.search(r'^(?:static )?(?:void\*|void|bool|int32_t|std::shared_ptr<Fast::Texture>)\s+' +
                      re.escape(name) + r'\([^;{}]*\)\s*\{', source, re.M)
    if not match:
        raise RuntimeError('Missing production function: ' + name)
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline-ref')
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--case', default='all', choices=['all', 'raw', 'unknown-hash', 'missing-hash'])
    args = parser.parse_args()

    def read(path):
        if args.baseline_ref:
            return subprocess.check_output(['git', 'show', args.baseline_ref + ':' + path],
                                           cwd=ROOT, text=True)
        return (ROOT / path).read_text()

    source = read('libultraship/src/fast/interpreter.cpp')
    header = read('libultraship/include/fast/interpreter.h')
    manager = read('libultraship/src/ship/resource/ResourceManager.cpp')
    names = ['Interpreter::SegAddr', 'IsValidResolvedAddress', 'gfx_check_image_signature',
             'ComboLoadTextureResource']
    traced = 'static void ReportTextureLoadFailure(' in source
    if traced:
        names.append('ReportTextureLoadFailure')
    names += ['gfx_set_timg_handler_rdp', 'gfx_set_timg_otr_hash_handler_custom']
    with tempfile.TemporaryDirectory(prefix='texture-command-') as temporary:
        build = Path(temporary)
        (build / 'texture_signature.inc').write_text(function(manager, 'ResourceManager::OtrSignatureCheck'))
        (build / 'texture_metadata.inc').write_text(re.search(r'struct RawTexMetadata \{.*?\n\};', header, re.S).group(0))
        (build / 'texture_failure_production.inc').write_text(''.join(function(source, name) for name in names))
        (build / 'spdlog').mkdir()
        (build / 'spdlog/spdlog.h').write_text('#pragma once\n#define SPDLOG_TRACE(...) ((void)0)\n')
        binary = build / 'texture_failure_test'
        flags = ['-std=c++20', '-Wall', '-Wextra', '-Wno-unused-variable', '-I' + str(build),
                 '-I' + str(ROOT / 'libultraship/include')]
        if traced:
            flags += ['-DHAS_FAILURE_TRACE']
        if args.sanitize:
            flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-g']
        subprocess.run([os.environ.get('CXX', 'c++'), *flags,
                        str(ROOT / 'libultraship/tests/texture_failure_command_test.cpp'),
                        str(ROOT / 'libultraship/src/ship/resource/Resource.cpp'),
                        str(ROOT / 'libultraship/src/fast/resource/type/Texture.cpp'), '-ldl', '-o', str(binary)],
                       check=True)
        subprocess.run([str(binary), args.case], check=True)


if __name__ == '__main__':
    main()
