#!/usr/bin/env python3
"""Execute the pause item-age macro with native IDs, table, NEI registry and mask ages.

Only the GameInteractor dispatch boundary is a fixture. The macro, the native
age table, custom registry age rows/lookup and ExtInv_GetItemAgeReq are read from
production. A routing regression must fail both unrestricted child items and
child-only items queried as adult, even though zero-filled native entries look
like valid adult requirements.
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    match = re.search(r"^[^\n;{}]*\b" + name + r"\([^;{}]*\)\s*\{", source, re.M)
    assert match, name
    end, depth = source.index("{", match.start()) + 1, 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


def fixture():
    header = (ROOT / "soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_scope.h").read_text()
    pause = (ROOT / "soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_scope_PAL.c").read_text()
    inventory = (ROOT / "soh/mods/extended_inventory.c").read_text()
    registry = (ROOT / "soh/mods/extended_player.c").read_text()
    save = (ROOT / "soh/include/z64save.h").read_text()
    equipment = (ROOT / "soh/mods/extended_equipment.h").read_text()
    equipment_ids = "\n".join(re.findall(r"^#define ITEM_EXT_\w+[^\n]*", equipment, re.M))
    # Keep the actual first five registry fields' age data; unrelated model,
    # texture, animation and message columns do not participate in age lookup.
    rows = re.findall(r"^\s*\{\s*(ITEM_\w+),\s*[^,]+,\s*[^,]+,\s*[^,]+,\s*(AGE_REQ_\w+),",
                      registry[registry.index("static const NeiItem sNeiItems[]"):registry.index("#define NEI_ITEMS_COUNT")], re.M)
    assert len(rows) > 50, "production registry projection is incomplete"
    ages = re.search(r"typedef enum \{\s*/\* 0 \*/ LINK_AGE_ADULT,.*?\} LinkAge;", save, re.S)[0]
    native_table = re.search(r"u8 gItemAgeReqs\[ITEM_NONE\] = \{.*?\n\};", pause, re.S)[0]
    masks = "\n".join(re.search(r"const uint8_t " + name + r"\[24\] = \{.*?\n\};", inventory, re.S)[0]
                      for name in ("gPage3MaskItems", "gPage3MaskAgeReqs"))
    macros = header[header.index("#define AGE_REQ_ADULT"):header.index("void KaleidoScope_DrawQuestStatus")]
    return r'''
#include <cstdint>
#include <cstdio>
#include <cstddef>
#include <initializer_list>
#include "soh/include/z64item.h"
#include "soh/soh/Enhancements/game-interactor/vanilla-behavior/GIVanillaBehavior.h"
using u8 = uint8_t;
''' + ages + '\n' + equipment_ids + '\n' + macros + '\n' + native_table + '\n' + masks + r'''
struct SaveContext { int linkAge; } gSaveContext;
struct NeiItem { int16_t item; uint8_t ageReq; };
#define NEI_NO_ITEM ((int16_t)-1)
static const NeiItem sNeiItems[] = {
''' + '\n'.join(f'    {{ {item}, {age} }},' for item, age in rows) + r'''
};
#define NEI_ITEMS_COUNT (sizeof(sNeiItems) / sizeof(sNeiItems[0]))
''' + function(registry, "Nei_FindByItem") + '\n' + function(inventory, "ExtInv_GetItemAgeReq") + r'''
static int overrideResult = -1, lastItem, hookCalls;
static GIVanillaBehavior lastBehavior;
static bool GameInteractor_Should(GIVanillaBehavior behavior, bool normal, int item) {
    ++hookCalls; lastBehavior = behavior; lastItem = item;
    return overrideResult < 0 ? normal : overrideResult;
}
static int checks, failures;
static void check(bool value, const char* label) {
    ++checks;
    if (!value) { ++failures; std::fprintf(stderr, "FAIL: %s\n", label); }
}
static void ageCase(int item, bool child, bool adult, const char* name) {
    for (LinkAge age : {LINK_AGE_CHILD, LINK_AGE_ADULT}) {
        gSaveContext.linkAge = age;
        const bool allowed = CHECK_AGE_REQ_ITEM(item);
        check(allowed == (age == LINK_AGE_CHILD ? child : adult), name);
        check(lastBehavior == VB_ITEM_MEETS_AGE_REQ && lastItem == item, "hook receives exact item ID");
    }
}
int main() {
    // Play_Init's native-feather entry assignment is retained in production.
    gItemAgeReqs[ITEM_ROCS_FEATHER] = AGE_REQ_NONE;
    ageCase(ITEM_BOW, false, true, "native bow remains adult-only");
    ageCase(ITEM_STICK, true, false, "native stick remains child-only");
    ageCase(ITEM_BOTTLE, true, true, "native bottle works at both ages");
    ageCase(ITEM_SCALE_GOLDEN, true, true, "native golden scale works at both ages");
    ageCase(ITEM_ROCS_FEATHER, true, true, "native 9.3 feather uses Play_Init age table");
    ageCase(ITEM_ROCS_FEATHER_SKIJER, true, true, "custom Roc feather works at both ages");
    ageCase(ITEM_ROCS_CAPE, true, true, "custom Roc cape works at both ages");
    ageCase(ITEM_HYLIAS_GRACE, true, true, "Hylia's Grace works at both ages");
    ageCase(ITEM_DEKU_LEAF, true, false, "NEI registry preserves child-only Deku Leaf");
    ageCase(ITEM_BALL_AND_CHAIN, false, true, "NEI registry preserves adult-only Ball and Chain");
    ageCase(ITEM_MM_MASK_POSTMAN, true, true, "MM regular mask works at both ages");
    ageCase(ITEM_MM_MASK_DEKU, true, false, "MM Deku mask retains child requirement");
    ageCase(ITEM_MM_MASK_GORON, true, false, "MM Goron mask retains child requirement");
    ageCase(ITEM_MM_MASK_ZORA, true, false, "MM Zora mask retains child requirement");
    ageCase(ITEM_MM_MASK_FIERCE_DEITY, true, false, "MM Fierce Deity mask retains child requirement");
    ageCase(ITEM_RITO_MASK, true, true, "Rito shared-slot cycle remains usable as child");
    ageCase(ITEM_EXT_SWORD_1, true, true, "extended equipment ID uses established NEI fallback");
    ageCase(ITEM_NET, true, true, "Net works at both ages");
    ageCase(ITEM_CUSTOM, true, true, "custom message sentinel/Gyorg alias keeps NEI fallback");
    ageCase(ITEM_NONE, true, true, "empty sentinel avoids native table out-of-bounds");

    // A different native entry is visible through the macro, proving the
    // 9.3 feather does not accidentally use the permissive custom fallback.
    gItemAgeReqs[ITEM_ROCS_FEATHER] = AGE_REQ_CHILD;
    ageCase(ITEM_ROCS_FEATHER, true, false, "native feather stays on table route");

    overrideResult = 1;
    ageCase(ITEM_DEKU_LEAF, true, true, "9.3 hook can allow an age-restricted custom item");
    overrideResult = 0;
    ageCase(ITEM_ROCS_FEATHER_SKIJER, false, false, "9.3 hook can refuse an unrestricted custom item");
    check(hookCalls == 46, "all item queries pass through the 9.3 behavior hook");
    std::printf("OoT pause item age routing: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
'''


def main():
    with tempfile.TemporaryDirectory(prefix="oot-pause-age-") as directory:
        folder = Path(directory)
        source = folder / "age.cpp"
        binary = folder / "age"
        source.write_text(fixture())
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-I" + str(ROOT),
                        "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-g",
                        str(source), "-o", str(binary)], check=True)
        env = os.environ.copy()
        # LeakSanitizer's ptrace probe is unavailable in the test host; keep
        # AddressSanitizer and UndefinedBehaviorSanitizer for table bounds.
        env["ASAN_OPTIONS"] = env.get("ASAN_OPTIONS", "") + ":detect_leaks=0"
        subprocess.run([str(binary)], check=True, env=env)


if __name__ == "__main__":
    main()
