#!/usr/bin/env python3
"""Exercise production sword save writers/loaders with actual NEI and JSON types.

MM includes the complete production converters. OoT uses the unmodified NEI
callbacks and SaveManager JSON methods, with only disk/sync/audit boundaries
omitted. No test-side replacement parser implements sword persistence.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile
from run_time_pedestal_tests import block_from, functions

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_DEPS = Path('/workspace/scratch/27a277416750/music-source/deps')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--deps', type=Path, default=DEFAULT_DEPS)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    checks = (ROOT / 'tests/elemental_swords/save_roundtrip.cpp').read_text()
    includes = [args.deps / path for path in ('json-3.11.3/include', 'spdlog-1.15.3/include',
                                             'glm-1.0.1', 'include')]
    with tempfile.TemporaryDirectory(prefix='sword-save-') as directory:
        build = Path(directory)
        for host in ('soh', 'mm'):
            source = '#include <cstring>\n#include <functional>\n#include <nlohmann/json.hpp>\n'
            source += 'namespace ItemGrantAudit { struct Scope { explicit Scope(const char*) {} }; }\n'
            source += 'using json = nlohmann::json;\n'
            store = (ROOT / host / 'mods/nei_save.cpp').read_text()
            callbacks = functions(store.replace('extern "C" ', ''))
            if host == 'mm':
                source += '#include "global.h"\n#include "BenJsonConversions.hpp"\n'
                source += 'SaveContext gSaveContext{};\n'
                source += callbacks['Nei_InitNewSave'] + '\n'
                source += '''
static NeiSaveData& State() { return gSaveContext.save.shipSaveInfo.nei; }
static void Load(const json& j) { from_json(j, State()); }
static json Persist() { return json(State()); }
static void Init() { Nei_InitNewSave(); }
'''
            else:
                header = (ROOT / 'soh/soh/SaveManager.h').read_text()
                manager = (ROOT / 'soh/soh/SaveManager.cpp').read_text()
                source += '#include "z64.h"\n#include "mods/nei_save.h"\n'
                source += 'static NeiSaveData gNeiSave{};\n'
                source += '''
void Bottle_WheelResetTracking() {}
void TradeItems_SyncWrite() {}
void TradeItems_SyncRead() {}
void Picto_SyncRead() {}
class SaveManager {
public:
    static SaveManager* Instance;
    json* currentJsonContext;
    json::iterator currentJsonArrayContext;
    using SaveArrayFunc = std::function<void(size_t)>;
    using LoadArrayFunc = std::function<void(size_t)>;
    void SaveArray(const std::string&, size_t, SaveArrayFunc);
    void LoadArray(const std::string&, size_t, LoadArrayFunc);
'''
                for method in ('SaveData', 'LoadData'):
                    start = header.index('template <typename T> void ' + method)
                    body = header.index(') {', start) + 2
                    source += header[start:body] + block_from(header, body) + '\n'
                source += '};\nSaveManager* SaveManager::Instance;\n'
                for method in ('SaveArray', 'LoadArray'):
                    start = manager.index('void SaveManager::' + method + '(')
                    source += block_from(manager, start) + '\n'
                for callback in ('NeiSave_Init', 'NeiSave_Save', 'NeiSave_Load'):
                    source += callbacks[callback] + '\n'
                source += '''
static json data;
static SaveManager manager;
static void Load(const json& j) {
    data = j; manager.currentJsonContext = &data; SaveManager::Instance = &manager;
    NeiSave_Load();
}
static json Persist() {
    data = json::object(); manager.currentJsonContext = &data; SaveManager::Instance = &manager;
    const SaveContext context{}; NeiSave_Save(context, 0, true); return data;
}
static void Init() { NeiSave_Init(false); }
'''
            source += checks
            cpp, binary = build / (host + '.cpp'), build / host
            cpp.write_text(source)
            roots = [ROOT / host / path for path in ('', 'include', 'include/PR', 'src', 'assets',
                                                     '2s2h' if host == 'mm' else 'soh')]
            roots += [ROOT / path for path in ('libultraship/include', 'combo')]
            command = [os.environ.get('CXX', 'c++'), '-std=c++20', '-w', '-DF3DEX_GBI_2',
                       '-DFMT_HEADER_ONLY', '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections',
                       *['-I' + str(path) for path in roots + includes], str(cpp), '-o', str(binary)]
            if args.sanitize:
                command[1:1] = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie']
            subprocess.run(command, cwd=ROOT, check=True)
            subprocess.run([str(binary)], check=True,
                           env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))


if __name__ == '__main__':
    main()
