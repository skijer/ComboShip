#include "combo/gameplay/SpinAttackState.h"
#include <cassert>
#include <cstdio>

int main() {
  CwSpinAttackState s;
  CwSpinAttack_Init(&s, 6, 1, 1, 10, 20, 3);
  CwSpinAttack_Preview(&s, 1, 1, 0);
  assert(s.style == CW_SPIN_STYLE_ASTRAL && s.element == 1 && !s.released);
  CwSpinAttack_Preview(&s, 2, 1, 1);
  assert(s.element == 1);
  CwSpinAttack_Preview(&s, 2, 1, 0);
  assert(s.element == 2);
  assert(CwSpinAttack_Release(&s, 2, 1, 0x40000000, 0x800, 0x1000, 0x2000) ==
         0x40001000);
  CwSpinAttack_Preview(&s, 3, 1, 0);
  assert(s.element == 2 && s.style == CW_SPIN_STYLE_ASTRAL);
  assert(CwSpinAttack_Original(&s, 0x40001000) == 0x40000000);
  assert(CwSpinAttack_Original(&s, 0x1000) == 0x1000);
  assert(CwSpinAttack_Original(&s, 0x40002000) == 0x40002000);
  assert(CwSpinAttack_Release(&s, 3, 1, 1, 2, 4, 8) == 0x40001000);
  CwSpinAttack_Init(&s, 7, 0, 1, 10, 20, 3);
  assert(CwSpinAttack_Release(&s, 3, 1, 0x01000000, 0x800, 0x1000, 0x2000) ==
         0x01002000);
  const auto visual =
      CwSpinStyle_Resolve(s.style, s.effectsEnabled, s.element, s.eligible);
  assert(visual.style == CW_SPIN_STYLE_DARK_ENERGY &&
         visual.accentElement == 0);
  CwSpinAttack_Init(&s, 4, 1, 1, 10, 20, 3);
  assert(CwSpinAttack_Release(&s, 1, 0, 0x01000000, 0x800, 0x1000, 0x2000) ==
         0x01000000);
  assert(!s.eligible && s.element == 0);
  CwSpinAttack_Reset(&s);
  assert(!s.eligible && !s.released && s.owner == 0 && s.context == 0);
  assert(CwSpinAttack_Original(&s, 0x01000800) == 0x01000800);
  std::puts("PASS spin preference snapshots, live charge preview, fixed "
            "release element and exact flag recovery");
}
