#!/usr/bin/env python3
"""Execute each save editor's Mirror button with production travel ownership."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_mm_scene_randomization_tests import function

for host, editor in [('soh', 'soh/Enhancements/debugger/debugSaveEditor.cpp'),
                     ('mm', '2s2h/DeveloperTools/SaveEditor.cpp')]:
    source = (ROOT / host / editor).read_text()
    assert 'static void DrawMagicMirrorEditor(' in source, host + ': dedicated Magic Mirror entry missing'
    ui = function(source, 'DrawMagicMirrorEditor')
    inv = (ROOT / host / 'mods/extended_inventory.c').read_text()
    backend = '\n'.join(function(inv, name) for name in [
        'TravelTools_Heal', 'TravelTools_IsOwned', 'TravelTools_Grant', 'MagicMirror_IsOwned'])
    fixture = r'''
#include "global.h"
#include "mods/extended_inventory.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <initializer_list>
SaveContext gSaveContext{};
static NeiSaveData saved{};
extern "C" NeiSaveData* Nei_Save() { return &saved; }
extern "C" uint16_t Nei_GetOwnedItem(uint8_t slot) { return saved.ownedItems[slot - 24]; }
extern "C" void Nei_SetOwnedItem(uint8_t slot, uint16_t item) { saved.ownedItems[slot - 24] = item; }
extern "C" uint8_t Picto_IsOwned() { assert(false); return 0; }
extern "C" uint8_t PowerKeg_IsOwned() { assert(false); return 0; }
extern "C" void ItemGrantAudit_Begin(const char*, int, int, int) {}
extern "C" void ItemGrantAudit_End() {}
extern "C" int32_t TradeAdult_OwnedCount() { assert(false); return 0; }
extern "C" uint8_t TradeAdult_CellItem() { assert(false); return ITEM_NONE; }
uint8_t ExtInv_GetOotSlotItem(int) { assert(false); return ITEM_NONE; }
void ExtInv_SetOotSlotItem(int, uint8_t) { assert(false); }
static bool click = false;
namespace ImGui {
void SeparatorText(const char*) {}
void BeginDisabled(bool) {}
void EndDisabled() {}
bool Button(const char*) { return click; }
void SameLine() {}
void TextDisabled(const char*) {}
}
/* BACKEND */
/* EDITOR */
int main() {
 for (bool timeGate: {false, true}) {
  saved = {}; for (auto& item: saved.ownedItems) item = ITEM_NONE;
  ExtInv_SetSlotItem(SLOT_MAGIC_MIRROR - 1, 0xF123);
  ExtInv_SetSlotItem(SLOT_MAGIC_MIRROR + 1, 0x0234);
  if (timeGate) ExtInv_SetSlotItem(SLOT_MAGIC_MIRROR, ITEM_TIME_GATE); // Legacy ownership in cell only.
  const auto nativeSave = gSaveContext;
  click = false; DrawMagicMirrorEditor(); assert(!MagicMirror_IsOwned());
  click = true; DrawMagicMirrorEditor();
  assert(MagicMirror_IsOwned() && saved.magicMirrorOwned);
  assert(bool(TravelTools_IsOwned(ITEM_TIME_GATE)) == timeGate);
  assert(ExtInv_GetSlotItem(SLOT_MAGIC_MIRROR) == (timeGate ? ITEM_TIME_GATE : EXT_ITEM_MAGIC_MIRROR));
  assert(ExtInv_GetSlotItem(SLOT_MAGIC_MIRROR - 1) == 0xF123);
  assert(ExtInv_GetSlotItem(SLOT_MAGIC_MIRROR + 1) == 0x0234);
  assert(std::memcmp(&nativeSave, &gSaveContext, sizeof(nativeSave)) == 0);
  const auto alreadyOwned = saved;
  DrawMagicMirrorEditor(); assert(std::memcmp(&alreadyOwned, &saved, sizeof(saved)) == 0);
 }
 puts("PASS dedicated Mirror editor button: grant, owned no-op, legacy Time Gate, selected u16 cell and native save/equips preserved");
}
'''.replace('/* BACKEND */', backend).replace('/* EDITOR */', ui)
    with tempfile.TemporaryDirectory(prefix='mirror-editor-' + host) as temp:
        path = Path(temp); (path / 'test.cpp').write_text(fixture)
        include = [ROOT, ROOT / host, ROOT / host / 'include', ROOT / host / 'include/PR',
                   ROOT / host / 'src', ROOT / host / 'assets', ROOT / 'combo',
                   ROOT / host / ('soh' if host == 'soh' else '2s2h'), ROOT / 'libultraship/include']
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-w', '-DF3DEX_GBI_2', '-DCOMBO_BUILD',
                        '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections',
                        *['-I' + str(p) for p in include], str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
        subprocess.run([str(path / 'test')], check=True)
