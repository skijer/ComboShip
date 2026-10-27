#!/usr/bin/env python3
"""Headless provenance checks; no game boot, seed build, or user-save mutation.

Compile actual capture functions against extracted production ownership structures.
Other SaveContext members are modeled: this is not a complete game build.
"""
import os
import hashlib
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
CXX = os.environ.get('CXX', 'c++')
CC = os.environ.get('CC', 'cc')


def read(path):
    return (ROOT / path).read_text(encoding='utf-8-sig')


def comments(text):
    return re.sub(r'/\*.*?\*/|//[^\n]*', '', text, flags=re.S)


def declaration(text, name):
    clean = comments(text)
    pattern = r'typedef (?:struct|union|enum)(?:\s+\w+)?\s*\{[^{}]*\}\s*' + name + r'\s*;'
    matches = re.findall(pattern, clean, re.S)
    assert len(matches) == 1, name
    return matches[0]


def function(text, signature):
    start = text.index(signature)
    brace = text.index('{', start)
    # Captures below contain no braces in string literals/comments.
    depth = 1
    end = brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


with tempfile.TemporaryDirectory(prefix='item_grant_audit_') as td:
    td = Path(td)
    flags = ['-std=c++17', '-Wall', '-Wextra', '-Werror', '-O1', '-I', str(ROOT / 'combo/menu')]
    sanitizer = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie']
    env = {**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'}

    def run_cpp(name, text):
        path = td / (name + '.cpp')
        path.write_text(text)
        binary = td / name
        subprocess.run([CXX, *flags, *sanitizer, str(path), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env=env)

    run_cpp('tracker', read('tests/item_grant_audit/tracker_test.cpp'))
    for game in ['soh', 'mm']:
        zsave = read(f'{game}/include/z64save.h')
        nei = declaration(read(f'{game}/mods/nei_save.h'), 'NeiSaveData')
        inv = declaration(zsave, 'Inventory')
        src = read('soh/soh/FleetShipCombo/FleetSync.cpp' if game == 'soh'
                   else 'mm/2s2h/FleetShipCombo/FleetSync.cpp')
        capture = function(src, 'static ItemGrantAudit::Snapshot CaptureItemGrantAudit()')
        preamble = '''
#include "ItemGrantAudit.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <algorithm>
#include "combo/rando/RpgStats.h"
using u8=uint8_t; using u16=uint16_t; using u32=uint32_t;
using s8=int8_t; using s16=int16_t;
#define FC_COMBO_OBTAINED_FC_SIZE 512
'''
        if game == 'soh':
            inf = read('soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerInf.h')
            preamble += '#define RANDO_ENUM_BEGIN(n) typedef enum {\n#define RANDO_ENUM_ITEM(n, ...) n,\n#define RANDO_ENUM_END(n) } n;\n'
            preamble += '#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerInf.h"\n'
            preamble += declaration(zsave, 'ShipRandomizerSaveContextData') + '\n'
        else:
            types = read('mm/2s2h/Rando/Types.h')
            preamble += declaration(types, 'RandoCheckId') + '\n'
            preamble += declaration(types, 'RandoInf') + '\n'
            preamble += declaration(zsave, 'SavePlayerData') + '\n'
            # The check shape is production; item identity isn't read by the observer.
            preamble += 'using RandoItemId=int;\n' + declaration(zsave, 'RandoSaveCheck') + '\n'
        preamble += nei + '\n' + inv + '\n'
        if game == 'soh':
            preamble += '''
struct {
    int fileNum=0, gameMode=0, healthCapacity=48;
    u8 isMagicAcquired=0, isDoubleMagicAcquired=0, isDoubleDefenseAcquired=0, bgsFlag=0;
    Inventory inventory{};
    struct {
        struct { int id=4; struct { ShipRandomizerSaveContextData randomizer{}; } data; } quest;
        u16 randomizerInf[(RAND_INF_MAX+15)/16]{};
    } ship;
} gSaveContext;
#define IS_RANDO (gSaveContext.ship.quest.id == 4)
NeiSaveData custom{};
NeiSaveData* Nei_Save() { return &custom; }
'''
        else:
            preamble += '''
struct {
    int fileNum=0, gameMode=0;
    struct {
        struct {
            Inventory inventory{}; SavePlayerData playerData{};
            struct { u16 equipment=0; } equips;
            u32 skullTokenCount=0;
        } saveInfo;
        struct {
            int saveType=1;
            struct {
                u32 finalSeed=123;
                s8 foundDungeonKeys[9]{};
                u16 randoInf[(RANDO_INF_MAX+15)/16]{};
                u16 foundTriforcePieces=0;
                RandoSaveCheck randoSaveChecks[RC_MAX]{};
            } rando;
            NeiSaveData nei{};
        } shipSaveInfo;
    } save;
} gSaveContext;
NeiSaveData* Nei_Save() { return &gSaveContext.save.shipSaveInfo.nei; }
'''
        test = '''
int main() {
    auto* custom = Nei_Save();
    std::fill(std::begin(custom->ownedItems), std::end(custom->ownedItems), 0xFF);
    custom->caneSkills=3; custom->capeOwned=1; custom->comboObtainedFc[511]=1;
    auto saveBefore = gSaveContext;
    auto neiBefore = *custom;
    auto s = CaptureItemGrantAudit();
    assert(!s.overflow && s.size > 1200 && s.size <= 2048);
    assert(std::memcmp(&saveBefore, &gSaveContext, sizeof(saveBefore)) == 0);
    assert(std::memcmp(&neiBefore, custom, sizeof(neiBefore)) == 0);
    bool cape=false, cane=false, lastFc=false;
    for(size_t i=0;i<s.size;++i) {
        auto& f=s.fields[i];
        if(std::string(f.name)=="nei.capeOwned") cape=f.value==1;
        if(std::string(f.name)=="nei.caneSkills") cane=f.value==3;
        if(std::string(f.name)=="nei.comboObtainedFc" && f.index==511) lastFc=f.value==1;
    }
    assert(cape && cane && lastFc);
    std::cout << "production capture: fields=" << s.size << " passive/complete registry PASS\\n";
}
'''
        path = td / (game + '.cpp')
        path.write_text(preamble + capture + test)
        binary = td / game
        subprocess.run([CXX, *flags, *sanitizer, '-I', str(ROOT), str(path), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env=env)

    # Execute the production dungeon/spell cases. These are local MM effects;
    # cross-game identity recording happens before the switch and is untouched.
    grant_src = read('mm/2s2h/Rando/GiveItem.cpp')
    dungeon_start = grant_src.index('        case RI_OOT_BOSS_KEY_FIRE_TEMPLE:')
    spell_start = grant_src.index('        case RI_OOT_DINS_FIRE:', dungeon_start)
    gear_start = grant_src.index('        case RI_OOT_BOOMERANG:', spell_start)
    local_cases = grant_src[dungeon_start:gear_start]
    dungeon_ids = re.findall(r'case (RI_\w+):', grant_src[dungeon_start:spell_start])
    assert len(dungeon_ids) == 46, 'Update coverage for added dungeon items'
    moon_start = grant_src.index('        case RI_MOONS_TEAR:')
    moon_end = grant_src.index('        case RI_DEED_LAND:', moon_start)
    moon_case = grant_src[moon_start:moon_end]
    types = read('mm/2s2h/Rando/Types.h')
    fixture = '#include <cassert>\n#include <iostream>\n'
    fixture += declaration(types, 'RandoItemId') + '\n'
    fixture += declaration(types, 'RandoInf') + '\n'
    fixture += '''
struct { unsigned ootSpellsOwned; } nei;
auto* Nei_Save() { return &nei; }
bool moonOwned=false;
unsigned nativeGives=0;
void Flags_SetRandoInf(RandoInf flag) {
    assert(flag == RANDO_INF_OBTAINED_MOONS_TEAR);
    moonOwned=true;
}
void* gPlayState=nullptr;
void Item_Give(void*, int item) { assert(item == 0x28); ++nativeGives; }
namespace Rando::StaticData { struct Entry { int itemId; }; Entry Items[RI_MAX]{}; }
void Grant(RandoItemId randoItemId) {
    switch(randoItemId) {
'''
    fixture += local_cases + moon_case + 'default: assert(false);\n}\n}\n'
    fixture += 'int main() {\nconst RandoItemId dungeonItems[] = {' + ','.join(dungeon_ids) + '};\n'
    fixture += '''
    Rando::StaticData::Items[RI_MOONS_TEAR].itemId=0x28;
    for (unsigned spells=0; spells<8; ++spells) {
        for (bool moon : {false, true}) {
            for (auto item : dungeonItems) {
                nei.ootSpellsOwned=spells; moonOwned=moon; nativeGives=0;
                Grant(item);
                assert(nei.ootSpellsOwned == spells);
                assert(moonOwned == moon && nativeGives == 0);
            }
            nei.ootSpellsOwned=spells; moonOwned=moon; nativeGives=0;
            Grant(RI_MOONS_TEAR);
            assert(nei.ootSpellsOwned == spells && moonOwned && nativeGives == 1);
            const RandoItemId spellsToGive[] = {RI_OOT_DINS_FIRE, RI_OOT_FARORES_WIND, RI_OOT_NAYRUS_LOVE};
            for (unsigned i=0; i<3; ++i) {
                nei.ootSpellsOwned=spells; moonOwned=moon; nativeGives=0;
                Grant(spellsToGive[i]);
                assert(nei.ootSpellsOwned == (spells | (1u << i)));
                assert(moonOwned == moon && nativeGives == 0);
            }
        }
    }
    std::cout << "MM production grant cases: 46 dungeon items, Moon's Tear and all spells PASS\\n";
}
'''
    run_cpp('mm_dungeon_spell_grants', fixture)

    # The launcher can still consider the MM slot resident during owl-save quit.
    # Exercise the actual tier reader with the reset (zero-filled) save, as well
    # as valid dormant/file-select saves; gameMode alone cannot validate a save.
    port = read('mm/2s2h/BenPort.cpp')
    tiers = function(port, 'extern "C" COMBO_EXPORT int MM_GetSharedTier(int family) try {')
    raise_start = port.index('extern "C" COMBO_EXPORT void MM_RaiseSharedTier(int family, int tier) try {')
    raise_prefix = port[raise_start:port.index('    const auto& def', raise_start)]
    fixture = '''
#include <cassert>
#include <iostream>
#include "ItemGrantAuditBridge.h"
namespace ComboRando {
'''
    shared = comments(read('combo/rando/SharedItems.h'))
    fixture += re.search(r'enum SharedFamily[^}]+};', shared, re.S)[0] + '\n}\n'
    fixture += declaration(read('mm/include/z64item.h'), 'ItemId') + '\n'
    fixture += '''
#define COMBO_EXPORT
#define IS_RANDO (gSaveContext.save.shipSaveInfo.saveType == SAVETYPE_RANDO)
constexpr int SAVETYPE_RANDO=1;
int inventory[256]{};
#define INV_CONTENT(item) inventory[item]
#define CUR_UPG_VALUE(upg) 0
#define CHECK_QUEST_ITEM(quest) 0
struct { struct {
    struct { struct { int isMagicAcquired=0, isDoubleMagicAcquired=0; } playerData; } saveInfo;
    struct { int saveType=0; } shipSaveInfo;
} save; } gSaveContext;
'''
    fixture += '''
constexpr int COMBO_RPG_MAGIC=3;
bool rpgMagicEnabled=false;
struct { struct { int nativeMagicLevel=0; } comboRpg; } nei;
auto Nei_Save() { return &nei; }
int ComboRpg_IsEnabled(int) { return rpgMagicEnabled; }
'''
    fixture += function(read('mm/mods/combo_rpg.cpp'), 'extern "C" int ComboRpg_NativeMagicTier(int nativeTier) {') + '\n'
    fixture += tiers + ' catch (...) { assert(false); return 0; }\n'
    # Model downstream grant services, after the actual production entry guard.
    fixture += 'int downstreamGrants=0;\n' + raise_prefix
    fixture += '(void)tier; ++downstreamGrants; } catch (...) { assert(false); }\n'
    fixture += '''
int main() {
    for(int family=0; family<ComboRando::SF_COUNT; ++family) {
        assert(MM_GetSharedTier(family) == 0);
        MM_RaiseSharedTier(family, 1);
    }
    assert(downstreamGrants == 0);
    // A loaded dormant save is valid even when no PlayState exists.
    gSaveContext.save.shipSaveInfo.saveType=SAVETYPE_RANDO;
    MM_RaiseSharedTier(ComboRando::SF_LIGHT_ARROWS, 1);
    assert(downstreamGrants == 1);
    const int families[] = {ComboRando::SF_FIRE_ARROWS, ComboRando::SF_ICE_ARROWS, ComboRando::SF_LIGHT_ARROWS};
    const int items[] = {ITEM_ARROW_FIRE, ITEM_ARROW_ICE, ITEM_ARROW_LIGHT};
    for(unsigned i=0; i<3; ++i) {
        for(int value : {0, static_cast<int>(ITEM_NONE), items[i]}) {
            inventory[items[i]]=value;
            assert(MM_GetSharedTier(families[i]) == (value == items[i]));
        }
    }
    auto& player = gSaveContext.save.saveInfo.playerData;
    player.isMagicAcquired = player.isDoubleMagicAcquired = 1;
    assert(MM_GetSharedTier(ComboRando::SF_MAGIC) == 2);
    rpgMagicEnabled = true;
    assert(MM_GetSharedTier(ComboRando::SF_MAGIC) == 0);
    nei.comboRpg.nativeMagicLevel = 1;
    assert(MM_GetSharedTier(ComboRando::SF_MAGIC) == 1);
    nei.comboRpg.nativeMagicLevel = 2;
    assert(MM_GetSharedTier(ComboRando::SF_MAGIC) == 2);
    std::cout << "MM shared tiers: cleared save rejected, dormant arrow ownership and native RPG magic exact PASS\\n";
}
'''
    run_cpp('mm_shared_tiers', fixture)

    # Verify C ABI and stock builds: stock bridge needs no linked observer.
    bridge = '''
#include "ItemGrantAuditBridge.h"
int main(void) {
    ItemGrantAudit_Begin("stock", 1, 2, 0);
    ItemGrantAudit_Checkpoint("stock");
    ItemGrantAudit_End();
    return 0;
}
'''
    path = td / 'stock.c'
    path.write_text(bridge)
    subprocess.run([CC, '-std=c11', '-Wall', '-Wextra', '-Werror', '-I', str(ROOT / 'combo/menu'),
                    str(path), '-o', str(td / 'stock')], check=True)
    run_cpp('stock_cpp', '#include "ItemGrantAuditBridge.h"\nint main(){ ItemGrantAudit::Scope s("stock"); }')
    # Original native give bodies remain byte-for-byte identical after renaming the wrapper.
    for game in ['soh', 'mm']:
        path = f'{game}/src/code/z_parameter.c'
        candidate = function(read(path), 'static u8 ItemGrantAudit_ItemGive(PlayState* play, u8 item) {')
        preserved_hash = {'soh': 'ccf744c9d538e4fbe619b1479369dbd6a158578944f35123f2c5ed19d697541f', 'mm': '6e67d423b5c2a26b4614dbd68081d51ef2a9fd44dc28cbf1ce895209c230d437'}
        assert hashlib.sha256(candidate[candidate.index('{'):].encode()).hexdigest() == preserved_hash[game], path
        wrapper = function(read(path), 'u8 Item_Give(PlayState* play, u8 item) {')
        assert 'ItemGrantAudit_Begin' in wrapper and 'ItemGrantAudit_End' in wrapper and 'return result;' in wrapper
        fixture = '''
#define COMBO_BUILD
#include "ItemGrantAuditBridge.h"
#include <assert.h>
typedef unsigned char u8;
typedef struct { int unused; } PlayState;
static int active, calls;
void ItemGrantAudit_Begin(const char* s, int item, int check, int quiet) {
    (void)s; (void)item; (void)check; (void)quiet; ++active;
}
void ItemGrantAudit_End(void) { --active; }
static u8 ItemGrantAudit_ItemGive(PlayState* play, u8 item) {
    (void)play; ++calls; return item;
}
'''
        fixture += wrapper + '''
int main(void) {
    for (int i=0; i<256; ++i) assert(Item_Give(0, (u8)i) == (u8)i);
    assert(active == 0 && calls == 256);
}
'''
        cpath = td / (game + '_bridge.c')
        cpath.write_text(fixture)
        subprocess.run([CC, '-std=c11', '-Wall', '-Wextra', '-Werror', '-I', str(ROOT / 'combo/menu'),
                        str(cpath), '-o', str(td / (game + '_bridge'))], check=True)
        subprocess.run([str(td / (game + '_bridge'))], check=True)
    print('C bridge, stock C/C++ and native grant preservation PASS')
