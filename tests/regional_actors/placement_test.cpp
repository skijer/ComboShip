#include "actors/RegionalActorPlacement.h"
#include <cassert>
#include <vector>
using namespace ComboRegionalActors;
struct World {
  struct Spawned {
    int family;
    Point p;
    bool live = true;
  };
  std::vector<Spawned> actors;
  int authored = 0, kills = 0;
  bool floor = true;
  bool Authored(int f, const State &) { return f == authored; }
  bool Live(unsigned handle) { return handle && actors[handle - 1].live; }
  void Kill(unsigned handle) {
    actors[handle - 1].live = false;
    ++kills;
  }
  bool Floor(Point *p) {
    if (!floor)
      return false;
    p->y = 321;
    return true;
  }
  unsigned Spawn(const Row &r, Point p, short) {
    actors.push_back({r.family, p});
    return actors.size();
  }
};
int main() {
  State state;
  World world;
  Update(state, world, false, 7, {1840, 280, 0}, -16384, false);
  assert(world.actors.empty());
  world.floor = false;
  Update(state, world, true, 7, {1840, 280, 0}, -16384, false);
  assert(world.actors.empty());
  world.floor = true;
  Update(state, world, true, 7, {1840, 280, 0}, -16384, false);
  assert(world.actors.size() == 1 && world.actors[0].family == 27);
  assert(world.actors[0].p.x == 1740 && world.actors[0].p.z == 150 &&
         world.actors[0].p.y == 333);
  Update(state, world, true, 7, {1840, 280, 0}, -16384, false);
  assert(world.actors.size() == 1);
  world.authored = 27;
  Update(state, world, true, 7, {1840, 280, 0}, -16384, false);
  assert(world.kills == 1);
  world.authored = 0;
  Update(state, world, true, 5, {0, 0, 0}, 0, false);
  assert(world.actors.size() == 3);
  assert(world.actors[1].p.x == -90 && world.actors[2].p.x == 90);
  world.authored = 25;
  Update(state, world, true, 5, {0, 0, 0}, 0, false);
  assert(world.kills == 3);
  Update(state, world, false, 5, {0, 0, 0}, 0, false);
  assert(state.route == 0);
  world.authored = 0;
  Update(state, world, true, 6, {0, 0, 0}, 0, true);
  assert(world.actors.size() == 5);
  assert(world.actors[3].family == 26 && world.actors[4].family == 22);
  assert(world.actors[3].p.x == 44 && world.actors[4].p.x == 64);
}
