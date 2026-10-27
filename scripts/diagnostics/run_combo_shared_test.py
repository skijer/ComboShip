#!/usr/bin/env python3
"""Compile focused production behavior checks; optional pinned baseline control demonstrates regressions."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
BASELINE_REV = 'cac526a8b9dc08c83f4c2ad7fece0113d284513d'
p = argparse.ArgumentParser()
p.add_argument('--json-include', type=Path, default=ROOT.parent / 'deps')
p.add_argument('--baseline', action='store_true', help='Run relevant cases against pre-integration cac526a8')
a = p.parse_args()
with tempfile.TemporaryDirectory(prefix='combo_shared_') as tmp:
    directory = Path(tmp)
    includes = ROOT / 'combo'
    flags = []
    if a.baseline:
        includes = directory / 'baseline'
        for relative in ['rando/ComboPlaythrough.h', 'rando/CrossWorldRando.h', 'rando/CrossForeign.h']:
            target = includes / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(subprocess.check_output(['git', 'show', f'{BASELINE_REV}:combo/{relative}'], cwd=ROOT,
                               env={**os.environ, 'GIT_NO_LAZY_FETCH': '1'}))
        (includes / 'gui').mkdir()
        (includes / 'gui/ComboGenProgress.h').write_bytes((ROOT / 'combo/gui/ComboGenProgress.h').read_bytes())
        (includes / 'rando/SharedItems.h').write_bytes((ROOT / 'combo/rando/SharedItems.h').read_bytes())
        flags = ['-DCOMBO_SHARED_BASELINE']
    binary = directory / 'combo_shared_test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra', '-O0', *flags,
                    '-I', str(includes), '-I', str(a.json_include),
                    str(ROOT / 'combo/tests/combo_shared_test.cpp'), '-o', str(binary)], check=True)
    result = subprocess.run([str(binary)], cwd=directory)
    if result.returncode or a.baseline:
        raise SystemExit(result.returncode)
    source = (ROOT / 'combo/ComboShip.cpp').read_text()
    begin = source.index('static bool Combo_BindCrossDungeonRuntime(')
    end = source.index('\n}', begin) + 2
    binding = source[begin:end]
    begin = source.index('static void Combo_OnSharedChanged(', source.index('// Shared Items tier reconcile'))
    end = source.index('// Seed utilities', begin)
    runtime = source[begin:end]
    begin = source.index('static bool Combo_WriteMMSaveForSlot(')
    end = source.index('\n}', begin) + 2
    runtime += source[begin:end]
    begin = source.index('static std::set<std::string> sAppliedCrossChecks;')
    end = source.index('// Network-receive idempotency:', begin)
    (directory / 'combo_shared_runtime_functions.h').write_text(binding + '\n' + runtime + source[begin:end])
    runtime_binary = directory / 'combo_shared_runtime_test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra', '-O0',
                    '-I', str(includes), '-I', str(a.json_include), '-I', str(directory),
                    str(ROOT / 'combo/tests/combo_shared_runtime_test.cpp'), '-o', str(runtime_binary)], check=True)
    result = subprocess.run([str(runtime_binary)], cwd=directory)
    if result.returncode:
        raise SystemExit(result.returncode)
    source = (ROOT / 'combo/gui/ComboMenu.cpp').read_text()
    begin = source.index('struct PlandoPickItem {')
    end = source.index('static PlandoState sPlando;', begin) + len('static PlandoState sPlando;')
    state = source[begin:end]
    begin = source.index('void PlandoBuildItems() {')
    end = source.index('\n}', begin) + 2
    (directory / 'combo_shared_plando_functions.h').write_text(state + source[begin:end])
    plando_binary = directory / 'combo_shared_plando_test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra', '-O0',
                    '-I', str(includes), '-I', str(a.json_include), '-I', str(directory),
                    str(ROOT / 'combo/tests/combo_shared_plando_test.cpp'), '-o', str(plando_binary)], check=True)
    result = subprocess.run([str(plando_binary)], cwd=directory)
    raise SystemExit(result.returncode)
