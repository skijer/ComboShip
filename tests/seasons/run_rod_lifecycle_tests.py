"""Run the rod through native effective input, button dispatch and use/finish/init."""
import ast
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_mm_nei_tests import flags
from run_mm_weather_tests import production_function

def body(path,name): return production_function((ROOT/path).read_text(),name)
def native_decl(text,start):
 begin=text.index(start);end=begin+re.search(r'\n}[^;\n]*;',text[begin:]).end()
 return text[begin:end]
# Reuse the existing real-native-call fixture's unrelated engine boundaries without running its tests.
fixture=ast.parse((ROOT/'tests/mm_nei/run_use_tests.py').read_text())
prefix=next(ast.literal_eval(n.value) for n in fixture.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='caller_prefix' for t in n.targets))
prefix='#include "mods/extended_inventory.h"\n'+prefix
for name in ['Player_GetItemOnButton', 'Player_Dpad_GetItemOnButton']:
 function = production_function(prefix, name)
 prefix = prefix.replace(function, '')
prefix=prefix.replace('void CustomItems_PutAwayHeldItems(Player* p,PlayState* play){}','static void Seasons_Stow(PlayState*,Player*);\nvoid CustomItems_PutAwayHeldItems(Player* p,PlayState* play){Seasons_Stow(play,p);}')
prefix=prefix.replace('void Player_InitItemActionWithAnim(PlayState* play,Player* p,PlayerItemAction action){++initializations;p->heldItemAction=p->itemAction=action;p->modelGroup=p->nextModelGroup;}','void Player_InitItemAction(PlayState*,Player*,PlayerItemAction);\nvoid Player_InitItemActionWithAnim(PlayState* play,Player* p,PlayerItemAction action){++initializations;Player_InitItemAction(play,p,action);}')
prefix=prefix.replace('void Player_StartChangingHeldItem(Player* p,PlayState* play){++transitions;animationDestination=p->nextModelGroup;p->stateFlags3&=~PLAYER_STATE3_START_CHANGING_HELD_ITEM;}','s32 Player_UpperAction_ChangeHeldItem(Player*,PlayState*);\nvoid Player_StartChangingHeldItem(Player* p,PlayState* play){++transitions;animationDestination=p->nextModelGroup;p->stateFlags3&=~PLAYER_STATE3_START_CHANGING_HELD_ITEM;p->upperActionFunc=Player_UpperAction_ChangeHeldItem;}')
prefix+=r'''
#include "overlays/actors/ovl_En_Horse/z_en_horse.h"
PlayState* gPlayState;
int rodEquips,rodStows,menus,menuOpen,sceneNumber;
static s32 sUpperBodyIsBusy;
s32 Player_UpdateHostileLockOn(Player* p){return 0;}
void func_8083A04C(Player* p){}
s32 func_80839F98(PlayState* play,Player* p){return 0;}
s32 func_80839A84(PlayState* play,Player* p){return 0;}
s32 func_80839800(Player* p,PlayState* play){return 0;}
u8 Seasons_IsDrawn(void);
s32 CustomItems_HasStowableHeldItem(Player* p){return Seasons_IsDrawn();}
static s32 sPlayerFloorType;
bool func_801242B4(Player* p){return false;}
bool Player_InBlockingCsMode(PlayState* play,Player* p){return p->csAction!=0;}
bool Player_CheckHostileLockOn(Player* p){return false;}
bool Player_FriendlyLockOnOrParallel(Player* p){return false;}
bool Player_IsZTargeting(Player* p){return false;}
bool func_800B7128(Player* p){return false;}
s32 func_800C9DDC(CollisionContext* ctx,CollisionPoly* poly,s32 id){return 0;}
s32 Player_CanThrowCarriedActor(Player* p,Actor* a){return 0;}
bool Play_InCsMode(PlayState* play){return false;}
void Player_Action_28(Player* p,PlayState* play){}
void Player_Action_53(Player* p,PlayState* play){}
void Player_Action_80(Player* p,PlayState* play){}
void Interface_SetAButtonDoAction(PlayState* play,u16 action){play->interfaceCtx.aButtonDoAction=action;}
void Interface_SetBButtonPlayerDoAction(PlayState* play,s16 action){play->interfaceCtx.bButtonPlayerDoAction=action;}
void Interface_SetTatlCall(PlayState* play,u16 call){}
void CutsceneManager_Queue(s16 id){}
u16 hookSuppressed;u8 movementBlocked;
s32 CustomItems_BlocksMovement(Player* p){return movementBlocked;}
void GameInteractor_ExecuteOnPassPlayerInputs(Input* input){
 input->cur.button&=~hookSuppressed;input->press.button&=~hookSuppressed;
}
u8 MasterCycle_IsRiding(void){return 0;}
u8 Pacci_UltrahandModeActive(void){return 0;}
int MMWeather_Season(void){return -1;}
void Seasons_UpdateWeather(PlayState* play){}
NeiSaveData* Nei_Save(void){return &gSaveContext.save.shipSaveInfo.nei;}
void Nei_SetOwnedItem(u8 slot,u16 item){Nei_Save()->ownedItems[slot-24]=item;}
u16 Nei_GetOwnedItem(u8 slot){return Nei_Save()->ownedItems[slot-24];}
u8 Seasons_RandoMode(void){return NEI_SEASONS_INDIVIDUAL;}
void Seasons_UpdateGates(void){}
u16 ExtButton_GetItem(s32 form,s32 btn){return BUTTON_ITEM_EQUIP(form,btn)==ITEM_EXT_BUTTON?EXT_BUTTON_ITEM(form,btn):BUTTON_ITEM_EQUIP(form,btn);}
u16 ExtButton_GetDpadItem(s32 form,s32 btn){return gSaveContext.save.shipSaveInfo.dpadEquips.extItems[form][btn];}
void ItemEquip_PlayEquipSFX(PlayState* play,Player* p){++rodEquips;}
void ItemEquip_PlayUnequipSFX(PlayState* play,Player* p){++rodStows;}
void ItemEquip_ResetUnequipSound(PlayState* play,Player* p,s32 action){}
u8 ItemEquip_ClaimUnequipSound(PlayState* play,Player* p,s32 action){return 0;}
PlayerSword Player_SwordFromIA(Player* p,PlayerItemAction a){return a==PLAYER_IA_SWORD_KOKIRI?PLAYER_SWORD_KOKIRI:PLAYER_SWORD_NONE;}
void NativeInitNone(PlayState* play,Player* p){}
ItemActionInitFunc ExtPlayer_GetItemActionInitFunc(s32 action){return NativeInitNone;}
void Player_SetModelGroup(Player* p,PlayerModelGroup group){p->modelGroup=group;p->rightHandType=PLAYER_MODELTYPE_RH_OPEN;}
s32 Player_UpperAction_ChangeHeldItem(Player* p,PlayState* play){return 0;}
typedef struct {const char* iconPath;u8 iconSize;u8 enabled;} BoxMenuEntry;
typedef void (*BoxMenuConfirmFn)(s32);
u8 BoxMenu_IsOpen(void){return menuOpen;}
void MM_BoxMenu_UseSeasonStyle(void){}
void MM_BoxMenu_StartSeasonFlash(PlayState* play){}
u8 BoxMenu_Open(PlayState* play,const BoxMenuEntry* entries,s32 count,s32 chosen,u16 hold,BoxMenuConfirmFn fn){++menus;return 1;}
void ExtInv_RefreshButtonIconsForItem(PlayState* play,u16 item){}
'''
player='mm/src/overlays/actors/ovl_player_actor/z_player.c'
lib='mm/src/code/z_player_lib.c'
ps=(ROOT/player).read_text();ls=(ROOT/lib).read_text()
parts=[native_decl(ps,'typedef enum ItemChangeType {'),native_decl(ps,'s8 sPlayerItemChangeTypes['),native_decl(ls,'PlayerModelIndices gPlayerModelTypes[')]
parts+=[body(player,'Player_ItemToItemAction'),body(lib,'Player_ActionToModelGroup')]
parts+=[body(player,n) for n in ['Player_InitItemAction','Player_UseItem','Player_FinishItemChange']]
parts+=[body(lib,n) for n in ['Player_GetItemOnButton','Player_Dpad_GetItemOnButton']]
parts+=[body(player,n) for n in ['func_8082FDC4','func_Dpad_8082FDC4','Player_ItemIsInUse','Player_ProcessItemButtons']]
parts+=[body('mm/mods/items/helpers/equip_helper.c',n) for n in ['ItemInput_IsBlockedEx','ItemInput_IsBlocked']]
parts+=[body('mm/mods/extended_inventory.c',n) for n in ['Seasons_HasRod','Seasons_GrantRod','Seasons_SeasonOwned','Seasons_GrantSeason','Seasons_SeasonCount','Seasons_SeasonAt','Seasons_GetSeason','Seasons_SetSeason']]
parts+=[(ROOT/'mm/mods/items/logic/item_rod_of_seasons.c').read_text().split('// Seasonal particles')[0]]
parts+=[body(player,'Player_ActionHandler_6')]
parts+=[body(player,'Player_UpdateInterface')]
# Keep MM's real choice of overridden/suppressed physical input and the final
# OnPassPlayerInputs policy hook. Animation/world scheduling remains a boundary.
input_start=ps.index('    if (play->actorCtx.isOverrideInputOn && (this == GET_PLAYER(play)))')
input_end=ps.index('    GameInteractor_ExecuteOnPassPlayerInputs(&input);',input_start)
input_end+=len('    GameInteractor_ExecuteOnPassPlayerInputs(&input);')
input_selection=re.sub(r'\bthis\b','p',ps[input_start:input_end])
tick_call=re.search(r'    Seasons_TickInput\([^;]+;',body(player,'Player_UpdateCommon')).group()
tick_call=re.sub(r'\bthis\b','p',tick_call)
tick_call=re.sub(r'\binput\b','sPlayerControlInput',tick_call)
parts+=['void CompleteRodFrame(Player* p,PlayState* play,u16 press){\n'
        '++play->gameplayFrames;play->state.input[0].press.button=play->state.input[0].cur.button=press;Input input;\n'
        +input_selection+'\nsPlayerControlInput=&input;\n'+tick_call+
        '\nPlayer_ProcessItemButtons(p,play);\n}']
checks=r'''
void ResetRod(Player* p,PlayState* play,ItemId old){
 ResetCaller(p,play,old,0);play->sceneId=++sceneNumber;gPlayState=play;
 play->actorCtx.actorLists[ACTORCAT_PLAYER].first=&p->actor;
 p->heldItemAction=p->itemAction=Player_ItemToItemAction(p,old);
 p->modelGroup=Player_ActionToModelGroup(p,p->heldItemAction);
 p->rightHandType=old==ITEM_NONE?PLAYER_MODELTYPE_RH_OPEN:PLAYER_MODELTYPE_RH_BOW;
 dpadEnabled=1;menuOpen=0;rodEquips=rodStows=menus=0;hookSuppressed=movementBlocked=0;
 memset(Nei_Save(),0,sizeof(*Nei_Save()));Seasons_GrantSeason(SEASON_WINTER);
 memset(&gSaveContext.save.shipSaveInfo.dpadEquips,0,sizeof(DpadSaveInfo));
 gSaveContext.save.shipSaveInfo.dpadEquips.extItems[0][EQUIP_SLOT_D_UP]=EXT_ITEM_ROD_OF_SEASONS;
}
void EquipRod(Player* p,PlayState* play,u16 button){
 ResetRod(p,play,ITEM_NONE);
 memset(gSaveContext.save.saveInfo.equips.buttonItems,ITEM_NONE,sizeof(gSaveContext.save.saveInfo.equips.buttonItems));
 memset(gSaveContext.save.shipSaveInfo.dpadEquips.dpadItems,ITEM_NONE,sizeof(gSaveContext.save.shipSaveInfo.dpadEquips.dpadItems));
 memset(gSaveContext.buttonStatus,BTN_ENABLED,sizeof(gSaveContext.buttonStatus));
 memset(gSaveContext.shipSaveContext.dpad.status,BTN_ENABLED,sizeof(gSaveContext.shipSaveContext.dpad.status));
 if(button==BTN_DUP){
  gSaveContext.save.shipSaveInfo.dpadEquips.dpadItems[0][EQUIP_SLOT_D_UP]=ITEM_EXT_BUTTON;
 }else{
  BUTTON_ITEM_EQUIP(0,EQUIP_SLOT_C_LEFT)=ITEM_EXT_BUTTON;
  EXT_BUTTON_ITEM(0,EQUIP_SLOT_C_LEFT)=EXT_ITEM_ROD_OF_SEASONS;
  gSaveContext.save.shipSaveInfo.dpadEquips.extItems[0][EQUIP_SLOT_D_UP]=ITEM_NONE;
 }
 BUTTON_ITEM_EQUIP(0,EQUIP_SLOT_C_DOWN)=ITEM_BOW;
}
void TickRod(Player* p,PlayState* play,u16 press){
 ++play->gameplayFrames;play->state.input[0].press.button=press;Seasons_TickInput(play,p,&play->state.input[0]);
}
void FinishNative(Player* p,PlayState* play){
 Player_StartChangingHeldItem(p,play);
 Player_FinishItemChange(play,p);
 assert(p->heldItemAction==PLAYER_IA_NONE&&p->itemAction==PLAYER_IA_NONE);
 // The animation owner releases the upper action only after the native item change completes.
 p->upperActionFunc=NULL;
}
int main(void){
 Player p;PlayState play;
 EquipRod(&p,&play,BTN_CLEFT);CompleteRodFrame(&p,&play,BTN_CLEFT);
 p.putAwayCooldownTimer=0;
 p.controlStickDirections[0]=PLAYER_STICK_DIR_NONE;
 Player_UpdateInterface(&play,&p);assert(play.interfaceCtx.aButtonDoAction==DO_ACTION_PUTAWAY);
 p.doorType=PLAYER_DOORTYPE_HANDLE;Player_UpdateInterface(&play,&p);
 assert(play.interfaceCtx.aButtonDoAction==DO_ACTION_OPEN&&Seasons_IsDrawn());
 p.doorType=PLAYER_DOORTYPE_NONE;p.putAwayCooldownTimer=0;
 BUTTON_ITEM_EQUIP(0,EQUIP_SLOT_B)=ITEM_SWORD_KOKIRI;
 Input putAwayInput={0};putAwayInput.cur.button=putAwayInput.press.button=BTN_A|BTN_B;
 Seasons_TickInput(&play,&p,&putAwayInput);
 sPlayerControlInput=&putAwayInput;
 Player_ActionHandler_6(&p,&play);
 Player_ProcessItemButtons(&p,&play);
 assert(!Seasons_IsDrawn()&&rodStows==1&&p.heldItemAction==PLAYER_IA_NONE&&p.heldItemId==ITEM_NONE);
 assert(!(putAwayInput.press.button&BTN_B));
 BUTTON_ITEM_EQUIP(0,EQUIP_SLOT_B)=ITEM_SWORD_KOKIRI;
 CompleteRodFrame(&p,&play,BTN_B);assert(p.heldItemId==ITEM_SWORD_KOKIRI);
 EquipRod(&p,&play,BTN_CLEFT);CompleteRodFrame(&p,&play,BTN_CLEFT);
 CompleteRodFrame(&p,&play,BTN_B);
 assert(Seasons_IsDrawn()&&rodStows==0&&p.heldItemAction==PLAYER_IA_NONE);
 EquipRod(&p,&play,BTN_CLEFT);CompleteRodFrame(&p,&play,BTN_CLEFT);
 BUTTON_ITEM_EQUIP(0,EQUIP_SLOT_C_LEFT)=ITEM_NONE;
 Input removedInput={0};removedInput.cur.button=removedInput.press.button=BTN_B;
 Seasons_TickInput(&play,&p,&removedInput);
 assert(!Seasons_IsDrawn()&&(removedInput.press.button&BTN_B));
 const u8 masks[]={PLAYER_MASK_BLAST,PLAYER_MASK_BREMEN,PLAYER_MASK_KAMARO};
 const u16 actions[]={DO_ACTION_EXPLODE,DO_ACTION_MARCH,DO_ACTION_DANCE};
 for(int mask=0;mask<3;++mask){
  EquipRod(&p,&play,BTN_CLEFT);CompleteRodFrame(&p,&play,BTN_CLEFT);
  p.currentMask=masks[mask];Player_UpdateInterface(&play,&p);
  assert(play.interfaceCtx.bButtonPlayerDoAction==actions[mask]);
  Input maskInput={0};maskInput.cur.button=maskInput.press.button=BTN_B;
  Seasons_TickInput(&play,&p,&maskInput);assert(maskInput.press.button&BTN_B);
 }
 for(int context=0;context<4;++context){
  EquipRod(&p,&play,BTN_CLEFT);CompleteRodFrame(&p,&play,BTN_CLEFT);
  if(context==0)play.msgCtx.msgMode=MSGMODE_TEXT_DISPLAYING;
  if(context==1)play.csCtx.state=1;
  if(context==2)p.doorType=PLAYER_DOORTYPE_HANDLE;
  if(context==3)p.stateFlags1=PLAYER_STATE1_FIRST_PERSON;
  Input contextInput={0};contextInput.cur.button=contextInput.press.button=BTN_B;
  Seasons_TickInput(&play,&p,&contextInput);assert(contextInput.press.button&BTN_B);
 }
 for(int dpad=0;dpad<2;dpad++){
  u16 button=dpad?BTN_DUP:BTN_CLEFT;
  EquipRod(&p,&play,button);
  CompleteRodFrame(&p,&play,button);
  assert(Seasons_IsDrawn()&&rodEquips==1&&rodStows==0&&menus==0);
  CompleteRodFrame(&p,&play,0);assert(Seasons_IsDrawn());
  CompleteRodFrame(&p,&play,button);
  assert(Seasons_IsDrawn()&&rodEquips==1&&rodStows==0&&menus==1);
  hookSuppressed=BTN_CDOWN;
  CompleteRodFrame(&p,&play,BTN_CDOWN);
  assert(Seasons_IsDrawn()&&p.heldItemId==ITEM_NONE&&rodStows==0);
  hookSuppressed=0;
  // The rod releases unrelated C presses to real native item dispatch.
  CompleteRodFrame(&p,&play,BTN_CDOWN);
  assert(!Seasons_IsDrawn()&&p.heldItemId==ITEM_BOW&&rodStows==1);
  // Native suppression and hooks determine usable input, even with a raw edge.
  EquipRod(&p,&play,button);movementBlocked=1;
  CompleteRodFrame(&p,&play,button);assert(!Seasons_IsDrawn()&&!rodEquips);
  movementBlocked=0;hookSuppressed=button;
  CompleteRodFrame(&p,&play,button);assert(!Seasons_IsDrawn()&&!rodEquips);
  hookSuppressed=0;play.actorCtx.isOverrideInputOn=1;
  memset(&play.actorCtx.overrideInput,0,sizeof(Input));
  CompleteRodFrame(&p,&play,button);assert(!Seasons_IsDrawn()&&!rodEquips);
  // The selected override may request the rod without a physical pad edge.
  play.actorCtx.overrideInput.cur.button=play.actorCtx.overrideInput.press.button=button;
  CompleteRodFrame(&p,&play,0);assert(Seasons_IsDrawn()&&rodEquips==1);
  EquipRod(&p,&play,button);
  if(dpad)gSaveContext.shipSaveContext.dpad.status[EQUIP_SLOT_D_UP]=BTN_DISABLED;
  else gSaveContext.buttonStatus[EQUIP_SLOT_C_LEFT]=BTN_DISABLED;
  CompleteRodFrame(&p,&play,button);assert(!Seasons_IsDrawn()&&!rodEquips);
  EquipRod(&p,&play,button);gSaveContext.save.saveInfo.playerData.health=0;
  CompleteRodFrame(&p,&play,button);assert(!Seasons_IsDrawn()&&!rodEquips);
 }
 puts("PASS complete native C/D rod dispatch: draw survives, next press opens HUD, replacement and input policy retained");
 ItemId items[]={ITEM_SWORD_KOKIRI,ITEM_BOW};
 for(int i=0;i<2;i++){
  ResetRod(&p,&play,items[i]);TickRod(&p,&play,BTN_DUP);
  assert(!Seasons_IsDrawn()&&!rodEquips);
  assert(p.stateFlags3&PLAYER_STATE3_START_CHANGING_HELD_ITEM);
  TickRod(&p,&play,0);assert(!Seasons_IsDrawn()); // pending request survives the native start flag
  Player_StartChangingHeldItem(&p,&play);TickRod(&p,&play,0);assert(!Seasons_IsDrawn());
  Player_FinishItemChange(&play,&p);assert(p.heldItemAction==PLAYER_IA_NONE);
  TickRod(&p,&play,0);assert(!Seasons_IsDrawn()); // final native model setup is still owned by the animation
  p.upperActionFunc=NULL;TickRod(&p,&play,0);
  assert(Seasons_IsDrawn()&&rodEquips==1&&p.rightHandType==PLAYER_MODELTYPE_RH_CLOSED);
  TickRod(&p,&play,0);assert(Seasons_IsDrawn()&&rodEquips==1);
  TickRod(&p,&play,BTN_DUP);assert(menus==1&&Seasons_IsDrawn());
  TickRod(&p,&play,BTN_B);assert(Seasons_IsDrawn()&&p.rightHandType==PLAYER_MODELTYPE_RH_CLOSED);
  assert(!(play.state.input[0].press.button&BTN_B));
 }
 // The copied native dispatcher also preserves an accepted delayed unequip.
 for(int dpad=0;dpad<2;dpad++){
  u16 button=dpad?BTN_DUP:BTN_CLEFT;
  EquipRod(&p,&play,button);
  p.heldItemId=ITEM_BOW;p.heldItemAction=p.itemAction=PLAYER_IA_BOW;
  p.modelGroup=Player_ActionToModelGroup(&p,p.heldItemAction);
  p.rightHandType=PLAYER_MODELTYPE_RH_BOW;
  CompleteRodFrame(&p,&play,button);
  assert(!Seasons_IsDrawn()&&!rodEquips&&(p.stateFlags3&PLAYER_STATE3_START_CHANGING_HELD_ITEM));
  CompleteRodFrame(&p,&play,0);assert(!Seasons_IsDrawn());
  FinishNative(&p,&play);CompleteRodFrame(&p,&play,0);
  assert(Seasons_IsDrawn()&&rodEquips==1&&p.rightHandType==PLAYER_MODELTYPE_RH_CLOSED);
  CompleteRodFrame(&p,&play,button);assert(Seasons_IsDrawn()&&menus==1);
 }
 // All cancellation paths must discard the delayed draw, even if native unequip later finishes.
 for(int cancel=0;cancel<8;cancel++){
  ResetRod(&p,&play,ITEM_BOW);TickRod(&p,&play,BTN_DUP);
  switch(cancel){
   case 0:p.stateFlags1|=PLAYER_STATE1_DAMAGED;break;
   case 1:menuOpen=1;break;
   case 2:p.transformation=PLAYER_FORM_DEKU;break;
   case 3:gSaveContext.save.shipSaveInfo.dpadEquips.extItems[0][EQUIP_SLOT_D_UP]=ITEM_NONE;break;
   case 4:play.pauseCtx.state=PAUSE_STATE_MAIN;break;
   case 5:play.csCtx.state=1;break;
   case 6:p.stateFlags1|=PLAYER_STATE1_SHIELDING;break;
  }
  TickRod(&p,&play,cancel==7?BTN_B:0);
  p.stateFlags1=0;p.transformation=PLAYER_FORM_HUMAN;menuOpen=0;play.pauseCtx.state=PAUSE_STATE_OFF;play.csCtx.state=CS_STATE_IDLE;
  gSaveContext.save.shipSaveInfo.dpadEquips.extItems[0][EQUIP_SLOT_D_UP]=EXT_ITEM_ROD_OF_SEASONS;
  FinishNative(&p,&play);TickRod(&p,&play,0);assert(!Seasons_IsDrawn()&&!rodEquips);
 }
 // A rejected native request must not arm a rod that appears later.
 ResetRod(&p,&play,ITEM_BOW);p.itemAction=PLAYER_IA_SWORD_KOKIRI;TickRod(&p,&play,BTN_DUP);
 assert(!Seasons_IsDrawn()&&p.heldItemId==ITEM_BOW&&!(p.stateFlags3&PLAYER_STATE3_START_CHANGING_HELD_ITEM));
 p.itemAction=p.heldItemAction=PLAYER_IA_NONE;TickRod(&p,&play,0);assert(!Seasons_IsDrawn()&&!rodEquips);
 // A script/native replacement, without a new button edge, also supersedes the pending rod.
 ResetRod(&p,&play,ITEM_BOW);TickRod(&p,&play,BTN_DUP);
 Player_UseItem(&play,&p,ITEM_SWORD_KOKIRI);TickRod(&p,&play,0);
 Player_StartChangingHeldItem(&p,&play);Player_FinishItemChange(&play,&p);p.upperActionFunc=NULL;
 TickRod(&p,&play,0);assert(!Seasons_IsDrawn()&&!rodEquips&&p.heldItemAction==PLAYER_IA_SWORD_KOKIRI);
 ResetRod(&p,&play,ITEM_NONE);TickRod(&p,&play,BTN_DUP);assert(Seasons_IsDrawn()&&rodEquips==1);
 // An empty starting Rod still draws and opens the wheel's Off entry.
 ResetRod(&p,&play,ITEM_NONE);Nei_Save()->seasonsOwned=0;Nei_Save()->season=SEASON_OFF;
 TickRod(&p,&play,BTN_DUP);assert(Seasons_IsDrawn()&&rodEquips==1);
 TickRod(&p,&play,BTN_DUP);assert(menus==1&&Seasons_GetSeason()==SEASON_OFF);
 puts("PASS rod native use/finish/init lifecycle, final hand capture, empty starting Rod and cancellation");
}
'''
with tempfile.TemporaryDirectory(prefix='rod-lifecycle-') as td:
 path=Path(td)/'test.c';path.write_text(prefix+'\n'.join(parts)+checks);binary=Path(td)/'test'
 # The real grant always uses the fixed Rod cell; Release-style inlining keeps
 # the unrelated Grace shared-slot implementation outside this rod fixture.
 sanitizer=['-fsanitize=address,undefined,bounds','-fno-sanitize-recover=all'] if '--sanitize' in sys.argv else []
 combined=['-DCOMBO_BUILD'] if '--combo' in sys.argv else []
 subprocess.run(['cc','-std=gnu17','-O2','-w','-Werror=implicit-function-declaration',*sanitizer,*combined,*flags(),'-ffunction-sections','-fdata-sections',str(path),'-Wl,--gc-sections','-lm','-o',str(binary)],check=True)
 environment={**os.environ,'ASAN_OPTIONS':os.environ.get('ASAN_OPTIONS','')+':detect_leaks=0'}
 subprocess.run([str(binary)],check=True,env=environment)
