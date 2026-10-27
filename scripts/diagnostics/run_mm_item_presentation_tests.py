#!/usr/bin/env python3
"""Execute the production cross-game GI selection boundary without either game runtime."""
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

def function(source, name):
    start = source.rfind('\n', 0, source.index(name + '(')) + 1
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

src = (ROOT / 'combo/menu/ComboItemDrawOOT.h').read_text()
song_body = function(src, 'OOT_FillSongDrawInfo')
body = function(src, 'OOT_FillItemDrawInfo')
collectible = (ROOT / 'combo/menu/ComboCollectibleShimmerHost.h').read_text()
body = function(collectible, 'ComboCollectibleShimmer_ProfileForDrawId') + '\n' + body
preamble = r'''
#include <atomic>
#define SPDLOG_WARN(...) ((void)0)
#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <iostream>
#include <cstring>
#include "combo/menu/ComboItemDrawABI.h"
#include "combo/menu/ComboCollectibleShimmer.h"
#include "soh/include/z64item.h"
#include "combo/menu/ComboItemEffectColors.h"
#include "combo/menu/ComboBottleContents.h"
using s16=int16_t; using s32=int32_t; using f32=float;
namespace Ship {
static bool ownerActive=false;
struct CrossRMRegistry { static int Get(const char* game) { assert(!strcmp(game,"oot")); return 1; } };
struct ResourceManagerScope {
 bool previous=ownerActive;
 ResourceManagerScope(int) { ownerActive=true; }
 ~ResourceManagerScope() { ownerActive=previous; }
};
}
#include <cstring>
#define RANDO_ENUM_BEGIN(x) enum x {
#define RANDO_ENUM_ITEM(x) x,
#define RANDO_ENUM_END(x) };
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h"
#include "combo/menu/ComboSongDrawOOT.h"
const char* gGiSongNoteDL="__OTR__objects/object_gi_melody/gGiSongNoteDL";
constexpr RandomizerGet RG_TEST_CUSTOM=static_cast<RandomizerGet>(RG_MAX+1),
 RG_TEST_NATIVE=static_cast<RandomizerGet>(RG_MAX+2),
 RG_TEST_PROGRESSIVE=static_cast<RandomizerGet>(RG_MAX+3),
 RG_TEST_TIER=static_cast<RandomizerGet>(RG_MAX+4);
constexpr int TABLE_RANDOMIZER=1;
struct GetItemEntry { int tableId, drawItemId, gid; void (*drawFunc)(); int itemId=0; };
void customDraw() {}
void Randomizer_DrawCaneOfSomaria() {}
void Randomizer_DrawCanePacci() {}
void Randomizer_DrawCaneSomariaUpgrade() {}
void Randomizer_DrawCanePacciUpgrade() {}
void Randomizer_DrawCanePacciUltrahand() {}
void (*awardDraw)()=customDraw;
int tableCalls=0; bool redesigned=false, effects=false;
int CVarGetInteger(const char*, int) { return effects; }
RandomizerGet described=RG_NONE;
namespace Rando::StaticData {
struct Name { std::string english="Awarded tier"; };
struct Item {
 RandomizerGet id;
 std::shared_ptr<GetItemEntry> GetGIEntry(RandomizerGet* actual) const {
  if(id==RG_TEST_PROGRESSIVE) *actual=RG_TEST_TIER;
  return std::make_shared<GetItemEntry>(GetItemEntry{TABLE_RANDOMIZER,id==RG_TEST_PROGRESSIVE?RG_TEST_TIER:id,
      id==RG_TEST_NATIVE?7:0, id==RG_TEST_NATIVE?nullptr:awardDraw});
 }
 const Name& GetName() const { static Name n; return n; }
};
Item RetrieveItem(RandomizerGet id) { return {id}; }
}
int OOT_DescribeCustomDraw(RandomizerGet id, CwItemDrawInfo*) { described=id;return 0; }
int NeiGi_DescribeEntry(const GetItemEntry*,CwItemDrawInfo* out) {
 if(!redesigned) return 0;
 out->drawKind=CW_DRAW_KIND_SIMPLE;out->dlists[0]="__OTR__objects/nei_gi_redesign/spinner/gi_dl";
 out->dlistCount=1;return 1;
}
int GetItem_GetDrawTableEntry(int,void** out,int,int*,float*,int*,uint8_t*) {
 assert(Ship::ownerActive);
 ++tableCalls;out[0]=(void*)"__OTR__native";return 1;
}
void GetItem_GetDrawSetupDLs(int,void**,void**) {}
int GetItem_GetShimmerColor(s16,uint8_t*) { return 0; }
void OOT_DescribeHeartCosmetics(s16,CwItemDrawInfo*) {}
void OOT_DescribeMagicJar(s16,CwItemDrawInfo*) {}
int CwAltSwordGi(RandomizerGet,CwItemDrawInfo*) {return 0;}
void ComboMaskShimmerColor(int,uint8_t*) {}
'''
checks = r'''
int main() {
 CwItemDrawInfo info{};
 assert(OOT_FillItemDrawInfo(RG_TEST_CUSTOM,&info)==0);
 assert(info.resolvedName && std::string(info.resolvedName)=="Awarded tier"); // self/base tiers also latch names
 assert(tableCalls==0); // custom callback must never consult bottle row zero
 redesigned=true;
 assert(OOT_FillItemDrawInfo(RG_TEST_CUSTOM,&info)==1);
 assert(tableCalls==0 && std::string(info.dlists[0]).find("spinner")!=std::string::npos);
 redesigned=false;
 assert(OOT_FillItemDrawInfo(RG_TEST_PROGRESSIVE,&info)==0);
 assert(described==RG_TEST_TIER && std::string(info.resolvedName)=="Awarded tier");
 assert(OOT_FillItemDrawInfo(RG_TEST_NATIVE,&info)==1 && tableCalls==1 && !Ship::ownerActive);
 void (*skills[])()={Randomizer_DrawCaneOfSomaria,Randomizer_DrawCanePacci,
   Randomizer_DrawCaneSomariaUpgrade,Randomizer_DrawCanePacciUpgrade,Randomizer_DrawCanePacciUltrahand};
 int expected[]={1,2,3,4,6};
 for(int i=0;i<5;i++) {
   awardDraw=skills[i]; info={};
   assert(OOT_FillItemDrawInfo(RG_TEST_CUSTOM,&info)==1);
   assert(info.drawKind==CW_DRAW_KIND_NEI_CANE && info.neiLegacyCane==expected[i]);
   assert(!info.itemShimmer);
   effects=true; info={};
   assert(OOT_FillItemDrawInfo(RG_TEST_CUSTOM,&info)==1 && info.itemShimmer);
   assert(info.itemShimmerColor[0]==255);
   assert(info.itemShimmerColor[1]==((i==0 || i==2) ? 60 : 215));
   effects=false;
   assert(tableCalls==1);
 }

 awardDraw=customDraw;
 for(auto id : {RG_MM_SONG_SONATA,RG_MM_SONG_TIME}) {
   info={};
   assert(OOT_FillItemDrawInfo(id,&info)==1);
   assert(info.drawKind==CW_DRAW_KIND_SONG_GI && info.dlistCount==1);
   assert(bool(info.itemShimmer)==(id==RG_MM_SONG_SONATA)); // Accepted POC2 intrinsic Sonata leaves.
   const uint8_t expected[4]={98,static_cast<uint8_t>(id==RG_MM_SONG_SONATA?255:177),
                             static_cast<uint8_t>(id==RG_MM_SONG_SONATA?98:211),255};
   assert(!std::memcmp(info.itemShimmerColor,expected,4));
 }
 info={};
 assert(OOT_FillItemDrawInfo(RG_MM_SONG_STORMS,&info)==1);
 assert(info.drawKind==CW_DRAW_KIND_SONG_GI && info.neiEffect==CW_SONG_STORMS && info.dlistCount==1 && info.itemShimmer);
 assert(!strcmp(info.dlists[0],gGiSongNoteDL) && info.primColorXlu[0]==146 && info.primColorXlu[1]==146 && info.primColorXlu[2]==146);
 const RandomizerGet warpIds[]={RG_MINUET_OF_FOREST,RG_BOLERO_OF_FIRE,RG_SERENADE_OF_WATER,
                               RG_REQUIEM_OF_SPIRIT,RG_NOCTURNE_OF_SHADOW,RG_PRELUDE_OF_LIGHT};
 const char* colorDls[]={"__OTR__objects/object_gi_melody/gGiMinuetColorDL","__OTR__objects/object_gi_melody/gGiBoleroColorDL",
                        "__OTR__objects/object_gi_melody/gGiSerenadeColorDL","__OTR__objects/object_gi_melody/gGiRequiemColorDL",
                        "__OTR__objects/object_gi_melody/gGiNocturneColorDL","__OTR__objects/object_gi_melody/gGiPreludeColorDL"};
 const uint32_t hues[]={0x62FF62,0xFF3C00,0x55B4DF,0xDE9E2F,0xA028D2,0xEDE73E};
 for(int i=0;i<6;++i) {
   info={};assert(OOT_FillItemDrawInfo(warpIds[i],&info)==1);
   assert(info.drawKind==CW_DRAW_KIND_SONG_GI && info.dlistCount==2 && info.xluStartIndex==0 && info.itemShimmer);
   assert(!strcmp(info.dlists[0],colorDls[i]) && !strcmp(info.dlists[1],gGiSongNoteDL));
   const uint8_t color[]={uint8_t(hues[i]>>16),uint8_t(hues[i]>>8),uint8_t(hues[i]),255};
   assert(!memcmp(info.itemShimmerColor,color,4));
 }
 assert(tableCalls==1);
 std::cout<<"PASS actual GI boundary: custom fallback cannot become bottle; resolved tier, native rows and imported song recipes preserved\n";
}
'''
with tempfile.TemporaryDirectory(prefix='mm-presentation-') as td:
    test=Path(td)/'test.cpp';test.write_text(preamble+song_body+body+checks)
    binary=Path(td)/'test'
    flags=['-std=c++20','-Wall','-Wextra','-I'+str(ROOT)]
    if '--sanitize' in __import__('sys').argv: flags += ['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g']
    subprocess.run([os.environ.get('CXX','c++'),*flags,str(test),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)

message = (ROOT / 'mm/src/code/z_message.c').read_text()
consumer = (ROOT / 'mm/2s2h/Rando/DrawItem.cpp').read_text()
fixture = (ROOT / 'tests/mm_presentation/icon_bridge_test.cpp').read_text()
fixture = "#include <atomic>\n#define SPDLOG_WARN(...) ((void)0)\n" + fixture
bindings = re.findall(r'itemTable\[(?:RG_BOTTLE_WITH_[A-Z_]+|RG_MAGIC_BEAN_PACK)\]\.CustomIcon\([^;]+;',
                      (ROOT / 'soh/soh/Enhancements/randomizer/item_list.cpp').read_text())
fixture = fixture.replace('/* BOTTLE_ICON_BINDINGS */', '\n'.join(bindings))
fixture = fixture.replace('/* OWNER_ICON */', function(src, 'OOT_FillItemIconInfo'))
fixture = fixture.replace('/* STAGE_ICON */', function(message, 'Message_StageCustomItemIconEx') + '\n' +
                          function(message, 'Message_StageCustomItemIcon') + '\n' +
                          function(message, 'Message_StageCustomItemIconTint'))
fixture = fixture.replace('/* CONSUMER_ICON */', function(consumer, 'Rando::ComboForeignMessageIcon'))
with tempfile.TemporaryDirectory(prefix='mm-icon-') as td:
    test=Path(td)/'test.cpp';test.write_text(fixture)
    binary=Path(td)/'test'
    subprocess.run([os.environ.get('CXX','c++'),*flags,'-I'+str(ROOT / 'soh/include'),str(test),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)

conversion = (ROOT / 'mm/2s2h/Rando/ConvertItem.cpp').read_text()
fixture = (ROOT / 'tests/mm_presentation/cane_progression_test.cpp').read_text()
fixture = fixture.replace('/* RESOLVE_CANE */', function(conversion, 'ResolveCanePresentation'))
fixture = fixture.replace('/* RAW_PREVIEW */', function(consumer, 'Rando::DrawItem'))
assert 'case RI_OOT_NEI_CANE_OF_SOMARIA:\n                return ResolveCanePresentation();' in conversion
queue = (ROOT / 'mm/2s2h/Rando/MiscBehavior/CheckQueue.cpp').read_text()
assert 'Rando::DrawResolvedItem(randoItemId, randoCheckId, actor);' in queue
with tempfile.TemporaryDirectory(prefix='mm-cane-') as td:
    test=Path(td)/'test.cpp';test.write_text(fixture)
    binary=Path(td)/'test'
    subprocess.run([os.environ.get('CXX','c++'),*flags,str(test),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
