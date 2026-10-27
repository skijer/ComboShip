#!/usr/bin/env python3
"""Compile actual native render objects and resolve their interpolation hooks.

This is an object/linkage regression, not a full executable link. Configured
commands are remapped into this checkout and all outputs stay in a temp folder.
"""
from pathlib import Path
import argparse
import json
import os
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--compile-commands', type=Path, required=True)
parser.add_argument('--output-dir', type=Path)
args = parser.parse_args()
env = dict(os.environ)
if env.get('COMBO_NEI_FIXTURE_CPATH'):
    env['CPATH'] = env['COMBO_NEI_FIXTURE_CPATH'] + os.pathsep + env.get('CPATH', '')
files = ('soh/soh/Enhancements/Gameplay/ComboFountainSnowheadBridge.cpp',
         'soh/soh/Enhancements/Gameplay/ComboLakeHyliaBridge.cpp',
         'soh/soh/frame_interpolation.cpp')
entries = json.loads(args.compile_commands.read_text())

def compile_object(relative, output):
    entry = next((item for item in entries if item['file'].endswith('/' + relative)), None)
    assert entry, 'Missing configured source: ' + relative
    command = entry.get('arguments') or shlex.split(entry['command'])
    old_root = entry['file'].split('/soh/soh/')[0]
    command = [arg.replace(old_root, str(ROOT)) for arg in command]
    command[command.index('-o') + 1] = str(output)
    cwd = entry['directory']
    result = subprocess.run(command, cwd=cwd, env=env, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    print('PASS actual production object:', relative, '(compiler warnings:', result.stderr.count('warning:'), ')', flush=True)

with tempfile.TemporaryDirectory(prefix='regional-render-linkage-') as temporary:
    build = args.output_dir.resolve() if args.output_dir else Path(temporary)
    build.mkdir(parents=True, exist_ok=True)
    objects = []
    for relative in files:
        obj = build / (Path(relative).stem + '.o')
        compile_object(relative, obj)
        objects.append(obj)
    linked = build / 'regional-interpolation-linked.o'
    subprocess.run([os.environ.get('LD', 'ld'), '-r', *map(str, objects), '-o', str(linked)], check=True, env=env)
    unresolved = subprocess.check_output(['nm', '-u', '-C', str(linked)], text=True, env=env)
    hooks = [line for line in unresolved.splitlines() if 'FrameInterpolation_Record' in line]
    assert not hooks, 'Native render hooks failed to resolve against actual production interpolation implementation:\n' + '\n'.join(hooks)
    for obj in objects[:2]:
        unresolved = subprocess.check_output(['nm', '-u', str(obj)], text=True, env=env)
        for name in ('FrameInterpolation_RecordOpenChild', 'FrameInterpolation_RecordCloseChild'):
            assert any(line.split() == ['U', name] for line in unresolved.splitlines()), (obj, name, unresolved)
    print('PASS native render C ABI: Snowhead/Lake hook imports resolve in relocatable link against actual frame_interpolation.cpp')
