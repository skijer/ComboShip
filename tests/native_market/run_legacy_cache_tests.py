#!/usr/bin/env python3
"""A complete legacy market must survive the real extension-cache convention."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from

with tempfile.TemporaryDirectory(prefix="market-legacy-cache-") as temporary:
    for host, file in (("mm", "mm/2s2h/Enhancements/Gameplay/BlackMarket.cpp"),
                       ("oot", "soh/soh/Enhancements/Gameplay/ComboBlackMarketNative.cpp")):
        source = (ROOT / file).read_text()
        body = block_from(source, source.index("bool PackAvailable("))
        body += "\n" + block_from(source, source.index("bool DestinationAvailable("))
        fixture = r'''
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include "combo/BlackMarketNativeResources.h"
#include "combo/BlackMarketMmShopScene.h"
#include "mm/2s2h/Enhancements/Gameplay/BlackMarketResources.h"
#include "combo/BlackMarketEntry.h"
std::set<std::string> cache;
bool ResourceMgr_FileExists(const char* path) {return cache.count(path);}
void MmAssets_Init() {assert(false && "legacy readiness must not import an MM model");}
bool MmAssets_BaseResourceExists(const char*) {return false;}
bool CustomKeeperAvailable() {return false;}
bool LoadNativeKeeper() {return false;}
/* PRODUCTION */
int main() {
/* PRIVATE_MM_CLONES */
 for(const char* path:kBlackMarketResources) {
  // OTRExtScanner drops the final extension. Typed members add a suffix;
  // the builder's raw manifest.json member instead loses its .json suffix.
  std::string member=path;
  if(member!="custom/skiejer_market/manifest.json")member+=".bin";
  cache.insert(member.substr(0,member.rfind('.')));
 }
 assert(PackAvailable() && "complete legacy pack must not fail on raw manifest.json");
 cache.erase("custom/skiejer_market/manifest");
 assert(!PackAvailable() && "a genuinely absent manifest must still fail legacy readiness");
 cache.clear();
 for(const char* path:BlackMarketEntry::k/* HOST */Destination)cache.insert(path);
 assert(DestinationAvailable() && "physical entry must not depend on stock or optional art");
 for(const char* path:BlackMarketEntry::k/* HOST */Destination) {
  cache.erase(path);
  assert(!DestinationAvailable() && "each scene/room/collision resource is required");
  cache.insert(path);
 }
 puts("PASS legacy manifest cache key; scene/room/collision required; stock and optional art independent");
}
'''.replace("/* PRODUCTION */", body).replace("/* HOST */", "Mm" if host == "mm" else "Oot").replace(
    "/* PRIVATE_MM_CLONES */", 'for(const char* path:BlackMarketMmShopScene::kPresentationResources)cache.insert(path);' if host == 'mm' else '')
        cpp, binary = Path(temporary)/(host+".cpp"), Path(temporary)/host
        cpp.write_text(fixture)
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-I"+str(ROOT), str(cpp), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
