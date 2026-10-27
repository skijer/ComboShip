#!/usr/bin/env python3
"""Compile the actual OoT transformed-form slot gate, preserving Time Gate restrictions."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "soh/mods/transformation_masks/mm_player_form.cpp").read_text()
start = source.index("static u8 MmForm_IsSlotAllowedInternal(")
end = source.index("{", start) + 1
depth = 1
while depth:
    depth += (source[end] == "{") - (source[end] == "}")
    end += 1
body = source[start:end]
fixture = r'''
#include "combo/gameplay/MagicMirrorItem.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
using u8 = uint8_t;
enum { MMFORM_STATE_ACTIVE=1, MMFORM_STATE_TRANSFORMING=2, MMFORM_STATE_DETRANSFORMING=3,
       MM_PLAYER_FORM_HUMAN=4 };
struct { u8 state, currentForm; } gFormState;
static u8 restricted[72];
static const u8* sFormSlotAllowed[4] = {restricted, restricted, restricted, restricted};
static uint16_t selected;
uint16_t ExtInv_GetSlotItem(int) { return selected; }
/* PRODUCTION */
int main() {
  gFormState.state = MMFORM_STATE_ACTIVE;
  for(int form=0; form<4; ++form) {
    gFormState.currentForm = form;
    selected = EXT_ITEM_MAGIC_MIRROR;
    assert(MmForm_IsSlotAllowedInternal(SLOT_MAGIC_MIRROR));
    selected = 0xAC;
    assert(!MmForm_IsSlotAllowedInternal(SLOT_MAGIC_MIRROR));
    selected = EXT_ITEM_MAGIC_MIRROR;
    assert(!MmForm_IsSlotAllowedInternal(SLOT_MAGIC_MIRROR+1));
  }
  puts("PASS native transformed slot gate: all four forms can equip Mirror; Time Gate and adjacent restrictions preserved");
}
'''.replace("/* PRODUCTION */", body)
with tempfile.TemporaryDirectory(prefix="mirror-forms-") as folder:
    path = Path(folder)
    (path / "test.cpp").write_text(fixture)
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    "-I" + str(ROOT), str(path / "test.cpp"), "-o", str(path / "test")], check=True)
    subprocess.run([str(path / "test")], check=True)
