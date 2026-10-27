#!/usr/bin/env python3
"""Run the real MM cape drawer through production texture handlers without a GPU.

This catches discarding replacement texture metadata at the producer/renderer
boundary. It also covers resource replacement and availability changes, both
cloth render lists, and preservation of the native triangle count/horse guard.
"""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
sys.path.insert(0, str(ROOT / 'tests/mm_presentation'))
from run_mm_nei_tests import flags, write_graph_helpers
from run_foreign_scale_tests import function
from run_time_pedestal_tests import functions


def main():
    interpreter = (ROOT / 'libultraship/src/fast/interpreter.cpp').read_text()
    manager = (ROOT / 'libultraship/src/ship/resource/ResourceManager.cpp').read_text()
    header = (ROOT / 'libultraship/include/fast/interpreter.h').read_text()
    names = ['Interpreter::SegAddr', 'IsValidResolvedAddress', 'gfx_check_image_signature',
             'ComboLoadTextureResource', 'ReportTextureLoadFailure', 'gfx_set_timg_handler_rdp',
             'gfx_set_timg_otr_filepath_handler_custom']
    sanitize = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer'] if '--sanitize' in sys.argv else []
    with tempfile.TemporaryDirectory(prefix='mm-cape-texture-') as directory:
        build = Path(directory)
        write_graph_helpers(build)
        with (build / 'mm_nei_graph.inc').open('a') as stream:
            stream.write('\n' + functions((ROOT / 'mm/src/code/stubs.c').read_text(), {'gSPVertex'})['gSPVertex'])
        (build / 'texture_signature.inc').write_text(function(manager, 'ResourceManager::OtrSignatureCheck'))
        (build / 'texture_metadata.inc').write_text(re.search(r'struct RawTexMetadata \{.*?\n\};', header, re.S).group(0))
        (build / 'foreign_texture_production.inc').write_text(''.join(function(interpreter, name) for name in names))
        (build / 'spdlog').mkdir()
        (build / 'spdlog/spdlog.h').write_text('#pragma once\n#define SPDLOG_TRACE(...) ((void)0)\n')
        native = build / 'cape_draw.o'
        compiled = subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu11', '-O2', '-Wall', '-Wextra', *sanitize,
                        '-Wno-comment', '-Wno-unused-function', '-Wno-unused-parameter', '-Wno-int-conversion',
                        *flags(), '-I' + str(build), '-ffunction-sections', '-fdata-sections',
                        '-c', str(ROOT / 'tests/mm_cape/cape_draw_fixture.c'), '-o', str(native)],
                        capture_output=True, text=True)
        if compiled.returncode:
            raise RuntimeError(compiled.stdout + compiled.stderr)
        print('PASS native cape fixture compilation (header warnings: ' + str(compiled.stderr.count('warning:')) + ')',
              flush=True)
        binary = build / 'cape_texture_test'
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra', *sanitize,
                        '-I' + str(build), '-I' + str(ROOT / 'libultraship/include'),
                        str(ROOT / 'tests/mm_cape/cape_texture_test.cpp'), str(native),
                        str(ROOT / 'libultraship/src/ship/resource/Resource.cpp'),
                        str(ROOT / 'libultraship/src/fast/resource/type/Texture.cpp'),
                        '-Wl,--gc-sections', '-ldl', '-lm', '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    main()
