#!/usr/bin/env python3
"""Compile the real settings merge. --baseline reproduces the pre-fix importer."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--json-include", type=Path)
parser.add_argument("--compiler", default=os.environ.get("CXX", "c++"))
parser.add_argument("--baseline", action="store_true")
parser.add_argument("--sanitize", action="store_true")
args = parser.parse_args()

with tempfile.TemporaryDirectory(prefix="combo-settings-import-") as temp:
    build = Path(temp)
    if args.baseline:
        source = subprocess.check_output(
            ["git", "show", "a82852a3:combo/ComboShip.cpp"], cwd=ROOT, text=True)
        merge_start = source.index("static void DeepMerge(")
        merge_end = source.index("// Soft validator", merge_start)
        import_start = source.index("                if (haveMm) {", source.index("// --- 3b."))
        import_end = source.index("                    SOH_ApplyImportedConfig(", import_start)
        header = build / "combo/ComboSettingsImportConfig.h"
        header.parent.mkdir()
        # Execute the original launcher statements, including its actual merge
        # function, without requiring the window/ROM/DLL boot infrastructure.
        header.write_text(
            "#include <nlohmann/json.hpp>\nnamespace ComboSettingsImport {\n" +
            source[merge_start:merge_end] +
            "inline nlohmann::json Merge(const nlohmann::json& sohJson, const nlohmann::json& mmJson) {\n"
            "const bool haveSoh = sohJson.is_object(), haveMm = mmJson.is_object();\n"
            "nlohmann::json merged = nlohmann::json::object();\n" +
            source[import_start:import_end] + "}\nreturn merged;\n}\n}\n", encoding="utf-8")

    command = [args.compiler, "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pedantic",
               "-I" + str(build), "-I" + str(ROOT)]
    if args.json_include:
        command += ["-I" + str(args.json_include)]
    if args.sanitize:
        command += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-g"]
    binary = build / "import-test"
    subprocess.run(command + [str(ROOT / "tests/settings_import/import_test.cpp"), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
