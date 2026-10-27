#include "actors/RegionalPassagePresentation.h"
#include "global.h"
#include "soh/soh/Enhancements/randomizer/NeiGiRender.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#ifdef REGIONAL_WAVE_BANK_TEST
#include "mods/sound_translator/mm_audio_sfx.h"
#endif

PlayState *gPlayState;
SaveContext gSaveContext{};
static bool endpointReady = true, arenaRoom = true, soundAvailable = true;
static int selectedRoute = COMBO_PORTAL_GREAT_BAY;
static Vec3f endpointAnchor{420, 80, -700};
static s16 endpointYaw = 8192;
static unsigned submissions, pushes, pops, shellQuads, textures, matrices,
    setups, projections, starts, stops, availabilityChecks;
static Vec3f origin, projectedInput;
static Vec3f *soundPointer;
static int waveId;
static std::string texture;
static NeiGi::Mesh submitted;
static unsigned coverageFailures;
static Gfx commands[4096];
static std::array<Gfx, 16> xluCommands, overlayCommands;
static size_t xluPeak;
static Player player;
static bool sWavesPlaying;
static Vec3f sWaveEmitter;
namespace RP = RegionalPassagePresentation;
#ifdef REGIONAL_WAVE_BANK_TEST
static signed char channelIO[16][8];
static unsigned noteTriggers;
static Vec3f *unrelatedEmitterPos;
extern "C" void lusprintf(const char *, int32_t, int32_t, const char *, ...) {}
extern "C" void Audio_SetVolScale(u8, u8, u8, u8) {}
extern "C" int MmSfxSynth_IsReady() { return 1; }
extern "C" void MmSfxSynth_SetChannelState(int, float, float frequency,
                                           signed char, signed char) {
  assert(frequency > 0);
}
extern "C" int MmSfxSynth_ReadChannelIO(int channel, int port) {
  return channelIO[channel][port];
}
extern "C" void MmSfxSynth_WriteChannelIO(int channel, int port,
                                          signed char value) {
  channelIO[channel][port] = value;
  if (port == 0 && value == 1)
    channelIO[channel][1] = 0;
  if (port == 4) {
    assert((unsigned char)value == 0xF2 &&
           "the native Sequence_0 must select the wave, not a waterfall");
    ++noteTriggers;
  }
  if (port == 5)
    assert(value == 1);
}
extern "C" s32 MmSfxBridge_Play(u16, f32, Vec3f *) {
  assert(false);
  return 0;
}
extern "C" void MmSfxBridge_RefreshProperties(u16, Vec3f *, f32) {
  assert(false);
}
extern "C" void MmSfxBridge_StopByMmSfxId(u16) { assert(false); }
extern "C" s32 MmSfxBridge_IsActive(u16) {
  assert(false);
  return 0;
}
bool HasWave(Vec3f *position) {
  for (u8 index = gMmSfxBanks[2][0].next; index != 0xFF;
       index = gMmSfxBanks[2][index].next)
    if (gMmSfxBanks[2][index].posX == &position->x &&
        gMmSfxBanks[2][index].sfxId == 0x20F2)
      return true;
  return false;
}
#endif

int Endpoint(PlayState *play, int *route, Vec3f *anchor, s16 *yaw) {
  assert(play == gPlayState);
  if (!endpointReady)
    return 0;
  *route = selectedRoute;
  *anchor = endpointAnchor;
  *yaw = endpointYaw;
  return 1;
}
extern "C" int ComboRegionalPassagesOot_Get(PlayState *play, int *route,
                                            Vec3f *anchor, s16 *yaw) {
  return Endpoint(play, route, anchor, yaw);
}
extern "C" int ComboRegionalPassagesMm_Get(PlayState *play, int *route,
                                           Vec3f *anchor, s16 *yaw) {
  return Endpoint(play, route, anchor, yaw);
}
extern "C" NeiGi::Basis NeiGi_CameraBasis(PlayState *) { return {}; }
extern "C" void NeiGi_DrawWorldParticles(PlayState *, const NeiGi::Mesh &mesh) {
  submitted = mesh;
  ++submissions;
}
/* NATIVE ARENA */
extern "C" bool NeiGi_CanDrawLayers(PlayState *play, size_t count, size_t opa,
                                    size_t xlu) {
  assert(count == 1);
#ifdef OOT_REGIONAL_TEST
  assert(opa == 0 && xlu == RP::kShellXluCommands &&
         "bridge providers own opaque geometry; common presenter draws effects "
         "only");
#else
  assert(opa >= 80);
#endif
  return arenaRoom && NativeCanDrawLayers(play, count, opa, xlu);
}
extern "C" s32 MmSfx_IsAvailable() {
  ++availabilityChecks;
  return soundAvailable;
}
extern "C" s32 MmSfx_PlayAtPos(u16 id, Vec3f *pos) {
  assert(pos && pos == &sWaveEmitter);
  assert(
      id == 0x20F2 &&
      "use the actual MM wave with native continuous-refresh flag semantics");
  waveId = id;
  soundPointer = pos;
  ++starts;
#ifdef REGIONAL_WAVE_BANK_TEST
  if (unrelatedEmitterPos)
    AudioMmSfx_PlaySfx(id, unrelatedEmitterPos, 4,
                       &gMmSfxDefaultFreqAndVolScale,
                       &gMmSfxDefaultFreqAndVolScale, &gMmSfxDefaultReverb);
  AudioMmSfx_PlaySfx(id, pos, 4, &gMmSfxDefaultFreqAndVolScale,
                     &gMmSfxDefaultFreqAndVolScale, &gMmSfxDefaultReverb);
  AudioMmSfx_ProcessRequests();
  AudioMmSfx_ProcessActiveSfx();
#endif
  return 1;
}
extern "C" void MmSfx_StopAtPos(u16 id, Vec3f *pos) {
  assert(pos == soundPointer && id == waveId &&
         "stop only this passage's retained emitter");
  ++stops;
#ifdef REGIONAL_WAVE_BANK_TEST
  AudioMmSfx_StopByPosAndId(pos, id);
#endif
}
void Project(PlayState *, Vec3f *world, Vec3f *result, f32 *inverseW) {
  ++projections;
  projectedInput = *world;
  *result = {-35, 12, 250};
  *inverseW = 1;
}
#undef GET_PLAYER
#define GET_PLAYER(play) (&::player)
#define Actor_ProjectPos Project
#define Matrix_Push() (++pushes)
#define Matrix_Pop() (++pops)
#define Matrix_Translate(x, y, z, mode)                                        \
  (origin = {float(x), float(y), float(z)})
#define Matrix_RotateYS(yaw, mode) assert((yaw) == endpointYaw)
#define Matrix_RotateY(yaw, mode)                                              \
  assert(std::fabs((yaw)-endpointYaw *(NeiGi::Tau / 65536.f)) < .00001f)
/* NATIVE GRAPH */
void RecordGraphPeak(GraphicsContext *gfx) {
  xluPeak = std::max(xluPeak, size_t(gfx->polyXlu.p - xluCommands.data()));
}
#undef OPEN_DISPS
#undef CLOSE_DISPS
#ifdef OOT_REGIONAL_TEST
#define OPEN_DISPS(ctx)                                                        \
  {                                                                            \
    GraphicsContext *__gfxCtx = (ctx);                                         \
    Gfx *dispRefs[4];                                                          \
    Graph_OpenDisps(dispRefs, ctx, __FILE__, __LINE__);                        \
    RecordGraphPeak(ctx);
#define CLOSE_DISPS(ctx)                                                       \
  Graph_CloseDisps(dispRefs, ctx, __FILE__, __LINE__);                         \
  }
Gfx sSetupDL[SETUPDL_MAX][6]{};
#else
#define OPEN_DISPS(ctx)                                                        \
  {                                                                            \
    GraphicsContext *__gfxCtx = (ctx);                                         \
    Gfx *dispRefs[3];                                                          \
    Gfx dispVals[3];                                                           \
    Graph_OpenDisps(dispRefs, dispVals, ctx, __FILE__, __LINE__);              \
    RecordGraphPeak(ctx);
#define CLOSE_DISPS(ctx)                                                       \
  Graph_CloseDisps(dispRefs, dispVals, ctx, __FILE__, __LINE__);               \
  }
Gfx gSetupDLs[SETUPDL_MAX][6]{};
#endif
// Setup's display-list target is a rendering boundary; emit its native packet
// while retaining the actual nested native Graph_Open/CloseDisps behavior.
#define gSPDisplayList __gSPDisplayList
/* NATIVE SETUP */
void CountSetup(GraphicsContext *ctx) {
  ++setups;
  NativeSetupOpa(ctx);
}
#define Gfx_SetupDL_25Opa CountSetup
#define Gfx_SetupDL25_Opa CountSetup
#undef MATRIX_FINALIZE_AND_LOAD
#define MATRIX_FINALIZE_AND_LOAD(packet, context) ((void)(packet), ++matrices)
#undef gSPMatrix
#define gSPMatrix(packet, ...) ((void)(packet), ++matrices)
#undef gSPComboRMPush
#define gSPComboRMPush(packet, owner) ((void)(packet))
#undef gSPComboRMPop
#define gSPComboRMPop(packet) ((void)(packet))
#undef gSPClearGeometryMode
#define gSPClearGeometryMode(packet, mode) ((void)(packet))
#undef gSPTexture
#define gSPTexture(packet, ...) ((void)(packet))
#undef gDPSetCombineMode
#define gDPSetCombineMode(packet, ...) ((void)(packet))
#undef gDPSetPrimColor
#define gDPSetPrimColor(packet, ...) ((void)(packet))
#undef gDPSetEnvColor
#define gDPSetEnvColor(packet, ...) ((void)(packet))
#undef gDPLoadTextureBlock
#define gDPLoadTextureBlock(packet, path, format, size, width, height, ...)    \
  ((void)(packet),                                                             \
   assert((width) == 32 && (height) == 32 && (format) == G_IM_FMT_RGBA &&      \
          (size) == G_IM_SIZ_16b),                                             \
   texture = (path), ++textures)
#undef gSPVertex
#define gSPVertex(packet, vertices, count, start)                              \
  ((void)(packet), assert((count) == 4 && (start) == 0),                       \
   assert(reinterpret_cast<const Vtx *>(vertices)[0].v.cn[3] == 255))
#undef gSP2Triangles
#define gSP2Triangles(packet, ...) ((void)(packet), ++shellQuads)
#undef gDPPipeSync
#define gDPPipeSync(packet) ((void)(packet))
/* PRODUCTION */

void ResetDraw() {
  submissions = pushes = pops = shellQuads = textures = matrices = setups = 0;
  texture.clear();
  submitted = {};
  xluPeak = 0;
}
void ResetArena(GraphicsContext &gfx, size_t xlu = 16) {
  std::memset(commands, 0xA5, sizeof(commands));
  std::memset(xluCommands.data(), 0xA5, sizeof(xluCommands));
  std::memset(overlayCommands.data(), 0xA5, sizeof(overlayCommands));
  gfx.polyOpa.p = commands;
  gfx.polyOpa.d = commands + 4096;
  gfx.polyXlu.p = xluCommands.data();
  gfx.polyXlu.d = xluCommands.data() + xlu;
  gfx.overlay.p = overlayCommands.data();
  gfx.overlay.d = overlayCommands.data() + 16;
}
void CheckCoverage(int route) {
  auto require = [](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "FAIL regional shimmer coverage: %s\n", message);
      ++coverageFailures;
    }
  };
#ifdef OOT_REGIONAL_TEST
  if (route == COMBO_PORTAL_IKANA) {
    unsigned crown = 0, side = 0, solidVertices = 0;
    // Independent native Bg_Haka stone bounds: object_haka, scale 0.1.
    // A center on the original y=2 aperture is hidden by the closed stone.
    for (size_t i = 0; i < submitted.count; ++i) {
      const auto p = submitted.vertices[i].p;
      solidVertices += std::fabs(p.x) < 34.6f && p.y >= 0 && p.y <= 48 &&
                       std::fabs(p.z) < 36;
      if (i % 72 == 0) {
        crown += p.y > 54;
        side += p.y < 48 && (std::fabs(p.x) > 40 || std::fabs(p.z) > 42);
      }
    }
    require(!solidVertices,
            "closed grave must not contain its own twinkle art");
    require(
        crown >= 8 && side >= 8,
        "closed native grave needs a visible crown and external side beacon");
    return;
  }
#endif
  float left = 1e6f, right = -1e6f, bottom = 1e6f, top = -1e6f;
  for (size_t i = 0; i < submitted.count; ++i) {
    const auto p = submitted.vertices[i].p;
    left = std::min(left, p.x);
    right = std::max(right, p.x);
    bottom = std::min(bottom, p.y);
    top = std::max(top, p.y);
  }
#ifdef OOT_REGIONAL_TEST
  // Hand-measured authored openings: Lake 180x216; Fountain 160x180.
  // These thresholds detect reusing the small WoM field at either endpoint.
  if (route == COMBO_PORTAL_GREAT_BAY)
    require(right - left > 170 && bottom < 10 && top > 205,
            "Lake twinkles must reach the complete 180x216 black face");
  else
    require(right - left > 150 && bottom < 10 && top > 170,
            "Snowhead twinkles must reach the complete 160x180 black face");
#else
  (void)route;
  require(right - left > 110 && bottom < 10 && top > 115,
          "MM twinkles must fit the complete native 120x126 arched face");
#endif
}
void AssertDraw(int route) {
  selectedRoute = route;
  ResetDraw();
  DrawRegionalPassage();
  assert(submissions == 1 && pushes == 1 && pops == 1);
  assert(submitted.count == 1440);
  CheckCoverage(route);
  assert(origin.x == 420 && origin.y == 80 && origin.z == -700);
#ifdef OOT_REGIONAL_TEST
  assert(shellQuads == 0 && textures == 0 && matrices == 0 && setups == 0 &&
         "fixed native bridge providers own geometry; common presenter must "
         "not add a second arch");

#else
  const char *expected =
      route == COMBO_PORTAL_SNOWHEAD
          ? "__OTR__objects/object_icefloe/gIcefloePlatformTex"
          : "__OTR__objects/gameplay_keep/gBoulderFragmentsTex";
  assert(shellQuads == 32 && textures == 1 && matrices == 1 && setups == 2);
  assert(texture == expected);
#endif
}

int main() {
  PlayState play{};
  GraphicsContext gfx{};
  gPlayState = &play;
  play.state.gfxCtx = &gfx;
  play.state.running = true;
  ResetArena(gfx);
  play.gameplayFrames = 75;
#ifdef REGIONAL_WAVE_BANK_TEST
  AudioMmSfx_Reset();
  std::memset(channelIO, -1, sizeof(channelIO));
  Vec3f unrelatedEmitter{40, 0, 120};
  unrelatedEmitterPos = &unrelatedEmitter;
#endif
  for (int route : {5, 6, 7})
    AssertDraw(route);
  assert(!coverageFailures &&
         "the source-produced field must cover each visible entrance");
  for (int blocked = 0; blocked < 8; ++blocked) {
    endpointReady = blocked != 0;
    play.state.running = blocked != 1;
    play.pauseCtx.state = blocked == 2 ? 1 : 0;
    play.csCtx.state = blocked == 3 ? 1 : CS_STATE_IDLE;
    play.transitionTrigger =
        blocked == 4 ? TRANS_TRIGGER_START : TRANS_TRIGGER_OFF;
    play.transitionMode = blocked == 5 ? TRANS_MODE_SETUP : TRANS_MODE_OFF;
    arenaRoom = blocked != 6;
    selectedRoute = blocked == 7 ? 0 : COMBO_PORTAL_GREAT_BAY;
    ResetDraw();
    DrawRegionalPassage();
    assert(!submissions && !shellQuads && !pushes);
  }
  endpointReady = arenaRoom = true;
  selectedRoute = COMBO_PORTAL_GREAT_BAY;
  play.state.running = true;
  play.pauseCtx.state = 0;
  play.csCtx.state = CS_STATE_IDLE;
  play.transitionTrigger = TRANS_TRIGGER_OFF;
  play.transitionMode = TRANS_MODE_OFF;
  // Native OPEN_DISPS writes all three arenas before CLOSE_DISPS can rewind
  // unused heads. Zero/tight XLU must reject the whole optional draw first.
  for (size_t available = 0; available < 8; ++available) {
    ResetArena(gfx, available);
    ResetDraw();
    const auto xluBefore = xluCommands, overlayBefore = overlayCommands;
    std::array<Gfx, 4096> opaBefore;
    std::memcpy(opaBefore.data(), commands, sizeof(commands));
    DrawRegionalPassage();
    assert(!submissions && !pushes && !shellQuads && !xluPeak &&
           "reject before native debug nodes transiently overrun the XLU tail");
    assert(!std::memcmp(opaBefore.data(), commands, sizeof(commands)));
    assert(!std::memcmp(xluBefore.data(), xluCommands.data(),
                        sizeof(xluCommands)));
    assert(!std::memcmp(overlayBefore.data(), overlayCommands.data(),
                        sizeof(overlayCommands)));
  }
  ResetArena(gfx, 8);
  AssertDraw(COMBO_PORTAL_GREAT_BAY);
#ifdef OOT_REGIONAL_TEST
  assert(xluPeak == 0 && gfx.polyXlu.p == xluCommands.data() &&
         "effects presenter must not submit opaque bridge packets");
#else
  assert(xluPeak == 2 && gfx.polyXlu.p == xluCommands.data() &&
         "nested native Opa setup uses XLU packets despite the final rewind");
#endif
  assert(gfx.polyOpa.p <= gfx.polyOpa.d && gfx.overlay.p <= gfx.overlay.d);
#ifdef OOT_REGIONAL_TEST
  player.actor.world.pos = {420, 80, -600};
  UpdateRegionalWaves();
  UpdateRegionalWaves();
  assert(starts == 2 && projections == 2 && !stops && sWavesPlaying);
  assert(projectedInput.x == 420 && projectedInput.y == 80 &&
         projectedInput.z == -700);
  assert(soundPointer->x == -35 && soundPointer->y == 12 &&
         soundPointer->z == 250);
#ifdef REGIONAL_WAVE_BANK_TEST
  assert(HasWave(&sWaveEmitter) && HasWave(&unrelatedEmitter) &&
         noteTriggers == 2 &&
         "continuous updates must refresh existing native notes");
#endif
  for (int blocked = 0; blocked < 7; ++blocked) {
    endpointReady = blocked != 0;
    play.pauseCtx.state = blocked == 1 ? 1 : 0;
    player.actor.world.pos.x = blocked == 2 ? 1100 : 420;
    player.actor.world.pos.y = blocked == 3 ? 400 : 80;
    soundAvailable = blocked != 4;
    selectedRoute =
        blocked == 5 ? COMBO_PORTAL_SNOWHEAD : COMBO_PORTAL_GREAT_BAY;
    play.transitionTrigger =
        blocked == 6 ? TRANS_TRIGGER_START : TRANS_TRIGGER_OFF;
    const auto previousStarts = starts, previousStops = stops,
               previousAvailability = availabilityChecks;
    UpdateRegionalWaves();
    UpdateRegionalWaves();
    assert(starts == previousStarts && stops == previousStops + 1 &&
           !sWavesPlaying);
    if (blocked != 4)
      assert(availabilityChecks == previousAvailability &&
             "ineligible endpoints must not touch foreign audio");
#ifdef REGIONAL_WAVE_BANK_TEST
    assert(!HasWave(&sWaveEmitter) && HasWave(&unrelatedEmitter));
#endif
    endpointReady = soundAvailable = true;
    play.pauseCtx.state = 0;
    player.actor.world.pos = {420, 80, -600};
    selectedRoute = COMBO_PORTAL_GREAT_BAY;
    play.transitionTrigger = TRANS_TRIGGER_OFF;
    UpdateRegionalWaves();
    assert(sWavesPlaying);
  }
  StopRegionalWaves();
  const auto previousStops = stops;
  StopRegionalWaves();
  assert(stops == previousStops && !sWavesPlaying);
#ifdef REGIONAL_WAVE_BANK_TEST
  assert(!HasWave(&sWaveEmitter) && HasWave(&unrelatedEmitter));
  AudioMmSfx_StopByPosAndId(&unrelatedEmitter, 0x20F2);
#endif
  gPlayState = nullptr;
  ResetDraw();
  DrawRegionalPassage();
  UpdateRegionalWaves();
  assert(!submissions);
#endif
  puts("PASS native regional presentation: exact active endpoint, stock "
       "textures, bounded shell, guards and wave emitter lifecycle");
}
