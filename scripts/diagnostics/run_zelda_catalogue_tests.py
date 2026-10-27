#!/usr/bin/env python3
"""Exercise Zelda catalogue and production native/replacement-model boundaries."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile
import zipfile
import sys

from run_child_ruto_face_test import function
from run_kokiri_pose_tests import animation

ROOT = Path(__file__).resolve().parents[2]
VIEWER = ROOT / 'soh/src/overlays/actors/ovl_En_Viewer'


def run(*args):
    subprocess.run(list(map(str, args)), cwd=ROOT, check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('oot_archive', type=Path)
    args = parser.parse_args()
    with zipfile.ZipFile(args.oot_archive) as archive:
        clips = [animation(archive.read(path)) for path in (
            'objects/object_zl4/gChildZeldaAnim_000654',
            'objects/object_zl4/gChildZeldaAnim_01910C',
            'objects/object_zl2_anime2/gZelda2Anime2Anim_009FBC',
            'objects/object_zl2_anime2/gZelda2Anime2Anim_003FF8')]
        for frames, values, joints, limit in clips:
            assert frames > 1
            assert all(index + (frames - 1 if index >= limit else 0) < len(values)
                       for joint in joints for index in joint)
        assert [len(clip[2]) for clip in clips] == [18, 18, 15, 15]
        assert clips[3][0] == 20 and clips[2][1:] != clips[3][1:]
    print('PASS supplied native Zelda animations: complete compatible channels, distinct adult 20-frame loop', flush=True)

    source = (VIEWER / 'z_en_viewer.c').read_text()
    fixture = (ROOT / 'soh/tests/static_story_zelda_viewer_test.c').read_text()
    fixture = fixture.replace('/* PRODUCTION_INCLUDES */', source[:source.index('#define FLAGS')])
    fixture = fixture.replace('/* PRODUCTION_MATRIX_MULTIPLY */', function(
        (ROOT / 'soh/src/code/sys_matrix.c').read_text(), 'Matrix_MultVec3f'))
    fixture = fixture.replace('/* PRODUCTION_NATIVE_INIT */', function(
        (ROOT / 'soh/src/code/z_skelanime.c').read_text(), 'SkelAnime_InitFlex'))
    fixture = fixture.replace('/* PRODUCTION_ANIMATION_AND_SKELETON */', function(
        source, 'EnViewerStatic_GetAnimation') + function(source, 'EnViewerStatic_InitSkeleton'))
    fixture = fixture.replace('/* PRODUCTION_CHILD_DRAW */', function(
        source, 'EnViewer_StaticChildZeldaOverrideLimbDraw') + function(source, 'EnViewer_DrawStaticChildZelda'))
    fixture = fixture.replace('/* PRODUCTION_ADULT_DRAW */', function(source, 'EnViewer_DrawStaticAdultZelda'))
    flags = ['-std=gnu11', '-DF3DEX_GBI_2', '-DCONTROLLERBUTTONS_T=uint32_t', '-DLOG_LEVEL_GAME_PRINTS=0',
             '-I' + str(VIEWER),
             '-ffunction-sections', '-fdata-sections', '-Wno-int-conversion', '-Wno-incompatible-pointer-types',
             '-Wno-discarded-qualifiers', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-no-pie',
             *['-I' + p for p in ('libultraship/include', 'soh/include', 'soh/src', 'soh/assets', 'soh',
                                  'soh/mods', 'combo', 'combo/menu')]]
    with tempfile.TemporaryDirectory(prefix='zelda-catalogue-') as directory:
        work = Path(directory)
        run('cc', *flags, ROOT / 'soh/tests/static_story_zelda_test.c', VIEWER / 'static_story_actor.c',
            VIEWER / 'static_story_dialogue.c', '-Wl,--gc-sections', '-lm', '-o', work / 'catalogue')
        run(work / 'catalogue')
        test_source = work / 'viewer.c'
        test_source.write_text(fixture)
        for combo in (False, True):
            binary = work / ('viewer-combo' if combo else 'viewer-original')
            run('cc', *flags, *(['-DCOMBO_BUILD'] if combo else []), test_source,
                VIEWER / 'static_story_actor.c', VIEWER / 'static_story_dialogue.c',
                '-Wl,--gc-sections', '-lm', '-o', binary)
            run(binary)
            run('cc', *flags, *(['-DCOMBO_BUILD'] if combo else []), '-fsyntax-only', VIEWER / 'z_en_viewer.c')
    print('PASS full native viewer compilation: Combo and original SoH guards', flush=True)


if __name__ == '__main__':
    main()
