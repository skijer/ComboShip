#!/usr/bin/env python3
"""Exercise the launcher's real release gate, cache and disk backup behavior."""
import argparse
import json
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--baseline-ref')
parser.add_argument('--json-include', default='/usr/include')
args = parser.parse_args()

def read(path):
    if args.baseline_ref:
        return subprocess.check_output(['git', 'show', f'{args.baseline_ref}:{path}'], cwd=ROOT, text=True)
    return (ROOT / path).read_text()

def function(source, name):
    # Unlike several C-oriented fixtures, this loader returns a C++ reference.
    match = re.search(r'^static[^\n]*\b' + re.escape(name) + r'\([^;{}]*\)\s*\{', source, re.M)
    assert match, name
    masked = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"',
                    lambda token: ' ' * len(token[0]), source)
    end, depth = match.end(), 1
    while depth:
        depth += (masked[end] == '{') - (masked[end] == '}')
        end += 1
    return source[match.start():end]

release = re.search(r'set\(COMBO_RELEASE_VERSION "([^"]+)"', read('CMakeLists.txt'))[1]
production = read('combo/ComboShip.cpp')
source = r'''
#include <cassert>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
static std::map<int, nlohmann::json> g_containerCache;
static std::vector<int> g_evictedSlots;
'''
source += '#define COMBO_RELEASE_VERSION ' + json.dumps(release) + '\n'
source += '\n'.join(function(production, name) for name in (
    'ComboContainerPath', 'ComboReleaseMajorMinor', 'LoadOrCreateContainer'))
source += r'''
int main() {
    namespace fs = std::filesystem;
    const auto current = ComboReleaseMajorMinor(COMBO_RELEASE_VERSION);
    assert(current != "0.3" && "pre-9.3 enum schemas must not pass the release gate");
    assert(ComboReleaseMajorMinor("0.3.99") == "0.3");
    fs::create_directory("Save");
    nlohmann::json old = {{"comboRelease", "0.3.0"}, {"oot", {{"sentinel", 123}}},
                          {"mm", {{"sentinel", 456}}}, {"combo", {{"notes", "keep me"}}}};
    const std::string bytes = old.dump();
    { std::ofstream f(ComboContainerPath(0)); f << bytes; }
    const auto fresh = LoadOrCreateContainer(0);
    assert(fresh["oot"].is_null() && fresh["mm"].is_null());
    assert(fresh["comboRelease"] == COMBO_RELEASE_VERSION);
    assert(g_evictedSlots == std::vector<int>{0});
    assert(!fs::exists(ComboContainerPath(0)));
    int backups = 0;
    for (const auto& entry : fs::directory_iterator("Save")) {
        assert(entry.path().extension() == ".bak");
        std::ifstream f(entry.path());
        std::string saved((std::istreambuf_iterator<char>(f)), {});
        assert(saved == bytes && "both game sections and notes must survive backup byte for byte");
        ++backups;
    }
    assert(backups == 1);
    // Reading the same slot twice is cache-only; no duplicate eviction notice.
    LoadOrCreateContainer(0);
    assert(g_evictedSlots.size() == 1);
    // Patch updates of this same schema remain compatible and retain contents.
    g_containerCache.clear(); g_evictedSlots.clear();
    old["comboRelease"] = current + ".99";
    { std::ofstream f(ComboContainerPath(1)); f << old.dump(); }
    const auto kept = LoadOrCreateContainer(1);
    assert(kept["oot"]["sentinel"] == 123 && kept["mm"]["sentinel"] == 456);
    assert(kept["combo"]["notes"] == "keep me" && g_evictedSlots.empty());
    assert(fs::exists(ComboContainerPath(1)));
    // A new slot is initialized with the current schema identity.
    const auto created = LoadOrCreateContainer(2);
    assert(created["comboRelease"] == COMBO_RELEASE_VERSION && created["slot"] == 2);
}
'''
with tempfile.TemporaryDirectory(prefix='combo-save-compat-') as directory:
    directory = Path(directory)
    cpp, binary = directory / 'test.cpp', directory / 'test'
    cpp.write_text(source)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-I' + args.json_include, str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], cwd=directory, check=True)
print('PASS production save compatibility: pre-9.3 retirement, byte-exact backups, one notice, patch retention, new slots')
