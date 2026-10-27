#!/usr/bin/env python3
"""Compile affected units with real headers; does not link a game executable.

Supply external include directories through CPLUS_INCLUDE_PATH/C_INCLUDE_PATH,
or --dependencies with a JSON mapping whose entries have an 'include' field.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dependencies', type=Path)
    parser.add_argument('--source', action='append', help='Compile only these repo-relative units (repeatable)')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    external = []
    if args.dependencies:
        external = [value['include'] for value in json.loads(args.dependencies.read_text()).values()
                    if isinstance(value, dict) and 'include' in value]
    shared = ['-DF3DEX_GBI_2', '-DLOG_LEVEL_GAME_PRINTS=0', '-DCONTROLLERBUTTONS_T=uint32_t',
              '-DNON_EQUIVALENT', '-DNON_MATCHING', '-DIMGUI_DEFINE_MATH_OPERATORS']
    units = [('soh', 'soh/soh/Enhancements/cosmetics/PlacedSariaResources.cpp', True),
             ('mm', 'mm/2s2h/Enhancements/Gameplay/ComboSaria.cpp', True)]
    for combo in [True, False]:
        units.append(('soh', 'soh/soh/Enhancements/Gameplay/ComboForestActors.cpp', combo))
        units.append(('soh', 'soh/src/overlays/actors/ovl_En_Horse/z_en_horse.c', combo))
        units.append(('soh', 'soh/src/code/z_actor.c', combo))
        units.append(('mm', 'mm/2s2h/Enhancements/Gameplay/MmPlacedSariaResources.cpp', combo))
        units.append(('mm', 'mm/2s2h/Enhancements/Gameplay/ComboForestSkullKid.cpp', combo))
        units.append(('mm', 'mm/src/overlays/actors/ovl_Dm_Stk/z_dm_stk.c', combo))
        units.append(('mm', 'mm/src/code/z_actor.c', combo))
        units.append(('mm', 'mm/2s2h/Rando/MiscBehavior/SariasSongHint.cpp', combo))
        units.append(('mm', 'mm/src/overlays/actors/ovl_En_Horse/z_en_horse.c', combo))
    for game in ['soh', 'mm']:
        for combo in [True, False]:
            units.append((game, game + '/src/overlays/actors/ovl_En_Viewer/z_en_viewer.c', combo))
    failures = []
    if args.source:
        unknown = set(args.source) - {source for _, source, _ in units}
        if unknown:
            parser.error('Unknown unit: ' + ', '.join(sorted(unknown)))
        units = [unit for unit in units if unit[1] in args.source]
    for game, source, combo in units:
        cpp = source.endswith('.cpp')
        flags = shared + (['-DCOMBO_BUILD'] if combo else [])
        for path in ['libultraship/cmake/cvars.cmake', 'CMake/lus-cvars.cmake',
                     'CMake/' + ('2ship' if game == 'mm' else 'soh') + '-cvars.cmake']:
            if (ROOT/path).exists():
                flags += [f'-D{key}="{value}"' for key, value in
                          re.findall(r'set\((CVAR_\w+) "([^"]+)"', (ROOT/path).read_text())]
        includes = [game+'/include', game+'/include/PR', game+'/src', game, game+'/assets', game+'/mods',
                    game + ('/2s2h' if game == 'mm' else '/soh'), 'libultraship/include',
                    'libultraship/src', 'combo', 'combo/menu', 'combo/gui'] + external
        name = game + '-' + Path(source).stem + ('-combo' if combo else '-standalone')
        command = [os.environ.get('CXX' if cpp else 'CC', 'c++' if cpp else 'cc'),
                   '-std=gnu++20' if cpp else '-std=gnu11', *flags, *['-I'+p for p in includes],
                   '-c', source, '-o', str(args.output/(name+'.o'))]
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        (args.output/(name+'.log')).write_text(result.stdout + result.stderr)
        (args.output/(name+'.command.json')).write_text(json.dumps({
            'cwd': str(ROOT), 'command': command, 'returncode': result.returncode,
            'source_sha256': hashlib.sha256((ROOT/source).read_bytes()).hexdigest(),
        }, indent=2) + '\n')
        print(('FAIL ' if result.returncode else 'PASS ') + name, flush=True)
        if result.returncode:
            failures.append(source)
    if failures:
        raise SystemExit(1)

if __name__ == '__main__':
    main()
