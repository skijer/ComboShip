#!/usr/bin/env python3
"""Exercise the actual shuffled Chest Game textbox registration and handler."""
from pathlib import Path
import os
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    start = source.index(signature)
    begin = source.index('{', start)
    depth, end = 1, begin + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


source = (ROOT / 'soh/soh/Enhancements/randomizer/Messages/ItemMessages.cpp').read_text()
catalog = (ROOT / 'soh/soh/Enhancements/randomizer/item_list.cpp').read_text()
row = re.search(r'itemTable\[RG_TREASURE_GAME_SMALL_KEY\]\s*=\s*Item\(([^\n]+)', catalog)[1]
entry = re.search(r'ITEMTYPE_SMALLKEY,\s*(\w+),.*?RHT_TREASURE_GAME_SMALL_KEY,\s*(\w+),\s*(\w+),\s*\w+,\s*(\w+),.*?ITEM_CATEGORY_SMALL_KEY,\s*(\w+)', row)
assert entry, 'Cannot recover the production Chest Game get-item entry'
gi, item, obj, text, mod = entry.groups()
assert (item, text, mod) == ('RG_TREASURE_GAME_SMALL_KEY', 'TEXT_RANDOMIZER_CUSTOM_ITEM', 'MOD_RANDOMIZER')
assert '.CustomIcon(gQuestIconSmallKeyTex, ICON_SIZE_24)' in row
fixture = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <type_traits>
// Windows RPC headers define this before MM includes the receipt helper.
#define small char
#include "combo/menu/ComboDungeonKeyReceipt.h"
#include "combo/menu/ComboKeyReceiptText.h"
static_assert(std::is_same_v<small, char>, "Receipt headers must preserve the Windows SDK macro");
constexpr int TEXTBOX_TYPE_BLUE=2, ITEM_KEY_SMALL=0x77, GI_DOOR_KEY=0x71,
    OBJECT_GI_KEY=1, OBJECT_INVALID=-1, MOD_NONE=0, MOD_RANDOMIZER=1;
enum RandomizerGet { RG_NONE, RG_TREASURE_GAME_SMALL_KEY };
enum { RSK_SHUFFLE_CHEST_MINIGAME, RSK_GERUDO_KEYS, RSK_SHUFFLE_MAPANDCOMPASS,
    RSK_BOSS_KEYSANITY, RSK_GANONS_BOSS_KEY, RSK_KEYSANITY };
enum { RO_GENERIC_OFF, RO_GENERIC_ON, RO_GERUDO_KEYS_VANILLA };
struct Option { int value; bool Is(int x)const{return value==x;}
    bool IsNot(int x)const{return value!=x;} explicit operator bool()const{return value!=0;} };
struct Context { int chest=RO_GENERIC_ON;
    Option GetOption(int key)const{return {key==RSK_SHUFFLE_CHEST_MINIGAME ? chest : RO_GERUDO_KEYS_VANILLA};} } context;
struct Globals { Context* gRandoContext=&context; static Globals* Instance; } globals;
Globals* Globals::Instance=&globals;
using OTRGlobals=Globals;
bool randoActive=true;
#define IS_RANDO randoActive
std::string loaded;
int loadedIcon=-1;
struct CustomMessage {
    std::string text; int icon=-1;
    CustomMessage()=default;
    CustomMessage(std::string x,int=0):text(std::move(x)){}
    CustomMessage(std::string x,std::string,std::string,int=0):text(std::move(x)){}
    std::string GetEnglish(int)const{return text;}
    void Replace(const char* key,const CustomMessage& value){auto pos=text.find(key); assert(pos!=std::string::npos);text.replace(pos,std::strlen(key),value.text);}
    void Replace(const char* key,const char* value){Replace(key,CustomMessage(value));}
    void AutoFormat(int value){icon=value;}
    void LoadIntoFont()const{loaded=text;loadedIcon=icon;}
};
constexpr int MF_RAW=0;
namespace Rando::StaticData {
struct Item { std::string GetName()const{return "Chest Game Small Key";}
    std::string GetArticle()const{return "a ";} };
Item RetrieveItem(RandomizerGet id){assert(id==RG_TREASURE_GAME_SMALL_KEY);return {};}
}
struct GetItemEntry { int getItemId,itemId,objectId,modIndex; };
struct Player { GetItemEntry getItemEntry; int getItemId; } receiptPlayer;
struct PlayState {} play;
PlayState* gPlayState=&play;
#define GET_PLAYER(play) (&receiptPlayer)
using Handler=void(*)(uint16_t*,bool*);
std::map<uint16_t,Handler> handlers;
#define COND_ID_HOOK(type,id,condition,callback) do { if(condition) handlers[id]=callback; } while(0)
#define DUNGEON_ITEMS_CAN_BE_OUTSIDE_DUNGEON(setting) false
constexpr int TEXT_RANDOMIZER_CUSTOM_ITEM=0xF8, TEXT_DESC_DUNGEON_MAP_INFO=0x9300,
    TEXT_DESC_DUNGEON_COMPASS_INFO=0x9301, TEXT_ITEM_DUNGEON_MAP=0x66,
    TEXT_ITEM_COMPASS=0x67, TEXT_ITEM_KEY_BOSS=0xC7, TEXT_ITEM_KEY_SMALL=0x60;
bool DungeonInformationEnabled(){return false;}
void BuildItemMessage(uint16_t*,bool*){}
void BuildWandMedallionMessage(uint16_t*,bool*){}
void BuildDungeonPauseInfoMessage(uint16_t*,bool*){}
void BuildMapMessage(uint16_t*,bool*){}
void BuildBossKeyMessage(uint16_t*,bool*){}
void BuildSmallKeyMessage(uint16_t*,bool*){}
/* BUILDERS */
int main(){
    const GetItemEntry shuffled={/* GI */,/* ITEM */,/* OBJECT */,/* MOD */};
    const uint16_t shuffledText=/* TEXT */;
    // 9.3 shuffled keys use the custom-item table and its registered builder.
    const GetItemEntry native={GI_DOOR_KEY,ITEM_KEY_SMALL,OBJECT_GI_KEY,MOD_NONE};
    const uint16_t nativeText=0xF3;
    receiptPlayer={native,native.getItemId};
    RegisterItemMessages();
    assert(handlers.at(shuffledText)==BuildItemMessage);
    CustomMessage shuffledMessage;
    assert(BuildDungeonKeyReceiptMessage(static_cast<RandomizerGet>(shuffled.itemId),shuffledMessage));
    assert(shuffledMessage.text=="You found a %gChest Game Small Key%w!");
    for(uint16_t id=0x3C; id<=0x41; ++id)
        assert(handlers.at(id)==BuildWandMedallionMessage && "native medallion tutorial hook is missing");
    assert(handlers.count(nativeText) && "Shuffled Chest Game key bypasses its dungeon receipt");
    const auto callback=handlers.at(nativeText);
    for(int repeat=0;repeat<2;++repeat) {
        uint16_t id=nativeText; bool fromTable=true;
        loaded.clear();loadedIcon=-1;
        const auto before=receiptPlayer;
        callback(&id,&fromTable);
        assert(!fromTable && id==nativeText);
        assert(loaded=="You found a %gChest Game Small Key%w!");
        assert(loadedIcon==ITEM_KEY_SMALL);
        assert(std::memcmp(&receiptPlayer,&before,sizeof(receiptPlayer))==0);
    }
    auto unchanged=[&](){
        uint16_t id=nativeText; bool fromTable=true;
        loaded="native receipt";loadedIcon=-1;
        callback(&id,&fromTable);
        assert(fromTable && id==nativeText && loaded=="native receipt" && loadedIcon==-1);
    };
    context.chest=RO_GENERIC_OFF;unchanged();
    context.chest=RO_GENERIC_ON;
    randoActive=false;unchanged();randoActive=true;
    receiptPlayer.getItemEntry.objectId=OBJECT_INVALID;unchanged();receiptPlayer.getItemEntry=native;
    receiptPlayer.getItemEntry.modIndex=MOD_RANDOMIZER;unchanged();receiptPlayer.getItemEntry=native;
    receiptPlayer.getItemEntry.itemId=0;unchanged();receiptPlayer.getItemEntry=native;
    receiptPlayer.getItemEntry.getItemId=0;unchanged();receiptPlayer.getItemEntry=native;
    receiptPlayer.getItemId=0;unchanged();receiptPlayer.getItemId=native.getItemId;
    gPlayState=nullptr;unchanged();
    std::cout<<"9.3 shuffled Chest Game keys register the custom receipt and retain the 24px key icon; legacy native text, shuffle-off, non-rando and invalid/stale entries remain covered\n";
}
'''
builders = function(source, 'bool BuildDungeonKeyReceiptMessage(RandomizerGet rg, CustomMessage& msg) {')
if 'void BuildChestGameSmallKeyMessage(' in source:
    builders += '\n' + function(source, 'void BuildChestGameSmallKeyMessage(')
builders += '\n' + function(source, 'void RegisterItemMessages()')
fixture = fixture.replace('/* BUILDERS */', builders)
for key, value in {'GI':gi, 'ITEM':item, 'OBJECT':obj, 'TEXT':text, 'MOD':mod}.items():
    fixture = fixture.replace('/* '+key+' */', value)
with tempfile.TemporaryDirectory(prefix='chest-game-key-receipt-') as tmp:
    cpp=Path(tmp)/'fixture.cpp'
    cpp.write_text(fixture)
    exe=Path(tmp)/'fixture'
    subprocess.run([*shlex.split(os.environ.get('CXX','c++')), '-std=c++20',
        '-Wall','-Wextra','-Werror','-Wno-unused-parameter','-I',str(ROOT),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
