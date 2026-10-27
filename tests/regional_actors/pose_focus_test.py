#!/usr/bin/env python3
"""Run the production focus callback/native matrix multiply over every donor pose frame."""
from pathlib import Path
import math
import os
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_mm_pose_behavior import Rig, animation, function

CASES = [
    (25, 'object_skb/gStalchildSkel', 11, 0, [
        'object_skb/gStalchildIdleAnim', 'object_skb/gStalchildSitLaughAnim',
        'object_skb/gStalchildSitTapToesAnim', 'object_skb/gStalchildSwingOnBranchAnim',
        'object_skb/gStalchildSaluteAnim']),
    (26, 'object_zog/object_zog_Skel_029170', 9, 1000, [
        'object_zog/object_zog_Anim_0041D0', 'object_zog/object_zog_Anim_00931C']),
    (27, 'object_gg/object_gg_Skel_00F6C0', 4, 0, [
        'object_gg/object_gg_Anim_00F578', 'object_gg/object_gg_Anim_00AF40']),
]
viewer = (ROOT / 'soh/src/overlays/actors/ovl_En_Viewer/z_en_viewer.c').read_text()
matrix_source = (ROOT / 'soh/src/code/sys_matrix.c').read_text()
prefix = r'''
#include "src/overlays/actors/ovl_En_Viewer/z_en_viewer.h"
#include "src/overlays/actors/ovl_En_Viewer/static_story_mm_actor.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static MtxF model;
static MtxF* sCurrentMatrix = &model;
static void EnViewer_StaticMikauPostLimbDraw(PlayState* p, s32 limb, Gfx** dl, Vec3s* r, void* a) {}
'''
samples = []
with zipfile.ZipFile(sys.argv[1]) as archive:
    resources = {name: archive.read(name) for name in archive.namelist()
                 if name.startswith(('objects/object_skb/', 'objects/object_zog/', 'objects/object_gg/'))}
    for actor, skeleton, limb, offset, clips in CASES:
        rig = Rig(resources, 'objects/' + skeleton)
        for pose, clip in enumerate(clips):
            frames = animation(resources['objects/' + clip])
            heights = []
            for frame in frames:
                m = rig.world(frame)[limb - 1]
                target = [sum(m[row][column] * [0, offset, 0, 1][column] for column in range(4)) * .01
                          for row in range(3)]
                assert all(math.isfinite(v) for v in target)
                heights.append(target[1])
                samples.append((actor, pose, limb, clip, m, target))
            if actor == 25 and pose in (1, 2):
                assert max(heights) < 25, (clip, heights)
            print(f'PASS donor focus {clip}: {len(frames)} frames, Y {min(heights):.2f}..{max(heights):.2f}', flush=True)

def number(value):
    return format(value, '.9g')

body = ['int main(void) { EnViewer actor = {0}; Vec3s rot = {0}; Gfx* dl = NULL;']
fields = ['xx', 'xy', 'xz', 'xw', 'yx', 'yy', 'yz', 'yw', 'zx', 'zy', 'zz', 'zw']
for actor, pose, limb, clip, matrix, target in samples:
    body += ['{', f'actor.staticState.type={actor};actor.staticState.pose={pose};',
             f'assert(StaticStoryMm_GetFocusLimb({actor})=={limb});',
             f'assert(!strcmp(StaticStoryMm_GetPresentation({actor},{pose})->animationPath,"objects/{clip}"));']
    for index, field in enumerate(fields):
        row, column = divmod(index, 4)
        value = matrix[row][column] * .01 + ([12, 100, 67][row] if column == 3 else 0)
        body.append(f'model.{field}={number(value)};')
    body += ['actor.actor.focus.pos=(Vec3f){-999,-999,-999};',
             'EnViewer_StaticRegionalPostLimbDraw(NULL,0,&dl,&rot,&actor);',
             'assert(actor.actor.focus.pos.y==-999);',
             f'EnViewer_StaticRegionalPostLimbDraw(NULL,{limb},&dl,&rot,&actor);']
    for field, expected in zip('xyz', target):
        world = expected + dict(x=12, y=100, z=67)[field]
        body.append(f'assert(fabsf(actor.actor.focus.pos.{field}-({number(world)}))<.002f);')
    body += ['}']
body += ['return 0;}']
source = prefix + function(matrix_source, 'Matrix_MultVec3f')
source += function(viewer, 'EnViewer_StaticRegionalPostLimbDraw') + '\n'.join(body)
with tempfile.TemporaryDirectory(prefix='regional-pose-focus-') as temp:
    temp = Path(temp)
    cpp = temp / 'focus.c'
    cpp.write_text(source)
    folder = ROOT / 'soh/src/overlays/actors/ovl_En_Viewer'
    includes = ['soh/include', 'soh/src', 'soh/assets', 'soh', 'libultraship/include', 'combo']
    subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu11', '-w', '-DF3DEX_GBI_2',
                    '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-no-pie',
                    *['-I' + str(ROOT / p) for p in includes], str(cpp),
                    str(folder / 'static_story_actor.c'), str(folder / 'static_story_mm_actor.c'),
                    '-lm', '-o', str(temp / 'focus')], check=True)
    subprocess.run([str(temp / 'focus')], check=True, env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
print(f'PASS production posed focus/native matrix multiply: {len(samples)} donor frames, all nine poses, wrong-limb rejection, world offset and scale')
