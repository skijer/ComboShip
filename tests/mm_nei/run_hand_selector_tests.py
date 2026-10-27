"""Execute native final hand selectors and actual adult override/fit query bodies."""
from pathlib import Path
import re,sys,tempfile,subprocess
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_mm_nei_tests import flags
from run_time_pedestal_tests import functions
native=(ROOT/'mm/src/code/z_player_lib.c').read_text()
adult=(ROOT/'mm/mods/items/logic/adult_link_render.cpp').read_text()
def extract(source,name):
 mask=re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"',lambda m:' '*len(m[0]),source,flags=re.S)
 match=re.search(r'^[^\n;]*\b'+name+r'\([^;]*?\)\s*\{',mask,re.M)
 assert match,name
 start=match.start();pos=match.end();depth=1
 while depth:
  depth+=(mask[pos]=='{')-(mask[pos]=='}');pos+=1
 return source[start:pos]
names=['Player_OverrideLimbDrawGameplayDefault','Player_OverrideLimbDrawGameplayFirstPerson','Player_ApplyNeiHeldHand']
nf={n:extract(native,n) for n in names}
af={n:extract(adult.replace('extern "C" ',''),n) for n in ['AdultLink_UsesAdultPresentation','AdultLink_ApplyNeiHeldHand','AdultLink_OverrideLimb']}
native_start=native.index('            u8 handIsSpokenFor =')
native_end=native.index('            // ⚠️ ESTE OCULTADO',native_start)
native_sword=native[native_start:native_end]
for name in ['Player_OverrideLimbDrawGameplayDefault','Player_OverrideLimbDrawGameplayFirstPerson']:
 assert 'Player_ApplyNeiHeldHand(play, player, limbIndex, dList)' in nf[name]
# Exercise the complete final adult override, not just an intermediate helper.
prefix=r'''
#include "z64.h"
#include "variables.h"
#include "mods/forms/custom_forms.h"
#include "render/sword_element_aura_profiles.h"
#include <cassert>
#include <string>
#include <iostream>
#include <array>
#include <cstring>
SaveContext gSaveContext{};
PlayState* gPlayState=nullptr;
u16 gEquipMasks[4]{};u8 gEquipShifts[4]{};
bool mode=false,lantern=false,hook=true,hasHand=true,claimedHand=false;
int form=CUSTOM_FORM_NONE, sPlayerLod=0,sPlayerLeftHandType=0;
u8 sReady=0,sIsChildRig=0,sIsMod=0,sIsForm=0;
bool extOwnsSwordDL=false,odwalda=false,goht=false,gold=false,customBody=false,heldEnabled=false,dinSelected=false;
int heldCalls=0,tunicR=30,tunicG=105,tunicB=27;
FlexSkeletonHeader skel{},*sSkel=nullptr;
Gfx nativeLeft{},nativeRight{},adultLeft{},adultRight{},customHand{},replacement{},vanillaHook{},swordMesh{},dinHand{};
static Gfx heldScratch[8]{};
static std::array<std::array<Gfx,8>,16> swordFrames;
static unsigned swordAllocations=0;
static Gfx* AllocateSword(size_t size){assert(size==8*sizeof(Gfx)&&swordAllocations<swordFrames.size());return swordFrames[swordAllocations++].data();}
#undef GRAPH_ALLOC
#define GRAPH_ALLOC(context,size) AllocateSword(size)
Gfx* gPlayerLeftHandClosedDLs[10]{};Gfx* gPlayerRightHandClosedDLs[10]{};
std::string resolved;
s32 AdultLink_IsActive(){return mode;}
s32 CustomForms_ActiveForm(){return form;}
f32 CustomForms_RootScale(){return .335f;}
f32 CustomForms_RootDropBefore(){return 0;}
f32 CustomForms_RootDropAfter(){return 0;}
Gfx* CustomForms_HandDL(Player*,s32,u8* claimed){*claimed=claimedHand;return &customHand;}
u8 CustomForms_HidesSheath(Player*){return 0;}
const char* ExtEquip_GetShieldDLOverride(){return nullptr;}
u8 ExtEquip_ShouldHideSwordDL(){return extOwnsSwordDL;}
u8 BossRemains_IsOdolwaWorn(){return odwalda;}
u8 BossRemains_IsGohtWorn(){return goht;}
u8 Trident_GoldenArmor(){return gold;}
u8 Player_IsCustomLinkModel(Player*){return customBody;}
// Active Four Sword is covered by the dedicated Four Sword visibility fixture.
u8 FourSword_IsEquipped(){return 0;}
s32 CVarGetInteger(const char* name,s32 fallback){
 if(std::strstr(name,"TunicR"))return tunicR;
 if(std::strstr(name,"TunicG"))return tunicG;
 if(std::strstr(name,"TunicB"))return tunicB;
 return fallback;
}
u8 WeaponUpgrade_ApplyHeldSwordDL(Gfx** dl,void* hand,Player* p,u8 r,u8 g,u8 b){
 ++heldCalls;
 if(!heldEnabled||!hand||(p->leftHandType!=PLAYER_MODELTYPE_LH_ONE_HAND_SWORD&&p->leftHandType!=PLAYER_MODELTYPE_LH_TWO_HAND_SWORD))return 0;
 std::memset(heldScratch,0,sizeof(heldScratch));
 heldScratch[0].words.w0=G_DL<<24;heldScratch[0].words.w1=reinterpret_cast<uintptr_t>(&swordMesh);
 heldScratch[1].words.w0=G_DL<<24;heldScratch[1].words.w1=reinterpret_cast<uintptr_t>(hand);
 heldScratch[3].words.w0=G_SETENVCOLOR<<24;heldScratch[3].words.w1=(uint32_t(r)<<24)|(uint32_t(g)<<16)|(uint32_t(b)<<8);
 heldScratch[5].words.w0=G_ENDDL<<24;*dl=heldScratch;return 1;
}
void* DinFireSword_HandDL(PlayState*,Player*,Gfx*){return dinSelected?&dinHand:nullptr;}
void DinFireSword_SetHandModel(Player*,void*,int){}
void DinFireSword_CopyHandModel(Player*,void*,void*){}
void* DinFireShield_HandDL(PlayState*,Player*,Gfx*){return nullptr;}
void Player_ApplyBackEquipmentVisibility(s32,Gfx**){}
bool NeiLantern_UsesGrip(const Player*){return lantern;}
bool NeiArticulated_UsesSwitchHook(const Player*){return hook;}
Gfx* capturedHand=nullptr;
bool NeiArticulated_ApplySwitchHookHand(PlayState*,Player*,Gfx** dl,Gfx* hand){if(!hook||!hand)return false;capturedHand=hand;*dl=&replacement;return true;}
u8 ResourceMgr_FileExists(const char*){return hasHand;}
Gfx* ResourceMgr_LoadGfxByName(const char* p){resolved=p;return resolved=="human_left"?&nativeLeft:&nativeRight;}
s32 Player_OverrideLimbDrawGameplayDefault(PlayState*,s32,Gfx** dl,Vec3f*,Vec3s*,Actor*){*dl=&vanillaHook;return 0;}
'''
prefix+='\n'.join(re.findall(r'^static Gfx\* sDL_\w+;',adult,re.M))+'\n'
checks=r'''
int main(){
 Player p{};p.transformation=PLAYER_FORM_HUMAN;p.actor.scale.y=.01f;PlayState play{};Vec3f pos{};Vec3s rot{};
 play.actorCtx.actorLists[ACTORCAT_PLAYER].first=&p.actor;gPlayState=&play;
 gSaveContext.save.linkAge=0;
 assert(!AdultLink_UsesAdultPresentation(nullptr));
 assert(!AdultLink_UsesAdultPresentation(&p));mode=true;assert(!AdultLink_UsesAdultPresentation(&p));
 sReady=1;sSkel=&skel;assert(AdultLink_UsesAdultPresentation(&p));
 sIsChildRig=1;assert(!AdultLink_UsesAdultPresentation(&p));mode=false;form=CUSTOM_FORM_KEATON;assert(!AdultLink_UsesAdultPresentation(&p));
 sIsChildRig=0;assert(AdultLink_UsesAdultPresentation(&p));
 sReady=0;assert(!AdultLink_UsesAdultPresentation(&p));sReady=1;sSkel=nullptr;assert(!AdultLink_UsesAdultPresentation(&p));sSkel=&skel;
 p.transformation=PLAYER_FORM_DEKU;assert(!AdultLink_UsesAdultPresentation(&p));p.transformation=PLAYER_FORM_HUMAN;
 mode=false;form=CUSTOM_FORM_NONE;
 for(int i=0;i<10;i++){gPlayerLeftHandClosedDLs[i]=(Gfx*)"wrong_form";gPlayerRightHandClosedDLs[i]=(Gfx*)"wrong_form";}
 for(int lod=0;lod<2;lod++){
 sPlayerLod=lod;gPlayerLeftHandClosedDLs[PLAYER_FORM_HUMAN*2+lod]=(Gfx*)"human_left";gPlayerRightHandClosedDLs[PLAYER_FORM_HUMAN*2+lod]=(Gfx*)"human_right";
 Gfx* limb=&vanillaHook;Player_ApplyNeiHeldHand(&play,&p,PLAYER_LIMB_RIGHT_HAND,&limb);assert(limb==&replacement&&capturedHand==&nativeRight&&resolved=="human_right");
 lantern=true;limb=&vanillaHook;Player_ApplyNeiHeldHand(&play,&p,PLAYER_LIMB_LEFT_HAND,&limb);assert(limb==&nativeLeft&&sPlayerLeftHandType==PLAYER_MODELTYPE_LH_CLOSED);lantern=false;
 hasHand=false;limb=&vanillaHook;Player_ApplyNeiHeldHand(&play,&p,PLAYER_LIMB_RIGHT_HAND,&limb);assert(limb==&vanillaHook);hasHand=true;
 }
 for(int f=0;f<PLAYER_FORM_HUMAN;f++){p.transformation=f;Gfx* limb=&vanillaHook;Player_ApplyNeiHeldHand(&play,&p,PLAYER_LIMB_RIGHT_HAND,&limb);assert(limb==&vanillaHook);}p.transformation=PLAYER_FORM_HUMAN;
 // Local overlay readiness must not suppress a native peer's third-person hand.
 Player remote=p;
 for(int custom:{CUSTOM_FORM_NONE,CUSTOM_FORM_KEATON}){
   mode=(custom==CUSTOM_FORM_NONE);form=custom;sReady=1;sSkel=&skel;sIsChildRig=0;
   assert(AdultLink_UsesAdultPresentation(&p));
   assert(!AdultLink_UsesAdultPresentation(&remote));
   Gfx* remoteLimb=&vanillaHook;capturedHand=nullptr;
   Player_ApplyNeiHeldHand(&play,&remote,PLAYER_LIMB_RIGHT_HAND,&remoteLimb);
   assert(remoteLimb==&replacement&&capturedHand==&nativeRight);
   lantern=true;remoteLimb=&vanillaHook;
   Player_ApplyNeiHeldHand(&play,&remote,PLAYER_LIMB_LEFT_HAND,&remoteLimb);
   assert(remoteLimb==&nativeLeft);lantern=false;
   Gfx* localLimb=&vanillaHook;Player_ApplyNeiHeldHand(&play,&p,PLAYER_LIMB_RIGHT_HAND,&localLimb);
   assert(localLimb==&vanillaHook);
 }
 sIsChildRig=1;assert(!AdultLink_UsesAdultPresentation(&p));
 sIsChildRig=0;sReady=0;assert(!AdultLink_UsesAdultPresentation(&p));
 sReady=1;sSkel=nullptr;assert(!AdultLink_UsesAdultPresentation(&p));sSkel=&skel;
 mode=true;sDL_RHClosed=&adultRight;sDL_LHClosed=&adultLeft;sDL_RHHookshot=&vanillaHook;
 p.rightHandType=PLAYER_MODELTYPE_RH_HOOKSHOT;
 for(bool mod:{false,true})for(bool child:{false,true}){
 sIsMod=mod;sIsChildRig=child;Gfx* limb=&customHand;
 AdultLink_OverrideLimb(&play,PLAYER_LIMB_RIGHT_HAND,&limb,&pos,&rot,&p.actor);
 assert(limb==&replacement&&capturedHand==((mod||child)?&customHand:&adultRight));
 lantern=true;limb=&customHand;AdultLink_OverrideLimb(&play,PLAYER_LIMB_LEFT_HAND,&limb,&pos,&rot,&p.actor);assert(limb==((mod||child)?&customHand:&adultLeft));lantern=false;
 }
 claimedHand=true;Gfx* limb=&customHand;AdultLink_OverrideLimb(&play,PLAYER_LIMB_RIGHT_HAND,&limb,&pos,&rot,&p.actor);assert(limb==&replacement&&capturedHand==&customHand);
 assert(gSaveContext.save.linkAge==0);
 // Execute the native sword seam and the full adult override. Deferred clones
 // must retain their original fist/color after the helper's scratch is reused.
 sIsMod=sIsForm=sIsChildRig=0;claimedHand=false;form=CUSTOM_FORM_NONE;mode=false;
 p.leftHandType=PLAYER_MODELTYPE_LH_ONE_HAND_SWORD;sPlayerLeftHandType=p.leftHandType;heldEnabled=true;heldCalls=0;
 Gfx* first=&vanillaHook;ApplyNativeSwordStage(&play,&p,&first);
 assert(first!=&vanillaHook&&first!=heldScratch&&first[1].words.w1==reinterpret_cast<uintptr_t>(&nativeLeft));
 assert(first[3].words.w1==0&&sPlayerLeftHandType==p.leftHandType);
 std::array<Gfx,8> nativeSnapshot;std::memcpy(nativeSnapshot.data(),first,sizeof(nativeSnapshot));
 gold=true;Gfx* second=&vanillaHook;ApplyNativeSwordStage(&play,&p,&second);gold=false;
 assert(first!=second&&second[3].words.w1==0xffcd2800&&std::memcmp(first,nativeSnapshot.data(),sizeof(nativeSnapshot))==0);
 for(int owner=0;owner<7;++owner){
  extOwnsSwordDL=owner==0;odwalda=owner==1;goht=owner==2;customBody=owner==3;
  form=owner==4?CUSTOM_FORM_KEATON:CUSTOM_FORM_NONE;p.actor.scale.y=owner==5?-.01f:.01f;
  p.transformation=owner==6?PLAYER_FORM_GORON:PLAYER_FORM_HUMAN;heldCalls=0;
  Gfx* skipped=&vanillaHook;ApplyNativeSwordStage(&play,&p,&skipped);assert(skipped==&vanillaHook&&heldCalls==0);
 }
 extOwnsSwordDL=odwalda=goht=customBody=false;form=CUSTOM_FORM_NONE;p.actor.scale.y=.01f;p.transformation=PLAYER_FORM_HUMAN;
 // Local adult mode owns only the local adult body; a native peer still gets
 // its real human fist. A local custom/child form owns its equipment entirely.
 mode=true;sReady=1;sSkel=&skel;heldCalls=0;first=&vanillaHook;
 ApplyNativeSwordStage(&play,&p,&first);assert(first==&vanillaHook&&heldCalls==0);
 remote=p;first=&vanillaHook;ApplyNativeSwordStage(&play,&remote,&first);assert(first!=&vanillaHook&&first[1].words.w1==reinterpret_cast<uintptr_t>(&nativeLeft));
 form=CUSTOM_FORM_KEATON;sIsChildRig=1;first=&vanillaHook;heldCalls=0;
 ApplyNativeSwordStage(&play,&p,&first);assert(first==&vanillaHook&&heldCalls==0);
 sIsChildRig=0;form=CUSTOM_FORM_NONE;sDL_LHSword=&vanillaHook;
 first=&customHand;AdultLink_OverrideLimb(&play,PLAYER_LIMB_LEFT_HAND,&first,&pos,&rot,&p.actor);
 assert(first!=heldScratch&&first[1].words.w1==reinterpret_cast<uintptr_t>(&adultLeft)&&first[3].words.w1==0x1e691b00);
 std::array<Gfx,8> adultSnapshot;std::memcpy(adultSnapshot.data(),first,sizeof(adultSnapshot));
 tunicR=99;tunicG=88;tunicB=77;second=&customHand;
 AdultLink_OverrideLimb(&play,PLAYER_LIMB_LEFT_HAND,&second,&pos,&rot,&p.actor);
 assert(first!=second&&second[3].words.w1==0x63584d00&&std::memcmp(first,adultSnapshot.data(),sizeof(adultSnapshot))==0);
 gold=true;second=&customHand;AdultLink_OverrideLimb(&play,PLAYER_LIMB_LEFT_HAND,&second,&pos,&rot,&p.actor);gold=false;assert(second[3].words.w1==0xffcd2800);
 for(int owner=0;owner<8;++owner){
  sIsMod=owner==0;sIsForm=owner==1;sIsChildRig=owner==2;
  form=owner==3?CUSTOM_FORM_KEATON:CUSTOM_FORM_NONE;extOwnsSwordDL=owner==4;odwalda=owner==5;goht=owner==6;
  p.actor.scale.y=owner==7?-.01f:.01f;heldCalls=0;second=&customHand;
  AdultLink_OverrideLimb(&play,PLAYER_LIMB_LEFT_HAND,&second,&pos,&rot,&p.actor);assert(heldCalls==0);
 }
 sIsMod=sIsForm=sIsChildRig=0;extOwnsSwordDL=odwalda=goht=false;p.actor.scale.y=.01f;form=CUSTOM_FORM_NONE;
 dinSelected=true;second=&customHand;AdultLink_OverrideLimb(&play,PLAYER_LIMB_LEFT_HAND,&second,&pos,&rot,&p.actor);assert(second==&dinHand);
 std::cout<<"PASS native/adult held sword fists, isolated frame compounds/colors, form/equipment/Din priority and original final hand selectors\n";
}
'''
parts=[af['AdultLink_UsesAdultPresentation'],nf['Player_ApplyNeiHeldHand'],af['AdultLink_ApplyNeiHeldHand'],af['AdultLink_OverrideLimb'],
       'static void ApplyNativeSwordStage(PlayState* play,Player* player,Gfx** dList){\n'+native_sword+'\n}']
with tempfile.TemporaryDirectory(prefix='mm-nei-hands-') as td:
 path=Path(td)/'hands.cpp';path.write_text(prefix+'\n'.join(parts)+checks);binary=Path(td)/'hands'
 subprocess.run(['c++','-std=c++20',*flags(),str(path),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
