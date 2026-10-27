#!/usr/bin/env python3
"""Narrow native-contract fixture; no renderer, audio mixer or game runtime.
Runs real extracted sound/context functions, Lizalfos stun and the ice draw branch.
The tiny fake engine records calls; symbolic SFX constants intentionally differ.
Usage after applying: python3 tests/elemental_sword_finish_native_test.py
--fixture-stdin accepts {"impl": "...", "core": "...", "stun": "..."} for no-checkout review.
"""
import json
from pathlib import Path
import subprocess
import sys
import tempfile

SOUND_PREFIX = r"""#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SWORD_ELEMENT_FIRE 1
#define SWORD_ELEMENT_ICE 2
#define SWORD_ELEMENT_LIGHT 3
#define SFX_FLAG 0x800
#define NA_SE_EV_ICE_FREEZE 0x280E
#define NA_SE_IT_EXPLOSION_ICE 0x183B
#define NA_SE_EN_GOMA_JR_FREEZE 0x30A4
#define NA_SE_EV_BURN_OUT 0x2891
#define NA_SE_EN_COMMON_DEADLIGHT 0x39F7
#define FINISH_SCENE(p) ((p)->sceneNum)
#define FINISH_DEBUG_PAUSE(p) ((p)->pauseCtx.debugState)
#define TRANS_TRIGGER_OFF 0
typedef struct {float x,y,z;} Vec3f;
typedef struct {int x;} MtxF;
typedef struct {uint32_t gameplayFrames; int sceneNum; struct {int state,debugState;} pauseCtx; struct {struct {int num;} curRoom;} roomCtx; int transitionTrigger; MtxF viewProjectionMtxF;} PlayState;
static unsigned requests, stops, oneShots, lastId;
static Vec3f* lastPos;
static void record(unsigned id, Vec3f* pos) { ++requests; lastId=id; lastPos=pos; }
void MmSfx_PlayAtPos(unsigned id,Vec3f* pos){record(id,pos);}
void MmSfx_StopAtPos(unsigned id,Vec3f* pos){++stops;lastId=id;lastPos=pos;}
void AudioSfx_StopByPosAndId(Vec3f* pos,unsigned id){++stops;lastId=id;lastPos=pos;}
void Audio_StopSfxByPosAndId(Vec3f* pos,unsigned id){++stops;lastId=id;lastPos=pos;}
void Audio_PlaySfx_AtPos(Vec3f* pos,unsigned id){record(id,pos);}
float gSfxDefaultFreqAndVolScale; int gSfxDefaultReverb;
void Audio_PlaySfxGeneral(unsigned id,Vec3f* pos,int token,float* freq,float* vol,int* reverb){record(id,pos);}
void SoundSource_PlaySfxAtFixedWorldPos(PlayState* p,Vec3f* pos,int life,unsigned id){++oneShots;lastId=id;}
void SkinMatrix_Vec3fMtxFMultXYZ(MtxF* m,Vec3f* in,Vec3f* out){*out=*in;out->z+=100;}
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#c);return 1;}}while(0)
"""
SOUND_MAIN = r"""int main(void) {
 PlayState play={0}; play.gameplayFrames=1; play.sceneNum=1;
 CHECK(DinFireSword_FinishContext(&play));
 SwordFinishEffect* ice=&sFinish.effects[0];
 ice->element=SWORD_ELEMENT_ICE; ice->life=24; ice->shape.origin=(SwordFinishPoint){1,2,3,0};
 DinFireSword_FinishPlayAllSounds(&play);
 CHECK(requests==1 && oneShots==0 && lastId==NA_SE_EV_ICE_FREEZE-SFX_FLAG);
 CHECK(lastPos==&sFinishSoundPos[0] && lastPos->z==103);
 DinFireSword_FinishPlayAllSounds(&play); CHECK(requests==1);
 ++play.gameplayFrames; CHECK(DinFireSword_FinishContext(&play));
 DinFireSword_FinishPlayAllSounds(&play); CHECK(requests==2);
 play.pauseCtx.state=1; CHECK(!DinFireSword_FinishContext(&play)); CHECK(stops==1 && !ice->sounded);
 DinFireSword_FinishPlayAllSounds(&play); CHECK(requests==2);
 play.pauseCtx.state=0; CHECK(DinFireSword_FinishContext(&play));
 DinFireSword_FinishPlayAllSounds(&play); CHECK(requests==3);
 ice->life=1; ++play.gameplayFrames; CHECK(DinFireSword_FinishContext(&play)); CHECK(!ice->life && stops==2);
 ice->element=SWORD_ELEMENT_ICE; ice->life=24; DinFireSword_FinishPlayAllSounds(&play); CHECK(requests==4);
 DinFireSword_FinishContext(NULL); CHECK(stops==3);
 CHECK(DinFireSword_FinishContext(&play));
 ice=&sFinish.effects[0];ice->element=SWORD_ELEMENT_ICE;ice->life=24;
 DinFireSword_FinishPlayAllSounds(&play);
 ++play.sceneNum;CHECK(DinFireSword_FinishContext(&play));
 CHECK(stops==4 && lastId==NA_SE_EV_ICE_FREEZE-SFX_FLAG && !sFinish.effects[0].life);
 CHECK(DinFireSword_FinishContext(&play));
 SwordFinishEffect* fire=&sFinish.effects[1]; fire->element=SWORD_ELEMENT_FIRE;fire->life=24;
 DinFireSword_FinishPlayAllSounds(&play); CHECK(oneShots==1 && lastId==NA_SE_EV_BURN_OUT);
 ++play.gameplayFrames; CHECK(DinFireSword_FinishContext(&play)); DinFireSword_FinishPlayAllSounds(&play); CHECK(oneShots==1);
 SwordFinishEffect* light=&sFinish.effects[2];light->element=SWORD_ELEMENT_LIGHT;light->life=24;
 DinFireSword_FinishPlayAllSounds(&play);CHECK(lastId==0x31F7);
 unsigned before=requests; DinFireSword_FinishPlayAllSounds(&play);CHECK(before==requests);
 ++play.gameplayFrames;CHECK(DinFireSword_FinishContext(&play));DinFireSword_FinishPlayAllSounds(&play);CHECK(requests==before+1);
 DinFireSword_FinishTeardown(&play); CHECK(stops==
 5
 );
 CHECK(DinFireSword_FinishContext(&play));
 for(unsigned i=0;i<40;++i){
#ifdef SWORD_FINISH_OOT
  sFinish.effects[i].element=SWORD_ELEMENT_LIGHT;
#else
  sFinish.effects[i].element=SWORD_ELEMENT_ICE;
#endif
  sFinish.effects[i].life=24;
 }
 ++play.gameplayFrames;CHECK(DinFireSword_FinishContext(&play));before=requests;
 DinFireSword_FinishPlayAllSounds(&play);CHECK(requests-before==32);
 DinFireSword_FinishPlayAllSounds(&play);CHECK(requests-before==32);
 ++play.gameplayFrames;CHECK(DinFireSword_FinishContext(&play));DinFireSword_FinishPlayAllSounds(&play);
 CHECK(sFinish.effects[39].sounded);
 puts("PASS host-native ice SFX routing, cadence/cleanup, fire/light preservation, request budget and fairness");
 return 0;
}"""
IMPACT_MAIN = r"""int main(void){
 PlayState play={0};play.sceneNum=1;
 const uint8_t orders[][2]={{0,2},{1,2},{3,2},{2,0},{2,1},{2,3},{2,2}};
 SwordFinishShape shape={0};shape.count=1;shape.origin=(SwordFinishPoint){1,2,3,0};
 for(unsigned order=0;order<sizeof(orders)/sizeof(orders[0]);++order){
  ++play.gameplayFrames;CHECK(DinFireSword_FinishContext(&play));oneShots=0;
  for(unsigned hit=0;hit<2;++hit){
   unsigned element=orders[order][hit];
   SwordFinishCore_Hit(&sFinish,7,element,0,1,&shape);
   DinFireSword_FinishIceImpact(&play,7,element,&shape);
  }
  CHECK(oneShots==1 && lastId==NA_SE_IT_EXPLOSION_ICE);
  DinFireSword_FinishIceImpact(&play,7,SWORD_ELEMENT_ICE,&shape);CHECK(oneShots==1);
  SwordFinishCore_Begin(&sFinish,7,2);SwordFinishCore_End(&sFinish,7,0,&shape.origin);
  CHECK(sFinish.effects[0].life==(order==6 ? 24 : 0));
  DinFireSword_FinishContext(NULL);
 }
 puts("PASS mixed-hit ice impact dedup independent of fatal attribution and hit order");return 0;
}"""
STUN_PREFIX = r"""#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#define BGCHECKFLAG_GROUND 1
#define ENZF_DMGEFF_ICE 15
#define ENZF_DMGEFF_STUN 1
#define ENZF_ACTION_STUNNED 16
#define DMG_ARROW_ICE 0x1000
#define NA_SE_EN_GOMA_JR_FREEZE 0x389E
typedef struct {struct {uint32_t dmgFlags;} toucher;} ColliderInfo;
typedef struct {struct {unsigned bgCheckFlags;struct {float y;} velocity;float speedXZ;} actor;
 int hopAnimIndex,damageEffect,iceTimer,action,skelAnime;
 struct {struct {ColliderInfo* acHitInfo;} info;} bodyCollider;} EnZf;
static unsigned sounds;static uint32_t original;
void* gPlayState;int gZfKnockedBackAnim;
uint32_t DinFireSword_OriginalDamageFlags(void* p,const ColliderInfo* hit){return original;}
void Audio_PlayActorSound2(void* actor,unsigned id){++sounds;}
void Animation_PlayOnceSetSpeed(void* anim,void* resource,float speed){}
void EnZf_Stunned(EnZf* actor,void* play){}
void EnZf_SetupAction(EnZf* actor,void(*action)(EnZf*,void*)){}
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#c);return 1;}}while(0)
"""
STUN_MAIN = r"""int main(void){
 EnZf actor={0};ColliderInfo hit={0};actor.bodyCollider.info.acHitInfo=&hit;
 actor.damageEffect=ENZF_DMGEFF_ICE;hit.toucher.dmgFlags=0x200|DMG_ARROW_ICE;original=0x200;
 EnZf_SetupStunned(&actor);CHECK(sounds==0 && actor.iceTimer==36 && actor.action==ENZF_ACTION_STUNNED);
 hit.toucher.dmgFlags=original=DMG_ARROW_ICE;
 EnZf_SetupStunned(&actor);CHECK(sounds==1); // Native ice arrows retain their own cue.
 actor.damageEffect=ENZF_DMGEFF_STUN;hit.toucher.dmgFlags=original=1;
 EnZf_SetupStunned(&actor);CHECK(sounds==2); // Real Deku nuts retain their own cue.
 actor.damageEffect=ENZF_DMGEFF_ICE;actor.bodyCollider.info.acHitInfo=NULL;
 EnZf_SetupStunned(&actor);CHECK(sounds==3);
 puts("PASS OoT Lizalfos sword-ice impact replaces shared nut cue; native arrows/nuts remain intact");return 0;
}"""
RENDER_PREFIX = r"""#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define SWORD_ELEMENT_ICE 2
#define MTXMODE_APPLY 1
#define G_MTX_NOPUSH 0
#define G_MTX_LOAD 0
#define G_MTX_MODELVIEW 0
typedef struct { float x,y,z; } Vec3f;
typedef struct {int n;} Gfx;
typedef int Mtx;
typedef struct {int n;} GraphicsContext;
typedef struct {uint32_t gameplayFrames;struct {GraphicsContext* gfxCtx;} state;struct {Vec3f eye;} view;} PlayState;
static int prim[4],env[4],rotXCalls,scrollChecks;
static float pitch;
static const char* displayLists[16];
static unsigned dlCount;
static Gfx commands[64],*head=commands;
#define POLY_XLU_DISP head
#define FINISH_MATRIX_TO(m) (m)
#define FINISH_ROTATE_X Matrix_RotateX
#define FINISH_ROTATE_Y Matrix_RotateY
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#c);exit(1);}}while(0)
void Matrix_Scale(float x,float y,float z,int mode){}
void Matrix_RotateX(float value,int mode){++rotXCalls;pitch=value;}
void Matrix_RotateY(float value,int mode){}
void gDPSetPrimColor(Gfx* g,int a,int b,int r,int gr,int bl,int al){prim[0]=r;prim[1]=gr;prim[2]=bl;prim[3]=al;}
void gDPSetEnvColor(Gfx* g,int r,int gr,int bl,int al){env[0]=r;env[1]=gr;env[2]=bl;env[3]=al;}
void gSPMatrix(Gfx* g,Mtx* m,int mode){}
void gSPSegment(Gfx* g,int slot,Gfx* dl){CHECK(slot==8);}
void gSPDisplayList(Gfx* g,const char* resource){CHECK(resource && dlCount<16);displayLists[dlCount++]=resource;}
void func_8002EB44(Vec3f* p,Vec3f* eye,Vec3f* light,GraphicsContext* g){CHECK(light->x==89.8f && light->z==89.8f);}
void func_800BCC68(Vec3f* p,PlayState* play){}
const char* gEffIceFragment2DL="native-oot-ice-clump";
const char* gEffIceFragment2MaterialDL="native-mm-ice-material";
const char* gEffIceFragment2ModelDL="native-mm-ice-model";
Gfx* SwordFinishIce_GetDisplayList(uint32_t frame){(void)frame;return NULL;}
Gfx* Gfx_TwoTexScrollEx(GraphicsContext* g,int a,int b,int c,int d,int e,int f,int h,int j,int k,int l,int m,int n,int o,int p){
 CHECK(a==0 && b==0 && d==32 && e==16 && f==1 && h==0 && k==64 && l==32 && m==0 && n==1 && o==0 && p==2);
 ++scrollChecks;return commands;
}
"""
RENDER_MAIN = r"""int main(void){
 PlayState playValue={0}; PlayState* play=&playValue; SwordFinishEffect effectValue={0};
 SwordFinishEffect* effect=&effectValue;effect->element=SWORD_ELEMENT_ICE;
 SwordFinishPoint pointValue={1,2,3,4};SwordFinishPoint* point=&pointValue;
 Mtx matrix;Mtx* allocatedMatrix=&matrix;float scale=1;uint8_t alpha=173;
 unsigned slot=5;
 for(unsigned i=0;i<7;++i){
 BODY
#ifdef SWORD_FINISH_OOT
 CHECK(prim[0]==150 && prim[1]==150 && prim[2]==150 && prim[3]==250);
 CHECK(env[0]==235 && env[1]==245 && env[2]==255 && env[3]==173);
 CHECK(rotXCalls==(int)i+1 && pitch>=-3.141593f && pitch<3.141593f);
#else
 CHECK(prim[0]==170 && prim[1]==255 && prim[2]==255 && prim[3]==255);
 CHECK(env[0]==200 && env[1]==200 && env[2]==255 && env[3]==173);
 CHECK(rotXCalls==0);
#endif
 }
 CHECK(scrollChecks==7);
#ifdef SWORD_FINISH_OOT
 CHECK(dlCount==7);
 for(unsigned j=0;j<dlCount;++j) CHECK(displayLists[j]==gEffIceFragment2DL);
#else
 CHECK(dlCount==14);
 for(unsigned j=0;j<dlCount;j+=2) CHECK(displayLists[j]==gEffIceFragment2MaterialDL && displayLists[j+1]==gEffIceFragment2ModelDL);
#endif
 puts("PASS host native ice palette/pitch contract and texture scroll dimensions");
 return 0;
}"""

def sound_body(source):
    return (source[source.index("static SwordFinishCore sFinish;"):
                   source.index("static SwordFinishPoint DinFireSword_FinishOrigin")] +
            source[source.index("void DinFireSword_FinishTeardown"):
                   source.index("// A finish is optional extra work")])

def ice_body(source):
    begin = source.index("if (effect->element == SWORD_ELEMENT_ICE) {")
    return source[begin:source.index(" else if (effect->element == SWORD_ELEMENT_FIRE)", begin)]

def function(source, name):
    begin = source.index("void " + name + "(")
    opening = source.index("{", begin)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[begin:end]


def run(impl, core, stun=None):
    sources = {
        "sound": SOUND_PREFIX + core + sound_body(impl) + SOUND_MAIN,
        "impact": SOUND_PREFIX + core + sound_body(impl) + function(impl, "DinFireSword_FinishIceImpact") + IMPACT_MAIN,
        "render": RENDER_PREFIX + core + RENDER_MAIN.replace("BODY", ice_body(impl)),
    }
    if stun:
        sources["stun"] = STUN_PREFIX + function(stun, "EnZf_SetupStunned") + STUN_MAIN
    failed = False
    with tempfile.TemporaryDirectory(prefix="sword-finish-native-") as temp:
        for name, source in sources.items():
            for host in ("MM", "OOT"):
                if name == "stun" and host != "OOT":
                    continue
                executable = str(Path(temp) / (name + "-" + host))
                command = ["cc", "-std=c99", "-x", "c", "-", "-o", executable]
                if host == "OOT":
                    command.append("-DSWORD_FINISH_OOT")
                compiled = subprocess.run(command, input=source, text=True, capture_output=True)
                if compiled.returncode:
                    print(name, host, "COMPILE ERROR", compiled.stderr)
                    failed = True
                    continue
                result = subprocess.run([executable], text=True, capture_output=True)
                print(name, host, result.stdout.strip() or result.stderr.strip())
                failed |= result.returncode != 0
    return int(failed)

if __name__ == "__main__":
    if "--fixture-stdin" in sys.argv:
        fixture = json.load(sys.stdin)
        raise SystemExit(run(fixture["impl"], fixture["core"], fixture.get("stun")))
    root = Path(__file__).resolve().parents[1]
    raise SystemExit(run(
        (root / "combo/gameplay/elemental_sword_finish_impl.h").read_text(),
        (root / "combo/gameplay/elemental_sword_finish_core.h").read_text(),
        (root / "soh/src/overlays/actors/ovl_En_Zf/z_en_zf.c").read_text()))
