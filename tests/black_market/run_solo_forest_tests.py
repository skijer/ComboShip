#!/usr/bin/env python3
"""Compile both production forest routes with their actual native engine types."""
from pathlib import Path
import json
import os
import subprocess
import sys
import tempfile
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from
native = "--native" in sys.argv
if native:
    sys.path.insert(0, str(ROOT / "scripts/assets"))
    import prepare_native_forest
    manifest_path = ROOT / "tools/native_forest/manifest.json"
    resources = prepare_native_forest.verified_resources(manifest_path)
    receipt = json.loads(manifest_path.read_text())
signatures = ["ForestOrigin DefaultForestOrigin(", "bool ForestVisit(", "bool NativePortal(", "bool OriginScene(",
              "bool ForestSafe(", "void ForestTransition(", "void StageForestOrigin(",
              "void ForestPlayerUpdate(", "void LoadForestState(", "void SaveForestState(",
              "void NormalizeForestEntry("]
with tempfile.TemporaryDirectory(prefix="solo-forest-native-") as temp:
    for engine, path in (("mm", "mm/2s2h/Enhancements/Gameplay/ComboSoloForest.cpp"),
                         ("soh", "soh/soh/Enhancements/Gameplay/ComboSoloForest.cpp")):
        assert (ROOT / path).exists(), f"native {engine} forest route is missing"
        source = (ROOT / path).read_text()
        selected = list(signatures)
        if engine == "soh":
            selected += [signature for signature in ("void ResetForestFile(", "void BindForestFile(") if signature in source]
            selected += ["void RegisterSoloForest("]
        else:
            selected += ['extern "C" int ComboSoloForestMm_PassageAvailable(']
        bodies = "\n".join(block_from(source, source.index(signature)) for signature in selected)
        fixture = (ROOT / "tests/black_market/solo_forest_native_test.cpp").read_text().replace("/* PRODUCTION */", bodies)
        if native:
            mode = "oot" if engine == "soh" else "mm"
            descriptor = receipt["dependency_paths"][mode]
            core, stock = prepare_native_forest.native_forest.read_dependency_blob(resources[descriptor])
            owned = [*core, descriptor, *(stock if engine == "mm" else [])]
            owner = [*stock, "scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS_room_00"]
            if engine == "mm":
                owned += owner
            paths = lambda values: ",".join(json.dumps(value) for value in values)
            payload = ",".join(str(value) for value in resources[descriptor][68:] + bytes(16))
            availability = (
                "#include <algorithm>\n#include <set>\n#include <string>\n"
                '#include "entrances/SoloForestResources.h"\n'
                f"std::set<std::string> nativeFiles={{{paths(owned)}}};\n"
                f"std::set<std::string> nativeStock={{{paths(owner)}}};\n"
                f"std::vector<uint8_t> nativePayload={{{payload}}};\n"
                "bool ResourceMgr_FileExists(const char* path) { return pack && nativeFiles.count(path); }\n"
                "void MmAssets_Init() {}\n"
                "bool MmAssets_BaseResourceExists(const char* path) { return pack && nativeStock.count(path); }\n"
                "namespace NativeForest {\n"
                "NativeForestDependencies::Graph LoadDependencies(const char* path) {\n"
                " return ResourceMgr_FileExists(path) ? NativeForestDependencies::Parse(nativePayload.data(),nativePayload.size())"
                " : NativeForestDependencies::Graph{};\n}\n}\n"
            )
            pack_body = block_from(source, source.index("bool ForestPack("))
            fixture = fixture.replace("bool ForestPack() { return pack; }", availability + pack_body)
        target = Path(temp) / (engine + ".cpp"); target.write_text(fixture)
        binary = Path(temp) / engine
        includes = [engine, engine + "/include", engine + "/include/PR", engine + "/src", engine + "/assets",
                    engine + ("/soh" if engine == "soh" else "/2s2h"), "libultraship/include", "combo", "combo/menu"]
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-DCOMBO_BUILD", "-DF3DEX_GBI_2",
                        *(["-DOOT_FOREST_TEST"] if engine == "soh" else []), "-g", "-fsanitize=address,undefined",
                        "-fno-sanitize-recover=all", "-no-pie", *["-I" + str(ROOT / p) for p in includes],
                        str(target), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
        if native:
            print(f"PASS {engine} solo route uses the real bundled native dependency gate without a visual archive", flush=True)
