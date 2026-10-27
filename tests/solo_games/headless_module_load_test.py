#!/usr/bin/env python3
"""Exercise the production loader's ELF failures and Windows error capture."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "combo/ComboRandoHeadless.cpp").read_text()


def loader_after(marker):
    branch = source[source.index(marker):]
    match = re.search(r"DllHandle LoadGameModule\([^;{}]+\)\s*\{", branch)
    end, depth = match.end(), 1
    while depth:
        depth += (branch[end] == "{") - (branch[end] == "}")
        end += 1
    return branch[match.start():end]


body = loader_after("#else\ntypedef void* DllHandle;")
cc = os.environ.get("CXX", "c++")
with tempfile.TemporaryDirectory(prefix="headless-module-load-") as temporary:
    work = Path(temporary)
    unit = work / "loader.cpp"
    unit.write_text("#include <dlfcn.h>\n#include <iostream>\n#include <string>\n"
                    "using DllHandle = void*;\n" + body +
                    "\nint main(int argc, char** argv) {\n"
                    "  auto library = LoadGameModule(argv[1]);\n"
                    "  if (!library) return 2;\n"
                    "  dlclose(library); return 0;\n}\n")
    binary = work / "loader"
    subprocess.run([cc, "-std=c++20", str(unit), "-ldl", "-o", str(binary)], check=True)

    def run(name, expected):
        result = subprocess.run([str(binary), name], cwd=work, text=True, capture_output=True)
        assert result.returncode == expected, result
        return result.stderr

    missing = run("missing-game.so", 2)
    assert "missing-game.so" in missing and "cannot open shared object file" in missing, missing
    broken = work / "broken.cpp"
    broken.write_text('extern "C" void MissingNativeBoundary();\n'
                      'extern "C" void NativeEntry() { MissingNativeBoundary(); }\n')
    subprocess.run([cc, "-shared", "-fPIC", str(broken), "-o", str(work / "broken-game.so")], check=True)
    unresolved = run("broken-game.so", 2)
    assert "broken-game.so" in unresolved and "MissingNativeBoundary" in unresolved, unresolved
    valid = work / "valid.cpp"
    valid.write_text('extern "C" void NativeEntry() {}\n')
    subprocess.run([cc, "-shared", "-fPIC", str(valid), "-o", str(work / "valid-game.so")], check=True)
    assert run("valid-game.so", 0) == ""
    print("PASS headless module loader: missing file, actual unresolved ELF symbol, successful load")

    # A simulated Windows API checks ordering: stream output may alter the thread's last error.
    # This exercises the unchanged production helper without claiming native Windows runtime proof.
    windows = work / "windows-loader.cpp"
    windows.write_text(r'''#include <iostream>
#include <streambuf>
#include <string>
using DllHandle = void*;
unsigned long lastError;
DllHandle LoadLibraryA(const char*) { lastError = 126; return nullptr; }
unsigned long GetLastError() { return lastError; }
struct ErrorClobberingOutput : std::streambuf {
    std::string text;
    std::streamsize xsputn(const char* data, std::streamsize size) override {
        text.append(data, size); lastError = 0; return size;
    }
    int overflow(int c) override {
        if (c != traits_type::eof()) text += char(c);
        lastError = 0; return traits_type::not_eof(c);
    }
};
''' + loader_after("#ifdef _WIN32\ntypedef HMODULE DllHandle;") + r'''
int main() {
    ErrorClobberingOutput output;
    auto* original = std::cerr.rdbuf(&output);
    auto module = LoadGameModule("missing-game.dll");
    std::cerr.rdbuf(original);
    if (module || output.text.find("missing-game.dll: Windows error 126") == std::string::npos) {
        std::cerr << output.text; return 1;
    }
}
''')
    windows_binary = work / "windows-loader"
    subprocess.run([cc, "-std=c++20", str(windows), "-o", str(windows_binary)], check=True)
    subprocess.run([str(windows_binary)], check=True)
    print("PASS Windows loader error survives stream operations that alter last error")
