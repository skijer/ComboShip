#include "entrances/IkanaGravePlacement.h"
#include <cassert>
#include <cstdio>
using namespace ComboRegional;
int main() {
  assert(IkanaGrave::Matches({22, 120, -336}, 0, 0, 1));
  for (float x : {-458.f, -338.f, -218.f, -98.f, -578.f})
    assert(!IkanaGrave::Matches({x, 120, -336}, 0, 0, 1));
  assert(!IkanaGrave::Matches({22, 120, -336}, 0, 1, 1));
  assert(!IkanaGrave::Matches({22, 120, -336}, 0, 0, 0));
  assert(!IkanaGrave::Matches({22, 120, -336}, -16384, 0, 1));
  assert(!IkanaGrave::Matches({22, 120, -335}, 0, 0, 1));
  assert(IkanaGrave::kPose.forward.z == 1);
  puts("PASS exact authored existing Ikana grave selector and neighboring "
       "controls");
}
