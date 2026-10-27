// Characterize the exact native Woods of Mystery guide voice, independently
// of the SoH-side sound aliases and custom actor caller.
#include "global.h"
#include "overlays/actors/ovl_En_Mnk/z_en_mnk.h"
#include <assert.h>
#include <stdio.h>

#define MONKEY_FOLLOWPATH_RESULT_FOLLOWING 0

static unsigned calls;
f32 gFramerateDivisorThird = 1.0f;
void SkelAnime_AnimateFrame(SkelAnime *animation) { (void)animation; }

/* NATIVE_ANIMATION */
void Actor_PlaySfx(Actor *actor, u16 id) {
  assert(actor != NULL);
  assert(id == (calls % 2 == 0 ? 0x2900 : 0x28EF));
  ++calls;
}
s32 SkelAnime_Update(SkelAnime *animation) {
  return SkelAnime_LoopFull(animation);
}
s32 EnMnk_FollowPath(EnMnk *monkey, u16 flags) {
  (void)monkey;
  assert(flags == 0);
  return 0;
}
void Actor_Kill(Actor *actor) {
  (void)actor;
  assert(false);
}

/* NATIVE_GUIDE */

int main(void) {
  EnMnk monkey = {0};
  PlayState play = {0};
  /* The actual object_mnk_Anim_009FE0 resource has nine frames. The native
   * guide selects that loop through EnMnk_StartNextPath / SetAnim(1). */
  monkey.skelAnime.animLength = 9.0f;
  monkey.skelAnime.playSpeed = 1.0f;
  for (unsigned tick = 0; tick < 90; ++tick) {
    EnMnk_Monkey_GuideThroughWoods(&monkey, &play);
    assert(calls == 2 * (1 + tick / 9));
  }
  puts("PASS native MM Woods of Mystery guide continuously emits voice 0x2900 "
       "and footstep 0x28EF at frame 1 of each nine-frame walk cycle");
}
