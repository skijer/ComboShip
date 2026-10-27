#include "render/RegionalSnow.h"
#include <cassert>
#include <cmath>
int main() {
  using namespace RegionalSnow;
  for (unsigned i = 0; i < kCount; ++i) {
    auto a = Flake(60, i), b = Flake(61, i);
    if (a.y > 4)
      assert(b.y < a.y);
    assert(a.y >= 0 && a.y <= 220);
    assert(a.alpha >= 0 && a.alpha <= 1);
  }
  for (unsigned i = 0; i < kChillCount; ++i) {
    auto a = Chill(30, i), b = Chill(31, i);
    if (a.alpha > .1f && b.alpha > .1f) {
      assert(b.x < a.x && b.y > a.y);
    }
    assert(a.alpha >= 0 && a.alpha <= 1);
  }
  assert(Chill(0, 0).alpha == 0);
  assert(Opacity(0) == 1 && Opacity(700) == 0);
  assert(Opacity(450) > 0 && Opacity(450) < 1);
}
