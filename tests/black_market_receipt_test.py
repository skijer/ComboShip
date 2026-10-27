#!/usr/bin/env python3
"""Compile source-derived market receipt ownership tests; no game/runtime proof.
Run from the repository: python3 tests/black_market_receipt_test.py
--snippets accepts a narrow captured-source fixture for review without a checkout.
"""
import argparse
import json
import pathlib
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
MM = "mm/2s2h/Enhancements/Gameplay/BlackMarket.cpp"
OOT = "soh/soh/Enhancements/Gameplay/ComboBlackMarketNative.cpp"
HEADER = "soh/soh/Enhancements/item-tables/ItemTableTypes.h"


def block(source, signature):
    start = source.index(signature)
    return source[start:source.index("\n}\n", start) + 3]


COMMON = r"""
#include <cassert>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <iostream>
using s16 = int16_t;
constexpr int ITEM_NONE=255, GI_NONE=0, OBJECT_INVALID=0, MOD_RANDOMIZER=1;
constexpr int SECTION_ID_BASE=0, RC_UNKNOWN=0, MTXMODE_APPLY=0, CAM_ID_MAIN=0;
constexpr int TRANS_TRIGGER_OFF=0, TRANS_MODE_OFF=0, GAMEOVER_INACTIVE=0;
constexpr unsigned PLAYER_STATE1_400=1, PLAYER_STATE1_GETTING_ITEM=1, PLAYER_STATE1_IN_ITEM_CS=2;
constexpr unsigned PLAYER_STATE1_CHARGING_SPIN_ATTACK=4, PLAYER_STATE1_2000=8, PLAYER_STATE1_4000=16;
constexpr unsigned PLAYER_STATE1_40000=32, PLAYER_STATE1_80000=64, PLAYER_STATE1_100000=128;
constexpr unsigned PLAYER_STATE1_200000=256, PLAYER_STATE1_8000000=512;
constexpr int PLAYER_EXPLOSIVE_NONE=-1, ACTOR_EN_ITEM00=9, ITEM00_NOTHING=0;
struct Item { const char* key="nei.magic_cape"; const char* label="Magic Cape"; };
struct Actor {
    int ticket=0,flags=4;
    bool killed=false;
    struct {struct{float x=0,y=0,z=0;}pos;}world;
    void (*draw)(Actor*, PlayState*)=nullptr;
    void (*destroy)(Actor*, PlayState*)=nullptr;
};
struct Player { unsigned stateFlags1=0; GetItemEntry getItemEntry{}; int getItemId=0; Actor actor; Actor* interactRangeActor=nullptr; };
struct PlayState { struct{bool running=true;}state; struct{int msgMode=0;}msgCtx;
    int transitionTrigger=0,transitionMode=0,actorCtx=0; struct{int state=0;}gameOverCtx; };
struct EnItem00;
using EnItem00ActionFunc=void(*)(EnItem00*,PlayState*);
struct EnItem00 {Actor actor; EnItem00ActionFunc actionFunc=nullptr;};
namespace nlohmann {using json=int;}
#define GET_PLAYER(play) (&nativePlayer)
#define CUSTOM_ITEM_PARAM (actor->ticket)
#define CUSTOM_ITEM_FLAGS (actor->flags)
namespace CustomItem {constexpr int HIDE_TILL_OVERHEAD=8,KEEP_ON_PLAYER=16,GIVE_OVERHEAD=2,GIVE_ITEM_CUTSCENE=4;}
struct GIEventGiveItem {bool showGetItemCutscene=true;s16 param=0;void(*giveItem)(Actor*,PlayState*);void(*drawItem)(Actor*,PlayState*)=nullptr;};
struct GIEventNone {};
using Event=std::variant<GIEventNone,GIEventGiveItem>;
namespace GameInteractor {struct Engine{std::vector<Event>events;Event currentEvent;};inline Engine value;inline Engine* Instance=&value;}
namespace CustomMessage {
struct Entry{int textboxType=0,icon=0,swordElementTutorial=0;std::string msg;bool capeVisibilityChoice=false;};
inline int receipts=0;inline bool capeSeen=false;
void SetActiveCustomMessage(std::string,Entry e){++receipts;capeSeen=e.capeVisibilityChoice;}
void StartTextbox(std::string s,Entry e){SetActiveCustomMessage(s,e);}
}
namespace Rando {
namespace StaticData {struct Metadata{int itemId=4;};inline std::map<int,Metadata>Items{{7,{}}};int GetIconForZMessage(int){return 3;}}
void ApplyItemReceiptText(int,CustomMessage::Entry&e){e.capeVisibilityChoice=true;}
void AppendReceiptSource(CustomMessage::Entry&,const char*){}
void DrawResolvedItem(int,int,Actor*){}
}
int SwordElement_MessageElementForPickup(int){return 2;}
void SwordElement_MessageAppend(PlayState*,int){}
void Matrix_Scale(float,float,float,int){}
inline int ootSaves=0;
struct SaveManager {struct Manager{void ThreadPoolWait(){}void SaveSection(int,int,bool){++ootSaves;}};inline static Manager value;inline static Manager*Instance=&value;};
GetItemEntry Entry(int ticket=0){GetItemEntry e=GET_ITEM_NONE;e.modIndex=1;e.getItemId=7;e.comboMarketReceipt=ticket;return e;}
"""

LOCAL = r"""
Player nativePlayer;PlayState play;PlayState*gPlayState=&play;void*gGameState=&play.state;
struct Save{int fileNum=1,rupees=99;struct NativeSave{
struct SaveInfo{struct PlayerData{int rupees=99;}playerData;}saveInfo;
struct ShipInfo{struct Rando{unsigned finalSeed=42;}rando;}shipSaveInfo;}save;}gSaveContext;
struct Session{int market=3,state=0;}sSession;
Item item;bool active=true,owned=false,available=true,blocking=false,safe=true;int grants=0,saves=0,resumes=0,camera=0,destructions=0,kills=0;
bool failSpawn=false;std::string lastKey;int sSelected=-1;struct{int slot=-1;}sBrowser;
bool Active(){return active;}unsigned Seed(){return 42;}
const Item*Offer(int slot){return slot>=0&&slot<8?&item:nullptr;}
enum class PurchaseResult{Ready,Unavailable};
PurchaseResult Status(int){return !owned&&available?PurchaseResult::Ready:PurchaseResult::Unavailable;}
void CommitPurchase(int&state,int){++state;}
namespace ComboBlackMarketItems{int Id(const Item&){return 7;}bool Grant(const Item&i){++grants;owned=true;lastKey=i.key;return true;}}
namespace ComboBlackMarketOotItems{void LatchToolOwnership(){}bool Owned(const Item&){return owned;}bool Grant(const Item&i){++grants;owned=true;lastKey=i.key;return true;}}
void SaveManager_SaveCurrentForCombo(bool){++saves;}
bool Player_InBlockingCsMode(PlayState*,Player*){return blocking;}
bool Safe(PlayState*){return safe;}
int Play_GetActiveCamId(PlayState*){return camera;}
bool BeginBrowsing(PlayState*){++resumes;return true;}
constexpr int kBrowseText=123;
void Message_StartTextbox(PlayState*,int,void*){}
int Player_GetExplosiveHeld(Player*){return PLAYER_EXPLOSIVE_NONE;}
int GiveItemEntryWithoutActor(PlayState*,GetItemEntry e){nativePlayer.getItemEntry=e;nativePlayer.getItemId=e.getItemId;nativePlayer.interactRangeActor=&nativePlayer.actor;return 1;}
std::vector<std::unique_ptr<EnItem00>> actors;
Actor*Actor_Spawn(int*,PlayState*,int,float,float,float,s16 flags,int,s16 ticket,int){
 if(failSpawn)return nullptr;
 actors.push_back(std::make_unique<EnItem00>());auto*a=&actors.back()->actor;a->ticket=ticket;a->flags=flags;return a;
}
void Actor_Kill(Actor*a){++kills;a->killed=true;a->draw=nullptr;}
void CustomItem00_Destroy(Actor*,PlayState*){++destructions;}
void NativeTick(Actor*a){if(!a->killed)reinterpret_cast<EnItem00*>(a)->actionFunc(reinterpret_cast<EnItem00*>(a),&play);}
void NativeDestroy(Actor*a){if(a->destroy)a->destroy(a,&play);}
"""

MM_TEST = r"""
void Unrelated(Actor*,PlayState*){assert(false && "market actor dispatched unrelated scheduler event");}
void Run(){
 sReceipt.Begin(2,1,42,40);sReceiptMarket=3;
 // Ordinary queue work keeps ownership and gets priority.
 GameInteractor::Instance->currentEvent=GIEventGiveItem{true,100,Unrelated,nullptr};
 OfferMarketReceipt(&play,&nativePlayer);assert(!sReceiptActor);
 GameInteractor::Instance->currentEvent=GIEventNone{};
 failSpawn=true;OfferMarketReceipt(&play,&nativePlayer);assert(!sReceiptActor&&sReceipt.pending);
 failSpawn=false;OfferMarketReceipt(&play,&nativePlayer);Actor*old=sReceiptActor;assert(old);
 auto oldAction=reinterpret_cast<EnItem00*>(old)->actionFunc;
 // A live cancellation retires actor update before unrelated work can start.
 CancelReceipt();assert(old->killed&&!sReceiptActor&&kills==1&&std::holds_alternative<GIEventNone>(GameInteractor::Instance->currentEvent));
 GameInteractor::Instance->currentEvent=GIEventGiveItem{true,100,Unrelated,nullptr};
 GameInteractor::Instance->events.push_back(GIEventGiveItem{true,100,Unrelated,nullptr});
 NativeTick(old);oldAction(reinterpret_cast<EnItem00*>(old),&play);
 assert(grants==0&&destructions==0&&GameInteractor::Instance->events.size()==1);
 assert(std::get<GIEventGiveItem>(GameInteractor::Instance->currentEvent).giveItem==Unrelated);
 GameInteractor::Instance->currentEvent=GIEventNone{};GameInteractor::Instance->events.clear();
 sReceipt.Begin(2,1,42,40);sReceiptMarket=3;OfferMarketReceipt(&play,&nativePlayer);
 auto*a=sReceiptActor;oldAction(reinterpret_cast<EnItem00*>(old),&play);NativeDestroy(old);
 assert(grants==0&&destructions==1&&sReceiptActor==a&&std::get<GIEventGiveItem>(GameInteractor::Instance->currentEvent).param==sReceipt.ticket);
 NativeTick(a);assert(grants==1&&saves==1&&sSession.state==1&&gSaveContext.save.saveInfo.playerData.rupees==59&&CustomMessage::capeSeen);
 NativeTick(a);assert(grants==1&&saves==1);
 // Choice/tutorial and native item camera/state finish before a single resume.
 play.msgCtx.msgMode=1;MonitorReceipt();assert(!resumes);
 play.msgCtx.msgMode=0;nativePlayer.stateFlags1=1;MonitorReceipt();assert(!resumes);
 nativePlayer.stateFlags1=0;blocking=true;MonitorReceipt();assert(!resumes);
 blocking=false;camera=1;MonitorReceipt();assert(!resumes);
 camera=0;MonitorReceipt();assert(resumes==1&&!sReceipt.pending&&sSelected==2);
 NativeDestroy(a);MonitorReceipt();assert(resumes==1);
 // After arena cleanup no pointer, player or actor can be dereferenced.
 sReceipt.Begin(2,1,42,40);sReceiptActor=reinterpret_cast<Actor*>(uintptr_t(1));
 CancelReceipt(true);assert(!sReceiptActor&&!sReceipt.pending&&kills==2);
}
"""

OOT_TEST = r"""
void Run(){
 sReceipt.Begin(2,1,42,40);sReceiptMarket=3;sReceiptEntry=Entry(sReceipt.ticket);
 sReceipt.offered=GiveItemEntryWithoutActor(&play,sReceiptEntry);
 GetItemEntry issued=sReceiptEntry;
 // Withdraw an unaccepted native offer, including the already-copied player entry.
 CancelReceipt();assert(nativePlayer.getItemId==GI_NONE&&nativePlayer.getItemEntry.objectId==OBJECT_INVALID&&!nativePlayer.interactRangeActor);
 assert(FixedMarketGrant(&play,issued));assert(!grants&&!ootSaves); // stale suppresses ordinary grant
 assert(!FixedMarketGrant(&play,Entry())); // same ID, ordinary item unaffected
 sReceipt.Begin(2,1,42,40);sReceiptMarket=3;sReceiptEntry=Entry(sReceipt.ticket);
 sReceipt.offered=GiveItemEntryWithoutActor(&play,sReceiptEntry);nativePlayer.stateFlags1=PLAYER_STATE1_GETTING_ITEM;
 assert(FixedMarketGrant(&play,issued));assert(!grants); // old/new same ID, distinct ticket
 item.key="nei.cane.blocks";assert(FixedMarketGrant(&play,sReceiptEntry));assert(grants==1&&lastKey=="nei.cane.blocks");
 ReceiveMarketItem(sReceiptEntry);ReceiveMarketItem(sReceiptEntry);
 assert(ootSaves==1&&gSaveContext.rupees==59&&sSession.state==1);
 play.msgCtx.msgMode=1;MonitorReceipt();assert(!resumes);
 play.msgCtx.msgMode=0;MonitorReceipt();assert(!resumes);
 nativePlayer.stateFlags1=0;camera=1;MonitorReceipt();assert(!resumes);
 camera=0;MonitorReceipt();assert(resumes==1&&!sReceipt.pending);
 owned=false;sReceipt.Begin(2,1,42,20);sReceiptMarket=3;sReceiptEntry=Entry(sReceipt.ticket);
 sReceipt.offered=GiveItemEntryWithoutActor(&play,sReceiptEntry);issued=sReceiptEntry;nativePlayer.stateFlags1=1;
 active=false;assert(FixedMarketGrant(&play,issued));assert(grants==1); // invalid context cannot fall through
 CancelReceipt();assert(nativePlayer.getItemId==GI_NONE);
 active=true;sReceipt.Begin(2,1,42,20);sReceiptMarket=3;sReceiptEntry=Entry(sReceipt.ticket);
 sReceipt.offered=GiveItemEntryWithoutActor(&play,sReceiptEntry);item.key="nei.magic_cape";
 assert(!FixedMarketGrant(&play,sReceiptEntry));owned=true;ReceiveMarketItem(sReceiptEntry);
 assert(ootSaves==2&&gSaveContext.rupees==39);
 CancelReceipt();sReceipt.Begin(2,1,42,20);sReceiptMarket=3;sReceiptEntry=Entry(sReceipt.ticket);
 nativePlayer.getItemEntry=Entry();nativePlayer.getItemId=7;nativePlayer.interactRangeActor=&nativePlayer.actor;
 CancelReceipt();assert(nativePlayer.getItemId==7); // never withdraw unrelated player's entry
}
"""


def generate(read):
    header = read(HEADER).replace("#pragma once", "")
    state = read("combo/BlackMarketReceipt.h").replace("#pragma once", "")
    mm, oot = read(MM), read(OOT)
    for source in (mm, oot):
        purchase = block(source, "bool Purchase()")
        assert "::Grant(" not in purchase and "rupees -=" not in purchase, "confirmation bypassed native acknowledgement"
    mm_functions = "\n".join(block(mm, sig) for sig in (
        "bool ReceiptContext()", "void AbortMarketReceipt(", "void GiveMarketReceipt(",
        "void DrawMarketReceipt(", "void ReleaseReceiptEvent(", "void MarketReceiptDestroyed(", "void CancelReceipt(",
        "void OfferMarketReceipt(", "void MonitorReceipt()"))
    oot_functions = "\n".join(block(oot, sig) for sig in (
        "bool ReceiptContext()", "void CancelReceipt(", "bool PendingReceiptEntry(",
        "bool FixedMarketGrant(", "void ReceiveMarketItem(", "void MonitorReceipt()"))
    declarations = "BlackMarketReceipt::State sReceipt; nlohmann::json sReceiptMarket;"
    return ("#define COMBO_BUILD\n" + header + COMMON + state +
            "\nnamespace MM {\n" + LOCAL + declarations + "Actor*sReceiptActor=nullptr;int sReceiptActorTicket=0;\n" +
            mm_functions + MM_TEST + "\n}\nnamespace OOT {\n" + LOCAL + declarations +
            "GetItemEntry sReceiptEntry{};\n" + oot_functions + OOT_TEST +
            "\n}\nint main(){MM::Run();OOT::Run();std::cout<<\"receipt ownership fixtures passed\\n\";}\n")



def generate_tool_ownership(read):
    """Exercise actual native slot setters and existing latch around both seams."""
    code = r"""
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <string_view>
constexpr int SLOT_SHOVEL=24,SLOT_ROD_OF_SEASONS=25,SLOT_MAGIC_MIRROR=26,SLOT_PHANTOM_HOURGLASS=27;
constexpr int ITEM_SHOVEL=101,ITEM_DOMINION_ROD=102,EXT_ITEM_ROD_OF_SEASONS=103;
constexpr int ITEM_TIME_GATE=104,EXT_ITEM_MAGIC_MIRROR=105,ITEM_HYLIAS_GRACE=106,EXT_ITEM_PHANTOM_HOURGLASS=107,SEASON_COUNT=4;
constexpr int SECTION_ID_BASE=0;
struct NeiSaveData{uint16_t ownedItems[48]{};uint8_t shovelOwned=0,dominionOwned=0,seasonsOwned=0,season=0,seasonsRodOwned=0;}gNeiSave;
NeiSaveData*Nei_Save(){return &gNeiSave;}
uint16_t Nei_GetOwnedItem(uint8_t slot){return gNeiSave.ownedItems[slot-24];}
namespace ItemGrantAudit{struct Scope{template<class...T>Scope(T...){}};}
void ItemGrantAudit_Begin(const char*,int,int,int){}void ItemGrantAudit_End(){}
struct SaveContext{struct{uint8_t items[24];}inventory;int fileNum=1,rupees=99;}gSaveContext;
void TravelTools_Grant(uint16_t){}void GraceHourglass_Grant(uint16_t){}
uint8_t ExtInv_GetItemSlot(uint16_t item){assert(item==ITEM_SHOVEL||item==ITEM_DOMINION_ROD);return SLOT_SHOVEL;}
SET
SLOT
BYID
GIVE
SEASON
STATE
struct PlayState{}play;PlayState*gPlayState=&play;
struct GetItemEntry{int comboMarketReceipt=0;};
BlackMarketReceipt::State sReceipt;
struct{int state=0;}sSession;
struct Item{const char*key;};Item chosen;
const Item*Offer(int){return &chosen;}
unsigned Seed(){return 42;}
bool PendingReceiptEntry(PlayState*,const GetItemEntry&e){return sReceipt.Matches(gSaveContext.fileNum,Seed(),e.comboMarketReceipt)&&!sReceipt.delivered;}
int saves=0,nativeGrants=0;bool savedShovel=false,savedDominion=false;
struct SaveManager{
 void ThreadPoolWait(){}
 void SaveSection(int,int,bool){++saves;savedShovel=gNeiSave.shovelOwned;savedDominion=gNeiSave.dominionOwned;}
 static SaveManager*Instance;
};SaveManager manager;SaveManager*SaveManager::Instance=&manager;
void CommitPurchase(int&state,int){++state;}
namespace ComboBlackMarketOotItems{
LATCH
bool Grant(const Item&){assert(false&&"ordinary native receipt was intercepted");return false;}
bool Owned(const Item&i){
 const auto key=std::string_view(i.key);
 if(key=="nei.shovel")return gNeiSave.shovelOwned||Nei_GetOwnedItem(SLOT_SHOVEL)==ITEM_SHOVEL;
 if(key=="nei.dominion_rod")return gNeiSave.dominionOwned||Nei_GetOwnedItem(SLOT_SHOVEL)==ITEM_DOMINION_ROD;
 return gNeiSave.seasonsOwned&1;
}
}
FIXED
RECEIVE
void NativeGrant(int kind){
 ++nativeGrants;
 if(kind==0)ExtInv_SetItemById(ITEM_DOMINION_ROD);
 else if(kind==1)ExtInv_SetItemById(ITEM_SHOVEL);
 else Seasons_GrantSeason(0);
}
void Run(int kind){
 gNeiSave={};gSaveContext.rupees=99;sReceipt.Cancel();sSession.state=0;saves=nativeGrants=0;savedShovel=savedDominion=false;
 if(kind==0)Nei_SetOwnedItem(SLOT_SHOVEL,ITEM_SHOVEL);
 else if(kind==1)Nei_SetOwnedItem(SLOT_SHOVEL,ITEM_DOMINION_ROD);
 else if(kind==2)Nei_SetOwnedItem(SLOT_ROD_OF_SEASONS,ITEM_DOMINION_ROD);
 chosen.key=kind==0?"nei.dominion_rod":kind==1?"nei.shovel":"nei.season.spring";
 assert(sReceipt.Begin(0,1,42,40));GetItemEntry entry{static_cast<int>(sReceipt.ticket)};
 assert(!FixedMarketGrant(&play,entry));NativeGrant(kind);
 ReceiveMarketItem(entry);ReceiveMarketItem(entry);
 assert(nativeGrants==1&&saves==1&&sSession.state==1&&gSaveContext.rupees==59);
 if(kind<2)assert(gNeiSave.shovelOwned&&gNeiSave.dominionOwned&&savedShovel&&savedDominion);
 else{
   assert(!gNeiSave.shovelOwned&&!savedShovel);
   assert(bool(gNeiSave.dominionOwned)==(kind==2)&&savedDominion==(kind==2));
   assert(gNeiSave.seasonsOwned==1&&gNeiSave.seasonsRodOwned);
 }
}
int main(){for(int kind=0;kind<4;++kind)Run(kind);puts("tool ownership: both shared-slot orders, legacy season replacement and no invented bits passed");}
"""
    inventory = read("soh/mods/extended_inventory.h")
    substitutions = {
        "SET": block(read("soh/mods/nei_save.cpp"), 'extern "C" void Nei_SetOwnedItem('),
        "SLOT": block(inventory, "static inline void ExtInv_SetSlotItem("),
        "BYID": block(inventory, "static inline void ExtInv_SetItemById("),
        "GIVE": block(inventory, "static inline void ExtInv_GiveItem("),
        "SEASON": block(read("soh/mods/extended_inventory.c"), "void Seasons_GrantSeason("),
        "STATE": read("combo/BlackMarketReceipt.h").replace("#pragma once", ""),
        "LATCH": block(read("soh/soh/Enhancements/Gameplay/ComboBlackMarketItems.cpp"), "void LatchToolOwnership()"),
        "FIXED": block(read(OOT), "bool FixedMarketGrant("),
        "RECEIVE": block(read(OOT), "void ReceiveMarketItem("),
    }
    for marker, source in substitutions.items():
        code = code.replace("\n" + marker + "\n", "\n" + source + "\n")
    return code


def run(read):
    with tempfile.TemporaryDirectory(prefix="black-market-receipt-") as tmp:
        tmp = pathlib.Path(tmp)
        cpp = tmp / "ownership.cpp"
        cpp.write_text(generate(read))
        cmd = ["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
               "-Wno-unused-parameter", "-Wno-missing-field-initializers", str(cpp), "-o", str(tmp / "ownership")]
        subprocess.run(cmd, check=True)
        subprocess.run([str(tmp / "ownership")], check=True)
        cpp.write_text(generate_tool_ownership(read))
        subprocess.run(cmd, check=True)
        subprocess.run([str(tmp / "ownership")], check=True)
        # Compile the actual header in both modes; ordinary aggregate macros
        # zero-fill the new field and all previous 64-bit offsets stay stable.
        header = read(HEADER).replace("#pragma once", "")
        for combo in (False, True):
            body = ("#define COMBO_BUILD\n" if combo else "") + header + r"""
#include <cassert>
#include <cstddef>
constexpr int ITEM_NONE=255,MOD_RANDOMIZER=1;
int main(){
 GetItemEntry ordinary=GET_ITEM_NONE;
 GetItemEntry custom=GET_ITEM(1,2,3,4,5,CHEST_ANIM_LONG,ITEM_CATEGORY_MAJOR,MOD_RANDOMIZER,7);
 static_assert(offsetof(GetItemEntry,drawFunc)==32);
"""
            if combo:
                body += "static_assert(sizeof(void*)!=8 || sizeof(GetItemEntry)==48);\n"
                body += "static_assert(sizeof(void*)!=8 || offsetof(GetItemEntry,comboForeignCheck)==40);\n"
                body += "static_assert(offsetof(GetItemEntry,comboMarketReceipt)==offsetof(GetItemEntry,comboForeignCheck)+4);\n"
                body += "assert(!ordinary.comboMarketReceipt&&!custom.comboMarketReceipt);\n"
            else:
                body += "static_assert(sizeof(void*)!=8 || sizeof(GetItemEntry)==40);\n"
            body += "(void)ordinary;(void)custom;}\n"
            cpp.write_text(body)
            subprocess.run(cmd, check=True)
            subprocess.run([str(tmp / "ownership")], check=True)
        print("actual item-table header: Combo/solo layout and ordinary zero-init passed")
        camera = read("combo/BlackMarketBrowsing.h").replace("#pragma once", "")
        cpp.write_text(camera + r"""
#include <cassert>
int main(){
 using namespace BlackMarketBrowsing;
 auto keeper=Target(-1);assert(keeper.eye.z==196&&keeper.fov==46);
 State s;assert(s.CameraSettled());assert(s.Move(1,0));int ticks=0;float previous=s.view.eye.z;
 while(!s.CameraSettled()&&ticks++<40){s.TickCamera();assert(s.view.eye.z>=previous&&s.view.eye.z<=Target(0).eye.z);previous=s.view.eye.z;}
 assert(s.CameraSettled()&&ticks<30);assert(s.Move(-1,0));ticks=0;
 while(!s.CameraSettled()&&ticks++<40)s.TickCamera();
 assert(s.CameraSettled()&&s.view.eye.z==keeper.eye.z);
}
""")
        subprocess.run(cmd, check=True)
        subprocess.run([str(tmp / "ownership")], check=True)
        print("camera target and settled-pan fixture passed")



if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--snippets", type=pathlib.Path)
    args = parser.parse_args()
    snippets = json.loads(args.snippets.read_text()) if args.snippets else None
    run(lambda path: snippets[path] if snippets else (ROOT / path).read_text())
