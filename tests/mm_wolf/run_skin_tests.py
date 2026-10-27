#!/usr/bin/env python3
"""Production MM skin and native matrix fixture, using real MM headers."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_mm_nei_tests import flags
SANITIZERS = ['-fsanitize=address,undefined,bounds,float-cast-overflow', '-fno-sanitize-recover=all']
MODE = ['-O2', '-ffast-math'] if '--fast-math' in sys.argv else ['-O1', '-g']

def run(command):
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode:
        print(result.stdout + result.stderr)
        raise SystemExit(result.returncode)

with tempfile.TemporaryDirectory(prefix='mm-wolf-skin-') as td:
    # Execute the exact native nested marker/setup bodies; stubbing these hides
    # their temporary xlu/overlay slots and opaque command cost.
    def native_body(path, signature):
        source = (ROOT/path).read_text()
        start = source.index(signature)
        brace = source.index('{', start)
        depth, end = 1, brace + 1
        while depth:
            depth += (source[end] == '{') - (source[end] == '}')
            end += 1
        return source[start:end]
    native_graph = Path(td)/'native-graph.c'
    native_graph.write_text('#include "global.h"\nGfx gSetupDLs[SETUPDL_MAX][6];\n' +
                            native_body('mm/src/code/graph.c', 'void Graph_OpenDisps(') + '\n' +
                            native_body('mm/src/code/graph.c', 'void Graph_CloseDisps(') + '\n' +
                            native_body('mm/src/code/z_rcp.c', 'void Gfx_SetupDL25_Opa(') + '\n')
    options = ['-DMM_WOLF_SKIN_OPTIONS'] if 'preserveRootMotion' in (ROOT/'mm/expansions/ssbb/ssbb_skin.h').read_text() else []
    objects = []
    for i, source in enumerate(['mm/expansions/ssbb/ssbb_skin.c', 'mm/src/code/z_skin_matrix.c',
                              'mm/expansions/ssbb/characters/pikachu_ssbb_skin.c',
                              'mm/expansions/ssbb/characters/pikachu_ssbb_skel.c', str(native_graph)]):
        obj = str(Path(td)/f'{i}.o')
        run([os.environ.get('CC','cc'), '-std=gnu11', *flags(), '-include', str(ROOT/'mm/include/variables.h'),
                        '-include', str(ROOT/'mm/include/functions.h'), '-include', str(ROOT/'mm/include/z64malloc.h'),
                        '-include', str(ROOT/'libultraship/include/libultraship/bridge/consolevariablebridge.h'),
                        *MODE, *SANITIZERS, '-ffunction-sections', '-fdata-sections', '-c', str(ROOT/source), '-o', obj])
        objects.append(obj)
    binary = str(Path(td)/'skin')
    run([os.environ.get('CXX','c++'), '-std=c++20', *flags(), *options, *MODE, *SANITIZERS,
         str(ROOT/'tests/mm_wolf/skin_runtime_test.cpp'), *objects, '-Wl,--gc-sections', '-o', binary])
    # LeakSanitizer cannot inspect /proc tasks in the managed executor; memory/bounds/UB checks remain enabled.
    env = dict(os.environ)
    env['ASAN_OPTIONS'] = env.get('ASAN_OPTIONS', '') + ':detect_leaks=0'
    subprocess.run([binary], check=True, env=env)
