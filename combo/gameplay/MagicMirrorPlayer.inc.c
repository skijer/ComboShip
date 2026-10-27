// Included after each engine's native player implementation. This borrows only
// Farore's casting motion: no spell actor, sparkle mesh, magic cost or warp point.
#include "gameplay/MagicMirrorPlayer.h"

void MagicMirrorPlayer_StopMotion(Player* player) {
#ifdef COMBO_MAGIC_MIRROR_MM
    player->speedXZ = player->actor.speed = 0.0f;
#else
    player->linearVelocity = player->actor.speedXZ = 0.0f;
#endif
    player->actor.velocity.x = player->actor.velocity.z = 0.0f;
    player->yaw = player->actor.shape.rot.y;
}

static void MagicMirrorPlayer_Action(Player* player, PlayState* play) {
    MagicMirrorPlayer_StopMotion(player);
#ifdef COMBO_MAGIC_MIRROR_MM
    player->stateFlags2 |= PLAYER_STATE2_20 | PLAYER_STATE2_40;
    if (PlayerAnimation_Update(play, &player->skelAnime))
#else
    player->stateFlags2 |= PLAYER_STATE2_DISABLE_ROTATION_Z_TARGET | PLAYER_STATE2_DISABLE_ROTATION_ALWAYS;
    if (LinkAnimation_Update(play, &player->skelAnime))
#endif
        player->av2.actionVar2 = 1;
}

static void MagicMirrorPlayer_Start(PlayState* play, Player* player) {
    player->afterPutAwayFunc = NULL;
    // Runtime owns activation/departure audio; animation and put-away stay silent.
#ifdef COMBO_MAGIC_MIRROR_MM
    Player_SetAction(play, player, MagicMirrorPlayer_Action, 0);
    player->stateFlags1 |= PLAYER_STATE1_20000000;
    PlayerAnimation_Change(play, &player->skelAnime, &gPlayerAnim_link_magic_kaze1, 1.25f, 0.0f,
                           Animation_GetLastFrame(&gPlayerAnim_link_magic_kaze1), ANIMMODE_ONCE, -4.0f);
#else
    Player_SetupAction(play, player, MagicMirrorPlayer_Action, 0);
    player->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
    LinkAnimation_Change(play, &player->skelAnime, &gPlayerAnim_link_magic_kaze1, 1.25f, 0.0f,
                         Animation_GetLastFrame(&gPlayerAnim_link_magic_kaze1), ANIMMODE_ONCE, -4.0f);
#endif
    MagicMirrorPlayer_StopMotion(player);
    // Native form controllers yield to the cast instead of holding its action.
#ifdef COMBO_MAGIC_MIRROR_MM
    player->stateFlags3 &= ~PLAYER_STATE3_4;
#else
    player->stateFlags3 &= ~PLAYER_STATE3_PAUSE_ACTION_FUNC;
#endif
}

int MagicMirrorPlayer_State(const Player* player) {
    if (!player)
        return MAGIC_MIRROR_CAST_NONE;
    if (player->actionFunc == MagicMirrorPlayer_Action)
        return player->av2.actionVar2 ? MAGIC_MIRROR_CAST_READY : MAGIC_MIRROR_CAST_PLAYING;
    if (player->actionFunc == Player_Action_WaitForPutAway && player->afterPutAwayFunc == MagicMirrorPlayer_Start)
        return MAGIC_MIRROR_CAST_PUT_AWAY;
    return MAGIC_MIRROR_CAST_NONE;
}

int MagicMirrorPlayer_Begin(PlayState* play, Player* player) {
    if (!play || !player || MagicMirrorPlayer_State(player))
        return 0;
    MagicMirrorPlayer_StopMotion(player);
    if (!Player_SetupWaitForPutAway(play, player, MagicMirrorPlayer_Start))
        MagicMirrorPlayer_Start(play, player);
#ifdef COMBO_MAGIC_MIRROR_MM
    player->stateFlags1 |= PLAYER_STATE1_20000000;
#else
    player->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
#endif
#ifdef COMBO_MAGIC_MIRROR_MM
    player->stateFlags3 &= ~PLAYER_STATE3_4;
#else
    player->stateFlags3 &= ~PLAYER_STATE3_PAUSE_ACTION_FUNC;
#endif
    return 1;
}

void MagicMirrorPlayer_End(PlayState* play, Player* player) {
    if (!MagicMirrorPlayer_State(player))
        return; // Damage, a cutscene or another native action now owns the actor.
    player->afterPutAwayFunc = NULL;
#ifdef COMBO_MAGIC_MIRROR_MM
    player->stateFlags2 &= ~(PLAYER_STATE2_20 | PLAYER_STATE2_40);
    func_80839E74(player, play);
#else
    player->stateFlags2 &= ~(PLAYER_STATE2_DISABLE_ROTATION_Z_TARGET | PLAYER_STATE2_DISABLE_ROTATION_ALWAYS);
    func_80839FFC(player, play);
    Player_AnimChangeOnceMorph(play, player, Player_GetIdleAnim(player));
#endif
}

void MagicMirrorPlayer_Arrive(PlayState* play, Player* player) {
    // The copied spawn already placed the native player (and Mario controller).
    // Leave the native doorway walk before its first update/camera setup.
    MagicMirrorPlayer_StopMotion(player);
#ifdef COMBO_MAGIC_MIRROR_MM
    func_80839E74(player, play);
    Audio_PlaySfx(NA_SE_PL_MAGIC_WIND_WARP);
#else
    func_80839FFC(player, play);
    Player_AnimChangeOnceMorph(play, player, Player_GetIdleAnim(player));
    Sfx_PlaySfxCentered(NA_SE_PL_MAGIC_WIND_WARP);
#endif
}
