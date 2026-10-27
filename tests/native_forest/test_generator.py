#!/usr/bin/env python3
"""Stock forest generation: real native edit plus a controlled ROM graph.

The small ROM graph exercises resource ownership and serialization. It is not
the absent original MM archive and must never become a shipped asset bundle.
"""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import struct
import tempfile
import unittest
import xml.etree.ElementTree as ET
import zipfile

ROOT = Path(__file__).resolve().parents[2]
MODULE = ROOT / 'tools/black_market/native_forest.py'
SCENE = 'scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS'
PREFIX = 'custom/comboship/native_forest/'
BARK = 'scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORITex_00BAC0'
COMBINED = 'scenes/nonmq/comboship_forest/comboship_forest'
MM = 'scenes/nonmq/skiejer_forest/skiejer_forest'
OOT = 'scenes/shared/skiejer_forest_oot/skiejer_forest_oot'
EDIT = ROOT / 'tools/magic_mirror/scene/LostWoods-MirrorStump.prelude.o2r'


def header(kind):
    return struct.pack('<B3xIIQ', 0, kind, 0, 0xDEADBEEFDEADBEEF).ljust(64, b'\0')


def string(value):
    data = value.encode()
    return struct.pack('<I', len(data)) + data


def crc64(value):
    result = 0xFFFFFFFFFFFFFFFF
    for byte in value.encode():
        result ^= byte << 56
        for _ in range(8):
            result = ((result << 1) ^ (0x42F0E1EBA9EA3693 if result >> 63 else 0)) & 0xFFFFFFFFFFFFFFFF
    return result


def expanded(opcode, path, word=0, low=0):
    value = crc64(path)
    return struct.pack('<IIII', (opcode << 24) | low, word, value >> 32, value & 0xFFFFFFFF)


def display(path, vertices, texture, child=None):
    data = header(0x4F444C54) + bytes([4]) + bytes(7)
    data += expanded(0x33, path)
    if child:
        data += expanded(0x31, child)
    else:
        data += expanded(0x20, texture)
        data += expanded(0x32, vertices, 0, (3 << 12) | (3 << 1))
        data += struct.pack('<II', 0x05000204, 0)
    return data + struct.pack('<II', 0xDF000000, 0)


def vertex(points):
    return (header(0x4F415252) + struct.pack('<II', 25, len(points)) +
            b''.join(struct.pack('<hhhHhhBBBB', *point, 0, 0, 0, 255, 255, 255, 255) for point in points))


def load_module(test):
    test.assertTrue(MODULE.is_file(), 'Native stock geometry generator is missing')
    spec = importlib.util.spec_from_file_location('native_forest', MODULE)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def real_edit():
    with zipfile.ZipFile(EDIT) as archive:
        return {name: archive.read(name) for name in archive.namelist() if not name.startswith('prelude/')}


def fixture(module):
    edit = real_edit()
    native = {name.removeprefix('alt/'): data for name, data in edit.items()
              if data[4:8] in (b'MORO', b'LOCO')}
    texture = SCENE + 'Tex_00B9D0'
    native[texture] = header(0x4F544558) + bytes(24)
    native[BARK] = header(0x4F544558) + bytes(24)
    animation = header(0x4F54414E) + struct.pack('<Ibh4b4bI', 1, -1, 1, 0, 0, 32, 32, 0, 1, 32, 64, 0)
    native[SCENE + 'TexAnim_001BD0'] = animation
    # Real room0/1 retain every original type2 cull entry. Each root has a
    # controlled render graph, including a changed vertex hidden behind a call.
    for room in range(2):
        commands = module.read_scene(native[SCENE + f'_room_0{room}'])
        mesh = module.read_mesh(commands[10])
        for entry in mesh['entries']:
            for path in (entry['opaque'], entry['translucent']):
                if not path:
                    continue
                vtx = path + '_vertices'
                native[vtx] = vertex([(0, 0, 0), (0, 40, 0), (40, 0, 0)])
                native[path] = display(path, vtx, texture)
    stump_root = SCENE + '_room_00DL_002548'
    stump_child = SCENE + '_room_00DL_fixture_child'
    moved_a = SCENE + '_room_00Vtx_0019D8'
    moved_b = SCENE + '_room_00Vtx_002820'
    native[moved_a] = vertex([(-520, 0, 120), (-520, 80, 120), (-480, 0, 120)])
    native[moved_b] = vertex([(0, 0, 0), (0, 40, 0), (40, 0, 0)])
    native[stump_root] = display(stump_root, '', texture, stump_child)
    native[stump_child] = display(stump_child, moved_a, texture)
    other = SCENE + '_room_00DL_0029F0'
    native[other] = display(other, moved_b, texture)
    room2 = module.read_scene(native[SCENE + '_room_01'])
    room2[10] = module.write_mesh({'data': 0, 'type': 2, 'entries': [
        {'type': 2, 'bounds': (2400, 50, 0, 180), 'opaque': SCENE + '_room_02DL_000B48', 'translucent': ''}]})
    native[SCENE + '_room_02'] = module.write_scene(room2)
    native[SCENE + '_room_02Vtx_fixture'] = vertex([(2400, 0, 0), (2400, 40, 0), (2440, 0, 0)])
    native[SCENE + '_room_02DL_000B48'] = display(
        SCENE + '_room_02DL_000B48', SCENE + '_room_02Vtx_fixture', texture)
    # FINAL's current door is the only final framing change in this fixture.
    final = dict(edit)
    scene = module.read_scene(final['alt/' + SCENE])
    transitions = bytearray(scene[14])
    struct.pack_into('<hhh', transitions, 4 + 16 + 6, -680, 0, 72)
    struct.pack_into('<h', transitions, 4 + 16 + 12, 13695)
    scene[14] = bytes(transitions)
    final['alt/' + SCENE] = module.write_scene(scene)
    final['custom/prelude/Z2_LOST_WOODS/do_not_bundle_poster'] = header(0x4F544558) + bytes(32)
    return native, edit, final


class GeneratorTests(unittest.TestCase):
    def test_stock_forest_empty_material_sentinel_needs_no_segment_bindings(self):
        module = load_module(self)
        # Captured native TexAnim_001BD0 payload: one zero-segment Empty row.
        animation = header(0x4F54414E) + bytes.fromhex('0100000000060000000000')
        self.assertEqual(module.scroll_bindings(animation), [])
        blob = module.material_blob([])
        self.assertEqual(blob[4:8], b'BLBO')
        self.assertEqual(blob[64:], bytes.fromhex('020000000100'))
        native, edit, final = fixture(module)
        native[SCENE + 'TexAnim_001BD0'] = animation
        resources, report = module.build_resources(native, edit, final)
        self.assertEqual(report['scroll_bindings'], [])
        self.assertEqual(resources[PREFIX + 'texanim'], animation)
        module.validate_resources(resources, report)
        malformed = animation[:68] + bytes.fromhex('01060000000000')
        with self.assertRaisesRegex(ValueError, 'material|binding'):
            module.scroll_bindings(malformed)

    def test_supplied_stock_donor_reproduces_native_geometry_without_images(self):
        module = load_module(self)
        native = Path(os.environ.get('NATIVE_FOREST_MM', str(ROOT.parent / 'upload/mm(1).o2r')))
        final = Path(os.environ.get('NATIVE_FOREST_FINAL', str(ROOT.parent / 'inputs/zzzz_MM3D_Lost_Woods_FINAL_avatar_POC19.prelude.o2r')))
        if not native.is_file() or not final.is_file():
            self.skipTest('Set NATIVE_FOREST_MM and NATIVE_FOREST_FINAL to verified native/geometry donors')
        resources, receipt = module.convert(native, EDIT, final)
        self.assertEqual(receipt['inputs']['native_mm']['sha256'], module.file_sha(native))
        self.assertEqual(receipt['scroll_bindings'], [])
        self.assertEqual(receipt['native_stock_rooms'], 3)
        self.assertEqual(receipt['custom_texture_payloads'], 0)
        module.validate_resources(resources, receipt)
        self.assertFalse(any(data[4:8] == b'XETO' for data in resources.values()))
        with zipfile.ZipFile(native) as archive:
            self.assertTrue(all(path in archive.namelist() for path in receipt['external_resources']))
            with zipfile.ZipFile(EDIT) as edits:
                for index, path in enumerate(module.MOVED):
                    before, after = archive.read(path), edits.read(path)
                    self.assertEqual(before[:72], after[:72])
                    for vertex in range((len(before)-72)//16):
                        offset = 72 + vertex * 16
                        moved = vertex < 175 if index == 0 else 8 <= vertex <= 20
                        x, y, z = struct.unpack_from('<3h', before, offset)
                        self.assertEqual(struct.unpack_from('<3h', after, offset),
                                         (x+143,y,z+296) if moved else (x,y,z))
                        self.assertEqual(before[offset+6:offset+16], after[offset+6:offset+16])
            for path, expected in zip(module.MOVED, (
                'a068e3555f7eb8b0dadd8a220e9f566cb323a14a215101082241e24705223f7d',
                'd1700f42b45f57cf02068b1ace08579ef7667e824a5833501611eb53feb22f8a')):
                self.assertEqual(module.sha(archive.read(path)), expected)

    def test_oot_private_rooms_retain_native_ambient_fairies_using_the_host_actor(self):
        module = load_module(self)
        # MM Demo_Kankyo params0 owns ambient forest sparkles. OoT's equivalent
        # is Object_Kankyo params0, whose draw owns its stock segment8 texture.
        row = struct.pack('<HhhhhhhH', 0x49, -48, 0, 7, 7, 127, 127, 0)
        native, edit, final = fixture(module)
        for room in (0, 1):
            path = SCENE + f'_room_0{room}'
            commands = module.read_scene(native[path])
            commands[1] = struct.pack('<I', 1) + row
            native[path] = module.write_scene(commands)
        resources, _ = module.build_resources(native, edit, final)
        for room in (0, 1):
            mm = module.read_actor_list(module.read_scene(resources[MM + f'_room_{room}'])[1])
            oot = module.read_actor_list(module.read_scene(resources[OOT + f'_room_{room}'], oot=True)[1])
            self.assertEqual(mm, [(0x49, -48, 0, 7, 7, 127, 127, 0)])
            self.assertEqual(oot, [(0x97, -48, 0, 7, 0, 0, 0, 0)])

    def test_stable_core_contract_covers_fixed_resources_without_inventing_native_graph_names(self):
        module = load_module(self)
        self.assertTrue(hasattr(module, 'native_core_paths'), 'Runtime needs a deterministic native core contract')
        resources, receipt = module.build_resources(*fixture(module))
        for mode, prefix in (('combined', COMBINED), ('mm', MM), ('oot', OOT)):
            paths = module.native_core_paths(mode)
            self.assertEqual(paths, sorted(set(paths)))
            self.assertTrue(set(paths) <= resources.keys() | set(receipt['external_resources']))
            for key in (prefix, prefix + '_collision', *(prefix + f'_room_{i}' for i in range(3)),
                        PREFIX + 'portal_root', PREFIX + 'portal_transform', PREFIX + 'portal_vtx',
                        PREFIX + 'portal_material', PREFIX + 'stock_materials', PREFIX + 'texanim'):
                self.assertIn(key, paths)
            self.assertNotIn(BARK, paths)
            self.assertFalse(any('_fixture_' in path for path in paths))
            for other in {COMBINED, MM, OOT} - {prefix}:
                self.assertFalse(any(path.startswith(other) for path in paths))
        with self.assertRaisesRegex(ValueError, 'mode'):
            module.native_core_paths('unknown')
        self.assertEqual(module.native_stock_paths(), [BARK])
        common, stock = module.native_core_header().split('kStock')
        self.assertIn('kCommon', common)
        for path in module.native_core_paths():
            self.assertIn('"' + path + '"', common)
        self.assertNotIn(BARK, common)
        self.assertIn('"' + BARK + '"', stock)
        for name, value in (('kCombinedScene', COMBINED), ('kMmSoloScene', MM), ('kOotSoloScene', OOT)):
            self.assertIn(f'{name} = "{value}"', stock)

    def test_optional_material_override_preserves_every_geometry_and_collision_resource(self):
        module = load_module(self)
        native, edit, final = fixture(module)
        resources, receipt = module.build_resources(native, edit, final)
        self.assertIn('building_materials', receipt, 'Native building materials need independent override identities')
        mapping = receipt['building_materials']
        expected = {'custom/prelude/Z2_LOST_WOODS/' + recipe: PREFIX + 'building/' + recipe + '_material'
                    for recipe in module.RECIPES}
        self.assertEqual(mapping, expected)
        original = dict(resources)
        optional = {}
        for source, material in mapping.items():
            mesh_path = PREFIX + 'building/' + source.rsplit('/', 1)[1]
            mesh = ET.fromstring(resources[mesh_path])
            self.assertEqual(mesh[0].tag, 'CallDisplayList')
            self.assertEqual(mesh[0].get('Path'), material)
            default = ET.fromstring(resources[material])
            self.assertIsNone(default.find('SetTextureImage'))
            self.assertIsNone(default.find('LoadVertices'))
            self.assertIsNone(default.find('Triangle1'))
            # Model a later archive replacing just a material. Its texture
            # comes from the ordinary native owner, not a missing custom pack.
            tree = ET.Element('DisplayList', Version='0')
            ET.SubElement(tree, 'SetTextureImage', Path=SCENE + 'Tex_00B9D0', Format='G_IM_FMT_IA', Size='G_IM_SIZ_16b', Width='64')
            ET.SubElement(tree, 'SetPrimColor', M='0', L='0', R='96', G='128', B='160', A='255')
            ET.SubElement(tree, 'EndDisplayList')
            optional[material] = ET.tostring(tree)
        effective = dict(resources, **optional)
        self.assertEqual({key for key in resources if effective[key] != resources[key]}, set(mapping.values()))
        for key, data in resources.items():
            if key not in mapping.values():
                self.assertEqual(effective[key], data, 'Material-only override changed native geometry or framing')
        for data in resources.values():
            if data.startswith(b'<'):
                for node in ET.fromstring(data).iter():
                    path = node.get('Path')
                    if path:
                        self.assertTrue(path in resources or path in receipt['external_resources'], path)
        self.assertEqual(resources, original)

    def test_current_final_building_geometry_accepts_authored_material_scroll_calls(self):
        module = load_module(self)
        path = Path(os.environ.get('NATIVE_FOREST_FINAL', str(ROOT.parent / 'inputs/zzzz_MM3D_Lost_Woods_FINAL_avatar_POC19.prelude.o2r')))
        if not path.is_file():
            self.skipTest('Set NATIVE_FOREST_FINAL to the retained real FINAL geometry donor')
        self.assertEqual(module.file_sha(path), module.FINAL_SHA)
        with zipfile.ZipFile(path) as archive:
            for recipe in module.RECIPES:
                source = 'custom/prelude/Z2_LOST_WOODS/' + recipe
                data, triangles = module.building_geometry(archive.read(source), archive.read(source + '_vtx'),
                                                           source, PREFIX + 'building/' + recipe + '_vtx')
                tree = ET.fromstring(data)
                self.assertIsNone(tree.find('SetTextureImage'))
                self.assertGreater(triangles, 0)

    def test_stock_material_blob_matches_lus_framing_and_effective_native_y(self):
        module = load_module(self)
        native, edit, final = fixture(module)
        resources, report = module.build_resources(native, edit, final)
        self.assertTrue(PREFIX + 'stock_materials' in resources, 'Stock runtime parameters must use a native Blob resource')
        data = resources[PREFIX + 'stock_materials']
        payload = b'\x01\x01\x08\x02\x00\x00\x20\x20\x00\xff\x20\x40'
        self.assertEqual(data[:12], bytes(4) + b'BLBO' + bytes(4))
        self.assertEqual(struct.unpack_from('<I', data, 64)[0], len(payload))
        self.assertEqual(data[68:], payload)
        self.assertEqual(len(data), 68 + len(payload), 'Factory supplies its own 16-byte zero padding')
        self.assertEqual(data, module.material_blob(json.loads(json.dumps(report['scroll_bindings']))))
        self.assertEqual(data, module.material_blob(report['scroll_bindings']))

    def test_stock_material_blob_rejects_unrepresentable_or_ambiguous_bindings(self):
        module = load_module(self)
        self.assertTrue(hasattr(module, 'material_blob'), 'Strict stock material serialization is missing')
        one = {'segment': 8, 'layers': [{'x': -1, 'y': 1, 'width': 32, 'height': 64}]}
        variants = [[one, one], [{'segment': 7, 'layers': one['layers']}],
                    [{'segment': 16, 'layers': one['layers']}], [{'segment': 8, 'layers': []}],
                    [{'segment': 8, 'layers': one['layers'] * 3}]]
        for field, value in (('x', -129), ('x', 128), ('y', 128), ('y', -129),
                             ('width', 0), ('width', 256), ('height', 0), ('height', 256)):
            layer = dict(one['layers'][0], **{field: value})
            variants.append([{'segment': 8, 'layers': [layer]}])
        for bindings in variants:
            with self.subTest(bindings=bindings), self.assertRaisesRegex(ValueError, 'material|binding|layer'):
                module.material_blob(bindings)

    def test_real_native_room_type2_round_trip_is_lossless(self):
        module = load_module(self)
        edit = real_edit()
        for room, count in ((0, 8), (1, 7)):
            data = edit['alt/' + SCENE + f'_room_0{room}']
            commands = module.read_scene(data)
            mesh = module.read_mesh(commands[10])
            self.assertEqual((mesh['type'], len(mesh['entries'])), (2, count))
            self.assertEqual(module.write_mesh(mesh), commands[10])
            self.assertEqual(module.write_scene(commands), data)

    def test_private_stock_graph_rebinds_moved_vertices_without_any_texture_payload(self):
        module = load_module(self)
        native, edit, final = fixture(module)
        snapshots = [dict(value) for value in (native, edit, final)]
        resources, report = module.build_resources(native, edit, final)
        self.assertEqual([native, edit, final], snapshots)
        self.assertTrue(resources)
        self.assertTrue(all(data[4:8] != b'XETO' for data in resources.values()))
        self.assertFalse(any('cor_mm3d' in name or name.startswith('alt/') for name in resources))
        self.assertFalse(any('poster' in name or 'mask' in name or '_tex0' in name for name in resources))
        self.assertEqual(report['external_textures'], sorted([SCENE + 'Tex_00B9D0', BARK]))
        for suffix in ('0019D8', '002820'):
            target = PREFIX + 'geometry/Z2_LOST_WOODS_room_00Vtx_' + suffix
            self.assertEqual(resources[target], edit[SCENE + '_room_00Vtx_' + suffix])
        child = PREFIX + 'geometry/Z2_LOST_WOODS_room_00DL_fixture_child'
        refs = module.display_dependencies(resources[child], child)
        self.assertIn(crc64(PREFIX + 'geometry/Z2_LOST_WOODS_room_00Vtx_0019D8'), refs.values())
        # A private rebind must not require overwriting a shared native ROM key.
        self.assertNotIn(SCENE + '_room_00DL_002548', resources)
        self.assertEqual(report['native_stock_rooms'], 3)

    def test_scene_framing_preserves_three_rooms_stock_animation_and_native_angles(self):
        module = load_module(self)
        native, edit, final = fixture(module)
        resources, report = module.build_resources(native, edit, final)
        self.assertEqual(resources[PREFIX + 'texanim'], native[SCENE + 'TexAnim_001BD0'])
        for prefix, oot in ((COMBINED, False), (MM, False), (OOT, True)):
            commands = module.read_scene(resources[prefix], oot=oot)
            rooms = module.read_room_list(commands[4])
            self.assertEqual([name for name, _, _ in rooms], [prefix + f'_room_{i}' for i in range(3)])
            self.assertTrue(all(start == end == 0 for _, start, end in rooms))
            self.assertNotIn(24, commands)
            self.assertNotIn(27, commands)
            self.assertNotIn(28, commands)
            self.assertEqual(module.read_string(commands[3]), prefix + '_collision')
            transition = struct.unpack_from('<4bHhhhhH', commands[14], 4 + 16)
            self.assertEqual(transition[4:8], (9 if oot else 5, -680, 0, 72))
            self.assertEqual(transition[8], 19296 if oot else 13695)
            if oot:
                self.assertNotIn(26, commands)
                self.assertEqual(len(commands[8]) if 8 in commands else 0, 0)
            else:
                self.assertEqual(module.read_string(commands[26]), PREFIX + 'texanim')
            for room in range(3):
                body = module.read_scene(resources[prefix + f'_room_{room}'], oot=oot)
                mesh = module.read_mesh(body[10])
                self.assertEqual(mesh['type'], 2)
                rows = module.read_actor_list(body[1])
                self.assertTrue(all(row[0] not in (0xD, 0x191, 0x192) for row in rows))
                if oot:
                    self.assertTrue(all(row[0] in (0x151, 0x97) for row in rows))
                    self.assertEqual(body[8], struct.pack('<bi', 0, 0))
        self.assertEqual(report['scroll_bindings'], [{'segment': 8, 'layers': [
            {'x': 0, 'y': 0, 'width': 32, 'height': 32}, {'x': 0, 'y': -1, 'width': 32, 'height': 64}]}])

    def test_moved_stump_bounds_enclose_real_geometry_and_building_uses_no_image(self):
        module = load_module(self)
        native, edit, final = fixture(module)
        resources, report = module.build_resources(native, edit, final)
        body = module.read_scene(resources[MM + '_room_0'])
        mesh = module.read_mesh(body[10])
        self.assertEqual(len(mesh['entries']), 10)
        stump = mesh['entries'][1]
        self.assertNotEqual(stump['bounds'], (-520, 40, 120, 112))
        points = module.vertex_positions(edit[SCENE + '_room_00Vtx_0019D8'])
        x, y, z, radius = stump['bounds']
        self.assertTrue(all(sum((point[i] - (x, y, z)[i]) ** 2 for i in range(3)) <= radius ** 2
                            for point in points))
        building = ET.fromstring(resources[PREFIX + 'building_root'])
        self.assertEqual(len(building.findall('CallDisplayList')), 3)
        for call in building.findall('CallDisplayList'):
            material = ET.fromstring(resources[call.get('Path')])
            self.assertIsNone(material.find('SetTextureImage'))
            self.assertGreater(len(material.findall('LoadVertices')), 0)
            self.assertGreater(len(material.findall('Triangle1')) + len(material.findall('Triangles2')), 0)
        self.assertEqual(report['building_material'], 'untextured native vertex shading')

    def test_deterministic_bundle_checks_scope_and_keeps_inputs_immutable(self):
        module = load_module(self)
        native, edit, final = fixture(module)
        resources, report = module.build_resources(native, edit, final)
        with tempfile.TemporaryDirectory(prefix='native-forest-bundle-') as folder:
            first, second = [Path(folder) / name for name in ('one.zip', 'two.zip')]
            manifest = module.write_bundle(resources, report, first)
            module.write_bundle(resources, report, second)
            self.assertEqual(first.read_bytes(), second.read_bytes())
            self.assertEqual(manifest['bundle']['sha256'], hashlib.sha256(first.read_bytes()).hexdigest())
            with zipfile.ZipFile(first) as archive:
                self.assertEqual(archive.namelist(), sorted(resources))
                self.assertIsNone(archive.testzip())
                for entry in archive.infolist():
                    self.assertEqual(entry.date_time, (1980, 1, 1, 0, 0, 0))
                    self.assertEqual(archive.read(entry), resources[entry.filename])
            with self.assertRaises(FileExistsError):
                module.write_bundle(resources, report, first)
            poisoned = dict(resources)
            poisoned[PREFIX + 'image'] = header(0x4F544558)
            with self.assertRaisesRegex(ValueError, 'texture'):
                module.write_bundle(poisoned, report, Path(folder) / 'invalid.zip')

    def test_building_fallback_retains_every_authored_triangle_and_vertex_byte(self):
        module = load_module(self)
        native, edit, final = fixture(module)
        resources, _ = module.build_resources(native, edit, final)
        # Decode input packets independently of the production material adapter.
        for recipe in ('recipe7624751371212635', 'recipe3635031522573470', 'recipe215851318676066'):
            original = 'custom/prelude/Z2_LOST_WOODS/' + recipe
            raw = edit[original + '_vtx']
            self.assertEqual(resources[PREFIX + 'building/' + recipe + '_vtx'], raw)
            positions = [struct.unpack_from('<3h', raw, 72 + index * 16)
                         for index in range(struct.unpack_from('<I', raw, 68)[0])]
            source, source_faces, cache, offset = edit[original], [], {}, 72
            while offset < len(source):
                w0, w1 = struct.unpack_from('<II', source, offset)
                opcode = w0 >> 24
                if opcode == 0x32:
                    count, end = (w0 >> 12) & 255, (w0 & 255) // 2
                    for index, point in enumerate(positions[w1 // 16:w1 // 16 + count]):
                        cache[end - count + index] = point
                elif opcode in (0x05, 0x06):
                    for word in ((w0, w1) if opcode == 0x06 else (w0,)):
                        source_faces.append(tuple(cache[((word >> shift) & 255) // 2] for shift in (16, 8, 0)))
                elif opcode == 0x26:
                    source_faces.append(tuple(cache[index] for index in (w0 & 65535, w1 >> 16, w1 & 65535)))
                offset += 16 if opcode in (0x20, 0x31, 0x32, 0x33, 0x35, 0x36, 0x42) else 8
            actual_faces, cache = [], {}
            for node in ET.fromstring(resources[PREFIX + 'building/' + recipe]):
                if node.tag == 'LoadVertices':
                    start, source_offset, count = [int(node.get(key)) for key in ('VertexBufferIndex', 'VertexOffset', 'Count')]
                    for index, point in enumerate(positions[source_offset:source_offset + count]):
                        cache[start + index] = point
                elif node.tag == 'Triangle1':
                    actual_faces.append(tuple(cache[int(node.get(key))] for key in ('V00', 'V01', 'V02')))
            self.assertEqual(actual_faces, source_faces, 'Fallback changed authored building geometry')

    def test_extensionless_images_and_file_directory_collisions_are_rejected(self):
        module = load_module(self)
        native, edit, final = fixture(module)
        resources, report = module.build_resources(native, edit, final)
        with tempfile.TemporaryDirectory(prefix='native-forest-scope-') as folder:
            resources[PREFIX + 'hidden_image'] = b'\x89PNG\r\n\x1a\n' + bytes(40)
            with self.assertRaisesRegex(ValueError, 'texture|image|encoding'):
                module.write_bundle(resources, report, Path(folder) / 'image.zip')
            del resources[PREFIX + 'hidden_image']
            resources[PREFIX + 'building'] = b'conflicts with building/ mesh files'
            with self.assertRaisesRegex(ValueError, 'directory|collision|encoding'):
                module.write_bundle(resources, report, Path(folder) / 'collision.zip')

    def test_unresolved_native_graph_fails_closed_and_native_bytes_are_preserved(self):
        module = load_module(self)
        native, edit, final = fixture(module)
        resources, _ = module.build_resources(native, edit, final)
        child = 'Z2_LOST_WOODS_room_00DL_fixture_child'
        before, after = native['scenes/nonmq/Z2_LOST_WOODS/' + child], resources[PREFIX + 'geometry/' + child]
        changed = [offset for offset, (one, two) in enumerate(zip(before, after)) if one != two]
        allowed = {offset + byte for offset in (72, 104) for byte in range(8, 16)}
        self.assertTrue(changed)
        self.assertTrue(set(changed) <= allowed, 'Native command changed outside resource hash operands')
        del native[SCENE + 'Tex_00B9D0']
        with self.assertRaisesRegex(ValueError, 'unresolved native resource hash'):
            module.build_resources(native, edit, final)

    def test_native_archive_index_does_not_decode_unrelated_rom_payloads(self):
        module = load_module(self)
        self.assertTrue(hasattr(module, 'ArchiveResources'), 'Native ROM index must read only requested dependencies')
        with tempfile.TemporaryDirectory(prefix='native-forest-rom-index-') as folder:
            path = Path(folder) / 'controlled-rom.zip'
            with zipfile.ZipFile(path, 'w') as archive:
                archive.writestr('wanted', b'valid resource')
                archive.writestr('unused', b'unrelated texture pixels')
            with zipfile.ZipFile(path) as archive:
                entry = archive.getinfo('unused')
                offset = entry.header_offset + 30 + len(entry.filename.encode()) + len(entry.extra)
            data = bytearray(path.read_bytes())
            data[offset] ^= 1
            path.write_bytes(data)
            with zipfile.ZipFile(path) as archive:
                resources = module.ArchiveResources(archive)
                self.assertEqual(set(resources), {'wanted', 'unused'})
                self.assertEqual(resources['wanted'], b'valid resource')
                with self.assertRaises(zipfile.BadZipFile):
                    resources['unused']

    def test_third_stock_rooms_original_mesh_kind_is_preserved(self):
        module = load_module(self)
        native, edit, final = fixture(module)
        path = SCENE + '_room_02'
        commands = module.read_scene(native[path])
        commands[10] = module.write_mesh({'data': 0, 'type': 0, 'entries': [
            {'type': 0, 'opaque': SCENE + '_room_02DL_000B48', 'translucent': ''}]})
        native[path] = module.write_scene(commands)
        resources, _ = module.build_resources(native, edit, final)
        for prefix in (COMBINED, MM, OOT):
            room = module.read_scene(resources[prefix + '_room_2'], oot=prefix == OOT)
            self.assertEqual(module.read_mesh(room[10])['type'], 0)

    def test_cli_rejects_wrong_native_donor_before_writing_output(self):
        module = load_module(self)
        with tempfile.TemporaryDirectory(prefix='native-forest-identity-') as folder:
            donor = Path(folder) / 'not-native.o2r'
            donor.write_bytes(b'unrelated archive')
            with self.assertRaisesRegex(ValueError, 'native MM.*SHA'):
                module.convert(donor, EDIT, donor)


if __name__ == '__main__':
    unittest.main()
