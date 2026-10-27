"""Execute MM climb entry and ordinary tool input/camera ownership with native structs."""
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_mm_nei_tests import flags
from run_time_pedestal_tests import functions

def body(path, name):
    return re.sub(r'\bthis\b', 'self', functions((ROOT / path).read_text())[name])

def run(name, prefix, parts, checks):
    if len(sys.argv) > 1 and sys.argv[1] != name:
        return
    with tempfile.TemporaryDirectory(prefix='mm-nei-action-') as td:
        source = Path(td) / (name + '.cpp')
        source.write_text('#include "mods/items/custom_items.h"\n#include "mods/items/helpers/equip_helper.h"\n#include "mods/extended_player.h"\n#include "functions.h"\n#include <cassert>\n#include <cstring>\n#include <iostream>\n' + prefix + '\n' + '\n'.join(parts) + '\nint main(){' + checks + '\nstd::cout<<"PASS ' + name + '\\n";}')
        binary = Path(td) / name
        subprocess.run(['c++', '-std=c++20', *flags(), '-ffunction-sections', '-fdata-sections', str(source), '-Wl,--gc-sections', '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)

player = 'mm/src/overlays/actors/ovl_player_actor/z_player.c'
mitts = 'mm/mods/items/logic/item_mitts.c'
equip = 'mm/mods/items/helpers/equip_helper.c'
run('mitts_climb_entry', r'''
#include "mods/items/logic/item_mitts.h"
CustomItemState gCustomItemState{};u8 gMogmaMittsClimbActive=0,gMogmaMittsForceGauntlets=0;
ItemEquipState sMittsEquipState{};s8 sMittsPrevInvinc=0;ItemInputState next{};
bool magic=true;int consumed=0,stows=0,climbs=0,sounds=0;
void ItemInput_Update(ItemInputState* out,u8,Player*,PlayState*){*out=next;}
u8 ItemInput_IsBlockedEx(Player*,PlayState*,u8){return 0;}
s32 ItemMagic_HasEnough(PlayState*,s16){return magic;}
void ItemMagic_Consume(PlayState*,s16){++consumed;}
void Audio_PlayActorSound2(Actor*,u16){++sounds;}
void ItemEquip_PlayEquipSFXForAction(PlayState*,Player*,s32){}
void ItemEquip_PlayUnequipSFXForAction(PlayState*,Player*,s32){}
void Player_Action_WaitForPutAway(Player*,PlayState*);
void func_80837C20(PlayState*,Player*){++climbs;}
void func_80837BF8(PlayState*,Player*){}
void OtherAction(PlayState*,Player*){}
s32 Player_SetAction(PlayState*,Player* p,PlayerActionFunc f,s32){p->actionFunc=f;return true;}
bool func_8083249C(Player*){return true;}
s32 PlayerAnimation_Update(PlayState*,SkelAnime*){return 0;}
s32 Player_UpdateUpperBody(Player*,PlayState*){return 0;}
s32 CustomItems_HasStowableHeldItem(Player*){return mmActive;}
void Player_UseItem(PlayState*,Player*,ItemId);
''', [body(equip, n) for n in ['ItemInput_CheckDamage', 'ItemEquip_Update']] +
    [body(mitts, n) for n in ['Mitts_Activate', 'Mitts_Deactivate', 'Mitts_OnEquip', 'Mitts_OnUnequip', 'Handle_MogmaMitts']] +
    ['void Player_UseItem(PlayState* play,Player* p,ItemId item){assert(item==ITEM_NONE);++stows;Mitts_OnUnequip(play,p);sMittsEquipState.isEquipped=0;p->heldItemAction=PLAYER_IA_NONE;}'] +
    [body(player, n) for n in ['Player_PutAwayHeldItem', 'Player_SetupWaitForPutAwayWithCs', 'Player_SetupWaitForPutAway', 'Player_Action_WaitForPutAway']], r'''
Player p{};PlayState play{};p.heldItemAction=PLAYER_IA_MOGMA_MITTS;next.wasEquipped=next.isPressed=1;
Handle_MogmaMitts(&p,&play);assert(mmActive&&gMogmaMittsClimbActive&&gMogmaMittsForceGauntlets);
next.isPressed=0;
assert(!Player_SetupWaitForPutAway(&play,&p,func_80837C20));
assert(!stows&&mmActive&&p.heldItemAction==PLAYER_IA_MOGMA_MITTS);
p.stateFlags1|=PLAYER_STATE1_CLIMBING_LADDER;Player_Action_WaitForPutAway(&p,&play);
assert(climbs==1&&mmActive);
for(int i=0;i<MITTS_DRAIN_INTERVAL;i++)Handle_MogmaMitts(&p,&play);
assert(consumed==1&&mmActive&&gMogmaMittsClimbActive);
magic=false;for(int i=0;i<MITTS_DRAIN_INTERVAL;i++)Handle_MogmaMitts(&p,&play);
assert(!mmActive&&!gMogmaMittsClimbActive&&!gMogmaMittsForceGauntlets);
next.isPressed=1;Handle_MogmaMitts(&p,&play);assert(!mmActive);
magic=true;Handle_MogmaMitts(&p,&play);assert(mmActive);
next.isPressed=0;
assert(Player_PutAwayHeldItem(&play,&p)&&stows==1&&!mmActive);
p.heldItemAction=PLAYER_IA_MOGMA_MITTS;next.isPressed=1;Handle_MogmaMitts(&p,&play);assert(mmActive);
assert(Player_SetupWaitForPutAwayWithCs(&play,&p,func_80837C20,1)&&stows==2&&!mmActive);
p.heldItemAction=PLAYER_IA_MOGMA_MITTS;Handle_MogmaMitts(&p,&play);assert(mmActive);
assert(Player_SetupWaitForPutAway(&play,&p,OtherAction)&&stows==3&&!mmActive);
p.heldItemAction=PLAYER_IA_MOGMA_MITTS;Handle_MogmaMitts(&p,&play);assert(mmActive);
next.isPressed=0;p.invincibilityTimer=1;Handle_MogmaMitts(&p,&play);
assert(!mmActive&&!gMogmaMittsClimbActive);
p.heldItemAction=PLAYER_IA_MOGMA_MITTS;
assert(Player_SetupWaitForPutAway(&play,&p,func_80837C20)&&stows==4); // inactive Mitts are ordinary held equipment
''')

leaf = 'mm/mods/items/logic/item_dekuleaf.c'
shovel = 'mm/mods/items/logic/item_shovel.c'
common = 'mm/mods/items/custom_items_common.c'
# This executes the production input selection expression, rather than duplicating its logic.
update = body(player, 'Player_Update')
start = update.index('if (play->actorCtx.isOverrideInputOn')
end = update.index('    GameInteractor_ExecuteOnPassPlayerInputs', start)
input_selection = update[start:end]
run('ordinary_tool_camera', r'''
#include "mods/items/logic/item_dekuleaf.h"
#include "mods/items/logic/item_shovel.h"
#include "mods/sound_translator/mm_sfx_ids.h"
#include "mods/transformation_masks/transformation_masks.h"
#define linearVelocity speedXZ
CustomItemState gCustomItemState{};u8 sDekuLeafBlowEffectFired=0,sDekuLeafColInitialized=1;
bool done=false,magic=true,asset=true,blocked=false,autoAnimate=false;ItemInputState next{};
s8 sDekuLeafPrevInvinc=0,sShovelPrevInvinc=0;
PlayerAnimationHeader anim{};int digs=0,spent=0;
#define NEI_ANIM_DEKULEAF_BLOW "leaf"
#define NEI_ANIM_DAMPE_DIG "dig"
s32 ItemMagic_HasEnough(PlayState*,s16){return magic;}
void ItemMagic_Consume(PlayState*,s16){++spent;}
LinkAnimationHeader* NeiAnim_Load(const char*){return asset?&anim:nullptr;}
void DekuLeaf_InitCollider(PlayState*,Player*){}
s32 PlayerAnimation_Once(PlayState*,SkelAnime*);
s32 StubAnimation(PlayState*,SkelAnime*){return done;}
void PlayerAnimation_AnimateFrame(PlayState*,SkelAnime*){}
void PlayerAnimation_PlayOnce(PlayState*,SkelAnime* s,PlayerAnimationHeader* anim){
 s->animation=anim;s->curFrame=0;s->endFrame=39;s->animLength=40;s->playSpeed=1;
 s->update.player=autoAnimate?PlayerAnimation_Once:StubAnimation;
}
void AudioSfx_StopById(u32){}
void MmSfx_Stop(u16){}
void DekuLeaf_PlayMmSfx(u16,Vec3f*){}
void ItemEquip_PlayUnequipSFXForAction(PlayState*,Player*,s32){}
void ItemEquip_PlayEquipSFXForAction(PlayState*,Player*,s32){}
void Player_PlaySfx(Player*,u16){}
void DekuLeaf_SpawnWindParticles(Player*,PlayState*){}
void DekuLeaf_BlowEffect(Player*,PlayState*){dlCollider.base.atFlags|=AT_ON;}
void PerformDig(Player*,PlayState*){++digs;}
void ItemInput_Update(ItemInputState* out,u8,Player*,PlayState*){*out=next;}
u8 ItemInput_IsBlocked(Player*,PlayState*){return blocked;}
s32 Movement_IsOnGround(Player*){return true;}
void DekuLeaf_UpdateGlide(Player*,PlayState*){}
u8 TransformMasks_IsTransformed(void){return false;}
MmPlayerTransformation MmPlayer_GetForm(void){return MM_PLAYER_FORM_HUMAN;}
bool func_8082DA90(PlayState*){return false;}
SaveContext gSaveContext{};
bool leafSlot=true,shovelSlot=true;
u8 ItemEquip_GetItemOnSlot(u8 slot){return slot==1&&leafSlot?ITEM_DEKU_LEAF:slot==2&&shovelSlot?ITEM_SHOVEL:ITEM_NONE;}
void DinFireSword_Reset(){}
void DinFireShield_Reset(){}
void HGrace_ResetTransient(){} // Fairy reset behavior is exercised by tests/mm_grace.
void WolfLinkHost_Destroy(PlayState*,Player*){}
void WolfLinkHost_RestorePlayerInput(Player*,Input*){}
void Effect_Destroy(PlayState*,s32){}
void LightContext_RemoveLight(PlayState*,LightContext*,LightNode*){}
s32 Collider_DestroyCylinder(PlayState*,ColliderCylinder*){return 1;}
s32 Collider_DestroyQuad(PlayState*,ColliderQuad*){return 1;}
void ZeldaArena_Free(void*){}
void Magic_Reset(PlayState*){}
void func_80831454(Player*){}
''', ([body(common, 'CustomItems_BlocksMovement')] if 'CustomItems_BlocksMovement' in (ROOT / common).read_text() else []) +
    [body('mm/src/code/z_skelanime.c', n) for n in ['PlayerAnimation_Once', 'PlayerAnimation_Update']] +
    [body(equip, 'ItemInput_CheckDamage')] +
    [body(leaf, n) for n in ['DekuLeaf_Stop', 'DekuLeaf_StartGlide', 'DekuLeaf_StartBlow', 'Player_UpperAction_DekuLeaf', 'Handle_DekuLeaf']] +
    [body(shovel, n) for n in ['Shovel_Stop', 'Shovel_Start', 'Shovel_UpdateAnimation', 'Player_UpperAction_Shovel', 'Handle_Shovel']] +
    [body(common, n) for n in ['CustomItems_IsBlocked', 'IsItemEquipped']] +
    [body('mm/mods/items/custom_items_stow.c', n) for n in ['CustomItems_CleanupTransientTools', 'CustomItems_ResetTransientTools']] +
    ['void InitToolLifetime(Actor* thisx,PlayState* play){' + body(player, 'Player_Init').split('{',1)[1].split('    play->playerInit =')[0] + '}'] +
    [body(player, 'Player_Destroy'), body('mm/src/code/z_camera.c', 'func_800CB854'),
     'Input SelectInput(Player* self,PlayState* play){Input input{};' + input_selection + 'return input;}'], r'''
Player p{};PlayState play{};play.actorCtx.actorLists[ACTORCAT_PLAYER].first=&p.actor;
Camera camera{};camera.play=&play;camera.focalActor=&p.actor;gSaveContext.save.saveInfo.playerData.health=16;
next.wasEquipped=1;p.actor.bgCheckFlags=BGCHECKFLAG_GROUND;
play.state.input[0].cur.button=BTN_CLEFT;play.state.input[0].press.button=BTN_CLEFT;play.state.input[0].cur.stick_x=40;
assert(SelectInput(&p,&play).cur.stick_x==40);
magic=false;DekuLeaf_StartBlow(&p,&play);assert(!dlActive);magic=true;asset=false;Shovel_Start(&p,&play);assert(!shActive);asset=true;
for(int tool=0;tool<2;tool++){
 done=false;p.heldItemAction=PLAYER_IA_NONE;p.upperActionFunc=nullptr;
 // CustomItems_Update starts tools BEFORE native item dispatch selects/installs the upper action.
 if(tool==0)DekuLeaf_StartBlow(&p,&play);else Shovel_Start(&p,&play);
 assert(SelectInput(&p,&play).press.button==BTN_CLEFT); // activation press must reach native equip
 p.heldItemAction=(PlayerItemAction)(tool==0?PLAYER_IA_DEKU_LEAF:PLAYER_IA_SHOVEL);
 p.upperActionFunc=tool==0?Player_UpperAction_DekuLeaf:Player_UpperAction_Shovel;

 if(tool==0){DekuLeaf_StartBlow(&p,&play);Player_UpperAction_DekuLeaf(&p,&play);p.skelAnimeUpper.curFrame=DEKULEAF_BLOW_EFFECT_FRAME;Player_UpperAction_DekuLeaf(&p,&play);}
 else{Shovel_Start(&p,&play);Shovel_UpdateAnimation(&p,&play);shAnimTimer=SHOVEL_DIG_FRAME-1;Player_UpperAction_Shovel(&p,&play);assert(digs==1);}
 assert(!func_800CB854(&camera)); // ordinary item use cannot request cutscene letterboxing
 assert(!SelectInput(&p,&play).cur.button&&!SelectInput(&p,&play).cur.stick_x);
 assert(p.actor.speed==0&&p.speedXZ==0);
 done=true;
 if(tool==0)Player_UpperAction_DekuLeaf(&p,&play);else Player_UpperAction_Shovel(&p,&play);
 assert(!dlActive&&!shActive&&SelectInput(&p,&play).cur.stick_x==40);
 // A real native suppression/cutscene flag must survive interrupted tool cleanup.
 done=false;p.stateFlags1|=PLAYER_STATE1_20|PLAYER_STATE1_20000000;
 blocked=true;
 if(tool==0){DekuLeaf_StartBlow(&p,&play);Handle_DekuLeaf(&p,&play);}
 else{Shovel_Start(&p,&play);Handle_Shovel(&p,&play);}
 assert(!dlActive&&!shActive);blocked=false;
 assert(func_800CB854(&camera)&&!SelectInput(&p,&play).cur.stick_x);
 assert(p.stateFlags1&PLAYER_STATE1_20000000);p.stateFlags1=0;
 assert(SelectInput(&p,&play).cur.stick_x==40);
 // Damage and loss of the equipped slot also release the item-owned input lock.
 for(int interruption=0;interruption<2;interruption++){
  next.wasEquipped=1;p.invincibilityTimer=0;
  if(tool==0)Handle_DekuLeaf(&p,&play);else Handle_Shovel(&p,&play);
  next.isPressed=1;
  if(tool==0)Handle_DekuLeaf(&p,&play);else Handle_Shovel(&p,&play);
  if(tool==0)Player_UpperAction_DekuLeaf(&p,&play);else Player_UpperAction_Shovel(&p,&play);
  assert(!SelectInput(&p,&play).cur.stick_x);next.isPressed=0;
  if(interruption==0)p.invincibilityTimer=1;else next.wasEquipped=0;
  if(tool==0)Handle_DekuLeaf(&p,&play);else Handle_Shovel(&p,&play);
  assert(!dlActive&&!shActive&&SelectInput(&p,&play).cur.stick_x==40);
 }
}
assert(spent==1);
// The reset path destroys the old Player while the process-global tool state remains allocated.
for(int tool=0;tool<2;tool++){
 done=false;
 if(tool==0)DekuLeaf_StartBlow(&p,&play);else Shovel_Start(&p,&play);
 Player_Destroy(&p.actor,&play);
 assert(!dlActive&&!dlBlowing&&!dlGliding&&!shActive&&!shAnimating);
 p=Player{}; // same address reused for the new Clock Tower entrance Player
 assert(SelectInput(&p,&play).cur.stick_x==40);
}
// Combo resume may skip destroy; execute the production prefix of Player_Init too.
DekuLeaf_StartGlide(&p,&play);Shovel_Start(&p,&play);
p=Player{};InitToolLifetime(&p.actor,&play);
assert(!dlActive&&!dlGliding&&!shActive&&!sDekuLeafColInitialized);
// Advance real PlayerAnimation_Once at the native 20 Hz rate, without manually forcing completion.
autoAnimate=true;play.state.framerateDivisor=2;
for(int tool=0;tool<2;tool++){
 int oldSpent=spent,oldDigs=digs;
 p.heldItemAction=(PlayerItemAction)(tool==0?PLAYER_IA_DEKU_LEAF:PLAYER_IA_SHOVEL);
 p.upperActionFunc=tool==0?Player_UpperAction_DekuLeaf:Player_UpperAction_Shovel;
 if(tool==0)DekuLeaf_StartBlow(&p,&play);else Shovel_Start(&p,&play);
 // Native Player_StartChangingHeldItem replaces the pending animation. The tool must
 // start its own animation at callback handoff, even when that equip Once has finished.
 PlayerAnimationHeader equipAnim{};p.skelAnimeUpper.animation=&equipAnim;
 p.skelAnimeUpper.curFrame=p.skelAnimeUpper.endFrame;
 for(int frame=0;frame<50&&(dlActive||shActive);frame++){
  play.gameplayFrames++;
  CustomItems_CleanupTransientTools(&p,&play);
  p.upperActionFunc(&p,&play);
  assert(!func_800CB854(&camera));
 }
 assert(!dlActive&&!shActive&&SelectInput(&p,&play).cur.stick_x==40);
 assert(spent-oldSpent==(tool==0)&&digs-oldDigs==(tool==1));
 // Scene/cutscene interruption, losing animation ownership, and slot removal clear busy state.
 for(int interruption=0;interruption<3;interruption++){
  p.heldItemAction=(PlayerItemAction)(tool==0?PLAYER_IA_DEKU_LEAF:PLAYER_IA_SHOVEL);
  p.upperActionFunc=tool==0?Player_UpperAction_DekuLeaf:Player_UpperAction_Shovel;
  if(tool==0)DekuLeaf_StartBlow(&p,&play);else Shovel_Start(&p,&play);
  p.upperActionFunc(&p,&play);assert(CustomItems_BlocksMovement(&p));
  Player remote{};CustomItems_ResetTransientTools(&remote,&play);
  assert(CustomItems_BlocksMovement(&p));
  if(interruption==0){p.csAction=PLAYER_CSACTION_5;p.stateFlags1|=PLAYER_STATE1_20;}
  else if(interruption==1){p.heldItemAction=PLAYER_IA_NONE;p.upperActionFunc=nullptr;}
  else{leafSlot=shovelSlot=false;}
  CustomItems_CleanupTransientTools(&p,&play);
  assert(!dlActive&&!shActive&&!CustomItems_BlocksMovement(&p));
  if(interruption==0)assert(p.stateFlags1&PLAYER_STATE1_20);
  p.csAction=PLAYER_CSACTION_NONE;p.stateFlags1=0;leafSlot=shovelSlot=true;
 }
}

''')
