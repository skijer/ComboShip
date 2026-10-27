#ifndef COMBO_MAGIC_MIRROR_PLAYER_H
#define COMBO_MAGIC_MIRROR_PLAYER_H

struct PlayState;
struct Player;

enum {
    MAGIC_MIRROR_CAST_NONE,
    MAGIC_MIRROR_CAST_PUT_AWAY,
    MAGIC_MIRROR_CAST_PLAYING,
    MAGIC_MIRROR_CAST_READY,
};

#ifdef __cplusplus
extern "C" {
#endif
int MagicMirrorPlayer_Begin(struct PlayState* play, struct Player* player);
int MagicMirrorPlayer_State(const struct Player* player);
void MagicMirrorPlayer_End(struct PlayState* play, struct Player* player);
void MagicMirrorPlayer_Arrive(struct PlayState* play, struct Player* player);
void MagicMirrorPlayer_StopMotion(struct Player* player);
// Includes the input tick that dismisses or confirms the private choice box.
int MagicMirror_PromptOwnsPlayer(struct PlayState* play, const struct Player* player);
#ifdef __cplusplus
}
#endif
#endif
