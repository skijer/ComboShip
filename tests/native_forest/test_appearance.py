#!/usr/bin/env python3
"""Optional terrain must preserve the native v2 presentation and readiness."""
import importlib.util
import io
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET
import zipfile

from test_generator import ROOT

MODULE = ROOT / 'tools/black_market/forest_appearance.py'
DONOR = Path(os.environ.get('NATIVE_FOREST_AUTHORED', str(
    ROOT.parent / 'inputs/zzzz_Skiejer_Native_Solo_Forest_FINAL_avatar_POC4.o2r')))
NATIVE = ROOT / 'tools/native_forest/resources.zip'
CORE = 'custom/comboship/native_forest/'
SOLO = 'custom/skiejer_forest/'
COMBINED = 'custom/cor_mm3d/lost_woods_poc1/'
SEASONAL = ('lw_enkei_01_0', 'lw_kabe_01_5400', 'lw_kusa_01_12600',
            'lw_wood_02_2CA00', 'lw_wood_03_31E00', 'lw_wood_04_37200',
            'lw_yuka_02_39C00', 'lw_yuka_03_44400')
NATIVE_OPA = {CORE + name for name in ('building_root', 'portal_root', 'mirror_access_root')}


class AppearanceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not MODULE.is_file() or not DONOR.is_file():
            return
        sys.path.insert(0, str(MODULE.parent))
        spec = importlib.util.spec_from_file_location('forest_appearance', MODULE)
        cls.module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(cls.module)
        cls.native_input = ROOT / 'tools/native_forest' if (MODULE.parent / 'native_forest_bundle.py').is_file() else NATIVE
        cls.resources, cls.report = cls.module.build(DONOR, cls.native_input)
        with zipfile.ZipFile(io.BytesIO(cls.module.native_bytes(cls.native_input))) as archive:
            cls.native = {path: archive.read(path) for path in archive.namelist()}

    def build(self):
        self.assertTrue(MODULE.is_file(), 'Appearance-only recipe is missing; old retarget overrides native additions')
        if not DONOR.is_file():
            self.skipTest('Set NATIVE_FOREST_AUTHORED to the immutable solo donor')
        return self.module, self.resources, self.report, self.native

    def test_only_six_room_mesh_overrides_preserve_all_native_addition_entries(self):
        module, resources, report, native = self.build()
        nf = module.nf
        expected = {'alt/' + prefix + f'_room_{room}' for prefix in nf.SCENES.values() for room in (0, 1)}
        self.assertEqual({name for name in resources if name.startswith('alt/')}, expected)
        self.assertFalse(any(name.removeprefix('alt/').startswith(CORE) for name in resources))
        self.assertFalse(any('/prelude/' in name or '/Return' in name for name in resources))
        self.assertFalse(any(name.endswith(('texanim', 'stock_materials')) for name in resources))
        for mode, prefix in nf.SCENES.items():
            for room in (0, 1):
                path = prefix + f'_room_{room}'
                old = nf.read_scene(native[path], oot=mode == 'oot')
                new = nf.read_scene(resources['alt/' + path], oot=mode == 'oot')
                self.assertEqual({k: v for k, v in old.items() if k != 10},
                                 {k: v for k, v in new.items() if k != 10})
                old_mesh, new_mesh = nf.read_mesh(old[10]), nf.read_mesh(new[10])
                self.assertEqual(new_mesh['type'], 2)
                retained = lambda mesh: [entry for entry in mesh['entries'] if entry['opaque'] in NATIVE_OPA or
                                        entry['translucent'] == CORE + 'stump_shimmer_root']
                self.assertEqual(retained(new_mesh), retained(old_mesh))
                self.assertEqual(len(new_mesh['entries']), 5 if room == 0 else 1)
        self.assertEqual(report['requires_native_sha256'],
                         'b0a33016b32d7e5415090038cff4cb8f4328cc8fd8a4b3f09d55918318be9fb5')

    def test_exact_terrain_pixels_vertices_and_seasonal_aliases_are_reachable(self):
        module, resources, report, native = self.build()
        with zipfile.ZipFile(DONOR) as source:
            immutable = []
            for name, data in resources.items():
                if data[4:8] == b'XETO' or name.endswith('_vtx'):
                    origin = name.replace(COMBINED, SOLO, 1)
                    self.assertEqual(data, source.read(origin), name)
                    immutable.append(name)
            self.assertTrue(immutable)
            self.assertNotIn(SOLO + 'textures/portal_bark', resources)
        effective = dict(native)
        effective.update({path.removeprefix('alt/'): data for path, data in resources.items()})
        nf = module.nf
        def walk(path, reached):
            if not path or path in reached:
                return
            reached.add(path)
            data = effective[path]
            if data.startswith(b'<DisplayList'):
                for node in ET.fromstring(data):
                    if node.get('Path'):
                        walk(node.get('Path'), reached)
        for mode, prefix in nf.SCENES.items():
            reached = set()
            for room in (0, 1):
                mesh = nf.read_mesh(nf.read_scene(effective[prefix + f'_room_{room}'], oot=mode == 'oot')[10])
                walk(mesh['entries'][0]['opaque'], reached)
            owner = COMBINED if mode == 'combined' else SOLO
            for texture in SEASONAL:
                self.assertIn(owner + 'textures/' + texture, reached)
        module.validate(resources, native)

    def test_forbidden_overrides_and_native_copies_are_rejected(self):
        module, resources, report, native = self.build()
        changes = ('alt/' + CORE + 'portal_root', 'alt/' + CORE + 'stock_materials',
                   CORE + 'authored/room_0/ReturnTunnelMouth',
                   SOLO + 'prelude/black_market_skeijer_poster',
                   'alt/' + module.nf.COMBINED + '_collision')
        for path in changes:
            with self.subTest(path=path), self.assertRaisesRegex(ValueError, 'scope|override|native|terrain'):
                module.validate({**resources, path: b'forbidden'}, native)
        path = 'alt/' + module.nf.MM + '_room_0'
        commands = module.nf.read_scene(resources[path])
        commands[16] = bytes(3)
        with self.assertRaisesRegex(ValueError, 'gameplay|command'):
            module.validate({**resources, path: module.nf.write_scene(commands)}, native)

    def test_native_missing_dependency_and_optional_missing_texture_fail_closed(self):
        module, resources, report, native = self.build()
        for missing in (CORE + 'portal_root', CORE + 'mirror_access_vtx',
                        CORE + 'authored/prelude/black_market_skeijer_poster',
                        CORE + 'stock_materials'):
            with self.subTest(missing=missing), self.assertRaisesRegex(ValueError, '(?i)native|dependency'):
                module.validate(resources, {path: data for path, data in native.items() if path != missing})
        missing = SOLO + 'textures/lw_kabe_01_5400'
        with self.assertRaisesRegex(ValueError, 'missing|closure|dependency'):
            module.validate({path: data for path, data in resources.items() if path != missing}, native)

    def test_real_cpp_readiness_still_requires_native_v2_assets(self):
        module, resources, report, native = self.build()
        fixture = r'''
#include "combo/entrances/NativeForest.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <vector>
int main(int argc,char** argv){
 assert(argc==5);std::set<std::string> files,stock;std::string line;
 std::ifstream a(argv[1]),b(argv[2]);
 while(std::getline(a,line))files.insert(line);
 while(std::getline(b,line))stock.insert(line);
 std::ifstream f(argv[3],std::ios::binary);
 std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)),{});
 auto load=[&](const char*){return NativeForestDependencies::Parse(bytes.data(),bytes.size());};
 auto exists=[&](const char* path){return files.count(path);};
 auto base=[&](const char* path){return stock.count(path);};
 assert(NativeForest::SceneAvailable(argv[4],exists,base,load));
 files.erase("custom/comboship/native_forest/authored/prelude/black_market_skeijer_poster");
 assert(!NativeForest::SceneAvailable(argv[4],exists,base,load));
 files.insert("custom/comboship/native_forest/authored/prelude/black_market_skeijer_poster");
 files.erase("custom/comboship/native_forest/mirror_access_vtx");
 assert(!NativeForest::SceneAvailable(argv[4],exists,base,load));
 files.clear();assert(!NativeForest::SceneAvailable(argv[4],exists,base,load));
}
'''
        with tempfile.TemporaryDirectory(prefix='appearance-gate-') as folder:
            root = Path(folder)
            cpp, binary = root / 'gate.cpp', root / 'gate'
            cpp.write_text(fixture)
            subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra', '-Werror',
                            '-I' + str(ROOT), str(cpp), '-o', str(binary)], check=True)
            for mode, prefix in module.nf.SCENES.items():
                descriptor = module.nf.dependency_path(mode)
                core, stock = module.nf.read_dependency_blob(native[descriptor])
                available = set(native) | {path.removeprefix('alt/') for path in resources}
                (root / 'files').write_text('\n'.join(sorted(available)) + '\n')
                (root / 'stock').write_text('\n'.join([*stock, 'scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS_room_00']) + '\n')
                (root / 'payload').write_bytes(native[descriptor][68:] + bytes(16))
                subprocess.run([str(binary), str(root / 'files'), str(root / 'stock'), str(root / 'payload'), prefix], check=True)

    def test_deterministic_archive_and_input_identity_gates(self):
        module, resources, report, native = self.build()
        with tempfile.TemporaryDirectory(prefix='appearance-deterministic-') as folder:
            root = Path(folder)
            one, two = root / 'one.o2r', root / 'two.o2r'
            module.write(resources, report, one)
            module.write(resources, report, two)
            self.assertEqual(module.nf.file_sha(one), module.nf.file_sha(two))
            with zipfile.ZipFile(one) as pack:
                self.assertIsNone(pack.testzip())
                self.assertEqual(set(pack.namelist()), set(resources))
            bad = root / 'bad.zip'
            bad.write_bytes(b'changed input')
            for donor, baseline in ((bad, self.native_input), (DONOR, bad)):
                with self.assertRaisesRegex(ValueError, 'identity|immutable|SHA256'):
                    module.build(donor, baseline)


if __name__ == '__main__':
    unittest.main()
