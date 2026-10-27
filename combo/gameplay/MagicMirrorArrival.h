#ifndef COMBO_MAGIC_MIRROR_ARRIVAL_H
#define COMBO_MAGIC_MIRROR_ARRIVAL_H

struct PlayState;
#ifdef __cplusplus
extern "C" {
#endif
// Queue only after the destination save has loaded on a cross-game handoff.
void MagicMirror_QueueArrival(void);
// Called immediately before/after Actor_InitContext, before native camera init.
void MagicMirror_PrepareArrival(struct PlayState* play);
void MagicMirror_FinishArrival(struct PlayState* play);
#ifdef __cplusplus
}
#endif
#endif
