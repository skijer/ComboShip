"""Verify actual prop resource bytes through the production raw upload method.

This is a renderer upload fixture, not a game screenshot or gameplay test.
--expect-legacy-failure proves the rejected RGBA16/RAW asset does not upload its
native decoded colors. Repaired assets must upload 256x128 exact RGBA colors.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import zipfile

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/npc_models'))
from native_assets import o2r
from native_assets.n64texture import decode
from run_gfx_texture_address_tests import function


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('archive',type=Path)
    p.add_argument('--expect-legacy-failure',action='store_true')
    a=p.parse_args()
    with zipfile.ZipFile(a.archive) as z:
        raw=z.read('alt/objects/object_sa/NativeNpcV1/Props/Textures/FairyOcarinaHD')
    kind,w,h,pixels=o2r.read_texture(raw)
    expected=decode(pixels,0,2,w,h).tobytes() if kind==2 else pixels
    with tempfile.TemporaryDirectory(prefix='npc-ocarina-upload-') as tmp:
        tmp=Path(tmp)
        (tmp/'texture.bin').write_bytes(raw)
        (tmp/'expected.bin').write_bytes(expected)
        source=(ROOT/'libultraship/src/fast/interpreter.cpp').read_text()
        (tmp/'npc_raw_import_production.inc').write_text(function(source,'Interpreter::ImportTextureRaw'))
        binary=tmp/'native-upload'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-O1','-g',
                        '-fsanitize=address,undefined','-fno-omit-frame-pointer','-I'+str(tmp),
                        str(ROOT/'libultraship/tests/npc_ocarina_raw_upload_test.cpp'),'-o',str(binary)],check=True)
        env=os.environ.copy()
        # LeakSanitizer cannot inspect /proc under this runner's tracing;
        # ASan bounds and UBSan checks remain enabled for the production method.
        env.setdefault('ASAN_OPTIONS','detect_leaks=0')
        r=subprocess.run([str(binary),str(tmp/'texture.bin'),str(tmp/'expected.bin')],text=True,capture_output=True,env=env)
        print(r.stdout,end='')
        if r.stderr: print(r.stderr,file=sys.stderr,end='')
        if not r.stdout:
            raise RuntimeError('Native upload fixture produced no result; exit '+str(r.returncode))
        result=json.loads(r.stdout)
        if a.expect_legacy_failure:
            if r.returncode!=2 or result['decoded_colors_exact']:
                raise AssertionError('Expected the rejected packing to fail exact native color upload')
            print('PASS: rejected packing reproduces the production upload mismatch')
        else:
            if r.returncode or not result['decoded_colors_exact']:
                raise AssertionError('Production upload differs from the reviewed color payload')
            print('PASS: production raw upload preserves all decoded pixels; ASan/UBSan clean')


if __name__=='__main__':
    main()
