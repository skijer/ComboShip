#!/usr/bin/env python3
"""Check the prepared native Mirror ramp's XLU owner and state contract."""
import copy
from pathlib import Path
import re
import sys
import struct
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/black_market'))
import forest_presentation as presentation
import native_forest as native


def check_material():
    material = ET.fromstring(presentation.ramp_material())
    low = [n for n in material if n.tag == 'SetOtherMode' and n.get('Cmd') == 'G_SETOTHERMODE_L'][0]
    assert low.get('G_RM_AA_ZB_XLU_SURF') == low.get('G_RM_AA_ZB_XLU_SURF2') == '1'
    assert not any('OPA' in k for k in low.attrib)
    color = material.find('SetPrimColor')
    assert [color.get(k) for k in 'RGB'] == ['184', '112', '248']
    assert color.get('A') == '112'
    combiner = material.find('SetCombineLERP')
    assert combiner.get('Ad0') == combiner.get('Ad1') == 'G_ACMUX_PRIMITIVE'
    assert material.find('Texture').get('Tile') == '1'
    assert material.find('CallDisplayList').get('Path') == '>0x08000000'
    texture = material.find('LoadTextureBlock')
    assert (texture.get('Width'), texture.get('Height'), texture.get('Format'), texture.get('Size')) == ('32', '64', '4', '1')
    assert texture.get('Path') == presentation.ART + 'prelude/black_market_wall_magic_mask'
    assert material.find('SetGeometryMode').get('G_CULL_BACK') == '1'


def room_fixtures():
    ramp = presentation.CORE + 'mirror_access_root'
    resources = {}
    for mode, prefix in native.SCENES.items():
        mesh = {'data': 1, 'type': 2, 'entries': [
            {'type': 2, 'bounds': (0, 1, 2, 3), 'opaque': 'protected/opaque', 'translucent': 'protected/translucent'},
            {'type': 2, 'bounds': (-225, 26, 416, 137), 'opaque': ramp, 'translucent': ''}]}
        commands = {10: native.write_mesh(mesh), 20: b''}
        resources[prefix + '_room_0'] = native.write_scene(commands)
    return resources


def check_routing():
    before = room_fixtures()
    snapshot = dict(before)
    after = presentation.route_ramp_xlu(before)
    assert before == snapshot and set(after) == set(before)
    root = presentation.CORE + 'mirror_access_root'
    for mode, prefix in native.SCENES.items():
        path = prefix + '_room_0'
        old = native.read_scene(before[path], oot=mode == 'oot')
        new = native.read_scene(after[path], oot=mode == 'oot')
        assert old.keys() == new.keys()
        assert {k: v for k, v in old.items() if k != 10} == {k: v for k, v in new.items() if k != 10}
        old_mesh, new_mesh = native.read_mesh(old[10]), native.read_mesh(new[10])
        expected = copy.deepcopy(old_mesh)
        expected['entries'][1].update(opaque='', translucent=root)
        assert new_mesh == expected
        pointer = struct.pack('<I', len(root.encode())) + root.encode()
        assert after[path] == before[path].replace(pointer + bytes(4), bytes(4) + pointer)
    assert presentation.route_ramp_xlu(after) == after
    for error in ('missing', 'duplicate', 'mixed', 'wrong_mesh'):
        bad = dict(before)
        path = next(iter(bad))
        commands = native.read_scene(bad[path])
        mesh = native.read_mesh(commands[10])
        if error == 'missing':
            mesh['entries'].pop()
        elif error == 'duplicate':
            mesh['entries'].append(copy.deepcopy(mesh['entries'][1]))
        elif error == 'mixed':
            mesh['entries'][1]['translucent'] = 'protected/other'
        else:
            mesh['type'] = 0
        commands[10] = native.write_mesh(mesh)
        bad[path] = native.write_scene(commands)
        try:
            presentation.route_ramp_xlu(bad)
        except ValueError:
            pass
        else:
            raise AssertionError('accepted invalid ramp owner: ' + error)


def check_native_blender():
    source = (ROOT / 'mm/include/PR/gbi.h').read_text()
    match = re.search(r'#define RM_AA_ZB_XLU_SURF\(clk\)(.*?)(?=\n\n)', source, re.S)
    assert match
    body = match[1]
    assert 'Z_CMP' in body and 'FORCE_BL' in body and 'Z_UPD' not in body
    assert 'G_BL_A_IN' in body and 'G_BL_1MA' in body


def check_restore():
    root = ET.Element('DisplayList', Version='0')
    ET.SubElement(root, 'CallDisplayList', Path=presentation.CORE + 'mirror_access_material')
    ET.SubElement(root, 'LoadVertices', Path='protected/vertices', VertexBufferIndex='0', VertexOffset='0', Count='3')
    ET.SubElement(root, 'Triangle1', V00='0', V01='1', V02='2', Flag0='0')
    ET.SubElement(root, 'EndDisplayList')
    source = native._xml(root)
    restored = ET.fromstring(presentation.ramp_root({presentation.CORE + 'mirror_access_root': source}))
    assert [ET.tostring(n) for n in list(restored)[:3]] == [ET.tostring(n) for n in list(root)[:3]]
    lows = [n for n in restored if n.tag == 'SetOtherMode' and n.get('Cmd') == 'G_SETOTHERMODE_L']
    assert len(lows) == 1 and lows[0].get('G_RM_AA_ZB_OPA_SURF2') == '1'
    assert lows[0].get('G_RM_FOG_SHADE_A') == '1'
    combine = restored.find('SetCombineLERP')
    assert combine.get('Ad0') == 'G_ACMUX_TEXEL0'
    assert combine.get('Aa1') == 'G_ACMUX_COMBINED' and combine.get('Ac1') == 'G_ACMUX_PRIMITIVE'
    high = [n for n in restored if n.tag == 'SetOtherMode' and n.get('Cmd') == 'G_SETOTHERMODE_H'][0]
    assert high.get('G_CYC_2CYCLE') == '1'
    assert restored.find('Texture').get('Tile') == '0'
    assert list(restored)[-1].tag == 'EndDisplayList'


def run():
    check_material()
    check_routing()
    check_native_blender()
    check_restore()
    print('PASS: native XLU/depth/alpha/tile1/segment08; all-three-host room routing, idempotence, preservation and 4 malformed-owner controls')


if __name__ == '__main__':
    run()
