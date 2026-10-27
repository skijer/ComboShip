#!/usr/bin/env python3
"""Move the purple return doorway onto the opening forest's large stump.

Only the added doorway's five vertex resources and four active mesh references
change. Native collision, scenery, UVs, textures, actors and room one are retained.
The input is preserved and must be the exact ExitSeasons POC2 archive.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import xml.etree.ElementTree as ET
import zipfile

BASE_SHA256 = 'f07a2e56c183b4600146434fd5f04b10cff7fd932227d33ab297e108a2bf78ac'
PREFIX = 'custom/cor_mm3d/lost_woods_poc1/room_0/'
OLD_ROOT = PREFIX + 'opa_combo_return_v1'
NEW_ROOT = PREFIX + 'opa_combo_return_stump_v2'
ROOM = 'alt/scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS_room_00'
VERTICES = { 'alt/' + PREFIX + name + '_vtx' for name in
             ('ReturnRootTunnel', 'ReturnTunnelMouth', 'ReturnTunnelPinkLight',
              'ReturnMushrooms_-1', 'ReturnMushrooms_1') }
OLD_ANCHOR = (-417, 0, -122)
NEW_ANCHOR = (120, 0, 240)
OLD_YAW, NEW_YAW = 11520, -16384


def sha(data):
    return hashlib.sha256(data).hexdigest()


def transform(x, z):
    old = OLD_YAW * math.tau / 65536
    new = NEW_YAW * math.tau / 65536
    dx, dz = x - OLD_ANCHOR[0], z - OLD_ANCHOR[2]
    lateral = dx * math.cos(old) - dz * math.sin(old)
    depth = dx * math.sin(old) + dz * math.cos(old)
    return (round(NEW_ANCHOR[0] + lateral * math.cos(new) + depth * math.sin(new)),
            round(NEW_ANCHOR[2] - lateral * math.sin(new) + depth * math.cos(new)))


def relocate(source, target):
    if source.resolve() == target.resolve() or target.exists():
        raise ValueError('choose a new output; preserve the baseline archive')
    if sha(source.read_bytes()) != BASE_SHA256:
        raise ValueError('expected the exact purple ExitSeasons POC2 baseline')
    changed, counts = {}, {}
    output_created = False
    try:
        with zipfile.ZipFile(source) as before, zipfile.ZipFile(target, 'x', zipfile.ZIP_DEFLATED, compresslevel=6) as after:
            output_created = True
            names = before.namelist()
            if before.testzip() is not None or len(names) != len(set(names)):
                raise ValueError('baseline archive integrity/duplicate resource failure')
            for info in before.infolist():
                original = data = before.read(info)
                if info.filename in VERTICES:
                    root = ET.fromstring(data)
                    if root.tag != 'Vertex' or not len(root):
                        raise ValueError('expected doorway vertices')
                    for v in root:
                        x, z = transform(int(v.get('X')), int(v.get('Z')))
                        v.set('X', str(x)); v.set('Z', str(z))
                    data = ET.tostring(root, encoding='utf-8') + b'\n'
                    counts[info.filename] = len(root)
                elif re.fullmatch(re.escape(ROOM) + r'(?:Set_[0-9A-F]+)?', info.filename):
                    old, new = OLD_ROOT.encode(), NEW_ROOT.encode()
                    needle = struct.pack('<I', len(old)) + old
                    if data.count(needle) != 1:
                        raise ValueError('expected one version-one active mesh reference')
                    data = data.replace(needle, struct.pack('<I', len(new)) + new)
                if data != original:
                    changed[info.filename] = {'before_sha256': sha(original), 'after_sha256': sha(data)}
                after.writestr(info, data)
            wrapper = ('<DisplayList Version="0">\n  <CallDisplayList Path="' + PREFIX +
                       'opa" />\n  <EndDisplayList />\n</DisplayList>\n')
            after.writestr('alt/' + NEW_ROOT, wrapper)
        headers = {name for name in names if re.fullmatch(re.escape(ROOM) + r'(?:Set_[0-9A-F]+)?', name)}
        if len(headers) != 4 or set(changed) != headers | VERTICES:
            raise ValueError('unexpected delta outside the five doorway vertices/four mesh references')
        with zipfile.ZipFile(source) as before, zipfile.ZipFile(target) as after:
            assert after.testzip() is None
            assert set(after.namelist()) == set(names) | {'alt/' + NEW_ROOT}
            for name in names:
                if name not in changed:
                    assert before.read(name) == after.read(name), name
                elif name in VERTICES:
                    old, new = [ET.fromstring(z.read(name)) for z in (before, after)]
                    assert len(old) == len(new)
                    for a, b in zip(old, new):
                        assert {k:v for k,v in a.attrib.items() if k not in ('X','Z')} == {
                            k:v for k,v in b.attrib.items() if k not in ('X','Z')}, name
                        assert (int(b.get('X')),int(b.get('Z'))) == transform(int(a.get('X')),int(a.get('Z')))
        return {'baseline_sha256':BASE_SHA256,'candidate_sha256':sha(target.read_bytes()),
                'changed_resources':changed,'vertex_counts':counts,'added_resource':'alt/'+NEW_ROOT,
                'anchor':list(NEW_ANCHOR),'yaw':NEW_YAW,
                'preservation':'Only doorway X/Z positions and active room-zero mesh identity change. All other resources, Y, UVs and RGBA are exact.',
                'runtime':'Pending; use matching source and replace the prior Lost Woods mod.'}
    except BaseException:
        if output_created:
            target.unlink(missing_ok=True)
        raise


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source',type=Path); parser.add_argument('target',type=Path)
    parser.add_argument('--receipt',type=Path,required=True)
    args=parser.parse_args()
    if args.receipt.resolve() in (args.source.resolve(), args.target.resolve()) or args.receipt.exists():
        raise ValueError('choose a new receipt path distinct from the source and candidate archives')
    receipt=relocate(args.source,args.target)
    args.receipt.write_text(json.dumps(receipt,indent=2)+'\n')
    print('PASS stump archive: five doorway vertex resources, four mesh references, one version-two wrapper; all other bytes preserved')


if __name__ == '__main__':
    main()
