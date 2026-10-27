#!/usr/bin/env python3
"""Exercise production grant boundaries with dormant-save and peer callbacks stubbed.

This does not boot either game. --control combines the pre-merge NEI helpers with
upstream tier raises to demonstrate the integration regressions.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--json-include', type=Path, default=ROOT.parent / 'deps')
parser.add_argument('--control', action='store_true')
args = parser.parse_args()


def source(path, ref=None):
    if ref:
        return subprocess.check_output(['git', 'show', f'{ref}:{path}'], cwd=ROOT,
            env={**os.environ, 'GIT_NO_LAZY_FETCH': '1'}).decode('utf-8-sig')
    return (ROOT / path).read_text(encoding='utf-8-sig')


def between(text, start, end):
    return text[text.index(start):text.index(end, text.index(start))]


oot_path = 'soh/soh/OTRGlobals.cpp'
mm_path = 'mm/2s2h/BenPort.cpp'
helper_ref = 'cac526a8b9dc08c83f4c2ad7fece0113d284513d' if args.control else None
tier_ref = '94eb185e4abcc2d568aa8241fa02c43cdd86c439' if args.control else None
oot_helper = between(source(oot_path, helper_ref), 'static bool GrantOotItemByName', '// A foreign check')
mm_helper = between(source(mm_path, helper_ref), 'static bool GrantMmItemByName', '// A foreign check')
oot_raise = between(source(oot_path, tier_ref), 'extern "C" COMBO_EXPORT void SOH_RaiseSharedTier', '// Shared Items pokes')
mm_raise = between(source(mm_path, tier_ref), 'extern "C" COMBO_EXPORT void MM_RaiseSharedTier', '// Shared Items pokes')
production = oot_helper + mm_helper + oot_raise + mm_raise
rg_names = sorted(set(re.findall(r'\bRG_[A-Z_0-9]+', production)) | {'RG_PROGRESSIVE_MAGIC_METER', 'RG_NONE'})
ri_names = sorted(set(re.findall(r'\bRI_[A-Z_0-9]+', production)) | {'RI_NONE'})
preamble = r'''
#include "ItemGrantAuditBridge.h"
#include <algorithm>
#include <iostream>
#include <map>
#include <stdexcept>
#include "rando/SharedItems.h"
#include "rando/RpgStats.h"
#include "FleetSharedItems.h"
#define COMBO_EXPORT
#define SPDLOG_INFO(...) ((void)0)
#define SPDLOG_WARN(...) ((void)0)
#define SPDLOG_ERROR(...) ((void)0)
'''
preamble += 'enum RandomizerGet {' + ','.join(rg_names) + '};\n'
preamble += 'enum RandoItemId {' + ','.join(ri_names) + '};\n'
preamble += r'''
int depth = 0, echoes = 0, ootTier = 0, mmTier = 0, persisted = 0;
bool throwGrant = false;
bool mmSaveLoaded = true;
bool obtainable = true, mmForeground = true;
int blueRupees = 0, parkedFlushes = 0;
bool Combo_MmIsForeground() { return mmForeground; }
void Combo_MM_FlushAccumulators() { ++parkedFlushes; }
#define IS_RANDO mmSaveLoaded
extern "C" void FleetShared_BeginReceive() { ++depth; }
extern "C" void FleetShared_EndReceive() { --depth; }
extern "C" int FleetShared_IsReceiving() { return depth > 0; }
extern "C" void FleetShared_OnNativeObtained(int) { if (!depth) ++echoes; }
struct { bool isMagicAcquired = false, isDoubleMagicAcquired = false; int fileNum = 0; } gSaveContext;
ComboRpgState ootRpg{};
namespace FleetRpg { ComboRpgState Read() { return ootRpg; } }
int gComboSuppressAnchorSend = 0;
void* gPlayState = nullptr;
int bombchu = -1, ammo = 0;
#define ITEM_BOMBCHU 1
#define UPG_BOMB_BAG 0
#define INV_CONTENT(x) bombchu
#define AMMO(x) ammo
#define CUR_CAPACITY(x) 20
struct GetItemEntry { RandomizerGet id; bool blueRupee = false; };
constexpr int CAN_OBTAIN = 0, CANNOT_OBTAIN = 1, MOD_NONE = 0, GI_RUPEE_BLUE = 7;
struct RandomizerBoundary {
    int GetItemObtainabilityFromRandomizerGet(RandomizerGet) const {
        return obtainable ? CAN_OBTAIN : CANNOT_OBTAIN;
    }
};
struct OTRGlobals {
    static OTRGlobals* Instance;
    RandomizerBoundary* gRandomizer;
};
RandomizerBoundary randomizerBoundary;
OTRGlobals ootGlobals{&randomizerBoundary};
OTRGlobals* OTRGlobals::Instance = &ootGlobals;
struct ItemTableManager {
    static ItemTableManager* Instance;
    GetItemEntry RetrieveItemEntry(int mod, int item) const {
        if (mod != MOD_NONE || item != GI_RUPEE_BLUE) throw std::logic_error("unexpected fallback item");
        return {RG_NONE, true};
    }
};
ItemTableManager itemTableBoundary;
ItemTableManager* ItemTableManager::Instance = &itemTableBoundary;
namespace Rando { namespace StaticData {
std::map<std::string, RandomizerGet> itemNameToEnum = {
    {"Progressive Magic Meter", RG_PROGRESSIVE_MAGIC_METER}
};
struct Item {
    RandomizerGet id;
    GetItemEntry GetGIEntry_Copy() const {
        // A dormant oracle's frozen logic still reports no magic.
        return {id == RG_PROGRESSIVE_MAGIC_METER ? RG_MAGIC_SINGLE : id};
    }
};
Item RetrieveItem(RandomizerGet id) { return {id}; }
}}
void Combo_GrantResolvedOOT(const GetItemEntry& item) {
    if (throwGrant) throw std::runtime_error("injected save failure");
    if (item.blueRupee) { ++blueRupees; return; }
    ++ootTier;
    if (item.id == RG_MAGIC_SINGLE) { gSaveContext.isMagicAcquired = true; ootRpg.nativeMagicLevel = 1; }
    if (item.id == RG_MAGIC_DOUBLE) { gSaveContext.isDoubleMagicAcquired = true; ootRpg.nativeMagicLevel = 2; }
    FleetShared_OnNativeObtained(item.id);
}
const std::map<std::string, RandoItemId>& Combo_MM_SpoilerNameToItemId() {
    static const std::map<std::string, RandoItemId> names = {{"Progressive Bow", RI_PROGRESSIVE_BOW}};
    return names;
}
void Combo_MM_GiveDormantResolved(RandoItemId item) {
    if (throwGrant) throw std::runtime_error("injected save failure");
    ++mmTier;
    FleetShared_OnNativeObtained(item);
}
namespace Rando {
RandoItemId ConvertItem(RandoItemId id) { return id; }
void GiveItem(RandoItemId id) { Combo_MM_GiveDormantResolved(id); }
}
void SaveManager_SaveCurrentForCombo() { ++persisted; }
int SOH_GetSharedTier(int) { return ootTier; }
int MM_GetSharedTier(int family) { return family == ComboRando::SF_BOMBCHU_BAG ? bombchu == ITEM_BOMBCHU : mmTier; }
'''
checks = r'''
int failures = 0;
void check(bool condition, const char* label) {
    if (!condition) { std::cerr << "FAIL: " << label << '\n'; ++failures; }
}
void reset() { depth = echoes = ootTier = mmTier = persisted = 0; throwGrant = false; gPlayState = nullptr;
    ootRpg = {};
    mmSaveLoaded = true;
    obtainable = mmForeground = true;
    blueRupees = parkedFlushes = 0;
    gSaveContext.isMagicAcquired = gSaveContext.isDoubleMagicAcquired = false; gComboSuppressAnchorSend = 0; }
int main() {
    reset(); mmSaveLoaded = false;
    MM_RaiseSharedTier(ComboRando::SF_LIGHT_ARROWS, 1);
    check(mmTier == 0 && persisted == 0 && depth == 0 && echoes == 0,
          "cleared MM quit save rejects shared grants without persistence");
    reset();
    SOH_RaiseSharedTier(ComboRando::SF_BOW, 1);
    check(ootTier == 1 && echoes == 0 && depth == 0, "OoT convergence raises once without NEI echo");
    reset();
    MM_RaiseSharedTier(ComboRando::SF_BOW, 1);
    check(mmTier == 1 && echoes == 0 && depth == 0, "dormant MM convergence raises once without NEI echo");
    reset(); gPlayState = &depth;
    MM_RaiseSharedTier(ComboRando::SF_BOW, 1);
    check(mmTier == 1 && echoes == 0 && persisted == 1 && parkedFlushes == 0,
          "live MM convergence is persisted without NEI echo or parked flush");
    reset(); gPlayState = &depth; mmForeground = false;
    MM_RaiseSharedTier(ComboRando::SF_BOW, 1);
    check(mmTier == 1 && echoes == 0 && persisted == 1 && parkedFlushes == 1 && depth == 0,
          "parked MM banks pending accumulators and persists without NEI echo");
    reset();
    MM_RaiseSharedTier(ComboRando::SF_GORON_MASK, 1);
    MM_RaiseSharedTier(ComboRando::SF_GORON_MASK, 1);
    check(mmTier == 1 && echoes == 1, "native mask also reaches NEI imported alias exactly once");
    reset(); depth = 1; gComboSuppressAnchorSend = 7;
    SOH_RaiseSharedTier(ComboRando::SF_BOW, 1);
    check(depth == 1 && gComboSuppressAnchorSend == 7, "nested OoT receive and Anchor suppression restored");
    reset();
    RandomizerGet granted = RG_NONE;
    obtainable = false;
    check(GrantOotItemByName("Progressive Magic Meter", &granted) && granted == RG_MAGIC_SINGLE,
          "unobtainable grant keeps its acquisition identity");
    check(blueRupees == 1 && ootTier == 0 && !gSaveContext.isMagicAcquired && echoes == 0 && depth == 0,
          "unobtainable grant becomes a blue rupee under receive suppression");
    reset();
    check(GrantOotItemByName("Progressive Magic Meter", &granted), "first dormant magic accepted");
    check(gSaveContext.isMagicAcquired && !gSaveContext.isDoubleMagicAcquired, "first dormant magic is single");
    check(GrantOotItemByName("Progressive Magic Meter", &granted), "second dormant magic accepted");
    check(gSaveContext.isDoubleMagicAcquired, "second dormant magic resolves from save flags");
    check(!GrantOotItemByName("Progressive Magic Meter", &granted), "maxed dormant magic is not regranted");
    check(!GrantOotItemByName("unknown item", nullptr) && !GrantOotItemByName(nullptr, nullptr), "invalid grants are ignored");
    reset();
    ootRpg.knownMask = ootRpg.enabledMask = 1 << COMBO_RPG_MAGIC;
    ootRpg.level[COMBO_RPG_MAGIC] = 6;
    gSaveContext.isMagicAcquired = gSaveContext.isDoubleMagicAcquired = true;
    check(GrantOotItemByName("Progressive Magic Meter", &granted) && granted == RG_MAGIC_SINGLE,
          "fractional RPG meter does not skip the first native magic pickup");
    check(GrantOotItemByName("Progressive Magic Meter", &granted) && granted == RG_MAGIC_DOUBLE,
          "second RPG native magic resolves from saved native tier");
    check(!GrantOotItemByName("Progressive Magic Meter", &granted), "RPG double magic stays maxed");
    reset(); throwGrant = true;
    try { GrantOotItemByName("Progressive Magic Meter", nullptr); } catch (const std::exception&) {}
    check(depth == 0, "OoT failed grant unwinds receive guard");
    reset(); throwGrant = true;
    try { GrantMmItemByName("Progressive Bow", nullptr); } catch (const std::exception&) {}
    check(depth == 0, "MM failed grant unwinds receive guard");
    reset(); throwGrant = true; gComboSuppressAnchorSend = 7;
    SOH_RaiseSharedTier(ComboRando::SF_BOW, 1);
    check(depth == 0 && gComboSuppressAnchorSend == 7, "failed tier raise restores both guards");
    if (!failures) std::cout << "Combo grant boundary tests PASS\n";
    return failures ? 1 : 0;
}
'''
with tempfile.TemporaryDirectory(prefix='combo_grants_') as temp:
    temp = Path(temp)
    fixture = temp / 'grants.cpp'
    fixture.write_text(preamble + production + checks)
    binary = temp / 'grants'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-O0',
        '-I', str(ROOT / 'combo'), '-I', str(ROOT / 'combo/menu'), '-I', str(ROOT / 'soh/soh/FleetShipCombo'),
        '-I', str(args.json_include), str(fixture), '-o', str(binary)], check=True)
    raise SystemExit(subprocess.run([str(binary)]).returncode)
