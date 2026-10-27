#!/usr/bin/env python3
"""Exercise the production pause lookup/display code, without a loaded ROM."""
from pathlib import Path
import argparse
import os
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    match = re.search(r"(?:^|\n)[^\n;{}]*\b" + name + r"\([^;{}]*\)\s*\{", source)
    if not match:
        raise RuntimeError("missing production function: " + name)
    first = source.index("{", match.start())
    level = 1
    end = first + 1
    while level:
        level += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


COMMON = r'''
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include "ComboItemReceiptText.h"
#if __has_include("ComboPauseTutorialText.h")
#include "ComboPauseTutorialText.h"
#endif
using u8=uint8_t; using u16=uint16_t; using s16=int16_t; using s32=int32_t;
#define ARRAY_COUNT(a) (sizeof(a)/sizeof((a)[0]))
#define PAUSE_ITEM 0
#define PAUSE_MAP 1
#define PAUSE_QUEST 2
#define PAUSE_MASK 3
#define PAUSE_EQUIP 3
#define PAUSE_ITEM_NONE 999
#define LANGUAGE_GER 1
#define LANGUAGE_FRE 2
#define LANGUAGE_FRA 2
#define LANGUAGE_ENG 0
#define CHECK_DUNGEON_ITEM(a,b) true
static int checks, failures;
static void check(bool ok,const char* label) { ++checks; if(!ok) {++failures; fprintf(stderr,"FAIL: %s\n",label);} }
static void contains(std::string value,const char* expected,const char* label) {
    for(char& c:value) if(c=='\x10'||c=='\x11'||c=='&'||c=='^')c=' ';
    check(value.find(expected)!=std::string::npos,label);
}
static u8 wandMode, rune, caneType, caneSkill, swordLevel, trueMaster, greatFairy, hammerAxe, ultrashot;
static int maskIndex=6, tradeIndex=0, extPage;
u8 Wand_GetMode(void) {return wandMode;} u8 Wand_RandoMode(void) {return 0;}
u8 Slate_GetRune(void) {return rune;} u8 Cane_GetType(void) {return caneType;} u8 Cane_GetActiveSkill(void) {return caneSkill;}
u8 Nei_HookshotLevel(void) {return 3;}
u8 Nei_UltrashotOwned(void) {return ultrashot;}
int OotMask_CursorIndex(void) {return maskIndex;} s32 TradeAdult_CursorIndex(void) {return tradeIndex;}
u8 WeaponUpgrade_KokiriLevel(void) {return swordLevel;} u8 WeaponUpgrade_HasTrueMaster(void) {return trueMaster;}
u8 WeaponUpgrade_HasGreatFairy(void) {return greatFairy;} u8 WeaponUpgrade_HasHammerAxe(void) {return hammerAxe;}
int ExtEquip_GetPage(void) {return extPage;}
static u8 capeOwned=1, capeVisible=1;
u8 ExtEquip_CapeOwned(void) {return capeOwned;} u8 ExtEquip_CapeVisible(void) {return capeVisible;}
u8 ExtEquip_PendantOwned(void) {return 1;}
u8 Sw97_IsBowItem(u16) {return 0;} u8 Sw97_IsSlingItem(u16) {return 0;} u8 Sw97_EffectiveElement(u8) {return 0;}
#define SW97_MEDALLIONS_ENABLED() true
static std::string shown;
'''


def defines(host):
    paths = [ROOT / host / "mods/extended_equipment.h", ROOT / host / "mods/extended_inventory.h"]
    paths += [ROOT / host / "mods/nei_save.h", ROOT / host / "expansions/sw97/sw97_config.h"]
    text = "\n".join(p.read_text() for p in paths)
    definitions = "\n".join(re.findall(r"^#define (?:ITEM_EXT_|EXT_ITEM_|WAND_MODE_|SLATE_RUNE_|SW97_ELEM_)[^\n]*", text, re.M))
    if host == "mm":
        definitions += "\n" + "\n".join(re.findall(r"^#define ITEM_(?:DINS_FIRE|FARORES_WIND|NAYRUS_LOVE|FAIRY_SLINGSHOT|HOOKSHOT_OOT|LONGSHOT_OOT)[^\n]*", text, re.M))
    return definitions


def fixture(host):
    code = COMMON + '\n#include "' + host + '/include/z64item.h"\n' + defines(host) + '\n'
    source = (ROOT / ("mm/2s2h/CustomMessage/PauseItemDescriptions.cpp" if host == "mm" else
                      "soh/soh/Enhancements/custom-message/PauseItemDescriptions.cpp")).read_text()
    if host == "mm":
        widths = (ROOT / "mm/src/code/z_message_nes.c").read_text()
        begin = widths.index("f32 sNESFontWidths[160]")
        code += widths[begin:widths.index("};", begin)+2].replace("f32 ", "float ") + "\n"
        code += r'''
struct MessageTableEntry {u16 textId;};
struct Font {struct {char schar[1280];} msgBuf;};
struct MessageContext {Font font; MessageTableEntry* messageTableNES; int msgLength,msgBufPos,textDrawPos,decodedTextLen; u16 currentTextId,nextTextId,unk1206C,unk12070,unk12074; int unk11F18,itemId,textboxY; u8 choiceIndex;};
struct PauseContext {s16 cursorX[4],cursorY[4];};
struct PlayState {MessageContext msgCtx; PauseContext pauseCtx;};
static PlayState state; static PlayState* gPlayState=&state;
static struct {int dungeonSceneSharedIndex; struct {int language;} options;} gSaveContext;
namespace Rando {std::string GetDungeonMapCompassInfo(s32,bool) {return "map";}}
namespace CustomMessage {
static bool shownCapeVisibilityChoice;
struct Entry {u8 textboxType,textboxYPos,icon; u16 nextMessageID,firstItemCost,secondItemCost; std::string msg; bool autoFormat=true,capeVisibilityChoice=false;};
void LoadCustomMessageIntoFont(const Entry& entry) {
    shown=entry.msg;
    shownCapeVisibilityChoice=entry.capeVisibilityChoice;
    char* header=state.msgCtx.font.msgBuf.schar;
    header[0]=entry.textboxType;header[1]=entry.textboxYPos;header[2]=entry.icon;
    for(int i=3;i<11;++i)header[i]=char(0xFF);
}
void EnsureMessageEnd(std::string* body) {if(body->empty() || body->back()!='\xBF')*body+='\xBF';}
}
static u16 chosenTemplate;
void func_801514B0(PlayState* play,u16 textId,u8) {
    chosenTemplate=textId;play->msgCtx.currentTextId=textId;
    play->msgCtx.font.msgBuf.schar[2]=0; // first vanilla template is the Ocarina
}
#define MESSAGE_CUSTOM_ICON_ITEM 0xFD
#define MESSAGE_ITEM_NONE 9999
static int sMsgStrayFairyIndex=-1,iconLoads;
static u8 D_801CFF94[256]={ITEM_OCARINA_OF_TIME};
void Message_LoadItemIcon(PlayState*,u8,int) {++iconLoads;}

'''
        code += function((ROOT / "mm/src/code/z_message.c").read_text(), "Message_DecodeHeader") + "\n"
        # Tables and lookup functions are copied unchanged from the production translation unit.
        code += source[source.index("struct ItemDescEntry"):]
        if "PauseItemDesc_ShowItem(" not in source:
            code += r'''
bool PauseItemDesc_ShowItem(PlayState* p,u16 id,s32 page,u8 pos) {const char* t=PauseItemDesc_Get(id,page); if(!t)return false; PauseItemDesc_Show(p,t,pos);return true;}
'''
        if "PauseItemDesc_ShowCapeVisibility(" not in source:
            code += "bool PauseItemDesc_ShowCapeVisibility(PlayState*,u8) {return false;}\n"
        code += r'''
static std::string describe(u16 id,s32 page=PAUSE_ITEM) {shown.clear(); PauseItemDesc_ShowItem(&state,id,page,1);return shown;}
int main() {
static MessageTableEntry table[]={{0x1700},{0xFFFF}}; state.msgCtx.messageTableNES=table;
'''
        code += r'''
contains(describe(ITEM_OOT_MASK_PLACEHOLDER),"Gerudo", "MM mask placeholder preserves selected Gerudo identity");
contains(describe(ITEM_OOT_MASK_PLACEHOLDER),"Urbosa", "MM Gerudo tutorial teaches rage finisher");
contains(describe(ITEM_TRADE_PLACEHOLDER),"Pocket Egg", "MM trade placeholder preserves selected identity");
const auto hourglass = describe(EXT_ITEM_PHANTOM_HOURGLASS);
Message_DecodeHeader(&state);
check(state.msgCtx.itemId==0xFE && iconLoads==0,"MM custom helper does not load inherited Ocarina icon");
static const u16 helperItems[]={EXT_ITEM_SHADOW_CRYSTAL,ITEM_GUST_JAR,ITEM_ELEMENTAL_WAND,EXT_ITEM_SHEIKAH_SLATE};
for(u16 id:helperItems) {
    describe(id);Message_DecodeHeader(&state);
    check(state.msgCtx.itemId==0xFE && iconLoads==0,"MM custom item helper remains iconless after header decode");
}
PauseItemDesc_ShowEquipment(&state,0,0,1,1);Message_DecodeHeader(&state);
check(state.msgCtx.itemId==0xFE && iconLoads==0,"MM equipment helper does not borrow template icon");
PauseItemDesc_ShowForm(&state,1,1);Message_DecodeHeader(&state);
check(state.msgCtx.itemId==0xFE && iconLoads==0,"MM form helper does not borrow template icon");
// A genuinely native description keeps its correct header icon.
iconLoads=0;state.msgCtx.msgBufPos=0;state.msgCtx.font.msgBuf.schar[2]=0;Message_DecodeHeader(&state);
check(state.msgCtx.itemId==ITEM_OCARINA_OF_TIME && iconLoads==1,"native helper header icon behavior is preserved");
check(hourglass.size() > strlen(hourglass.c_str()), "MM encoded body retains embedded white-color zero bytes");
contains(hourglass, "Release", "MM text after embedded zero reaches display boundary");
check(!hourglass.empty() && hourglass.back()=='\xBF', "MM encoded tutorial has message terminator");
caneType=2;caneSkill=3;
const auto flip=describe(ITEM_CANE_OF_SOMARIA);unsigned lines=0;bool pageFits=true;
for(unsigned char c:flip) {if(c==0x10)lines=0;else if(c==0x11)pageFits &= ++lines<3;}
check(pageFits,"MM encoded cane tutorial fits the decoder's three line offsets");
static const char* names[4][3] = {{"Kokiri Sword","Master Sword","Biggoron's Sword"},{"Deku Shield","Hylian Shield","Mirror Shield"},
    {"Kokiri Tunic","Goron Tunic","Zora Tunic"},{"Kokiri Boots","Iron Boots","Hover Boots"}};
for(int row=0;row<4;++row) for(int col=1;col<=3;++col) {
    shown.clear(); check(PauseItemDesc_ShowEquipment(&state,0,row,col,1),"MM every vanilla equipment cell opens");
    contains(shown,names[row][col-1],"MM equipment tutorial follows cell rather than shared icon");
    check(PauseItemDesc_ShowEquipment(&state,1,row,col,1),"MM every extended equipment cell opens");
}
shown.clear(); PauseItemDesc_ShowEquipment(&state,1,0,0,1); contains(shown,"Magic Cape","MM passive Cape distinct from Champion grid");
contains(shown,"Show/Hide", "MM Cape helper teaches the A visibility popup");
check(!CustomMessage::shownCapeVisibilityChoice,"C-Up Cape help remains a description without choices");
check(PauseItemDesc_ShowCapeVisibility(&state,3),"MM Cape A popup opens through native pause template");
check(CustomMessage::shownCapeVisibilityChoice,"MM Cape A popup arms existing Show/Hide choices");
check(state.msgCtx.choiceIndex==0,"visible Cape initially selects Show cape");
Message_DecodeHeader(&state);check(state.msgCtx.itemId==0xFE,"Cape visibility popup remains iconless");
capeVisible=0;check(PauseItemDesc_ShowCapeVisibility(&state,3),"hidden Cape can reopen appearance choices");
check(state.msgCtx.choiceIndex==1,"hidden Cape initially selects Hide cape");
capeOwned=0;check(!PauseItemDesc_ShowCapeVisibility(&state,3),"unowned Cape cannot open appearance choices");
capeOwned=capeVisible=1;
PauseItemDesc_ShowEquipment(&state,0,2,2,1); contains(shown,"Fireproof","MM Goron tunic teaches actual fireproof effect");
check(!CustomMessage::shownCapeVisibilityChoice,"ordinary equipment help clears Cape appearance choices");
PauseItemDesc_ShowEquipment(&state,0,2,3,1); contains(shown,"electric shock","MM Zora tunic teaches actual electric immunity");
PauseItemDesc_ShowEquipment(&state,0,3,2,1); contains(shown,"knockback","MM Iron Boots teach actual knockback effect");
PauseItemDesc_ShowEquipment(&state,1,1,0,1); contains(shown,"Pendant","MM passive Pendant distinct from Climb grid");
for(int form=0;form<3;++form) {check(PauseItemDesc_ShowForm(&state,form,3),"MM every form opens controls");contains(shown,"A equips", "MM form tutorial preserves A equip hint");}
check(!PauseItemDesc_ShowForm(&state,-1,3),"MM invalid form has no tutorial");
check(!PauseItemDesc_ShowEquipment(&state,0,4,1,1),"MM invalid equipment row has no tutorial");
caneType=1; caneSkill=6; contains(describe(ITEM_CANE_OF_SOMARIA),"not implemented in MM", "MM Tri Rod does not borrow donor echo controls");
contains(describe(ITEM_EXT_SWORD_1,PAUSE_MASK),"Cosmetic", "MM Byrna preserves host-specific behavior");
static MessageTableEntry secondTable[]={{0x1702},{0xFFFF}}; state.msgCtx.messageTableNES=secondTable;
describe(EXT_ITEM_PHANTOM_HOURGLASS); check(chosenTemplate==0x1702,"MM template refreshes when message table changes");
state.msgCtx.messageTableNES=nullptr; check(!PauseItemDesc_ShowItem(&state,EXT_ITEM_PHANTOM_HOURGLASS,PAUSE_ITEM,1),"MM no template fails closed");
check(!PauseItemDesc_ShowCapeVisibility(&state,3),"MM Cape popup fails closed without a native pause template");
state.msgCtx.messageTableNES=table;
'''
    else:
        trade_source = (ROOT / "soh/mods/items/logic/trade_items.c").read_text()
        begin = trade_source.index("static const u8 sTradeAdultItems[")
        code += "#define TRADE_ADULT_COUNT 23\n" + trade_source[begin:trade_source.index("};", begin)+2] + "\n"
        code += function(trade_source, "TradeAdult_IndexOfItem") + "\n"
        enum_text = (ROOT / "soh/soh/Enhancements/custom-message/CustomMessageTypes.h").read_text()
        enum_text = enum_text[enum_text.index("    TEXT_DESC_ROCS_FEATHER"):enum_text.index("}", enum_text.index("    TEXT_DESC_ROCS_FEATHER"))]
        code += "enum {" + enum_text + "};\n"
        code += r'''
struct PauseContext {s16 cursorX[4],cursorY[4];}; struct PlayState {PauseContext pauseCtx;};
static PlayState state; static PlayState* gPlayState=&state;
static struct {int language;} gSaveContext;
struct CustomMessage {std::string body; CustomMessage(const std::string& a,const std::string&,const std::string&):body(a) {} void Format() {} void AutoFormat() {} void LoadIntoFont() {shown=body;}};
extern "C" u16 Randomizer_GetDungeonItemInfoTextId(u16) {return 0;}
'''
        code += source[source.index("struct ItemDescEntry"):source.index("// Register all description hooks")]
        code += r'''
static std::string describe(u16 id,s32 page=PAUSE_ITEM) {shown.clear(); u16 textId=PauseItemDesc_GetTextId(id,page); bool table=true; if(textId)OnOpenTextDescHook(&textId,&table); return shown;}
int main() {
'''
        code += r'''
contains(describe(ITEM_MASK_GERUDO),"Urbosa", "OoT Gerudo mask reaches current full tutorial");
static const char* tradeNames[]={"Pocket Egg","Pocket Cucco","Cojiro","Odd Mushroom","Odd Potion","Poacher's Saw",
    "Broken Goron's Sword","Prescription","Eyeball Frog","Eye Drops","Claim Check","Moon's Tear","Land Title Deed",
    "Swamp Title Deed","Mountain Title Deed","Ocean Title Deed","Room Key","Letter to Kafei","Special Delivery",
    "Mortal Draw","Weird Egg","Chicken","Zelda's Letter"};
for(int selected=0;selected<TRADE_ADULT_COUNT;++selected)
    contains(describe(sTradeAdultItems[selected]),tradeNames[selected],"OoT all 23 selected trade IDs preserve identity/tutorial");
check(describe(ITEM_EXT_BOOTS_2).find("Climb")==std::string::npos,"OoT trade Pendant retains combat tutorial rather than grid boots");
static const char* names[4][3] = {{"Kokiri Sword","Master Sword","Biggoron's Sword"},{"Deku Shield","Hylian Shield","Mirror Shield"},
    {"Kokiri Tunic","Goron Tunic","Zora Tunic"},{"Kokiri Boots","Iron Boots","Hover Boots"}};
for(int row=0;row<4;++row) for(int col=1;col<=3;++col) {
    state.pauseCtx.cursorX[PAUSE_EQUIP]=col; state.pauseCtx.cursorY[PAUSE_EQUIP]=row;
    extPage=0; const u16 selectedItem=row==0&&col==3 ? ITEM_HEART_PIECE_2 : ITEM_SWORD_KOKIRI+row*3+col-1;
    contains(describe(selectedItem,PAUSE_EQUIP),names[row][col-1],"OoT every vanilla equipment cell follows coordinates");
    extPage=1; check(!describe(ITEM_EXT_SWORD_1+row*3+col-1,PAUSE_EQUIP).empty(),"OoT every extended cell opens tutorial");
}
state.pauseCtx.cursorX[PAUSE_EQUIP]=0; state.pauseCtx.cursorY[PAUSE_EQUIP]=0;
contains(describe(ITEM_EXT_TUNIC_1,PAUSE_EQUIP),"Magic Cape","OoT Cape distinct from Champion grid");
state.pauseCtx.cursorY[PAUSE_EQUIP]=1; contains(describe(ITEM_EXT_BOOTS_2,PAUSE_EQUIP),"Pendant","OoT Pendant distinct from Climb grid");
contains(describe(ITEM_OCARINA_TIME,PAUSE_EQUIP),"Link Mode","OoT Link form controls");
contains(describe(ITEM_MARIO_MASK,PAUSE_EQUIP),"Mario Mode","OoT Mario form controls");
contains(describe(ITEM_POKEBALL,PAUSE_EQUIP),"Pikachu Mode","OoT Pikachu form controls");
caneType=1;caneSkill=7; contains(describe(ITEM_CANE_OF_SOMARIA),"echo grid","OoT Tri Rod teaches implemented echo grid");
state.pauseCtx.cursorX[PAUSE_EQUIP]=1;state.pauseCtx.cursorY[PAUSE_EQUIP]=0;extPage=1;
contains(describe(ITEM_EXT_SWORD_1,PAUSE_EQUIP),"Kinsect","OoT Byrna preserves host-specific combat");
state.pauseCtx.cursorX[PAUSE_EQUIP]=3;state.pauseCtx.cursorY[PAUSE_EQUIP]=0;extPage=0;
contains(describe(ITEM_SWORD_BGS,PAUSE_EQUIP),"Giant's Knife","OoT unbroken fragile knife keeps its selected identity");
contains(describe(ITEM_SWORD_KNIFE,PAUSE_EQUIP),"Broken Giant's Knife","OoT broken knife keeps its selected identity");
contains(describe(ITEM_HEART_PIECE_2,PAUSE_EQUIP),"Biggoron's Sword","OoT full Biggoron marker gets durable sword identity");
greatFairy=1;contains(describe(ITEM_HEART_PIECE_2,PAUSE_EQUIP),"Great Fairy's Sword","OoT fairy upgrade retains longsword priority");greatFairy=0;
ultrashot=0;contains(describe(ITEM_LONGSHOT),"twice","OoT ordinary Longshot keeps normal reach tutorial");
ultrashot=1;contains(describe(ITEM_LONGSHOT),"Ultrashot","OoT upgraded Longshot routes active Ultrashot identity");ultrashot=0;
wandMode=0; u16 snapshot=PauseItemDesc_GetTextId(ITEM_ELEMENTAL_WAND,PAUSE_ITEM); wandMode=5;
bool table=true; OnOpenTextDescHook(&snapshot,&table); contains(shown,"Sand Rod","OoT lookup snapshots mode before opening");
shown.clear(); table=true; OnOpenTextDescHook(&snapshot,&table); check(shown.empty()&&table,"OoT consumed tutorial is not replayed by an unrelated hook");
state.pauseCtx.cursorX[PAUSE_EQUIP]=1;
'''
    code += r'''
for(const auto language:{ComboPauseTutorialText::Language::English,ComboPauseTutorialText::Language::German,ComboPauseTutorialText::Language::French}) {
    for(const char* name:{"Phantom Hourglass","Shadow Crystal"}) {
        const auto* receipt=ComboToolReceiptText::Find(name);
        const std::string original=language==ComboPauseTutorialText::Language::German ? receipt->german :
            language==ComboPauseTutorialText::Language::French ? receipt->french : receipt->english;
        const auto expected=original.substr(original.find('&')+1);
        const auto tutorial=ComboPauseTutorialText::Shared(name,language);
        check(tutorial.substr(tutorial.find('&')+1)==expected && tutorial.front()=='%' && tutorial.substr(0,tutorial.find('&')).find('!')==std::string::npos,
              "tool helper keeps colored heading and complete controls without localized grant wording");
        check(original.find("!&")!=std::string::npos,"pickup receipt retains grant introduction");
    }
}
static const char* rodNames[]={"Sand Rod","Tornado Rod","Water Rod","Meteor Rod","Storm Rod","Shadow Scepter"};
static const char* runeNames[]={"Rune: Remote Bomb","Rune: Stasis","Rune: Cryonis","Rune: Master Cycle","Rune: Sheikah Sensor"};
for(const auto language:{ComboPauseTutorialText::Language::English,ComboPauseTutorialText::Language::German,ComboPauseTutorialText::Language::French}) {
    for(int rule=0;rule<3;++rule) for(int mode=-1;mode<6;++mode) {
        const auto receipt=ComboMagicItemReceiptText::Body(*ComboMagicItemReceiptText::Find(mode<0 ? "Elemental Wand" : rodNames[mode],rule),language);
        const auto tutorial=ComboPauseTutorialText::Wand(mode,rule,language);
        check(tutorial.substr(tutorial.find('&')+1)==receipt.substr(receipt.find('&')+1) && tutorial.front()=='%' && tutorial.substr(0,tutorial.find('&')).find('!')==std::string::npos,
              "rod helper keeps heading, power, wheel controls and fallback guide without grant wording");
    }
    for(int active=-1;active<5;++active) {
        const auto receipt=ComboMagicItemReceiptText::Body(*ComboMagicItemReceiptText::Find(active<0 ? "Sheikah Slate" : runeNames[active]),language);
        const auto tutorial=ComboPauseTutorialText::Slate(active,language);
        check(tutorial.substr(tutorial.find('&')+1)==receipt.substr(receipt.find('&')+1) && tutorial.front()=='%' && tutorial.substr(0,tutorial.find('&')).find('!')==std::string::npos,
              "rune helper keeps heading, localized power and wheel controls without learned/grant wording");
    }
}
for(int mode=0;mode<6;++mode) {
    wandMode=mode;const auto tutorial=describe(ITEM_ELEMENTAL_WAND);
    check(tutorial.find("You got")==std::string::npos,"production scepter helper excludes grant wording");
}
for(int active=0;active<5;++active) {
    rune=active;const auto tutorial=describe(EXT_ITEM_SHEIKAH_SLATE);
    check(tutorial.find("You learned")==std::string::npos,"production rune helper excludes learned wording");
}
for(u16 id:{EXT_ITEM_PHANTOM_HOURGLASS,EXT_ITEM_SHADOW_CRYSTAL}) {
    const auto tutorial=describe(id);
    check(tutorial.find("You got")==std::string::npos,"production tool helper excludes pickup grant wording");
    contains(tutorial,"Equip to","production tool helper retains equip controls");
}
contains(describe(EXT_ITEM_PHANTOM_HOURGLASS),"recorded path", "Hourglass reuses full recorded-path tutorial");
contains(describe(EXT_ITEM_SHADOW_CRYSTAL),"bites", "Shadow Crystal reuses current combat tutorial");
wandMode=0; contains(describe(ITEM_ELEMENTAL_WAND),"temporary platform", "Sand Rod reaches power tutorial");
wandMode=5; contains(describe(ITEM_ELEMENTAL_WAND),"shadow bolt", "Shadow Scepter reaches power tutorial");
rune=1; contains(describe(EXT_ITEM_SHEIKAH_SLATE),"Stasis", "Slate follows active rune");
caneType=2; caneSkill=3; contains(describe(ITEM_CANE_OF_SOMARIA),"Flip", "Cane follows active Pacci skill");
caneSkill=4; contains(describe(ITEM_CANE_OF_SOMARIA),"Stone", "Cane follows active stone skill");
for(int mode=0;mode<6;++mode) {wandMode=mode;check(!describe(ITEM_ELEMENTAL_WAND).empty(),"every active rod has a tutorial");}
for(int active=0;active<5;++active) {rune=active;check(!describe(EXT_ITEM_SHEIKAH_SLATE).empty(),"every active rune has a tutorial");}
caneType=0;for(int active=0;active<3;++active) {caneSkill=active;check(!describe(ITEM_CANE_OF_SOMARIA).empty(),"every Somaria summon has a tutorial");}
caneType=3;caneSkill=5;contains(describe(ITEM_CANE_OF_SOMARIA),"Ultrahand","Ultrahand entry has its own tutorial");
check(describe(0xFFFF).empty(),"unknown item does not receive an unrelated description");
'''
    code += 'printf("%s pause tutorial checks: %d, failures: %d\\n", "' + host + '", checks, failures); return failures?1:0;}\n'
    return code


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    failed = False
    with tempfile.TemporaryDirectory(prefix="pause-tutorials-") as folder:
        for host in ("mm", "soh"):
            source, exe = Path(folder) / (host + ".cpp"), Path(folder) / host
            source.write_text(fixture(host))
            command = [os.environ.get("CXX", "c++"), "-std=c++20", "-I"+str(ROOT),
                       "-I"+str(ROOT/"combo/menu"), str(source), "-o", str(exe)]
            if args.sanitize:
                command += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-g"]
            subprocess.run(command, check=True)
            failed |= subprocess.run([str(exe)]).returncode != 0
        routing = [sys.executable, str(Path(__file__).with_name("run_routing_tests.py"))]
        if args.sanitize:
            routing.append("--sanitize")
        failed |= subprocess.run(routing).returncode != 0
    raise SystemExit(1 if failed else 0)


if __name__ == "__main__":
    main()
