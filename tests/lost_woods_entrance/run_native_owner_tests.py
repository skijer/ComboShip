#!/usr/bin/env python3
"""Query the real base-owner metadata API without loading optional overrides."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import block_from
source=(ROOT/'soh/mods/transformation_masks/assets/mm_asset_loader.cpp').read_text()
body=block_from(source,source.index('u8 MmAssets_BaseResourceExists('))
fixture=r'''
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <set>
#include <string>
using u8=unsigned char;
bool sMmO2rLoaded=false;
struct Archive {
 std::set<std::string> files;
 int probes=0;
 bool HasFile(const std::string& path) { ++probes; return files.count(path); }
};
std::shared_ptr<Archive> sMmArchive;
/* PRODUCTION */
int main() {
 assert(!MmAssets_BaseResourceExists(nullptr));
 assert(!MmAssets_BaseResourceExists("scenes/native"));
 sMmO2rLoaded=true;
 assert(!MmAssets_BaseResourceExists("scenes/native"));
 sMmArchive=std::make_shared<Archive>();
 sMmArchive->files={"scenes/native"};
 assert(MmAssets_BaseResourceExists("scenes/native"));
 assert(MmAssets_BaseResourceExists("__OTR__scenes/native"));
 assert(!MmAssets_BaseResourceExists("scenes/optional_only"));
 assert(sMmArchive->probes==3);
 assert(!MmAssets_BaseResourceExists(""));
 assert(!MmAssets_BaseResourceExists("__OTR__"));
 assert(sMmArchive->probes==3);
 sMmO2rLoaded=false;
 assert(!MmAssets_BaseResourceExists("scenes/native"));
 assert(sMmArchive->probes==3);
 std::puts("PASS native MM owner: pinned archive membership only, prefix handling, absent owner/path rejection, no resource loads or optional fallback");
}
'''.replace('/* PRODUCTION */',body)
with tempfile.TemporaryDirectory(prefix='native-mm-owner-') as directory:
 cpp=Path(directory)/'test.cpp';binary=Path(directory)/'test';cpp.write_text(fixture)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',
                 '-I'+str(ROOT),str(cpp),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
