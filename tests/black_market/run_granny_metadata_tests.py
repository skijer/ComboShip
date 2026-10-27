#!/usr/bin/env python3
from pathlib import Path
import argparse,os,subprocess,tempfile,zipfile
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--native-oot',type=Path,required=True);a=p.parse_args()
assert (ROOT/'combo/BlackMarketGrannyScene.h').exists(),'Missing read-only native Granny metadata reader'
with tempfile.TemporaryDirectory(prefix='granny-metadata-') as temp,zipfile.ZipFile(a.native_oot) as pack:
    temp=Path(temp)
    scene=temp/'scene';room=temp/'room'
    scene.write_bytes(pack.read('scenes/shared/mahouya_scene/mahouya_scene'))
    room.write_bytes(pack.read('scenes/shared/mahouya_scene/mahouya_room_0'))
    binary=temp/'test'
    subprocess.run(['c++','-std=c++20','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',
        '-I'+str(ROOT),str(ROOT/'tests/black_market/granny_metadata_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(scene),str(room)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
