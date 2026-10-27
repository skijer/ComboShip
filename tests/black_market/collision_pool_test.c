#include "entrances/BlackMarketEntrance.h"
#include "fault.h"
#include "global.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Internal production entry points are absent from the public functions header.
s32 BgCheck_TryGetCustomMemsize(s32 sceneId, u32 *memSize);
StaticLookup *BgCheck_GetStaticLookup(CollisionContext *colCtx,
                                      StaticLookup *lookupTbl, Vec3f *pos);
f32 BgCheck_RaycastFloorStatic(StaticLookup *lookup, CollisionContext *colCtx,
                               u16 xpFlags, CollisionPoly **poly, Vec3f *pos,
                               u32 flags, f32 checkDist, f32 yIntersectMin,
                               Actor *actor);
s32 CollisionPoly_CheckYIntersect(CollisionPoly *poly, Vec3s *vtxList, f32 x,
                                  f32 z, f32 *yIntersect, f32 checkDist);

// Test boundary: isolate each arena allocation so sanitizers detect writes past
// the actual requested size. Collision construction and queries are production.
static void *allocations[16];
static size_t allocationCount;
#ifndef MARKET_REAL_ARENA
void *THA_AllocTailAlign(TwoHeadArena *arena, size_t size, uintptr_t mask) {
  assert(allocationCount < ARRAY_COUNT(allocations));
  void *result = calloc(1, size);
  assert(result);
  allocations[allocationCount++] = result;
  return result;
}
void *THA_AllocTailAlign16(TwoHeadArena *arena, size_t size) {
  return THA_AllocTailAlign(arena, size, ~(uintptr_t)15);
}
#endif
void Fault_AddHungupAndCrash(const char *file, s32 line) {
  fprintf(stderr, "Unexpected collision fault: %s:%d\n", file, line);
  abort();
}
void Fault_AddHungupAndCrashImpl(const char *first, const char *second) {
  fprintf(stderr, "Unexpected collision fault: %s %s\n", first, second);
  abort();
}

static u32 Read32(FILE *file) {
  unsigned char bytes[4];
  assert(fread(bytes, 1, 4, file) == 4);
  return bytes[0] | ((u32)bytes[1] << 8) | ((u32)bytes[2] << 16) |
         ((u32)bytes[3] << 24);
}
static void ReadCollision(const char *path, CollisionHeader *header) {
  FILE *file = fopen(path, "rb");
  assert(file);
  assert(fseek(file, 64, SEEK_SET) == 0);
  assert(fread(&header->minBounds, sizeof(Vec3s), 1, file) == 1);
  assert(fread(&header->maxBounds, sizeof(Vec3s), 1, file) == 1);
  header->numVertices = Read32(file);
  header->vtxList = calloc(header->numVertices, sizeof(Vec3s));
  assert(fread(header->vtxList, sizeof(Vec3s), header->numVertices, file) ==
         header->numVertices);
  header->numPolygons = Read32(file);
  header->polyList = calloc(header->numPolygons, sizeof(CollisionPoly));
  assert(fread(header->polyList, sizeof(CollisionPoly), header->numPolygons,
               file) == header->numPolygons);
  assert(header->numVertices == 4573 && header->numPolygons == 7823);
  fclose(file);
}

static void ReleaseAllocations(void) {
  for (size_t i = 0; i < allocationCount; ++i) {
    free(allocations[i]);
  }
  allocationCount = 0;
}

static void CheckLookup(CollisionContext *context) {
  unsigned cells = context->subdivAmount.x * context->subdivAmount.y *
                   context->subdivAmount.z;
  unsigned walked = 0;
  unsigned char *seen = calloc(context->polyNodes.count, 1);
  unsigned char *polys = calloc(context->colHeader->numPolygons, 1);
  for (unsigned i = 0; i < cells; ++i) {
    u16 heads[] = {context->lookupTbl[i].floor.head,
                   context->lookupTbl[i].wall.head,
                   context->lookupTbl[i].ceiling.head};
    for (unsigned list = 0; list < ARRAY_COUNT(heads); ++list) {
      for (u16 node = heads[list]; node != SS_NULL;
           node = context->polyNodes.tbl[node].next) {
        assert(node < context->polyNodes.count && !seen[node]);
        seen[node] = 1;
        s16 poly = context->polyNodes.tbl[node].polyId;
        assert(poly >= 0 && poly < context->colHeader->numPolygons);
        polys[poly] = 1;
        ++walked;
      }
    }
  }
  assert(walked == context->polyNodes.count);
  for (unsigned i = 0; i < context->colHeader->numPolygons; ++i) {
    assert(polys[i]);
  }
  free(seen);
  free(polys);
}

static void CheckFloors(CollisionContext *context) {
  unsigned hits = 0;
  for (int x = -150; x <= 150; x += 50) {
    for (int z = -250; z <= 400; z += 50) {
      Vec3f point = {x, 250.0f, z};
      CollisionPoly *result = NULL;
      float actual = BGCHECK_Y_MIN;
      // Match the native raycast caller: each list covers one Y cell.
      // Search downward until the first hit, then compare to all triangles.
      for (Vec3f cell = point; cell.y >= context->minBounds.y;
           cell.y -= context->subdivLength.y) {
        StaticLookup *lookup =
            BgCheck_GetStaticLookup(context, context->lookupTbl, &cell);
        if (lookup) {
          actual =
              BgCheck_RaycastFloorStatic(lookup, context, 0, &result, &point, 4,
                                         1.0f, BGCHECK_Y_MIN, NULL);
          if (actual > BGCHECK_Y_MIN) {
            break;
          }
        }
      }
      float expected = BGCHECK_Y_MIN;
      for (unsigned i = 0; i < context->colHeader->numPolygons; ++i) {
        CollisionPoly *poly = &context->colHeader->polyList[i];
        float y;
        if (poly->normal.y > COLPOLY_SNORMAL(0.5f) &&
            !COLPOLY_VIA_FLAG_TEST(poly->flags_vIB, 4) &&
            CollisionPoly_CheckYIntersect(poly, context->colHeader->vtxList,
                                          point.x, point.z, &y, 1.0f) &&
            y < point.y && y > expected) {
          expected = y;
        }
      }
      if (fabsf(actual - expected) >= 0.01f) {
        fprintf(stderr, "Floor mismatch at %d,%d: indexed=%f linear=%f\n", x, z,
                actual, expected);
        abort();
      }
      hits += actual > BGCHECK_Y_MIN;
    }
  }
  assert(hits > 10);
}

int main(int argc, char **argv) {
  assert(argc == 2);
  PlayState *play = calloc(1, sizeof(PlayState));
  CollisionHeader header = {0};
  ReadCollision(argv[1], &header);
  u32 memory;
  assert(BgCheck_TryGetCustomMemsize(SCENE_00KEIKOKU, &memory) &&
         memory == 0xC800);
  assert(!BgCheck_TryGetCustomMemsize(SCENE_20SICHITAI, &memory));
#ifndef COMBO_BUILD
  assert(!BgCheck_TryGetCustomMemsize(COMBO_BLACK_MARKET_SCENE, &memory));
  puts("PASS normal MM build retains native scene memory policy");
#else
#ifdef MARKET_REAL_ARENA
  void *arena = malloc(4 * 1024 * 1024);
  assert(arena);
#endif
  for (unsigned cycle = 0; cycle < 4; ++cycle) {
#ifdef MARKET_REAL_ARENA
    THA_Init(&play->state.tha, arena, 4 * 1024 * 1024);
#endif
    play->sceneId = COMBO_BLACK_MARKET_SCENE;
    BgCheck_Allocate(&play->colCtx, play, &header);
    assert(play->colCtx.polyNodes.count < play->colCtx.polyNodes.max);
    assert(play->colCtx.polyNodes.count == 34490);
    assert(play->colCtx.subdivAmount.x == 16 &&
           play->colCtx.subdivAmount.y == 4 &&
           play->colCtx.subdivAmount.z == 16);
    CheckLookup(&play->colCtx);
    CheckFloors(&play->colCtx);
#ifdef MARKET_REAL_ARENA
    assert(!THA_IsCrash(&play->state.tha));
    assert(
        (uintptr_t)(play->colCtx.polyNodes.tbl + play->colCtx.polyNodes.max) <=
        (uintptr_t)play->colCtx.lookupTbl);
#endif
    ReleaseAllocations();
  }
  printf("PASS market collision: polygons=%u nodes=%u capacity=%u; four "
         "initializations, complete chains and floor queries\n",
         header.numPolygons, play->colCtx.polyNodes.count,
         play->colCtx.polyNodes.max);
#ifdef MARKET_REAL_ARENA
  free(arena);
  puts("PASS production two-head arena");
#endif
#endif
  free(header.vtxList);
  free(header.polyList);
  free(play);
  return 0;
}
