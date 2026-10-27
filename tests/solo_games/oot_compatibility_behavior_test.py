#!/usr/bin/env python3
"""Run native Farore input, Triforce receipt and optional MM-model fallback bodies."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
def extract(path,marker):
 s=(ROOT/path).read_text();a=s.index(marker);b=s.index('{',a);n=1;i=b+1
 while n:n+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[a:i]+'\n'
player=(ROOT/'soh/src/overlays/actors/ovl_player_actor/z_player.c').read_text();st=player.rfind('#ifdef COMBO_BUILD',0,player.index('// DOWN is the parent entrance here;'));en=player.index('#endif',st)+len('#endif')
cpp=r'''
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <string>
#include <initializer_list>
#include <iostream>
#include "ComboGameMode.h"
extern "C"{int gComboGameMode=COMBO_GAME_OOT;}
struct PlayState{int sceneNum=1;int gameplayFrames=0;};struct GetItemEntry{};
constexpr int SCENE_GROTTOS=1,SCENE_FAIRYS_FOUNTAIN=2,PLAYER_IA_FARORES_WIND=1,RESPAWN_MODE_TOP=0,NA_SE_SY_ERROR=1;
int errors=0,nativeAllowed=0,nativeDraws=0,peerDraws=0,peerCounts=0;
struct {struct{int data=0;}respawn[1];struct{struct{struct{int triforcePiecesCollected=1;}randomizer;}data;}quest; }unused;
struct {struct{int data=0;}respawn[1];struct{struct{struct{struct{int triforcePiecesCollected=1;}randomizer;}data;}quest;}ship;}gSaveContext;
int ComboFw_HasPoint(int){return 0;}void Sfx_PlaySfxCentered(int){++errors;}
bool mmOwner=false;
namespace Ship{struct CrossRMRegistry{static bool Get(const char*){return mmOwner;}};}
struct CwItemDrawInfo { const char* dlists[2]={"__OTR__a","__OTR__b"};float scale=1;uint8_t primColorXlu[4]{};};
int OOT_DescribeMmSpinAttackDraw(CwItemDrawInfo*){return 1;}
const char* ComboInternRoutedPath(const std::string&){return "routed";}
void ComboDrawSpinAttackGi(PlayState*,const char*,const char*,float,const uint8_t*,const char*){++peerDraws;}
constexpr int GID_SWORD_KOKIRI=1;void GetItem_Draw(PlayState*,int){++nativeDraws;}
enum {RSK_RAINBOW_BRIDGE,RSK_RAINBOW_BRIDGE_TRIFORCE_COUNT,RSK_WINCON,RSK_WINCON_TRIFORCE_COUNT,RSK_GANONS_BOSS_KEY,RSK_GBK_TRIFORCE_COUNT,RSK_GANONS_SOUL,RSK_GANONS_SOUL_TRIFORCE_COUNT};
constexpr int RO_BRIDGE_TRIFORCE_PIECES=7,RO_WINCON_TRIFORCE_PIECES=8,RO_GANON_BOSS_KEY_TRIFORCE_PIECES=9,RO_GANONS_SOUL_TRIFORCE_PIECES=10,ITEM_CUSTOM=1;
struct Randomizer{uint8_t GetRandoSettingValue(int key){return key==RSK_WINCON?RO_WINCON_TRIFORCE_PIECES:(key==RSK_WINCON_TRIFORCE_COUNT?2:0);}};
struct OTRGlobals{static OTRGlobals* Instance;Randomizer* gRandomizer;};Randomizer rando;OTRGlobals globals{&rando};OTRGlobals* OTRGlobals::Instance=&globals;
int gComboGoalRequired=8;int CountPeer(){++peerCounts;return 10;}int(*gComboOtherTriforceCount)()=CountPeer;
#define TODO_TRANSLATE ""
struct CustomMessage{
 std::string text;
 CustomMessage()=default;CustomMessage(std::initializer_list<const char*> parts):text(*parts.begin()){}
 void InsertNumber(int){}void AutoFormat(int){}
 void Replace(const std::string&,const std::string&){}void Replace(const std::string&,const CustomMessage&){}
};
'''
cpp+=r'''
#include <cstring>
#include "menu/ComboIkanaShieldResources.h"
using u8=uint8_t;using s16=int16_t;
struct Gfx{const char* path=nullptr;};
Gfx nativeOpaque{"opaque"},nativeTranslucent{"translucent"};
int nativeLoads=0,shieldDraws=0,bindings=0,matrixDepth=0;
bool optionalMm=true,opaquePresent=true,translucentPresent=true;
bool MmAssets_IsAvailable(){return optionalMm;}
bool ResourceMgr_IsGiModelAvailableForGame(const char* owner,const char* path){
 assert(!std::strcmp(owner,"mm"));
 if(!std::strcmp(path,COMBO_IKANA_GI_OPA))return mmOwner&&opaquePresent;
 assert(!std::strcmp(path,COMBO_IKANA_GI_XLU));return mmOwner&&translucentPresent;
}
Gfx* MmAssets_LoadDisplayListGraphStrict(const char* path){
 ++nativeLoads;assert(optionalMm);
 if(!std::strcmp(path,COMBO_IKANA_GI_OPA))return opaquePresent?&nativeOpaque:nullptr;
 assert(!std::strcmp(path,COMBO_IKANA_GI_XLU));return translucentPresent?&nativeTranslucent:nullptr;
}
void MmAssets_EnsureStrictTextureBindings(){++bindings;}
void Matrix_Push(){++matrixDepth;}void Matrix_Pop(){assert(matrixDepth==1);--matrixDepth;}
constexpr int MTXMODE_APPLY=1;
void Matrix_RotateY(float,int mode){assert(mode==MTXMODE_APPLY);}
void Matrix_RotateZ(float value,int mode){assert(value==0.2f&&mode==MTXMODE_APPLY);}
void Matrix_RotateX(float value,int mode){assert(value==0.1f&&mode==MTXMODE_APPLY);}
int ResourceMgr_GetIkanaShieldGiPoseForGame(const char* owner,const char* path,float* tilt,float* roll){
 assert(!std::strcmp(owner,"mm")&&!std::strcmp(path,COMBO_IKANA_GI_XLU));*tilt=0.1f;*roll=0.2f;return 1;
}
void NeiGi_ReportBlackMarketModel(const char* owner,const char* opa,const char* xlu){
 assert(!std::strcmp(owner,"mm")&&!std::strcmp(opa,COMBO_IKANA_GI_OPA)&&!std::strcmp(xlu,COMBO_IKANA_GI_XLU));
}
void DrawMmGiModel(PlayState*,Gfx* opa,Gfx* xlu,bool shop){
 assert(shop&&matrixDepth==1&&opa&&xlu&&opa!=xlu);
 if(mmOwner){assert(!std::strcmp(opa->path,COMBO_IKANA_GI_OPA_MM)&&!std::strcmp(xlu->path,COMBO_IKANA_GI_XLU_MM));}
 else{assert(opa==&nativeOpaque&&xlu==&nativeTranslucent);}
 ++shieldDraws;
}
#define G_DL_OTR_FILEPATH 1
#define G_DL_PUSH 1
#define gDma1p(packet,command,resource,...) ((packet)->path=(resource))
#define gSPEndDisplayList(packet) ((packet)->path=nullptr)
'''
cpp+=extract('soh/soh/ResourceManagerHelpers.cpp','int ResourceMgr_IsIkanaShieldGiAvailable(')
cpp+=extract('soh/soh/Enhancements/randomizer/draw.cpp','void Randomizer_DrawExtShieldOfIkana(')
cpp+='void Spell(PlayState* play){int itemAction=PLAYER_IA_FARORES_WIND;bool isMedallionSpell=false;\n'+player[st:en]+'\n ++nativeAllowed;\n}\n'
cpp+=extract('soh/soh/Enhancements/randomizer/draw.cpp','extern "C" void Randomizer_DrawMmGreatSpinAttack(')
cpp+=extract('soh/soh/Enhancements/randomizer/Messages/ItemMessages.cpp','void BuildTriforcePieceMessage(')
cpp+=r'''
int main(){
 int failed=0;auto check=[&](bool v,const char* s){if(!v){++failed;std::cerr<<"FAIL "<<s<<"\n";}};
 PlayState p;Randomizer_DrawExtShieldOfIkana(&p,nullptr);
 check(shieldDraws==1&&nativeLoads>0&&bindings==1&&matrixDepth==0,"solo Shield of Ikana retains strict native MM pair and texture bindings");
 const int soloLoads=nativeLoads;
 Spell(&p);check(nativeAllowed==1 && errors==0,"solo Farore follows native restrictions");
 Randomizer_DrawMmGreatSpinAttack(&p,nullptr);check(nativeDraws==1 && peerDraws==0,"missing MM owner draws native fallback");
 CustomMessage msg;BuildTriforcePieceMessage(msg);check(msg.text.find("Courage")!=std::string::npos && peerCounts==0,"solo receipt ignores combined goal and peer count");
 gComboGameMode=COMBO_GAME_BOTH;Randomizer_DrawExtShieldOfIkana(&p,nullptr);
 check(shieldDraws==1&&nativeLoads==soloLoads,"combined shield without MM owner never substitutes an optional raw pointer");
 Spell(&p);check(errors==1,"combined Farore safety restriction retained");
 mmOwner=true;Randomizer_DrawMmGreatSpinAttack(&p,nullptr);check(peerDraws==1,"available MM owner retains native effect");
 BuildTriforcePieceMessage(msg);check(peerCounts==1 && msg.text.find("Courage")==std::string::npos,"combined receipt retained");
 Randomizer_DrawExtShieldOfIkana(&p,nullptr);
 check(shieldDraws==2&&nativeLoads==soloLoads&&matrixDepth==0,"combined owner uses deferred native passes without optional raw loads");
 translucentPresent=false;Randomizer_DrawExtShieldOfIkana(&p,nullptr);
 check(shieldDraws==2&&nativeLoads==soloLoads,"incomplete combined pair stays unavailable");
 mmOwner=false;gComboGameMode=COMBO_GAME_OOT;optionalMm=false;
 Randomizer_DrawExtShieldOfIkana(&p,nullptr);
 check(shieldDraws==2&&nativeLoads==soloLoads,"missing optional archive stays unavailable without loading");
 optionalMm=true;Randomizer_DrawExtShieldOfIkana(&p,nullptr);
 check(shieldDraws==2,"incomplete optional pair is never drawn");
 translucentPresent=true;Randomizer_DrawExtShieldOfIkana(&p,nullptr);
 check(shieldDraws==3&&bindings==2&&matrixDepth==0,"late optional pair availability retries and restores matrix scope");
 return failed?1:0;
}
'''
with tempfile.TemporaryDirectory(prefix='oot-compatibility-') as tmp:
 p=Path(tmp);(p/'test.cpp').write_text(cpp)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-DCOMBO_BUILD','-I'+str(ROOT/'combo'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS native Farore, Triforce receipt isolation and optional MM draw fallback')
