#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define REQUIRE(c)                                                             \
  do {                                                                         \
    if (!(c)) {                                                                \
      fprintf(stderr, "FAIL finish core line %d: %s\n", __LINE__, #c);         \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)
#if __has_include("../../combo/gameplay/elemental_sword_finish_core.h")
#include "../../combo/gameplay/elemental_sword_finish_core.h"

static SwordFinishCore core;
static SwordFinishShape shape;
static void Setup(void) {
  memset(&core, 0, sizeof(core));
  memset(&shape, 0, sizeof(shape));
  shape.count = 2;
  shape.origin = (SwordFinishPoint){10, 20, 30, 0};
  shape.points[0] = (SwordFinishPoint){10, 30, 30, 8};
  shape.points[1] = (SwordFinishPoint){12, 45, 32, 10};
  SwordFinishCore_Context(&core, 1, 2, 3, 100, 0, 1);
}
static int Effects(void) {
  int result = 0;
  for (int i = 0; i < SWORD_FINISH_EFFECT_CAPACITY; ++i)
    result += core.effects[i].life != 0;
  return result;
}
static void Hit(uintptr_t key, int element, int native) {
  SwordFinishCore_Hit(&core, key, element, native, 3, &shape);
}
static void Attribution(void) {
  for (int element = 1; element <= 3; ++element) {
    Setup();
    Hit(7, element, 0);
    SwordFinishCore_Begin(&core, 7, 3);
    SwordFinishCore_End(&core, 7, 0, &shape.origin);
    REQUIRE(Effects() == 1 && core.effects[0].element == element);
    REQUIRE(!memcmp(&core.effects[0].shape, &shape, sizeof(shape)));
    memset(&shape, 0xAB,
           sizeof(shape)); // No retained actor/collider/shape storage.
    REQUIRE(core.effects[0].shape.points[0].x == 10);
    SwordFinishCore_Fatal(&core, 7, NULL);
    REQUIRE(Effects() == 1);
  }
  Setup();
  Hit(7, 2, 0);
  SwordFinishCore_Begin(&core, 7, 4);
  SwordFinishCore_End(&core, 7, 1, NULL);
  SwordFinishCore_Context(&core, 1, 2, 3, 101, 0, 1);
  SwordFinishCore_Begin(&core, 7, 1);
  SwordFinishCore_End(&core, 7, 0, NULL);
  REQUIRE(Effects() ==
          0); // An earlier nonfatal elemental hit grants no later credit.
  Setup();
  Hit(7, 3, 0);
  Hit(7, 0, 0);
  SwordFinishCore_Begin(&core, 7, 3);
  SwordFinishCore_End(&core, 7, 0, NULL);
  REQUIRE(Effects() ==
          0); // Ambiguous simultaneous foreign damage is conservative.
  Setup();
  Hit(7, 0, 0);
  Hit(7, 3, 0);
  SwordFinishCore_Begin(&core, 7, 3);
  SwordFinishCore_Fatal(&core, 7, NULL);
  REQUIRE(Effects() == 0);
  Setup();
  Hit(7, 1, 1);
  SwordFinishCore_Begin(&core, 7, 3);
  SwordFinishCore_Fatal(&core, 7, NULL);
  REQUIRE(Effects() == 0);
  Setup();
  Hit(7, 2, 0);
  SwordFinishCore_Fatal(&core, 7, NULL);
  REQUIRE(Effects() ==
          0); // Unload/teardown Kill outside actor consumption is not a kill.
  Setup();
  Hit(7, 2, 0);
  SwordFinishCore_Begin(&core, 7, 3);
  SwordFinishCore_End(&core, 7, 3, NULL);
  REQUIRE(Effects() == 0); // Soul/armor rejection.
  Setup();
  Hit(7, 2, 0);
  SwordFinishCore_Begin(&core, 7, 3);
  SwordFinishPoint moved = {15, 25, 35, 0};
  SwordFinishCore_Fatal(&core, 7, &moved);
  REQUIRE(Effects() == 1 && core.effects[0].shape.points[0].x == 15);
}
static void Lifecycle(void) {
  Setup();
  SwordFinishCore_Forget(&core, 0);
  REQUIRE(core.hitCount == 0);
  Setup();
  Hit(7, 3, 0);
  SwordFinishCore_Begin(&core, 7, 3);
  SwordFinishCore_Fatal(&core, 7, NULL);
  unsigned life = core.effects[0].life;
  SwordFinishCore_Context(&core, 1, 2, 3, 101, 1, 1);
  REQUIRE(core.effects[0].life == life);
  SwordFinishCore_Context(&core, 1, 2, 3, 102, 0, 1);
  REQUIRE(core.effects[0].life == life - 1);
  SwordFinishCore_Context(&core, 1, 2, 3, 102, 0, 1);
  REQUIRE(core.effects[0].life == life - 1);
  for (int i = 103; i < 200; ++i)
    SwordFinishCore_Context(&core, 1, 2, 3, i, 0, 1);
  REQUIRE(Effects() == 0);
  Setup();
  Hit(7, 2, 0);
  SwordFinishCore_Begin(&core, 7, 3);
  SwordFinishCore_Forget(&core, 7);
  SwordFinishCore_End(&core, 7, 0, NULL);
  REQUIRE(Effects() == 0);
  Hit(7, 1, 0);
  SwordFinishCore_Begin(&core, 7, 3);
  SwordFinishCore_Fatal(&core, 7, NULL);
  REQUIRE(Effects() ==
          1); // Same address reused only after a fresh owned collision.
  SwordFinishCore_Context(&core, 1, 2, 4, 101, 0, 1);
  REQUIRE(Effects() == 0);
  Setup();
  Hit(7, 2, 0);
  SwordFinishCore_Begin(&core, 7, 3);
  SwordFinishCore_Context(&core, 1, 2, 3, 101, 1, 1);
  SwordFinishCore_Fatal(&core, 7, NULL);
  REQUIRE(Effects() == 0);
  Setup();
  Hit(7, 2, 0);
  SwordFinishCore_Begin(&core, 7, 3);
  SwordFinishCore_Fatal(&core, 7, NULL);
  SwordFinishCore_Context(&core, 1, 2, 3, 99, 0, 1);
  REQUIRE(Effects() == 0);
  Setup();
  Hit(7, 2, 0);
  SwordFinishCore_Begin(&core, 7, 3);
  SwordFinishCore_Fatal(&core, 7, NULL);
  SwordFinishCore_Context(&core, 1, 2, 3, 101, 0, 0);
  REQUIRE(Effects() == 0);
  Setup();
  for (int key = 1; key <= SWORD_FINISH_HIT_CAPACITY + 20; ++key)
    Hit(key, 3, 0);
  SwordFinishCore_Begin(&core, 1, 3);
  SwordFinishCore_Fatal(&core, 1, NULL);
  REQUIRE(Effects() ==
          1); // Capacity pressure never steals another actor's identity.
}
static void NativeIceKeepsDeathAudioWithoutDuplicateMesh(void) {
  Setup();
  Hit(7, 2, 1);
  SwordFinishCore_Begin(&core, 7, 3);
  SwordFinishPoint moved = {15, 25, 35, 0};
  SwordFinishCore_End(&core, 7, 0, &moved);
  REQUIRE(Effects() == 1 && core.effects[0].element == 2);
  REQUIRE(core.effects[0].shape.count == 0);
  REQUIRE(core.effects[0].shape.origin.x == 15 &&
          core.effects[0].shape.origin.y == 25 &&
          core.effects[0].shape.origin.z == 35);
  memset(&shape, 0xDE, sizeof(shape));
  REQUIRE(core.effects[0].shape.origin.x == 15);
}
static void NativeIceMaterialOutlivesHitWithoutOwningActorMemory(void) {
#ifdef SWORD_FINISH_HAS_ICE_MATERIAL
  Setup();
  REQUIRE(!SwordFinishCore_UseIceMaterial(&core, 7));
  SwordFinishCore_SetIceMaterial(&core, 7, 1);
  Hit(7, 2, 0);
  SwordFinishCore_Begin(&core, 7, 3);
  SwordFinishCore_End(&core, 7, 1, NULL);
  REQUIRE(!SwordFinishCore_Find(&core, 7));
  for (int frame = 101; frame <= 137; ++frame)
    SwordFinishCore_Context(&core, 1, 2, 3, frame, 0, 1);
  REQUIRE(SwordFinishCore_UseIceMaterial(&core, 7)); // Delayed native 36-frame ice spawns.
  for (int frame = 138; frame <= 197; ++frame)
    SwordFinishCore_Context(&core, 1, 2, 3, frame, 1, 1);
  REQUIRE(SwordFinishCore_UseIceMaterial(&core, 7)); // Pause does not age provenance.
  for (int frame = 198; frame <= 326; ++frame)
    SwordFinishCore_Context(&core, 1, 2, 3, frame, 0, 1);
  REQUIRE(!SwordFinishCore_UseIceMaterial(&core, 7)); // Provenance remains bounded.
  SwordFinishCore_SetIceMaterial(&core, 7, 1);
  REQUIRE(SwordFinishCore_UseIceMaterial(&core, 7));
  SwordFinishCore_SetIceMaterial(&core, 8, 1);
  SwordFinishCore_SetIceMaterial(&core, 7, 0);
  REQUIRE(!SwordFinishCore_UseIceMaterial(&core, 7));
  REQUIRE(SwordFinishCore_UseIceMaterial(&core, 8));
  SwordFinishCore_SetIceMaterial(&core, 7, 1);
  SwordFinishCore_Context(&core, 1, 2, 4, 327, 0, 1);
  REQUIRE(!SwordFinishCore_UseIceMaterial(&core, 7));
#else
  REQUIRE(!"accepted ice material provenance is missing");
#endif
}
static void SeparateParts(void) {
#ifdef SWORD_FINISH_HAS_PARTS
  Setup();
  Hit(7, 2, 0);
  SwordFinishCore_Begin(&core, 7, 1);
  SwordFinishShape part = shape;
  part.count = 1;
  part.points[0].x = 88;
  SwordFinishCore_Part(&core, 7, 0, &part);
  part.points[0].x = 99;
  SwordFinishCore_Part(&core, 7, 1, &part);
  SwordFinishCore_Part(&core, 7, 1, &part);
  REQUIRE(Effects() == 2);
  REQUIRE(core.effects[0].shape.count == 1 &&
          core.effects[0].shape.points[0].x == 88);
  REQUIRE(core.effects[1].shape.count == 1 &&
          core.effects[1].shape.points[0].x == 99);
  SwordFinishCore_Forget(&core, 7);
  SwordFinishCore_Part(&core, 7, 2, &part);
  REQUIRE(Effects() == 2);
#else
  REQUIRE(!"individual enemy-part finish attribution is missing");
#endif
}
int main(void) {
  Attribution();
  Lifecycle();
  NativeIceKeepsDeathAudioWithoutDuplicateMesh();
  NativeIceMaterialOutlivesHitWithoutOwningActorMemory();
  SeparateParts();
  puts("PASS production finish core: owned fatal attribution, native dedup, "
       "copied anchors and lifecycle");
}
#else
int main(void) { REQUIRE(!"production elemental finish core is missing"); }
#endif
