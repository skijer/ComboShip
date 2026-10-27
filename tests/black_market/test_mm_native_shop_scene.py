#!/usr/bin/env python3
"""Verify MM's native shop scene records before native actor initialization."""
import importlib.util
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/black_market'))
sys.path.insert(0, str(ROOT / 'scripts/assets'))
import forest
import prepare_native_market

SCENE = 'scenes/nonmq/skiejer_market/skiejer_market'
ROOM = SCENE + '_room_0'

FACTORY_PROOF = r'''
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include "combo/BlackMarketMmShopScene.h"
#include "2s2h/resource/importer/scenecommand/SetCsCameraFactory.h"
#include "2s2h/resource/importer/scenecommand/SetActorCutsceneListFactory.h"
#include "2s2h/resource/type/scenecommand/SetCsCamera.h"
#include "2s2h/resource/type/scenecommand/SetActorCutsceneList.h"

void Require(bool condition, const char* reason) {
    if (!condition) throw std::runtime_error(reason);
}

std::shared_ptr<Ship::IResource> Load(const char* path, SOH::SceneCommandFactoryBinaryV0& factory) {
    std::ifstream file(path, std::ios::binary);
    std::vector<char> bytes((std::istreambuf_iterator<char>(file)), {});
    auto reader = std::make_shared<Ship::BinaryReader>(bytes.data(), bytes.size());
    reader->SetEndianness(Ship::Endianness::Little);
    auto resource = factory.ReadResource(std::make_shared<Ship::ResourceInitData>(), reader);
    Require(reader->GetBaseAddress() == bytes.size(), "actual native factory did not consume EOF");
    return resource;
}

int main(int argc, char** argv) {
    try {
        Require(argc == 3, "expected native camera and cutscene command payloads");
        SOH::SetCsCameraFactory cameraFactory;
        auto cameras = std::dynamic_pointer_cast<SOH::SetCsCamera>(Load(argv[1], cameraFactory));
        Require(cameras && cameras->cmdId == SOH::SceneCommandID::SetCsCamera, "native camera command ID");
        Require(cameras->csCamera.size() == 4, "native camera record count");
        for (size_t i = 0; i < 4; ++i) {
            auto& camera = cameras->csCamera[i];
            Require(camera.setting == BlackMarketMmShopScene::kCameraSetting && camera.count == 3,
                    "native camera setting/vector count");
            for (size_t j = 0; j < 3; ++j) {
                const auto point = camera.actorCsCamFuncData[j];
                int16_t observed[] = { point.x, point.y, point.z };
                for (size_t axis = 0; axis < 3; ++axis)
                    Require(observed[axis] == BlackMarketMmShopScene::kCameraRecords[i][j * 3 + axis],
                            "actual native camera factory decoded a different authored pose");
            }
        }
        SOH::SetActorCutsceneListFactory cutsceneFactory;
        auto cutscenes = std::dynamic_pointer_cast<SOH::SetActorCutsceneList>(Load(argv[2], cutsceneFactory));
        Require(cutscenes && cutscenes->cmdId == SOH::SceneCommandID::SetActorCutsceneList,
                "native cutscene command ID");
        Require(cutscenes->entries.size() == 4, "native cutscene count");
        for (int i = 0; i < 4; ++i) {
            const auto& entry = cutscenes->entries[i];
            Require(entry.priority == 10 + i && entry.length == -1 && entry.scriptIndex == -1,
                    "native cutscene priority/length/script");
            Require(entry.csCamId == BlackMarketMmShopScene::kCutsceneCameras[i], "native cutscene camera");
            Require(entry.additionalCsId == (i < 3 ? i + 1 : -1), "native cutscene chain");
            Require(entry.endSfx == 0 && entry.customValue == 255 && entry.hudVisibility == 4 &&
                    entry.endCam == 0 && entry.letterboxSize == 32, "native donor cutscene metadata");
        }
        std::cout << "PASS actual MM serialized camera/cutscene factories\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
'''


def shop_module(case):
    path = ROOT / 'tools/black_market/mm_native_shop.py'
    case.assertTrue(path.is_file(), 'MM preparation has no native shop scene adapter')
    spec = importlib.util.spec_from_file_location('mm_native_shop_scene_test', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class MmNativeShopScene(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.original = prepare_native_market.prepared_resources()

    def test_scene_contains_private_oot_data4_cameras_and_linked_cutscenes(self):
        module = shop_module(self)
        result = module.apply(self.original)
        commands = module.native_commands(result[SCENE])
        self.assertLess(list(commands).index(2), list(commands).index(27))
        self.assertLess(list(commands).index(27), list(commands).index(20))
        self.assertEqual(struct.unpack_from('<I', commands[2])[0], 4)
        offset = 4
        cameras = []
        for _ in range(4):
            setting, count = struct.unpack_from('<HH', commands[2], offset)
            self.assertEqual((setting, count), (0x5B, 3))
            cameras.append(struct.unpack_from('<9h', commands[2], offset + 4))
            offset += 4 + 6 * count
        self.assertEqual(offset, len(commands[2]))
        self.assertEqual(cameras, [tuple(row) for row in module.contract()['CameraRecords']])
        self.assertEqual([row[6:] for row in cameras], [(-1, 3, -1)] * 4)
        self.assertEqual(struct.unpack_from('<I', commands[27])[0], 4)
        self.assertEqual(len(commands[27]), 4 + 4 * 16)
        self.assertEqual([struct.unpack_from('<5h2Bh2B', commands[27], 4 + i * 16) for i in range(4)], [
            (10, -1, 0, -1, 1, 0, 255, 4, 0, 32),
            (11, -1, 2, -1, 2, 0, 255, 4, 0, 32),
            (12, -1, 3, -1, 3, 0, 255, 4, 0, 32),
            (13, -1, 1, -1, -1, 0, 255, 4, 0, 32),
        ])

    def test_only_mm_scene_and_keeper_actor_bytes_change(self):
        module = shop_module(self)
        original = dict(self.original)
        result = module.apply(original)
        self.assertEqual(original, self.original, 'preparation mutated its input resources')
        self.assertEqual(set(result), set(original))
        self.assertEqual({name for name in result if result[name] != original[name]}, {SCENE, ROOM})
        before = forest.native_commands(original[SCENE])
        after = module.native_commands(result[SCENE])
        self.assertEqual({key: value for key, value in before.items() if key != 27},
                         {key: value for key, value in after.items() if key not in (2, 27)})
        self.assertEqual(result[SCENE][:64], original[SCENE][:64])
        before, after = forest.native_commands(original[ROOM]), module.native_commands(result[ROOM])
        self.assertEqual({key: value for key, value in before.items() if key != 1},
                         {key: value for key, value in after.items() if key != 1})
        old_actor = struct.unpack_from('<H6hH', before[1], 4)
        new_actor = struct.unpack_from('<H6hH', after[1], 4)
        self.assertEqual(old_actor, (0x1C1, 0, 1, 86, 0, 0, 0, 0))
        self.assertEqual(new_actor, (0x2A, *old_actor[1:7], 1))
        self.assertEqual(new_actor[5] & 0x7F, 0, 'native actor must start at cutscene 0')
        self.assertEqual(result, module.apply(result), 'native MM preparation must be idempotent')

    def test_host_specific_preparation_is_deterministic_and_oot_is_byte_exact(self):
        shop_module(self)
        self.assertEqual(prepare_native_market.prepared_resources(host='oot'), self.original)
        expected_mm = prepare_native_market.prepared_resources(host='mm')
        self.assertNotEqual(expected_mm[SCENE], self.original[SCENE])
        with tempfile.TemporaryDirectory(prefix='native-mm-shop-') as directory:
            output = Path(directory)
            prepare_native_market.prepare(output)
            for game, resources in [('soh', self.original), ('mm', expected_mm)]:
                for name, data in resources.items():
                    self.assertEqual((output / game / 'assets/custom' / name).read_bytes(), data, (game, name))
            self.assertEqual(prepare_native_market.prepare(output), 0)
        with self.assertRaisesRegex(ValueError, 'host'):
            prepare_native_market.prepared_resources(host='unsupported')

    def test_unexpected_native_records_and_missing_keeper_are_rejected(self):
        module = shop_module(self)
        prepared = module.apply(self.original)
        malformed = dict(prepared)
        cameras = bytearray(prepared[SCENE])
        commands = module.native_commands(prepared[SCENE])
        start = cameras.index(struct.pack('<I', 2) + commands[2])
        struct.pack_into('<H', cameras, start + 8, 1)
        malformed[SCENE] = bytes(cameras)
        with self.assertRaisesRegex(ValueError, 'camera|cutscene'):
            module.apply(malformed)
        malformed = dict(self.original)
        room = bytearray(malformed[ROOM])
        actor = forest.native_commands(room)[1]
        start = room.index(struct.pack('<I', 1) + actor)
        struct.pack_into('<H', room, start + 8, 0x12)
        malformed[ROOM] = bytes(room)
        with self.assertRaisesRegex(ValueError, 'keeper'):
            module.apply(malformed)
        with self.assertRaises((ValueError, struct.error)):
            module.native_commands(prepared[SCENE][:-1])

    def test_actual_native_factories_decode_the_prepared_scene_commands(self):
        module = shop_module(self)
        commands = module.native_commands(module.apply(self.original)[SCENE])
        sources = [
            'mm/2s2h/resource/importer/scenecommand/SceneCommandFactory.cpp',
            'mm/2s2h/resource/importer/scenecommand/SetCsCameraFactory.cpp',
            'mm/2s2h/resource/importer/scenecommand/SetActorCutsceneListFactory.cpp',
            'mm/2s2h/resource/type/scenecommand/SetCsCamera.cpp',
            'mm/2s2h/resource/type/scenecommand/SetActorCutsceneList.cpp',
            'libultraship/src/ship/utils/binarytools/BinaryReader.cpp',
            'libultraship/src/ship/utils/binarytools/MemoryStream.cpp',
            'libultraship/src/ship/utils/binarytools/Stream.cpp',
            'libultraship/src/ship/resource/Resource.cpp',
        ]
        with tempfile.TemporaryDirectory(prefix='native-mm-scene-factories-') as directory:
            temporary = Path(directory)
            source, binary = temporary / 'factories.cpp', temporary / 'factories'
            source.write_text(FACTORY_PROOF)
            # Resource's destructor trace is the sole replaced boundary. Native
            # factories, resource classes and the whole BinaryReader/Stream path
            # compile from the production files without extraction or rewriting.
            logger = temporary / 'spdlog/spdlog.h'
            logger.parent.mkdir()
            logger.write_text('#pragma once\n#define SPDLOG_TRACE(...)\n')
            include = [temporary, ROOT, ROOT / 'mm', ROOT / 'libultraship/include']
            subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra',
                            '-Werror', '-Wno-sign-compare', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                            '-no-pie', *['-I' + str(path) for path in include], str(source),
                            *[str(ROOT / path) for path in sources], '-o', str(binary)], check=True)
            paths = []
            for identity in (2, 27):
                path = temporary / f'command-{identity}.bin'
                path.write_bytes(struct.pack('<I', identity) + commands[identity])
                paths.append(str(path))
            environment = {**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'}
            subprocess.run([str(binary), *paths], check=True, env=environment)
            malformed = bytearray(Path(paths[0]).read_bytes())
            struct.pack_into('<h', malformed, 12, 999)
            Path(paths[0]).write_bytes(malformed)
            rejected = subprocess.run([str(binary), *paths], capture_output=True, text=True, env=environment)
            self.assertNotEqual(rejected.returncode, 0, 'factory proof must detect a different decoded camera pose')
            self.assertIn('different authored pose', rejected.stderr)


if __name__ == '__main__':
    unittest.main()
