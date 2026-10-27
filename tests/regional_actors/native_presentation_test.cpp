extern "C" {
#include "2s2h/Enhancements/Gameplay/ComboRegionalActorPresentation.h"
}
#include <cassert>
#include <cstring>
static bool available = true;
static const char *missing;
static unsigned killed, normalInit, flexInit, opaDraw, xluDraw, freed, updates;
static Actor *expected;
static Gfx commands[128];
void FrameInterpolation_RecordOpenChild(const void *, int) {}
void FrameInterpolation_RecordCloseChild() {}
extern "C" {
uint8_t ResourceMgr_FileExists(const char *path) {
  return available && (!missing || !strstr(path, missing));
}
void ResourceMgr_UnregisterSkeleton(SkelAnime *) { ++freed; }
void Actor_Kill(Actor *a) {
  ++killed;
  a->update = nullptr;
  a->draw = nullptr;
}
void Actor_ChangeCategory(PlayState *, ActorContext *, Actor *a, u8 category) {
  a->category = category;
}
void Actor_SetScale(Actor *a, f32 s) { a->scale = {s, s, s}; }
void ActorShape_Init(ActorShape *a, f32 y, ActorShadowFunc, f32) {
  a->yOffset = y;
}
void Actor_SetFocus(Actor *a, f32 y) { a->focus.pos = a->world.pos; a->focus.pos.y += y; }
void Matrix_MultVec3f(Vec3f* offset, Vec3f* target) {
  *target = {12, 17 + .01f * offset->y, 67};
}
void Scene_SetRenderModeXlu(PlayState* play, s32 index, u32 flags) {
  assert(index == 0 && flags == 1);
}
s32 SkelAnime_Update(SkelAnime *) {
  ++updates;
  return 0;
}
void SkelAnime_Init(PlayState *, SkelAnime *a, SkeletonHeader *s,
                    AnimationHeader *anim, Vec3s *j, Vec3s *m, s32 count) {
  assert(strstr((char *)s, "gStalchildSkel") && count == 20 && j && m);
  ++normalInit;
  a->jointTable = j;
}
void SkelAnime_InitFlex(PlayState *, SkelAnime *a, FlexSkeletonHeader *s,
                        AnimationHeader *anim, Vec3s *j, Vec3s *m, s32 count) {
  assert(j && m);
  ++flexInit;
  a->jointTable = j;
  a->dListCount = count == 20 ? 17 : 20;
  if (expected->id == ACTOR_EN_GG)
    assert(strstr((char *)s, "Skel_00F6C0") && count == 20);
  else
    assert(strstr((char *)s, "Skel_029170") && count == 23);
}
void gSPDisplayList(Gfx *p, Gfx *d) { p->words.w1 = (uintptr_t)d; }
void gSPSegment(void *p, int seg, uintptr_t d) { ((Gfx *)p)->words.w1 = d; }
void Gfx_SetupDL25_Opa(GraphicsContext *) {}
void Gfx_SetupDL25_Xlu(GraphicsContext *) {}
void Graph_OpenDisps(Gfx **, Gfx *, GraphicsContext *, const char *, s32) {}
void Graph_CloseDisps(Gfx **, Gfx *, GraphicsContext *, const char *, s32) {}
Mtx *Matrix_Finalize(GraphicsContext *) {
  static Mtx m;
  return &m;
}
void SkelAnime_DrawOpa(PlayState *play, void **, Vec3s *, OverrideLimbDrawOpa,
                       PostLimbDrawOpa post, Actor *a) {
  assert(a->id == ACTOR_EN_RAIL_SKB);
  assert(post);
  Vec3s rot{}; Gfx* list = nullptr;
  post(play, STALCHILD_LIMB_HEAD, &list, &rot, a);
  assert(a->focus.pos.y == 17);
  ++opaDraw;
}
void SkelAnime_DrawFlexOpa(PlayState *p, void **, Vec3s *, s32 count,
                           OverrideLimbDrawOpa, PostLimbDrawOpa post,
                           Actor *a) {
  assert(a->id == ACTOR_EN_ZOG && count == 20);
  ++opaDraw;
  Gfx *start = p->state.gfxCtx->polyOpa.p;
  Vec3s r{};
  Gfx *d = nullptr;
  post(p, OBJECT_ZOG_LIMB_09, &d, &r, a);
  assert(a->focus.pos.y == 27);
  post(p, 16, &d, &r, a);
  assert(start == p->state.gfxCtx->polyOpa.p);
  post(p, 17, &d, &r, a);
  assert(p->state.gfxCtx->polyOpa.p ==
         start + (((a->params >> 4) & 15) ? 2 : 0));
}
Gfx *SkelAnime_DrawFlex(PlayState *p, void **, Vec3s *, s32 count,
                        OverrideLimbDraw, PostLimbDraw post, Actor *a, Gfx *g) {
  assert(a->id == ACTOR_EN_GG && count == 17);
  ++xluDraw;
  assert(p->state.gfxCtx->polyOpa.p == commands &&
         g[-1].words.w1 == 0xffffffB0U);
  assert(post);
  Vec3s rot{}; Gfx* list = nullptr;
  post(p, OBJECT_GG_LIMB_04, &list, &rot, a, &g);
  assert(a->focus.pos.y == 17);
  return g;
}
}
int main() {
  PlayState p{};
  GraphicsContext gfx{};
  p.state.gfxCtx = &gfx;
  EnRailSkb child{};
  EnZog zora{};
  EnGg ghost{};
  Actor *actors[] = {&child.actor, &zora.actor, &ghost.actor};
  const int ids[] = {ACTOR_EN_RAIL_SKB, ACTOR_EN_ZOG, ACTOR_EN_GG};
  const int params[] = {0x7E0F, 0x7D01, 0x7D02};
  for (unsigned i = 0; i < 3; i++) {
    Actor *a = actors[i];
    a->id = ids[i];
    expected = a;
    a->params = 0;
    a->flags = 123;
    assert(!ComboRegionalActor_TryInit(a, &p) && a->flags == 123);
    for (unsigned pose = 0; pose < (i ? 2 : 5); pose++) {
      a->params = params[i] | (pose << 4);
      a->world.pos = {3, 4, 5};
      a->world.rot = {111, 222, 333};
      assert(ComboRegionalActor_TryInit(a, &p));
      assert(a->update && a->draw && a->destroy);
      assert(!(a->flags & ACTOR_FLAG_REACT_TO_LENS));
      assert(a->category == ACTORCAT_NPC);
      assert(a->flags & ACTOR_FLAG_ATTENTION_ENABLED);
      a->update(a, &p);
      assert(a->world.pos.x == 3 && a->world.pos.y == 4 && a->world.pos.z == 5);
      assert(a->shape.rot.x == 0 && a->shape.rot.y == 222 &&
             a->shape.rot.z == 0);
      gfx.polyOpa.p = commands;
      gfx.polyXlu.p = commands + 64;
      a->draw(a, &p);
      const Vec3f focus = a->focus.pos;
      a->update(a, &p);
      assert(!std::memcmp(&focus, &a->focus.pos, sizeof(focus)));
      a->destroy(a, &p);
    }
    available = false;
    assert(ComboRegionalActor_TryInit(a, &p) && !a->destroy);
    available = true;
  }
  expected = &zora.actor;
  zora.actor.params = 0x7D11;
  missing = "DL_0280A8";
  assert(ComboRegionalActor_TryInit(expected, &p) && !expected->destroy);
  assert(killed == 4 && freed == 9 && normalInit == 5 && flexInit == 4 &&
         opaDraw == 7 && xluDraw == 2 && updates == 18);
}
