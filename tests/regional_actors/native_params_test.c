#include "2s2h/Enhancements/Gameplay/ComboRegionalActors.h"
#include <assert.h>
int main(void) {
  for (unsigned p = 0; p < 65536; p++) {
    int stal = (p == 0x7E0F || p == 0x7E1F || p == 0x7E2F || p == 0x7E3F ||
                p == 0x7E4F);
    int mikau = (p == 0x7D01 || p == 0x7D11),
        ghost = (p == 0x7D02 || p == 0x7D12);
    assert(ComboRegionalActors_IsPlaced(ACTOR_EN_RAIL_SKB, p) == stal);
    assert(ComboRegionalActors_IsPlaced(ACTOR_EN_ZOG, p) == mikau);
    assert(ComboRegionalActors_IsPlaced(ACTOR_EN_GG, p) == ghost);
    assert(!ComboRegionalActors_IsPlaced(ACTOR_EN_GG2, p));
    assert(ComboRegionalActors_ObjectId(ACTOR_EN_GG, p, OBJECT_GG) ==
           (ghost ? GAMEPLAY_KEEP : OBJECT_GG));
  }
}
