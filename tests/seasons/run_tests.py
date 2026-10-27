"""Execute season grants, native D-pad equip, and consumed-shop transitions."""
import re, sys, subprocess, tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_mm_nei_tests import flags
from run_time_pedestal_tests import block_from
from run_mm_weather_tests import production_function

def body(path,name): return production_function((ROOT/path).read_text(),name)
def run(name,prefix,parts,checks):
 if len(sys.argv)>1 and sys.argv[1]!=name:return
 if any('Seasons_SeasonOwned(' in part or 'Seasons_HasRod(' in part for part in parts):
  parts.insert(0, body('mm/mods/extended_inventory.c', 'Seasons_HasRod'))
  prefix += '\nu16 Nei_GetOwnedItem(u8 slot){return Nei_Save()->ownedItems[slot-24];}\n'
  prefix += 'u8 Seasons_RandoMode(){return NEI_SEASONS_INDIVIDUAL;}\nvoid Seasons_UpdateGates(){}\n'
 if any('MMSeasonParticles_' in part for part in parts):
  # Extracted native snow/leaf bodies now share the real editor particle policy.
  prefix = '#include "mods/items/helpers/season_particles.h"\n' + prefix
  if not re.search(r'\bCVarGetInteger\([^;]*\)\s*\{',prefix):
   prefix += '\nextern "C" int32_t CVarGetInteger(const char*,int32_t fallback){return fallback;}\n'
 with tempfile.TemporaryDirectory(prefix='seasons-') as td:
  p=Path(td)/'test.cpp'
  p.write_text('#include "mods/extended_inventory.h"\n#include "mods/ext_buttons/ext_buttons.h"\n#include "overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope.h"\n#include <cassert>\n#include <cstring>\n#include <iostream>\n'+prefix+'\n'+'\n'.join(parts)+'\nint main(){'+checks+'\nstd::cout<<"PASS '+name+'\\n";}')
  binary=Path(td)/'test'
  # Fold the fixed Rod slot in ExtInv_GiveItem so the unrelated shared Grace
  # grant is not a fixture dependency. The actual season grant still executes.
  subprocess.run(['c++','-std=c++20','-O2','-w',*(['-fpermissive'] if name in ('icon','native_dpad') else []),*flags(),'-ffunction-sections','-fdata-sections',str(p),'-Wl,--gc-sections','-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True)

p='mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_item.c'
# This first assertion reproduces the reported rod->mushroom conversion using the real equip body.
run('dpad',r'''
SaveContext gSaveContext{};
extern "C" int32_t CVarGetInteger(const char*,int32_t){return 1;}
void Interface_LoadItemIcon(PlayState*,u8){}
void Interface_LoadItemIconImpl(PlayState*,u8){}
void Interface_Dpad_LoadItemIcon(PlayState*,u8){}
void Interface_Dpad_LoadItemIconImpl(PlayState*,u8){}
''',[(ROOT/'mm/mods/ext_buttons/ext_buttons.cpp').read_text().replace('#include \"ext_buttons.h\"', '#include \"mods/ext_buttons/ext_buttons.h\"'), body(p,'KaleidoScope_UpdateDpadItemEquip')],r'''
PlayState play{};
memset(gSaveContext.save.saveInfo.equips.buttonItems,ITEM_NONE,sizeof(gSaveContext.save.saveInfo.equips.buttonItems));
memset(gSaveContext.save.saveInfo.equips.cButtonSlots,SLOT_NONE,sizeof(gSaveContext.save.saveInfo.equips.cButtonSlots));
memset(gSaveContext.save.shipSaveInfo.dpadEquips.dpadItems,ITEM_NONE,sizeof(gSaveContext.save.shipSaveInfo.dpadEquips.dpadItems));
memset(gSaveContext.save.shipSaveInfo.dpadEquips.dpadSlots,SLOT_NONE,sizeof(gSaveContext.save.shipSaveInfo.dpadEquips.dpadSlots));
play.pauseCtx.equipTargetCBtn=PAUSE_EQUIP_D_UP;
play.pauseCtx.equipTargetItem=EXT_ITEM_ROD_OF_SEASONS;play.pauseCtx.equipTargetSlot=95;
KaleidoScope_UpdateDpadItemEquip(&play);
assert(DPAD_BUTTON_ITEM_EQUIP(0,EQUIP_SLOT_D_UP)==ITEM_EXT_BUTTON);
assert(ExtButton_GetDpadItem(0,EQUIP_SLOT_D_UP)==EXT_ITEM_ROD_OF_SEASONS);
assert(ExtButton_EquipItem(&play,PAUSE_EQUIP_C_LEFT,EXT_ITEM_ROD_OF_SEASONS,95));
assert(ExtButton_GetItem(0,EQUIP_SLOT_C_LEFT)==EXT_ITEM_ROD_OF_SEASONS);
assert(C_SLOT_EQUIP(0,EQUIP_SLOT_C_LEFT)==SLOT_NONE);
assert(ExtButton_GetDpadItem(0,EQUIP_SLOT_D_UP)==ITEM_NONE);
// Move from C to D-pad over a real mushroom: swap preserves both full identities.
ExtButton_SetDpadItem(0,EQUIP_SLOT_D_DOWN,ITEM_MUSHROOM);
DPAD_SLOT_EQUIP(0,EQUIP_SLOT_D_DOWN)=SLOT_BOTTLE_1;
assert(ExtButton_EquipItem(&play,PAUSE_EQUIP_D_DOWN,EXT_ITEM_ROD_OF_SEASONS,95));
assert(ExtButton_GetItem(0,EQUIP_SLOT_C_LEFT)==ITEM_MUSHROOM);
assert(ExtButton_GetDpadItem(0,EQUIP_SLOT_D_DOWN)==EXT_ITEM_ROD_OF_SEASONS);
assert(BUTTON_ITEM_EQUIP(0,EQUIP_SLOT_C_LEFT)==ITEM_MUSHROOM);
assert(EXT_BUTTON_ITEM(0,EQUIP_SLOT_C_LEFT)==0);
// A native item swapped onto a rod-equipped destination moves the rod back to its source.
assert(ExtButton_EquipItem(&play,PAUSE_EQUIP_D_DOWN,ITEM_MUSHROOM,SLOT_BOTTLE_1));
assert(ExtButton_GetItem(0,EQUIP_SLOT_C_LEFT)==EXT_ITEM_ROD_OF_SEASONS);
assert(ExtButton_GetDpadItem(0,EQUIP_SLOT_D_DOWN)==ITEM_MUSHROOM);
assert(!ExtButton_EquipItem(&play,PAUSE_EQUIP_D_UP,ITEM_BOW,SLOT_BOW));
''')

run('grant',r'''
NeiSaveData save{};
NeiSaveData* Nei_Save(){return &save;}
void Nei_SetOwnedItem(uint8_t slot,uint16_t item){save.ownedItems[slot-24]=item;}
''',[body('mm/mods/extended_inventory.c','Seasons_GrantSeason')],r'''
Seasons_GrantSeason(SEASON_WINTER);
assert(save.seasonsOwned==(1<<SEASON_WINTER));assert(save.season==SEASON_WINTER);
assert(save.ownedItems[SLOT_ROD_OF_SEASONS-24]==EXT_ITEM_ROD_OF_SEASONS);
Seasons_GrantSeason(SEASON_SPRING);assert(save.seasonsOwned==9&&save.season==SEASON_SPRING);
Seasons_GrantSeason(255);assert(save.seasonsOwned==9);
''')

if len(sys.argv)==1 or sys.argv[1]=='pool':
 text=(ROOT/'mm/2s2h/Rando/Logic/GeneratePools.cpp').read_text()
 pool=re.search(r'sNeiPoolItems\[\]\s*=\s*\{(.*?)\};',text,re.S).group(1)
 assert not re.search(r'\bRI_OOT_NEI_ROD_OF_SEASONS\b',pool),'bare rod must not be a placeable reward'
 for s in ['SPRING','SUMMER','AUTUMN','WINTER']: assert 'RI_OOT_NEI_SEASON_'+s in text
 assert 'NEI_SEASONS_INDIVIDUAL' in text and 'RO_STARTING_ROD_OF_SEASONS' in text
 print('PASS pool')

shop='mm/2s2h/Rando/ActorBehavior/EnGirlA.cpp'
run('shop',r'''
#include "overlays/actors/ovl_En_GirlA/z_en_girla.h"
SaveContext gSaveContext{};
#define RANDO_SAVE_CHECKS gSaveContext.save.shipSaveInfo.rando.randoSaveChecks
bool CanBePurchased(RandoSaveCheck,RandoCheckId){return true;}
void EnGirlA_RandoDrawFunc(Actor*,PlayState*){}
void NativeDraw(Actor*,PlayState*){}
void EnGirlA_InitialUpdate(EnGirlA* item,PlayState*){item->actor.draw=NativeDraw;item->isOutOfStock=false;}
''',[body(shop,'EnGirlA_RandoRestock')],r'''
PlayState play{};EnGirlA item{};item.actor.world.rot.z=RC_BOMB_SHOP_ITEM_01;
item.actor.params=SI_BOMB_1;
RANDO_SAVE_CHECKS[RC_BOMB_SHOP_ITEM_01].obtained=true;
EnGirlA_RandoRestock(&play,&item);
assert(item.actor.params==SI_BOMB_1 && item.actor.draw==NativeDraw);
''')

run('menu',r'''
#define BOXM_STICK_DEAD 30
u8 sBoxMOpen=1,sBoxMHoldSeen=0,sBoxMStickHeld=0;
s16 sBoxMPulse=0;u16 sBoxMHoldButton=0;s32 sBoxMCursor=0;
int confirmed=0;
void BoxMenu_PlaySfx(u16){}
void BoxMenu_Close(PlayState*,u8 confirm){confirmed=confirm;sBoxMOpen=0;}
s32 BoxMenu_Step(s32 from,s32){return from;}
''',[body('mm/mods/items/helpers/box_menu.c','BoxMenu_Update')],r'''
PlayState play{};play.state.input[0].press.button=BTN_A;
BoxMenu_Update(&play);assert(confirmed==1&&!sBoxMOpen);
''')

rodpath=ROOT/'mm/mods/items/logic/item_rod_of_seasons.c'
rodsource=rodpath.read_text() if rodpath.exists() else 'void Seasons_TickInput(PlayState*,Player*,Input*){}\nu8 Seasons_IsDrawn(){return 0;}'
run('rod_input',r'''
#include "mods/items/helpers/equip_helper.h"
SaveContext gSaveContext{};PlayState* gPlayState=nullptr;
NeiSaveData* Nei_Save(){return &gSaveContext.save.shipSaveInfo.nei;}
void Nei_SetOwnedItem(u8 slot,u16 item){Nei_Save()->ownedItems[slot-24]=item;}
void Seasons_UpdateWeather(PlayState*){}
void Player_UseItem(PlayState*,Player*,ItemId){}
s32 Player_UpperAction_ChangeHeldItem(Player*,PlayState*){return 0;}
void ItemEquip_PlayEquipSFX(PlayState*,Player*){}
void ItemEquip_PlayUnequipSFX(PlayState*,Player*){}
u8 ItemInput_IsBlocked(Player*,PlayState*){return 0;}
u8 ItemInput_CheckDamage(Player*,s8*){return 0;}
u8 MasterCycle_IsRiding(){return 0;}
u8 Pacci_UltrahandModeActive(){return 0;}
extern "C" int32_t CVarGetInteger(const char*,int32_t){return 1;}
void Interface_LoadItemIconImpl(PlayState*,u8){}
void Interface_Dpad_LoadItemIconImpl(PlayState*,u8){}
int menus=0,seasonStyles=0,seasonFlashes=0;typedef struct {const char* iconPath;u8 iconSize;u8 enabled;} BoxMenuEntry;
typedef void (*BoxMenuConfirmFn)(s32);
BoxMenuConfirmFn confirmation=nullptr;BoxMenuEntry entries[5];
u8 BoxMenu_IsOpen(){return 0;}
void MM_BoxMenu_UseSeasonStyle(){++seasonStyles;}
void MM_BoxMenu_StartSeasonFlash(PlayState* play){assert(play==gPlayState);++seasonFlashes;}
u8 BoxMenu_Open(PlayState*,const BoxMenuEntry* values,s32 n,s32,u16 hold,BoxMenuConfirmFn fn){
 assert(n==5&&hold==0);memcpy(entries,values,sizeof(entries));confirmation=fn;++menus;return 1;}
void ExtInv_RefreshButtonIconsForItem(PlayState*,u16){}
''',[(ROOT/'mm/mods/ext_buttons/ext_buttons.cpp').read_text().replace('#include "ext_buttons.h"','#include "mods/ext_buttons/ext_buttons.h"')]+[body('mm/mods/extended_inventory.c',n) for n in ['Seasons_SeasonOwned','Seasons_GrantSeason','Seasons_SeasonCount','Seasons_SeasonAt','Seasons_GetSeason','Seasons_SetSeason']]+[rodsource.split('// Seasonal particles')[0]],r'''
PlayState play{};Player player{};gPlayState=&play;player.transformation=PLAYER_FORM_HUMAN;
Seasons_GrantSeason(SEASON_WINTER);ExtButton_SetDpadItem(0,EQUIP_SLOT_D_UP,EXT_ITEM_ROD_OF_SEASONS);
play.state.input[0].press.button=BTN_DUP;Seasons_TickInput(&play,&player,&play.state.input[0]);
assert(Seasons_IsDrawn()&&menus==0);
play.state.input[0].press.button=BTN_DUP;Seasons_TickInput(&play,&player,&play.state.input[0]);assert(menus==1&&confirmation&&seasonStyles==1);
assert(entries[SEASON_WINTER].enabled&&entries[SEASON_OFF].enabled&&!entries[SEASON_SUMMER].enabled);
confirmation(SEASON_SUMMER);assert(Seasons_GetSeason()==SEASON_WINTER&&seasonFlashes==0);
confirmation(SEASON_OFF);assert(Seasons_GetSeason()==SEASON_OFF&&seasonFlashes==1);
confirmation(SEASON_OFF);assert(Seasons_GetSeason()==SEASON_OFF&&seasonFlashes==1);
play.state.input[0].press.button=BTN_B;Seasons_TickInput(&play,&player,&play.state.input[0]);assert(Seasons_IsDrawn());
''')

codec=(ROOT/'mm/2s2h/BenJsonConversions.hpp').read_text()
codec=codec[codec.index('inline void to_json(json& j, const DpadSaveInfo&'):codec.index('// Extended-button storage')]
run('save',r'''
#include <nlohmann/json.hpp>
using json=nlohmann::json;
SaveContext gSaveContext{};
void Interface_LoadItemIconImpl(PlayState*,u8){}
void Interface_Dpad_LoadItemIconImpl(PlayState*,u8){}
''',[(ROOT/'mm/mods/ext_buttons/ext_buttons.cpp').read_text().replace('#include "ext_buttons.h"','#include "mods/ext_buttons/ext_buttons.h"'),codec],r'''
ExtButton_SetDpadItem(0,EQUIP_SLOT_D_UP,EXT_ITEM_ROD_OF_SEASONS);
DPAD_SLOT_EQUIP(0,EQUIP_SLOT_D_UP)=95;
ExtButton_SetDpadItem(0,EQUIP_SLOT_D_DOWN,ITEM_MUSHROOM);
DPAD_SLOT_EQUIP(0,EQUIP_SLOT_D_DOWN)=SLOT_BOTTLE_1;
json j=gSaveContext.save.shipSaveInfo.dpadEquips;
memset(&gSaveContext.save.shipSaveInfo.dpadEquips,0,sizeof(DpadSaveInfo));
j.get_to(gSaveContext.save.shipSaveInfo.dpadEquips);
assert(ExtButton_GetDpadItem(0,EQUIP_SLOT_D_UP)==EXT_ITEM_ROD_OF_SEASONS);
assert(ExtButton_GetDpadItem(0,EQUIP_SLOT_D_DOWN)==ITEM_MUSHROOM);
// Pre-fix saves contain only raw u8 items and slots; recover the rod by its owned custom slot.
j.erase("extItems");j["dpadItems"][0][EQUIP_SLOT_D_UP]=ITEM_MUSHROOM;
j.get_to(gSaveContext.save.shipSaveInfo.dpadEquips);
gSaveContext.save.shipSaveInfo.nei.ownedItems[23]=EXT_ITEM_ROD_OF_SEASONS;
assert(ExtButton_GetDpadItem(0,EQUIP_SLOT_D_UP)==EXT_ITEM_ROD_OF_SEASONS);
assert(ExtButton_GetDpadItem(0,EQUIP_SLOT_D_DOWN)==ITEM_MUSHROOM);
assert(gSaveContext.save.shipSaveInfo.dpadEquips.extItems[0][EQUIP_SLOT_D_DOWN]==0);
''')

run('icon',r'''
#include "2s2h_assets.h"
SaveContext gSaveContext{};TexturePtr gItemIcons[131]{};
extern "C" int32_t CVarGetInteger(const char*,int32_t){return 0;}
int GameInteractor_Should(int,int result,...){return result;}
#define VB_INTERFACE_LOAD_DPAD_ITEM_ICON 0
void* ExtInv_GetItemIcon(u16 item){return item==EXT_ITEM_ROD_OF_SEASONS?(void*)0x1000:nullptr;}
void Interface_LoadItemIconImpl(PlayState*,u8){}
''',[(ROOT/'mm/mods/ext_buttons/ext_buttons.cpp').read_text().replace('#include "ext_buttons.h"','#include "mods/ext_buttons/ext_buttons.h"'),body('mm/src/code/z_parameter.c','Interface_Dpad_LoadItemIconImpl')],r'''
PlayState play{};char* icons[8]{};play.interfaceCtx.iconItemSegment=icons;
ExtButton_SetDpadItem(0,EQUIP_SLOT_D_UP,EXT_ITEM_ROD_OF_SEASONS);
Interface_Dpad_LoadItemIconImpl(&play,EQUIP_SLOT_D_UP);
assert(play.interfaceCtx.iconItemSegment[EQUIP_SLOT_D_UP+EQUIP_SLOT_MAX]==(void*)0x1000);
gItemIcons[ITEM_MUSHROOM]=(void*)0x2000;
ExtButton_SetDpadItem(0,EQUIP_SLOT_D_UP,ITEM_MUSHROOM);
Interface_Dpad_LoadItemIconImpl(&play,EQUIP_SLOT_D_UP);
assert(play.interfaceCtx.iconItemSegment[EQUIP_SLOT_D_UP+EQUIP_SLOT_MAX]==(void*)0x2000);
''')

nei_codec=(ROOT/'mm/2s2h/BenJsonConversions.hpp').read_text()
nei_codec=nei_codec[nei_codec.index('inline void to_json(json& j, const NeiSaveData&'):nei_codec.index('// Spiritual Stones')]
run('season_save',r'''
#include <nlohmann/json.hpp>
#include "rando/RpgStatsJson.h"
using json=nlohmann::json;
''',[nei_codec],r'''
NeiSaveData original{};
original.seasonsOwned=(1<<SEASON_WINTER)|(1<<SEASON_SUMMER);
original.season=SEASON_WINTER;
original.ownedItems[SLOT_ROD_OF_SEASONS-24]=EXT_ITEM_ROD_OF_SEASONS;
json j=original;
NeiSaveData restored{};j.get_to(restored);
assert(restored.seasonsOwned==original.seasonsOwned&&restored.season==SEASON_WINTER);
assert(restored.ownedItems[SLOT_ROD_OF_SEASONS-24]==EXT_ITEM_ROD_OF_SEASONS);
j.erase("season");j.erase("seasonsOwned");j.get_to(restored);
assert(restored.seasonsOwned==0&&restored.season==SEASON_SPRING);
''')

run('snow',r'''
#include "mods/items/helpers/season_particles.h"
int activeSeason=SEASON_WINTER,spawns=0;
int snowEnabled=1;
extern "C" int32_t CVarGetInteger(const char* key,int32_t fallback){
 return !strcmp(key,"gAudioEditor.MMWeather.Winter.Snow") ? snowEnabled : fallback;
}
u8 D_801F4E30;
int MMWeather_Season(){return activeSeason;}
int MMWeather_SeasonForPlay(PlayState*){return activeSeason;}
void Actor_Kill(Actor* actor){actor->update=nullptr;actor->draw=nullptr;}
Actor* Actor_Spawn(ActorContext*,PlayState*,s16,f32,f32,f32,s16,s16,s16,s32){++spawns;return nullptr;}
''',['// Seasonal particles'+rodsource.split('// Seasonal particles')[1].split('#include "../objects')[0]],r'''
PlayState play{};play.envCtx.precipitation[PRECIP_SNOW_MAX]=12;
Seasons_UpdateWeather(&play);assert(play.envCtx.precipitation[PRECIP_SNOW_MAX]==12&&spawns==1);
Actor snow{};snow.id=ACTOR_OBJECT_KANKYO;snow.params=2;snow.update=[](Actor*,PlayState*){};play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first=&snow;
Seasons_UpdateWeather(&play);assert(spawns==1);
activeSeason=SEASON_SPRING;Seasons_UpdateWeather(&play);assert(play.envCtx.precipitation[PRECIP_SNOW_MAX]==12&&play.envCtx.precipitation[PRECIP_SNOW_CUR]==0);
activeSeason=-1;Seasons_UpdateWeather(&play);assert(play.envCtx.precipitation[PRECIP_SNOW_MAX]==12);
activeSeason=SEASON_WINTER;Seasons_UpdateWeather(&play);
play.roomCtx.curRoom.num=1;activeSeason=-1;Seasons_UpdateWeather(&play);
assert(play.envCtx.precipitation[PRECIP_SNOW_MAX]==12);
activeSeason=SEASON_WINTER;Seasons_UpdateWeather(&play);
play.envCtx.precipitation[PRECIP_SNOW_MAX]=20;activeSeason=-1;Seasons_UpdateWeather(&play);
assert(play.envCtx.precipitation[PRECIP_SNOW_MAX]==20);
// Exercise the production presentation switch without replacing its policy.
activeSeason=SEASON_WINTER;snowEnabled=0;
int before=spawns;Seasons_UpdateWeather(&play);
assert(spawns==before&&play.envCtx.precipitation[PRECIP_SNOW_MAX]==20);
snowEnabled=1;Seasons_UpdateWeather(&play);
assert(play.envCtx.precipitation[PRECIP_SNOW_MAX]==20);
''')

run('winter_water',r'''
int activeSeason=SEASON_WINTER;u8 bootsWorn=0;static u8 sRocOnWater=0;
int MMWeather_Season(){return activeSeason;}
u8 RocBoots_IsWorn(){return bootsWorn;}
''',[(body('mm/mods/items/logic/item_rod_of_seasons.c','Seasons_WalksOnWater') if 'Seasons_WalksOnWater' in rodsource else 'u8 Seasons_WalksOnWater(){return 0;}'), body('mm/mods/equipment/behaviors/equip_roc_boots.c','RocBoots_WalksOnWater')],r'''
Player p{};p.transformation=PLAYER_FORM_HUMAN;p.actor.depthInWater=1;
assert(RocBoots_WalksOnWater(&p)==2);assert(RocBoots_WalksOnWater(&p)==1);
p.stateFlags1|=PLAYER_STATE1_8000000;assert(!RocBoots_WalksOnWater(&p));
p.stateFlags1=0;p.actor.velocity.y=1;assert(!RocBoots_WalksOnWater(&p));
p.actor.velocity.y=0;activeSeason=-1;assert(!RocBoots_WalksOnWater(&p));
bootsWorn=1;assert(RocBoots_WalksOnWater(&p)==2);
p.transformation=PLAYER_FORM_ZORA;assert(!RocBoots_WalksOnWater(&p));
''')

run('native_dpad',r'''
SaveContext gSaveContext{};
u8 MasterCycle_IsRiding(){return 0;}
int GameInteractor_Should(int,int result,...){return result;}
#define VB_GET_ITEM_ON_BUTTON 0
void Interface_LoadItemIconImpl(PlayState*,u8){}
void Interface_Dpad_LoadItemIconImpl(PlayState*,u8){}
''',[(ROOT/'mm/mods/ext_buttons/ext_buttons.cpp').read_text().replace('#include "ext_buttons.h"','#include "mods/ext_buttons/ext_buttons.h"'),body('mm/src/code/z_player_lib.c','Player_Dpad_GetItemOnButton')],r'''
PlayState play{};Player p{};
ExtButton_SetDpadItem(0,EQUIP_SLOT_D_UP,EXT_ITEM_ROD_OF_SEASONS);
assert(Player_Dpad_GetItemOnButton(&play,&p,EQUIP_SLOT_D_UP)==ITEM_EXT_BUTTON);
ExtButton_SetDpadItem(0,EQUIP_SLOT_D_UP,ITEM_MUSHROOM);
assert(Player_Dpad_GetItemOnButton(&play,&p,EQUIP_SLOT_D_UP)==ITEM_MUSHROOM);
''')

merges=[]
for game,path in [('MM','mm/2s2h/FleetShipCombo/FleetSync.cpp'),('OOT','soh/soh/FleetShipCombo/FleetSync.cpp')]:
 source=(ROOT/path).read_text()
 merges.append('void Apply'+game+'(const json& sh){NeiSaveData* nei=Nei_Save();'+block_from(source,source.index('if (sh.contains("seasonsOwned")'))+'}')
run('cross_game',r'''
#include <nlohmann/json.hpp>
using json=nlohmann::json;
NeiSaveData save{};
NeiSaveData* Nei_Save(){return &save;}
void Nei_SetOwnedItem(u8 slot,u16 item){save.ownedItems[slot-24]=item;}
''',[body('mm/mods/extended_inventory.c','Seasons_GrantSeason')]+merges,r'''
for(auto apply:{ApplyMM,ApplyOOT}){
 save={};Seasons_GrantSeason(SEASON_SPRING);
 apply(json{{"seasonsOwned",1<<SEASON_WINTER}});
 assert(save.seasonsOwned==9&&save.season==SEASON_WINTER);
 assert(save.ownedItems[SLOT_ROD_OF_SEASONS-24]==EXT_ITEM_ROD_OF_SEASONS);
 save.season=SEASON_OFF;apply(json{{"seasonsOwned",1<<SEASON_WINTER}});
 assert(save.seasonsOwned==9&&save.season==SEASON_OFF);
 apply(json{{"seasonsOwned",0}});assert(save.seasonsOwned==9);
}
''')

run('shop_purchase',r'''
#define COMBO_BUILD 1
#include "overlays/actors/ovl_En_GirlA/z_en_girla.h"
SaveContext gSaveContext{};
#define RANDO_SAVE_CHECKS gSaveContext.save.shipSaveInfo.rando.randoSaveChecks
#define RANDO_SAVE_OPTIONS gSaveContext.save.shipSaveInfo.rando.randoSaveOptions
int charges=0,grants=0,foreign=0,broadcasts=0;
void Rupees_ChangeBy(s16){++charges;}
void RollTrapType(){}
namespace Rando {
RandoItemId ConvertItem(RandoItemId id,RandoCheckId){return id;}
bool IsItemObtainable(RandoItemId,RandoCheckId){return true;}
RandoItemId CurrentJunkItem(RandoCheckId){return RI_RUPEE_BLUE;}
void GiveItem(RandoItemId){++grants;}
void LatchComboForeign(RandoCheckId){}
namespace MiscBehavior {
void SendForeignCheck(RandoCheckId){++foreign;}
void BroadcastCheckObtainedIfFirst(RandoCheckId,RandoItemId,bool wasObtained){assert(!wasObtained);++broadcasts;}
}}
''',[body(shop,'CanBePurchased'),body(shop,'EnGirlA_RandoBuyFunc')],r'''
PlayState play{};EnGirlA shelf{};shelf.actor.world.rot.z=RC_BOMB_SHOP_ITEM_01;
auto& check=RANDO_SAVE_CHECKS[RC_BOMB_SHOP_ITEM_01];check.randoItemId=RI_OOT_NEI_SEASON_WINTER;
assert(CanBePurchased(check,RC_BOMB_SHOP_ITEM_01));
EnGirlA_RandoBuyFunc(&play,&shelf);assert(charges==1&&grants==1&&broadcasts==1&&check.obtained);
assert(!CanBePurchased(check,RC_BOMB_SHOP_ITEM_01));
EnGirlA_RandoBuyFunc(&play,&shelf);assert(charges==1&&grants==1&&broadcasts==1);
check.obtained=false;check.randoItemId=RI_COMBO_FOREIGN;
EnGirlA_RandoBuyFunc(&play,&shelf);assert(charges==2&&grants==1&&foreign==1);
EnGirlA_RandoBuyFunc(&play,&shelf);assert(charges==2&&foreign==1);
''')

run('held',r'''
#include "mods/items/helpers/equip_helper.h"
#include "2s2h/Rando/NeiHeldPresentation.h"
#include "2s2h/Rando/NeiResourceRouting.h"
bool drawn=true,mod=false,redesign=true,hand=true;int models=0,depth=0;
const char* lastModel=nullptr;
u8 Seasons_IsDrawn(){return drawn;}
void Matrix_Push(){++depth;}void Matrix_Pop(){--depth;}
u8 ItemEquip_ApplyHandPose(Player*,const ItemHandPose*){return hand;}
int NeiResource_IsMod(const char*){return mod;}
bool NeiHeld_HasResources(const char* model,const char*){
 // The shared renderer probes OoT and supplies the @oot route itself.
 assert(!strstr(model,"@oot:"));return redesign;
}
bool NeiHeld_DrawModel(PlayState*,const char* model,const char*){++models;lastModel=model;return true;}
''',[body('mm/mods/items/objects/object_rod_of_seasons.c','CustomItems_DrawRodOfSeasons')],r'''
PlayState play{};Player player{};
CustomItems_DrawRodOfSeasons(&player,&play);assert(strstr(lastModel,"nei_held_redesign/rod_of_seasons/gi_dl"));
mod=true;CustomItems_DrawRodOfSeasons(&player,&play);assert(strstr(lastModel,"object_nei_rod_of_seasons"));
mod=false;redesign=false;CustomItems_DrawRodOfSeasons(&player,&play);assert(strstr(lastModel,"object_nei_rod_of_seasons"));
int before=models;drawn=false;CustomItems_DrawRodOfSeasons(&player,&play);assert(models==before);
drawn=true;hand=false;CustomItems_DrawRodOfSeasons(&player,&play);assert(models==before&&depth==0);
''')

snowpath='mm/src/overlays/actors/ovl_Object_Kankyo/z_object_kankyo.c'
snowfunctions=['ObjectKankyo_SetupAction','func_808DC454','func_808DCB7C','func_808DCBF8','func_808DBEB0','func_808DBFB0','ObjectKankyo_Init','ObjectKankyo_Update']
autumnfunctions=['ObjectKankyo_IsAutumnOwner','ObjectKankyo_AutumnBand',
                 'ObjectKankyo_IsCompactAutumnScene','ObjectKankyo_AutumnViewBasis','ObjectKankyo_InitAutumnParticle',
                 'ObjectKankyo_UpdateAutumnParticles','ObjectKankyo_RestoreAutumnParticle']
if 'ObjectKankyo_UpdateSnowTarget' in (ROOT/snowpath).read_text():
 snowfunctions.insert(2,'ObjectKankyo_UpdateSnowTarget')
if 'ObjectKankyo_UpdateSeasonSnowParticles' in (ROOT/snowpath).read_text():
 snowfunctions.insert(2,'ObjectKankyo_UpdateSeasonSnowParticles')
if 'ObjectKankyo_IsSeasonSnowOwner' in (ROOT/snowpath).read_text():
 snowfunctions.insert(2,'ObjectKankyo_IsSeasonSnowOwner')
snowfunctions=autumnfunctions+snowfunctions
run('snow_native',r'''
#include "overlays/actors/ovl_Object_Kankyo/z_object_kankyo.h"
int activeSeason=SEASON_WINTER,spawns=0;
float D_808DE5B0=0;u8 D_801F4E30=0;u16 D_808DE340=0;
int MMWeather_Season(){return activeSeason;}
int MMWeather_SeasonForPlay(const PlayState*){return activeSeason;}
void Actor_Kill(Actor* actor){actor->update=nullptr;actor->draw=nullptr;}
f32 Rand_ZeroOne(){return .25f;}
f32 MMWeather_RandomFloat(){assert(MMSeasonParticles_IsFallingSeason(activeSeason));return .25f;}
// Window aspect is an external boundary; use the native 4:3 fixture viewport.
f32 OTRGetAspectRatio(){return 4.0f/3.0f;}
s16 Camera_GetCamDirPitch(Camera*){return 0;}
f32 Math_Vec3f_DistXZ(Vec3f* a,Vec3f* b){return sqrtf(SQ(a->x-b->x)+SQ(a->z-b->z));}
void func_808DBE8C(ObjectKankyo*){}
void func_808DC038(ObjectKankyo*,PlayState*){}
void ObjectKankyo_Init(Actor*,PlayState*);
void ObjectKankyo_Update(Actor*,PlayState*);
ObjectKankyo spawnedSnow{};
Actor* Actor_Spawn(ActorContext* ctx,PlayState* play,s16 id,f32,f32,f32,s16,s16,s16,s32 params){
 ++spawns;spawnedSnow={};spawnedSnow.actor.id=id;spawnedSnow.actor.params=params;spawnedSnow.actor.update=ObjectKankyo_Update;
 ObjectKankyo_Init(&spawnedSnow.actor,play);
 spawnedSnow.actor.next=ctx->actorLists[ACTORCAT_ITEMACTION].first;
 ctx->actorLists[ACTORCAT_ITEMACTION].first=&spawnedSnow.actor;
 return &spawnedSnow.actor;
}
''',[re.sub(r'\bthis\b','self',body(snowpath,n)) for n in snowfunctions]+['// Seasonal particles'+rodsource.split('// Seasonal particles')[1].split('#include "../objects')[0]],r'''
PlayState play{};Camera camera{};play.cameraPtrs[0]=&camera;play.view.at.z=1;
ObjectKankyo first{},second{};
first.actor.id=second.actor.id=ACTOR_OBJECT_KANKYO;
first.actor.params=second.actor.params=2;
first.actor.update=second.actor.update=ObjectKankyo_Update;
ObjectKankyo_Init(&first.actor,&play);ObjectKankyo_Init(&second.actor,&play);
assert(first.unk_114C==0&&second.unk_114C==1);
assert(first.actor.room==-1&&second.actor.room==-1);
first.actor.next=&second.actor;play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first=&first.actor;
play.envCtx.precipitation[PRECIP_SNOW_MAX]=12;
Seasons_UpdateWeather(&play);assert(spawns==0);
play.state.frames=16;
ObjectKankyo_Update(&first.actor,&play);ObjectKankyo_Update(&second.actor,&play);
assert(play.envCtx.precipitation[PRECIP_SNOW_CUR]==119); // native first blizzard still drains by nine
assert(first.unk_14C[0].epoch==1&&second.unk_14C[0].epoch==1); // actual particle motion ran once per native actor
for(int i=0;i<80;i++){play.state.frames+=16;Seasons_UpdateWeather(&play);ObjectKankyo_Update(&first.actor,&play);ObjectKankyo_Update(&second.actor,&play);}
assert(play.envCtx.precipitation[PRECIP_SNOW_CUR]==0&&spawns==0);
assert(first.actor.params==2&&first.actionFunc==func_808DCBF8);
assert(play.envCtx.sandstormState==SANDSTORM_A&&D_801F4E30==0);
// Spring hides snow without overwriting an odd native count or target.
static_assert(PRECIP_SNOW_CUR==2&&PRECIP_SNOW_MAX==3&&WEATHER_MODE_SNOW==3);
play.envCtx.precipitation[PRECIP_SNOW_CUR]=119;activeSeason=SEASON_SPRING;
Seasons_UpdateWeather(&play);
assert(play.envCtx.precipitation[PRECIP_SNOW_CUR]==119&&play.envCtx.precipitation[PRECIP_SNOW_MAX]==12);
for(int i=0;i<80;i++){play.state.frames+=16;Seasons_UpdateWeather(&play);ObjectKankyo_Update(&first.actor,&play);ObjectKankyo_Update(&second.actor,&play);}
assert(play.envCtx.precipitation[PRECIP_SNOW_CUR]==0);
// Off exposes the live native state: its blizzard already drained while Spring was active.
activeSeason=-1;Seasons_UpdateWeather(&play);assert(play.envCtx.precipitation[PRECIP_SNOW_MAX]==12);
play.state.frames+=16;ObjectKankyo_Update(&first.actor,&play);ObjectKankyo_Update(&second.actor,&play);
assert(play.envCtx.precipitation[PRECIP_SNOW_CUR]==0);
// Native instance numbering persists: a later scene can have no params2 index zero.
PlayState revisit{};revisit.cameraPtrs[0]=&camera;revisit.view.at.z=1;
ObjectKankyo later{};later.actor.id=ACTOR_OBJECT_KANKYO;later.actor.params=2;later.actor.update=ObjectKankyo_Update;
ObjectKankyo_Init(&later.actor,&revisit);assert(later.unk_114C>0);
revisit.actorCtx.actorLists[ACTORCAT_ITEMACTION].first=&later.actor;
activeSeason=SEASON_WINTER;Seasons_UpdateWeather(&revisit);revisit.state.frames=16;
ObjectKankyo_Update(&later.actor,&revisit);
assert(revisit.envCtx.precipitation[PRECIP_SNOW_CUR]==128);
activeSeason=-1;Seasons_UpdateWeather(&revisit);revisit.state.frames+=16;
ObjectKankyo_Update(&later.actor,&revisit);
assert(revisit.envCtx.precipitation[PRECIP_SNOW_CUR]==128); // restore native count and indexed behavior when Off
// Normal snow follows its native target while blizzard particles still update for Winter.
ObjectKankyo normal{};normal.actor.id=ACTOR_OBJECT_KANKYO;normal.actor.params=3;normal.actor.update=ObjectKankyo_Update;
ObjectKankyo_Init(&normal.actor,&revisit);normal.actor.next=&later.actor;
revisit.actorCtx.actorLists[ACTORCAT_ITEMACTION].first=&normal.actor;
activeSeason=SEASON_WINTER;Seasons_UpdateWeather(&revisit);revisit.state.frames+=16;
ObjectKankyo_Update(&normal.actor,&revisit);ObjectKankyo_Update(&later.actor,&revisit);
assert(revisit.envCtx.precipitation[PRECIP_SNOW_CUR]==126&&spawns==0);
// Normal snow retains native target tracking; a rod-created actor is scoped to its room.
PlayState next{};next.cameraPtrs[0]=&camera;next.view.at.z=1;next.roomCtx.curRoom.num=5;
activeSeason=SEASON_WINTER;Seasons_UpdateWeather(&next);
assert(spawns==1&&spawnedSnow.actor.params==1&&spawnedSnow.actor.room==5);
next.state.frames=16;ObjectKankyo_Update(&spawnedSnow.actor,&next);
assert(next.envCtx.precipitation[PRECIP_SNOW_CUR]==0&&spawnedSnow.unk_14C[0].epoch==1);
Seasons_UpdateWeather(&next);assert(spawns==1);
activeSeason=-1;Seasons_UpdateWeather(&next);
assert(next.envCtx.precipitation[PRECIP_SNOW_CUR]==0&&spawnedSnow.actor.update==nullptr);
''')
