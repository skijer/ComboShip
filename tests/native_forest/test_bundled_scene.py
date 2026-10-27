#!/usr/bin/env python3
"""Consume the checked-in real native bundle through the runtime dependency gate."""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[2]

class BundledSceneTests(unittest.TestCase):
    def test_real_bundle_prepares_authored_additions_without_a_scene_mod(self):
        spec=importlib.util.spec_from_file_location('prepare_native_forest',ROOT/'scripts/assets/prepare_native_forest.py')
        prepare=importlib.util.module_from_spec(spec);spec.loader.exec_module(prepare)
        manifest=ROOT/'tools/native_forest/manifest.json'
        resources=prepare.prepared_resources(manifest)
        receipt=json.loads(manifest.read_text())
        self.assertEqual(receipt['status'],'generated')
        self.assertEqual(receipt['native_stock_rooms'],3)
        self.assertEqual([row['segment'] for row in receipt['scroll_bindings']],list(range(8,14)))
        self.assertEqual(receipt['custom_texture_payloads'],12)
        self.assertEqual(receipt['inputs']['native_mm']['sha256'],
                         '2f44bafe1a00f44ddaf7eb5e5c99e73a3d23771038529d6d1748819ad575dc80')
        self.assertEqual(receipt['presentation']['version'],2)
        self.assertEqual(len(receipt['presentation']['sources']),45)
        self.assertEqual(sum(data[4:8]==b'XETO' for data in resources.values()),12)
        self.assertEqual((ROOT/'combo/entrances/NativeForestResources.h').read_text(),
                         prepare.native_forest.native_core_header(resources))
        with tempfile.TemporaryDirectory(prefix='real-native-runtime-') as name:
            folder=Path(name)
            prepare.prepare(folder,manifest)
            for game in ('soh','mm'):
                self.assertEqual({p.relative_to(folder/game/'assets/custom').as_posix():p.read_bytes()
                                  for p in (folder/game/'assets/custom').rglob('*') if p.is_file()},resources)
            fixture=r'''
#include "combo/entrances/NativeForest.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <vector>
int main(int argc,char** argv) {
 assert(argc==4);
 std::set<std::string> core,stock;
 std::ifstream c(argv[1]),s(argv[2]);std::string path;
 while(std::getline(c,path))core.insert(path);
 while(std::getline(s,path))stock.insert(path);
 std::ifstream f(argv[3],std::ios::binary);
 std::vector<uint8_t> payload((std::istreambuf_iterator<char>(f)),{});
 auto load=[&](const char*){return NativeForestDependencies::Parse(payload.data(),payload.size());};
 auto exists=[&](const char* p){return core.count(p);};
 auto native=[&](const char* p){return stock.count(p);};
 const auto graph=load(nullptr);assert(!graph.core.empty() && graph.stock.size()==88);
 const char* scene="/* SCENE */";
 assert(NativeForest::SceneAvailable(scene,exists,native,load));
 core.erase("custom/comboship/native_forest/geometry/Z2_LOST_WOODS_room_00DL_002548");
 assert(!NativeForest::SceneAvailable(scene,exists,native,load));
 core.insert("custom/comboship/native_forest/geometry/Z2_LOST_WOODS_room_00DL_002548");
 stock.erase("scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS_room_02DL_000D90");
 assert(!NativeForest::SceneAvailable(scene,exists,native,load));
 stock.insert("scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS_room_02DL_000D90");
 assert(NativeForest::SceneAvailable(scene,exists,native,load));
}
'''
            for mode,scene in receipt['scene_paths'].items():
                descriptor=receipt['dependency_paths'][mode]
                core,stock=prepare.native_forest.read_dependency_blob(resources[descriptor])
                index=folder/(mode+'.core');index.write_text('\n'.join([*core,descriptor])+'\n')
                owner=folder/(mode+'.stock');owner.write_text('\n'.join([*stock,'scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS_room_00'])+'\n')
                data=folder/(mode+'.payload');data.write_bytes(resources[descriptor][68:]+bytes(16))
                cpp=folder/(mode+'.cpp');cpp.write_text(fixture.replace('/* SCENE */',scene))
                binary=folder/(mode+'-gate')
                subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',
                                '-I'+str(ROOT),str(cpp),'-o',str(binary)],check=True)
                subprocess.run([str(binary),str(index),str(owner),str(data)],check=True)

if __name__=='__main__':unittest.main()
