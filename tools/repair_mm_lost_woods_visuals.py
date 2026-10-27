#!/usr/bin/env python3
"""Repair the POC3 forest atlas and move its single doorway to the corner stump."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import re
import struct
import xml.etree.ElementTree as ET
import zipfile

BASE_SHA256 = '6f120f52340da1874d5a650235bc57154cc62d5a8a44a5429a056ed8eb3f6284'
PREFIX = 'custom/cor_mm3d/lost_woods_poc1/'
OLD_ROOT = PREFIX + 'room_0/opa_combo_return_stump_v2'
NEW_ROOT = PREFIX + 'room_0/opa_combo_return_corner_v3'
ROOM = 'alt/scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS_room_00'
DOOR = {'alt/' + PREFIX + 'room_0/' + n + '_vtx' for n in (
    'ReturnRootTunnel', 'ReturnTunnelMouth', 'ReturnTunnelPinkLight',
    'ReturnMushrooms_-1', 'ReturnMushrooms_1')}
SCENERY = re.compile(re.escape('alt/' + PREFIX) + r'room_[01]/Geometry(?:\d+)?_vtx')


def sha(data):
    return hashlib.sha256(data).hexdigest()


def repair(source: Path, target: Path):
    if source.resolve() == target.resolve() or target.exists():
        raise ValueError('choose a new output and preserve the baseline')
    if sha(source.read_bytes()) != BASE_SHA256:
        raise ValueError('expected the exact Stump Return POC3 baseline')
    changed = {}
    created = False
    try:
        with zipfile.ZipFile(source) as before:
            names = before.namelist()
            if len(names) != len(set(names)) or before.testzip() is not None:
                raise ValueError('baseline integrity or duplicate resource failure')
            scenery = {n for n in names if SCENERY.fullmatch(n)}
            headers = {n for n in names if re.fullmatch(re.escape(ROOM) + r'(?:Set_[0-9A-F]+)?', n)}
            if len(scenery) != 22 or len(headers) != 4 or not DOOR <= set(names):
                raise ValueError('unexpected baseline resource contract')
            with zipfile.ZipFile(target, 'x', zipfile.ZIP_DEFLATED, compresslevel=6) as after:
                created = True
                for info in before.infolist():
                    original = data = before.read(info)
                    name = info.filename
                    if name in scenery:
                        vertices = ET.fromstring(data)
                        tile = ET.fromstring(before.read(name[:-4])).find('SetTileSize')
                        period = (int(tile.get('Lrt')) // 4 + 1) * 32
                        for vertex in vertices:
                            # The source DAE's negative V already encodes image-row
                            # order. Undo the port's second flip, preserving wrapping.
                            vertex.set('T', str(2 * period - int(vertex.get('T'))))
                        data = ET.tostring(vertices, encoding='utf-8') + b'\n'
                    elif name in DOOR:
                        vertices = ET.fromstring(data)
                        for vertex in vertices:
                            # Rigid 180-degree move: (120,240), yaw -16384 ->
                            # (-390,120), yaw 16384. Clear the stump's front root.
                            # Y/UV/color remain exact.
                            vertex.set('X', str(-270 - int(vertex.get('X'))))
                            vertex.set('Z', str(360 - int(vertex.get('Z'))))
                        data = ET.tostring(vertices, encoding='utf-8') + b'\n'
                    elif name in headers:
                        old = struct.pack('<I', len(OLD_ROOT)) + OLD_ROOT.encode()
                        new = struct.pack('<I', len(NEW_ROOT)) + NEW_ROOT.encode()
                        if data.count(old) != 1:
                            raise ValueError('expected one active POC3 room-zero mesh')
                        data = data.replace(old, new)
                    if data != original:
                        changed[name] = {'before_sha256': sha(original), 'after_sha256': sha(data)}
                    # ZipFile updates ZipInfo offsets when writing. Keep the
                    # source index valid for material lookups and comparisons.
                    after.writestr(copy.copy(info), data)
                wrapper = ('<DisplayList Version="0">\n  <CallDisplayList Path="' + PREFIX +
                           'room_0/opa" />\n  <EndDisplayList />\n</DisplayList>\n')
                wrapper_info = copy.copy(before.getinfo('alt/' + OLD_ROOT))
                wrapper_info.filename = 'alt/' + NEW_ROOT
                after.writestr(wrapper_info, wrapper)
            if set(changed) != scenery | DOOR | headers:
                raise ValueError('unexpected resource delta')
            with zipfile.ZipFile(target) as after:
                if after.testzip() is not None or set(after.namelist()) != set(names) | {'alt/' + NEW_ROOT}:
                    raise ValueError('candidate integrity/resource set failure')
                for name in names:
                    if name not in changed and before.read(name) != after.read(name):
                        raise ValueError('protected resource changed: ' + name)
        return {'baseline_sha256': BASE_SHA256, 'candidate_sha256': sha(target.read_bytes()),
                'changed_resources': changed, 'added_resource': 'alt/' + NEW_ROOT,
                'corner_stump_center': [-520, 80, 120], 'door_anchor': [-390, 0, 120], 'door_yaw': 16384,
                'preservation': 'Textures, topology, actors, collision, room-one headers and non-mesh header bytes unchanged. Scenery changes only T; doorway changes only X/Z.',
                'runtime': 'Untested; requires matching v3 source and replacement of the older Lost Woods mod.'}
    except BaseException:
        if created:
            target.unlink(missing_ok=True)
        raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('target', type=Path)
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    if args.receipt.exists() or args.receipt.resolve() in (args.source.resolve(), args.target.resolve()):
        raise ValueError('choose a new receipt distinct from source and output')
    receipt_created = False
    try:
        with args.receipt.open('x') as stream:
            receipt_created = True
            result = repair(args.source, args.target)
            json.dump(result, stream, indent=2)
            stream.write('\n')
    except BaseException:
        if receipt_created:
            args.receipt.unlink(missing_ok=True)
        raise
    print('PASS repair: 22 scenery UV resources, five doorway moves, four active mesh bindings; all other bytes preserved')


if __name__ == '__main__':
    main()
