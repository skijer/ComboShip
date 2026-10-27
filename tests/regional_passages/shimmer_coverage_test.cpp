#include "actors/RegionalPassagePresentation.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

struct Point2 {
  float x, y;
};
float Cross(Point2 a, Point2 b, Point2 c) {
  return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}
std::vector<Point2> Hull(std::vector<Point2> points) {
  std::sort(points.begin(), points.end(), [](Point2 a, Point2 b) {
    return a.x < b.x || (a.x == b.x && a.y < b.y);
  });
  points.erase(
      std::unique(points.begin(), points.end(),
                  [](Point2 a, Point2 b) { return a.x == b.x && a.y == b.y; }),
      points.end());
  std::vector<Point2> result;
  for (auto p : points) {
    while (result.size() >= 2 &&
           Cross(result[result.size() - 2], result.back(), p) <= 0)
      result.pop_back();
    result.push_back(p);
  }
  const size_t lower = result.size();
  for (auto i = points.rbegin() + 1; i != points.rend(); ++i) {
    while (result.size() > lower &&
           Cross(result[result.size() - 2], result.back(), *i) <= 0)
      result.pop_back();
    result.push_back(*i);
  }
  result.pop_back();
  return result;
}
bool Inside(const std::vector<Point2> &hull, NeiGi::Point p) {
  for (size_t i = 0; i < hull.size(); ++i)
    if (Cross(hull[i], hull[(i + 1) % hull.size()], {p.x, p.y}) < -.01f)
      return false;
  return true;
}
NeiGi::Point Packed(NeiGi::Point p) {
  return {std::round(p.x * 16) / 16, std::round(p.y * 16) / 16,
          std::round(p.z * 16) / 16};
}

std::vector<Point2> LakeOpening() {
  std::vector<Point2> points;
  // Derive the hollow's cross-section from its actual render quads, without
  // calling the roof-height helper used by the shimmer implementation.
  for (const auto &q : ComboLakeHylia::BuildGeometry().solid) {
    const ComboLakeHylia::Point vertices[] = {q.a, q.b, q.c, q.d};
    unsigned onBack = 0;
    for (auto p : vertices)
      onBack += p.x == ComboLakeHylia::kBackX;
    if (!onBack || onBack == 4)
      continue; // The rectangular back cap includes hidden roof corners.
    for (auto p : vertices)
      if (p.x == ComboLakeHylia::kBackX)
        points.push_back(
            {ComboLakeHylia::kMouth.z - p.z, p.y - ComboLakeHylia::kMouth.y});
  }
  return Hull(points);
}
std::vector<Point2> SnowheadOpening() {
  std::vector<Point2> points;
  using namespace ComboFountainSnowhead;
  // Independently intersect the authored floor triangles with the shimmer
  // plane. A doorway-width constant would let a narrow beacon test itself.
  const float plane = kMouth.x - kShimmerFront;
  for (const auto &triangle : ComboFountainValley::kTriangles) {
    if (!triangle.floor)
      continue;
    const ComboFountainValley::Point vertices[] = {
        triangle.a, triangle.b, triangle.c};
    for (unsigned i = 0; i < 3; ++i) {
      const auto a = vertices[i], b = vertices[(i + 1) % 3];
      if (a.x == b.x || plane < std::min(a.x, b.x) ||
          plane > std::max(a.x, b.x))
        continue;
      const float along = (plane - a.x) / (b.x - a.x);
      const float across = a.z + along * (b.z - a.z);
      points.push_back({across, a.y + along * (b.y - a.y) - kMouth.y});
      points.push_back({across, kOpeningHeight});
    }
  }
  return Hull(points);
}
std::vector<Point2> MmOpening() {
  std::vector<Point2> points;
  for (auto v : RegionalPassagePresentation::BuildShell())
    if (!v.shade)
      points.push_back({std::round(v.p.x), std::round(v.p.y)});
  return Hull(points);
}

// Native object_haka/gGravestoneStoneDL bounds after Bg_Haka's 0.1 scale.
// An enclosing box deliberately makes this proof stricter than the sloped mesh:
// complete source twinkles must stay outside the stone without a depth bypass.
bool InNativeGrave(NeiGi::Point p, float pulled) {
  return std::fabs(p.x) < 34.6f && p.y >= 0 && p.y <= 48 &&
         p.z > -36 - pulled && p.z < 36 - pulled;
}

int main() {
  const NeiGi::Basis cameras[] = {
      {},
      {{0, 0, -1}, {0, 1, 0}, {1, 0, 0}},
      {{1, 0, 0}, {0, .70710678f, .70710678f}, {0, -.70710678f, .70710678f}},
  };
  unsigned failures = 0;
  for (const auto &camera : cameras) {
    for (uint32_t frame = 0; frame < 180; ++frame) {
      const auto mesh =
          RegionalPassagePresentation::GraveShimmer(frame, camera);
      assert(mesh.count == 1440);
      unsigned embedded = 0, visibleCrown = 0, externalSides = 0,
               pulledCrown = 0;
      for (size_t i = 0; i < mesh.count; ++i) {
        const auto p = Packed(mesh.vertices[i].p);
        embedded += InNativeGrave(p, 0);
        if (i % 72 == 0) {
          visibleCrown += p.y > 54;
          externalSides +=
              p.y < 48 && (std::fabs(p.x) > 40 || std::fabs(p.z) > 42);
          pulledCrown += p.y > 54 && !InNativeGrave(p, 60);
        }
      }
      if (embedded || visibleCrown < 8 || externalSides < 8 ||
          pulledCrown < 8) {
        if (!failures)
          std::fprintf(stderr,
                       "FAIL grave beacon at tick%u: embedded=%u crown=%u "
                       "sides=%u pulled-crown=%u\n",
                       frame, embedded, visibleCrown, externalSides,
                       pulledCrown);
        ++failures;
      }
    }
  }
  assert(!failures && "closed and pulled grave must beckon using source art "
                      "outside the native stone");
  puts("PASS grave beacon: all180 ticks, three cameras, complete native-AABB "
       "clearance and open/closed crown");
  struct Face {
    RegionalPassagePresentation::Host host;
    int route;
    std::vector<Point2> boundary;
    float cap;
  };
  using Host = RegionalPassagePresentation::Host;
  const Face faces[] = {
      {Host::Oot, 6, LakeOpening(),
       ComboLakeHylia::kBackX - ComboLakeHylia::kMouth.x},
      {Host::Oot, 7, SnowheadOpening(), 0},
      {Host::Mm, 5, MmOpening(), -1},
      {Host::Mm, 6, MmOpening(), -1},
      {Host::Mm, 7, MmOpening(), -1},
  };
  for (const auto &face : faces) {
    assert(face.boundary.size() >= 4);
    for (const auto &camera : cameras) {
      for (uint32_t frame = 0; frame < 180; ++frame) {
        const auto original =
            RegionalPassagePresentation::Shimmer(frame, face.route, camera);
        const auto fitted = RegionalPassagePresentation::Shimmer(
            frame, face.host, face.route, camera);
        assert(fitted.count == 1440);
        if (face.host == Host::Oot && face.route == 7) {
          std::vector<float> columns;
          for (size_t first = 0; first < fitted.count; first += 72)
            columns.push_back(fitted.vertices[first].p.x);
          std::sort(columns.begin(), columns.end());
          const auto banks = std::minmax_element(
              face.boundary.begin(), face.boundary.end(),
              [](Point2 a, Point2 b) { return a.x < b.x; });
          if (columns.front() > banks.first->x + 20 ||
              columns.back() < banks.second->x - 20) {
            std::fprintf(stderr,
                         "FAIL valley-wide shimmer: centers %g..%g, "
                         "actual banks %g..%g\n", columns.front(),
                         columns.back(), banks.first->x, banks.second->x);
            assert(false && "shimmer must reach both valley banks");
          }
          for (size_t i = 1; i < columns.size(); ++i)
            assert(columns[i] - columns[i - 1] < 80 &&
                   "valley-wide shimmer must not leave large horizontal gaps");
        }
        for (size_t i = 0; i < fitted.count; ++i) {
          const auto p = Packed(fitted.vertices[i].p);
          bool contained = Inside(face.boundary, p);
          if (face.host == Host::Oot && face.route == 7) {
            float y;
            contained =
                ComboFountainValley::FloorAt(
                    ComboFountainSnowhead::kMouth.x - p.z, p.x, &y) &&
                p.y + ComboFountainSnowhead::kMouth.y >= y - .01f &&
                p.y <= ComboFountainSnowhead::kOpeningHeight;
          }
          if (!contained || p.z <= face.cap) {
            std::fprintf(stderr,
                         "FAIL native face containment: host%d route%d tick%u "
                         "vertex%zu at %g,%g,%g\n",
                         int(face.host), face.route, frame, i, p.x, p.y, p.z);
            assert(false && "complete packed twinkle must stay inside actual "
                            "native face and in front of cap");
          }
          const auto &a = original.vertices[i];
          const auto &b = fitted.vertices[i];
          const size_t first = i / 72 * 72;
          const auto oldOffset = a.p - original.vertices[first].p;
          const auto newOffset = b.p - fitted.vertices[first].p;
          assert(std::fabs(oldOffset.x - newOffset.x) < .00002f &&
                 std::fabs(oldOffset.y - newOffset.y) < .00002f &&
                 std::fabs(oldOffset.z - newOffset.z) < .00002f);
          assert(a.rgb == b.rgb && a.alpha == b.alpha && a.u == b.u &&
                 a.v == b.v);
        }
      }
    }
  }
  puts("PASS regional fitting: all180 ticks, three cameras, five actual mesh "
       "faces, complete packed-art containment and unchanged NEI art");
}
