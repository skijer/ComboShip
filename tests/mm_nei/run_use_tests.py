"""Native MM lifecycle regressions: execute production bodies with native data layouts."""
import re,sys,tempfile,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_mm_nei_tests import flags
from run_time_pedestal_tests import functions

SOURCE_REF=None
SANITIZE='--sanitize' in sys.argv
if SANITIZE:sys.argv.remove('--sanitize')
SANITIZER_FLAGS=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g'] if SANITIZE else []
if '--source-ref' in sys.argv:
 index=sys.argv.index('--source-ref');SOURCE_REF=sys.argv[index+1]
 del sys.argv[index:index+2]

def source(path):
 if SOURCE_REF:return subprocess.check_output(['git','show',SOURCE_REF+':'+path],cwd=ROOT,text=True)
 return (ROOT/path).read_text()
def body(path,name): return functions(source(path))[name]
def run(name,prefix,parts,checks):
 if len(sys.argv)>1 and sys.argv[1]!=name: return
 with tempfile.TemporaryDirectory(prefix='mm-nei-use-') as td:
  path=Path(td)/(name+'.cpp');path.write_text('#include "mods/items/custom_items.h"\n#include "mods/items/helpers/equip_helper.h"\n#include "mods/extended_player.h"\n#include <cassert>\n#include <iostream>\n'+prefix+'\n'+ '\n'.join(parts)+'\nint main(){'+checks+'\nstd::cout<<"PASS '+name+'\\n";}')
  binary=Path(td)/name
  subprocess.run(['c++','-std=c++20',*flags(),*SANITIZER_FLAGS,'-ffunction-sections','-fdata-sections',str(path),'-Wl,--gc-sections','-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True)

bc='mm/mods/items/logic/item_ballchain.c'
run('inactive_ballchain',r'''
#include "mods/items/logic/item_ballchain.h"
CustomItemState gCustomItemState{};
static u8 sBallChainColInitialized=1;static s8 sBallChainPrevInvinc=0;
ItemInputState next{};int stops=0,starts=0;
void ItemInput_Update(ItemInputState* out,u8,Player*,PlayState*){*out=next;}
u8 ItemInput_IsBlocked(Player*,PlayState*){return 0;}
u8 ItemInput_CheckDamage(Player*,s8*){return 0;}
u8 BallChain_ShouldInterrupt(Player*,PlayState*){return 0;}
void BallChain_InitCollider(PlayState*,Player*){}
void BallChain_Stop(Player* p,PlayState*){++stops;bcActive=0;p->skelAnime.playSpeed=1;}
void BallChain_Start(Player*,PlayState*){++starts;bcActive=1;}
void StateEquip(Player*,PlayState*,ItemInputState*){}
void StateSpinning(Player*,PlayState*,ItemInputState*){}
void StateThrown(Player*,PlayState*){}
''',[body(bc,'Handle_BallAndChain')],r'''
Player p{};PlayState play{};next.wasEquipped=1;next.otherButtonPressed=1;p.skelAnime.playSpeed=2;
Handle_BallAndChain(&p,&play);assert(stops==0 && p.skelAnime.playSpeed==2);
bcActive=1;Handle_BallAndChain(&p,&play);assert(stops==1&&!bcActive);
next.otherButtonPressed=0;next.isHeld=1;Handle_BallAndChain(&p,&play);assert(starts==1);
''')
player='mm/src/overlays/actors/ovl_player_actor/z_player.c'
run('native_stow_dispatch',r'''
bool held=true;int stows=0;
s32 CustomItems_HasStowableHeldItem(Player*){return held;}
void Player_UseItem(PlayState*,Player*,ItemId item){assert(item==ITEM_NONE);++stows;}
''',[re.sub(r'\bthis\b','self',body(player,'Player_PutAwayHeldItem'))],r'''
Player p{};PlayState play{};p.heldItemAction=PLAYER_IA_NONE;
assert(Player_PutAwayHeldItem(&play,&p)&&stows==1);
held=false;assert(!Player_PutAwayHeldItem(&play,&p));
p.heldItemAction=PLAYER_IA_SWORD_KOKIRI;assert(Player_PutAwayHeldItem(&play,&p)&&stows==2);
''')
whip='mm/mods/items/logic/item_whip.c'
w=functions(subprocess.check_output(['git','show','8013d116:'+whip],text=True) if '--baseline' in sys.argv else source(whip))
run('whip_raised_release',r'''
#include "mods/items/logic/item_whip.h"
#include "assets/objects/gameplay_keep/gameplay_keep.h"
CustomItemState gCustomItemState{};s32 sWhipAnimState=-1;int copies=0;
PlayerAnimationHeader* lastAnim=nullptr;
void PlayerAnimation_PlayLoop(PlayState*,SkelAnime*,PlayerAnimationHeader* a){lastAnim=a;}
void PlayerAnimation_PlayLoop(PlayState* p,SkelAnime* s,const void* a){PlayerAnimation_PlayLoop(p,s,(PlayerAnimationHeader*)a);}
void PlayerAnimation_PlayOnce(PlayState*,SkelAnime*,const void* a){lastAnim=(PlayerAnimationHeader*)a;}
s32 PlayerAnimation_Update(PlayState*,SkelAnime*){return 1;}
void ExtPlayer_CopyUpperBody(PlayState*,Player*){++copies;}
float gSfxDefaultFreqAndVolScale=1;s8 gSfxDefaultReverb=0;
u8 Player_IsZTargeting(Player*){return 0;}
bool func_80831010(Player* p,PlayState*){p->unk_AA5=PLAYER_UNKAA5_3;return true;}
s16 Math_Atan2S(f32 y,f32 x){return atan2(y,x)*32768/3.14159265;}
s16 FirstPerson_GetAimYaw(Player*){return 2345;}
s16 FirstPerson_GetAimPitch(Player*){return 1234;}
void Audio_PlaySoundGeneral(u16,Vec3f*,u8,f32*,f32*,s8*){}

''',[w[n] for n in ['Whip_ApplyLashAim','Whip_BeginLashPose','WhipStateEquip','Player_UpperAction_Whip'] if n in w],r'''
Player p{};PlayState play{};whipActive=1;whipExtendPitch=1234;
whipState=WHIP_STATE_EQUIP;whipFirstPerson=1;ItemInputState input{};input.isPressed=1;
WhipStateEquip(&p,&play,&input);
assert(!whipFirstPerson&&p.unk_AA5==PLAYER_UNKAA5_0&&!(p.stateFlags1&PLAYER_STATE1_8));
assert(whipState==WHIP_STATE_EXTENDING&&copies==1&&p.upperLimbRot.x==1234&&p.yaw==2345);

for(int state:{WHIP_STATE_EXTENDING,WHIP_STATE_HIT_ENEMY,WHIP_STATE_RETRACTING}){
whipState=state;sWhipAnimState=-1;assert(Player_UpperAction_Whip(&p,&play)==1);
assert(lastAnim==(PlayerAnimationHeader*)&gPlayerAnim_link_hook_wait);
assert(p.upperLimbRot.x==1234 && (p.unk_AA6_rotFlags&UNKAA6_ROT_UPPER_X));}
whipActive=0;assert(Player_UpperAction_Whip(&p,&play)==0);
''')
hook='mm/src/overlays/actors/ovl_Arms_Hook/z_arms_hook.c'
h=functions(source(hook))
run('instant_switchhook',r'''
#include "overlays/actors/ovl_Arms_Hook/z_arms_hook.h"
#include "mods/items/helpers/target_select_helper.h"
#include <cmath>
Actor* sSwitchSelection=nullptr;Actor* chosen=nullptr;int charges=5,fired=0,swaps=0,released=0;bool manual=false;u8 variant=4;
void ArmsHook_Shoot(ArmsHook*,PlayState*){}
void ArmsHook_SetupAction(ArmsHook* h,ArmsHookActionFunc fn){h->actionFunc=fn;}
u8 Nei_ArmsHookVariant(Player*){return variant;}
u8 SwitchHook_IsAimingManual(){return manual;}
s32 SwitchHook_ConsumeCharge(){if(!charges)return 0;--charges;return 1;}
void SwitchHook_OnFired(Player*){++fired;manual=false;}
Actor* ArmsHook_SelectSwapCandidate(PlayState*){return chosen;}
Actor* ArmsHook_SelectManualSwapCandidate(ArmsHook*,PlayState*){return chosen;}
void TargetSelect_Highlight(Actor*,s16){}
void AudioSfx_PlaySfx(u16){}
void Audio_PlaySfx(u16){}
void Actor_SetSpeeds(Actor* a,f32 v){a->speed=v;}
s16 Math_Atan2S(f32 y,f32 x){return atan2(y,x)*32768/3.14159265;}
void ArmsHook_StartSwap(ArmsHook*,PlayState*,Player* p,Actor* a){++swaps;Vec3f old=p->actor.world.pos;p->actor.world.pos=a->world.pos;a->world.pos=old;}
void ArmsHook_ReleaseAfterSwap(ArmsHook* h,Player* p){++released;h->actor.parent=&p->actor;p->heldActor=&h->actor;p->actor.child=&h->actor;h->timer=0;}
float CVarGetFloat(const char*,float d){return d;}
''',[re.sub(r'\bthis\b','self',h['ArmsHook_Wait'])],r'''
Player p{};PlayState play{};ArmsHook hook{};Actor target{};play.actorCtx.actorLists[ACTORCAT_PLAYER].first=&p.actor;
target.world.pos.z=100;target.focus.pos.z=100;chosen=&target;
ArmsHook_Wait(&hook,&play);assert(swaps==1&&charges==4&&fired==1&&p.actor.world.pos.z==100&&hook.timer==0&&hook.actor.speed==0);
hook.actor.parent=nullptr;chosen=nullptr;ArmsHook_Wait(&hook,&play);assert(charges==3&&released==1&&p.heldActor==&hook.actor);
hook.actor.parent=nullptr;charges=0;ArmsHook_Wait(&hook,&play);assert(fired==2&&p.heldActor==&hook.actor&&p.actor.child==&hook.actor);
hook.actor.parent=nullptr;charges=3;chosen=&target;target.world.pos.y=10000;ArmsHook_Wait(&hook,&play);assert(charges==3&&released==2);
hook.actor.parent=nullptr;target.world.pos.y=0;manual=true;ArmsHook_Wait(&hook,&play);assert(swaps==2&&!manual);
hook.actor.parent=nullptr;variant=0;ArmsHook_Wait(&hook,&play);assert(hook.timer==13&&hook.actor.speed==20);
''')
helper=source('mm/mods/items/helpers/equip_helper.c');hf=functions(helper)
state=helper[helper.index('typedef struct {\n    u8 changing;'):helper.index('static ItemUnequipSoundState*')]
run('sound_ownership',state+r'''
int sounds=0;
void ItemEquip_PlayEquipSFX(PlayState*,Player*){++sounds;}
void ItemEquip_PlayUnequipSFX(PlayState*,Player*){++sounds;}
''',[hf[n] for n in ['ItemEquip_UnequipSoundState','ItemEquip_ResetUnequipSound','ItemEquip_BeginItemChangeSound','ItemEquip_ClaimUnequipSound','ItemEquip_PlayEquipSFXForAction','ItemEquip_PlayUnequipSFXForAction']],r'''
Player p{},other{};PlayState play{},next{};play.gameplayFrames=12;
const int action=PLAYER_IA_WHIP,second=PLAYER_IA_ROD_FIRE;
ItemEquip_PlayEquipSFXForAction(&play,&p,action);sounds=0;
ItemEquip_PlayUnequipSFXForAction(&play,&p,action);assert(sounds==1);
assert(!ItemEquip_ClaimUnequipSound(&play,&p,action));
ItemEquip_PlayUnequipSFXForAction(&play,&p,action);assert(sounds==1);
ItemEquip_ResetUnequipSound(&play,&p,action);assert(ItemEquip_ClaimUnequipSound(&play,&p,action));
ItemEquip_PlayUnequipSFXForAction(&play,&p,action);assert(sounds==1);
assert(ItemEquip_ClaimUnequipSound(&play,&p,second));
ItemEquip_ResetUnequipSound(&play,&p,action);ItemEquip_PlayUnequipSFXForAction(&play,&p,action);
++play.gameplayFrames;ItemEquip_PlayUnequipSFXForAction(&play,&p,action);assert(sounds==3);
++play.gameplayFrames;assert(ItemEquip_ClaimUnequipSound(&play,&p,action));
assert(ItemEquip_ClaimUnequipSound(&play,&other,action));
assert(ItemEquip_ClaimUnequipSound(&next,&other,action));
play.gameplayFrames=30;assert(ItemEquip_ClaimUnequipSound(&play,&p,action));play.gameplayFrames=1;
assert(ItemEquip_ClaimUnequipSound(&play,&p,action));
''')
cache=helper[helper.index('typedef struct {'):helper.index('u8 ItemEquip_GetItemOnSlot')]
run('stowed_input_rearm',cache+r'''
u8 itemSlot[8]={ITEM_NONE,ITEM_WHIP,ITEM_NONE,ITEM_NONE,ITEM_NONE,ITEM_NONE,ITEM_NONE,ITEM_NONE};
u8 ItemEquip_GetItemOnSlot(u8 slot){return itemSlot[slot];}
u8 Pacci_UltrahandModeActive(){return 0;}
u8 Sw97_IsBowItem(u8){return 0;}u8 Sw97_IsSlingItem(u8){return 0;}u8 Sw97_EffectiveElement(u8){return 0;}
#define SW97_ELEM_BOMB 6
''',[hf[n] for n in ['EquipCache_Update','ItemInput_GetEquippedButton','ItemInput_SuppressUntilRelease','ItemInput_CheckOtherButtons','ItemInput_Update']],r'''
Player p{};PlayState play{};ItemInputState in{};play.gameplayFrames=1;
play.state.input[0].cur.button=play.state.input[0].press.button=BTN_CLEFT;
ItemInput_Update(&in,ITEM_WHIP,&p,&play);assert(in.isPressed&&in.isHeld);
ItemInput_SuppressUntilRelease(ITEM_WHIP,&play);ItemInput_Update(&in,ITEM_WHIP,&p,&play);assert(!in.isPressed&&!in.isHeld&&!in.isReleased);
++play.gameplayFrames;play.state.input[0].press.button=0;ItemInput_Update(&in,ITEM_WHIP,&p,&play);assert(!in.isHeld);
++play.gameplayFrames;play.state.input[0].cur.button=0;ItemInput_Update(&in,ITEM_WHIP,&p,&play);assert(in.isReleased);
++play.gameplayFrames;play.state.input[0].cur.button=play.state.input[0].press.button=BTN_CLEFT;
ItemInput_Update(&in,ITEM_WHIP,&p,&play);assert(in.isPressed&&in.isHeld);
ItemInput_SuppressUntilRelease(ITEM_WHIP,&play);play.gameplayFrames+=10;ItemInput_Update(&in,ITEM_WHIP,&p,&play);assert(in.isPressed);
ItemInput_SuppressUntilRelease(ITEM_WHIP,&play);PlayState nextPlay{};nextPlay.gameplayFrames=play.gameplayFrames;
nextPlay.state.input[0].cur.button=BTN_CLEFT;ItemInput_Update(&in,ITEM_WHIP,&p,&nextPlay);assert(in.isHeld);
ItemInput_SuppressUntilRelease(ITEM_WHIP,&nextPlay);nextPlay.gameplayFrames=0;ItemInput_Update(&in,ITEM_WHIP,&p,&nextPlay);assert(in.isHeld);
''')
finish=body(player,'Player_FinishItemChange')
if '--baseline' in sys.argv:
 finish=functions(subprocess.check_output(['git','show','8013d116:'+player],text=True))['Player_FinishItemChange']
run('native_sound_dispatch',state+r'''
int sounds=0;
void ItemEquip_PlayEquipSFX(PlayState*,Player*){++sounds;}
void ItemEquip_PlayUnequipSFX(PlayState*,Player*){++sounds;}
PlayerSword Player_SwordFromIA(Player*,s8){return PLAYER_SWORD_NONE;}
void func_8082E1F0(Player*,u16){++sounds;}
void Player_UseItem(PlayState* play,Player* p,u8){ItemEquip_PlayUnequipSFXForAction(play,p,PLAYER_IA_WHIP);p->heldItemAction=PLAYER_IA_NONE;}
''',[hf[n] for n in ['ItemEquip_UnequipSoundState','ItemEquip_ResetUnequipSound','ItemEquip_BeginItemChangeSound','ItemEquip_ClaimUnequipSound','ItemEquip_PlayEquipSFXForAction','ItemEquip_PlayUnequipSFXForAction']]+[re.sub(r'\bthis\b','self',finish)],r'''
Player p{};PlayState play{};p.transformation=PLAYER_FORM_HUMAN;p.heldItemAction=(PlayerItemAction)PLAYER_IA_WHIP;
ItemEquip_PlayUnequipSFXForAction(&play,&p,PLAYER_IA_WHIP);
Player_FinishItemChange(&play,&p);assert(sounds==1);
''')
lantern='mm/mods/items/logic/item_lantern.c'
run('lantern_stow_lifecycle',r'''
CustomItemState gCustomItemState{};ItemInputState next{};int swings=0;
#define LANTERN_FIRE_NONE 0
void ItemInput_Update(ItemInputState* out,u8,Player*,PlayState*){*out=next;}
u8 ItemInput_IsBlocked(Player*,PlayState*){return 0;}
void Lantern_UpdateSwing(Player*,PlayState*){}
void Player_StartLanternSwing(Player*,PlayState*){++swings;gCustomItemState.lanternEquipped=1;}
''',[re.sub(r'\bthis\b','self',body(lantern,n)) for n in ['Player_InitLanternIA','Lantern_PutAway','Handle_Lantern']],r'''
Player p{};PlayState play{};p.heldItemAction=PLAYER_IA_NONE;next.wasEquipped=1;
gCustomItemState.lanternFireType=1;Handle_Lantern(&p,&play);assert(gCustomItemState.lanternEquipped);
Lantern_PutAway(&p,&play);Handle_Lantern(&p,&play);assert(!gCustomItemState.lanternEquipped&&gCustomItemState.lanternFireType==1);
next.isPressed=1;Handle_Lantern(&p,&play);assert(swings==1&&!gCustomItemState.lanternStowed);
next.isPressed=0;p.heldItemAction=PLAYER_IA_SWORD_KOKIRI;Handle_Lantern(&p,&play);assert(!gCustomItemState.lanternEquipped);
p.heldItemAction=PLAYER_IA_NONE;Handle_Lantern(&p,&play);assert(gCustomItemState.lanternEquipped);
''')
run('native_item_change_input', 'int nativeRequests=0; void Player_UseItem(PlayState*,Player*,ItemId item){assert(item==ITEM_NONE);++nativeRequests;}', [hf[n] for n in ['ItemInput_IsBlockedEx','ItemInput_RequestItemChange']],r'''
Player p{};PlayState play{};p.heldItemAction=(PlayerItemAction)PLAYER_IA_WHIP;
assert(!ItemInput_IsBlockedEx(&p,&play,0));p.stateFlags3|=PLAYER_STATE3_START_CHANGING_HELD_ITEM;
assert(ItemInput_IsBlockedEx(&p,&play,1));p.stateFlags3=0;ItemInput_RequestItemChange(&p,&play);
assert(nativeRequests==1);
''')
h=functions(source(hook))
run('manual_hook_live_targets',r'''
#include "overlays/actors/ovl_Arms_Hook/z_arms_hook.h"
#include "mods/items/helpers/target_select_helper.h"
#include <cmath>
const u8 gTargetSelectDefaultCats[4]={ACTORCAT_ENEMY,ACTORCAT_PROP,ACTORCAT_CHEST,ACTORCAT_NPC};
s32 ArmsHook_IsSwappable(Actor* a){return a->update!=nullptr;}
f32 Math_SinS(s16 v){return sin(v*3.14159265/32768);}
f32 Math_CosS(s16 v){return cos(v*3.14159265/32768);}
f32 CVarGetFloat(const char*,f32 d){return d;}
void alive(Actor*,PlayState*){}
''',[re.sub(r'\bthis\b','self',h[n]) for n in ['ArmsHook_IsLiveSwapTarget','ArmsHook_SelectManualSwapCandidate']],r'''
PlayState play{};ArmsHook hook{};Actor front{},near{},behind{},above{};
front.update=near.update=behind.update=above.update=alive;
front.world.pos.z=200;near.world.pos.z=100;behind.world.pos.z=-50;above.world.pos.y=100;
play.actorCtx.actorLists[gTargetSelectDefaultCats[0]].first=&front;front.next=&near;near.next=&behind;behind.next=&above;
assert(ArmsHook_SelectManualSwapCandidate(&hook,&play)==&near);
near.update=nullptr;assert(ArmsHook_SelectManualSwapCandidate(&hook,&play)==&front);
hook.actor.world.rot.x=-0x4000;assert(ArmsHook_SelectManualSwapCandidate(&hook,&play)==&above);
assert(ArmsHook_IsLiveSwapTarget(&play,&front));assert(!ArmsHook_IsLiveSwapTarget(&play,(Actor*)1));
play.actorCtx.actorLists[gTargetSelectDefaultCats[0]].first=nullptr;
assert(!ArmsHook_IsLiveSwapTarget(&play,&front));assert(!ArmsHook_SelectManualSwapCandidate(&hook,&play));
''')
leaf='mm/mods/items/logic/item_dekuleaf.c'
run('native_leaf_lifecycle',r'''
#include "mods/items/logic/item_dekuleaf.h"
#include "mods/items/logic/item_shovel.h"
s32 Player_UpperAction_Shovel(Player*,PlayState*){return 0;}
#include "mods/sound_translator/mm_sfx_ids.h"
#define linearVelocity speedXZ
CustomItemState gCustomItemState{};u8 sDekuLeafBlowEffectFired=0;bool magic=true,asset=true,done=false;
int consumes=0,sounds=0,closes=0,plays=0,wind=0;PlayerAnimationHeader anim{};
#define NEI_ANIM_DEKULEAF_BLOW "leaf"
s32 ItemMagic_HasEnough(PlayState*,s16){return magic;}
void ItemMagic_Consume(PlayState*,s16){++consumes;}
LinkAnimationHeader* NeiAnim_Load(const char*){return asset?&anim:nullptr;}
void DekuLeaf_InitCollider(PlayState*,Player*){}
void PlayerAnimation_PlayOnce(PlayState*,SkelAnime* s,PlayerAnimationHeader*){++plays;s->curFrame=0;}
s32 PlayerAnimation_Update(PlayState*,SkelAnime*){return done;}
void AudioSfx_StopById(u32){}
void MmSfx_Stop(u16){}
void DekuLeaf_PlayMmSfx(u16 id,Vec3f*){if(id==MM_NA_SE_IT_DEKUNUTS_FLOWER_CLOSE)++closes;}
void ItemEquip_PlayUnequipSFXForAction(PlayState*,Player*,s32){++sounds;}
void ItemEquip_PlayEquipSFXForAction(PlayState*,Player*,s32){++sounds;}
void Player_PlaySfx(Player*,u16){}
void DekuLeaf_SpawnWindParticles(Player*,PlayState*){++wind;}
void DekuLeaf_BlowEffect(Player*,PlayState*){dlCollider.base.atFlags|=AT_ON;}
''',[body('mm/mods/items/custom_items_common.c','CustomItems_BlocksMovement')]+[body(leaf,n) for n in ['DekuLeaf_Stop','DekuLeaf_StartGlide','DekuLeaf_StartBlow','Player_UpperAction_DekuLeaf']],r'''
Player p{};PlayState play{};p.heldItemAction=(PlayerItemAction)PLAYER_IA_DEKU_LEAF;p.upperActionFunc=Player_UpperAction_DekuLeaf;magic=false;DekuLeaf_StartBlow(&p,&play);assert(!dlActive&&!sounds&&!plays);
magic=true;asset=false;DekuLeaf_StartBlow(&p,&play);assert(!dlActive&&!sounds&&!plays);
asset=true;DekuLeaf_StartBlow(&p,&play);assert(dlActive&&dlBlowing&&plays==0);
Player_UpperAction_DekuLeaf(&p,&play);assert(plays==1&&p.skelAnimeUpper.playSpeed==2);
p.skelAnimeUpper.curFrame=DEKULEAF_BLOW_EFFECT_FRAME;Player_UpperAction_DekuLeaf(&p,&play);Player_UpperAction_DekuLeaf(&p,&play);
assert(consumes==1&&CustomItems_BlocksMovement(&p)&&!(p.stateFlags1&PLAYER_STATE1_INPUT_DISABLED)&&(dlCollider.base.atFlags&AT_ON));
done=true;assert(!Player_UpperAction_DekuLeaf(&p,&play));assert(!dlActive&&!dlBlowing&&!(dlCollider.base.atFlags&(AT_ON|AT_HIT))&&!CustomItems_BlocksMovement(&p)&&closes==0);
int prior=sounds;DekuLeaf_Stop(&p,&play);assert(sounds==prior);
done=false;DekuLeaf_StartBlow(&p,&play);DekuLeaf_Stop(&p,&play);assert(!dlActive);DekuLeaf_StartGlide(&p,&play);DekuLeaf_Stop(&p,&play);assert(closes==1);
''')
for element in ['Fire','Ice','Light']:
 low=element.lower();rp='mm/mods/items/logic/item_rod_'+low+'.c';rf=functions(source(rp));stem='s' if element=='Fire' else 's'+element
 equip='sEquipState' if element=='Fire' else 's'+element+'EquipState'
 prefix=f'''\n#include "mods/items/logic/item_rod_{low}.h"\n#include "mods/items/logic/item_rod_common.h"\nCustomItemState gCustomItemState{{}};
 ItemEquipState {equip}{{}};u8 {stem}LastSwingType=0,{stem}ChargeButtonHeld=0;s16 {stem}ChargeHoldCounter=0;
 RodProjSet s{element}ProjSets[ROD_MAX_PROJ_SETS];int sounds=0,kills=0,exits=0;
 void {element}Rod_ExitFirstPerson(Player*,PlayState*){{++exits;{low}RodFirstPerson=0;}}
 void {element}Rod_StopSpin{element}(){{{low}RodSpinActive=0;}}
 void AudioSfx_StopById(u32){{}}
 void FX_KillSwordTrail(PlayState*,s32){{++kills;}}
 void ItemEquip_PlayUnequipSFXForAction(PlayState*,Player*,s32){{++sounds;}}
 void {element}Rod_DestroySetColliders(RodProjSet*,PlayState*){{}}
 '''
 prefix+='\n'.join(re.findall(r'^#define \w+ gCustomItemState\.\w+.*$',source(rp),re.M))+'\n'
 run(low+'_rod_stow',prefix,[rf[element+'Rod_OnUnequip'],rf[element+'Rod_PutAway']],f'''
 Player p{{}};PlayState play{{}};{low}RodActive={low}RodFirstPerson={low}RodCharging={low}RodSpinActive=1;
 {low}RodBlureIdx=3;{equip}.isEquipped=1;for(auto& set:s{element}ProjSets)set.active=1;
 {element}Rod_PutAway(&p,&play);assert(!{low}RodActive&&!{low}RodFirstPerson&&!{low}RodCharging&&!{low}RodSpinActive&&!{equip}.isEquipped);
 assert({stem}LastSwingType==0xFF&&kills==1&&exits==1&&sounds==1);for(auto& set:s{element}ProjSets)assert(!set.active);
 {element}Rod_PutAway(&p,&play);assert(kills==1&&exits==1&&sounds==1);
 ''')
run('native_stow_ownership',r'''
#include "mods/items/logic/item_whip.h"
#include "mods/items/logic/item_rod_fire.h"
#include "mods/items/logic/item_rod_ice.h"
#include "mods/items/logic/item_rod_light.h"
CustomItemState gCustomItemState{};ItemEquipState sMittsEquipState{};int stops=0,suppress=0;
u8 seasonsDrawn=0;
u8 Seasons_IsDrawn(){return seasonsDrawn;}
void Seasons_Stow(PlayState*,Player*){seasonsDrawn=0;}
void Lantern_PutAway(Player*,PlayState*){gCustomItemState.lanternEquipped=gCustomItemState.lanternSwinging=0;}
void FireRod_PutAway(Player*,PlayState*){fireRodActive=fireRodFirstPerson=0;}
void IceRod_PutAway(Player*,PlayState*){iceRodActive=iceRodFirstPerson=0;}
void LightRod_PutAway(Player*,PlayState*){lightRodActive=lightRodFirstPerson=0;}
void GustJar_Unequip(PlayState*,Player*){gCustomItemState.gustJarEquipped=0;}
void Mitts_OnUnequip(PlayState*,Player*){gCustomItemState.mogmaMittsActive=0;}
void BallChain_Stop(Player*,PlayState*){++stops;gCustomItemState.ballAndChainThrown=0;}
void Whip_Stop(Player*,PlayState*){++stops;whipActive=0;}
void ItemInput_SuppressUntilRelease(u8,PlayState*){++suppress;}
void DekuLeaf_Stop(Player*,PlayState*){gCustomItemState.dekuLeafActive=0;}
void Shovel_Stop(Player*,PlayState*){gCustomItemState.shovelActive=0;}
''',[body('mm/mods/items/custom_items_stow.c',n) for n in ['CustomItems_CanStowWhip','CustomItems_HasStowableHeldItem','CustomItems_PutAwayHeldItems']],r'''
Player p{},remote{};PlayState play{};play.actorCtx.actorLists[ACTORCAT_PLAYER].first=&p.actor;
gCustomItemState.lanternEquipped=fireRodActive=iceRodFirstPerson=lightRodActive=seasonsDrawn=1;
CustomItems_PutAwayHeldItems(&remote,&play);assert(gCustomItemState.lanternEquipped&&fireRodActive);
gCustomItemState.ballAndChainThrown=whipActive=1;whipState=WHIP_STATE_EQUIP;
assert(CustomItems_HasStowableHeldItem(&p));CustomItems_PutAwayHeldItems(&p,&play);
assert(!CustomItems_HasStowableHeldItem(&p)&&stops==2&&suppress==2);
CustomItems_PutAwayHeldItems(&p,&play);assert(stops==2&&suppress==2);
whipActive=1;whipState=WHIP_STATE_SWINGING;CustomItems_PutAwayHeldItems(&p,&play);assert(whipActive);
whipState=WHIP_STATE_LAUNCHED;CustomItems_PutAwayHeldItems(&p,&play);assert(whipActive);
CustomItems_PutAwayHeldItems(nullptr,&play);assert(!CustomItems_HasStowableHeldItem(nullptr));
''')
# Integration checks complement the executable native lifecycle cases.
ps=source(player)
for name in ['Player_PutAwayHeldItem','Player_InitItemAction','Player_UseItem']:
 assert 'CustomItems_' in functions(ps)[name],name
for name in ['Whip_Start','WhipStateEquip']:
 assert 'Whip_BeginLashPose(p, play)' in functions(source(whip))[name],name
assert '#include "../custom_items_stow.c"' in source('mm/mods/items/logic/custom_items.c')
assert 'COLSHAPE_SPHERE' in source('mm/mods/items/logic/item_switchhook.c')
print('PASS native lifecycle production integration')

# Real native caller -> item-button dispatch -> accepted use/transition pipeline.
# C preserves native enum and asset-pointer ABI; unrelated engine services are boundaries.
caller_prefix=r'''
#include "mods/items/custom_items.h"
#include "mods/items/helpers/equip_helper.h"
#include "mods/extended_player.h"
#include "mods/extended_equipment.h"
#include "mods/items/logic/item_spinner.h"
#include "variables.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "overlays/actors/ovl_En_Bom/z_en_bom.h"
#include <assert.h>
#include <stdio.h>
SaveContext gSaveContext;
CustomItemState gCustomItemState;
RegEditor editor;RegEditor* gRegEditor=&editor;
static ItemEquipState sNetEquipState;
static u8 sNetActive;
static Input* sPlayerControlInput;
static s32 sPlayerUseHeldItem,sPlayerHeldItemButtonIsHeldDown;
static u16 sPlayerItemButtons[]={BTN_B,BTN_CLEFT,BTN_CDOWN,BTN_CRIGHT};
static u16 sDpadItemButtons[]={BTN_DRIGHT,BTN_DLEFT,BTN_DDOWN,BTN_DUP};
static ItemInputState netInput;
static int transitions,initializations;static u8 animationDestination;
static int equipmentToggles;static u16 toggledEquipment;
void ExtEquip_ToggleFromCButton(u16 item){++equipmentToggles;toggledEquipment=item;}
static int dpadEnabled;
f32 gSfxDefaultFreqAndVolScale=1;s8 gSfxDefaultReverb=0;
u8 gItemSlots[77];
PlayerAgeProperties sPlayerAgeProperties[PLAYER_FORM_MAX];
typedef struct {u8 itemId;s16 actorId;} ExplosiveInfo;
ExplosiveInfo sPlayerExplosiveInfo[PLAYER_EXPLOSIVE_MAX];
void Player_UseItem(PlayState*,Player*,ItemId);
void ItemInput_Update(ItemInputState* out,u8 item,Player* p,PlayState* play){*out=netInput;}
u8 ItemInput_CheckDamage(Player* p,s8* previous){return 0;}
u16 ItemInput_GetEquippedButton(u8 item,PlayState* play){return BTN_CLEFT;}
s32 CVarGetInteger(const char* name,s32 fallback){return dpadEnabled;}
bool GameInteractor_Should(GIVanillaBehavior flag,uint32_t result,...){return result;}
bool func_801240DC(Player* p){return 0;}
s32 func_8082DA90(PlayState* play){return 0;}
s32 func_8082FD0C(Player* p,PlayerItemAction action){return EQUIP_SLOT_C_LEFT;}
DpadEquipSlot func_Dpad_8082FD0C(Player* p,PlayerItemAction action){return EQUIP_SLOT_D_NONE;}
u8 Player_MaskIdToItemId(s32 mask){return ITEM_NONE;}
void func_80838A20(PlayState* play,Player* p){}
s32 Inventory_GetBtnBItem(PlayState* play){return ITEM_SWORD_KOKIRI;}
ItemId Player_GetItemOnButton(PlayState* play,Player* p,EquipSlot slot){
 if(slot==EQUIP_SLOT_B)return ITEM_SWORD_KOKIRI;
 if(slot==EQUIP_SLOT_C_DOWN)return ITEM_BOW;
 if(slot==EQUIP_SLOT_C_LEFT)return ITEM_NET;
 return ITEM_NONE;
}
ItemId Player_Dpad_GetItemOnButton(PlayState* play,Player* p,DpadEquipSlot slot){return slot==EQUIP_SLOT_D_DOWN?ITEM_BOW:ITEM_NONE;}
s8 ExtPlayer_GetItemAction(s32 item){
 if(item==ITEM_NET||item==ITEM_SWORD_KOKIRI)return PLAYER_IA_SWORD_KOKIRI;
 if(item==ITEM_BOW)return PLAYER_IA_BOW;
 return PLAYER_IA_NONE;
}
u8 ExtPlayer_GetActionModelGroup(s32 action){
 if(action==PLAYER_IA_SWORD_KOKIRI)return PLAYER_MODELGROUP_ONE_HAND_SWORD;
 if(action==PLAYER_IA_BOW)return PLAYER_MODELGROUP_BOW;
 return PLAYER_MODELGROUP_DEFAULT;
}
PlayerBButtonSword Player_GetHeldBButtonSword(Player* p){return PLAYER_B_SWORD_NONE;}
PlayerBButtonSword Player_BButtonSwordFromIA(Player* p,PlayerItemAction a){return PLAYER_B_SWORD_NONE;}
PlayerMeleeWeapon Player_MeleeWeaponFromIA(PlayerItemAction a){return a==PLAYER_IA_SWORD_KOKIRI?PLAYER_MELEEWEAPON_SWORD_KOKIRI:PLAYER_MELEEWEAPON_NONE;}
PlayerExplosive Player_ExplosiveFromIA(Player* p,PlayerItemAction a){return PLAYER_EXPLOSIVE_NONE;}
void func_80839978(PlayState* play,Player* p){}
void func_80839A10(PlayState* play,Player* p){}
s32 RocsFeatherVanilla_TryUse(PlayState* play,Player* p,s32 item){return 0;}
u8 CustomForms_UseItem(Player* p,ItemId item){return 0;}
void WolfLinkHost_OnUseItem(PlayState* play,Player* p,s32 item){}
void WolfLinkHost_RestorePlayerInput(Player* p,Input* input){}
void CustomItems_PutAwayHeldItems(Player* p,PlayState* play){}
void ItemEquip_BeginItemChangeSound(PlayState* play,Player* p,s32 action){}
s32 OotSpells_TryUseItem(PlayState* play,Player* p,PlayerItemAction a,ItemId item){return 0;}
void func_808318C0(PlayState* play){}
s32 func_80831814(Player* p,PlayState* play,s32 v){return 0;}
void func_8083A658(PlayState* play,Player* p){}
void func_8082E1F0(Player* p,u16 id){}
void Player_DestroyHookshot(Player* p){}
void Player_DetachHeldActor(PlayState* play,Player* p){}
void Player_InitItemActionWithAnim(PlayState* play,Player* p,PlayerItemAction action){++initializations;p->heldItemAction=p->itemAction=action;p->modelGroup=p->nextModelGroup;}
void Player_StartChangingHeldItem(Player* p,PlayState* play){++transitions;animationDestination=p->nextModelGroup;p->stateFlags3&=~PLAYER_STATE3_START_CHANGING_HELD_ITEM;}
void Audio_PlaySoundGeneral(u16 id,Vec3f* pos,u8 token,f32* freq,f32* vol,s8* reverb){}
void Audio_PlaySfx(u16 id){}
void lusprintf(const char* file,int line,int priority,const char* fmt,...){}
bool Player_IsGoronOrDeku(Player* p){return p->transformation==PLAYER_FORM_GORON||p->transformation==PLAYER_FORM_DEKU;}
s32 BgCheck_EntityCheckCeiling(CollisionContext* ctx,f32* y,Vec3f* pos,f32 height,CollisionPoly** poly,s32* bg,Actor* actor){return 0;}
Actor* Actor_Spawn(ActorContext* ctx,PlayState* play,s16 id,f32 x,f32 y,f32 z,s16 rx,s16 ry,s16 rz,s32 params){return NULL;}
u8 Message_GetState(MessageContext* ctx){return TEXT_STATE_NONE;}
void Message_StartTextbox(PlayState* play,u16 id,Actor* a){}
void ResetCaller(Player* p,PlayState* play,u8 held,u16 press){
 memset(p,0,sizeof(*p));memset(play,0,sizeof(*play));
 p->actor.id=ACTOR_PLAYER;p->transformation=PLAYER_FORM_HUMAN;
 p->heldItemId=held;p->itemAction=p->heldItemAction=PLAYER_IA_SWORD_KOKIRI;
 p->modelGroup=PLAYER_MODELGROUP_ONE_HAND_SWORD;p->nextModelGroup=PLAYER_MODELGROUP_BOW;
 p->csAction=PLAYER_CSACTION_NONE;play->activeCamId=CAM_ID_MAIN;play->csCtx.state=CS_STATE_IDLE;
 play->state.input[0].press.button=press;sPlayerControlInput=&play->state.input[0];
 gSaveContext.save.saveInfo.playerData.health=0x30;gSaveContext.timerStates[TIMER_ID_MINIGAME_2]=TIMER_STATE_OFF;
 gSaveContext.save.saveInfo.equips.buttonItems[0][EQUIP_SLOT_C_LEFT]=ITEM_NET;
 netInput=(ItemInputState){.wasEquipped=1,.otherButtonPressed=1,.equippedButton=BTN_CLEFT};
 sNetActive=1;sNetEquipState=(ItemEquipState){.isEquipped=1};
 transitions=initializations=dpadEnabled=equipmentToggles=0;toggledEquipment=ITEM_NONE;
 sPlayerUseHeldItem=sPlayerHeldItemButtonIsHeldDown=0;
}
'''
def native_decl(text,start):
 begin=text.index(start);end=begin+re.search(r'\n}[^;\n]*;',text[begin:]).end()
 return text[begin:end]
caller_player=source(player)
caller_lib=source('mm/src/code/z_player_lib.c')
caller_parts=[native_decl(caller_player,'typedef enum ItemChangeType {'),
 native_decl(caller_player,'s8 sPlayerItemChangeTypes['),native_decl(caller_lib,'PlayerModelIndices gPlayerModelTypes[')]
caller_parts += [body(player,n) for n in ['Player_ItemToItemAction','Player_ItemIsInUse','func_8082FDC4','func_Dpad_8082FDC4']]
start=caller_lib.index('PlayerModelGroup Player_ActionToModelGroup(')
caller_parts += [caller_lib[start:caller_lib.index('\n}',start)+2]]
caller_parts += [body(player,n) for n in ['Player_UseItem','Player_ProcessItemButtons','Player_UpdateItems']]
caller_parts += [body('mm/mods/items/helpers/equip_helper.c',n) for n in ['ItemInput_RequestItemChange','ItemEquip_Update']]
caller_parts += [body('mm/mods/items/logic/custom_items.c',n) for n in ['Net_OnEquip','Net_OnUnequip','Handle_Net']]
caller_parts += [body('mm/mods/items/logic/item_spinner.c','Spinner_Start')]
def run_caller(name,checks):
 if len(sys.argv)>1 and sys.argv[1]!=name:return
 with tempfile.TemporaryDirectory(prefix='mm-nei-caller-') as td:
  src=Path(td)/'caller.c';binary=Path(td)/'caller'
  src.write_text(caller_prefix+'\n'.join(caller_parts)+'\nint main(void){'+checks+'\nputs("PASS '+name+'");}')
  subprocess.run(['cc','-std=gnu17',*flags(),*SANITIZER_FLAGS,'-Werror=implicit-function-declaration','-ffunction-sections','-fdata-sections',str(src),'-Wl,--gc-sections','-lm','-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True)
run_caller('net_native_replacement',r'''
 Player p;PlayState play;ResetCaller(&p,&play,ITEM_NET,BTN_B);
 Handle_Net(&p,&play);Player_UpdateItems(&p,&play);
 assert(p.heldItemId==ITEM_SWORD_KOKIRI&&initializations==1&&transitions==0);
 ResetCaller(&p,&play,ITEM_NET,BTN_CDOWN);Handle_Net(&p,&play);Player_UpdateItems(&p,&play);
 assert(p.heldItemId==ITEM_BOW&&transitions==1&&animationDestination==PLAYER_MODELGROUP_BOW);
 ResetCaller(&p,&play,ITEM_NET,BTN_DDOWN);dpadEnabled=1;Handle_Net(&p,&play);Player_UpdateItems(&p,&play);
 assert(p.heldItemId==ITEM_BOW&&transitions==1&&animationDestination==PLAYER_MODELGROUP_BOW);
 ResetCaller(&p,&play,ITEM_NET,BTN_DDOWN);Handle_Net(&p,&play);Player_UpdateItems(&p,&play);
 assert(p.heldItemId==ITEM_NONE&&transitions==1&&animationDestination==PLAYER_MODELGROUP_DEFAULT);
 // Native processing and use gates still reject unavailable actions; cleanup must not force NONE.
 ResetCaller(&p,&play,ITEM_NET,BTN_CDOWN);p.stateFlags1|=PLAYER_STATE1_CARRYING_ACTOR;
 Handle_Net(&p,&play);Player_UpdateItems(&p,&play);
 assert(p.heldItemId==ITEM_NET&&!transitions&&!initializations);
 ResetCaller(&p,&play,ITEM_NET,BTN_CDOWN);p.stateFlags1|=PLAYER_STATE1_8000000;
 Handle_Net(&p,&play);Player_UpdateItems(&p,&play);
 assert(p.heldItemId==ITEM_NET&&!transitions&&!initializations);
 ResetCaller(&p,&play,ITEM_NET,BTN_B);p.itemAction=PLAYER_IA_BOW;
 Handle_Net(&p,&play);Player_UseItem(&play,&p,ITEM_SWORD_KOKIRI);
 assert(p.heldItemId==ITEM_NET&&!transitions&&!initializations);
 // A cancellation and removal from every button explicitly stow through the native path.
 ResetCaller(&p,&play,ITEM_NET,BTN_A);Handle_Net(&p,&play);Player_UpdateItems(&p,&play);
 assert(p.heldItemId==ITEM_NONE&&transitions==1&&animationDestination==PLAYER_MODELGROUP_DEFAULT);
 ResetCaller(&p,&play,ITEM_NET,0);netInput.wasEquipped=0;Handle_Net(&p,&play);Player_UpdateItems(&p,&play);
 assert(p.heldItemId==ITEM_NONE&&transitions==1&&animationDestination==PLAYER_MODELGROUP_DEFAULT);
 // Reverse Net identity changes work; ordinary same-action sword use remains the native reuse path.
 ResetCaller(&p,&play,ITEM_SWORD_KOKIRI,0);Player_UseItem(&play,&p,ITEM_NET);
 assert(p.heldItemId==ITEM_NET&&initializations==1);
 ResetCaller(&p,&play,ITEM_SWORD_KOKIRI,0);Player_UseItem(&play,&p,ITEM_SWORD_KOKIRI);
 assert(p.heldItemId==ITEM_SWORD_KOKIRI&&!initializations&&!transitions&&sPlayerUseHeldItem);
''')
run_caller('spinner_native_stow',r'''
 Player p;PlayState play;ResetCaller(&p,&play,ITEM_SWORD_KOKIRI,BTN_CLEFT);
 Spinner_Start(&p,&play);Player_UpdateItems(&p,&play);
 assert(sActive&&sState==SPINNER_STATE_CHARGING&&p.actor.velocity.y==8);
 assert(p.heldItemId==ITEM_NONE&&transitions==1&&animationDestination==PLAYER_MODELGROUP_DEFAULT);
 // Explicit stow still respects the native mismatched action gate.
 ResetCaller(&p,&play,ITEM_SWORD_KOKIRI,0);p.itemAction=PLAYER_IA_BOW;
 Spinner_Start(&p,&play);
 assert(p.heldItemId==ITEM_SWORD_KOKIRI&&p.nextModelGroup==PLAYER_MODELGROUP_BOW);
 assert(!(p.stateFlags3&PLAYER_STATE3_START_CHANGING_HELD_ITEM)&&!initializations);
 // Native Goron immediate initialization remains distinct from human change animation.
 ResetCaller(&p,&play,ITEM_SWORD_KOKIRI,0);p.transformation=PLAYER_FORM_GORON;
 Spinner_Start(&p,&play);
 assert(p.heldItemAction==PLAYER_IA_NONE&&p.nextModelGroup==PLAYER_MODELGROUP_DEFAULT&&initializations==1);
 assert(!(p.stateFlags3&PLAYER_STATE3_START_CHANGING_HELD_ITEM));
''')

# Extended swords share a native action but retain distinct held/model identities.
# Use the complete production resolver and native button/use/acceptance pipeline.
equipment_prefix = caller_prefix.replace('#include "mods/extended_player.h"',
    '#include "mods/extended_player.h"\n#include "mods/extended_equipment.h"\n#include "mods/extended_inventory.h"')
equipment_prefix = re.sub(r's8 ExtPlayer_GetItemAction\(s32 item\)\{.*?\n\}', '', equipment_prefix, flags=re.S)
equipment_prefix = equipment_prefix.replace('static int dpadEnabled;',
    'static int dpadEnabled;static u8 requestedB,requestedC,requestedD;')
equipment_prefix = equipment_prefix.replace('return ITEM_SWORD_KOKIRI;}', 'return requestedB;}')
equipment_prefix = equipment_prefix.replace('if(slot==EQUIP_SLOT_B)return ITEM_SWORD_KOKIRI;',
    'if(slot==EQUIP_SLOT_B)return requestedB;')
equipment_prefix = equipment_prefix.replace('if(slot==EQUIP_SLOT_C_LEFT)return ITEM_NET;',
    'if(slot==EQUIP_SLOT_C_LEFT)return requestedC;')
equipment_prefix = equipment_prefix.replace('slot==EQUIP_SLOT_D_DOWN?ITEM_BOW:ITEM_NONE;',
    'slot==EQUIP_SLOT_D_DOWN?requestedD:ITEM_NONE;')
equipment_prefix += r'''
uint8_t Sw97_EffectiveElement(uint8_t carrier){return 0;}
const NeiItem* Nei_FindByItem(int32_t item){return NULL;}
'''
equipment_parts = [native_decl(caller_player, 's8 sItemItemActions['),
    body('mm/mods/extended_player.c', 'ExtPlayer_GetItemAction'), *caller_parts]

def run_equipment_caller(name, checks):
 global caller_prefix,caller_parts
 original_prefix,original_parts=caller_prefix,caller_parts
 caller_prefix,caller_parts=equipment_prefix,equipment_parts
 try: run_caller(name, checks)
 finally: caller_prefix,caller_parts=original_prefix,original_parts

run_equipment_caller('extended_sword_native_identity',r'''
 Player p;PlayState play;
 const u8 swords[]={ITEM_EXT_SWORD_2,ITEM_EXT_SWORD_3,ITEM_EXT_SWORD_1};
 for(unsigned a=0;a<3;a++)for(unsigned b=0;b<3;b++){
  ResetCaller(&p,&play,swords[a],BTN_B);requestedB=swords[b];
  assert(ExtPlayer_GetItemAction(requestedB)==PLAYER_IA_SWORD_KOKIRI);
  Player_UpdateItems(&p,&play);
  assert(p.heldItemId==requestedB&&"B sword selection retained the previous sword identity");
  assert(!equipmentToggles&&p.heldItemAction==PLAYER_IA_SWORD_KOKIRI);
  if(a==b)assert(sPlayerUseHeldItem&&!initializations&&!transitions);
  else assert(initializations==1&&!transitions);
 }
 for(unsigned a=0;a<3;a++){
  ResetCaller(&p,&play,ITEM_SWORD_KOKIRI,BTN_B);requestedB=swords[a];Player_UpdateItems(&p,&play);
  assert(p.heldItemId==requestedB&&initializations==1);
  ResetCaller(&p,&play,swords[a],BTN_B);requestedB=ITEM_SWORD_KOKIRI;Player_UpdateItems(&p,&play);
  assert(p.heldItemId==ITEM_SWORD_KOKIRI&&initializations==1);
 }
 ResetCaller(&p,&play,ITEM_EXT_SWORD_3,BTN_B);requestedB=ITEM_EXT_SWORD_2;
 p.stateFlags1|=PLAYER_STATE1_8000000;Player_UpdateItems(&p,&play);
 assert(p.heldItemId==ITEM_EXT_SWORD_3&&!initializations&&!transitions);
 ResetCaller(&p,&play,ITEM_EXT_SWORD_3,BTN_B);requestedB=ITEM_EXT_SWORD_2;p.itemAction=PLAYER_IA_BOW;
 Player_UseItem(&play,&p,requestedB);assert(p.heldItemId==ITEM_EXT_SWORD_3&&!initializations&&!transitions);
''')
run_equipment_caller('extended_equipment_native_toggle',r'''
 Player p;PlayState play;
 for(u8 item=ITEM_EXT_SWORD_1;item<=ITEM_EXT_BOOTS_3;item++){
  ResetCaller(&p,&play,ITEM_SWORD_KOKIRI,BTN_CLEFT);requestedB=ITEM_SWORD_KOKIRI;requestedC=item;
  Player_UpdateItems(&p,&play);
  assert(equipmentToggles==1&&toggledEquipment==item&&"C equipment press never reached its toggle");
  assert(p.heldItemId==ITEM_SWORD_KOKIRI&&!transitions&&!initializations);
  ResetCaller(&p,&play,ITEM_SWORD_KOKIRI,BTN_DDOWN);requestedD=item;dpadEnabled=1;
  Player_UpdateItems(&p,&play);assert(equipmentToggles==1&&toggledEquipment==item);
  assert(p.heldItemId==ITEM_SWORD_KOKIRI&&!transitions&&!initializations);
 }
 ResetCaller(&p,&play,ITEM_SWORD_KOKIRI,BTN_CLEFT);requestedC=ITEM_EXT_TUNIC_2;
 p.stateFlags1|=PLAYER_STATE1_CARRYING_ACTOR;Player_UpdateItems(&p,&play);
 assert(!equipmentToggles&&p.heldItemId==ITEM_SWORD_KOKIRI);
 ResetCaller(&p,&play,ITEM_SWORD_KOKIRI,0);requestedC=ITEM_EXT_TUNIC_2;
 play.state.input[0].cur.button=BTN_CLEFT;Player_UpdateItems(&p,&play);assert(!equipmentToggles);
''')

# Custom tools see raw presses before the native button dispatcher. Run the complete
# Gust Jar handler and its real unequip path around that production update boundary.
gust_path='mm/mods/items/logic/item_gustjar.c'
gust_prefix=equipment_prefix.replace('const NeiItem* Nei_FindByItem(int32_t item){return NULL;}',
    'const NeiItem* Nei_FindByItem(int32_t item){static NeiItem gust={.ia=PLAYER_IA_GUST_JAR};return item==ITEM_GUST_JAR?&gust:NULL;}')
gust_prefix=gust_prefix.replace('static int dpadEnabled;', 'static int buttonQueries;static int dpadEnabled;')
gust_prefix=gust_prefix.replace('ItemId Player_GetItemOnButton(PlayState* play,Player* p,EquipSlot slot){',
    'ItemId Player_GetItemOnButton(PlayState* play,Player* p,EquipSlot slot){++buttonQueries;')
gust_prefix=gust_prefix.replace('ItemId Player_Dpad_GetItemOnButton(PlayState* play,Player* p,DpadEquipSlot slot){',
    'ItemId Player_Dpad_GetItemOnButton(PlayState* play,Player* p,DpadEquipSlot slot){++buttonQueries;')
gust_prefix+='\n#include "mods/items/logic/item_gustjar.h"\n#include "mods/transformation_masks/transformation_masks.h"\n#include "mods/transformation_masks/assets/mm_asset_loader.h"\nstatic u8 sGustTornadoOn;\n'
def service_stub(path,name):
 signature=body(path,name).split('{',1)[0]
 return signature+'{'+('' if re.search(r'\bvoid\s+'+name+r'\(',signature) else 'return 0;')+'}\n'
for name in ['GustJar_ClearScaleCache','GustJar_TornadoStop','GustJar_GetElement','GustJar_ElementCount',
             'GustJar_ElementNeighbor','GustJar_SetElement','GustJar_ApplyCarryPose','GustJar_GetAimYaw',
             'GustJar_Absorb','GustJar_Blow','Player_InitGustJarIA']:
 gust_prefix+=service_stub(gust_path,name)
for name in ['FirstPerson_Init','FirstPerson_Exit','FirstPerson_Update','FirstPerson_GetAimPitch']:
 gust_prefix+=service_stub('mm/mods/items/helpers/camera_helper.c',name)
gust_prefix+=r'''
static u8 equipmentOwned=1,equipmentAgeAllowed=1,currentEquipment,pikachu;
static int genericOtherButtons,consumeR;
void SetGustAssignments(void){
 BUTTON_ITEM_EQUIP(0,EQUIP_SLOT_C_LEFT)=requestedC;
 DPAD_BUTTON_ITEM_EQUIP(0,EQUIP_SLOT_D_DOWN)=requestedD;
 gSaveContext.buttonStatus[EQUIP_SLOT_C_LEFT]=BTN_ENABLED;
 gSaveContext.shipSaveContext.dpad.status[EQUIP_SLOT_D_DOWN]=BTN_ENABLED;
 buttonQueries=0;
}
u8 ExtEquip_HasItem(s16 type,u8 index){return equipmentOwned;}
u8 ExtEquip_CheckAgeReq(s16 type,u8 index){return equipmentAgeAllowed;}
u8 ExtEquip_GetCurrent(s16 type){return currentEquipment;}
u8 ExtEquip_SlotRetired(s16 type,u8 index){return 0;}
u8 ExtEquip_TridentAllowsShield(u8 index,u16 native){return index==1||index==3;}
u8 TransformMasks_IsTransformedAny(void){return pikachu;}
MmPlayerTransformation MmForm_GetCurrentForm(void){return pikachu?MM_PLAYER_FORM_PIKACHU:MM_PLAYER_FORM_HUMAN;}
u8 ItemInput_IsBlocked(Player* p,PlayState* play){return 0;}
void ItemEquip_PlayEquipSFXForAction(PlayState* play,Player* p,s32 a){}
void ItemEquip_PlayUnequipSFXForAction(PlayState* play,Player* p,s32 a){}
void Audio_StopSfxById(u32 id){}
bool Player_IsZTargeting(Player* p){return 0;}
void Player_PlaySfx(Player* p,u16 id){}
f32 Math_SinS(s16 a){return 0;}
f32 Math_CosS(s16 a){return 0;}
s32 Math_ScaledStepToS(s16* a,s16 b,s16 c){return 0;}
s16 Math_Vec3f_Yaw(Vec3f* a,Vec3f* b){return 0;}
'''
gust_parts=[body('mm/mods/items/helpers/equip_helper.c','ItemInput_CheckOtherButtons'),
    *[body(gust_path,n) for n in ['GustJar_Equip','GustJar_Unequip','Handle_GustJar']]]
gust_parts+=[r'''
void CustomItems_Update(Player* p,PlayState* play){
 genericOtherButtons=ItemInput_CheckOtherButtons(BTN_CRIGHT,&play->state.input[0]);
 Handle_GustJar(p,play);
 if(consumeR)play->state.input[0].press.button&=~BTN_R;
}
''']
player_functions=functions(source(player))
if 'Player_UpdateCustomItems' in player_functions:
 assert 'Player_UpdateCustomItems(this, play);' in player_functions['Player_Update']
 gust_parts.append(player_functions['Player_UpdateCustomItems'])
else:
 # Preserve the baseline's actual call as the control, without changing HEAD.
 old_call=re.search(r'CustomItems_Update\(this, play\);',player_functions['Player_Update'])[0]
 gust_parts.append('static void Player_UpdateCustomItems(Player* this,PlayState* play){'+old_call+'}')
gust_parts+=equipment_parts
original_prefix,original_parts=caller_prefix,caller_parts
caller_prefix,caller_parts=gust_prefix,gust_parts
run_caller('extended_equipment_held_tool',r'''
 Player p;PlayState play;
 for(int dpad=0;dpad<2;dpad++)for(u8 item=ITEM_EXT_SWORD_1;item<=ITEM_EXT_BOOTS_3;item++){
  ResetCaller(&p,&play,ITEM_GUST_JAR,dpad?BTN_DDOWN:BTN_CLEFT);
  p.heldItemAction=p.itemAction=PLAYER_IA_GUST_JAR;p.modelGroup=PLAYER_MODELGROUP_DEFAULT;
  requestedB=ITEM_SWORD_KOKIRI;requestedC=item;requestedD=item;dpadEnabled=dpad;
  SetGustAssignments();
  gSaveContext.save.saveInfo.equips.buttonItems[0][EQUIP_SLOT_C_RIGHT]=ITEM_GUST_JAR;
  netInput=(ItemInputState){.wasEquipped=1,.equippedButton=BTN_CRIGHT};
  gjEquipped=1;gjFirstPerson=0;gjCollider.base.shape=COLSHAPE_CYLINDER;sGustTornadoOn=1;
  Player_UpdateCustomItems(&p,&play);Player_UpdateItems(&p,&play);
  assert(gjEquipped&&"equipment toggle stowed the held Gust Jar before native dispatch");
  assert(!genericOtherButtons&&equipmentToggles==1&&toggledEquipment==item);
  assert(p.heldItemId==ITEM_GUST_JAR&&p.heldItemAction==PLAYER_IA_GUST_JAR);
 }
 const u16 unrelated[]={BTN_A,BTN_B,BTN_START,BTN_CDOWN};
 for(unsigned i=0;i<sizeof(unrelated)/sizeof(unrelated[0]);i++){
  ResetCaller(&p,&play,ITEM_GUST_JAR,unrelated[i]);requestedC=ITEM_EXT_TUNIC_2;
  SetGustAssignments();
  gjEquipped=1;Player_UpdateCustomItems(&p,&play);assert(!gjEquipped&&genericOtherButtons);
 }
 ResetCaller(&p,&play,ITEM_GUST_JAR,BTN_CUP);gjEquipped=1;gjFirstPerson=0;
 Player_UpdateCustomItems(&p,&play);assert(gjEquipped&&gjFirstPerson&&!genericOtherButtons);
 // Ownership, age, transformation and disabled D-pad slots do not reserve presses.
 for(int blocked=0;blocked<5;blocked++){
  ResetCaller(&p,&play,ITEM_GUST_JAR,blocked==3?BTN_DDOWN:BTN_CLEFT);
  netInput=(ItemInputState){.wasEquipped=1,.equippedButton=BTN_CRIGHT};
  requestedC=requestedD=ITEM_EXT_TUNIC_2;equipmentOwned=blocked!=0;
  SetGustAssignments();
  if(blocked==4)gSaveContext.buttonStatus[EQUIP_SLOT_C_LEFT]=BTN_DISABLED;
  equipmentAgeAllowed=blocked!=1;pikachu=blocked==2;dpadEnabled=0;gjEquipped=1;
  Player_UpdateCustomItems(&p,&play);assert(!gjEquipped&&genericOtherButtons);
 }
 equipmentOwned=equipmentAgeAllowed=1;pikachu=0;
 // B and C retain their native priority over an equipment press on a lower-priority button.
 ResetCaller(&p,&play,ITEM_GUST_JAR,BTN_B|BTN_CLEFT);requestedC=ITEM_EXT_TUNIC_2;
 SetGustAssignments();
 netInput=(ItemInputState){.wasEquipped=1,.equippedButton=BTN_CRIGHT};gjEquipped=1;
 Player_UpdateCustomItems(&p,&play);assert(!gjEquipped&&genericOtherButtons);
 ResetCaller(&p,&play,ITEM_GUST_JAR,BTN_CLEFT|BTN_DDOWN);requestedC=ITEM_BOW;
 requestedD=ITEM_EXT_TUNIC_2;dpadEnabled=1;
 SetGustAssignments();
 netInput=(ItemInputState){.wasEquipped=1,.equippedButton=BTN_CRIGHT};gjEquipped=1;
 Player_UpdateCustomItems(&p,&play);assert(!gjEquipped&&genericOtherButtons);
 // An age-restricted selected piece can still be removed, matching the existing toggle path.
 ResetCaller(&p,&play,ITEM_GUST_JAR,BTN_CLEFT);requestedC=ITEM_EXT_TUNIC_2;
 SetGustAssignments();
 equipmentAgeAllowed=0;currentEquipment=2;
 netInput=(ItemInputState){.wasEquipped=1,.equippedButton=BTN_CRIGHT};gjEquipped=1;
 Player_UpdateCustomItems(&p,&play);assert(gjEquipped&&!genericOtherButtons);
 equipmentAgeAllowed=1;currentEquipment=3;requestedC=ITEM_EXT_SHIELD_2;gjEquipped=1;
 SetGustAssignments();
 Player_UpdateCustomItems(&p,&play);assert(!gjEquipped&&genericOtherButtons);currentEquipment=0;
 ResetCaller(&p,&play,ITEM_GUST_JAR,BTN_CLEFT|BTN_R);requestedC=ITEM_EXT_TUNIC_2;
 SetGustAssignments();
 gjEquipped=1;gjFirstPerson=0;consumeR=1;Player_UpdateCustomItems(&p,&play);
 assert(gjEquipped&&play.state.input[0].press.button==BTN_CLEFT);
 // Unrelated item getters can have authored side effects: never call them before native gates.
 ResetCaller(&p,&play,ITEM_GUST_JAR,BTN_CLEFT);requestedC=ITEM_OCARINA_OF_TIME;
 SetGustAssignments();Player_UpdateCustomItems(&p,&play);assert(!buttonQueries);
''')
caller_prefix,caller_parts=original_prefix,original_parts

# Regression: the tool handler starts before Player_Update selects input. Execute that
# selection and the real native button/use pipeline; the transition-animation boundary
# remains the same recorder used by the Net tests above.
caller_prefix = caller_prefix.replace('static int transitions,initializations;', 'static u8 requestedTool;static int transitions,initializations;')
caller_prefix = caller_prefix.replace('if(slot==EQUIP_SLOT_C_LEFT)return ITEM_NET;', 'if(slot==EQUIP_SLOT_C_LEFT)return requestedTool;')
caller_prefix = caller_prefix.replace('return slot==EQUIP_SLOT_D_DOWN?ITEM_BOW:ITEM_NONE;', 'return slot==EQUIP_SLOT_D_DOWN?requestedTool:ITEM_NONE;')
caller_prefix = caller_prefix.replace('if(item==ITEM_NET||item==ITEM_SWORD_KOKIRI)', 'if(item==ITEM_DEKU_LEAF)return PLAYER_IA_DEKU_LEAF;\n if(item==ITEM_SHOVEL)return PLAYER_IA_SHOVEL;\n if(item==ITEM_NET||item==ITEM_SWORD_KOKIRI)')
caller_prefix += r'''
#include "mods/items/logic/item_dekuleaf.h"
#include "mods/items/logic/item_shovel.h"
static u8 sDekuLeafBlowEffectFired;
static PlayerAnimationHeader toolAnim;
#define NEI_ANIM_DEKULEAF_BLOW "leaf"
#define NEI_ANIM_DAMPE_DIG "dig"
s32 ItemMagic_HasEnough(PlayState* play,s16 amount){return 1;}
LinkAnimationHeader* NeiAnim_Load(const char* key){return &toolAnim;}
void DekuLeaf_InitCollider(PlayState* play,Player* p){}
void PlayerAnimation_PlayOnce(PlayState* play,SkelAnime* skel,PlayerAnimationHeader* anim){skel->curFrame=0;}
void ItemEquip_PlayEquipSFXForAction(PlayState* play,Player* p,s32 action){}
s32 Player_UpperAction_DekuLeaf(Player* p,PlayState* play){return 1;}
s32 Player_UpperAction_Shovel(Player* p,PlayState* play){return 1;}
'''
update = body(player, 'Player_Update')
selection = update[update.index('if (play->actorCtx.isOverrideInputOn'):update.index('    GameInteractor_ExecuteOnPassPlayerInputs')]
caller_parts += [body(leaf,'DekuLeaf_StartBlow'),body('mm/mods/items/logic/item_shovel.c','Shovel_Start'),body('mm/mods/items/custom_items_common.c','CustomItems_BlocksMovement')]
caller_parts += ['Input SelectToolInput(Player* this,PlayState* play){Input input={0};'+selection+'return input;}']
run_caller('tool_native_activation',r'''
 Player p;PlayState play;
 for(int tool=0;tool<2;tool++)for(int dpad=0;dpad<2;dpad++){
  u16 button=dpad?BTN_DDOWN:BTN_CLEFT;
  requestedTool=tool==0?ITEM_DEKU_LEAF:ITEM_SHOVEL;
  ResetCaller(&p,&play,ITEM_SWORD_KOKIRI,button);dpadEnabled=dpad;
  play.actorCtx.actorLists[ACTORCAT_PLAYER].first=&p.actor;
  play.state.input[0].cur.button=button;play.state.input[0].cur.stick_x=40;
  memset(&gCustomItemState,0,sizeof(gCustomItemState));
  if(tool==0)DekuLeaf_StartBlow(&p,&play);else Shovel_Start(&p,&play);
  Input input=SelectToolInput(&p,&play);sPlayerControlInput=&input;
  Player_UpdateItems(&p,&play);
  assert(input.press.button==button&&transitions==1&&p.heldItemId==requestedTool);
  assert(animationDestination==PLAYER_MODELGROUP_DEFAULT);
  // The movement lock belongs only to an installed and advancing tool upper action.
  p.heldItemAction=tool==0?PLAYER_IA_DEKU_LEAF:PLAYER_IA_SHOVEL;
  p.upperActionFunc=tool==0?Player_UpperAction_DekuLeaf:Player_UpperAction_Shovel;
  assert(SelectToolInput(&p,&play).press.button==button);
  if(tool==0)dlAnimTimer=1;else shAnimTimer=1;
  assert(!SelectToolInput(&p,&play).cur.stick_x);
  p.upperActionFunc=NULL;
  assert(SelectToolInput(&p,&play).cur.stick_x==40);
 }
''')
