// Native engine calls are fixtures; every tested cast transition is the shared
// production MagicMirrorPlayer implementation used by both games.
#include "z64.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "gameplay/MagicMirrorPlayer.h"

constexpr u32 PLAYER_STATE1_IN_CUTSCENE = 1u << 29, PLAYER_STATE1_20000000 = 1u << 29;
constexpr u32 PLAYER_STATE2_DISABLE_ROTATION_Z_TARGET = 0x20, PLAYER_STATE2_DISABLE_ROTATION_ALWAYS = 0x40;
constexpr u32 PLAYER_STATE2_20 = 0x20, PLAYER_STATE2_40 = 0x40;
constexpr u32 PLAYER_STATE3_PAUSE_ACTION_FUNC = 4, PLAYER_STATE3_4 = 4;
constexpr int ANIMMODE_ONCE = 0;
constexpr u16 NA_SE_PL_MAGIC_WIND_WARP = 0x087B;
struct Animation {};
Animation gPlayerAnim_link_magic_kaze1;
struct SkelAnime { bool complete = false; };
struct Player {
    struct {
        float speed = 0, speedXZ = 0;
        struct { float x = 0, y = 0, z = 0; } velocity;
        struct { struct { s16 y = 0; } rot; } shape;
    } actor;
    float speedXZ = 0, linearVelocity = 0;
    s16 yaw = 0;
    u32 stateFlags1 = 0, stateFlags2 = 0, stateFlags3 = 0;
    struct { int actionVar2 = 0; } av2;
    SkelAnime skelAnime;
    void (*actionFunc)(Player*, PlayState*) = nullptr;
    void (*afterPutAwayFunc)(PlayState*, Player*) = nullptr;
};
static bool sPutAway = false;
static std::vector<u16> sSounds;
void Sfx_PlaySfxCentered(u16 sound) { sSounds.push_back(sound); }
void Audio_PlaySfx(u16 sound) { sSounds.push_back(sound); }
void Player_Action_WaitForPutAway(Player*, PlayState*) {}
static void Idle(Player*, PlayState*) {}
static void SetAction(Player* player, void (*action)(Player*, PlayState*)) {
    player->actionFunc = action;
    player->av2.actionVar2 = 0;
}
void Player_SetAction(PlayState*, Player* p, void (*action)(Player*, PlayState*), int) { SetAction(p, action); }
void Player_SetupAction(PlayState*, Player* p, void (*action)(Player*, PlayState*), int) { SetAction(p, action); }
int Player_SetupWaitForPutAway(PlayState*, Player* player, void (*next)(PlayState*, Player*)) {
    if (!sPutAway) return 0;
    SetAction(player, Player_Action_WaitForPutAway);
    player->afterPutAwayFunc = next;
    return 1;
}
int Animation_GetLastFrame(Animation*) { return 30; }
void PlayerAnimation_Change(PlayState*, SkelAnime* skel, Animation*, float, float, int, int, float) { skel->complete = false; }
void LinkAnimation_Change(PlayState*, SkelAnime* skel, Animation*, float, float, int, int, float) { skel->complete = false; }
int PlayerAnimation_Update(PlayState*, SkelAnime* skel) { return skel->complete; }
int LinkAnimation_Update(PlayState*, SkelAnime* skel) { return skel->complete; }
void func_80839E74(Player* p, PlayState*) { SetAction(p, Idle); p->stateFlags1 &= ~PLAYER_STATE1_20000000; }
void func_80839FFC(Player* p, PlayState*) { SetAction(p, Idle); p->stateFlags1 &= ~PLAYER_STATE1_IN_CUTSCENE; }
Animation* Player_GetIdleAnim(Player*) { return nullptr; }
void Player_AnimChangeOnceMorph(PlayState*, Player*, Animation*) {}
#include "gameplay/MagicMirrorPlayer.inc.c"

static void Check(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
int main() {
    PlayState play;
    Player player;
    Check(!MagicMirrorPlayer_Begin(nullptr, &player) && !MagicMirrorPlayer_Begin(&play, nullptr),
          "Rejected cast does not begin");
    Check(sSounds.empty(), "Rejected cast is silent");
    player.actor.velocity.x = 5;
    player.actor.velocity.z = 7;
    Check(MagicMirrorPlayer_Begin(&play, &player) == 1, "Eligible cast starts");
    Check(sSounds.empty(), "Animation start leaves activation and departure audio to the runtime");
    Check(player.actor.velocity.x == 0 && player.actor.velocity.z == 0, "Cast stops motion");
    Check(!MagicMirrorPlayer_Begin(&play, &player), "Repeated begin is rejected");
    for (int frame = 0; frame < 60; ++frame) player.actionFunc(&player, &play);
    Check(sSounds.empty(), "Casting update does not emit audio");
    player.skelAnime.complete = true;
    player.actionFunc(&player, &play);
    Check(MagicMirrorPlayer_State(&player) == MAGIC_MIRROR_CAST_READY, "Native animation completion commits readiness");
    Check(sSounds.empty(), "Readiness alone does not emit departure audio");
    MagicMirrorPlayer_End(&play, &player);
    Check(sSounds.empty(), "Cast cancellation/release is silent");

    sPutAway = true;
    Check(MagicMirrorPlayer_Begin(&play, &player), "Holstered cast can queue");
    Check(MagicMirrorPlayer_State(&player) == MAGIC_MIRROR_CAST_PUT_AWAY && sSounds.empty(),
          "Put-away wait does not announce a cast that has not started");
    MagicMirrorPlayer_End(&play, &player);
    Check(player.afterPutAwayFunc == nullptr && sSounds.empty(), "Cancelled put-away leaves no audio callback");
    Check(MagicMirrorPlayer_Begin(&play, &player), "Next put-away cast queues");
    player.afterPutAwayFunc(&play, &player);
    Check(MagicMirrorPlayer_State(&player) == MAGIC_MIRROR_CAST_PLAYING && sSounds.empty(),
          "Successful post-holster casting does not prematurely play the departure cue");
    for (int frame = 0; frame < 60; ++frame) player.actionFunc(&player, &play);
    Check(sSounds.empty(), "Post-holster update does not loop audio");
    MagicMirrorPlayer_End(&play, &player);
    MagicMirrorPlayer_Arrive(&play, &player);
    Check(sSounds.size() == 1 && sSounds.back() == NA_SE_PL_MAGIC_WIND_WARP,
          "Actual Mirror arrival has its own native Farore cue");
    std::puts("PASS: native cast lifecycle, silent animation/holster/cancel/repeat, one-shot Farore arrival");
}
