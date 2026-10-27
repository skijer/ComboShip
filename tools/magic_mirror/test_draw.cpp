// Real native GraphicsContext, arena allocation, debug brackets, setup
// functions and GBI command encodings; only hardware matrix/interpolation work
// is replaced.
#include "global.h"
#ifdef COMBO_MAGIC_MIRROR_MM
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#else
#include "soh/frame_interpolation.h"
#endif
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <vector>

#undef HREG
#define HREG(index) 0
/* NATIVE_FUNCTIONS */

static int matrixCalls = 0, stackDepth = 0;
void Matrix_Push() {
  ++matrixCalls;
  ++stackDepth;
}
void Matrix_Pop() {
  ++matrixCalls;
  --stackDepth;
  assert(stackDepth >= 0);
}
#ifdef COMBO_MAGIC_MIRROR_MM
void Matrix_Scale(f32, f32, f32, MatrixMode) { ++matrixCalls; }
Mtx *Matrix_ToMtx(Mtx *matrix) {
  ++matrixCalls;
  std::memset(matrix, 0x33, sizeof(*matrix));
  return matrix;
}
#else
void Matrix_Scale(f32, f32, f32, u8) { ++matrixCalls; }
Mtx *Matrix_ToMtx(Mtx *matrix, char *, s32) {
  ++matrixCalls;
  std::memset(matrix, 0x33, sizeof(*matrix));
  return matrix;
}
#endif
// Same raw-pointer branches as the native wrappers; these issue real GBI
// commands.
static std::vector<const Vtx *> submitted;
void gSPVertex(Gfx *packet, uintptr_t vertices, int count, int first) {
  submitted.push_back(reinterpret_cast<const Vtx *>(vertices));
  __gSPVertex(packet, vertices, count, first);
}
void gSPDisplayList(Gfx *packet, Gfx *list) { __gSPDisplayList(packet, list); }
void FrameInterpolation_RecordOpenChild(const void *, int) {}
void FrameInterpolation_RecordCloseChild() {}
static bool hasReflection = false;
extern "C" u8 ResourceMgr_FileExists(const char *path) {
  assert(std::strcmp(
             path,
             "__OTR__textures/magic_mirror/gMagicMirrorForestReflectionTex") ==
         0);
  return hasReflection;
}
#include "combo/gameplay/MagicMirrorDraw.inc.h"

namespace {
constexpr size_t kStorageSize = 196608;
constexpr size_t kPrefixSize = 65536;
struct alignas(16) ArenaStorage {
  std::array<unsigned char, kStorageSize> bytes;
  unsigned char *Head() { return bytes.data() + kPrefixSize; }
};
ArenaStorage opaStorage, xluStorage, overlayStorage;
GraphicsContext gfx;
PlayState play;

void Reset(size_t opaBytes, size_t xluBytes, size_t overlayBytes) {
  std::memset(&gfx, 0, sizeof(gfx));
  std::memset(&play, 0, sizeof(play));
  std::memset(opaStorage.bytes.data(), 0xA5, kStorageSize);
  std::memset(xluStorage.bytes.data(), 0xA5, kStorageSize);
  std::memset(overlayStorage.bytes.data(), 0xA5, kStorageSize);
  gfx.polyOpa.p = reinterpret_cast<Gfx *>(opaStorage.Head());
  gfx.polyOpa.d = reinterpret_cast<Gfx *>(opaStorage.Head() + opaBytes);
  gfx.polyXlu.p = reinterpret_cast<Gfx *>(xluStorage.Head());
  gfx.polyXlu.d = reinterpret_cast<Gfx *>(xluStorage.Head() + xluBytes);
  gfx.overlay.p = reinterpret_cast<Gfx *>(overlayStorage.Head());
  gfx.overlay.d = reinterpret_cast<Gfx *>(overlayStorage.Head() + overlayBytes);
  play.state.gfxCtx = &gfx;
  play.gameplayFrames = 240;
  matrixCalls = stackDepth = 0;
  submitted.clear();
}

void RejectWithoutWrites(size_t opaBytes, size_t xluBytes,
                         size_t overlayBytes) {
  Reset(opaBytes, xluBytes, overlayBytes);
  const auto opaBefore = opaStorage.bytes, xluBefore = xluStorage.bytes,
             overlayBefore = overlayStorage.bytes;
  const auto *opaHead = gfx.polyOpa.p;
  const auto *opaTail = gfx.polyOpa.d;
  const auto *xluHead = gfx.polyXlu.p;
  const auto *xluTail = gfx.polyXlu.d;
  const auto *overlayHead = gfx.overlay.p;
  const auto *overlayTail = gfx.overlay.d;
  ComboMagicMirrorVisual::Draw(&play);
  assert(gfx.polyOpa.p == opaHead && gfx.polyOpa.d == opaTail &&
         "short opaque arena must not allocate or emit");
  assert(gfx.polyXlu.p == xluHead && gfx.polyXlu.d == xluTail &&
         "short translucent arena must leave both streams untouched");
  assert(gfx.overlay.p == overlayHead && gfx.overlay.d == overlayTail);
  assert(opaStorage.bytes == opaBefore && xluStorage.bytes == xluBefore &&
         overlayStorage.bytes == overlayBefore &&
         "reject before either native allocator or debug bracket can overwrite "
         "arena canaries");
  assert(matrixCalls == 0 && stackDepth == 0);
}

void Guard(const ArenaStorage &storage, size_t allowedBytes) {
  for (size_t i = 0; i < kPrefixSize; ++i)
    assert(storage.bytes[i] == 0xA5);
  for (size_t i = kPrefixSize + allowedBytes; i < kStorageSize; ++i)
    assert(storage.bytes[i] == 0xA5);
}
} // namespace

static void Run(bool reflection) {
  hasReflection = reflection;
  using namespace ComboMagicMirrorGeometry;
  Mesh mesh;
  Build(0, mesh);
  size_t opaqueCommands = 14, translucentCommands = 15 + (reflection ? 15 : 0);
  for (int b = 0; b < mesh.batchCount; ++b) {
    const auto &batch = mesh.batches[b];
    const size_t count = 1 + (batch.triangleCount + 1) / 2;
    (b < mesh.opaqueBatchCount ? opaqueCommands : translucentCommands) += count;
  }
  const size_t dataBytes =
      ALIGN16(sizeof(Vtx) * mesh.vertexCount) + ALIGN16(sizeof(Mtx) * 2);
  const size_t opaqueBytes = dataBytes + opaqueCommands * sizeof(Gfx);
  const size_t translucentBytes = translucentCommands * sizeof(Gfx);
  const size_t overlayBytes = 2 * sizeof(Gfx);
  // Each native data/command boundary, not only a null allocator result.
  for (size_t bytes : {size_t(0), size_t(1), dataBytes - 1, dataBytes,
                       opaqueBytes - sizeof(Gfx), opaqueBytes - 1})
    RejectWithoutWrites(bytes, translucentBytes, overlayBytes);
  for (size_t bytes : {size_t(0), size_t(1), translucentBytes - sizeof(Gfx),
                       translucentBytes - 1})
    RejectWithoutWrites(opaqueBytes, bytes, overlayBytes);
  for (size_t bytes : {size_t(0), sizeof(Gfx), overlayBytes - 1})
    RejectWithoutWrites(opaqueBytes, translucentBytes, bytes);
  // At the precise supported boundary, real setup/debug commands and native
  // subtracting allocation must stay inside all three guarded regions.
  Reset(opaqueBytes, translucentBytes, overlayBytes);
  ComboMagicMirrorVisual::Draw(&play);
  assert(matrixCalls > 0 && stackDepth == 0);
  assert(reinterpret_cast<uintptr_t>(gfx.polyOpa.p) <=
         reinterpret_cast<uintptr_t>(gfx.polyOpa.d));
  assert(reinterpret_cast<uintptr_t>(gfx.polyXlu.p) <=
         reinterpret_cast<uintptr_t>(gfx.polyXlu.d));
  assert(gfx.polyOpa.p ==
         reinterpret_cast<Gfx *>(opaStorage.Head()) + opaqueCommands);
  assert(gfx.polyXlu.p ==
         reinterpret_cast<Gfx *>(xluStorage.Head()) + translucentCommands);
  assert(gfx.overlay.p == reinterpret_cast<Gfx *>(overlayStorage.Head()));
  int images = 0;
  for (Gfx *command = reinterpret_cast<Gfx *>(xluStorage.Head());
       command < gfx.polyXlu.p; ++command) {
    images += (command->words.w0 >> 24) == G_SETTIMG;
    if (reflection && (command->words.w0 >> 24) == G_SETTILESIZE) {
      assert(((command->words.w1 >> 12) & 4095) == 252); // (64 - 1) * 4
      assert((command->words.w1 & 4095) == 124);         // (32 - 1) * 4
    }
  }
  assert(images == (reflection ? 1 : 0));
  assert(submitted.size() == mesh.batchCount);
  for (int b = mesh.opaqueBatchCount; b < mesh.glassEndBatch; ++b)
    for (int v = 0; v < mesh.batches[b].vertexCount; ++v) {
      const auto &vertex = submitted[b][v].v;
      if (reflection) {
        // Textured scene pixels must retain their contrast and opacity.
        assert(vertex.cn[0] == 255 && vertex.cn[1] == 255 &&
               vertex.cn[2] == 255 && vertex.cn[3] == 255);
      }
      const auto &original = mesh.vertices[mesh.batches[b].firstVertex + v];
      assert(vertex.tc[0] ==
             static_cast<s16>(std::lround(ReflectionS(original.x) * 64 * 32)));
      assert(submitted[b][v].v.tc[0] >= 0 && submitted[b][v].v.tc[0] <= 2048);
      assert(submitted[b][v].v.tc[1] >= 0 && submitted[b][v].v.tc[1] <= 1024);
    }
  Guard(opaStorage, opaqueBytes);
  Guard(xluStorage, translucentBytes);
  Guard(overlayStorage, overlayBytes);
  std::printf(
      "Native mirror arenas: short data/OPA/XLU/overlay reject untouched; "
      "exact fit passes (%zu data bytes, %zu OPA / %zu XLU commands).\n",
      dataBytes, opaqueCommands, translucentCommands);
}
int main() {
  Run(false);
  Run(true);
}
