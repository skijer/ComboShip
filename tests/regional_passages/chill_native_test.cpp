#include "ComboGameMode.h"
#include "entrances/FountainSnowheadBridge.h"
#include "entrances/LostWoodsEntrance.h"
#include "global.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include "overlays/effects/ovl_Effect_Ss_Ice_Smoke/z_eff_ss_ice_smoke.h"
#include "render/RegionalSnow.h"
#include "render/RegionalBridgeMaterials.h"
#include "fast/lus_gbi.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
SaveContext gSaveContext{};
extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
uintptr_t gSegments[16]{};
u32 EffectSsIceSmoke_Init(PlayState *, u32, EffectSs *, void *);
void EffectSsIceSmoke_Update(PlayState *, u32, EffectSs *);
}
using namespace ComboFountainSnowhead;
struct SnowheadActor {
  DynaPolyActor dyna;
  uint32_t snowTick;
};
SnowheadActor *sActor = nullptr;
PlayState *sOwner = nullptr;
const Mesh sMesh = BuildMesh();
static std::array<Vtx, 192 * 3> sDrawVertices{};
static bool available = true, room = true;
static unsigned matrices;
static size_t requestedMatrices, requestedXlu;
static std::array<Vtx, 15 * RegionalSnow::kChillCount> vertexArena;
static size_t allocatedVertices;
extern "C" void* Graph_Alloc(GraphicsContext*, size_t size) {
  assert(size == sizeof(Vtx) * 15);
  auto* result = vertexArena.data() + allocatedVertices;
  allocatedVertices += 15;
  assert(allocatedVertices <= vertexArena.size());
  return result;
}
static std::array<Vec3f, 128> poses;
extern "C" u8 ResourceMgr_FileExists(const char *path) {
  if (std::strcmp(path, dgDust5Tex + 7) == 0)
    return available;
  assert(std::strncmp(path, "textures/comboship/regional/", 27) == 0);
  return false;
}
extern "C" bool NeiGi_CanDrawLayers(PlayState *, size_t m, size_t, size_t xlu) {
  requestedMatrices = m; requestedXlu = xlu;
  return room;
}
extern "C" int ResourceMgr_OTRSigCheck(char *path) {
  return path && std::memcmp(path, "__OTR__", 7) == 0;
}
extern "C" Gfx *PakLoader_GetDLOverride(const char *) {
  assert(false);
  return nullptr;
}
extern "C" Gfx *ResourceMgr_LoadGfxByName(char *) {
  assert(false);
  return nullptr;
}
extern "C" void Matrix_Translate(f32 x, f32 y, f32 z, u8) {
  poses[matrices] = {x, y, z};
}
extern "C" void Matrix_Mult(MtxF *, u8) {}
extern "C" Mtx *Matrix_NewMtx(GraphicsContext *, char *, s32) {
  static Mtx m[128];
  return &m[matrices++];
}
#undef OPEN_DISPS
#undef CLOSE_DISPS
#define OPEN_DISPS(ctx)                                                        \
  {                                                                            \
    GraphicsContext *__gfxCtx = (ctx)
#define CLOSE_DISPS(ctx) }
#define gSPVertex __gSPVertex
#include "soh/Enhancements/Graphics/SeasonWinterMist.h"
/* NATIVE_SETUP */
/* PRODUCTION */

using F3DGfx = Gfx;
/* NATIVE_COMBINER */
struct MaskShader {
  struct State {
    uint64_t combine_mode = 0;
    u32 other_mode_l = 0;
    RGBA prim_color{}, env_color{}, key_center{}, key_scale{};
    u8 prim_lod_fraction = 0, convert_k[6]{};
  } state;
  State *mRdp = &state;
  float mBufVbo[28]{};
  size_t mBufVboLen = 0;
  void GfxDpSetCombineMode(u32, u32, u32, u32);
  static void GenerateCC(ColorCombiner *, const ColorCombinerKey &);
  void PackInputs(ColorCombiner *comb, u8 fogFactor) {
    struct Vertex { RGBA color; float w; } vertex{{180, 180, 180, fogFactor}, 1};
    auto *v1 = &vertex;
    Vertex *v_arr[] = {&vertex};
    const int i = 0, numInputs = 7;
    const bool use_alpha = true, use_fog = false, use_blend_color = false;
    mBufVboLen = 0;
    /* NATIVE_PACKED_INPUTS */
  }
  ColorCombiner Combine(bool twoCycle = false) {
    ColorCombinerKey key{};
    key.options = SHADER_OPT(ALPHA) | (twoCycle ? SHADER_OPT(_2CYC) : 0);
    key.combine_mode = state.combine_mode;
    ColorCombiner comb{};
    GenerateCC(&comb, key);
    return comb;
  }
  float Channel(const ColorCombiner& comb, float texture, unsigned channel, bool twoCycle = false) {
    float combined = 0;
    for (unsigned cycle = 0; cycle < (twoCycle ? 2u : 1u); ++cycle) {
      float terms[4]{};
      for (unsigned j = 0; j < 4; ++j) {
        const auto input = (comb.shader_id0 >> (cycle * 32 + (channel == 3 ? 16 : 0) + j * 4)) & 15;
        if (input >= SHADER_INPUT_1 && input <= SHADER_INPUT_7)
          terms[j] = mBufVbo[(input - SHADER_INPUT_1) * 4 + channel];
        else if (input == SHADER_TEXEL0 || input == SHADER_TEXEL0A)
          terms[j] = texture;
        else if (input == SHADER_COMBINED)
          terms[j] = combined;
        else {
          assert(input == SHADER_0 || input == SHADER_1);
          terms[j] = input == SHADER_1 ? 1.f : 0.f;
        }
      }
      combined = (terms[0] - terms[1]) * terms[2] + terms[3];
    }
    return combined;
  }
  bool WhiteMaskedSnow(bool twoCycle = false) {
    auto comb = Combine(twoCycle);
    if (!comb.usedTextures[0] || comb.usedTextures[1]) return false;
    PackInputs(&comb, 0);
    const float rgb[] = {state.prim_color.r/255.f, state.prim_color.g/255.f, state.prim_color.b/255.f};
    for (float texture : {0.f, .25f, .5f, 1.f}) {
      if (std::fabs(Channel(comb, texture, 3, twoCycle) - texture * state.prim_color.a / 255.f) > .000001f)
        return false;
      for (unsigned c = 0; c < 3; ++c)
        if (std::fabs(Channel(comb, texture, c, twoCycle) - rgb[c]) > .000001f) return false;
    }
    return true;
  }
  bool VertexMist() {
    auto comb = Combine();
    if (comb.usedTextures[0] || comb.usedTextures[1]) return false;
    PackInputs(&comb, 0);
    // Native packing must retain shade alpha when the scene fog is disabled.
    return std::fabs(Channel(comb, 0, 3)) < .000001f;
  }
};
#define C0(pos, width) ((cmd->words.w0 >> (pos)) & ((1U << (width)) - 1))
#define C1(pos, width) ((cmd->words.w1 >> (pos)) & ((1U << (width)) - 1))
/* NATIVE_SHADER */

// Follow the real SETUPDL_25 sublist; submitting it to polyXlu does not make
// its opaque render mode translucent. Inspect the state at each actual draw.
struct ParticleBlendState {
  u32 high = 0, low = 0, geometry = G_FOG;
  unsigned snow = 0, chill = 0, triangles = 0;
  unsigned vertices = 0;
  bool snowValid = true, chillValid = true;
  MaskShader shader;
  void Walk(Gfx *begin, Gfx *end, unsigned depth = 0) {
    assert(depth < 2);
    for (auto *c = begin; c < end; ++c) {
      switch (c->words.w0 >> 24) {
      case G_DL: {
        auto *target = reinterpret_cast<Gfx *>(c->words.w1);
        assert(target == sSetupDL[SETUPDL_25]);
        Walk(target, target + 6, depth + 1);
        break;
      }
      case G_RDPSETOTHERMODE:
        high = c->words.w0 & 0xffffffu; low = c->words.w1; break;
      case G_SETOTHERMODE_H:
      case G_SETOTHERMODE_L: {
        const u32 bits = (c->words.w0 & 255) + 1;
        const u32 shift = 32 - ((c->words.w0 >> 8) & 255) - bits;
        const u32 mask = u32(((u64(1) << bits) - 1) << shift);
        auto& mode = (c->words.w0 >> 24) == G_SETOTHERMODE_H ? high : low;
        mode = (mode & ~mask) | u32(c->words.w1); break;
      }
      case G_GEOMETRYMODE:
        geometry = (geometry & (c->words.w0 & 0xffffff)) | c->words.w1; break;
      case G_SETPRIMCOLOR:
        shader.state.prim_color = {u8(c->words.w1 >> 24), u8(c->words.w1 >> 16), u8(c->words.w1 >> 8), u8(c->words.w1)}; break;
      case G_SETCOMBINE: { auto *packet = c; gfx_set_combine_handler_rdp(&shader, &packet); break; }
      case G_VTX:
        vertices = (c->words.w0 >> 12) & 255;
        if (vertices == 15) {
          ++chill;
          auto* v = reinterpret_cast<const Vtx*>(c->words.w1);
          assert(v[7].v.cn[3] > 0 && v[6].v.cn[3] == v[8].v.cn[3]);
          assert(std::abs(int(v[6].v.cn[3]) - std::lround(v[7].v.cn[3] * .68f)) <= 1);
          for (unsigned n=0; n<15; ++n) {
            assert(v[n].v.cn[0] == 192 && v[n].v.cn[1] == 222 && v[n].v.cn[2] == 245);
            if (n != 6 && n != 7 && n != 8) assert(v[n].v.cn[3] == 0);
          }
        } else { assert(vertices == 4); ++snow; }
        break;
      case G_TRI2: {
        ++triangles;
        const bool blends = (((low >> 20) & 3) == G_BL_CLR_MEM && ((low >> 16) & 3) == G_BL_1MA) ||
                            (((low >> 22) & 3) == G_BL_CLR_MEM && ((low >> 18) & 3) == G_BL_1MA);
        const bool valid = blends && (low & FORCE_BL) && (low & Z_CMP) && !(low & Z_UPD) &&
                           !(geometry & (G_FOG | G_LIGHTING | G_CULL_BOTH)) &&
                           (geometry & (G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH)) == (G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH) &&
                           (high & (3u << G_MDSFT_CYCLETYPE)) == G_CYC_1CYCLE;
        if (vertices == 15) chillValid &= valid && shader.VertexMist();
        else snowValid &= valid && shader.WhiteMaskedSnow();
        break;
      }
      }
    }
  }
};

static void Alive(Actor *, PlayState *) {}
int main() {
  PlayState play{};
  GraphicsContext gfx{};
  Player player{};
  SnowheadActor actor{};
  static Gfx commands[1024];
  static Gfx opaqueCommands[8192];
  play.state.gfxCtx = &gfx;
  play.state.running = true;
  play.view.eye = {1000, kMouth.y + 70, 0};
  play.view.lookAt = {2000, kMouth.y, 0};
  play.view.up = {0, 1, 0};
  play.view.zNear = 10; play.view.zFar = 12800;
  // Independently reproduce the original Winter vertex math, so extraction
  // cannot silently change the season's shape, alpha ramp, or icy color.
  for (float width : {180.f, 257.f, 370.f})
    for (float height : {10.f, 17.f, 20.f})
      for (float alpha : {0.f, .031f, .24f}) {
        Vtx vertices[15];
        SoHSeasonWinterMist::Vertices(vertices, .6f, -.8f, width, height, alpha);
        for (int row = 0; row < 3; ++row)
          for (int column = 0; column < 5; ++column) {
            const float x = (column - 2) * .5f;
            const auto& v = vertices[row * 5 + column].v;
            assert(v.ob[0] == std::lround(.6f * (x * width)) && v.ob[2] == std::lround(-.8f * (x * width)));
            assert(v.ob[1] == std::lround((row - 1) * height * (1 - .3f * std::abs(x))));
            assert(v.cn[0] == 192 && v.cn[1] == 222 && v.cn[2] == 245);
            const float a = row == 1 && column > 0 && column < 4 ? alpha * (column == 2 ? 1 : .68f) : 0;
            assert(v.cn[3] == std::lround(a * 255));
          }
      }
  {
    auto savedView = play.view;
    play.view.eye = {0, 0, 0}; play.view.lookAt = {0, 0, 1};
    play.view.zNear = 10; play.view.zFar = 1000;
    auto opacity = [&](float depth) { return ParticleViewOpacity(&play, {0, 0, depth}, 20); };
    assert(opacity(42) == 0 && opacity(162) == 1 && opacity(968) == 0);
    assert(opacity(102) > .49f && opacity(102) < .51f);
    assert(opacity(848) > .49f && opacity(848) < .51f);
    float previous = opacity(-50);
    for (int depth = -49; depth <= 1200; ++depth) {
      float current = opacity(float(depth));
      assert(current >= 0 && current <= 1 && std::abs(current - previous) < .013f);
      previous = current;
    }
    play.view = savedView;
  }
  play.sceneNum = SCENE_ZORAS_FOUNTAIN;
  play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
  gSaveContext.gameMode = GAMEMODE_NORMAL;
  gSaveContext.fileNum = 0;
  sOwner = &play;
  sActor = &actor;
  actor.dyna.actor.update = Alive;
  actor.snowTick = 50;
  // Leave the ascent and old threshold clear: the local snow and Winter mist
  // now belong to the final approach, not the start of the valley walk.
  for (const auto early : {Point{1000, 0, 850}, Grounded(1600, 0),
                           Grounded(1840, 0), Grounded(2200, 0)}) {
    player.actor.world.pos = {early.x, early.y, early.z};
    play.view.eye = {early.x - 840, early.y + 70, early.z};
    play.view.lookAt = {early.x + 160, early.y, early.z};
    gfx.polyOpa.p = opaqueCommands;
    gfx.polyXlu.p = commands;
    matrices = 0;
    allocatedVertices = 0;
    Draw(&actor.dyna.actor, &play);
    ParticleBlendState earlyBlend;
    earlyBlend.Walk(commands, gfx.polyXlu.p);
    assert(!earlyBlend.snow && !earlyBlend.chill &&
           "cold effects must wait until the deep valley approach");
  }
  play.view.eye = {kMouth.x - 840, kMouth.y + 70, 0};
  play.view.lookAt = {kMouth.x + 160, kMouth.y, 0};
  player.actor.world.pos = {kMouth.x, kMouth.y, 0};
  matrices = 0;
  allocatedVertices = 0;
  gfx.polyOpa.p = opaqueCommands;
  gfx.polyXlu.p = commands;
  Draw(&actor.dyna.actor, &play);
  ParticleBlendState blend;
  blend.Walk(commands, gfx.polyXlu.p);
  assert(blend.chill && "Winter mist must submit 15-vertex soft-edge meshes, not dust cards");
  assert(blend.snow && blend.snowValid && "snow RGB must stay white while texture controls alpha; no inherited fog/depth writes");
  assert(blend.chillValid && "mist must use Winter's untextured SHADE material without fog/depth writes");
  assert((blend.low & Z_UPD) && "the final native setup must be restored");
  puts("PASS actual snow RGB independent of dark texture; native mask alpha; Winter 15-vertex material and edge alpha; setup restored");
  Gfx bad{};
  gDPSetCombineMode(&bad, G_CC_MODULATEIA_PRIM, G_CC_PASS2);
  auto* badPacket = &bad;
  gfx_set_combine_handler_rdp(&blend.shader, &badPacket);
  assert(!blend.shader.WhiteMaskedSnow(true) && "soot-producing texture RGB modulation negative control rejected");
  // Walk the actual camera through one anchored wisp. Its emitted center alpha
  // must reach zero before clipping, then grow smoothly while local time stays
  // frozen. This catches a renderer that forgets to apply the tested fade.
  {
    const auto savedView = play.view;
    const auto savedPlayer = player.actor.world.pos;
    const size_t i = 5;
    const auto puff = RegionalSnow::Chill(actor.snowTick, i);
    const auto center = Grounded(3200, 0);
    const Vec3f position = {center.x + puff.x, center.y + puff.y + 20.f, center.z + puff.z};
    player.actor.world.pos = {center.x, center.y, center.z};
    unsigned previous = 0, full = 0;
    for (int depth = -30; depth <= 300; ++depth) {
      play.view.eye = {position.x - depth, position.y, position.z};
      play.view.lookAt = {play.view.eye.x + 1, position.y, position.z};
      gfx.polyXlu.p = commands; matrices = 0; allocatedVertices = 0;
      DrawLocalChill(&actor.dyna.actor, &play);
      unsigned alpha = 0;
      for (unsigned n = 0; n < matrices; ++n)
        if (std::fabs(poses[n].x - position.x) < .001f && std::fabs(poses[n].z - position.z) < .001f)
          alpha = vertexArena[n * 15 + 7].v.cn[3];
      assert(alpha >= previous && alpha - previous <= 1);
      if (depth <= 42) assert(alpha == 0);
      if (depth == 180) full = alpha;
      if (depth > 180) assert(alpha == full);
      previous = alpha;
    }
    assert(full > 0);
    play.view = savedView;
    player.actor.world.pos = savedPlayer;
  }
  puts("PASS exact Winter vertex parity; smooth actual near-camera mist sweep and near/far clip alpha limits");
  // Actual native Fountain object lists, decoded independently in review.
  const std::initializer_list<s16> lists[] = {
      {0x74, 0x40, 0x12f, 0x12c, 0xa4, 0x170, 0x7c, 0x188, 0x163, 0x24},
      {0x74, 0x12f, 7, 0x16, 0x11f, 0x188, 0x24, 0x154, 0x12c, 0x163},
      {0x74, 0x40, 0x13, 0x12f, 0x12c, 0xa4, 0x170, 0x7c, 0x188, 0x24, 0x163}};
  for (auto list : lists) {
    play.objectCtx = {};
    for (s16 id : list)
      play.objectCtx.status[play.objectCtx.num++].id = id;
    auto before = play.objectCtx;
    EffectSs old{};
    EffectSsIceSmokeInitParams init{};
    assert(!EffectSsIceSmoke_Init(&play, 0, &old, &init));
    gfx.polyXlu.p = commands;
    matrices = 0; allocatedVertices = 0;
    DrawLocalChill(&actor.dyna.actor, &play);
    ParticleBlendState local;
    local.Walk(commands, gfx.polyXlu.p);
    unsigned textures = 0, pushes = 0, pops = 0;
    for (auto* c = commands; c < gfx.polyXlu.p; ++c) {
      const auto op = c->words.w0 >> 24;
      textures += op == G_SETTIMG;
      pushes += op == G_COMBO_RM_PUSH;
      pops += op == G_COMBO_RM_POP;
    }
    assert(local.chill > 0 && !local.snow && local.chillValid && local.triangles == local.chill * 8);
    assert(textures == 0 && matrices == local.chill && allocatedVertices == local.chill * 15 && pushes == 1 && pops == 1);
    assert(requestedMatrices * sizeof(Mtx) >= RegionalSnow::kChillCount * (sizeof(Mtx) + 15 * sizeof(Vtx)));
    assert(requestedXlu >= size_t(gfx.polyXlu.p - commands));
    assert(!std::memcmp(&before, &play.objectCtx, sizeof(before)));
    auto first = poses;
    unsigned count = matrices;
    gfx.polyXlu.p = commands;
    matrices = 0; allocatedVertices = 0;
    DrawLocalChill(&actor.dyna.actor, &play);
    assert(count == matrices &&
           !std::memcmp(first.data(), poses.data(), count * sizeof(Vec3f)));
    ++actor.snowTick;
    gfx.polyXlu.p = commands;
    matrices = 0; allocatedVertices = 0;
    DrawLocalChill(&actor.dyna.actor, &play);
    assert(std::memcmp(first.data(), poses.data(), count * sizeof(Vec3f)) != 0);
  }
  auto none = [&] {
    gfx.polyXlu.p = commands;
    matrices = 0; allocatedVertices = 0;
    DrawLocalChill(&actor.dyna.actor, &play);
    assert(gfx.polyXlu.p == commands && matrices == 0);
  };
  available = false;
  gfx.polyXlu.p = commands; matrices = 0; allocatedVertices = 0;
  DrawLocalChill(&actor.dyna.actor, &play);
  assert(matrices > 0 && "untextured Winter mist has no dust-resource dependency");
  available = true;
  room = false;
  none();
  room = true;
  gfx.polyXlu.p = commands;
  matrices = 0; allocatedVertices = 0;
  DrawLocalChill(&actor.dyna.actor, &play);
  assert(gfx.polyXlu.p > commands && matrices > 0);
  player.actor.world.pos = {0, 0, 4000};
  none();
  player.actor.world.pos = {kMouth.x, kMouth.y, 0};
  sOwner = nullptr;
  none();
  sOwner = &play;
  sActor = nullptr;
  none();
  sActor = &actor;
  actor.dyna.actor.update = nullptr;
  none();
  actor.dyna.actor.update = Alive;
  gComboGameMode = COMBO_GAME_OOT;
  none();
  gComboGameMode = COMBO_GAME_BOTH;
  play.sceneNum = SCENE_LAKE_HYLIA;
  none();
  // Native positive/control cleanup proves this is real effect initialization,
  // rather than an attempted-spawn counter or an unconditional failure double.
  play.objectCtx = {};
  play.objectCtx.num = 1;
  play.objectCtx.status[0].id = -OBJECT_FZ;
  play.objectCtx.status[0].segment = (void *)uintptr_t(0x80001000);
  EffectSs old{};
  EffectSsIceSmokeInitParams init{};
  assert(!EffectSsIceSmoke_Init(&play, 0, &old, &init));
  play.objectCtx.status[0].id = OBJECT_FZ;
  assert(EffectSsIceSmoke_Init(&play, 0, &old, &init));
  assert(old.life == 50 && old.draw && old.update);
  play.objectCtx.num = 0;
  EffectSsIceSmoke_Update(&play, 0, &old);
  assert(old.life == -1);
  puts("PASS native Fountain FZ initialization negative/positive controls; "
       "actual actor Winter mesh packets without FZ or texture resources, epoch "
       "motion/freeze, owner/mode/scene cleanup and untouched object context");
}
