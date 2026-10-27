#!/usr/bin/env python3
"""Check real item registry/action routing with neutral engine calls instrumented."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_scene_randomization_tests import function

for host in ("mm", "soh"):
    player = (ROOT / host / "mods/extended_player.c").read_text()
    # Keep actual Mirror and Wand descriptor rows in their original order. Other
    # registry families and the wand's actual spell implementation are separate.
    rows = re.findall(r'    \{ (?:EXT_ITEM_MAGIC_MIRROR|ITEM_ELEMENTAL_WAND),.*?\},', player, re.S)
    prefix = r'''
#include "global.h"
#include "mods/extended_inventory.h"
#include "mods/extended_player.h"
#include "mods/extended_equipment.h"
#include "mods/items/custom_items.h"
static const char gItemIconElementalWandTex[] = "wand";
#include <cassert>
#include <cstdio>
#include <cstring>
#ifndef EXT_ITEM_MAGIC_MIRROR
#define EXT_ITEM_MAGIC_MIRROR 0x0224
#define SLOT_MAGIC_MIRROR 31
#endif
static int genericUpdates, genericInits, wandUpdates, wandInits;
SaveContext gSaveContext{};
int8_t sItemActions[256]{};
int8_t sItemItemActions[256]{};
uint8_t sActionModelGroups[256]{};
s32 (*sItemActionUpdateFuncs[256])(Player*, PlayState*){};
void (*sItemActionInitFuncs[256])(PlayState*, Player*){};
extern "C" s32 func_8083485C(Player*, PlayState*) { ++genericUpdates; return 17; }
extern "C" void Player_InitDefaultIA(PlayState*, Player*) { ++genericInits; }
extern "C" s32 Player_UpperAction_ElementalWand(Player*, PlayState*) { ++wandUpdates; return 23; }
extern "C" void Player_InitElementalWandIA(PlayState*, Player*) { ++wandInits; }
void Randomizer_DrawElementalWand(PlayState*, GetItemEntry*) {}
extern "C" int32_t CVarGetInteger(const char*, int32_t fallback) { return fallback; }
uint8_t Sw97_EffectiveElement(uint8_t) { return 0; }
static s32 Sw97_PreferBow() { return 1; }
'''
    if host == "mm":
        prefix += r'''
#include "mods/nei_oot_compat.h"
s32 Player_UpperAction_6(Player*, PlayState*) { return 0; }
s32 Player_UpperAction_OotBoomerang(Player*, PlayState*) { return 0; }
void Player_InitSlingshotIA(PlayState*, Player*) {}
void Player_InitOotBoomerangIA(PlayState*, Player*) {}
'''
    prefix += "\n#define RG_ELEMENTAL_WAND 1\n"
    prefix += "#define EXPECTED_MIRROR_RG " + ("RI_OOT_NEI_MAGIC_MIRROR" if host == "mm" else "RG_MAGIC_MIRROR") + "\n"
    production = "\n".join(function(player, name) for name in ("Nei_SharedIA_UpperAction", "Nei_SharedIA_Init") if name + "(" in player)
    production += "\nstatic const NeiItem sNeiItems[] = {\n" + "\n".join(rows) + "\n};\n#define NEI_ITEMS_COUNT (sizeof(sNeiItems)/sizeof(sNeiItems[0]))\n"
    production += "\n".join(function(player, name) for name in ("Nei_FindByItem", "ExtPlayer_GetItemAction", "ExtPlayer_FindByIA", "ExtPlayer_GetItemActionUpdateFunc", "ExtPlayer_GetItemActionInitFunc"))
    checks = r'''
int main() {
    const auto* mirror = Nei_FindByItem(EXT_ITEM_MAGIC_MIRROR);
    assert(mirror && "Magic Mirror missing from production registry");
    assert(mirror->slot == SLOT_MAGIC_MIRROR && mirror->rg == EXPECTED_MIRROR_RG && !mirror->drawFunc);
    assert(mirror->ageReq == AGE_REQ_NONE && mirror->modelGroup == PLAYER_MODELGROUP_DEFAULT);
    assert(mirror->ia == PLAYER_IA_ELEMENTAL_WAND && mirror->ia <= 127);
    assert(std::strcmp(static_cast<const char*>(mirror->iconTex), "__OTR__textures/icon_item_custom/gItemIconMagicMirrorTex") == 0);
    assert(ExtPlayer_GetItemAction(EXT_ITEM_MAGIC_MIRROR) == PLAYER_IA_ELEMENTAL_WAND);
    assert(ExtPlayer_GetItemAction(ITEM_ELEMENTAL_WAND) == PLAYER_IA_ELEMENTAL_WAND);
    PlayState play{}; Player p{};
    p.heldItemId = ITEM_NONE; // EXT IDs live in ExtButton's u16 backing store.
    auto update = ExtPlayer_GetItemActionUpdateFunc(PLAYER_IA_ELEMENTAL_WAND);
    auto init = ExtPlayer_GetItemActionInitFunc(PLAYER_IA_ELEMENTAL_WAND);
    assert(update(&p, &play) == 17);
    init(&play, &p);
    assert(genericUpdates == 1 && genericInits == 1 && !wandUpdates && !wandInits);
    p.heldItemId = ITEM_ELEMENTAL_WAND;
    assert(update(&p, &play) == 23);
    init(&play, &p);
    assert(wandUpdates == 1 && wandInits == 1 && genericUpdates == 1 && genericInits == 1);
    puts("PASS travel actions: real descriptor and native shuffle identity, signed action safe, neutral Mirror and unchanged Wand dispatch");
}
'''
    with tempfile.TemporaryDirectory(prefix="mirror-action-" + host + "-") as td:
        path=Path(td); cpp=path/"test.cpp"; binary=path/"test"
        cpp.write_text(prefix + re.sub(r"\bthis\b", "self", production) + checks)
        flags = ["-std=c++20", "-w", "-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DCONTROLLERBUTTONS_T=uint32_t"]
        for cmake in ("CMake/soh-cvars.cmake", "CMake/lus-cvars.cmake"):
            for key,value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)', (ROOT/cmake).read_text()):
                flags.append(f'-D{key}="{value}"')
        includes=["-I"+str(ROOT/p) for p in (host,host+"/include",host+"/include/PR",host+"/src",host+"/assets",host+"/2s2h","libultraship/include","combo","combo/menu")]
        subprocess.run([os.environ.get("CXX","c++"),*flags,*includes,str(cpp),"-o",str(binary)],check=True)
        subprocess.run([str(binary)],check=True)
        print("PASS action host:",host,flush=True)
