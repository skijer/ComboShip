"""Exercise the production summer state and verify preview/native motion parity."""
import argparse
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import sys
sys.dont_write_bytecode = True
from run_mm_audio_runtime_test import function_body

ROOT = Path(__file__).resolve().parents[2]

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='mm-summer-') as temp:
        build = Path(temp)
        flags = ['-std=c++20', '-Wall', '-Wextra', '-Werror', '-I' + str(ROOT / 'mm')]
        if args.sanitize:
            flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
        compiler = shlex.split(os.environ.get('CXX', 'c++'))
        state = ROOT / 'mm/2s2h/Enhancements/Graphics/MMSummerAtmosphereState.cpp'
        test = build / 'summer_test'
        subprocess.run([*compiler, *flags, str(ROOT / 'mm/tests/summer_atmosphere_test.cpp'), str(state), '-o', str(test)], check=True)
        subprocess.run([str(test)], check=True)
        exporter = build / 'summer_export'
        subprocess.run([*compiler, *flags, str(ROOT / 'tools/summer_weather/export_motion.cpp'), str(state), '-o', str(exporter)], check=True)
        samples = build / 'samples.json'
        samples.write_text(subprocess.check_output([str(exporter)], text=True))
        subprocess.run(['node', str(ROOT / 'tools/summer_weather/verify_preview.cjs'), str(samples)], check=True)
        game_flags = ['-DF3DEX_GBI_2', '-DCOMBO_BUILD', '-DMM_BUILD_DLL',
                      '-DCONTROLLERBUTTONS_T=uint32_t', '-DNON_EQUIVALENT', '-DNON_MATCHING',
                      '-Wno-error', '-Wno-deprecated-enum-enum-conversion', '-fpermissive']
        for directory in ('mm/include', 'mm/include/PR', 'mm/src', 'mm/2s2h', 'mm/assets',
                          'libultraship/include', 'libultraship/src', 'combo'):
            game_flags += ['-I' + str(ROOT / directory)]
        game_flags += shlex.split(os.environ.get('MM_SUMMER_TEST_CXXFLAGS', ''))
        camera_functions = []
        for source, name in (('mm/src/libultra/gu/lookat.c', 'guLookAtF'), ('mm/2s2h/gu_pc.c', 'guMtxIdentF')):
            contents = (ROOT / source).read_text()
            start, end = function_body(contents, name)
            camera_functions.append(contents[start:end])
        (build / 'summer_native_camera.inc').write_text('\n'.join(camera_functions))
        game_flags += ['-I' + str(build)]
        renderer = build / 'summer_renderer'
        result = subprocess.run([*compiler, *flags, *game_flags,
                                 str(ROOT / 'mm/tests/summer_atmosphere_render_test.cpp'),
                                 str(ROOT / 'mm/2s2h/Enhancements/Graphics/MMSummerAtmosphere.cpp'),
                                 str(state), '-o', str(renderer)], text=True, capture_output=True)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        subprocess.run([str(renderer)], check=True)

if __name__ == '__main__':
    main()
