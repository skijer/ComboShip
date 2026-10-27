// Executes the production MM skin renderer and native skin matrix math.
// Native Open/CloseDisps and setup execute unchanged; allocation, DL resource
// submission, CVars and the actor matrix boundary are replaced.
extern "C" {
#include "expansions/ssbb/characters/pikachu_ssbb_register.h"
#include "expansions/ssbb/ssbb_anim.h"
#include "expansions/ssbb/ssbb_skin.h"
}
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

static float scaleOverride, submittedScale;
static int displayLists;
extern "C" {
int32_t CVarGetInteger(const char *, int32_t value) { return value; }
float CVarGetFloat(const char *name, float value) {
  return std::strcmp(name, "gExpansions.SSBB.SkinScale") == 0 ? scaleOverride
                                                              : value;
}
void *ZeldaArena_Malloc(size_t size) { return std::calloc(1, size); }
void ZeldaArena_Free(void *p) { std::free(p); }
void FrameInterpolation_RecordOpenChild(const void *, int) {}
void FrameInterpolation_RecordCloseChild(void) {}
void gSPSegment(void *out, int segment, uintptr_t address) {
  Gfx *command = static_cast<Gfx *>(out);
  command->words.w0 = segment;
  command->words.w1 = address;
}
void gSPDisplayList(Gfx *command, Gfx *list) {
  ++displayLists;
  command->words.w0 = 0xde;
  command->words.w1 = reinterpret_cast<uintptr_t>(list);
}
void Matrix_SetTranslateRotateYXZ(f32, f32, f32, Vec3s *) {}
void Matrix_Scale(f32 x, f32 y, f32 z, MatrixMode) {
  assert(x == y && y == z);
  submittedScale = x;
}
Mtx *Matrix_ToMtx(Mtx *out) {
  std::memset(out, 0x42, sizeof(*out));
  return out;
}
Gfx gCullBackDList[1];
AnimationHeader pikachu_ssbb_Wait1_anim{}, pikachu_ssbb_Wait3_anim{};
}
static void close(float a, float b) {
  if (std::fabs(a - b) > .002f) {
    std::fprintf(stderr, "FAIL skin pose: expected %.3f, got %.3f\n", b, a);
    std::exit(1);
  }
}
int main() {
  StandardLimb limbs[3]{};
  void *skeleton[] = {&limbs[0], &limbs[1], &limbs[2]};
  for (int i = 0; i < 3; ++i) {
    limbs[i].child = i == 2 ? LIMB_DONE : i + 1;
    limbs[i].sibling = LIMB_DONE;
  }
  SSBBBoneFrame frames[6]{};
  for (auto &f : frames)
    f.sx = f.sy = f.sz = 1;
  frames[0].tx = 10;
  frames[1].ty = 20;
  frames[2].tx = 30;
  frames[2].ty = 5;
  frames[3] = frames[0];
  frames[3].tx = 30;
  frames[3].sx = 2;
  frames[4] = frames[1];
  frames[5] = frames[2];
  SSBBAnim anim{"fixture", 2, 3, 30, frames};
  SSBBSkinVertex vertex{2, 0, 0, 127, 0, 0, 32, 64, 255};
  SSBBSkinWeight weight{{0, 0, 0, 0}, {255, 0, 0, 0}};
  MtxF inverse[3]{};
  for (auto &m : inverse)
    m.xx = m.yy = m.zz = m.ww = 1;
  SSBBSkinMesh skin{};
  skin.vertexCount = 1;
  skin.boneCount = 3;
  skin.vertices = &vertex;
  skin.weights = &weight;
  skin.invBindMatrices = inverse;
  skin.daeToF64.xx = skin.daeToF64.yy = skin.daeToF64.zz = skin.daeToF64.ww = 1;
  Gfx mesh[1]{};
  skin.displayList = mesh;
  SSBBCharacterDef def{};
  def.skinMesh = &skin;
  def.scale = .3f;
  def.numLimbs = 3;
  SSBBCharacterInstance inst{};
  inst.def = &def;
  inst.skeleton = skeleton;
  inst.initialized = 1;
  inst.ssbbAnim = &anim;
  inst.curFrame = .5f;
  GraphicsContext gfx{};
  Gfx commands[128]{};
  Gfx translucent[4]{}, overlay[4]{};
  PlayState play{};
  play.state.gfxCtx = &gfx;
  Vec3f position{};
  Vec3s rotation{};
  auto draw = [&] {
    gfx.polyOpa = {{sizeof(commands), commands, commands, commands + 128}};
    gfx.polyXlu = {
        {sizeof(translucent), translucent, translucent, translucent + 4}};
    gfx.overlay = {{sizeof(overlay), overlay, overlay, overlay + 4}};
    SSBBSkin_Draw(&inst, &play, &position, &rotation);
  };
  SSBBSkin_Init(&skin);
  // Real helper nesting needs 10 opaque packets with material, 9 without,
  // one aligned matrix, and two temporary packets in each other arena.
  for (Gfx *material : {static_cast<Gfx *>(nullptr), mesh}) {
    skin.materialDL = material;
    const size_t commandCount = material ? 10 : 9;
    const size_t required = commandCount * sizeof(Gfx) + ALIGN16(sizeof(Mtx));
    auto arenas = [&](size_t opaqueBytes, size_t xluSlots = 2,
                      size_t overlaySlots = 2) {
      std::memset(commands, 0xa5, sizeof(commands));
      std::memset(translucent, 0xa5, sizeof(translucent));
      std::memset(overlay, 0xa5, sizeof(overlay));
      gfx.polyOpa = {{sizeof(commands), commands, commands,
                      reinterpret_cast<u8 *>(commands) + opaqueBytes}};
      gfx.polyXlu = {{sizeof(translucent), translucent, translucent,
                      translucent + xluSlots}};
      gfx.overlay = {
          {sizeof(overlay), overlay, overlay, overlay + overlaySlots}};
    };
    auto rejected = [&] {
      const GraphicsContext previous = gfx;
      Gfx oldCommands[128], oldTranslucent[4], oldOverlay[4];
      std::memcpy(oldCommands, commands, sizeof(commands));
      std::memcpy(oldTranslucent, translucent, sizeof(translucent));
      std::memcpy(oldOverlay, overlay, sizeof(overlay));
      Vtx previousVertices[2] = {skin.vtxBuf[0][0], skin.vtxBuf[1][0]};
      const auto previousIndex = skin.bufIndex;
      const int before = displayLists;
      SSBBCharacterInstance other = inst;
      assert(SSBBSkin_ComputePose(&other));
      SSBBSkin_Draw(&inst, &play, &position, &rotation);
      if (displayLists != before) {
        std::fputs("FAIL short MM graphics arena submitted skin draw\n",
                   stderr);
        std::exit(1);
      }
      assert(gfx.polyOpa.p == previous.polyOpa.p &&
             gfx.polyOpa.d == previous.polyOpa.d);
      assert(gfx.polyXlu.p == previous.polyXlu.p &&
             gfx.polyXlu.d == previous.polyXlu.d);
      assert(gfx.overlay.p == previous.overlay.p &&
             gfx.overlay.d == previous.overlay.d);
      assert(std::memcmp(oldCommands, commands, sizeof(commands)) == 0);
      assert(std::memcmp(oldTranslucent, translucent, sizeof(translucent)) ==
             0);
      assert(std::memcmp(oldOverlay, overlay, sizeof(overlay)) == 0);
      assert(previousIndex == skin.bufIndex);
      assert(std::memcmp(&previousVertices[0], &skin.vtxBuf[0][0],
                         sizeof(Vtx)) == 0);
      assert(std::memcmp(&previousVertices[1], &skin.vtxBuf[1][0],
                         sizeof(Vtx)) == 0);
      Vec3f bone;
      assert(SSBBSkin_GetBoneWorldPos(&other, 0, &bone));
      assert(!SSBBSkin_GetBoneWorldPos(&inst, 0, &bone));
    };
    arenas(required - 1);
    rejected();
    arenas(required, 1);
    rejected();
    arenas(required, 2, 1);
    rejected();
    arenas(required);
    gfx.polyOpa.p = nullptr;
    rejected();
    arenas(required);
    gfx.polyOpa.d = nullptr;
    rejected();
    arenas(required);
    gfx.polyOpa.p = commands + 2;
    gfx.polyOpa.d = commands + 1;
    rejected();
    const SSBBSkinDrawReserve reserve = {4 * sizeof(Gfx), sizeof(Gfx), sizeof(Gfx)};
    arenas(required + reserve.opaBytes - 1, 3, 3);
    const auto savedIndex = skin.bufIndex;
    assert(!SSBBSkin_DrawWithReserve(&inst, &play, &position, &rotation, &reserve));
    assert(skin.bufIndex == savedIndex && gfx.polyOpa.p == commands);
    arenas(required + reserve.opaBytes, 2, 3);
    assert(!SSBBSkin_DrawWithReserve(&inst, &play, &position, &rotation, &reserve));
    arenas(required + reserve.opaBytes, 3, 2);
    assert(!SSBBSkin_DrawWithReserve(&inst, &play, &position, &rotation, &reserve));
    arenas(required + reserve.opaBytes, 3, 3);
    assert(SSBBSkin_DrawWithReserve(&inst, &play, &position, &rotation, &reserve));
    assert((uintptr_t)gfx.polyOpa.d - (uintptr_t)gfx.polyOpa.p == reserve.opaBytes);
    assert(gfx.polyXlu.p == translucent && gfx.overlay.p == overlay);
    for (size_t extra : {size_t(0), size_t(1), sizeof(Gfx)}) {
      arenas(required + extra);
      const int before = displayLists;
      SSBBSkin_Draw(&inst, &play, &position, &rotation);
      assert(displayLists - before == (material ? 3 : 2));
      assert(gfx.polyOpa.p == commands + commandCount);
      assert(reinterpret_cast<uintptr_t>(gfx.polyOpa.d) -
                 reinterpret_cast<uintptr_t>(gfx.polyOpa.p) ==
             extra);
      assert(gfx.polyXlu.p == translucent && gfx.overlay.p == overlay);
      const u8 *matrixEnd =
          reinterpret_cast<const u8 *>(gfx.polyOpa.d) + ALIGN16(sizeof(Mtx));
      for (const u8 *p = matrixEnd;
           p < reinterpret_cast<const u8 *>(commands + 128); ++p)
        assert(*p == 0xa5);
      for (const u8 *p = reinterpret_cast<const u8 *>(translucent);
           p < reinterpret_cast<const u8 *>(translucent + 4); ++p)
        assert(*p == 0xa5);
      for (const u8 *p = reinterpret_cast<const u8 *>(overlay);
           p < reinterpret_cast<const u8 *>(overlay + 4); ++p)
        assert(*p == 0xa5);
    }
  }
  skin.materialDL = nullptr;
  // Existing Pikachu defaults: snap fractional frames, strip TopN/EyeYellowM
  // motion, and retain the shared scale override.
  scaleOverride = .7f;
  draw();
  close(skin.vtxBuf[skin.bufIndex ^ 1][0].n.ob[0], 2);
  close(submittedScale, .7f);
#ifdef MM_WOLF_SKIN_OPTIONS
  skin.preserveRootMotion = 1;
  skin.interpolateFrames = 1;
  skin.useDefinitionScale = 1;
#endif
  draw();
  // The unmodified renderer fails here (2 instead of 23): Wolf's authored root
  // translation and fractional scale must reach the actual packed vertex.
  close(skin.vtxBuf[skin.bufIndex ^ 1][0].n.ob[0], 23);
  close(submittedScale, .3f);
#ifdef MM_WOLF_SKIN_OPTIONS
  Vec3f bone{};
  assert(SSBBSkin_GetBoneWorldPos(&inst, 2, &bone));
  close(bone.x, 65);
  close(bone.y, 25);
  assert(!SSBBSkin_GetBoneWorldPos(&inst, 3, &bone));
  assert(!SSBBSkin_GetBoneWorldPos(&inst, -1, &bone));
  assert(!SSBBSkin_GetBoneWorldPos(&inst, 0, nullptr));
  SSBBCharacterInstance other = inst;
  assert(!SSBBSkin_GetBoneWorldPos(&other, 0, &bone));
  assert(SSBBSkin_ComputePose(&other));
  assert(!SSBBSkin_GetBoneWorldPos(&inst, 0, &bone));
  assert(SSBBSkin_GetBoneWorldPos(&other, 0, &bone));
  close(bone.x, 20);
  // Shortest rotation arc, including wrap across -180/180.
  frames[0].rz = 170;
  frames[3].rz = -170;
  inst.curFrame = .5f;
  draw();
  close(skin.vtxBuf[skin.bufIndex ^ 1][0].n.ob[0], 17);
  frames[0].rz = 720;
  frames[3].rz = -270;
  draw();
  close(skin.vtxBuf[skin.bufIndex ^ 1][0].n.ob[0], 22);
  inst.curFrame = 1;
  draw();
  close(submittedScale, .3f);
  frames[3].rz = 0;
  vertex.posX = 30000;
  draw();
  assert(skin.vtxBuf[skin.bufIndex ^ 1][0].n.ob[0] == 32767);
  vertex.posX = -30000;
  draw();
  assert(skin.vtxBuf[skin.bufIndex ^ 1][0].n.ob[0] == -32768);
  const u32 invalidFrame = 0x7fc00001;
  std::memcpy(&inst.curFrame, &invalidFrame, sizeof(invalidFrame));
  assert(!SSBBSkin_ComputePose(&inst));
  assert(!SSBBSkin_GetBoneWorldPos(&inst, 0, &bone));
  SSBBSkin_Destroy(&skin);
  assert(!SSBBSkin_GetBoneWorldPos(&inst, 0, &bone));
#else
  SSBBSkin_Destroy(&skin);
#endif
  // Actual production Pikachu has 48 skeleton nodes but only 47 weighted bones.
  // The trailing node is a rigid extra limb; it has no inverse bind or skin
  // weights.
  SSBBBoneFrame pikaFrames[47]{};
  for (auto &f : pikaFrames)
    f.sx = f.sy = f.sz = 1;
  SSBBAnim pikaAnim{"pikachu-fixture", 1, 47, 30, pikaFrames};
  SSBBCharacterInstance pika{};
  pika.def = &pikachu_ssbb_def;
  pika.skeleton = (void **)pikachu_ssbb_skeleton.sh.segment;
  pika.initialized = 1;
  pika.ssbbAnim = &pikaAnim;
  assert(pika.def->numLimbs == 48 && pika.def->skinMesh->boneCount == 47);
  SSBBSkin_Init(pika.def->skinMesh);
  gfx.polyOpa = {{sizeof(commands), commands, commands, commands + 128}};
  gfx.polyXlu = {
      {sizeof(translucent), translucent, translucent, translucent + 4}};
  gfx.overlay = {{sizeof(overlay), overlay, overlay, overlay + 4}};
  int before = displayLists;
  SSBBSkin_Draw(&pika, &play, &position, &rotation);
  if (displayLists == before) {
    std::fputs(
        "FAIL actual Pikachu 48-node/47-bone metadata stopped rendering\n",
        stderr);
    return 1;
  }
  close(submittedScale, .7f);
#ifdef MM_WOLF_SKIN_OPTIONS
  assert(!SSBBSkin_GetBoneWorldPos(&pika, 47, &position));
  assert(!SSBBSkin_GetBoneWorldPos(&pika, 48, &position));
#endif
  SSBBSkin_Destroy(pika.def->skinMesh);
  std::puts("PASS production MM skin: Pikachu defaults, Wolf root/TRS "
            "interpolation, scale and owner-scoped bone queries");
}
