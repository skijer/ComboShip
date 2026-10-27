#include "combo/render/SpinAttackStyle.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>

// Regressions caught: an imbue overriding a manual style, visual preferences
// changing element ownership, and invalid saved values indexing resource
// tables.
int main() {
  const auto fire = CwSpinStyle_Resolve(0, 1, 1, 1);
  assert(fire.style == CW_SPIN_STYLE_NATIVE &&
         fire.nativeMaterialElement == 1 && fire.accentElement == 1);
  const auto arcaneIce = CwSpinStyle_Resolve(5, 1, 2, 1);
  assert(arcaneIce.style == CW_SPIN_STYLE_ARCANE &&
         arcaneIce.nativeMaterialElement == 0 && arcaneIce.accentElement == 2);
  for (int style = 0; style < 8; ++style) {
    auto none = CwSpinStyle_Resolve(style, 1, 0, 1);
    assert(none.style == style && none.nativeMaterialElement == 0 &&
           none.accentElement == 0);
    for (int element = 1; element <= 3; ++element) {
      auto disabled = CwSpinStyle_Resolve(style, 0, element, 1);
      assert(disabled.style == style && disabled.nativeMaterialElement == 0 &&
             disabled.accentElement == 0);
      auto enabled = CwSpinStyle_Resolve(style, 1, element, 1);
      assert(enabled.style == style && enabled.accentElement == element);
      assert(enabled.nativeMaterialElement == (style == 0 ? element : 0));
      auto excluded = CwSpinStyle_Resolve(style, 1, element, 0);
      assert(excluded.style == 0 && excluded.nativeMaterialElement == 0 &&
             excluded.accentElement == 0);
    }
  }
  for (int invalid : {-100, -1, 8, 255, 2147483647}) {
    auto fallback = CwSpinStyle_Resolve(invalid, 1, 3, 1);
    assert(fallback.style == 0 && fallback.nativeMaterialElement == 3 &&
           fallback.accentElement == 3);
  }
  auto invalidElement = CwSpinStyle_Resolve(7, 1, 255, 1);
  assert(invalidElement.style == 7 &&
         invalidElement.nativeMaterialElement == 0 &&
         invalidElement.accentElement == 0);
  std::puts("PASS independent eight-style policy, elemental accents, invalid "
            "preferences and eligibility");
}
