#!/usr/bin/env python3
"""The optional room material graph must never rewrite the accepted room."""
import hashlib
import importlib
import json
from pathlib import Path
import shutil
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / 'tools/black_market'), str(ROOT / 'scripts/assets')]
import exit_door
import market_interior
import market_title
import mm_native_shop
import mm_shop_presentation
import native_market


class RoomMagicAssets(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.magic = importlib.import_module('room_magic')
        base = exit_door.apply(market_title.apply(market_interior.apply(
            native_market.read_bundle(ROOT / 'tools/native_market/bundled'))))
        cls.originals = {'oot': base, 'mm': mm_shop_presentation.apply(mm_native_shop.apply(base))}

    def test_private_variants_keep_every_existing_resource_exact(self):
        for host, original in self.originals.items():
            with self.subTest(host=host):
                result = self.magic.apply(original, host)
                self.assertEqual({p: result[p] for p in original}, original)
                self.assertTrue(all(p.startswith(self.magic.PREFIX) for p in result.keys() - original.keys()))
                self.assertEqual(self.magic.apply(original, host), result)
                # On/off uses the same current vertices, including MM's moved
                # basket and cauldron components and corrected book/glass UVs.
                def vertices(resources, path):
                    seen, found = set(), []
                    def visit(name):
                        if name in seen:
                            return
                        seen.add(name)
                        for node in ET.fromstring(resources[name]):
                            if node.tag in ('CallDisplayList', 'JumpToDisplayList'):
                                visit(node.get('Path'))
                            elif node.tag == 'LoadVertices':
                                found.append((node.get('Path'), dict(node.attrib)))
                    visit(path)
                    return found
                source = (mm_shop_presentation.CLONES[mm_shop_presentation.ROOM_ROOT]
                          if host == 'mm' else 'custom/skiejer_market/room')
                self.assertEqual(vertices(result, self.magic.ROOM), vertices(original, source))

    def test_all_original_material_roles_have_separate_room_colors_and_scroll(self):
        for host, original in self.originals.items():
            result = self.magic.apply(original, host)
            for index in range(28):
                name = f'geometry/mat_mahouya_room_0DL_006548_GranmaShop{index:02}'
                before = ET.fromstring(original['custom/skiejer_market/' + name])
                after = ET.fromstring(result[self.magic.PREFIX + name])
                self.assertEqual(after.find('SetTextureImage').get('Path'), self.magic.TEXTURE)
                self.assertEqual(tuple(int(after.find('SetPrimColor').get(c)) for c in 'RGB'),
                                 ((165,148,230), (135,194,210), (204,170,215), (175,160,216))[index % 4])
                self.assertEqual(after.find('SetPrimColor').get('A'), before.find('SetPrimColor').get('A'))
                scroll = after.findall('ScrollTexture')
                self.assertEqual(len(scroll), 1)
                self.assertEqual((int(scroll[0].get('SpeedX')), int(scroll[0].get('SpeedY'))),
                                 (-1 if index % 3 == 0 else 1, 0 if index % 2 else 1))
                # Render state and shading stay on the established native path.
                changed = {'SetTextureImage','SetTextureLUT','SetTile','LoadBlock','SetTileSize','SetPrimColor','ScrollTexture'}
                stable = lambda tree: [(n.tag, dict(n.attrib)) for n in tree if n.tag not in changed]
                self.assertEqual(stable(before), stable(after))

    def test_texture_is_neutral_recovered_art_in_independent_resource(self):
        result = self.magic.apply(self.originals['mm'], 'mm')
        data = result[self.magic.TEXTURE]
        self.assertEqual(struct.unpack_from('<IIIIffI', data, 64), (1,512,512,1,16.0,8.0,1048576))
        self.assertEqual(hashlib.sha256(data[92:]).hexdigest(),
                         '08fc11578f90b1b834b6fb026bf6defbad4374a0013a448d3fcc7822df355237')
        self.assertNotEqual(self.magic.TEXTURE, market_interior.SHELF)
        self.assertEqual(result[market_interior.SHELF], self.originals['mm'][market_interior.SHELF])

    def test_builtin_magic_graph_is_closed_without_mod_archives_in_both_asset_modes(self):
        # Only the normal port preparation inputs are present. There are no
        # mounted Djipi/Granny/mod resources to satisfy a missing dependency.
        for host, original in self.originals.items():
            resources = self.magic.apply(original, host)
            for alt in (False, True):
                with self.subTest(host=host, alt=alt):
                    visited = set()
                    def resolve(path):
                        selected = 'alt/' + path if alt and 'alt/' + path in resources else path
                        self.assertIn(selected, resources, 'magical room depends on an external archive: ' + selected)
                        self.assertTrue(selected.removeprefix('alt/').startswith('custom/skiejer_market/'))
                        return selected
                    def visit(path):
                        selected = resolve(path)
                        if selected in visited:
                            return
                        visited.add(selected)
                        data = resources[selected]
                        if not data.lstrip().startswith(b'<'):
                            return
                        for node in ET.fromstring(data):
                            child = node.get('Path')
                            if child:
                                visit(child)
                    visit(self.magic.ROOM)
                    for index in range(28):
                        visit(self.magic.PREFIX + f'geometry/mat_mahouya_room_0DL_006548_GranmaShop{index:02}')
                    self.assertIn(self.magic.TEXTURE, visited)

    def test_corrupt_or_reordered_source_parts_are_rejected(self):
        # Exercise the real decoder used by room preparation; compressed source
        # corruption must fail rather than produce a broken native upload.
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            manifest = json.loads((market_interior.ASSETS / 'shelf-encoded.json').read_text())
            shutil.copy(market_interior.ASSETS / 'shelf-encoded.json', directory)
            for part in manifest['parts']:
                shutil.copy(market_interior.ASSETS / part['name'], directory)
            path = directory / manifest['parts'][0]['name']
            path.write_bytes(b'A' + path.read_bytes()[1:])
            with patch.object(market_interior, 'ASSETS', directory):
                market_interior.shelf_pixels.cache_clear()
                with self.assertRaises(ValueError):
                    self.magic.texture()
            market_interior.shelf_pixels.cache_clear()
            shutil.copy(market_interior.ASSETS / manifest['parts'][0]['name'], directory)
            manifest['parts'].reverse()
            (directory / 'shelf-encoded.json').write_text(json.dumps(manifest))
            with patch.object(market_interior, 'ASSETS', directory):
                with self.assertRaises(ValueError):
                    self.magic.texture()
            market_interior.shelf_pixels.cache_clear()

    def test_recursive_or_missing_room_resources_fail_closed(self):
        source = dict(self.originals['oot'])
        path = 'custom/skiejer_market/room'
        source[path] = f'<DisplayList><CallDisplayList Path="{path}"/></DisplayList>'.encode()
        with self.assertRaisesRegex(ValueError, 'Recursive'):
            self.magic.apply(source, 'oot')
        del source[path]
        with self.assertRaisesRegex(ValueError, 'Missing'):
            self.magic.apply(source, 'oot')

    def test_source_preparation_includes_variant_for_both_hosts(self):
        import prepare_native_market
        for host in ('oot', 'mm'):
            result = prepare_native_market.prepared_resources(host=host)
            self.assertIn(self.magic.ROOM, result)
            self.assertIn(self.magic.TEXTURE, result)


if __name__ == '__main__':
    unittest.main()
