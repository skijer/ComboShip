#!/usr/bin/env python3
"""Execute the real message replacement overloads, including their foreign receipt call."""
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "soh/soh/Enhancements/custom-message/CustomMessageManager.cpp"
HEADER = SOURCE.with_suffix(".h")


def function(source, signature):
    start = source.index(signature)
    begin = source.index("{", start)
    depth, end = 1, begin + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


source = SOURCE.read_text()
declarations = re.findall(r"    void Replace\(std::string&&[^;]+;", HEADER.read_text())
assert len(declarations) == 2, "Exercise both real replacement overloads"
production = "\n".join(function(source, signature) for signature in (
    "void CustomMessage::Replace(std::string&& oldStr, std::string&& newStr)",
    "void CustomMessage::Replace(std::string&& oldStr, CustomMessage newMessage)",
))
fixture = (ROOT / "tests/item_receipts/message_replace_test.cpp").read_text()
for marker in ("/* REPLACE_DECLARATIONS */", "/* PRODUCTION_REPLACE */"):
    assert fixture.count(marker) == 1, f"Missing or duplicate fixture marker: {marker}"
fixture = fixture.replace("/* REPLACE_DECLARATIONS */", "\n".join(declarations))
fixture = fixture.replace("/* PRODUCTION_REPLACE */", production)
flags = ["-std=c++20", "-Wall", "-Wextra", "-Werror"]
if "--sanitize" in sys.argv:
    flags += ["-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
              "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
with tempfile.TemporaryDirectory(prefix="foreign-master-receipt-") as directory:
    cpp = Path(directory) / "replace.cpp"
    cpp.write_text(fixture)
    binary = Path(directory) / "replace"
    subprocess.run([*shlex.split(os.environ.get("CXX", "c++")), *flags,
                    "-I" + str(ROOT), str(cpp), "-o", str(binary)], check=True)
    try:
        subprocess.run([str(binary)], check=True, timeout=3)
    except subprocess.TimeoutExpired:
        raise SystemExit("FAIL: foreign Master Sword receipt replacement did not terminate")
