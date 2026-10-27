#!/usr/bin/env python3
"""Verify the HD resource's loading contract and preservation of the POC1 mesh."""
import argparse
import json
from pathlib import Path
import struct
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/assets'))
sys.path.insert(0, str(Path(__file__).parent))
import build_chateau_romani_3ds as builder
from verify import Graph

PREFIX = builder.PREFIX


def verify_hd(files, baseline):
    assert set(files) == set(baseline), 'Resource inventory changed'
    atlas = files[PREFIX + 'AtlasTex']
    version = struct.unpack_from('<I', atlas, 8)[0]
    assert version == 1, 'HD atlas must use texture version 1 with explicit scaling'
    kind, width, height, flags, hs, vs, length = struct.unpack_from('<IIIIffI', atlas, 64)
    assert (kind, width, height, flags, hs, vs, length) == (1, 4096, 4096, 1, 64., 64., 4096*4096*4)
    assert len(atlas) == 92 + length, 'Truncated or surplus texture payload'
    reflection = files[PREFIX + 'GlassReflectionTex']
    assert struct.unpack_from('<I', reflection, 8)[0] == 1, 'Glass sheen needs the smooth HD reflection map'
    assert struct.unpack_from('<IIIIffI', reflection, 64) == (1,512,512,1,16.,16.,512*512*4)
    assert len(reflection) == 92+512*512*4
    # Native 64-pixel commands/UVs stay in range; the raw loader uploads 4K.
    assert int(64*4*hs) == width*4 and int(64*vs) == height
    allowed = {'AtlasTex', 'GlassReflectionTex', 'GlassSurfaceDL', 'GlassReflectionDL', 'LabelReflectionDL'}
    changed = {k for k in files if files[k] != baseline[k]}
    assert changed == {PREFIX+k for k in allowed}, changed
    for role, alpha in [('GlassSurface', 46), ('GlassReflection', 110), ('LabelReflection', 40)]:
        old, new = bytearray(baseline[PREFIX+role+'DL']), bytearray(files[PREFIX+role+'DL'])
        offsets = [i for i in range(72, len(new), 8) if struct.unpack_from('<I', new, i)[0] == 0xFA000000]
        assert len(offsets) == 2
        assert struct.unpack_from('<I', new, offsets[0]+4)[0] == 0xFFFFFF00 | alpha
        new[offsets[0]+4] = old[offsets[0]+4]
        assert new == old, 'Unexpected material command change: '+role
    graph = Graph(files)
    opaque = graph.triangles(PREFIX+'OpaqueDL')
    glass = graph.triangles(PREFIX+'GlassDL')
    assert (len(opaque), len(glass)) == (96,376)
    for root in ['gGiChateauRomaniBottleEmptyDL','gGiChateauRomaniBottleDL']:
        graph.triangles(builder.NATIVE+root)
    assert graph.used == set(files), 'Unresolved or unreachable resource'
    return {'result':'PASS', 'atlas':[width,height], 'texture_version':version,
            'resources':len(files), 'triangles':len(opaque)+len(glass),
            'all_vertex_and_uv_bytes_preserved':True, 'only_expected_materials_changed':True,
            'native_coordinate_hd_scaling':[hs,vs], 'runtime_proven':False}


if __name__ == '__main__':
    p=argparse.ArgumentParser()
    p.add_argument('--archive',type=Path,required=True)
    p.add_argument('--baseline',type=Path,required=True)
    a=p.parse_args()
    def read(path):
        with zipfile.ZipFile(path) as z:
            assert z.testzip() is None
            assert len(z.namelist()) == len(set(z.namelist()))
            return {k:z.read(k) for k in z.namelist()}
    print(json.dumps(verify_hd(read(a.archive),read(a.baseline))))
