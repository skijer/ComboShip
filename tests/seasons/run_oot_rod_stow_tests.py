"""Exercise the OoT Rod with native HUD, A stow and B/C item dispatch."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/nei_item_stow"))
from run_stow_tests import player_functions, extract
from run_ballchain_tests import flags

source = (ROOT / "soh/src/overlays/actors/ovl_player_actor/z_player.c").read_text()
native = extract.functions(source)
rod = (ROOT / "soh/mods/items/logic/item_rod_of_seasons.c").read_text().split('// Unity include,')[0]
fixture = (ROOT / "tests/nei_item_stow/stow_contract_test.c").read_text()
fixture = '#define REAL_SEASONS_STOW\n' + fixture
fixture = fixture.replace('static void Reset(void) {', 'static void Reset(void) {\n  sRodDrawn = 0;')
fixture = fixture.replace('#include "mods/items/custom_items_stow.c"', r'''
#include "mods/extended_inventory.h"
#include "mods/extended_equipment.h"
#include "overlays/actors/ovl_En_Horse/z_en_horse.h"
PlayState* gPlayState = &play;
u32 gUpgradeMasks[8]; u8 gUpgradeShifts[8];
static s16 sFloorType;
static f32 D_80854784[] = {120,240,360};
static int menus, menuOpen, rodSounds;
static int rodOnButton=1;
typedef struct {const char* iconPath;u8 iconSize;u8 enabled;} BoxMenuEntry;
u8 BoxMenu_IsOpen(void) {return menuOpen;}
void BoxMenu_Open(PlayState*,const BoxMenuEntry*,s32,s32,u16,void(*)(s32)) {++menus;}
void Seasons_UpdateGates(void) {}
u8 Seasons_HasRod(void) {return 1;}
u8 Seasons_SeasonCount(void) {return 1;}
u8 Seasons_GetSeason(void) {return SEASON_WINTER;}
void Seasons_SetSeason(u8 season) {}
u8 Seasons_SeasonOwned(u8 season) {return 1;}
void* Seasons_SeasonIcon(u8 season) {return NULL;}
u16 ExtButton_GetItem(s32 slot) {return slot==1&&rodOnButton?EXT_ITEM_ROD_OF_SEASONS:ITEM_NONE;}
void SeasonScene_UpdateWater(PlayState*) {}
void SeasonScene_UpdateWasteland(PlayState*) {}
void SeasonScene_SuppressVoidDamage(void) {}
void SeasonWeather_Update(PlayState*) {}
f32 gSfxDefaultFreqAndVolScale; s8 gSfxDefaultReverb;
void Audio_PlaySoundGeneral(u16 sound,Vec3f*,u8,f32*,f32*,s8*) {++rodSounds;}
''' + rod + '\n#include "mods/items/custom_items_stow.c"')
fixture = fixture.replace('assert(flag == VB_PLAYER_TOGGLE_NAVI);', '')
fixture = fixture.replace('item == ITEM_NONE ? PLAYER_IA_NONE : PLAYER_IA_SWORD_MASTER',
                          '(item == ITEM_NONE || item == ITEM_EXT_BUTTON) ? PLAYER_IA_NONE : PLAYER_IA_SWORD_MASTER')
boundaries = r'''
static s32 sZoraBoomerangActive,sDekuBubbleActive,sHeldItemButtonIsHeldDown;
static u16 sItemButtons[]={BTN_B,BTN_CLEFT,BTN_CDOWN,BTN_CRIGHT,BTN_DUP,BTN_DDOWN,BTN_DLEFT,BTN_DRIGHT};
s32 Player_ItemIsInUse(Player* p,s32 item) {return Player_ItemToItemAction(item)==p->heldItemAction;}
s32 func_8008F128(Player*) {return 0;}
void ExtEquip_ToggleFromCButton(u16 item) {}
u8 TransformMasks_HandleFormItemUse(PlayState*,Player*,s32) {return 0;}
s32 func_808332B8(Player*) {return 0;}
s32 func_8083A0D4(Player* p) {return p->interactRangeActor!=NULL&&p->heldActor==NULL;}
void Player_Action_8084E3C4(Player*,PlayState*) {}
void Player_Action_8084D3E4(Player*,PlayState*) {}
s32 Player_CanThrowCarriedActor(Player*,Actor*) {return 0;}
s32 Player_InBlockingCsMode(PlayState*,Player* p) {return p->csAction!=0;}
s32 Player_CheckHostileLockOn(Player*) {return 0;}
int Player_IsZTargeting(Player*) {return 0;}
s32 Player_FriendlyLockOnOrParallel(Player*) {return 0;}
s32 Player_IsChildWithHylianShield(Player*) {return 0;}
s32 func_8002DD78(Player*) {return 0;}
s32 MmMaskWear_CanUseBlastA(PlayState*,Player*) {return 0;}
u8 Message_GetState(MessageContext* ctx) {return ctx->msgMode==MSGMODE_NONE?TEXT_STATE_NONE:TEXT_STATE_DONE;}
void Interface_SetDoAction(PlayState* play,u16 action) {play->interfaceCtx.unk_1F0=action;}
void Interface_SetNaviCall(PlayState*,u16) {}
'''
parts = '\n'.join(native[name] for name in ('Player_ItemIsItemAction', 'Player_GetItemOnButton',
                                             'Player_ProcessItemButtons', 'Player_UpdateInterface'))
common = native['Player_UpdateCommon']
hook = re.search(r'\s+Seasons_FilterPlayerInput\([^;]+;', common)
filter_hook = re.sub(r'\binput\b', '&input', hook[0].replace('this', 'p')) if hook else ''
checks = r'''
static void RodReset(void) {
 Reset();sRodDrawn=0;menus=menuOpen=rodSounds=0;rodOnButton=1;
 player.actor.category=ACTORCAT_PLAYER;player.actor.bgCheckFlags=BGCHECKFLAG_GROUND;
 player.doorType=PLAYER_DOORTYPE_NONE;player.controlStickDirections[0]=PLAYER_STICK_DIR_NONE;
 gSaveContext.equips.buttonItems[0]=ITEM_SWORD_MASTER;
 gSaveContext.equips.buttonItems[1]=ITEM_EXT_BUTTON;
}
static Input RodFrame(u16 press) {
 ++play.gameplayFrames;
 play.state.input[0].cur.button=play.state.input[0].press.button=press;
 Input input=play.state.input[0];Player* p=&player;
 sControlInput=&input;
 /* FILTER_HOOK */
 Player_UpdateInterface(&play,p);
 // Native interaction handlers precede Roll. Only the free A path reaches it.
 if(play.interfaceCtx.unk_1F0==DO_ACTION_PUTAWAY)Player_ActionHandler_Roll(p,&play);
 Player_ProcessItemButtons(p,&play);
 Seasons_TickInput(&play,p);
 return input;
}
static void TestRod(void) {
 RodReset();RodFrame(BTN_CLEFT);assert(Seasons_IsDrawn());
 player.putAwayCooldownTimer=0;
 RodFrame(0);assert(play.interfaceCtx.unk_1F0==DO_ACTION_PUTAWAY);
 Input putAway=RodFrame(BTN_A|BTN_B);
 assert(!Seasons_IsDrawn()&&player.heldItemAction==PLAYER_IA_NONE&&!(putAway.press.button&BTN_B));
 RodFrame(0);assert(!Seasons_IsDrawn());
 RodFrame(BTN_CLEFT);assert(Seasons_IsDrawn());
 Input input=RodFrame(BTN_B);assert(Seasons_IsDrawn()&&!(input.press.button&BTN_B));
 RodFrame(BTN_CLEFT);assert(Seasons_IsDrawn()&&menus==1); // native NONE must not eat the chooser edge
 menuOpen=1;input=RodFrame(BTN_B);assert(input.press.button&BTN_B);menuOpen=0;
 player.doorType=PLAYER_DOORTYPE_HANDLE;input=RodFrame(BTN_A);
 assert(play.interfaceCtx.unk_1F0==DO_ACTION_OPEN&&Seasons_IsDrawn()&&(input.press.button&BTN_A));
 player.doorType=PLAYER_DOORTYPE_NONE;
 Actor npc={0};npc.category=ACTORCAT_NPC;
 player.talkActor=&npc;player.stateFlags2=PLAYER_STATE2_CAN_ACCEPT_TALK_OFFER;
 input=RodFrame(BTN_A);assert(play.interfaceCtx.unk_1F0==DO_ACTION_SPEAK&&Seasons_IsDrawn());
 player.stateFlags2=0;player.talkActor=NULL;
 for(int context=0;context<4;++context) {
  RodReset();RodFrame(BTN_CLEFT);
  if(context==0)play.msgCtx.msgMode=MSGMODE_TEXT_DISPLAYING;
  if(context==1)play.csCtx.state=1;
  if(context==2)player.doorType=PLAYER_DOORTYPE_HANDLE;
  if(context==3)player.stateFlags1=PLAYER_STATE1_FIRST_PERSON;
  input=RodFrame(BTN_B);assert(input.press.button&BTN_B);
 }
 RodReset();RodFrame(BTN_CLEFT);
 player.heldItemAction=player.itemAction=PLAYER_IA_SWORD_MASTER;
 input=RodFrame(BTN_B);assert(input.press.button&BTN_B); // native combat owns B
 RodReset();RodFrame(BTN_CLEFT);rodOnButton=0;
 input=RodFrame(BTN_B);assert(!Seasons_IsDrawn()&&(input.press.button&BTN_B));
 puts("PASS OoT Rod native A HUD/stow, higher-priority interactions, B ownership and repeated-C chooser");
}
'''.replace('/* FILTER_HOOK */', filter_hook.replace('play,', '&play,'))
fixture = fixture.replace('int main(void) {', boundaries + '\n' + parts + '\n' + checks + '\nint main(void) {\nTestRod();')
with tempfile.TemporaryDirectory(prefix='oot-rod-stow-') as directory:
    folder = Path(directory)
    (folder / 'stow_player.inc').write_text(player_functions())
    path = folder / 'test.c'; path.write_text(fixture)
    binary = folder / 'test'
    sanitize = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-pie', '-no-pie'] if '--sanitize' in sys.argv else []
    combined = ['-DCOMBO_BUILD'] if '--combo' in sys.argv else []
    subprocess.run([os.environ.get('CC', 'cc'), *flags(), *combined, '-O1', '-w', '-Werror=implicit-function-declaration',
                    '-I'+str(folder), '-I'+str(ROOT/'combo/menu'), '-ffunction-sections', '-fdata-sections',
                    *sanitize, str(path), '-Wl,--gc-sections', '-lm','-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS':'detect_leaks=0'})
