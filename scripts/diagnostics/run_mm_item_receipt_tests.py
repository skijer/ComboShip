#!/usr/bin/env python3
"""Exercise MM's production queued receipt/grant boundary with engine seams replaced."""
from pathlib import Path
import os
import re
import sys
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
subprocess.run([sys.executable, '-B', str(ROOT / 'tests/item_receipts/run_bottle_contents_tests.py')], check=True)


def verify_windows_reward_export_contract():
    """MSVC requires the declaration and definition to agree on DLL linkage."""
    header = (ROOT / 'mm/2s2h/Rando/ItemReceiptText.h').read_text()
    source = (ROOT / 'mm/2s2h/Rando/ItemReceiptText.cpp').read_text()
    declaration = re.search(r'extern "C"[^;]*MM_GetDungeonRewardName\([^;]*;', header)
    definition = re.search(r'extern "C"[^\{]*MM_GetDungeonRewardName\([^\{]*\{', source)
    assert declaration and definition
    assert 'COMBO_EXPORT' in declaration.group(0), 'MM reward declaration lacks Windows DLL export linkage'
    assert 'COMBO_EXPORT' in definition.group(0), 'MM reward definition lacks Windows DLL export linkage'


verify_windows_reward_export_contract()
subprocess.run([sys.executable, str(ROOT / 'scripts/diagnostics/run_chest_game_key_receipt_tests.py')], check=True)


def block(source, start):
    begin = source.index('{', source.index(start))
    depth, end = 1, begin + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[begin:end]


source = (ROOT / 'mm/2s2h/Rando/MiscBehavior/CheckQueue.cpp').read_text()
give = block(source, '.giveItem =')
donor_catalog = (ROOT / 'soh/soh/Enhancements/randomizer/item_list.cpp').read_text()
donor_names = set(re.findall(r'itemTable\[RG_\w+\]\s*=\s*Item\(.*?Text\{\s*"([^"\n]*)"', donor_catalog))
concrete_names = set(re.findall(r'"([^"\n]*)"',
    block((ROOT / 'mm/2s2h/Rando/ItemReceiptText.cpp').read_text(), 'const char* ConcreteReceiptName')))
assert concrete_names <= donor_names, f'Unknown exact donor keys: {concrete_names - donor_names}'
preamble = r'''
#include "combo/ComboGameMode.h"
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <iostream>
#include "mm/2s2h/Rando/Types.h"
struct Actor {}; struct PlayState {};
namespace CustomItem { constexpr int GIVE_ITEM_CUTSCENE=1; }
int flags=1, param=RC_UNKNOWN, param2=0;
#define CUSTOM_ITEM_FLAGS flags
#define CUSTOM_ITEM_PARAM param
#define CUSTOM_ITEM_PARAM2 param2
struct Check { RandoItemId randoItemId=RI_UNKNOWN; bool obtained=false,cycleObtained=false,eligible=true; };
std::map<int,Check> checks;
#define RANDO_SAVE_CHECKS checks
#define RANDO_SAVE_OPTIONS options
std::map<int,int> options;
struct { struct { struct { struct { int foundTriforcePieces=0; } rando; } shipSaveInfo; } save; } gSaveContext;
int gMMComboGoalRequired=0;
int (*gMMComboOtherTriforceCount)()=nullptr;
int CVarGetInteger(const char*,int fallback) { return fallback; }
// This legacy receipt fixture has every enhancement off. The elemental-arrow
// suite exercises these pickup adapters with real game headers and inventories.
uint8_t SwordElement_MessageElementForPickup(int) { return 0; }
void SwordElement_MessageAppend(PlayState*,uint8_t) {}
int granted=0,delivered=0;
bool showCutscene=true;
bool donorAvailable=true;
const char* latchedName="Deku Leaf";
bool queued=true;
RandoCheckId queuedCheck = RC_UNKNOWN;
// Mirror token delivery is covered by the dedicated stump grant fixture.
bool MagicMirror_CommitStumpRewardToken() { assert(false); return false; }
namespace CustomMessage {
struct Entry { uint8_t textboxType=0,textboxYPos=0,icon=0xFE; uint16_t nextMessageID=0xFFFF,firstItemCost=0xFFFF,secondItemCost=0xFFFF; bool autoFormat=true; std::string msg; bool capeVisibilityChoice=false; uint8_t swordElementTutorial=0; };
Entry shown;
int shownCount=0;
void SetActiveCustomMessage(std::string msg,Entry e) { e.msg=msg;shown=e;++shownCount; }
void StartTextbox(std::string msg,Entry e) { e.msg=msg;shown=e;++shownCount; }
std::string RemoveColorCodes(std::string s) { return s; }
}
namespace Notification { struct Info { const char* itemIcon=nullptr;std::string message,suffix; };void Emit(Info){} }
namespace ComboRando {
struct ForeignItem { int itemGame=0;std::string itemName="Deku Leaf",displayName="Deku Leaf (OOT)";bool advancement=true,trap=false;std::string category="major"; };
std::string ShownForeignName(const ForeignItem&,const char* p) {return std::string(p)+" (OOT)";}
}
ComboRando::ForeignItem foreign;
void MMAnchor_BroadcastCrossItem(int,const char*,const char*) {}
void SaveManager_SaveCurrentForCombo() {}
namespace Rando {
void AppendReceiptSource(CustomMessage::Entry&,const std::string&);
namespace StaticData {
struct Item {RandoItemType randoItemType=RITYPE_MAJOR; int itemId=0;};
std::map<RandoItemId,Item> Items;
std::string GetItemName(RandoItemId id,bool=true,RandoCheckId=RC_UNKNOWN) {return id==RI_OOT_EXT_MAGIC_CAPE ? "the Magic Cape" : "the Deku Leaf";}
uint8_t GetIconForZMessage(RandoItemId) {return 0xF5;}
bool ShouldShowGetItemCutscene(RandoItemId) {return showCutscene;}
const char* GetIconTexturePath(RandoItemId) {return "icon";}
std::string GetCheckDisplayName(RandoCheckId) {return "test check";}
}
RandoItemId ConvertItem(RandoItemId i,RandoCheckId){return i;}
RandoItemId CurrentJunkItem(RandoCheckId){return RI_RUPEE_GREEN;}
void GiveItem(RandoItemId,RandoCheckId){++granted;}
void LatchComboForeign(RandoCheckId){}
const char* ComboForeignLatchedName(RandoCheckId){return latchedName;}
uint8_t ComboForeignMessageIcon(RandoCheckId){return 0xF5;}
namespace MiscBehavior {
std::string BankRewardSourceSuffix(RandoCheckId){return " (Bank reward)";}
const ComboRando::ForeignItem* MM_LookupForeign(RandoCheckId){return &foreign;}
bool ShouldShowForeignCutscene(RandoCheckId){return showCutscene;}
void OfferTrapItem(){}
void SendForeignCheck(RandoCheckId){++delivered;}
void BroadcastCheckObtainedIfFirst(RandoCheckId,RandoItemId,bool){}
std::string GetTrapMessage(){return "A trap!";}
}
// This seam stands in for the engine/catalog lookup, not for receipt selection.
bool ApplyItemReceiptText(RandoItemId id, CustomMessage::Entry& e) {
 if(id==RI_OOT_EXT_MAGIC_CAPE) {
   e.capeVisibilityChoice=true;
   if(!donorAvailable) return false;
   e.msg="You got the Magic Cape!";e.autoFormat=false;return true;
 }
 if(id!=RI_OOT_NEI_DEKU_LEAF) return false;
 e.msg="You got the Deku Leaf!\x10Use it to glide and blow gusts.";e.autoFormat=false;return true;
}
bool ApplyForeignItemReceiptText(const char* name,CustomMessage::Entry& e,RandoCheckId=RC_UNKNOWN) {
 if(std::string(name)=="Magic Cape") return ApplyItemReceiptText(RI_OOT_EXT_MAGIC_CAPE,e);
 if(std::string(name)!="Deku Leaf") return false;
 return ApplyItemReceiptText(RI_OOT_NEI_DEKU_LEAF,e);
}
}
'''
checks_source = r'''
int main(int argc,char** argv) {
 Actor actor; PlayState play;
 checks[param].randoItemId=RI_OOT_NEI_DEKU_LEAF;
 Apply(&actor,&play);
 assert(CustomMessage::shown.msg.find("glide")!=std::string::npos && "major item lost its full description");
 assert(CustomMessage::shown.icon==0xF5);
 assert(CustomMessage::shown.msg.find("Bank reward")!=std::string::npos);
 assert(granted==1 && checks[RC_UNKNOWN].obtained); // message change cannot swallow the grant
 param=RC_UNKNOWN;flags=1;checks[param]={RI_COMBO_FOREIGN};
 Apply(&actor,&play);
 assert(CustomMessage::shown.msg.find("glide")!=std::string::npos && "foreign OoT item lost its description");
 assert(delivered==1 && granted==1);
 param=RC_UNKNOWN;flags=1;checks[param]={RI_COMBO_FOREIGN};foreign.trap=true;
 Apply(&actor,&play);
 assert(CustomMessage::shown.msg.find("glide")==std::string::npos && "trap revealed its disguise description");
 assert(delivered==1);
 // Visibility is a required player choice even when all pickup animations are skipped.
 showCutscene=false;flags=0;checks[param]={RI_OOT_EXT_MAGIC_CAPE};
 const int shownBefore=CustomMessage::shownCount;
 Apply(&actor,&play);
 assert(CustomMessage::shownCount==shownBefore+1 && CustomMessage::shown.capeVisibilityChoice);
 assert(granted==2 && checks[RC_UNKNOWN].obtained);
 param=RC_UNKNOWN;checks[param]={RI_COMBO_FOREIGN};latchedName="Magic Cape";foreign.itemName="Magic Cape";foreign.trap=false;
 Apply(&actor,&play);
 assert(CustomMessage::shownCount==shownBefore+2 && CustomMessage::shown.capeVisibilityChoice);
 assert(delivered==2 && granted==2);
 param=RC_UNKNOWN;checks[param]={RI_COMBO_FOREIGN};foreign.trap=true;
 Apply(&actor,&play);
 assert(CustomMessage::shownCount==shownBefore+2 && "disguised cape trap opened a visibility prompt");
 donorAvailable=false;param=RC_UNKNOWN;checks[param]={RI_OOT_EXT_MAGIC_CAPE};
 Apply(&actor,&play);
 assert(CustomMessage::shown.capeVisibilityChoice && CustomMessage::shown.autoFormat);
 assert(CustomMessage::shown.msg.find('\x1C')==std::string::npos && "native fallback fades before the choice");
 assert(argc==3);
 std::ofstream(argv[1],std::ios::binary)<<CustomMessage::shown.msg;
 param=RC_UNKNOWN;checks[param]={RI_COMBO_FOREIGN};foreign.trap=false;
 Apply(&actor,&play);
 assert(CustomMessage::shown.capeVisibilityChoice && CustomMessage::shown.autoFormat);
 assert(CustomMessage::shown.msg.find('\x1C')==std::string::npos && "foreign fallback fades before the choice");
 std::ofstream(argv[2],std::ios::binary)<<CustomMessage::shown.msg;
 std::cout<<"queued native/foreign receipts, icons, bank attribution, grants and traps passed\n";
}
'''
with tempfile.TemporaryDirectory(prefix='mm-item-receipts-') as tmp:
    tmp = Path(tmp)
    compiler = os.environ.get('CXX', 'c++')
    extra = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-g'] if '--sanitizers' in sys.argv else []
    icon = (ROOT / 'tests/item_receipts/icon_test.cpp').read_text()
    icon_source = (ROOT / 'combo/menu/ComboItemDrawOOT.h').read_text()
    # Use each production catalog row's actual CustomIcon arguments. The
    # fixture replaces the catalog service, not the receipt's icon identity.
    asset_names = ((ROOT / 'soh/assets/soh_assets.h').read_text() +
                   (ROOT / 'soh/assets/textures/icon_item_24_static/icon_item_24_static.h').read_text())
    catalog_data = []
    for rg, name in [('RG_PIECE_OF_HEART', 'heart'), ('RG_EXT_PEGASUS_ANKLET', 'anklet')]:
        row = next(line for line in donor_catalog.splitlines() if 'itemTable[' + rg + '] =' in line)
        custom = re.search(r'\.CustomIcon\((\w+)(?:,\s*(ICON_SIZE_\d+))?\)', row)
        path, size = 'nullptr', '1'
        if custom:
            resource = re.search(r'#define d' + custom[1] + r' "([^"]+)"', asset_names)
            assert resource, (rg, custom[1], 'native asset name')
            path = '"' + resource[1] + '"'
            size = '0' if custom[2] == 'ICON_SIZE_24' else '1'
        catalog_data += [f'static const char* {name}CatalogIcon = {path};',
                         f'static int {name}CatalogIconSize = {size};']
    icon = icon.replace('/* CATALOG_ICON_DATA */', '\n'.join(catalog_data))
    icon = icon.replace('/* ICON_SELECTOR */',
        'static int32_t OOT_FillItemIconInfo(RandomizerGet rg,CwItemIconInfo* out,bool resolveProgressive = true) ' +
        block(icon_source, 'static int32_t OOT_FillItemIconInfo'))
    tu = tmp / 'icon.cpp'
    tu.write_text(icon)
    exe = tmp / 'icon'
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', *extra,
                    '-I', str(ROOT), str(tu), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
    foreign_icon = (ROOT / 'tests/item_receipts/foreign_icon_test.cpp').read_text()
    owner_header = ROOT / 'combo/menu/ComboItemIconOwnership.h'
    foreign_icon = foreign_icon.replace('/* ICON_OWNER_INCLUDE */',
        '#include "combo/menu/ComboItemIconOwnership.h"' if owner_header.exists() else '')
    foreign_icon = foreign_icon.replace('/* FOREIGN_ICON_SELECTOR */',
        'uint8_t Rando::ComboForeignMessageIcon(RandoCheckId check) ' +
        block((ROOT / 'mm/2s2h/Rando/DrawItem.cpp').read_text(), 'uint8_t Rando::ComboForeignMessageIcon'))
    tu = tmp / 'foreign_icon.cpp'
    tu.write_text(foreign_icon)
    exe = tmp / 'foreign_icon'
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', *extra,
                    '-I', str(ROOT), str(tu), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
    song = (ROOT / 'tests/item_receipts/song_icons_test.cpp').read_text()
    messages = (ROOT / 'soh/soh/Enhancements/randomizer/Messages/ItemMessages.cpp').read_text()
    song = song.replace('/* CUSTOM_ICON_FUNCTIONS */',
        'void LoadCustomItemIcon(bool displayAsEnglish) ' + block(messages, 'void LoadCustomItemIcon') + '\n' +
        'void DrawCustomItemIcon(Gfx** p) ' + block(messages, 'void DrawCustomItemIcon'))
    engine = (ROOT / 'soh/src/code/z_message_PAL.c').read_text()
    song = song.replace('/* ENGINE_ICON_DRAW */',
        'u16 Message_DrawItemIcon(PlayState* play,u16 itemId,Gfx** p,u16 i) ' +
        block(engine, 'u16 Message_DrawItemIcon'))
    mm_message = (ROOT / 'mm/src/code/z_message.c').read_text()
    song_palette = ''
    for channel in ('D_801CFE04', 'D_801CFE1C', 'D_801CFE34'):
        song_palette += 's16 ' + channel + '[] = ' + block(mm_message, 's16 ' + channel + '[]') + ';\n'
    song = song.replace('/* MM_SONG_PALETTE */', song_palette)
    native_load = block(mm_message, 'void Message_LoadItemIcon')
    stage_start = mm_message.index('#define MESSAGE_CUSTOM_ICON_ITEM')
    song = song.replace('/* MM_CUSTOM_STAGE */',
        mm_message[stage_start:mm_message.index('// #endregion', stage_start)])
    song = song.replace('/* MM_CUSTOM_LOAD */',
        block(native_load, 'if ((itemId == MESSAGE_CUSTOM_ICON_ITEM)').replace('return;', ''))
    song = song.replace('/* MM_SONG_LOAD */',
        block(native_load, '} else if ((itemId >= ITEM_SONG_SONATA)'))
    native_draw = block(mm_message, 'void Message_DrawItemIcon')
    song = song.replace('/* MM_CUSTOM_DRAW */',
        block(native_draw, '} else if ((msgCtx->itemId == MESSAGE_CUSTOM_ICON_ITEM)'))
    song = song.replace('/* MM_SONG_DRAW */',
        block(native_draw, '} else if ((msgCtx->itemId >= ITEM_SONG_SONATA)'))
    rect = native_draw.rindex('gSPTextureRectangle(')
    song = song.replace('/* MM_ICON_RECTANGLE */', native_draw[rect:native_draw.index(';', rect) + 1])
    tu = tmp / 'song.cpp'
    tu.write_text(song)
    exe = tmp / 'song'
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', *extra,
                    '-I', str(ROOT), str(tu), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
    layout = (ROOT / 'tests/item_receipts/layout_test.cpp').read_text()
    for tag, game_engine, font_name, font_type in (
        ('OOT', engine, 'sFontWidths', '144'),
        ('MM', mm_message, 'sNESFontWidths', '160')):
        font_engine = game_engine if tag == 'OOT' else (ROOT / 'mm/src/code/z_message_nes.c').read_text()
        layout = layout.replace('/* ' + tag + '_FONT_WIDTHS */',
            'float ' + font_name + '[' + font_type + '] = ' + block(font_engine, 'f32 ' + font_name) + ';')
        start = game_engine.index('static CwItemReceiptPresentation sItemReceiptPresentation;')
        end = game_engine.index('u16 Message_DrawItemIcon' if tag == 'OOT' else 'void Message_DrawItemIcon', start)
        layout = layout.replace('/* ' + tag + '_RECEIPT_RENDERER */', game_engine[start:end])
    nes = (ROOT / 'mm/src/code/z_message_nes.c').read_text()
    nes_draw = block(nes, 'void Message_DrawTextNES')
    begin = nes_draw.index('    msgCtx->textPosX =')
    end = nes_draw.index('    msgCtx->textColorR =', begin)
    dispatch = ('void Message_DrawTextNES(PlayState* play, Gfx** gfxP, u16 textDrawPos) {\n'
                'MessageContext* msgCtx = &play->msgCtx; Gfx* gfx = *gfxP; s16 sp130;\n' +
                nes_draw[begin:end] + '(void)sp130; (void)textDrawPos; *gfxP = gfx;\n}\n')
    dispatch += 'void Message_DrawText(PlayState* play, Gfx** gfxP) ' + block(mm_message, 'void Message_DrawText(')
    line_position = block(nes, '} else if ((curChar == MESSAGE_CARRIAGE_RETURN)')
    line_position = line_position[1:line_position.index('            spC6++;')]
    dispatch += ('\nvoid DecodeReceiptLine(int spC6, float spA4) {\n'
                 'MessageContext* msgCtx = &play.msgCtx;\n' + line_position + '}\n')
    space = nes_draw[nes_draw.index("            case ' ':") + len("            case ' ':"):]
    space = space[:space.index('                break;')]
    glyph = re.search(r'msgCtx->textPosX \+= \(s32\)\(sNESFontWidths\[\(u8\)character - \' \'\] \* msgCtx->textCharScale\);', nes_draw)[0]
    dispatch += ('\nint NativeLineWidth(const std::string& line) {\n'
                 'MessageContext* msgCtx = &play.msgCtx; msgCtx->textPosX = 0;\n'
                 # MM's color commands do not index the native glyph table.
                 'for (uint8_t character : line) { if (character <= 8) continue; if (character == \' \') {\n' + space +
                 '} else {\n' + glyph.replace('(s32)', '(int)').replace('(u8)', '(uint8_t)') +
                 '} } return msgCtx->textPosX; }\n')
    layout = layout.replace('/* MM_TEXT_DISPATCH */', dispatch)
    oot_decode = block(engine, 'void Message_Decode(PlayState* play)')
    oot_position = 'if (msgCtx->textBoxType != TEXTBOX_TYPE_NONE_BOTTOM) ' + block(
        oot_decode, 'if (msgCtx->textBoxType != TEXTBOX_TYPE_NONE_BOTTOM)')
    oot_position += '\nif (sItemReceiptPresentation.singleBox) ' + block(
        oot_decode, 'if (sItemReceiptPresentation.singleBox)')
    layout = layout.replace('/* OOT_DECODE_POSITION */',
        'void FinishReceiptDecodePosition(int numLines) { MessageContext* msgCtx = &play.msgCtx;\n'
        'R_TEXT_INIT_YPOS = R_TEXTBOX_Y + 8;\n' + oot_position + '\n}\n')
    mm_decode = block(mm_message, 'void Message_Decode(PlayState* play)')
    mm_position = 'if (curChar == MESSAGE_BOX_BREAK2) ' + block(nes, 'if (curChar == MESSAGE_BOX_BREAK2)')
    start = nes.index('if (curChar == MESSAGE_BOX_BREAK2)')
    remaining = nes[start + len(mm_position):]
    # Include native END positioning too, before the receipt-only override.
    mm_position += ' else ' + block(remaining, 'else {')
    mm_position += '\nif (sItemReceiptPresentation.singleBox) ' + block(
        mm_decode, 'if (sItemReceiptPresentation.singleBox)')
    layout = layout.replace('/* MM_DECODE_POSITION */',
        '#define XREG(i) ((i) == 13 ? 4 : (i) == 10 ? 22 : (i) == 11 ? 16 : 12)\n'
        'constexpr int MESSAGE_BOX_BREAK2 = 0x12, TEXTBOX_TYPE_3 = 3, TEXTBOX_TYPE_4 = 4;\n'
        'void FinishReceiptDecodePosition(int numLines) { MessageContext* msgCtx = &play.msgCtx;\n'
        'int curChar = 0xBF;\n' + mm_position + '\n}\n#undef XREG\n')
    tu = tmp / 'layout.cpp'
    tu.write_text(layout)
    exe = tmp / 'layout'
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', *extra,
                    '-I', str(ROOT), str(tu), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
    if '--layout-only' in sys.argv:
        raise SystemExit(0)
    magic = (ROOT / 'tests/item_receipts/magic_test.cpp').read_text()
    item_messages = (ROOT / 'soh/soh/Enhancements/randomizer/Messages/ItemMessages.cpp').read_text()
    magic = magic.replace('/* MAGIC_BUILDER */',
        'void BuildMagicStatUpgradeMessage(CustomMessage& msg) ' +
        block(item_messages, 'void BuildMagicStatUpgradeMessage(CustomMessage& msg) {'))
    tu = tmp / 'magic.cpp'
    tu.write_text(magic)
    exe = tmp / 'magic'
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', *extra,
                    '-I', str(ROOT), str(tu), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
    tu = tmp / 'queue.cpp'
    helpers = (ROOT / 'mm/2s2h/Rando/ItemReceiptText.cpp').read_text()
    append = block(helpers, 'void Rando::AppendReceiptSource')
    tu.write_text(preamble + '\nvoid Rando::AppendReceiptSource(CustomMessage::Entry& entry,const std::string& source) ' + append +
                  '\nnamespace Rando::MiscBehavior { void Apply(Actor* actor,PlayState* play) ' + give +
                  '\n}\nusing Rando::MiscBehavior::Apply;\n' + checks_source)
    exe = tmp / 'queue'
    subprocess.run([compiler, '-std=c++20', '-DCOMBO_BUILD', '-I', str(ROOT), *extra, str(tu), '-o', str(exe)], check=True)
    native_fallback, foreign_fallback = tmp / 'native-cape.bin', tmp / 'foreign-cape.bin'
    subprocess.run([str(exe), str(native_fallback), str(foreign_fallback)], check=True)

    subprocess.run([sys.executable, '-B', str(ROOT / 'tests/cape_choice/run_tests.py'),
                    '--queued-fallback', str(native_fallback), '--queued-fallback', str(foreign_fallback),
                    *(['--sanitize'] if '--sanitizers' in sys.argv else [])], check=True)
    codec = tmp / 'codec'
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(ROOT), *extra,
                    str(ROOT / 'tests/item_receipts/codec_test.cpp'), '-o', str(codec)], check=True)
    subprocess.run([str(codec)], check=True)

    host = (ROOT / 'combo/menu/ComboForeignDrawMM.h').read_text()
    cache = host[host.index('struct ComboForeignDrawCacheOOT {'):host.index('} // namespace')]
    latch = (ROOT / 'tests/item_receipts/latch_test.cpp').read_text().replace('/* FOREIGN_CACHE */', cache)
    tu = tmp / 'latch.cpp'
    tu.write_text(latch)
    exe = tmp / 'latch'
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(ROOT), *extra,
                    str(tu), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)

    # Compile the real MM builder with actual item/FC/vanilla-GI catalog data.
    items = (ROOT / 'mm/2s2h/Rando/StaticData/Items.cpp').read_text()
    macro = items[items.index('#define RI('):items.index('// clang-format off')]
    catalog = macro + '\nnamespace Rando::StaticData { std::map<RandoItemId,RandoStaticItem> Items = ' + \
        block(items, 'std::map<RandoItemId, RandoStaticItem> Items') + ';\n}\n#undef RI\n'
    message = (ROOT / 'mm/src/code/z_message.c').read_text()
    catalog += '#define MESSAGE_ITEM_NONE 9999\ns16 D_801CFF94[] = ' + block(message, 's16 D_801CFF94[]') + ';\n'
    for channel in ('D_801CFE04', 'D_801CFE1C', 'D_801CFE34'):
        catalog += 's16 ' + channel + '[] = ' + block(message, 's16 ' + channel + '[]') + ';\n'
    catalog += 'extern "C" void Message_StageCustomItemIcon(void*,s16);\n'
    catalog += 'extern "C" void Message_StageCustomItemIconTint(void*,s16,s16,u8,u8,u8,u8);\n'
    catalog += 'u8 Rando::StaticData::GetIconForZMessage(RandoItemId randoItemId) ' + \
        block(items, 'u8 GetIconForZMessage') + '\n'
    native = (ROOT / 'mm/src/overlays/actors/ovl_player_actor/z_player.c').read_text()
    catalog += 'struct NativeGetItemEntry { u8 itemId,field; s8 gid; u8 textId; u16 objectId; };\n'
    catalog += native[native.index('#define GIFIELD_'):native.index('GetItemEntry sGetItemTable')]
    catalog += '#define GET_ITEM(itemId, objectId, drawId, textId, field, chestAnim) { itemId,0,0,textId,0 }\n'
    catalog += 'NativeGetItemEntry sGetItemTable[GI_MAX-1] = ' + block(native, 'GetItemEntry sGetItemTable') + ';\n#undef GET_ITEM\n'
    catalog += 'extern "C" u16 Player_GetItemReceiptTextId(s16 getItemId,s16 itemId) ' + \
        block(native, 'u16 Player_GetItemReceiptTextId').replace('GetItemEntry', 'NativeGetItemEntry') + '\n'
    glue = (ROOT / 'mm/2s2h/FleetShipCombo/FleetComboItemsGlue.cpp').read_text()
    catalog += 'namespace { const int sFcNative[] = ' + block(glue, 'const int sFcNative[]') + ';\n'
    catalog += 'std::unordered_map<int,int>& ReverseMap() ' + block(glue, 'ReverseMap()') + '\n}\n'
    catalog += 'extern "C" int FcCombo_ItemForNative(int nativeId) ' + block(glue, 'int FcCombo_ItemForNative') + '\n'
    catalog += 'extern "C" int FcCombo_NativeForItem(int fcId) ' + block(glue, 'int FcCombo_NativeForItem') + '\n'
    mm_give = (ROOT / 'mm/2s2h/Rando/GiveItem.cpp').read_text()
    fairy_arms = mm_give[mm_give.index('        case RI_WOODFALL_STRAY_FAIRY:'):
                         mm_give.index('        case RI_GREAT_SPIN_ATTACK:')]
    fc_grants = 'int gFcCombo_SuppressRecord = 0;\n'
    fc_grants += 'void Rando::GiveItem(RandoItemId randoItemId, RandoCheckId) {\n' \
        'assert(gFcCombo_SuppressRecord > 0);\nswitch (randoItemId) {\n' + fairy_arms + \
        'default: assert(false && "unexpected non-fairy FC grant");\n}\n}\n'
    mm_sync = (ROOT / 'mm/2s2h/FleetShipCombo/FleetSync.cpp').read_text()
    fc_grants += '#pragma push_macro("SPDLOG_WARN")\n#undef SPDLOG_WARN\n#define SPDLOG_WARN(...) ((void)0)\n' \
        '#define FS_TRACE(...) ((void)0)\nvoid ApplyReceiptFcRegistry() ' + \
        block(mm_sync, 'void ApplyFcRegistryToNatives()').replace('spdlog::default_logger()->flush();', '(void)0;') + \
        '\n#undef FS_TRACE\n#pragma pop_macro("SPDLOG_WARN")\n'
    (tmp / 'receipt_fairy_grants.inc').write_text(fc_grants)
    registry = (ROOT / 'soh/mods/extended_player.c').read_text()
    leaf = re.search(r'RG_DEKU_LEAF,\s*((?:"(?:[^"\\]|\\.)*"\s*)+)', registry).group(1)
    catalog += 'const char* kDekuLeafMessage = ' + leaf + ';\n'
    mmregistry = (ROOT / 'mm/mods/extended_player.c').read_text()
    rows = block(mmregistry, 'static const NeiItem sNeiItems[]')[1:-1]
    catalog += 'const NeiItem sReceiptNeiItems[] = {\n'
    for row in re.findall(r'\{[^{}]*\}', rows, re.S):
        item = re.match(r'\{\s*(ITEM_\w+)', row)
        text = re.search(r'\b(RG_\w+|NEI_NO_RG),\s*((?:"(?:[^"\\]|\\.)*"\s*)+)', row)
        if item and text:
            catalog += '{ .item = ' + item.group(1) + ', .rg = ' + text.group(1) + ', .nameEn = ' + text.group(2) + ' },\n'
    catalog += '};\n'
    inventory = (ROOT / 'mm/src/code/z_inventory.c').read_text()
    for declaration in ('u32 gUpgradeMasks', 'u8 gUpgradeShifts'):
        catalog += 'extern "C" ' + declaration + '[8] = ' + block(inventory, declaration) + ';\n'
    catalog += 'const char* Rando::StaticData::GetIconTexturePath(RandoItemId randoItemId) ' + block(items, 'const char* GetIconTexturePath') + '\n'
    catalog += 'extern "C" { TexturePtr gItemIcons[131] = ' + re.sub(r'(?m)^(\s*)(g\w+),', r'\1(TexturePtr)\2,', block(inventory, 'TexturePtr gItemIcons[]')) + '; }\n'
    font_source = (ROOT / 'mm/src/code/z_message_nes.c').read_text()
    catalog += 'extern "C" { float sNESFontWidths[160] = ' + block(font_source, 'f32 sNESFontWidths[160]') + '; }\n'
    # Run the actual story-skip grant lambdas against the real receipt builder.
    grant_rows = (
        ('SkipLearningSongOfSoaring.cpp', 'RI_SONG_SOARING', 'ITEM_SONG_SOARING', 'statue', 'statue', 'statue'),
        ('SkipLearningSongOfHealing.cpp', 'RI_SONG_HEALING', 'ITEM_SONG_HEALING', 'masks', 'masques', 'Masken'),
        ('SkipLearningSongOfTime.cpp', 'RI_SONG_TIME', 'ITEM_SONG_TIME', 'Termina', 'Termina', 'Termina'),
        ('SkipLearningSongOfStorms.cpp', 'RI_SONG_STORMS', 'ITEM_SONG_STORMS', 'thunder', 'tonnerre', 'Donner'),
        ('SkipLearningEponasSong.cpp', 'RI_SONG_EPONA', 'ITEM_SONG_EPONA', 'horse', 'cheval', 'Pferd'),
    )
    grants = ''
    for index, (filename, ri, item, *descriptions) in enumerate(grant_rows):
        text = (ROOT / 'mm/2s2h/Enhancements/Cutscenes/StoryCutscenes' / filename).read_text()
        grants += f'void GiveStorySong{index}(Actor* actor, PlayState* play) ' + block(text, '.giveItem =') + '\n'
    grants += 'void CheckDirectSongGrants() { Actor actor{};\n'
    for index, (filename, ri, item, *descriptions) in enumerate(grant_rows):
        for lang, detail in zip(('LANGUAGE_ENG', 'LANGUAGE_FRE', 'LANGUAGE_GER'), descriptions):
            grants += f'for(bool cutscene : {{false,true}}) {{ actor.home.rot.x=cutscene?CustomItem::GIVE_ITEM_CUTSCENE:0; gSaveContext.options.language={lang}; const int before=songGrants; GiveStorySong{index}(&actor,gPlayState);\n'
            grants += f'assert(songGrants==before+1 && grantedSong=={item}); assert(CustomMessage::startedSong==!cutscene);\n'
            grants += f'const auto& shown=CustomMessage::shownSong; assert(!shown.autoFormat && shown.icon==Rando::StaticData::GetIconForZMessage({ri}) && shown.textboxType==2);\n'
            grants += f'assert(shown.msg.find("{detail}")!=std::string::npos && shown.msg.back()==char(0xBF) && shown.msg.find(char(0x1C))==std::string::npos); }}\n'
    grants += 'gSaveContext.options.language=LANGUAGE_ENG; }\n'
    (tmp / 'receipt_song_grants.inc').write_text(grants)
    (tmp / 'receipt_catalogs.inc').write_text(catalog)
    rupee_hooks = ROOT / 'mm/2s2h/Enhancements/Dialogue/RandomRupeeNames.cpp'
    rupee_code = 'void RegisterNativeRandomRupeeNames() {}\n'
    if rupee_hooks.exists():
        rupee_source = rupee_hooks.read_text()
        rupee_code = 'void BuildNativeRandomRupeeName(u16* textId,bool* loadFromMessageTable) ' + \
            block(rupee_source,'void BuildNativeRandomRupeeName') + '\n'
        rupee_code += 'void RegisterNativeRandomRupeeNames() ' + block(rupee_source,'void RegisterNativeRandomRupeeNames')
    (tmp / 'receipt_rupee_hooks.inc').write_text(rupee_code)
    pause_desc = (ROOT / 'mm/2s2h/CustomMessage/PauseItemDescriptions.cpp').read_text()
    (tmp / 'receipt_map_pause.inc').write_text('extern "C" const char* PauseItemDesc_GetMapInfo(s32 dungeon,u16 itemId) ' +
                                             block(pause_desc, 'const char* PauseItemDesc_GetMapInfo'))
    mmflags = ['-DCOMBO_BUILD', '-DMM_BUILD_DLL', '-DF3DEX_GBI_2', '-DCONTROLLERBUTTONS_T=uint32_t',
               '-DNON_EQUIVALENT', '-DNON_MATCHING']
    mmflags += ['-I' + str(ROOT)]
    mmflags += ['-I' + str(ROOT / p) for p in ('mm/include', 'mm/include/PR', 'mm/src', 'mm', 'mm/2s2h',
                 'mm/assets', 'libultraship/include', 'libultraship/src', 'combo', 'combo/menu')]
    linked = tmp / 'catalog'
    result = subprocess.run([compiler, '-std=c++20', *mmflags, *extra, '-I' + str(tmp),
                            str(ROOT / 'tests/item_receipts/catalog_test.cpp'),
                            str(ROOT / 'mm/2s2h/Rando/ItemReceiptText.cpp'), '-rdynamic', '-ldl', '-o', str(linked)],
                            capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    subprocess.run([str(linked)], check=True)
    if '--catalog-only' in sys.argv:
        raise SystemExit(0)

    # Execute the actual exported donor builder, with the authored descriptions.
    donor = (ROOT / 'tests/item_receipts/donor_test.cpp').read_text()
    enum_gets = re.findall(r'RANDO_ENUM_ITEM\((RG_\w+)\)',
                          (ROOT / 'soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h').read_text())
    groups = (('RG_ZELDAS_LULLABY', 'RG_PRELUDE_OF_LIGHT', None),
              ('RG_DEKU_TREE_MAP', 'RG_ICE_CAVERN_MAP', 0x66),
              ('RG_DEKU_TREE_COMPASS', 'RG_ICE_CAVERN_COMPASS', 0x67),
              ('RG_FOREST_TEMPLE_SMALL_KEY', 'RG_TREASURE_GAME_SMALL_KEY', 0x60),
              ('RG_FOREST_TEMPLE_BOSS_KEY', 'RG_GANONS_CASTLE_BOSS_KEY', 0xC7))
    traditional = 'const TraditionalReceiptFixture traditionalReceipts[] = {\n'
    for first, last, expected_text in groups:
        for rg in enum_gets[enum_gets.index(first):enum_gets.index(last) + 1]:
            row = re.search(r'itemTable\[' + rg + r'\]\s*=\s*Item\([^\n]+', donor_catalog).group(0)
            name = re.search(r'Text\{\s*("(?:[^"\\]|\\.)*")', row).group(1)
            native_text = re.search(r'GID_\w+,\s*(0x[0-9A-Fa-f]+|TEXT_\w+)', row).group(1)
            if native_text == 'TEXT_ITEM_DUNGEON_MAP':
                native_text = '0x66'
            elif native_text == 'TEXT_ITEM_COMPASS':
                native_text = '0x67'
            expected = '0xF3' if rg == 'RG_TREASURE_GAME_SMALL_KEY' else native_text if expected_text is None else hex(expected_text)
            traditional += f'{{{rg}, {name}, {native_text}, {expected}}},\n'
    traditional += '};\n'
    donor = donor.replace('/* TRADITIONAL_CATALOG */', traditional)
    # Use the production key names/IDs, independently of the expected palette
    # fixture. This also covers MM keys and existing OoT key rings.
    keys = 'const KeyCatalogFixture keyCatalog[] = {\n'
    for row in donor_catalog.splitlines():
        key = re.search(r'itemTable\[(RG_\w+)\]\s*=\s*Item\(', row)
        name = re.search(r'Text\{\s*("(?:[^"\\]|\\.)*")', row)
        if key and name and name[1].endswith(('Small Key"', 'Boss Key"', 'Key Ring"')):
            keys += f'{{{key[1]}, {name[1]}}},\n'
    keys += '};\n'
    donor = donor.replace('/* KEY_CATALOG */', keys)
    descriptions = 'const CustomItemMessageEntry receiptMessages[] = {\n'
    randomizer_messages = (ROOT / 'soh/soh/Enhancements/randomizer/randomizer.cpp').read_text()
    # Copy the real shared-tool rows, including their icon and all locales.
    for rg in ('RG_PHANTOM_HOURGLASS', 'RG_SHADOW_CRYSTAL'):
        row = re.search(r'\{\s*' + rg + r',\s*[^,]+,\s*ComboToolReceiptText::.*?\}',
                        randomizer_messages, re.S)
        assert row, f'Missing shared tool receipt: {rg}'
        descriptions += row.group(0) + ',\n'
    for rg in ('RG_SKULL_MASK', 'RG_SPOOKY_MASK', 'RG_MASK_OF_TRUTH', 'RG_MM_MASK_TRUTH', 'RG_GERUDO_MASK',
               'RG_KEATON_MASK', 'RG_MM_MASK_KEATON'):
        row = re.search(r'\{\s*' + rg + r',\s*[^,]+,\s*ComboMaskReceiptText::.*?\}',
                        randomizer_messages, re.S)
        assert row, f'Missing shared mask receipt: {rg}'
        descriptions += row.group(0) + ',\n'
    for rg in ('RG_CANE_OF_SOMARIA', 'RG_PROGRESSIVE_ROCS', 'RG_CANE_PACCI_FLIP',
               'RG_ROCS_CAPE', 'RG_QUARTZ_OF_MOTION', 'RG_DEKU_LEAF',
               'RG_MM_STRAY_FAIRY', 'RG_MM_STRAY_FAIRY_WOODFALL', 'RG_MM_STRAY_FAIRY_SNOWHEAD',
               'RG_MM_STRAY_FAIRY_GREAT_BAY', 'RG_MM_STRAY_FAIRY_STONE_TOWER',
               'RG_MM_REMAINS_GOHT', 'RG_MM_SONG_LULLABY', 'RG_MM_SONG_LULLABY_INTRO', 'RG_MM_SONG_NOVA',
               'RG_MM_SONG_HEALING', 'RG_MM_SONG_STORMS', 'RG_MM_SONG_SOARING'):
        text = re.search(r'\{\s*' + rg + r',.*?,\s*((?:"(?:[^"\\]|\\.)*"\s*)+)',
                         randomizer_messages, re.S)
        if not text:
            text = re.search(r'\b' + rg + r',\s*((?:"(?:[^"\\]|\\.)*"\s*)+)', registry)
        descriptions += '{' + rg + ', 0, ' + text.group(1) + ', nullptr, nullptr},\n'
    descriptions += '};\n'
    donor = donor.replace('/* DONOR_MESSAGES */', descriptions)
    formatter = (ROOT / 'soh/soh/Enhancements/custom-message/CustomMessageManager.cpp').read_text()
    actual_formatter = "static const std::unordered_map<std::string, std::string> percentColors = {\n" \
        "{\"w\", std::string(1, '\\x00')}, {\"r\", \"\\x41\"}, {\"g\", \"\\x42\"}, {\"b\", \"\\x43\"}," \
        "{\"c\", \"\\x44\"}, {\"p\", \"\\x45\"}, {\"y\", \"\\x46\"}, {\"B\", \"\\x47\"}};\n" \
        "static const std::map<std::string, std::string> colorToPercent;\n" \
        "static const std::map<std::string, uint8_t> altarIcons;\n"
    for signature in ('static const std::unordered_map<std::string, char> textBoxSpecialCharacters',
                      'static std::map<std::string, int> pixelWidthTable'):
        actual_formatter += signature + ' = ' + block(formatter, signature) + ';\n'
    for signature in ('static size_t NextLineLength(const std::string* textStr, const size_t lastNewline, bool hasIcon = false)',
                      'void CustomMessage::FormatString(std::string& str) const',
                      'void CustomMessage::AutoFormatString(std::string& str) const',
                      'size_t CustomMessage::FindNEWLINE(std::string& str, size_t lastNewline) const',
                      'bool CustomMessage::AddBreakString(std::string& str, size_t pos, std::string breakString) const',
                      'void CustomMessage::ReplaceSpecialCharacters(std::string& str) const',
                      'void CustomMessage::ReplaceColors(std::string& str) const',
                      'void CustomMessage::ReplaceAltarIcons(std::string& str) const',
                      'void CustomMessage::EncodeColors(std::string& str) const'):
        actual_formatter += signature + ' ' + block(formatter, signature) + '\n'
    donor = donor.replace('/* ACTUAL_FORMATTER */', actual_formatter)
    oot_glue = (ROOT / 'soh/soh/FleetShipCombo/FleetComboItemsGlue.cpp').read_text()
    grant_source = (ROOT / 'soh/soh/Enhancements/randomizer/randomizer.cpp').read_text()
    grant = block(grant_source, 'u16 Randomizer_Item_Give')
    grant_record = 'namespace { const int sFcNative[] = ' + block(oot_glue, 'const int sFcNative[]') + ';\n'
    grant_record += 'std::unordered_map<int,int>& ReverseMap() ' + block(oot_glue, 'ReverseMap()') + '\n}\n'
    grant_record += 'extern "C" int FcCombo_ItemForNative(int nativeId) ' + block(oot_glue, 'int FcCombo_ItemForNative') + '\n'
    grant_record += 'void RecordReceiptGrant(RandomizerGet item) { if (!FleetSync_IsApplyingFc()) ' + \
        block(grant, 'if (!FleetSync_IsApplyingFc())') + '\n}\n'
    donor = donor.replace('/* NATIVE_GRANT_RECORD */', grant_record)
    exported = (ROOT / 'soh/soh/Enhancements/randomizer/Messages/ItemMessages.cpp').read_text()
    if '--compass-baseline' in sys.argv:
        revision = sys.argv[sys.argv.index('--compass-baseline') + 1]
        exported = subprocess.check_output(['git', 'show', revision + ':soh/soh/Enhancements/randomizer/Messages/ItemMessages.cpp'],
                                           cwd=ROOT, text=True)
    context_builders = 'bool BuildDungeonItemReceiptMessage(RandomizerGet rg, CustomMessage& msg, bool received = true);\n'
    for signature in ('bool BuildDungeonKeyReceiptMessage(RandomizerGet rg, CustomMessage& msg)',
                      'static bool BuildVanillaEquipmentBottleReceipt(RandomizerGet rg, CustomMessage& msg)',
                      'static bool WandMedallionDescriptionsEnabled()',
                      'static bool DungeonInformationEnabled()',
                      'extern "C" COMBO_EXPORT int32_t OOT_MapCompassInfoEnabled(void)',
                      'static CustomMessage DungeonRewardName(RandomizerCheck check)',
                      'static int16_t DungeonEntranceDestination(int16_t entrance)',
                      *(['static int DungeonBossDestination(int dungeon)'] if 'static int DungeonBossDestination' in exported else []),
                      'static std::string DungeonPhysicalEntranceName(int16_t entrance)',
                      'static std::string DungeonEntranceSource(int16_t dungeonEntrance)',
                      'static void AddDungeonRewardIcon(CustomMessage& msg, RandomizerCheck check)',
                      *(['static RandomizerCheck PhysicalBossRewardCheck(int32_t boss)',
                         'extern "C" COMBO_EXPORT int32_t OOT_GetDungeonBossIndex(int32_t entrance)',
                         'extern "C" COMBO_EXPORT int32_t OOT_GetNativeBossHintTarget(int32_t dungeon, int32_t* entrance, int32_t* nextDungeon)',
                         'extern "C" COMBO_EXPORT int32_t OOT_GetDungeonEntranceSourceName(int32_t entrance, char* buffer, uint32_t capacity)',
                         'extern "C" COMBO_EXPORT int32_t OOT_GetPhysicalBossRewardName(int32_t boss, char* buffer, uint32_t capacity)',
                         'extern "C" COMBO_EXPORT int32_t OOT_GetPhysicalBossRewardPresentation(int32_t boss, CwItemReceiptPresentation* out)']
                        if 'PhysicalBossRewardCheck' in exported else []),
                      'bool BuildDungeonItemReceiptMessage(RandomizerGet rg, CustomMessage& msg, bool received)',
                      'bool BuildTokenReceiptMessage(RandomizerGet rg, CustomMessage& msg)',
                      'extern "C" COMBO_EXPORT int32_t OOT_GetDungeonItemReceiptPresentation(const char* itemName, CwItemReceiptPresentation* out)',
                      'extern "C" uint16_t Randomizer_GetDungeonItemInfoTextId(uint16_t cursorItem)',
                      'void BuildDungeonPauseInfoMessage(uint16_t* textId, bool* loadFromMessageTable)',
                      'void BuildMapMessage(uint16_t* textId, bool* loadFromMessageTable)',
                      'static bool SplitWandMedallionMessage(std::string& prefix, std::string& ending, int& icon)',
                      'void BuildWandMedallionMessage(uint16_t* textId, bool* loadFromMessageTable)'):
        context_builders += signature + ' ' + block(exported, 'int32_t OOT_GetDungeonItemReceiptPresentation' if 'OOT_GetDungeonItemReceiptPresentation' in signature else signature + ' {') + '\n'
    donor = donor.replace('/* CONTEXT_BUILDERS */', context_builders)
    donor = donor.replace('/* NATIVE_ITEM_BUILDER */',
        'void BuildCustomItemMessage(Player* player, CustomMessage& msg) ' +
        block(exported, 'void BuildCustomItemMessage'))
    donor = donor.replace('/* FOREIGN_BUILDER */',
        'void BuildComboForeignMessage(Player* player,CustomMessage& msg) ' +
        block(exported, 'void BuildComboForeignMessage'))
    donor = donor.replace('/* DONOR_EXPORT */',
        'extern "C" COMBO_EXPORT int32_t OOT_GetItemReceiptText(const char* itemName,char* buffer,uint32_t capacity) ' +
        block(exported, 'int32_t OOT_GetItemReceiptText'))
    tu = tmp / 'donor.cpp'
    tu.write_text(donor)
    exe = tmp / 'donor'
    # Text's native RNG moved to ShipUtils in 9.3. Keep its real implementation
    # and native headers in separate TUs, away from the donor engine fixtures.
    ship_utils = (ROOT / 'soh/soh/ShipUtils.cpp').read_text()
    rng = tmp / 'ship_rng.cpp'
    rng.write_text('#include <bit>\n#include <random>\n#include <cassert>\n'
                   '#include "soh/ShipUtils.h"\n' +
                   ship_utils[ship_utils.index('static bool default_init = false;'):
                              ship_utils.index('// Returns a random floating point number')])
    native_flags = ['-DF3DEX_GBI_2', '-I', str(ROOT / 'soh'),
                    '-I', str(ROOT / 'soh/include'), '-I', str(ROOT / 'libultraship/include')]
    native_objects = []
    for native_source in (ROOT / 'soh/soh/Enhancements/custom-message/text.cpp', rng):
        native_object = tmp / (native_source.stem + '.o')
        subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror',
                        '-Wno-unused-parameter', '-fPIC', *native_flags, *extra,
                        '-c', str(native_source), '-o', str(native_object)], check=True)
        native_objects.append(str(native_object))
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter', '-I', str(ROOT), *extra,
                    str(tu), *native_objects, '-o', str(exe)], check=True)
    generated_routes = tmp / 'native-entrance-routes.json'
    subprocess.run([sys.executable, str(ROOT / 'tests/item_receipts/run_seed_settings_tests.py'),
                    '--dump-entrance-fixture', str(generated_routes),
                    *(['--sanitizers'] if '--sanitizers' in sys.argv else [])], check=True)
    subprocess.run([str(exe), str(generated_routes)], check=True)

    # Link the real donor export to the real MM native/foreign receipt routes.
    # Private engine fixtures remain separate, as the two DLLs are in-game.
    donor_library = tmp / 'libcompass_donor.so'
    donor_private = ['-DCOMPASS_DONOR_BRIDGE', '-Dmain=OotFixtureTestMain', '-DgSaveContext=ootFixtureSaveContext',
                     '-DgPlayState=ootFixturePlayState', '-DNei_Save=ootFixtureNeiSave',
                     '-DRando=OotFixtureRando', '-DCustomMessage=OotFixtureCustomMessage',
                     '-DComboRando=OotFixtureComboRando', '-DCombo_ResolveSym=OotFixtureResolveSym',
                     '-DCOMBO_EXPORT=__attribute__((visibility("default")))']
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter',
                    '-fPIC', '-shared', '-fvisibility=hidden',
                    *donor_private, *extra, '-I', str(ROOT), str(tu),
                    *native_objects, '-o', str(donor_library)], check=True)
    integrated = tmp / 'compass_receiver'
    result = subprocess.run([compiler, '-std=c++20', *mmflags, *extra, '-DCOMPASS_DONOR_INTEGRATION', '-I' + str(tmp),
                    str(ROOT / 'tests/item_receipts/catalog_test.cpp'),
                    str(ROOT / 'mm/2s2h/Rando/ItemReceiptText.cpp'),
                    '-L' + str(tmp), '-lcompass_donor', '-Wl,-rpath,' + str(tmp), '-rdynamic', '-ldl',
                    '-o', str(integrated)], capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    subprocess.run([str(integrated), str(generated_routes)], check=True)

    information = (ROOT / 'tests/item_receipts/information_test.cpp').read_text()
    hint = (ROOT / 'soh/soh/Enhancements/randomizer/hint.cpp').read_text()
    save = (ROOT / 'soh/soh/SaveManager.cpp').read_text()
    save_header = (ROOT / 'soh/soh/SaveManager.h').read_text()
    otr = (ROOT / 'soh/soh/OTRGlobals.cpp').read_text()
    information = information.replace('/* ALTAR_BUILDER */',
        'const CustomMessage Hint::GetHintMessage(MessageFormat format,size_t id) const ' + block(hint, 'const CustomMessage Hint::GetHintMessage'))
    information = information.replace('/* SETTINGS_RESTORE */',
        'extern "C" void SOH_RestoreRandoSettings(const char* json) ' + block(otr, 'void SOH_RestoreRandoSettings'))
    information = information.replace('/* CLEAR_NUMERIC_EXCLUSIONS */',
        'static bool Combo_ClearNumericExclusions() ' + block(otr, 'static bool Combo_ClearNumericExclusions() {'))
    information = information.replace('/* ALTAR_EXPORT */',
        'extern "C" const char* SOH_DumpAltarHintMessages(void) ' + block(otr, 'const char* SOH_DumpAltarHintMessages'))
    information = information.replace('/* LOAD_DATA */', block(
        save_header[save_header.index('template <typename T> void LoadData'):], ') {\n        if (name == "")'))
    information = information.replace('/* LOAD_ARRAY */',
        'void SaveManager::LoadArray(const std::string& name,const size_t size,LoadArrayFunc func) ' + block(save, 'void SaveManager::LoadArray'))
    information = information.replace('/* LOAD_SETTINGS */',
        'SaveManager::Instance->LoadArray("randoSettings",RSK_MAX,[&](size_t i) ' + block(save, 'SaveManager::Instance->LoadArray("randoSettings"') + ');')
    settings = (ROOT / 'soh/soh/Enhancements/randomizer/settings.cpp').read_text()
    option = (ROOT / 'soh/soh/Enhancements/randomizer/option.cpp').read_text()
    metadata = ''
    for signature, text in (
            ('std::shared_ptr<Settings> Settings::GetInstance()', settings),
            ('const std::array<Option, RSK_MAX>& Settings::GetAllOptions() const', settings),
            ('const std::string& Option::GetCVarName() const', option)):
        metadata += signature + ' ' + block(text, signature) + '\n'
    information = information.replace('/* OPTION_METADATA */', metadata)
    information = information.replace('/* SPOILER_SETTINGS */',
        block(settings, 'void Settings::ParseJson')[1:].split('    nlohmann::json jsonExcludedLocations')[0])
    tu = tmp / 'information.cpp'
    tu.write_text(information)
    exe = tmp / 'information'
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', *extra,
                    '-I', str(ROOT), str(tu), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)

    tracker = (ROOT / 'tests/item_receipts/tracker_test.cpp').read_text()
    launcher = (ROOT / 'combo/ComboShip.cpp').read_text()
    tracker = tracker.replace('/* TRACKER_PUSH */', 'static void Combo_PushHintTrackerData(int slot) ' +
                              block(launcher, 'static void Combo_PushHintTrackerData'))
    tu = tmp / 'tracker.cpp'
    tu.write_text(tracker)
    exe = tmp / 'tracker'
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', *extra,
                    '-I', str(ROOT), '-I', str(ROOT / 'combo'), str(tu), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
