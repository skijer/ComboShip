#!/usr/bin/env python3
"""Compile the production metadata expressions with their real incomplete-array declarations."""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--json-include", default="/usr/include")
parser.add_argument("--compiler", default="c++")
parser.add_argument("--msvc", action="store_true")
args = parser.parse_args()
variables = (root / "soh/include/variables.h").read_text(encoding="utf-8")
production = (root / "soh/soh/OTRGlobals.cpp").read_text(encoding="utf-8")
declarations = []
fields = []
for key, symbol in [("build", "gBuildVersion"), ("commit", "gGitCommitHash")]:
    declarations.append(re.search(r"extern const char " + symbol + r"\[\];", variables)[0])
    fields.append(re.search(r'\{ "' + key + r'", [^\n]+? \}', production)[0])
with tempfile.TemporaryDirectory(prefix="build-metadata-") as directory:
    directory = Path(directory)
    source = directory / "metadata.cpp"
    binary = directory / ("metadata.exe" if args.msvc else "metadata")
    source.write_text(
        '#include <nlohmann/json.hpp>\n#include <string>\n#include <cstdlib>\n'
        + '\n'.join(declarations)
        + '\nnlohmann::json Metadata() { return {' + ','.join(fields) + '}; }\n'
        + 'const char gBuildVersion[] = "version-test";\n'
        + 'const char gGitCommitHash[] = "commit-test";\n'
        + 'int main() { auto j = Metadata(); return '
          'j.at("build") == "version-test" && j.at("commit") == "commit-test" ? EXIT_SUCCESS : EXIT_FAILURE; }\n',
        encoding="utf-8",
    )
    command = ([args.compiler, "/nologo", "/std:c++20", "/EHsc", "/W4", "/WX",
                "/I" + args.json_include, str(source), "/Fe:" + str(binary)] if args.msvc else
               [args.compiler, "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pedantic",
                "-I" + args.json_include, str(source), "-o", str(binary)])
    subprocess.run(command, cwd=directory, check=True)
    subprocess.run([str(binary)], check=True)
print("PASS production build/commit metadata with incomplete-array declarations")
