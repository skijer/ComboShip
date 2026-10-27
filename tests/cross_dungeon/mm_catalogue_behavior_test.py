#!/usr/bin/env python3
"""Compile real MM catalogue/adapters against native enum values and narrow engine structs."""
import argparse
import pathlib
import re
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--sanitize', action='store_true')
args = parser.parse_args()
required = [ROOT / 'combo/rando/CrossEntranceMmCatalogue.h', ROOT / 'combo/rando/CrossEntranceMmGrottos.inc.h']
assert all(p.exists() for p in required), 'MM catalogue and grotto adapter feature is missing'
scene = (ROOT / 'mm/include/z64scene.h').read_text()
types = (ROOT / 'mm/2s2h/Rando/Types.h').read_text()
constants = '\n'.join('constexpr int '+name+' = '+value+';' for value, name in re.findall(r'/\*\s*(0x[0-9A-Fa-f]+)\s*\*/\s*(ENTR_SCENE_\w+)', scene))
regions = list(dict.fromkeys(re.findall(r'\bRR_\w+', types)))
constants += '\nenum RandoRegionId { '+', '.join(regions)+' };\n'
constants += '''
#define ENTRANCE(scene, spawn) ((((ENTR_SCENE_##scene)&0x7F)<<9)|(((spawn)&0x1F)<<4))
constexpr int SCENE_KAKUSIANA=7;
constexpr int RESPAWN_MODE_DOWN=0, RESPAWN_MODE_TOP=2, RESPAWN_MODE_UNK_3=3;
constexpr int PLAYER_START_MODE_GROTTO=4, PLAYER_START_MODE_D=13;
#define PLAYER_PARAMS(camera, mode) ((camera)|((mode)<<8))
constexpr int WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE=1, WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE=2;
bool snowhead=false, woodfall=false;
#define CHECK_WEEKEVENTREG(flag) ((flag)==1?snowhead:woodfall)
struct Vec3 { float x=0,y=0,z=0; };
struct Respawn { int entrance=0,roomIndex=0,data=0,yaw=0,playerParams=0,tempSwitchFlags=99,unk_18=99,tempCollectFlags=99; Vec3 pos; };
struct SaveContext { struct { int entrance=0; } save; int respawnFlag=0; Respawn respawn[8]; } gSaveContext;
struct Play { int sceneId=0; struct { struct { int num=0; } curRoom; } roomCtx; } play;
Play* gPlayState=&play;
'''
# Compile the native respawn table declarations themselves as the independent reference.
native = (ROOT / 'mm/2s2h/DeveloperTools/BetterMapSelect.c').read_text()
native_tables = native[native.index('typedef struct {\n    u16 entrance;'):native.index('// clang-format on', native.index('sBetterMapSelectCowGrottoInfo[]'))]
native_tables = 'using u16=uint16_t; using s16=int16_t; using Vec3f=Vec3;\n'+native_tables
# Expected portal direction comes independently from the native EXIT declarations.
pairs=[]
for region_source in (ROOT / 'mm/2s2h/Rando/Logic/Regions').glob('*.cpp'):
    pairs.extend(re.findall(r'EXIT\((ENTRANCE\(\w+,\s*\d+\)),\s*(ENTRANCE\(\w+,\s*\d+\))', region_source.read_text()))
pairs={(re.sub(r',\s*', ', ', a), re.sub(r',\s*', ', ', b)) for a,b in pairs}
expected_native = 'void CheckNativePortals() {\n'
expected_native += 'for (const auto& p : ComboCrossMmCatalogue::Interiors()) { bool matched=false;\n'
for a,b in sorted(pairs):
    if (b,a) in pairs:
        expected_native += f'if (p.entrance=={a} && p.exit=={b}) matched=true;\n'
expected_native += 'assert(matched); }\n'
expected_native += 'for (const auto& p : ComboCrossMmCatalogue::ExpandedDungeons()) { bool matched=false;\n'
for a,b in sorted(pairs):
    if (b,a) in pairs:
        expected_native += f'if (p.entrance=={a} && p.exit=={b}) matched=true;\n'
expected_native += 'assert(matched); }\n}\n'
body = expected_native + (ROOT / 'tests/cross_dungeon/mm_catalogue_boundary_cases.inc').read_text()

with tempfile.TemporaryDirectory(prefix='mm-catalogue-') as tmp:
    source = pathlib.Path(tmp) / 'test.cpp'
    source.write_text('#include <cassert>\n#include <cstdint>\n#include <iostream>\n#include <set>\n'+constants+native_tables+'\n#include "combo/rando/CrossDungeonSeed.h"\n#include "combo/rando/CrossEntranceMmCatalogue.h"\n#include "combo/rando/CrossEntranceMmGrottos.inc.h"\n'+body)
    binary = pathlib.Path(tmp) / 'test'
    cmd = ['g++','-std=c++20','-Wall','-Wextra','-Werror','-I',str(ROOT),str(source),'-o',str(binary)]
    if args.sanitize:
        cmd += ['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g']
    subprocess.run(cmd,check=True)
    subprocess.run([str(binary)],check=True)
