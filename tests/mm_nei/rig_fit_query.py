"""Compile the production rig-fit query into native item-drawer fixtures."""
from pathlib import Path
import re


def write_query(root: Path, destination: Path) -> None:
    source = (root / 'mm/mods/items/logic/adult_link_render.cpp').read_text()
    mask = re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"',
                  lambda match: ' ' * len(match[0]), source, flags=re.S)
    match = re.search(r'^extern\s+\s* s32 AdultLink_UsesAdultPresentation\([^;]*?\)\s*\{', mask, re.M)
    assert match
    pos = match.end()
    depth = 1
    while depth:
        depth += (mask[pos] == '{') - (mask[pos] == '}')
        pos += 1
    body = source[match.start():pos]
    destination.write_text('''#include "z64.h"
#include "variables.h"
#include "mods/forms/custom_forms.h"
static u8 sReady = 0, sIsChildRig = 0;
static FlexSkeletonHeader skeleton{}, *sSkel = nullptr;
static s32 mode = 0, form = CUSTOM_FORM_NONE;
extern "C" s32 AdultLink_IsActive(void) { return mode; }
extern "C" s32 CustomForms_ActiveForm(void) { return form; }
extern "C" void RigFit_Set(u8 ready, u8 child, s32 adult, s32 custom) {
    sReady = ready;
    sSkel = ready ? &skeleton : nullptr;
    sIsChildRig = child;
    mode = adult;
    form = custom;
}
''' + body + '\n')
