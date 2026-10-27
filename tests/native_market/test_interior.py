#!/usr/bin/env python3
"""Verify the shipped market material graph, using the retained real geometry."""
import importlib.util
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/black_market'))
import native_market
import market_interior
import market_title
import exit_door
import room_magic


def preparation():
    spec = importlib.util.spec_from_file_location('prepare_market', ROOT / 'scripts/assets/prepare_native_market.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class DefaultInterior(unittest.TestCase):
    def test_archive_preparation_needs_no_image_authoring_packages(self):
        with tempfile.TemporaryDirectory() as directory:
            result = subprocess.run([sys.executable, '-S', str(ROOT / 'scripts/assets/prepare_native_market.py'),
                                     '--output-root', directory], text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr)

    def test_default_graph_has_original_surfaces_and_complete_shelf_texture(self):
        module = preparation()
        self.assertTrue(hasattr(module, 'prepared_resources'), 'Default build has no self-contained interior material pass')
        base = native_market.read_bundle(module.SOURCE)
        complete = module.prepared_resources(module.SOURCE)
        # The optional material mode has a private graph; the required default
        # graph retains its original exact inventory and may not depend on it.
        optional = {name for name in complete if name.startswith(room_magic.PREFIX)}
        self.assertIn(room_magic.ROOM, optional)
        self.assertIn(room_magic.TEXTURE, optional)
        resources = {name: data for name, data in complete.items() if name not in optional}
        for name, data in resources.items():
            if data.lstrip().startswith(b'<'):
                for node in ET.fromstring(data).iter():
                    self.assertFalse((node.get('Path') or '').startswith(room_magic.PREFIX), name)
        door_scenes = exit_door.apply(base)
        material_paths = set()
        vertex_changes = {f'{market_interior.GEOMETRY}_vtx_{suffix}' for suffix in (2,5,8,12,14,24)}
        geometry_changes = vertex_changes | {f'{market_interior.GEOMETRY}_tri_23'}
        for name, data in base.items():
            if '/mat_' not in name:
                if name in exit_door.SCENES.values():
                    self.assertEqual(resources[name], door_scenes[name], name)
                elif name not in geometry_changes:
                    self.assertEqual(resources[name], data, name)
                continue
            material = ET.fromstring(resources[name])
            image = material.find('SetTextureImage')
            path = image.get('Path')
            self.assertIn(path, resources, 'Interior material still requires a modpack or foreign bark')
            material_paths.add(path)
            self.assertEqual(material.find('SetTextureLUT').get('Mode'), 'G_TT_NONE')
            self.assertEqual(image.get('Format'), 'G_IM_FMT_RGBA')
            logical = 32 if path in {market_interior.TEXTURES[key] for key in market_interior.BASE_NAMES} else 64
            self.assertEqual(material.find('SetTileSize').get('Lrs'), str((logical-1)*4))
        self.assertGreaterEqual(len(material_paths), 10, 'Retained props need independently mapped material surfaces')
        shelf = 'custom/skiejer_market/texture/arcane_stone'
        self.assertIn(shelf, resources, 'Native shelves must never load an absent texture')
        self.assertEqual(hashlib.sha256(resources[shelf][92:]).hexdigest(),
                         '08fc11578f90b1b834b6fb026bf6defbad4374a0013a448d3fcc7822df355237',
                         'Animated shelves/backdrop must use the neutral arcane base, never floor slate')
        added = set(resources) - set(base)
        self.assertEqual(added, material_paths | {shelf, market_title.RESOURCE})
        market_title.validate_texture(resources[market_title.RESOURCE])
        for name in added - {market_title.RESOURCE}:
            data = resources[name]
            kind, width, height, flags, hs, vs, size = struct.unpack_from('<IIIIffI', data, 64)
            logical = 32 if name in {market_interior.TEXTURES[key] for key in ('timber', 'slate', 'plaster')} else 64
            self.assertEqual(data[4:8], b'XETO')
            self.assertEqual((kind, flags, width, height), (1, 1, 512, 512))
            self.assertEqual((width * 2 / hs, height / vs), (logical, logical))
            self.assertEqual(size, width * height * 4)
            self.assertEqual(len(data), 92 + size)
            self.assertEqual(set(data[95::4]), {255})
        for host, other in [('mm', native_market.SCENES['oot']), ('oot', native_market.SCENES['mm'])]:
            contract = native_market.core_contract(base, host)
            self.assertTrue(added <= set(contract))
            self.assertNotIn(native_market.STOCK_TEXTURE, contract)
            self.assertNotIn(other, contract)

    def test_every_retained_material_has_an_audited_role_and_preserves_opaque_commands(self):
        base = native_market.read_bundle(preparation().SOURCE)
        self.assertTrue(hasattr(market_interior, 'MATERIAL_ROLES'), 'All28 material IDs require an explicit role map')
        self.assertEqual(set(market_interior.MATERIAL_ROLES), set(range(28)))
        expected = {0:'liquid', 1:'slate', 2:'bronze', 3:'timber', 4:'herbs', 5:'fittings_atlas',
                    6:'cloth', 7:'cloth', 8:'timber', 9:'plaster', 10:'wicker', 11:'walltrim_atlas',
                    12:'curios', 13:'wicker', 14:'herbs', 15:'celestial', 16:'liquid', 17:'celestial',
                    18:'curios', 19:'timber', 20:'herbs', 21:'counter_atlas', 22:'cloth', 23:'ceramic',
                    24:'slate', 25:'herbs', 26:'curios', 27:'liquid'}
        after = market_interior.apply(base)
        self.assertEqual(len(after), 140)
        for group, surface in expected.items():
            role = market_interior.MATERIAL_ROLES[group]
            self.assertEqual(role['surface'], surface, group)
            self.assertTrue(role['role'])
            name = f'custom/skiejer_market/geometry/mat_mahouya_room_0DL_006548_GranmaShop{group:02}'
            old, new = ET.fromstring(base[name]), ET.fromstring(after[name])
            self.assertEqual(new.find('SetTextureImage').get('Path'), market_interior.TEXTURES[surface])
            # No changes to opaque routing, combine/alpha modes, tint or geometry.
            exempt = {'SetTextureImage', 'SetTextureLUT', 'SetTile', 'SetTileSize', 'LoadBlock'}
            self.assertEqual([(node.tag,node.attrib) for node in old if node.tag not in exempt],
                             [(node.tag,node.attrib) for node in new if node.tag not in exempt], name)
            size = 32 if surface in ('timber','slate','plaster') else 64
            self.assertEqual(new.find('SetTileSize').get('Lrs'), str((size-1)*4))
            self.assertEqual(new.find('LoadBlock').get('Lrs'), str(size*size-1))

    def test_role_audit_traces_actual_draw_owners_and_uv_islands(self):
        base = native_market.read_bundle(preparation().SOURCE)
        self.assertTrue(hasattr(market_interior, 'audit_materials'), 'Role evidence must follow real material calls')
        audit = market_interior.audit_materials(base)
        self.assertEqual(set(audit), set(range(28)))
        self.assertEqual([group for group, row in audit.items() if not row['active']], [17])
        self.assertEqual((audit[0]['triangles'], audit[10]['triangles'], audit[15]['triangles']), (36,1488,218))
        self.assertEqual(audit[0]['bounds'], [[49,37,24],[97,37,72]])
        self.assertEqual(audit[21]['uv_bounds'], [[-16,-16],[2032,2032]])
        self.assertTrue(any(path.endswith('_tri_26') for path in audit[15]['geometry']))
        self.assertFalse(any(path.endswith('_tri_27') for row in audit.values() for path in row['geometry']))
        # Three differently located cauldron body islands occupy dedicated glaze;
        # torch columns remain on their separate bronze strip in the same atlas.
        self.assertEqual(market_interior.ATLAS_RECTS['counter_atlas']['bronze'], [416,184,512,512])
        self.assertTrue(market_interior.ATLAS_RECTS['counter_atlas']['ceramic'])

    def test_prop_uv_roles_and_whole_torch_fixtures_are_bounded(self):
        base = native_market.read_bundle(preparation().SOURCE)
        after = market_interior.apply(base)
        for suffix in (5,8,12):
            path = f'{market_interior.GEOMETRY}_vtx_{suffix}'
            old, new = list(ET.fromstring(base[path])), list(ET.fromstring(after[path]))
            self.assertEqual(len(new), len(old))
            for before, prepared in zip(old,new):
                self.assertEqual({key:value for key,value in before.attrib.items() if key not in ('S','T')},
                                 {key:value for key,value in prepared.attrib.items() if key not in ('S','T')})
            # The large flat panels are paper; the disjoint skull/horn assembly
            # must sample bone, rather than glass or leather across UV overlaps.
            if suffix == 12:
                for vertex in new:
                    s,t = int(vertex.get('S')),int(vertex.get('T'))
                    if int(vertex.get('Z')) < 0:
                        self.assertTrue(32 <= s <= 992 and 32 <= t <= 992)
                    else:
                        self.assertTrue(32 <= s <= 992 and 1056 <= t <= 2016)
            if suffix == 8:
                skull = [vertex for vertex in new if 118 <= int(vertex.get('X')) <= 140 and
                         74 <= int(vertex.get('Y')) <= 120 and 171 <= int(vertex.get('Z')) <= 189]
                self.assertEqual(len(skull), 48)
                self.assertTrue(all(32 <= int(v.get('S')) <= 992 and 1056 <= int(v.get('T')) <= 2016 for v in skull))
        path = f'{market_interior.GEOMETRY}_vtx_2'
        old, new = list(ET.fromstring(base[path])), list(ET.fromstring(after[path]))
        moved = [0,0]
        from prop_appearance import POTS
        pot_bounds = {tuple(tuple(edge) for edge in spec[1]) for spec in POTS}
        pot_uv_indices = {index for part in market_interior.vertex_components(base, f'{market_interior.GEOMETRY}_tri_2')
                          if tuple(tuple(edge) for edge in part['bounds']) in pot_bounds for index in part['indices']}
        for vertex_index,(before, prepared) in enumerate(zip(old,new)):
            xyz = [int(before.get(key)) for key in ('X','Y','Z')]
            expected = xyz[:]
            for index,spec in enumerate(market_interior.TORCH_STANDS):
                low,high = spec['bounds']
                if all(low[axis] <= xyz[axis] <= high[axis] for axis in range(3)):
                    expected = [a+b for a,b in zip(xyz,spec['delta'])]
                    moved[index] += 1
            self.assertEqual([int(prepared.get(key)) for key in ('X','Y','Z')],expected)
            protected = ('R','G','B','A') if vertex_index in pot_uv_indices else ('S','T','R','G','B','A')
            self.assertEqual([prepared.get(key) for key in protected],
                             [before.get(key) for key in protected])
        self.assertEqual(moved, [78,73])
        self.assertEqual([[[a+b for a,b in zip(edge,spec['delta'])] for edge in spec['bounds']]
                          for spec in market_interior.TORCH_STANDS],
                         [[[56,0,194],[90,83,226]],[[-127,0,194],[-93,83,226]]])

    def test_checked_in_material_audit_and_native_contract_match_preparation(self):
        base = native_market.read_bundle(preparation().SOURCE)
        audit = json.loads((market_interior.ASSETS/'material-audit.json').read_text())
        self.assertEqual(audit,json.loads(json.dumps(market_interior.audit_manifest(base))))
        self.assertEqual((ROOT/'combo/BlackMarketNativeResources.h').read_text(),native_market.core_contract_header(base))

    def test_both_packers_receive_identical_graph_and_repair_a_missing_shelf(self):
        module = preparation()
        self.assertTrue(hasattr(module, 'prepared_resources'), 'Default build has no self-contained interior material pass')
        resources = module.prepared_resources(module.SOURCE)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            module.prepare(root)
            for game, host in (('soh', 'oot'), ('mm', 'mm')):
                for name, data in module.prepared_resources(host=host).items():
                    self.assertEqual((root / game / 'assets/custom' / name).read_bytes(), data)
            shelf = root / 'mm/assets/custom/custom/skiejer_market/texture/arcane_stone'
            shelf.unlink()
            self.assertEqual(module.prepare(root), 1)
            self.assertEqual(shelf.read_bytes(), resources['custom/skiejer_market/texture/arcane_stone'])


if __name__ == '__main__':
    unittest.main()
