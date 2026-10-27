#include "entrances/RegionalPassages.h"
#include <cassert>
#include <cmath>
#include <cstdio>
using namespace ComboRegional;
struct World {
  bool wet = false, exit = false, dynamic = false, broken = false,
       occupied = false, obstruction = false, lowCeiling = false,
       disconnected = false, floorGap = false;
  bool Floor(Point p, float expected, float *y) {
    if (floorGap && p.z > 20 && p.z < 35)
      return false;
    if (wet || exit || dynamic || std::fabs(expected) > 1 ||
        (broken && p.x > 45))
      return false;
    *y = 0;
    return true;
  }
  bool Wall(Point a, Point b, Point *hit, Point *normal) {
    if (broken && a.y > 100)
      return false;
    const float planes[2] = {disconnected ? 100.f : 200.f,
                             disconnected ? 500.f : 200.f};
    float nearest = 2;
    for (float z : planes) {
      if ((a.z - z) * (b.z - z) >= 0)
        continue;
      const float t = (z - a.z) / (b.z - a.z);
      if (disconnected && z == 100 && std::fabs(a.x + (b.x - a.x) * t) > 80)
        continue;
      if (t < nearest) {
        nearest = t;
        *hit = {a.x + (b.x - a.x) * t, a.y, z};
        *normal = {0, 0, -1};
      }
    }
    return nearest <= 1;
  }
  bool Clear(Point a, Point b) {
    Point hit{}, normal{};
    if (lowCeiling && std::fmin(a.y, b.y) < 40 && std::fmax(a.y, b.y) > 40)
      return false;
    return !obstruction && !Wall(a, b, &hit, &normal);
  }
  bool Occupied(Point, float) { return occupied; }
  bool NearWater(Point) { return wet; }
};
int main() {
  World world;
  Pose result;
  assert(FindMouth(world, {{0, 0, 0}, 0}, &result));
  assert(std::fabs(result.pos.z - 198) < .01f && result.pos.y == 0);
  assert(result.forward.z < -.99f);
  assert(!FindMouth(world, {{0, 0, 0}, 0}, &result, true));
  assert(InMouth(result, {0, 0, 178}) && !InMouth(result, {90, 0, 178}));
  for (int bad = 0; bad < 8; ++bad) {
    World rejected;
    bool *fields[] = {&rejected.wet,        &rejected.exit,
                      &rejected.dynamic,    &rejected.broken,
                      &rejected.occupied,   &rejected.obstruction,
                      &rejected.lowCeiling, &rejected.floorGap};
    *fields[bad] = true;
    assert(!FindMouth(rejected, {{0, 0, 0}, 0}, &result));
  }
  World separated;
  separated.disconnected = true;
  assert(!FindMouth(separated, {{0, 0, 0}, 0}, &result));
  for (int route = 0; route < 8; ++route) {
    assert(ComboRegionalPassages_IsRoute(route) == (route >= 5));
    assert(ComboPassages_IsRoute(route) == (route >= 1));
    assert(ComboLostWoods_IsForestRoute(route) == (route >= 1 && route <= 4));
  }
  Origin good{5, 0xE4, 0, {{0, 0, 0}, 0}};
  assert(ValidOrigin(good, 0x53));
  good.pose.pos.x = NAN;
  assert(!ValidOrigin(good, 0x53));
  good = {6, 0x3CC, 0, {{100, 0, 100}, 0}};
  assert(ValidOrigin(good, 0x57));
  assert(!ValidOrigin(good, 0x09));
  good = {COMBO_PORTAL_SNOWHEAD, 0x3D4, 0, {{2150, 400, 0}, -16384}};
  assert(ValidOrigin(good, 0x59));
  assert(!ValidOrigin(good, 0x09));
  puts("PASS regional geometry: native-wall seating, footprint, "
       "dry/static/non-exit floor, occupancy and unchanged forest policy");
}
