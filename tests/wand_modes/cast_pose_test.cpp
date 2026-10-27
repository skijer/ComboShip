// Animation decoding and unrelated item actions are boundaries; the MM handoff stays native.
#include "global.h"
#include "mods/items/helpers/equip_helper.h"
#include "mods/extended_inventory.h"
#include "mods/extended_player.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>

PlayState* gPlayState;
f32 gSfxDefaultFreqAndVolScale = 1;
s8 gSfxDefaultReverb = 0;
ItemInputState nextInput{};
static u8 currentMode;
static bool magicAvailable = true, castAvailable = true, animationDone = false;
static int effects, spent, changes, copies;
static std::string lastClip;
u8 Wand_ModeCount() { return WAND_MODE_COUNT; }
u8 Wand_ModeAt(u8 index) { return index; }
u8 Wand_GetMode() { return currentMode; }
void Wand_SetMode(u8 mode) { currentMode = mode; }
void* Wand_ModeIcon(u8) { static char icon[] = "icon"; return icon; }
void ExtInv_RefreshButtonIconsForItem(PlayState*, u16) {}
void ItemInput_Update(ItemInputState* out, u8, Player*, PlayState*) { *out = nextInput; }
u8 ItemInput_IsBlocked(Player* p, PlayState*) { return !!(p->stateFlags1 & (PLAYER_STATE1_DAMAGED | PLAYER_STATE1_SHIELDING)); }
s32 ItemMagic_HasEnough(PlayState*, s16) { return magicAvailable; }
void ItemMagic_Consume(PlayState*, s16 amount) { spent += amount; }
u8 BoxMenu_IsOpen() { return 0; }
u8 BoxMenu_Open(PlayState*, const BoxMenuEntry*, s32, s32, u16, BoxMenuConfirmFn) { return 0; }
void Audio_PlaySoundGeneral(u16, Vec3f*, u8, f32*, f32*, s8*) {}
#define CAST_BOUNDARY(name) u8 name(Player*,PlayState*) { if (castAvailable) ++effects; return castAvailable; }
CAST_BOUNDARY(WandSand_Cast) CAST_BOUNDARY(WandWind_Cast) CAST_BOUNDARY(WandWater_Cast)
CAST_BOUNDARY(WandMeteor_Cast) CAST_BOUNDARY(WandStorm_Cast) CAST_BOUNDARY(WandShadow_Cast)
u8 WandSand_HoldElapsed(Player*, u8) { return 0; }
void WandSand_Forget() {} void WandWater_Forget() {} void WandShadow_Forget() {} void WandStorm_Forget() {}
void WandShadow_Tick(PlayState*) {} void WandStorm_Tick(PlayState*,Player*) {} void WandWind_Tick(PlayState*,Player*) {}
void WandWind_TickHover(Player*,u8) {}
s32 func_8083485C(Player*,PlayState*) { return 0; }
s16 Animation_GetLastFrame(void*) { return 12; }
void PlayerAnimation_Change(PlayState*,SkelAnime* skel,PlayerAnimationHeader* clip,f32,f32,f32,u8,f32) {
    ++changes; lastClip = reinterpret_cast<const char*>(clip);
    skel->jointTable[PLAYER_LIMB_R_FOREARM].x = 123;
}
s32 PlayerAnimation_Update(PlayState*,SkelAnime*) { return animationDone; }
bool Player_IsHoldingHookshot(Player*) { return false; }
bool Player_CanUpdateItems(Player*) { return false; }
void Player_UpdateItems(Player*,PlayState*) {}
s32 Player_SetAction(PlayState*,Player*,PlayerActionFunc,s32) { return 0; }
void Player_Action_HookshotFly(Player*,PlayState*) {} void Player_Action_64(Player*,PlayState*) {}
void Player_Anim_PlayOnce(PlayState*,Player*,PlayerAnimationHeader*) {}
void Player_AnimReplace_Setup(PlayState*,Player*,s32) {}
void func_8082DAD4(Player*) {} void Player_AnimSfx_PlayVoice(Player*,u16) {}
s16 gNeiHoverTimer;
#define IDLE_ANIM_NONE -1
s32 Player_CheckForIdleAnim(Player*) { return IDLE_ANIM_NONE; }
s32 Math_StepToF(f32* value,f32 target,f32) { *value=target; return true; }
void AnimTaskQueue_AddCopy(PlayState*,s32 count,Vec3s* dest,Vec3s* src) {
    ++copies; std::copy(src, src+count, dest);
}
void AnimTaskQueue_AddCopyUsingMap(PlayState*,s32 count,Vec3s* dest,Vec3s* src,u8* map) {
    ++copies; for(int i=0;i<count;++i) if(map[i]) dest[i]=src[i];
}
void AnimTaskQueue_AddCopyUsingMapInverted(PlayState*,s32 count,Vec3s* dest,Vec3s* src,u8* map) {
    for(int i=0;i<count;++i) if(!map[i]) dest[i]=src[i];
}
void AnimTaskQueue_AddInterp(PlayState*,s32,Vec3s*,Vec3s*,f32) {}

// PRODUCTION_BODIES

static bool check(bool condition,const char* message) {
    if(!condition) std::cerr << "FAIL cast pose: " << message << '\n';
    return condition;
}
int main() {
    const int costs[] = {2,0,2,3,6,3};
    PlayState play{}; Player player{}; Vec3s joints[PLAYER_LIMB_MAX]{}, upper[PLAYER_LIMB_MAX]{};
    gPlayState=&play;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first=&player.actor;
    for(u8 mode=0; mode<WAND_MODE_COUNT; ++mode) {
        ++play.sceneId; player={}; currentMode=mode; effects=spent=changes=copies=0;
        nextInput={}; nextInput.wasEquipped=1; animationDone=false; magicAvailable=castAvailable=true;
        player.skelAnime.limbCount=PLAYER_LIMB_MAX; player.skelAnime.jointTable=joints;
        player.skelAnimeUpper.jointTable=upper; player.upperActionFunc=Player_UpperAction_ElementalWand;
        player.speedXZ=1.0f; // native handoff must copy the upper body while Link walks
        Wand_TickInput(&play,&player);
        player.heldItemAction=(PlayerItemAction)PLAYER_IA_ELEMENTAL_WAND;
        Player_InitElementalWandIA(&play,&player); Wand_TickInput(&play,&player);
        nextInput.isPressed=1; Wand_TickInput(&play,&player);
        if(!check(effects==1 && spent==costs[mode],"press must dispatch and charge exactly once")) return 1;
        if(!check(changes==1,"a successful cast must start a native upper-body animation")) return 1;
        if(!check(Player_UpdateUpperBody(&player,&play) && copies==1 && joints[PLAYER_LIMB_R_FOREARM].x==123,
                  "MM must copy the pose into the drawn skeleton")) return 1;
        Wand_TickInput(&play,&player);
        if(!check(effects==1,"pressing during a pose must not recast")) return 1;
        animationDone=true; nextInput.isPressed=0;
        Player_UpdateUpperBody(&player,&play); Player_UpdateUpperBody(&player,&play);
        if(!check(!Player_UpdateUpperBody(&player,&play),"finished pose must release the upper body")) return 1;
        if(!check(changes==((mode==WAND_MODE_METEOR || mode==WAND_MODE_STORM)?1:2),
                  "summoning rods must finish with a swing")) return 1;
        nextInput.isPressed=1; magicAvailable=false;
        if(mode!=WAND_MODE_TORNADO) { Wand_TickInput(&play,&player); if(!check(effects==1,"empty magic must reject the cast")) return 1; }
        magicAvailable=true; castAvailable=false; Wand_TickInput(&play,&player);
        if(!check(effects==1 && spent==costs[mode],"failed handler must not animate or spend magic")) return 1;
        castAvailable=true; nextInput.isPressed=1; Wand_TickInput(&play,&player);
        player.heldItemAction=PLAYER_IA_NONE; Wand_TickInput(&play,&player);
        player.heldItemAction=(PlayerItemAction)PLAYER_IA_ELEMENTAL_WAND;
        Player_InitElementalWandIA(&play,&player);
        if(!check(!Player_UpdateUpperBody(&player,&play),"stowing must cancel the previous pose")) return 1;
    }
    std::cout << "PASS MM cast poses: six modes, native moving upper-body handoff, completion, magic, stow\n";
}
