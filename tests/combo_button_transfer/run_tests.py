#!/usr/bin/env python3
"""Run real departure/arrival equip functions against native saves and JSON."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_scene_randomization_tests import function

PREFIX = r'''
#include "global.h"
#include "mods/nei_save.h"
#include "gameplay/MagicMirrorItem.h"
#include "mods/ext_buttons/ext_buttons.h"
#include "FleetComboIds.h"
#include <nlohmann/json.hpp>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
SaveContext gSaveContext{};
static nlohmann::json sNetShared, sNetFlat;
static bool sNetPrimed;
static int sNetMismatchStreak, sNetScanTick;
static NeiSaveData nei{};
extern "C" NeiSaveData* Nei_Save() { return &nei; }
extern "C" uint16_t Nei_GetOwnedItem(uint8_t slot) { return slot>=24 && slot<72 ? nei.ownedItems[slot-24] : ITEM_NONE; }
extern "C" void Nei_SetOwnedItem(uint8_t slot, uint16_t item) { nei.ownedItems[slot-24]=item; }
static uint16_t ExtInv_GetSlotItem(int slot) { return Nei_GetOwnedItem(slot); }
'''

COMMON_CHECKS = r'''
static void own() {
    nei.ownedItems[3]=0xD0; // elemental-wand cell 27 in both games
    nei.ownedItems[23]=0x0223; // full Rod of Seasons ID, cell 47
}
int main(int argc, char** argv) {
    const char* mode=argc<2?"apply":argv[1];
    reset(); own();
#ifdef TEST_OOT_HOST
    if (!strcmp(mode,"zero-source") || !strcmp(mode,"dump-zero-source")) {
        ownDirections();
        if (!strcmp(mode,"dump-zero-source")) {
            setD(1,ITEM_STICK,SLOT_NONE);
            ExtButton_SetItem(6,0); // invalid EXT marker with an empty u16 shadow
            nlohmann::json sh; ExtractEquips(sh); std::cout << sh.dump();
            return 0;
        }
        // A real native stick carries SLOT_STICK. Empty or zero-shadow
        // representations must not become real sticks in an owned peer save.
        for (int btn=1;btn<=7;++btn) for (int representation=0;representation<3;++representation) {
            reset(); ownDirections();
            gSaveContext.equips.buttonItems[btn]=representation==0?ITEM_STICK:ITEM_EXT_BUTTON;
            gSaveContext.equips.cButtonSlots[btn-1]=representation==2?SLOT_STICK:SLOT_NONE;
            gSaveContext.ship.extButtons.items[btn]=0;
            const auto before=gSaveContext;
            nlohmann::json sh; ExtractEquips(sh);
            const auto& emitted=btn<4?sh["cEquips"][btn-1]:sh["dEquips"][btn-4];
            if (emitted!=0xFF) {
                std::fprintf(stderr,"invalid zero source btn=%d representation=%d emitted canonical item %s instead of keep-local 255\n",
                             btn,representation,emitted.dump().c_str());
                return 1;
            }
            assert(!memcmp(&before,&gSaveContext,sizeof(before)) && "departure validation must not mutate the local save");
        }
        reset(); ownDirections();
        for (int btn=1;btn<=7;++btn) {
            gSaveContext.equips.buttonItems[btn]=ITEM_STICK;
            gSaveContext.equips.cButtonSlots[btn-1]=SLOT_STICK;
        }
        nlohmann::json sh; ExtractEquips(sh);
        for (const auto& item:sh["cEquips"]) assert(item==0);
        for (const auto& item:sh["dEquips"]) assert(item==0);
        puts("PASS zero source: invalid native/EXT zero never exported; valid C/D sticks retained without save mutation");
        return 0;
    }
#endif
    if (!strcmp(mode,"dump-directions") || !strcmp(mode,"dump-left") || !strcmp(mode,"dump-empty")) {
        ownDirections();
        if (!strcmp(mode,"dump-directions")) {
            setD(0,NATIVE_STICK,NATIVE_STICK_SLOT); setD(1,NATIVE_BOW,NATIVE_BOW_SLOT);
            setD(2,NATIVE_BOMB,NATIVE_BOMB_SLOT); setD(3,0x0223,ROD_D_SLOT);
        } else if (!strcmp(mode,"dump-left")) {
            setD(2,NATIVE_STICK,NATIVE_STICK_SLOT);
        }
        nlohmann::json sh; ExtractEquips(sh); std::cout << sh.dump();
        return 0;
    } else if (!strcmp(mode,"receive-directions") || !strcmp(mode,"receive-left") || !strcmp(mode,"receive-empty")) {
        ownDirections(); nlohmann::json sh; std::cin >> sh; ApplyEquips(sh);
        const uint16_t expectedItems[4] = { NATIVE_STICK, NATIVE_BOW, NATIVE_BOMB, 0x0223 };
        const uint8_t expectedSlots[4] = { NATIVE_STICK_SLOT, NATIVE_BOW_SLOT, NATIVE_BOMB_SLOT, ROD_D_SLOT };
        const char* directions[] = { "Up", "Down", "Left", "Right" };
        for (int direction=0; direction<4; ++direction) {
            const bool populated = !strcmp(mode,"receive-directions") ||
                                   (!strcmp(mode,"receive-left") && direction==2);
            const uint16_t expected = populated ? (!strcmp(mode,"receive-left") ? NATIVE_STICK : expectedItems[direction]) : ITEM_NONE;
            const uint8_t expectedSlot = populated ? (!strcmp(mode,"receive-left") ? NATIVE_STICK_SLOT : expectedSlots[direction]) : SLOT_NONE;
            if (expected >= 0x0200 && expected != ITEM_NONE)
                assert(rawD(direction)==ITEM_EXT_BUTTON && shadowD(direction)==expected);
            else
                assert(shadowD(direction)==0);
            if (getD(direction)!=expected || slotD(direction)!=expectedSlot) {
                std::fprintf(stderr,"physical D-pad %s expected item=0x%04x slot=0x%02x, got item=0x%04x slot=0x%02x\n",
                             directions[direction],expected,expectedSlot,getD(direction),slotD(direction));
                return 1;
            }
        }
        for (int c=1;c<=3;++c) assert(getC(c)==ITEM_NONE && slotC(c)==SLOT_NONE);
        checkOtherForm();
        puts("PASS physical D-pad: Up/Down/Left/Right items and native slots preserved");
        return 0;
    } else if (!strcmp(mode,"keep-local")) {
        ownDirections();
        setC(1,NATIVE_STICK,NATIVE_STICK_SLOT); setD(1,NATIVE_STICK,NATIVE_STICK_SLOT);
        setC(2,0x0223,0xFF); setD(2,0x0223,ROD_D_SLOT);
        const auto before=gSaveContext;
        nlohmann::json sh={{"cEquips",nlohmann::json::array({0xFF,0xFF,0xFF})},
                           {"dEquips",nlohmann::json::array({0xFF,0xFF,0xFF,0xFF})},
                           {"dEquipsOrder","UDLR"}};
        ApplyEquips(sh);
        assert(!memcmp(&before,&gSaveContext,sizeof(before)) &&
               "empty peer payload must retain valid local native sticks and full custom IDs");
        puts("PASS empty peer payload preserves legitimate native C/Down sticks and local full custom IDs");
    } else if (!strcmp(mode,"extract")) {
        setC(1,0xD0,WAND_SLOT); setC(2,0x0223,0xFF);
        setD(0,0x0223,ROD_D_SLOT);
        nlohmann::json sh; ExtractEquips(sh);
        assert(sh["cEquips"][0]==0xD0);
        assert(sh["cEquips"][1]==0x0223 && "departure must carry the full Rod of Seasons ID");
        assert(sh["dEquips"][0]==0x0223);
        assert(sh["dEquipsOrder"]=="UDLR");
        puts("PASS departure: shared wand and full u16 Rod of Seasons C/D payloads");
    } else if (!strcmp(mode,"apply")) {
        nlohmann::json sh;
        sh["cEquips"]=nlohmann::json::array({0xD0,0x0223,0xFF});
        sh["dEquipsOrder"]="UDLR";
        sh["dEquips"]=nlohmann::json::array({0x0223,0xD0,0xFF,0xFF});
        ApplyEquips(sh);
        assert(getC(1)==0xD0 && "arrival must resolve custom NEI ownership, not only native inventory");
        assert(slotC(1)==WAND_SLOT);
        assert(getC(2)==0x0223 && slotC(2)==0xFF);
        assert(getD(0)==0x0223 && slotD(0)==ROD_D_SLOT);
        assert(getD(1)==0xD0 && slotD(1)==WAND_SLOT);
        checkOtherForm();
        puts("PASS arrival: owned NEI cells, marker/shadow/slot coherence and shared MM buttons");
    } else if (!strcmp(mode,"mirror")) {
        nei.timeGateOwned=nei.magicMirrorOwned=1;
        nei.ownedItems[SLOT_MAGIC_MIRROR-24]=ITEM_TIME_GATE;
        nlohmann::json sh;
        sh["cEquips"]=nlohmann::json::array({EXT_ITEM_MAGIC_MIRROR,0xFF,0xFF});
        sh["dEquipsOrder"]="UDLR";
        sh["dEquips"]=nlohmann::json::array({EXT_ITEM_MAGIC_MIRROR,0xFF,0xFF,0xFF});
        ApplyEquips(sh);
        assert(getC(1)==EXT_ITEM_MAGIC_MIRROR && getD(0)==EXT_ITEM_MAGIC_MIRROR);
        assert(nei.ownedItems[SLOT_MAGIC_MIRROR-24]==ITEM_TIME_GATE);
        puts("PASS Mirror arrival: full u16 C/D payloads restore behind selected Time Gate");
    } else if (!strcmp(mode,"metadata")) {
        for (const auto& order : nlohmann::json::array({"RLD U",17,nullptr,nlohmann::json::array()})) {
            reset(); own(); ownDirections();
            nlohmann::json sh={{"cEquips",nlohmann::json::array({0x03,0xFF,0xFF})},
                               {"dEquips",nlohmann::json::array({0x00,0x03,0x02,0x0223})},
                               {"dEquipsOrder",order}};
            ApplyEquips(sh);
            assert(getC(1)==NATIVE_BOW && slotC(1)==NATIVE_BOW_SLOT);
            for (int d=0;d<4;++d) assert(getD(d)==ITEM_NONE && slotD(d)==SLOT_NONE && shadowD(d)==0);
            checkOtherForm();
        }
        nlohmann::json sh={{"cEquips",nlohmann::json::array({0x03})},
                           {"dEquips",nlohmann::json::array({0x00})}, {"dEquipsOrder","UDLR"},
                           {"sourceGame","mm"},{"unrelated",42}};
        NetResetBaseline(sh);
        assert(!sNetShared.contains("cEquips") && !sNetShared.contains("dEquips") &&
               !sNetShared.contains("dEquipsOrder"));
        assert(sNetShared["unrelated"]==42 && sNetShared["sourceGame"]=="mm" &&
               sNetFlat==sNetShared.flatten());
        puts("PASS metadata: unknown D-pad order leaves C policy intact; transient order excluded from FleetNet");
    } else {
        nlohmann::json sh;
        sh["cEquips"]=nlohmann::json::array({0xD0,0x0223,0xFF});
        // A quest-only EXT item has no shared inventory route: retain it.
        setC(1,0x0201,0xFF); setD(0,0x0202,0xFF);
        sh["dEquipsOrder"]="UDLR";
        sh["dEquips"]=nlohmann::json::array({0x0223}); ApplyEquips(sh);
        assert(getC(1)==0x0201 && getD(0)==0x0202);
        reset();
        ApplyEquips(sh); assert(getC(1)==0xFF && getC(2)==0xFF && getD(0)==0xFF);
        own();
        sh["cEquips"]=nlohmann::json::array({-48,0x10223,"invalid"});
        sh["dEquipsOrder"]="UDLR";
        sh["dEquips"]=nlohmann::json::array({0x10223,0x0201,-48,nullptr});
        ApplyEquips(sh);
        assert(getC(1)==0xFF && getC(2)==0xFF && getD(0)==0xFF && getD(1)==0xFF);
        // A native arrival clears old shadow metadata.
        ownBow(); setC(2,0x0223,0xFF);
        sh["cEquips"]=nlohmann::json::array({0xFF,0x03}); ApplyEquips(sh);
        assert(getC(2)==NATIVE_BOW && slotC(2)==NATIVE_BOW_SLOT && shadowC(2)==0);
        puts("PASS policy: local quest items, missing ownership, malformed IDs and native shadow clearing");
    }
    return 0;
}
'''

MM = r'''
#define MM_INV gSaveContext.save.saveInfo.inventory
#define MM_EQ gSaveContext.save.saveInfo.equips
static constexpr uint8_t kSlotOcarina=SLOT_OCARINA;
#define WAND_SLOT 75
#define ROD_D_SLOT 95
#define NATIVE_BOW ITEM_BOW
#define NATIVE_BOW_SLOT SLOT_BOW
#define NATIVE_STICK ITEM_DEKU_STICK
#define NATIVE_STICK_SLOT SLOT_DEKU_STICK
#define NATIVE_BOMB ITEM_BOMB
#define NATIVE_BOMB_SLOT SLOT_BOMB
static constexpr int kPhysicalDpad[] = { EQUIP_SLOT_D_UP, EQUIP_SLOT_D_DOWN, EQUIP_SLOT_D_LEFT, EQUIP_SLOT_D_RIGHT };
static void reset() {
    gSaveContext={}; nei={};
    memset(MM_INV.items,0xFF,sizeof(MM_INV.items));
    memset(MM_EQ.buttonItems,0xFF,sizeof(MM_EQ.buttonItems));
    memset(MM_EQ.cButtonSlots,0xFF,sizeof(MM_EQ.cButtonSlots));
    memset(gSaveContext.save.shipSaveInfo.dpadEquips.dpadItems,0xFF,sizeof(gSaveContext.save.shipSaveInfo.dpadEquips.dpadItems));
    memset(gSaveContext.save.shipSaveInfo.dpadEquips.dpadSlots,0xFF,sizeof(gSaveContext.save.shipSaveInfo.dpadEquips.dpadSlots));
    for (auto& item:nei.ownedItems) item=ITEM_NONE;
    // Native MM C and D buttons are shared in form 0; only B is form-specific.
    gSaveContext.save.playerForm=PLAYER_FORM_DEKU;
}
static void setC(int b,uint16_t item,uint8_t slot) {
    if(item>=0x200) ExtButton_SetItem(0,b,item); else MM_EQ.buttonItems[0][b]=item;
    MM_EQ.cButtonSlots[0][b]=slot;
}
static void setD(int b,uint16_t item,uint8_t slot) { const int native=kPhysicalDpad[b]; ExtButton_SetDpadItem(0,native,item); gSaveContext.save.shipSaveInfo.dpadEquips.dpadSlots[0][native]=slot; }
static uint16_t getC(int b) {return ExtButton_GetItem(0,b);}
static uint16_t getD(int b) {return ExtButton_GetDpadItem(0,kPhysicalDpad[b]);}
static uint8_t slotC(int b) {return MM_EQ.cButtonSlots[0][b];}
static uint8_t slotD(int b) {return gSaveContext.save.shipSaveInfo.dpadEquips.dpadSlots[0][kPhysicalDpad[b]];}
static uint8_t rawD(int b) {return gSaveContext.save.shipSaveInfo.dpadEquips.dpadItems[0][kPhysicalDpad[b]];}
static uint16_t shadowD(int b) {return gSaveContext.save.shipSaveInfo.dpadEquips.extItems[0][kPhysicalDpad[b]];}
static uint16_t shadowC(int b) {return gSaveContext.save.shipSaveInfo.extButtons.items[0][b];}
static void ownBow() {MM_INV.items[SLOT_BOW]=ITEM_BOW;}
static void ownDirections() {MM_INV.items[NATIVE_STICK_SLOT]=NATIVE_STICK; ownBow(); MM_INV.items[NATIVE_BOMB_SLOT]=NATIVE_BOMB;}
static void checkOtherForm() {
    assert(MM_EQ.buttonItems[PLAYER_FORM_DEKU][1]==0xFF);
    for (int form=1;form<4;++form) for (int button=0;button<4;++button) {
        assert(gSaveContext.save.shipSaveInfo.dpadEquips.dpadItems[form][button]==ITEM_NONE);
        assert(gSaveContext.save.shipSaveInfo.dpadEquips.dpadSlots[form][button]==SLOT_NONE);
        assert(gSaveContext.save.shipSaveInfo.dpadEquips.extItems[form][button]==0);
    }
}
'''

OOT = r'''
#define TEST_OOT_HOST
#define WAND_SLOT 27
#define ROD_D_SLOT 0xFF
#define NATIVE_BOW ITEM_BOW
#define NATIVE_BOW_SLOT SLOT_BOW
#define NATIVE_STICK ITEM_STICK
#define NATIVE_STICK_SLOT SLOT_STICK
#define NATIVE_BOMB ITEM_BOMB
#define NATIVE_BOMB_SLOT SLOT_BOMB
static void reset() {
    gSaveContext={}; nei={};
    memset(gSaveContext.inventory.items,0xFF,sizeof(gSaveContext.inventory.items));
    memset(gSaveContext.equips.buttonItems,0xFF,sizeof(gSaveContext.equips.buttonItems));
    memset(gSaveContext.equips.cButtonSlots,0xFF,sizeof(gSaveContext.equips.cButtonSlots));
    for (auto& item:nei.ownedItems) item=ITEM_NONE;
}
static void setC(int b,uint16_t item,uint8_t slot) {
    if(item>=0x200) ExtButton_SetItem(b,item); else gSaveContext.equips.buttonItems[b]=item;
    gSaveContext.equips.cButtonSlots[b-1]=slot;
}
static void setD(int b,uint16_t item,uint8_t slot) {setC(b+4,item,slot);}
static uint16_t getC(int b) {return ExtButton_GetItem(b);}
static uint16_t getD(int b) {return ExtButton_GetItem(b+4);}
static uint8_t slotC(int b) {return gSaveContext.equips.cButtonSlots[b-1];}
static uint8_t slotD(int b) {return gSaveContext.equips.cButtonSlots[b+3];}
static uint8_t rawD(int b) {return gSaveContext.equips.buttonItems[b+4];}
static uint16_t shadowD(int b) {return gSaveContext.ship.extButtons.items[b+4];}
static uint16_t shadowC(int b) {return gSaveContext.ship.extButtons.items[b];}
static void ownBow() {gSaveContext.inventory.items[SLOT_BOW]=ITEM_BOW;}
static void ownDirections() {gSaveContext.inventory.items[NATIVE_STICK_SLOT]=NATIVE_STICK; ownBow(); gSaveContext.inventory.items[NATIVE_BOMB_SLOT]=NATIVE_BOMB;}
static void checkOtherForm() {}
'''

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-ref")
    parser.add_argument("--sanitize",action="store_true")
    parser.add_argument("--json-include",type=Path,default=Path("/usr/include"))
    args=parser.parse_args()
    def read(path):
        if args.source_ref:
            return subprocess.check_output(["git","show",args.source_ref+":"+path],cwd=ROOT,text=True)
        return (ROOT/path).read_text()
    failures=0
    with tempfile.TemporaryDirectory(prefix="combo-button-transfer-") as td:
        binaries={}
        for host,fleet in (("mm","mm/2s2h/FleetShipCombo"),("soh","soh/soh/FleetShipCombo")):
            sync=read(fleet+"/FleetSync.cpp")
            buttons=read(host+"/mods/ext_buttons/ext_buttons.cpp")
            names=["ExtButton_GetItem","ExtButton_SetItem","ExtButton_ClearItem"]
            if host=="mm": names += ["ExtButton_GetDpadItem","ExtButton_SetDpadItem"]
            production="\n".join(function(buttons,n) for n in names)
            inventory=read(host+"/mods/extended_inventory.c")
            if "TravelTools_IsOwned(" in inventory:
                production += "\n"+function(inventory,"TravelTools_IsOwned")
            if host=="mm": production += "\n"+function(sync,"MmButtonCanonical")
            production += "\n"+"\n".join(function(sync,n) for n in ("ExtractEquips","FindInvSlot","ApplyOneButton","ApplyEquips","NetResetBaseline"))
            code=PREFIX+(MM if host=="mm" else OOT)+production+COMMON_CHECKS
            flags=["-std=c++20","-w","-DF3DEX_GBI_2","-DCOMBO_BUILD","-DCONTROLLERBUTTONS_T=uint32_t"]
            flags += ["-I"+str(ROOT/p) for p in (host,host+"/include",host+"/include/PR",host+"/src",host+"/assets",host+"/2s2h","libultraship/include","combo","combo/menu",fleet)]
            flags += ["-I"+str(args.json_include)]
            if args.sanitize: flags += ["-fsanitize=address,undefined","-fno-omit-frame-pointer","-g","-no-pie"]
            path=Path(td); source=path/(host+".cpp"); binary=path/host; source.write_text(code)
            subprocess.run([os.environ.get("CXX","c++"),*flags,str(source),"-o",str(binary)],check=True)
            binaries[host]=binary
            env=dict(os.environ,ASAN_OPTIONS="detect_leaks=0",UBSAN_OPTIONS="halt_on_error=1")
            for mode in ("extract","apply","mirror","policy","metadata","keep-local") + (("zero-source",) if host=="soh" else ()):
                result=subprocess.run([str(binary),mode],env=env)
                failures += bool(result.returncode)
                print(host,mode,"FAIL" if result.returncode else "PASS",flush=True)
        for origin,target in (("soh","mm"),("mm","soh")):
            for sample in ("directions","left","empty"):
                payload=subprocess.check_output([str(binaries[origin]),"dump-"+sample],env=env,text=True)
                result=subprocess.run([str(binaries[target]),"receive-"+sample],input=payload,env=env,text=True)
                failures += bool(result.returncode)
                print(origin+"->"+target,sample,"FAIL" if result.returncode else "PASS",flush=True)
        payload=subprocess.check_output([str(binaries["soh"]),"dump-zero-source"],env=env,text=True)
        result=subprocess.run([str(binaries["mm"]),"receive-empty"],input=payload,env=env,text=True)
        failures += bool(result.returncode)
        print("soh->mm invalid-zero source","FAIL" if result.returncode else "PASS",flush=True)
        # Pre-fix temp files have no order tag and use the departing host's native
        # layout. Literal fixtures are independent of either production mapper.
        for target,source,values in (("mm","oot",[0x00,0x03,0x02,0x0223]),
                                     ("soh","mm",[0x0223,0x02,0x03,0x00])):
            for source_game in (None,source,0 if source=="oot" else 1):
                payload={"dEquips":values}
                if source_game is not None: payload["sourceGame"]=source_game
                result=subprocess.run([str(binaries[target]),"receive-directions"],input=json.dumps(payload),env=env,text=True)
                failures += bool(result.returncode)
                print(source+"->"+target,"legacy",source_game,"FAIL" if result.returncode else "PASS",flush=True)
    return int(bool(failures))

if __name__=="__main__":
    raise SystemExit(main())
