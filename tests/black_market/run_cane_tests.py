#!/usr/bin/env python3
"""Exercise actual exact-grant and both native FleetSync cane blocks."""
from pathlib import Path
import os, subprocess, sys, tempfile
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from
items = (ROOT / "mm/2s2h/Enhancements/Gameplay/BlackMarketItems.cpp").read_text()
grant = block_from(items, items.index("bool Grant("))
ids = ["RI_OOT_NEI_CANE_OF_SOMARIA", "RI_OOT_NEI_CANE_SOMARIA_BLOCK", "RI_OOT_NEI_CANE_SOMARIA_PLATFORM", "RI_OOT_NEI_CANE_PACCI_FLIP", "RI_OOT_NEI_CANE_PACCI_STONE", "RI_OOT_NEI_CANE_PACCI_ULTRAHAND", "RI_OOT_EXT_CANE_OF_BYRNA"]
fixture = r'''
#include "rando/BlackMarket.h"
#include <cassert>
#include <cstdio>
using u8 = unsigned char;
using uint8_t = unsigned char;
using Item = ComboRando::BlackMarket::Item;
struct State { u8 caneSkills = 0; bool cell = false; std::array<u8,2> comboObtainedFc{}, comboAppliedFc{}; } states[2];
State* nei = &states[0];
u8 Cane_GiveSkill(u8 skill) { nei->caneSkills |= 1 << skill; nei->cell = true; return 1; }
constexpr int SLATE_RUNE_COUNT=5, SEASON_COUNT=4, WAND_MODE_COUNT=6;
void Slate_GrantRune(u8) {} void Seasons_GrantRod() {} void Seasons_GrantSeason(u8) {} void Wand_GrantMode(u8) {}
/* IDS */
int Id(const Item& item) {
    static const char* ids[] = { /* NAMES */ };
    for (int i=0;i<7;++i) if (item.identity == std::string_view(ids[i])) return i;
    return -1;
}
bool Owned(const Item& item) { const int i=Id(item); return i>=0 && i<6 && (nei->caneSkills & (1<<i)); }
bool Available(const Item&) { return true; }
using NeiSaveData = State;
State* Nei_Save() { return nei; }
constexpr int FCI_CANE_OF_SOMARIA=0, FCI_MAX=2, FC_COMBO_OBTAINED_FC_SIZE=2, FCI_NO_ITEM=-1;
int FcCombo_NativeForItem(int id) { return 10+id; }
using RandoItemId=int; using RandomizerGet=int;
int extras=0, otherGrants=0, gFcCombo_SuppressRecord=0;
bool sApplyingFc=false; void* gPlayState=nullptr;
constexpr int MOD_RANDOMIZER=1;
struct GetItemEntry { int modIndex=MOD_RANDOMIZER; int native; GetItemEntry GetGIEntry_Copy() { return *this; } };
namespace Rando { namespace StaticData { GetItemEntry RetrieveItem(int native) { return {MOD_RANDOMIZER,native}; } } }
void Replay(int id) { if(id==10) { ++extras; for(int i=0;i<6;++i) if(!(nei->caneSkills&(1<<i))) { Cane_GiveSkill(i);break; } } else ++otherGrants; }
void Randomizer_Item_Give(void*,GetItemEntry entry) { Replay(entry.native); }
#define SPDLOG_WARN(...)
#define FS_TRACE(...)
namespace spdlog { struct Logger { void flush() {} } logger; Logger* default_logger() { return &logger; } }
int fallback;
namespace Rando { void GiveItem(int id) { if(id>=10) Replay(id); else { assert(id == 6); ++fallback; } } }
/* GRANT */
/* SYNC */
int main() {
    for (const auto& item: ComboRando::BlackMarket::Catalog()) {
        if (std::string_view(item.key).starts_with("nei.cane.")) {
            states[0] = {}; nei=&states[0]; fallback=0;
            Grant(item);
            if (Id(item)==6) assert(fallback==1 && nei->caneSkills==0);
            else assert(nei->caneSkills == (1 << Id(item)));
        }
    }
    for(int a=0;a<64;++a) for(int b=0;b<64;++b) {
        states[0] = {(u8)a,false}; states[1] = {(u8)b,false};
        nlohmann::json sh = nlohmann::json::object();
        nei=&states[0]; Extract0(sh);
        nei=&states[1]; Apply1(sh); Extract1(sh);
        nei=&states[0]; Apply0(sh);
        assert(states[0].caneSkills==(a|b) && states[1].caneSkills==(a|b));
        // Repeated / older partial snapshots never regress a named power.
        Apply0(nlohmann::json{{"caneSkills",a}});
        assert(states[0].caneSkills==(a|b));
    }
    // Ordinary progressive pickup already has its native bit and records one FC copy.
    // Counter and mask deltas may arrive in either order; neither order may replay a second skill.
    for(int direction=0;direction<2;++direction) for(int order=0;order<2;++order) {
        states[0]={}; states[1]={}; nei=&states[direction];
        Cane_GiveSkill(0); nei->comboObtainedFc[0]=nei->comboAppliedFc[0]=1;
        nlohmann::json mask=nlohmann::json::object(), counts={{"comboObtainedFc",std::array<int,2>{1,1}}};
        if(direction==0) Extract0(mask); else Extract1(mask);
        nei=&states[1-direction]; extras=otherGrants=0;
        if(order==0) {
            if(direction==0) Merge1(counts); else Merge0(counts);
            if(direction==0) Materialize1(); else Materialize0();
        }
        if(direction==0) Apply1(mask); else Apply0(mask);
        if(direction==0) Merge1(counts); else Merge0(counts);
        if(direction==0) Materialize1(); else Materialize0();
        assert(nei->caneSkills==1 && extras==0 && otherGrants==1);
        assert(nei->comboAppliedFc[0]==1 && nei->comboAppliedFc[1]==1);
    }
    puts("PASS: exact cane grants (including Byrna separation); all bit combinations both native sync directions");
}
'''
fixture=fixture.replace("/* IDS */", "enum {"+",".join(ids)+"};").replace("/* NAMES */", ",".join('"'+x+'"' for x in ids)).replace("/* GRANT */",grant)
blocks=[]
for i,game in enumerate(("soh/soh", "mm/2s2h")):
    source=(ROOT / game / "FleetShipCombo/FleetSync.cpp").read_text()
    extract=next(line for line in source.splitlines() if 'sh["caneSkills"] =' in line)
    apply=block_from(source,source.index('if (sh.contains("caneSkills")'))
    materialize=block_from(source,source.index("void ApplyFcRegistryToNatives()")).replace("ApplyFcRegistryToNatives", f"Materialize{i}")
    merge=block_from(source,source.index('if (sh.contains("comboObtainedFc")'))
    blocks.extend([materialize, f"void Merge{i}(const nlohmann::json& sh) {{{merge}}}"])
    blocks.extend([f"void Extract{i}(nlohmann::json& sh) {{{extract}}}",f"void Apply{i}(const nlohmann::json& sh) {{{apply}}}"])
fixture=fixture.replace("/* SYNC */", "\n".join(blocks))
with tempfile.TemporaryDirectory(prefix="market-cane-") as temp:
    path=Path(temp)/"test.cpp"; path.write_text(fixture)
    binary=Path(temp)/"test"
    subprocess.run([os.environ.get("CXX","c++"),"-std=c++20","-I"+str(ROOT/"combo"),str(path),"-o",str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
