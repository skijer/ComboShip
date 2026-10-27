#include "combo/render/SpinAttackAccents.h"
#include "combo/render/SpinAttackStyleResources.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

int main() {
  CwSpinAccentParticle p[24], other[24];
  assert(CwSpinAccents_Build(0, 1, 0, 1, p, 24) == 0);
  assert(CwSpinAccents_Build(255, 1, 0, 1, p, 24) == 0);
  assert(CwSpinAccents_Build(1, 1, 0, 0, p, 24) == 0);
  assert(CwSpinAccents_Build(1, 1, NAN, 1, p, 24) == 0);
  assert(CwSpinAccents_Build(1, 1, 0, 1, nullptr, 24) == 0);
  for (unsigned element = 1; element <= 3; ++element) {
    assert(CwSpinAccents_Build(element, 0, 0.2f, 1, p, 24) == 12);
    assert(CwSpinAccents_Build(element, 1, 0.2f, 1, p, 24) == 24);
    assert(CwSpinAccents_Build(element, 1, 0.2f, 1, other, 24) == 24);
    assert(std::memcmp(p, other, sizeof(p)) == 0);
    CwSpinAccents_Build(element, 1, 0.7f, 1, other, 24);
    assert(std::memcmp(p, other, sizeof(p)) != 0);
    CwSpinAccents_Build(element, 1, 0.2f, 0.25f, other, 24);
    for (unsigned i = 0; i < 24; ++i) {
      assert(std::isfinite(p[i].x) && std::isfinite(p[i].y) &&
             std::isfinite(p[i].z));
      assert(std::hypot(p[i].x, p[i].z) < 2400.0f &&
             std::fabs(p[i].y) < 1600.0f);
      assert(other[i].a < p[i].a && p[i].size > 0);
    }
    assert(CwSpinAccents_Build(element, 1, 0.2f, 1, p, 3) == 3);
  }
  for (int style = 1; style < 8; ++style) {
    for (int great = 0; great <= 1; ++great) {
      const auto resources =
          CwSpinStyle_Resources((CwSpinStyle)style, 0, great);
      assert(resources.charge && resources.disk && resources.cylinder);
      assert(resources.burstTex[0] && resources.burstTex[1]);
    }
  }
  assert(CwSpinStyle_Resources(CW_SPIN_STYLE_NATIVE, 0, 0).disk == nullptr);
  for (unsigned element = 1; element <= 3; ++element) {
    const auto material =
        CwSpinStyle_Resources(CW_SPIN_STYLE_NATIVE, element, 0);
    assert(material.disk == nullptr && material.burstTex[0] &&
           material.chargeTex[0]);
  }
  std::puts("PASS bounded deterministic elemental accents and complete "
            "eight-style recipes");
}
