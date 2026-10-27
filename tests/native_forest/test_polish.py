"""Active preparation must apply the bounded presentation with bounded room draw ownership edits."""
import hashlib
import io
import itertools
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET
import zipfile

from test_generator import ROOT

sys.path.insert(0, str(ROOT / 'scripts/assets'))
from prepare_native_forest import prepared_resources
from native_forest_bundle import load_bundle

CORE = 'custom/comboship/native_forest/'
ROOMS = {
    'scenes/nonmq/comboship_forest/comboship_forest_room_0': False,
    'scenes/nonmq/skiejer_forest/skiejer_forest_room_0': False,
    'scenes/shared/skiejer_forest_oot/skiejer_forest_oot_room_0': True,
}


class PolishTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = ROOT / 'tools/native_forest/manifest.json'
        cls.metadata = json.loads(cls.manifest.read_text())
        with zipfile.ZipFile(io.BytesIO(load_bundle(cls.manifest.parent))) as archive:
            cls.before = {name: archive.read(name) for name in archive.namelist()}
        cls.after = prepared_resources(cls.manifest)

    def test_preparation_applies_only_the_pinned_presentation_and_room_draw_changes(self):
        self.assertEqual(self.metadata.get('presentation_polish', {}).get('version'), 2)
        self.assertEqual(set(self.before), set(self.after))
        changed = {name for name in self.before if self.before[name] != self.after[name]}
        self.assertEqual(changed, set(self.metadata['presentation_polish']['resources']))
        self.assertEqual(changed, set(ROOMS) | {CORE + name for name in (
            'authored/room_0/ReturnTunnelMouth',
            'building/recipe7624751371212635_vtx',
            'mirror_access_material', 'mirror_access_root', 'portal_xlu',
        )})
        for name in changed:
            self.assertNotEqual(self.before[name][4:8], b'XETO', name)
            self.assertEqual(hashlib.sha256(self.after[name]).hexdigest(),
                             self.metadata['presentation_polish']['resources'][name]['sha256'])
        self.assertTrue(changed)

    def test_room_changes_only_move_the_ramp_from_opaque_to_translucent(self):
        from native_forest import read_mesh, read_scene, write_mesh, write_scene
        ramp = CORE + 'mirror_access_root'
        for name, oot in ROOMS.items():
            with self.subTest(room=name):
                commands = read_scene(self.before[name], oot=oot)
                mesh = read_mesh(commands[10])
                self.assertEqual(mesh['type'], 2)
                entries = [entry for entry in mesh['entries']
                           if ramp in (entry['opaque'], entry['translucent'])]
                self.assertEqual(len(entries), 1)
                self.assertEqual((entries[0]['opaque'], entries[0]['translucent']), (ramp, ''))
                entries[0]['opaque'], entries[0]['translucent'] = '', ramp
                commands[10] = write_mesh(mesh)
                # All other scene commands, mesh entries, bounds and headers stay byte-exact.
                self.assertEqual(self.after[name], write_scene(commands))

    def test_portal_uses_building_purple_and_has_no_legacy_frame_glow(self):
        from native_forest import display_commands
        name = CORE + 'authored/room_0/ReturnTunnelMouth'
        colors = [word for _, _, word, op in display_commands(self.after[name], name) if op == 0xFA]
        self.assertEqual(colors, [0x58188060])
        calls = [node.get('Path') for node in ET.fromstring(self.after[CORE + 'portal_xlu']) if node.get('Path')]
        self.assertEqual(calls, [name])

    def test_signs_swap_atlas_cells_while_avatar_and_all_positions_stay_exact(self):
        name = CORE + 'building/recipe7624751371212635_vtx'
        before, after = self.before[name], self.after[name]
        self.assertEqual(before[:72], after[:72])
        for index in range(52):
            offset = 72 + index * 16
            old = struct.unpack_from('<hhhHhhBBBB', before, offset)
            new = struct.unpack_from('<hhhHhhBBBB', after, offset)
            self.assertEqual(old[:4] + old[5:], new[:4] + new[5:])
            self.assertEqual(new[4], old[4] + (379 if old[2] > 480 else -379) if index < 48 else old[4])

    def test_ramp_owns_its_full_single_tile_emissive_pipeline(self):
        material = ET.fromstring(self.after[CORE + 'mirror_access_material'])
        high = material.find("SetOtherMode[@Cmd='G_SETOTHERMODE_H']")
        self.assertIsNotNone(high, 'Ramp must not inherit cycle/LOD state from the previous culled draw')
        self.assertEqual(high.get('G_CYC_1CYCLE'), '1')
        self.assertEqual(high.get('G_TL_TILE'), '1')
        texture = material.find('LoadTextureBlock')
        self.assertEqual(texture.get('Path'), CORE + 'authored/prelude/black_market_wall_magic_mask')
        self.assertEqual((texture.get('Width'), texture.get('Height')), ('32', '64'))
        self.assertEqual(texture.get('Format'), '4', 'The authored mask is native I8, not IA8')
        self.assertEqual(struct.unpack_from('<I', self.before[texture.get('Path')], 64)[0], 6)
        self.assertEqual(material.find('SetPrimColor').get('A'), '112')
        low = material.find("SetOtherMode[@Cmd='G_SETOTHERMODE_L']")
        self.assertIsNotNone(low)
        self.assertEqual(low.attrib, {
            'Cmd': 'G_SETOTHERMODE_L', 'Sft': '0', 'Length': '32',
            'G_AC_NONE': '1', 'G_ZS_PIXEL': '1',
            'G_RM_AA_ZB_XLU_SURF': '1', 'G_RM_AA_ZB_XLU_SURF2': '1',
        })
        combine = material.find('SetCombineLERP')
        self.assertIsNotNone(combine)
        for cycle in ('0', '1'):
            self.assertEqual([combine.get(key + cycle) for key in ('Aa', 'Ab', 'Ac', 'Ad')],
                             ['G_ACMUX_0', 'G_ACMUX_0', 'G_ACMUX_0', 'G_ACMUX_PRIMITIVE'])
        self.assertEqual(material.find('SetEnvColor').get('R'), '88')
        self.assertEqual(self.before[CORE + 'mirror_access_vtx'], self.after[CORE + 'mirror_access_vtx'])

    def test_ramp_uses_the_authored_second_layer_scroll_in_both_hosts(self):
        from native_forest import scroll_bindings
        material = ET.fromstring(self.after[CORE + 'mirror_access_material'])
        image = material.find('LoadTextureBlock')
        track = next(row for row in scroll_bindings(self.after[CORE + 'texanim']) if row['segment'] == 8)
        layer = track['layers'][1]
        self.assertEqual((layer['x'], layer['y'], layer['width'], layer['height']), (0, -1, 32, 64))
        self.assertEqual((int(image.get('Width')), int(image.get('Height'))),
                         (layer['width'], layer['height']))
        self.assertEqual(material.find('Texture').get('Tile'), '1')
        tile = material.find("SetTile[@Tile='1']")
        self.assertIsNotNone(tile, 'The ramp mask must own render tile one before the authored scroll call')
        self.assertEqual((tile.get('Format'), tile.get('Size'), tile.get('Line'), tile.get('TMem')),
                         ('G_IM_FMT_I', 'G_IM_SIZ_8b', '4', '0'))
        calls = material.findall('CallDisplayList')
        self.assertEqual([node.get('Path') for node in calls], ['>0x08000000'])
        nodes = list(material)
        self.assertLess(nodes.index(tile), nodes.index(calls[0]), 'Scroll must follow texture/tile setup')
        self.assertEqual(self.before[CORE + 'texanim'], self.after[CORE + 'texanim'])
        self.assertEqual(self.before[CORE + 'stock_materials'], self.after[CORE + 'stock_materials'])

    def test_native_stump_keeps_two_cycle_material_after_each_culled_ramp_order(self):
        from native_forest import display_commands
        stump = CORE + 'geometry/Z2_LOST_WOODS_room_00DL_002548'
        # This is the real native I8/I4 bark combiner, not a replacement fixture.
        self.assertIn((0xFC272C04, 0x1F0C93FF),
                      [(a, b) for _, a, b, op in display_commands(self.after[stump], stump) if op == 0xFC])
        resources = self.after

        def draw(path, state, observations):
            data = resources[path]
            if data.startswith(b'<DisplayList'):
                for node in ET.fromstring(data):
                    if node.tag == 'CallDisplayList':
                        target = node.get('Path')
                        if not target.startswith('>'):
                            draw(target, state, observations)
                    elif node.tag == 'SetOtherMode' and node.get('Cmd') == 'G_SETOTHERMODE_H':
                        if node.get('G_CYC_1CYCLE'):
                            state['cycle'] = 1
                        elif node.get('G_CYC_2CYCLE'):
                            state['cycle'] = 2
                    elif node.tag == 'SetGeometryMode' and node.get('G_FOG'):
                        state['fog'] = True
                    elif node.tag == 'ClearGeometryMode' and node.get('G_FOG'):
                        state['fog'] = False
            elif data[4:8] == b'TLDO':
                for _, _, _, opcode in display_commands(data, path):
                    if path == stump and opcode in (5, 6):
                        observations.append(dict(state))

        # Culling sorts by camera depth; test both orders and repetition/re-entry.
        for order in itertools.permutations((CORE + 'mirror_access_root', stump)):
            state, observations = {'cycle': 2, 'fog': True}, []
            for path in order * 2:
                draw(path, state, observations)
            self.assertTrue(observations)
            self.assertTrue(all(value == {'cycle': 2, 'fog': True} for value in observations), order)

    def test_authoring_rebakes_still_receive_the_immutable_source_bytes(self):
        from prepare_native_forest import verified_resources
        self.assertEqual(verified_resources(self.manifest), self.before)

    def test_a_bad_polish_receipt_fails_before_preparing_either_port(self):
        from prepare_native_forest import prepare
        for damage in ('version', 'hash', 'membership'):
            with self.subTest(damage=damage), tempfile.TemporaryDirectory(prefix='forest-polish-receipt-') as folder:
                root = Path(folder)
                metadata = json.loads(self.manifest.read_text())
                polish = metadata['presentation_polish']
                if damage == 'version':
                    polish['version'] = 3
                elif damage == 'hash':
                    polish['resources'][CORE + 'mirror_access_material']['sha256'] = '0' * 64
                else:
                    polish['resources'].pop(CORE + 'portal_xlu')
                manifest = root / 'manifest.json'
                manifest.write_text(json.dumps(metadata))
                (root / 'parts').symlink_to(self.manifest.parent / 'parts', target_is_directory=True)
                with self.assertRaises(ValueError):
                    prepare(root / 'output', manifest)
                self.assertFalse((root / 'output').exists())


if __name__ == '__main__':
    unittest.main()
