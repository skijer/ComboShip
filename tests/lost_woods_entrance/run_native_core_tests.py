#!/usr/bin/env python3
"""Native forest access and Mirror readiness use the active host's base scene."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from

with tempfile.TemporaryDirectory(prefix="native-forest-core-") as directory:
    for host, sub in (("mm", "2s2h"), ("soh", "soh")):
        for filename, name in (("ComboSoloForest.cpp", "ForestPack"), ("MagicMirror.cpp", "PackAvailable")):
            source = (ROOT / host / sub / "Enhancements/Gameplay" / filename).read_text()
            body = block_from(source, source.index("bool " + name + "("))
            scene = "scenes/nonmq/skiejer_forest/skiejer_forest" if host == "mm" else "scenes/shared/skiejer_forest_oot/skiejer_forest_oot"
            fixture = r'''
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <set>
#include <string>
#include "combo/entrances/NativeForest.h"
#include "combo/entrances/NativeForestResources.h"
#include "combo/entrances/NativeForestDependencies.h"
#include "combo/entrances/SoloForestResources.h"
std::set<std::string> files;
std::set<std::string> stockFiles={"scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS_room_00",
                                 "scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORITex_00BAC0"};
bool stockAvailable=true;
bool dependencyDataAvailable=true;
namespace NativeForest {
NativeForestDependencies::Graph LoadDependencies(const char*) {
 if(!dependencyDataAvailable)return {};
 return {{"custom/comboship/native_forest/geometry/required_ancestor"},
         {"scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS_room_00",
          "scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORITex_00BAC0"}};
}
}
int nativeInitializations=0;
void MmAssets_Init() { ++nativeInitializations; }
bool MmAssets_BaseResourceExists(const char* path) { return stockAvailable && stockFiles.count(path); }
bool Combo_IsCombinedGame() { return false; }
bool ResourceMgr_FileExists(const char* path) { return files.count(path); }
/* PRODUCTION */
int main() {
 const std::string scene="/* SCENE */";
 files={"custom/comboship/native_forest/materials_v1",scene,scene+"_room_0",scene+"_room_1",scene+"_room_2"};
 files.insert(scene+"_collision");
 for(const char* core:NativeForestResources::kCommon)files.insert(core);
 files.insert("custom/comboship/native_forest/dependencies_/* MODE */_v1");
 files.insert("custom/comboship/native_forest/geometry/required_ancestor");
 /* HOST_STOCK */
 assert(/* CALL */());
 files.erase("custom/comboship/native_forest/geometry/required_ancestor");
 assert(!/* CALL */());
 files.insert("custom/comboship/native_forest/geometry/required_ancestor");
 dependencyDataAvailable=false;assert(!/* CALL */());dependencyDataAvailable=true;
 for(const char* core:{"custom/comboship/native_forest/building_root",
                       "custom/comboship/native_forest/portal_root",
                       "custom/comboship/native_forest/geometry/Z2_LOST_WOODS_room_00Vtx_0019D8",
                       "custom/comboship/native_forest/stock_materials",
                       "custom/comboship/native_forest/texanim"}) {
   files.erase(core); assert(!/* CALL */()); files.insert(core);
 }
 files.erase(scene+"_collision"); assert(!/* CALL */()); files.insert(scene+"_collision");
 /* NATIVE_OWNER */
 files.erase(scene+"_room_2");
 assert(!/* CALL */());
 files={"custom/skiejer_forest/title","custom/skiejer_forest/final_scroll_v1"};
 assert(!/* CALL */());
 files.clear();
 nativeInitializations=0;
 for(const char* path:kSoloForestResources)files.insert(path);
 assert(/* CALL */());
 assert(nativeInitializations==0);
 puts("PASS native forest/Mirror readiness: own host base, no visual pack, incomplete base rejected, legacy compatibility");
}
'''.replace("/* PRODUCTION */", body).replace("/* SCENE */", scene).replace("/* CALL */", name).replace(
                "/* MODE */", "oot" if host=="soh" else "mm").replace("/* HOST_STOCK */",
                "for(const auto& stock:stockFiles)files.insert(stock);" if host=="mm" else "").replace(
                "/* NATIVE_OWNER */", "stockAvailable=false; assert(!"+name+"()); stockAvailable=true; "
                "stockFiles.erase(\"scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORITex_00BAC0\"); assert(!"+name+
                "()); stockFiles.insert(\"scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORITex_00BAC0\");" if host=="soh" else "")
            path = Path(directory) / (host + filename + ".cpp")
            binary = Path(directory) / (host + filename)
            path.write_text(fixture)
            subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-I"+str(ROOT), str(path), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)
