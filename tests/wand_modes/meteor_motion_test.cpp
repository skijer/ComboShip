// Floor queries and actor allocation are boundaries; EnBom_Move remains native.
#include "global.h"
#include "mods/items/helpers/target_select_helper.h"
#include "overlays/actors/ovl_En_Bom/z_en_bom.h"
#include <cmath>
#include <iostream>

f32 gSfxDefaultFreqAndVolScale=1;
s8 gSfxDefaultReverb=0;
static f32 sActorMovementScale=1.0f;
f32 gChampionSlowFactor=1.0f;
u8 TimeCtl_IsActorExempt(Actor*) { return 1; }
static EnBom spawned;
static bool floorAvailable=true;
f32 Math_SinS(s16 yaw) { return std::sin(yaw*M_PI/32768.0); }
f32 Math_CosS(s16 yaw) { return std::cos(yaw*M_PI/32768.0); }
s16 Math_Atan2S_XY(f32 y,f32 x) { return std::atan2(x,y)*32768.0/M_PI; }
void Math_ApproachF(f32*,f32,f32,f32) {}
void Math_ApproachS(s16*,s16,s16,s16) {}
s32 Math_ScaledStepToS(s16*,s16,s16) { return 0; }
FloorType SurfaceType_GetFloorType(CollisionContext*,CollisionPoly*,s32) { return FLOOR_TYPE_0; }
FloorEffect SurfaceType_GetFloorEffect(CollisionContext*,CollisionPoly*,s32) { return FLOOR_EFFECT_0; }
void Actor_GetSlopeDirection(CollisionPoly*,Vec3f* normal,s16* yaw) { *normal={}; *yaw=0; }
s32 Actor_HasParent(Actor* actor,PlayState*) { return actor->parent!=nullptr; }
s32 Actor_OfferCarry(Actor*,PlayState*) { return 0; }
void Actor_PlaySfx(Actor*,u16) {}
void Audio_PlaySoundGeneral(u16,Vec3f*,u8,f32*,f32*,s8*) {}
Actor* TargetSelect_FindNearest(PlayState*,const u8*,s32,TargetSelectFilter,Vec3f*,f32) { return nullptr; }
void EnBom_WaitForRelease(EnBom*,PlayState*) {}
void EnBom_Move(EnBom*,PlayState*);
void NativeBombUpdate(Actor* actor,PlayState* play) {
    EnBom* bomb=(EnBom*)actor;
    if(bomb->timer==0) { actor->params=BOMB_TYPE_EXPLOSION; return; }
    --bomb->timer; actor->gravity=-1.2f;
    EnBom_Move(bomb,play);
    const bool grounded=!!(actor->bgCheckFlags&BGCHECKFLAG_GROUND);
    actor->bgCheckFlags&=~(BGCHECKFLAG_GROUND|BGCHECKFLAG_GROUND_TOUCH);
    actor->floorHeight=floorAvailable?0.0f:BGCHECK_Y_MIN;
    if(floorAvailable && actor->world.pos.y<=0) {
        actor->world.pos.y=0; actor->bgCheckFlags|=BGCHECKFLAG_GROUND;
        if(!grounded) actor->bgCheckFlags|=BGCHECKFLAG_GROUND_TOUCH;
    }
}
static void WandMeteor_TintDraw(Actor*,PlayState*) {}
Actor* Actor_Spawn(ActorContext*,PlayState*,s16,f32 x,f32 y,f32 z,s16,s16 yaw,s16,s32 params) {
    spawned={}; Actor* actor=&spawned.actor;
    actor->world.pos={x,y,z}; actor->world.rot.y=yaw; actor->params=params;
    actor->terminalVelocity=-20; actor->floorHeight=BGCHECK_Y_MIN;
    actor->update=NativeBombUpdate; actor->draw=WandMeteor_TintDraw;
    return actor;
}

// PRODUCTION_BODIES

static bool check(bool condition,const char* message) {
    if(!condition) std::cerr << "FAIL Meteor: " << message << '\n';
    return condition;
}
int main() {
    PlayState play{}; Player player{};
    if(!check(WandMeteor_Cast(&player,&play),"spawn must succeed")) return 1;
    Actor* actor=&spawned.actor;
    f32 peaks[4]{};
    for(int i=0;i<64;++i) {
        actor->update(actor,&play);
        if(actor->world.pos.y>peaks[i/16]) peaks[i/16]=actor->world.pos.y;
    }
    if(!check(peaks[0]>40 && peaks[0]>peaks[1] && peaks[1]>peaks[2] && peaks[2]>peaks[3] && peaks[3]>20,
              "the four travelling hops must decay gradually")) return 1;
    if(!check(actor->speed>0 && actor->world.pos.z>400,"four hops must keep moving through native floor friction")) return 1;
    for(int i=64;i<160;++i) actor->update(actor,&play);
    if(!check(actor->speed>0 && actor->world.pos.z>1000,"bomb must travel until its fuse expires")) return 1;

    WandMeteor_Cast(&player,&play); actor=&spawned.actor;
    actor->update(actor,&play); actor->wallYaw=(s16)0x8000; actor->bgCheckFlags|=BGCHECKFLAG_WALL;
    const f32 before=actor->world.pos.z;
    actor->update(actor,&play);
    if(!check(actor->world.pos.z<before && actor->speed<9 && actor->params==BOMB_TYPE_BODY,
              "native wall reflection must survive before the fourth hop")) return 1;
    METEOR_HOP_TIME(actor)=64; actor->bgCheckFlags|=BGCHECKFLAG_WALL;
    actor->update(actor,&play);
    if(!check(actor->params==BOMB_TYPE_EXPLOSION,"wall contact after four hops must detonate")) return 1;

    WandMeteor_Cast(&player,&play); actor=&spawned.actor; floorAvailable=false;
    actor->world.pos.y=100; actor->update(actor,&play); actor->update(actor,&play);
    if(!check(actor->world.pos.y<100 && actor->velocity.y<0,"a pit must hand the bomb back to gravity")) return 1;
    floorAvailable=true; WandMeteor_Cast(&player,&play); actor=&spawned.actor;
    actor->parent=&player.actor; actor->world.pos={0,50,0}; actor->floorHeight=0;
    actor->update(actor,&play);
    if(!check(actor->world.pos.y==50,"carried bomb must leave the hands in control")) return 1;
    std::cout << "PASS Meteor: native movement, four hops, full fuse travel, wall reflection, detonation, pit, carry\n";
}
