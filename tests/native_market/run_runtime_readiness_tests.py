#!/usr/bin/env python3
"""Execute both native market readiness bodies with exact resource inventories."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from

with tempfile.TemporaryDirectory(prefix="native-market-ready-") as directory:
    for host, path, label, other in (
        ("mm", "mm/2s2h/Enhancements/Gameplay/BlackMarket.cpp", "Mm", "scenes/shared/skiejer_market_oot/skiejer_market_oot"),
        ("oot", "soh/soh/Enhancements/Gameplay/ComboBlackMarketNative.cpp", "Oot", "scenes/nonmq/skiejer_market/skiejer_market"),
    ):
        source = (ROOT / path).read_text()
        body = block_from(source, source.index("bool PackAvailable("))
        fixture = r'''
#include <algorithm>
#include <cassert>
#include <set>
#include <string>
#include <cstring>
#include <cstdio>
#include "combo/BlackMarketEntry.h"
#include "combo/BlackMarketNativeResources.h"
#include "combo/BlackMarketMmShopScene.h"
#include "mm/2s2h/Enhancements/Gameplay/BlackMarketResources.h"
std::set<std::string> files;
std::set<std::string> nativeFiles;
bool nativeAvailable=true;
int mmInitializations=0;
enum class ImportFailure { None, Limb, DisplayList, Texture };
ImportFailure importFailure=ImportFailure::None;
int nativeImports=0;
int loadingQueries=0;
bool ResourceMgr_FileExists(const char* path) { return files.count(path); }
bool IsNativeMm(const char* path) {
 return std::strncmp(path,"objects/object_bg/",18)==0 ||
        std::strcmp(path,"scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORITex_00BAC0")==0;
}
void MmAssets_Init() {
 ++mmInitializations;
 if(nativeAvailable)for(const char* path:kNativeBlackMarketOotResources)
  if(IsNativeMm(path))nativeFiles.insert(path);
}
bool MmAssets_ResourceExists(const char* path) { ++loadingQueries;return nativeFiles.count(path); }
bool MmAssets_BaseResourceExists(const char* path) { return nativeFiles.count(path); }
bool CustomKeeperAvailable() {
 for(const char* path:kBlackMarketResources)
  if(std::strncmp(path,"custom/skiejer_market/keeper/",29)==0 && !files.count(path))return false;
 return true;
}
bool LoadNativeKeeper() {
 ++nativeImports;
 return importFailure==ImportFailure::None;
}
/* PRODUCTION */
int main() {
 for(const char* path:kNativeBlackMarket/* LABEL */Resources)files.insert(path);
#ifndef OOT_READY_TEST
 assert(!PackAvailable());
 for(const char* path:BlackMarketMmShopScene::kPresentationResources)files.insert(path);
 for(const char* path:BlackMarketMmShopScene::kPresentationResources) {
  files.erase(path);
  assert(!PackAvailable() && "each private MM presentation clone is required");
  files.insert(path);
 }
#endif
#ifdef OOT_READY_TEST
 for(auto it=files.begin();it!=files.end();) {
  if(IsNativeMm(it->c_str()))it=files.erase(it);else ++it;
 }
 assert(nativeFiles.empty());
#endif
 assert(PackAvailable());
 assert(loadingQueries==0 && "readiness membership checks must not load native resources");
#ifdef OOT_READY_TEST
 assert(mmInitializations==1 && !nativeFiles.empty());
 for(const auto failure:{ImportFailure::Limb,ImportFailure::DisplayList,ImportFailure::Texture}) {
  importFailure=failure;
  const int imports=nativeImports;
  assert(!PackAvailable() && "a missing native limb or DL/texture graph must reject keeper readiness");
  assert(nativeImports==imports+1);
 }
 for(const char* path:kBlackMarketResources)
  if(std::strncmp(path,"custom/skiejer_market/keeper/",29)==0)files.insert(path);
 const int imports=nativeImports;
 assert(PackAvailable() && "a complete optional keeper bypasses native model failure");
 assert(nativeImports==imports);
 files.erase("custom/skiejer_market/keeper/ShopGalMMDTexture4");
 assert(!PackAvailable() && "a partial optional keeper cannot bypass native model failure");
 for(auto it=files.begin();it!=files.end();)
  if(it->starts_with("custom/skiejer_market/keeper/"))it=files.erase(it);else ++it;
 importFailure=ImportFailure::None;
 assert(PackAvailable() && "native model failure must remain retryable");
#endif
 assert(files.count("custom/skiejer_market/texture/arcane_stone"));
 files.erase("custom/skiejer_market/texture/arcane_stone");
 assert(!PackAvailable() && "the shipped shelf surface is a required default dependency");
 files.insert("custom/skiejer_market/texture/arcane_stone");
 assert(PackAvailable());
 assert(files.count("custom/skiejer_market/title") && "default location card is a shipped core resource");
 assert(!files.count("CosmeticEntries"));
 assert(!files.count("/* OTHER */"));
 files.erase("objects/object_bg/gTreasureChestShopGalSkel");
 nativeFiles.erase("objects/object_bg/gTreasureChestShopGalSkel");
 nativeAvailable=false;
 assert(!PackAvailable());
 files={"custom/skiejer_market/keeper/gTreasureChestShopGalSkel","custom/skiejer_market/texture/arcane_stone"};
 assert(!PackAvailable());
 files.clear();
 for(const char* path:kBlackMarketResources)files.insert(path);
#ifndef OOT_READY_TEST
 assert(!PackAvailable() && "stale legacy MM presentation needs its private companion resources");
 for(const char* path:BlackMarketMmShopScene::kPresentationResources)files.insert(path);
#endif
 mmInitializations=0;nativeImports=0;nativeFiles.clear();
 importFailure=ImportFailure::Limb;
 assert(PackAvailable());
 assert(mmInitializations==0 && "self-contained legacy pack must not initialize the base MM owner");
 assert(nativeImports==0 && "self-contained legacy pack must not import a native model");
 puts("PASS native market readiness: own host geometry/native keeper, failed model preflight rejected, complete custom/legacy bypass, no optional art required");
}
'''.replace("/* PRODUCTION */", body).replace("/* LABEL */", label).replace("/* OTHER */", other)
        target = Path(directory) / (host + ".cpp")
        binary = Path(directory) / host
        target.write_text(fixture)
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", *( ["-DOOT_READY_TEST"] if host=="oot" else [] ),
                        "-I"+str(ROOT), str(target), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
