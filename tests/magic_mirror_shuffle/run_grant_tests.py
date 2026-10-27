"""Run actual RG/RI switch arms, independent travel grant and page gate on native save structs."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_mm_scene_randomization_tests import function
for host in ['soh', 'mm']:
    inv = (ROOT / host / 'mods/extended_inventory.c').read_text()
    native_path = 'soh/soh/Enhancements/randomizer/randomizer.cpp' if host == 'soh' else 'mm/2s2h/Rando/GiveItem.cpp'
    native = (ROOT / native_path).read_text()
    identity = 'RG_MAGIC_MIRROR' if host == 'soh' else 'RI_OOT_NEI_MAGIC_MIRROR'
    start = native.index('        case ' + identity + ':')
    end = native.index('            break;', start) + len('            break;')
    arm = native[start:end]
    production = '\n'.join(function(inv, name) for name in [
        'TravelTools_Heal', 'TravelTools_IsOwned', 'TravelTools_Grant', 'MagicMirror_IsOwned',
        'ExtInv_IsCustomItemsEnabled', 'ExtInv_GetMaxPages'])
    fixture = r'''
#include "global.h"
#include "mods/extended_inventory.h"
#include "tests/test_require.h"
#include <cstring>
#include <initializer_list>
/* ENUMS */
SaveContext gSaveContext{};
static NeiSaveData saved{};
static bool broad = false;
extern "C" NeiSaveData* Nei_Save() { return &saved; }
extern "C" uint16_t Nei_GetOwnedItem(uint8_t slot) { return saved.ownedItems[slot - 24]; }
extern "C" void Nei_SetOwnedItem(uint8_t slot, uint16_t item) { saved.ownedItems[slot - 24] = item; }
extern "C" uint8_t Picto_IsOwned() { return 0; }
extern "C" uint8_t PowerKeg_IsOwned() { return 0; }
extern "C" void ItemGrantAudit_Begin(const char*, int, int, int) {}
extern "C" void ItemGrantAudit_End() {}
extern "C" int32_t TradeAdult_OwnedCount() { return 0; }
extern "C" uint8_t TradeAdult_CellItem() { return ITEM_NONE; }
uint8_t ExtInv_GetOotSlotItem(int) { return ITEM_NONE; }
void ExtInv_SetOotSlotItem(int, uint8_t) {}
extern "C" int32_t CVarGetInteger(const char*, int32_t) { return broad; }
bool ExtInv_IsMmMasksEnabled() { return false; }
/* PRODUCTION */
void NativeGrant(int item) { switch (item) { /* ARM */ default: REQUIRE(false); } }
int main() {
 for (bool gate : {false, true}) {
  saved = {}; for (auto& item : saved.ownedItems) item = ITEM_NONE;
  if (gate) TravelTools_Grant(ITEM_TIME_GATE);
  const auto native = gSaveContext;
  REQUIRE(!ExtInv_IsCustomItemsEnabled() && ExtInv_GetMaxPages() == 1);
  NativeGrant(/* IDENTITY */);
  REQUIRE(MagicMirror_IsOwned() && saved.magicMirrorOwned);
  REQUIRE(bool(TravelTools_IsOwned(ITEM_TIME_GATE)) == gate);
  REQUIRE(ExtInv_GetSlotItem(SLOT_MAGIC_MIRROR) == (gate ? ITEM_TIME_GATE : EXT_ITEM_MAGIC_MIRROR));
  REQUIRE(ExtInv_IsCustomItemsEnabled() && ExtInv_GetMaxPages() == 2);
  REQUIRE(!broad);
  for (int slot = 24; slot < 72; ++slot)
   if (slot != SLOT_MAGIC_MIRROR) REQUIRE(saved.ownedItems[slot - 24] == ITEM_NONE);
  REQUIRE(std::memcmp(&native, &gSaveContext, sizeof(native)) == 0);
  const auto once = saved;
  NativeGrant(/* IDENTITY */);
  REQUIRE(std::memcmp(&once, &saved, sizeof(saved)) == 0);
 }
 puts("PASS native rando grant: Mirror independent of Time Gate/NEI, page accessible, no unrelated ownership/save writes");
}
'''.replace('/* ENUMS */', '#include "soh/Enhancements/randomizer/randomizerEnums.h"' if host == 'soh' else '#include "2s2h/Rando/Types.h"').replace('/* PRODUCTION */', production).replace('/* ARM */', arm).replace('/* IDENTITY */', identity)
    with tempfile.TemporaryDirectory(prefix='mirror-shuffle-grant-' + host) as td:
        temp = Path(td); (temp / 'test.cpp').write_text(fixture)
        flags = ['-std=c++20', '-w', '-DF3DEX_GBI_2', '-DCOMBO_BUILD', '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections']
        for p in ['', host, host+'/include', host+'/include/PR', host+'/src', host+'/assets', 'combo', host+('/soh' if host=='soh' else '/2s2h'), 'libultraship/include']:
            flags += ['-I' + str(ROOT / p)]
        result = subprocess.run([os.environ.get('CXX', 'c++'), *flags, str(temp / 'test.cpp'), '-o', str(temp / 'test')], capture_output=True, text=True)
        if result.returncode: raise RuntimeError(host + '\n' + result.stdout + result.stderr)
        subprocess.run([str(temp / 'test')], check=True)
