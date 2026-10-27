#include "2s2h/Enhancements/Audio/MMWeather.h"
#include "2s2h/Enhancements/Audio/MMWeatherAudio.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include "2s2h/Enhancements/Graphics/MMSummerAtmosphere.h"
#include "mods/extended_inventory.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include "overlays/actors/ovl_En_Test4/z_en_test4.h"
#include "overlays/actors/ovl_En_Weather_Tag/z_en_weather_tag.h"
#include "overlays/actors/ovl_Object_Kankyo/z_object_kankyo.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "ComboGameMode.h"
#include "entrances/SoloForest.h"
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }

void MMAutumnSceneFoliage_Update(const PlayState *) {}
void MMAutumnSceneFoliage_Reset() {}

static_assert(PRECIP_RAIN_MAX == 0 && PRECIP_RAIN_CUR == 1 &&
              PRECIP_SNOW_CUR == 2 && PRECIP_SNOW_MAX == 3);
static_assert(WEATHER_MODE_RAIN == 1 && WEATHER_MODE_SNOW == 3 &&
              SEASON_WINTER == 3);

SaveContext gSaveContext{};
PlayState *gPlayState;
RegEditor editor{};
RegEditor *gRegEditor = &editor;
u8 gWeatherMode;
u8 gInterruptSongOfStorms, gLightConfigAfterUnderwater;
u8 D_801BDBC0, D_801BDBC4, D_801F4E30;
u8 sSeqCmdWritePos;
u32 sAudioSeqCmds[256];
static f32 D_808DE5B0;
static u16 D_808DE340;
static int snowDraws, rainDraws, spawns, refreshes, seasonFlashes;
static int gameplayRandomCalls;
static int springBlossoms = 1, autumnLeaves = 1, winterSnow = 1,
           winterFlurries = 0;
static int independentRain = 0;
static unsigned leafPalettes, leafDraws, firstLeafAlpha, nonzeroLeafDraws,
    petalDraws, blossomDraws;
static u8 submittedLeafAlpha[96];
static float rainGain;
static u8 nativeRainAmbience, nativeThunderAmbience;
static ObjectKankyo supplemental[128];
static bool perspectiveProjection;
static bool forceParticleCulled;
#include "native_particle_setup.inc"
static float aspectRatio = 4.f / 3.f;
static FILE *previewFile;
static bool firstPreviewLeaf;
static Vec3f previewPosition;
static float previewScaleX = 1, previewScaleY = 1, previewRotation;
static const Gfx *previewCommandsBegin;
static int previewIndex, previewEpoch;
static void TracePreviewLeaf(const Gfx *list);
static void TraceParticleDisplayList(const Gfx *list);

// A real perspective frustum catches leaves wasted behind the view. The older
// lifecycle tests intentionally retain their all-visible submission boundary.
static void ProjectLeaf(PlayState *play, const Vec3f &world, Vec3f *screen) {
  Vec3f up = play->view.up;
  if (SQ(up.x) + SQ(up.y) + SQ(up.z) < .001f)
    up = {0, 1, 0};
  float native[4][4];
  guLookAtF(native, play->view.eye.x, play->view.eye.y, play->view.eye.z,
            play->view.at.x, play->view.at.y, play->view.at.z, up.x, up.y,
            up.z);
  const float x = world.x * native[0][0] + world.y * native[1][0] +
                  world.z * native[2][0] + native[3][0];
  const float y = world.x * native[0][1] + world.y * native[1][1] +
                  world.z * native[2][1] + native[3][1];
  const float depth = -(world.x * native[0][2] + world.y * native[1][2] +
                        world.z * native[2][2] + native[3][2]);
  const Camera *camera = GET_ACTIVE_CAM(play);
  const float fov = play->view.fovy > 0         ? play->view.fovy
                    : camera && camera->fov > 0 ? camera->fov
                                                : 60.f;
  const float focal = 120.f / std::tan(fov * 3.14159265f / 360.f);
  if (depth <= 0) {
    *screen = {-10000, -10000, depth};
    return;
  }
  *screen = {160.f + x * focal / depth, 120.f - y * focal / depth, depth};
}

extern "C" {
// Summer has its own production-state/renderer harness; these calls are outside
// this native seasonal-particle fixture's draw/update boundary.
void MMSummerAtmosphere_Update(PlayState *) {}
void MMSummerAtmosphere_Reset() {}
void MMWinterAtmosphere_Reset() {}
int32_t CVarGetInteger(const char *name, int32_t fallback) {
  if (!std::strcmp(name, "gAudioEditor.MMWeather.Enabled"))
    return independentRain;
  if (!std::strcmp(name, "gAudioEditor.MMWeather.Spring.Blossoms"))
    return springBlossoms;
  if (!std::strcmp(name, "gAudioEditor.MMWeather.Autumn.Leaves"))
    return autumnLeaves;
  if (!std::strcmp(name, "gAudioEditor.MMWeather.Winter.Snow"))
    return winterSnow;
  if (!std::strcmp(name, "gAudioEditor.MMWeather.Winter.Flurries"))
    return winterFlurries;
  return fallback;
}
Color_RGBA8 CVarGetColor(const char *, Color_RGBA8 fallback) {
  return fallback;
}
void MMWeatherAudio_Reset() { rainGain = 0; }
void MMWeatherAudio_SetRain(float gain) { rainGain = gain; }
void MMWeatherAudio_Thunder(float) {}
void MMWeather_ClearBolts() {}
void MMWeather_StartBolt() {}
void Environment_DrawLightningFlash(PlayState *, u8, u8, u8, u8) {}
void Audio_SetAmbienceChannelIO(u8 channel, u8 port, u8 value) {
  assert(port == CHANNEL_IO_PORT_1);
  SEQCMD_SET_CHANNEL_IO(SEQ_PLAYER_AMBIENCE, channel, port, value);
  value = sAudioSeqCmds[(u8)(sSeqCmdWritePos - 1)] & 0xFF;
  if (channel == AMBIENCE_CHANNEL_RAIN)
    nativeRainAmbience = value;
  else if (channel == AMBIENCE_CHANNEL_LIGHTNING)
    nativeThunderAmbience = value;
  else
    assert(false);
}
void Environment_PlayStormNatureAmbience(PlayState *) {
  Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_RAIN, CHANNEL_IO_PORT_1, 1);
  Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_LIGHTNING, CHANNEL_IO_PORT_1, 1);
}
void Environment_StopStormNatureAmbience(PlayState *) {
  Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_RAIN, CHANNEL_IO_PORT_1, 0);
  Audio_SetAmbienceChannelIO(AMBIENCE_CHANNEL_LIGHTNING, CHANNEL_IO_PORT_1, 0);
}
void Environment_WipeRumbleRequests() {}
void Environment_UpdateSkyboxRotY(PlayState *) {}
void Environment_UpdateTimeBasedSequence(PlayState *) {}
void Environment_UpdateNextDayTime() {}
void Environment_UpdateTime(PlayState *, EnvironmentContext *, PauseContext *,
                            MessageContext *, GameOverContext *) {}
void Environment_UpdateSun(PlayState *) {}
void Environment_UpdateLights(PlayState *, EnvironmentContext *,
                              LightContext *) {}
void Environment_UpdatePostmanEvents(PlayState *) {}
void Environment_DrawRainImpl(PlayState *, View *, GraphicsContext *) {
  ++rainDraws;
}
u32 Environment_GetStormState(PlayState *play) {
  return play->envCtx.stormState;
}
NeiSaveData *Nei_Save() { return &gSaveContext.save.shipSaveInfo.nei; }
u16 Nei_GetOwnedItem(u8 slot) { return Nei_Save()->ownedItems[slot - 24]; }
u8 Seasons_RandoMode() { return NEI_SEASONS_INDIVIDUAL; }
void Seasons_UpdateGates() {}
void ExtInv_RefreshButtonIconsForItem(PlayState *, u16 item) {
  assert(item == EXT_ITEM_ROD_OF_SEASONS);
  ++refreshes;
}
// Pulse rendering has its own native selector fixture; retain its callback
// boundary here.
void MM_BoxMenu_StartSeasonFlash(PlayState *play) {
  assert(play == gPlayState && play->pauseCtx.state == PAUSE_STATE_OFF);
  ++seasonFlashes;
}
void Actor_Kill(Actor *actor) { actor->update = actor->draw = nullptr; }
f32 Rand_ZeroOne() {
  ++gameplayRandomCalls;
  return 0.25f;
}
s16 Camera_GetCamDirPitch(Camera *) { return 0; }
f32 Math_Vec3f_DistXZ(Vec3f *a, Vec3f *b) {
  return sqrtf(SQ(a->x - b->x) + SQ(a->z - b->z));
}
f32 Math_Vec3f_DistXYZ(Vec3f *a, Vec3f *b) {
  return sqrtf(SQ(a->x - b->x) + SQ(a->y - b->y) + SQ(a->z - b->z));
}
void func_808DBE8C(ObjectKankyo *) {}
void func_808DC038(ObjectKankyo *, PlayState *) {}
void ObjectKankyo_Init(Actor *, PlayState *);
void ObjectKankyo_Update(Actor *, PlayState *);
void func_808DD3C8(Actor *, PlayState *);
Actor *Actor_Spawn(ActorContext *context, PlayState *play, s16 id, f32, f32,
                   f32, s16, s16, s16, s32 params) {
  assert(spawns < 128 && id == ACTOR_OBJECT_KANKYO && params == 1);
  ObjectKankyo *snow = &supplemental[spawns++];
  *snow = {};
  snow->actor.id = id;
  snow->actor.params = params;
  snow->actor.update = ObjectKankyo_Update;
  snow->actor.draw = func_808DD3C8;
  ObjectKankyo_Init(&snow->actor, play);
  snow->actor.next = context->actorLists[ACTORCAT_ITEMACTION].first;
  context->actorLists[ACTORCAT_ITEMACTION].first = &snow->actor;
  return &snow->actor;
}
void Play_GetScreenPos(PlayState *play, Vec3f *world, Vec3f *screen) {
  if (forceParticleCulled)
    *screen = {-10000, -10000, 1};
  else if (perspectiveProjection)
    ProjectLeaf(play, *world, screen);
  else
    *screen = {100, 100, 1};
}
void Matrix_Translate(f32 x, f32 y, f32 z, MatrixMode) {
  previewPosition = {x, y, z};
  previewScaleX = previewScaleY = 1;
  previewRotation = 0;
}
void Matrix_Scale(f32 x, f32 y, f32, MatrixMode) {
  previewScaleX *= x;
  previewScaleY *= y;
}
void Matrix_Mult(MtxF *, MatrixMode) {}
void Matrix_RotateZS(s16 angle, MatrixMode) {
  previewRotation = angle * 6.2831853f / 65536.f;
}
Mtx *Matrix_Finalize(GraphicsContext *) {
  static Mtx matrix;
  return &matrix;
}
Gfx *Gfx_SetupDL(Gfx *gfx, u32 setup) {
  assert(setup == SETUPDL_0);
  __gSPDisplayList(gfx++, sNativeParticleSetup);
  return gfx;
}
void *Lib_SegmentedToVirtual(void *resource) { return resource; }
s32 func_80173B48(GameState *) { return 14000000; }
f32 Math_SmoothStepToF(f32 *value, f32 target, f32, f32, f32) {
  *value = target;
  return 0;
}
void FrameInterpolation_RecordOpenChild(const void *particle, int epoch) {
  if (previewFile) {
    const auto *actor =
        (ObjectKankyo *)gPlayState->actorCtx.actorLists[ACTORCAT_ITEMACTION]
            .first;
    previewIndex = ((uintptr_t)particle - (uintptr_t)&actor->unk_14C[0]) /
                   sizeof(ObjectKankyoStruct);
    previewEpoch = epoch;
  }
}
void FrameInterpolation_RecordCloseChild() {}
f32 Actor_WorldDistXZToActor(Actor *a, Actor *b) {
  return Math_Vec3f_DistXZ(&a->world.pos, &b->world.pos);
}
void func_80966E0C(EnWeatherTag *, PlayState *) {}
float OTRGetAspectRatio() { return aspectRatio; }
float OTRGetDimensionFromLeftEdge(float value) {
  return value - (240.f * aspectRatio - 320.f) / 2.f;
}
float OTRGetDimensionFromRightEdge(float value) {
  return value + (240.f * aspectRatio - 320.f) / 2.f;
}
}

// Keep native GBI writes and snow renderer execution; resource submission,
// projection and frame interpolation are the asset-free engine boundaries.
#undef OPEN_DISPS_PORT_HELPERS
#undef CLOSE_DISPS_PORT_HELPERS
#define OPEN_DISPS_PORT_HELPERS(gfxCtx)
#define CLOSE_DISPS_PORT_HELPERS(gfxCtx)
#undef gSPDisplayList
#define gSPDisplayList(pkt, dl)                                                \
  do {                                                                         \
    ++snowDraws;                                                               \
    TraceParticleDisplayList((const Gfx *)(dl));                               \
    if (previewFile)                                                           \
      TracePreviewLeaf((const Gfx *)(dl));                                     \
    __gSPDisplayList(pkt, (Gfx *)(dl));                                        \
  } while (0)
#define gSPSegment(pkt, segment, resource)                                     \
  __gSPSegment(pkt, segment, (uintptr_t)(resource))
#define Lib_SegmentedToVirtual(resource) ((void *)(resource))
#include "mods/items/helpers/season_particles.h"
#include "mods/items/objects/object_autumn_leaves.h"
#include "mods/items/objects/object_spring_petals.h"

static void TraceParticleDisplayList(const Gfx *list) {
  if (list == sSpringPetalGeometry || list == sSpringBlossomGeometry)
    ++petalDraws;
  if (list == sSpringBlossomGeometry)
    ++blossomDraws;
}

static void TracePreviewLeaf(const Gfx *list) {
  if (list != sAutumnLeafGeometry)
    return;
  Vec3f screen;
  ProjectLeaf(gPlayState, previewPosition, &screen);
  if (screen.z <= 0)
    return;
  unsigned alpha = 0, palette = 0;
  const auto *end = gPlayState->state.gfxCtx->polyXlu.p;
  for (const Gfx *command = previewCommandsBegin; command < end; ++command) {
    if ((command->words.w0 >> 24) == G_SETPRIMCOLOR)
      alpha = command->words.w1 & 255;
    if ((command->words.w0 >> 24) == G_SETTIMG)
      for (unsigned i = 0; i < 4; ++i)
        if (command->words.w1 == (uintptr_t)sAutumnLeafTextures[i])
          palette = i;
  }
  if (!alpha)
    return;
  const float focal = 120.f / std::tan(60.f * 3.14159265f / 360.f);
  const float width = 240.f * aspectRatio;
  std::fprintf(previewFile, "%s[%.4f,%.4f,%.5f,%.5f,%.3f,%u,%u,%d,%d]",
               firstPreviewLeaf ? "" : ",",
               (screen.x + (width - 320.f) / 2.f) / width, screen.y / 240.f,
               320.f * previewScaleX * focal / (screen.z * width),
               320.f * previewScaleY * focal / (screen.z * 240.f),
               previewRotation, alpha, palette, previewIndex, previewEpoch);
  firstPreviewLeaf = false;
}

typedef void (*BoxMenuConfirmFn)(s32);
static u8 sBoxMOpen, sBoxMHoldSeen, sBoxMStickHeld;
static s16 sBoxMPulse;
static u16 sBoxMHoldButton;
static s32 sBoxMCursor, sBoxMCount;
static BoxMenuConfirmFn sBoxMOnConfirm;
#define BOXM_STICK_DEAD 30
static void BoxMenu_PlaySfx(u16) {}
static s32 BoxMenu_Step(s32 from, s32) { return from; }
extern "C" {
#include "native_weather.inc"
}

static void Reset(PlayState *play, Camera *camera, GraphicsContext *gfx) {
  MMWeather_Reset();
  *play = {};
  *camera = {};
  play->sceneId = SCENE_TOWN;
  play->skyboxId = SKYBOX_NORMAL_SKY;
  play->cameraPtrs[0] = camera;
  play->view.at.z = 1;
  play->state.gfxCtx = gfx;
  play->envCtx.lightSettingOverride = LIGHT_SETTING_OVERRIDE_NONE;
  play->envCtx.stormState = STORM_STATE_ON;
  gSaveContext = {};
  gSaveContext.gameMode = GAMEMODE_NORMAL;
  gSaveContext.save.day = 1;
  gSaveContext.save.time = CLOCK_TIME(12, 0);
  gSaveContext.save.shipSaveInfo.nei.seasonsOwned = 0x0F;
  gSaveContext.save.shipSaveInfo.nei.season = SEASON_OFF;
  gPlayState = play;
  D_801F4E30 = 0;
  gWeatherMode = WEATHER_MODE_CLEAR;
  AudioSeq_QueueSeqCmd((SEQCMD_OP_PLAY_SEQUENCE << 28) |
                       (SEQ_PLAYER_AMBIENCE << 24) | NA_BGM_AMBIENCE);
  nativeRainAmbience = nativeThunderAmbience = 0;
  // Same storage, scene and gameplay frame rewind must reset rod ownership.
  play->gameplayFrames = 0;
  Environment_Update(play, &play->envCtx, &play->lightCtx, &play->pauseCtx,
                     &play->msgCtx, &play->gameOverCtx, gfx);
}

static void EnvironmentFrame(PlayState *play) {
  Environment_Update(play, &play->envCtx, &play->lightCtx, &play->pauseCtx,
                     &play->msgCtx, &play->gameOverCtx, play->state.gfxCtx);
}

static void Frame(PlayState *play, EnTest4 *dayActor = nullptr) {
  ++play->state.frames;
  ++play->gameplayFrames;
  if (dayActor) {
    if (dayActor->weather == THREEDAY_WEATHER_RAIN)
      EnTest4_UpdateWeatherRainy(dayActor, play);
    else
      EnTest4_UpdateWeatherClear(dayActor, play);
  }
  for (Actor *actor = play->actorCtx.actorLists[ACTORCAT_ITEMACTION].first;
       actor; actor = actor->next) {
    if (actor->update)
      actor->update(actor, play);
  }
  EnvironmentFrame(play);
}

static void Confirm(PlayState *play, u8 season) {
  // Play_Update's paused wheel update precedes Environment_Update, and does
  // not run Player_Update or advance gameplayFrames on the confirm frame.
  const u32 frame = play->gameplayFrames;
  const int previous = Seasons_GetSeason();
  const int flashes = seasonFlashes;
  sBoxMOpen = 1;
  sBoxMCursor = season;
  sBoxMCount = 5;
  sBoxMOnConfirm = Seasons_OnWheelConfirm;
  play->pauseCtx.state = PAUSE_STATE_MAIN;
  play->state.input[0].press.button = BTN_A;
  BoxMenu_Update(play);
  assert(!sBoxMOpen && play->pauseCtx.state == PAUSE_STATE_OFF);
  assert(seasonFlashes == flashes + (Seasons_GetSeason() != previous));
  EnvironmentFrame(play);
  assert(play->gameplayFrames == frame);
}

static int DrawSnow(PlayState *play) {
  static Gfx commands[2048];
  previewCommandsBegin = commands;
  play->state.gfxCtx->polyXlu.p = commands;
  int before = snowDraws;
  leafPalettes = leafDraws = nonzeroLeafDraws = petalDraws = blossomDraws = 0;
  std::memset(submittedLeafAlpha, 0, sizeof(submittedLeafAlpha));
  firstLeafAlpha = 0;
  for (Actor *actor = play->actorCtx.actorLists[ACTORCAT_ITEMACTION].first;
       actor; actor = actor->next) {
    if (actor->draw)
      actor->draw(actor, play);
  }
  bool havePrim = false;
  unsigned currentAlpha = 0;
  for (const Gfx *command = commands; command < play->state.gfxCtx->polyXlu.p;
       ++command) {
    if ((command->words.w0 >> 24) == G_SETPRIMCOLOR)
      currentAlpha = command->words.w1 & 255;
    if ((command->words.w0 >> 24) == G_SETPRIMCOLOR && !havePrim) {
      firstLeafAlpha = command->words.w1 & 255;
      havePrim = true;
    }
    if ((command->words.w0 >> 24) == G_SETTIMG) {
      for (unsigned palette = 0; palette < 4; ++palette) {
        if (command->words.w1 == (uintptr_t)sAutumnLeafTextures[palette]) {
          assert(((command->words.w0 >> 19) & 3) == G_IM_SIZ_32b);
          leafPalettes |= 1 << palette;
          assert(leafDraws < 96);
          submittedLeafAlpha[leafDraws] = currentAlpha;
          nonzeroLeafDraws += currentAlpha > 0;
          ++leafDraws;
        }
      }
    }
  }
  return snowDraws - before;
}

static void AutumnCoverageRegression() {
  static PlayState play;
  Camera camera{};
  GraphicsContext gfx{};
  Reset(&play, &camera, &gfx);
  play.sceneId = SCENE_00KEIKOKU;
  camera.fov = 60;
  Confirm(&play, SEASON_AUTUMN);
  for (int frame = 0; frame < 30; ++frame)
    Frame(&play);
  auto *actor =
      (ObjectKankyo *)play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first;
  unsigned near = 0, middle = 0, far = 0;
  for (const auto &p : actor->unk_14C) {
    if (!p.unk_1C)
      continue;
    Vec3f pos{p.unk_00 + p.unk_0C, p.unk_04 + p.unk_10, p.unk_08 + p.unk_14};
    const float distance = Math_Vec3f_DistXZ(&pos, &play.view.eye);
    near += distance < 1200;
    middle += distance >= 1200 && distance < 3200;
    far += distance >= 3200;
  }
  assert(near && middle &&
         far); // The old snow bubble never reaches the middle/background.
  assert(near + middle + far ==
         96); // Keep the shared native allocation bounded.
  auto first = actor->unk_14C[0];
  play.view.eye.x += 30;
  play.view.at = {30, 0,
                  -1}; // Turn the view without dragging existing leaves along.
  Frame(&play);
  assert(actor->unk_14C[0].epoch == first.epoch);
  assert(fabsf(actor->unk_14C[0].unk_00 - first.unk_00) < 2);
  assert(fabsf(actor->unk_14C[0].unk_08 - first.unk_08) < 2);
  assert(DrawSnow(&play) <= 128);
  // A background leaf must retain visible alpha rather than inherit the snow's
  // 300-unit fade.
  actor->unk_14C[0].unk_00 = actor->unk_14C[0].unk_0C =
      actor->unk_14C[0].unk_10 = actor->unk_14C[0].unk_14 = 0;
  actor->unk_14C[0].unk_04 = 0;
  actor->unk_14C[0].unk_18 = 30;
  for (float distance : {1500.0f, 6000.0f}) {
    actor->unk_14C[0].unk_08 = distance;
    DrawSnow(&play);
    assert(firstLeafAlpha > 100);
  }
  Player link{};
  link.actor.world.pos.z = actor->unk_14C[0].unk_08;
  play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &link.actor;
  DrawSnow(&play);
  assert(firstLeafAlpha ==
         0); // The clear pocket follows Link, even after camera/wind movement.
  play.actorCtx.actorLists[ACTORCAT_PLAYER].first = nullptr;
  std::puts("PASS autumn foreground/midground/background coverage, world "
            "stability and bounded rendering");
}

static unsigned VisibleDistantLeaves(ObjectKankyo *actor, PlayState *play) {
  unsigned count = 0;
  for (int i = 16; i < 96; ++i) {
    const auto &p = actor->unk_14C[i];
    Vec3f screen;
    ProjectLeaf(play,
                {p.unk_00 + p.unk_0C, p.unk_04 + p.unk_10, p.unk_08 + p.unk_14},
                &screen);
    count += screen.z > 0 && screen.x >= 0 && screen.x < 320 && screen.y >= 0 &&
             screen.y < 240;
  }
  return count;
}

static void VisibleAutumnRegression() {
  static PlayState play;
  Camera camera{};
  GraphicsContext gfx{};
  perspectiveProjection = true;
  for (int scene : {SCENE_00KEIKOKU, SCENE_TOWN, SCENE_ICHIBA, SCENE_BACKTOWN,
                    SCENE_CLOCKTOWER, SCENE_ALLEY}) {
    Reset(&play, &camera, &gfx);
    play.sceneId = scene;
    camera.fov = 60;
    Confirm(&play, SEASON_AUTUMN);
    for (int frame = 0; frame < 30; ++frame) {
      Frame(&play);
      DrawSnow(&play);
    }
    auto *actor =
        (ObjectKankyo *)play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first;
    assert(actor && actor->actor.update);
    const unsigned visible = VisibleDistantLeaves(actor, &play);
    std::printf("Scene %d: %u distant leaves in the perspective view\n", scene,
                visible);
    std::fflush(stdout);
    assert(visible >=
           40); // A nominal budget scattered behind the camera is insufficient.
    const auto first = actor->unk_14C[16];
    for (int frame = 0; frame < 20; ++frame)
      Frame(&play);
    const auto &moved = actor->unk_14C[16];
    assert(moved.epoch == first.epoch && first.unk_10 - moved.unk_10 >= 45.f);
    if (scene != SCENE_00KEIKOKU) {
      for (int i = 16; i < 96; ++i) {
        const auto &p = actor->unk_14C[i];
        Vec3f position{p.unk_00 + p.unk_0C, p.unk_04 + p.unk_10,
                       p.unk_08 + p.unk_14};
        assert(Math_Vec3f_DistXZ(&position, &play.view.eye) < 2000.f);
      }
    }
    perspectiveProjection = true;
    assert(DrawSnow(&play) <= 96);
    for (int frame = 0; frame < 600; ++frame) {
      Frame(&play);
      DrawSnow(&play); // The actual draw sees this frame's camera matrices and
                       // recycles offscreen leaves.
      if (frame % 60 == 0) {
        const unsigned live = VisibleDistantLeaves(actor, &play);
        std::printf("Scene %d at tick %d: %u visible distant leaves\n", scene,
                    frame, live);
        std::fflush(stdout);
        assert(live >=
               40); // Compact streets must stay populated after the first fall.
      }
    }
  }
  perspectiveProjection = false;
  std::puts("PASS visible moving autumn leaves in the field and all five "
            "outdoor Clock Town districts");
}

static void ExportAutumnPreview(const char *path) {
  static PlayState play;
  Camera camera{};
  GraphicsContext gfx{};
  Reset(&play, &camera, &gfx);
  play.sceneId = SCENE_00KEIKOKU;
  camera.fov = 60;
  play.envCtx.windDirection.x = -100;
  play.envCtx.windSpeed = 20;
  Confirm(&play, SEASON_AUTUMN);
  perspectiveProjection = true;
  aspectRatio = 16.f / 9.f;
  for (int frame = 0; frame < 30; ++frame)
    Frame(&play);
  previewFile = std::fopen(path, "w");
  assert(previewFile);
  std::fputs("{\"fps\":10,\"frames\":[", previewFile);
  for (int frame = 0; frame < 240; ++frame) {
    Frame(&play);
    if (frame % 2 != 0)
      continue;
    std::fputs(frame ? ",[" : "[", previewFile);
    firstPreviewLeaf = true;
    DrawSnow(&play);
    std::fputc(']', previewFile);
  }
  std::fputs("]}", previewFile);
  std::fclose(previewFile);
  previewFile = nullptr;
  std::puts("PASS exported twelve seconds of production leaf motion and draw "
            "transforms");
}

static void AllAutumnSlotsVisibleRegression() {
  static PlayState play;
  Camera camera{};
  GraphicsContext gfx{};
  perspectiveProjection = true;
  for (int scene : {SCENE_00KEIKOKU, SCENE_TOWN, SCENE_ICHIBA, SCENE_BACKTOWN,
                    SCENE_CLOCKTOWER, SCENE_ALLEY}) {
    Reset(&play, &camera, &gfx);
    Confirm(&play, SEASON_AUTUMN);
    play.sceneId = scene;
    Frame(&play);
    auto *actor =
        (ObjectKankyo *)play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first;
    assert(actor);
    unsigned totalRecycled = 0;
    for (float aspect : {4.f / 3.f, 16.f / 9.f, 21.f / 9.f}) {
      aspectRatio = aspect;
      const float left = -(240.f * aspect - 320.f) / 2.f;
      const float right = 320.f - left;
      for (float fov : {35.f, 60.f, 90.f}) {
        camera.fov = play.view.fovy = fov;
        play.view.at.y = fov == 35.f ? .4f : fov == 90.f ? -.4f : 0.f;
        for (int frame = 0; frame < 180; ++frame) {
          Frame(&play);
          Vec3f previous[96];
          s16 epochs[96];
          for (int i = 0; i < 96; ++i) {
            const auto &p = actor->unk_14C[i];
            ProjectLeaf(
                &play,
                {p.unk_00 + p.unk_0C, p.unk_04 + p.unk_10, p.unk_08 + p.unk_14},
                &previous[i]);
            epochs[i] = p.epoch;
          }
          const int drawn = DrawSnow(&play);
          if (drawn != 96) {
            std::printf(
                "Scene %d aspect %.3f FOV %.0f: %d of 96 leaves submitted\n",
                scene, aspect, fov, drawn);
            std::fflush(stdout);
          }
          assert(drawn == 96); // Every allocated slot must reach the view, not
                               // just the counter.
          unsigned recycled = 0;
          for (int i = 0; i < 96; ++i) {
            const auto &p = actor->unk_14C[i];
            Vec3f projected;
            ProjectLeaf(
                &play,
                {p.unk_00 + p.unk_0C, p.unk_04 + p.unk_10, p.unk_08 + p.unk_14},
                &projected);
            assert(projected.z > 0 && projected.x >= left &&
                   projected.x < right && projected.y >= 0 &&
                   projected.y < 240);
            if (p.epoch != epochs[i]) {
              ++recycled;
              // Recycling must not teleport an opaque leaf inside the viewport.
              assert(previous[i].z <= 0 || previous[i].x < left ||
                     previous[i].x >= right || previous[i].y < 0 ||
                     previous[i].y >= 240);
            }
          }
          if (frame != 0)
            assert(recycled < 16); // No periodic whole-field restart.
          totalRecycled += recycled;
        }
      }
    }
    assert(totalRecycled > 0);
    std::printf("PASS all 96 autumn slots in scene %d across aspect, FOV, "
                "pitch and individual recycling\n",
                scene);
  }
  aspectRatio = 4.f / 3.f;
  perspectiveProjection = false;
}

static void NativeProjectionHandednessRegression() {
  static PlayState play;
  Camera camera{};
  GraphicsContext gfx{};
  Reset(&play, &camera, &gfx);
  camera.fov = play.view.fovy = 60;
  play.view.up = {0, 1, 0};
  float nativeMatrix[4][4];
  guLookAtF(nativeMatrix, 0, 0, 0, 0, 0, 1, 0, 1, 0);
  Vec3f world{100, 40, 1000}, projected;
  ProjectLeaf(&play, world, &projected);
  const float nativeX = world.x * nativeMatrix[0][0] +
                        world.y * nativeMatrix[1][0] +
                        world.z * nativeMatrix[2][0];
  const float nativeDepth =
      -(world.x * nativeMatrix[0][2] + world.y * nativeMatrix[1][2] +
        world.z * nativeMatrix[2][2]);
  const float expected =
      160.f +
      nativeX * (120.f / std::tan(60.f * 3.14159265f / 360.f)) / nativeDepth;
  assert(expected < 160.f && std::fabs(projected.x - expected) < .001f);
  std::puts("PASS preview/test projection screen-right matches actual native "
            "guLookAtF handedness");
}

static void SteepDownwardAutumnRegression() {
  static PlayState play;
  Camera camera{};
  GraphicsContext gfx{};
  perspectiveProjection = true;
  aspectRatio = 16.f / 9.f;
  for (int scene : {SCENE_00KEIKOKU, SCENE_TOWN}) {
    Reset(&play, &camera, &gfx);
    play.sceneId = scene;
    camera.fov = play.view.fovy = 60;
    play.view.at = {
        0, -20,
        1}; // Nearly straight down: falling leaves stay projected onscreen.
    Confirm(&play, SEASON_AUTUMN);
    Frame(&play);
    auto *actor =
        (ObjectKankyo *)play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first;
    assert(actor);
    unsigned totalRecycled = 0;
    for (int frame = 0; frame < 3600;
         ++frame) { // Three minutes at the native 20 Hz update cadence.
      Frame(&play);
      Vec3f before[96], projected[96];
      s16 epochs[96];
      for (int i = 0; i < 96; ++i) {
        const auto &p = actor->unk_14C[i];
        before[i] = {p.unk_00 + p.unk_0C, p.unk_04 + p.unk_10,
                     p.unk_08 + p.unk_14};
        ProjectLeaf(&play, before[i], &projected[i]);
        epochs[i] = p.epoch;
      }
      assert(DrawSnow(&play) == 96);
      if (nonzeroLeafDraws != 96) {
        std::printf("FAIL steep-down scene %d frame %d: %u of 96 nonzero-alpha "
                    "leaves\n",
                    scene, frame, nonzeroLeafDraws);
        std::fflush(stdout);
      }
      assert(
          nonzeroLeafDraws ==
          96); // Submission counts alone hide permanently distance-faded slots.
      unsigned recycled = 0;
      for (int i = 0; i < 96; ++i) {
        const auto &p = actor->unk_14C[i];
        if (p.epoch != epochs[i]) {
          ++recycled;
          const float left = -(240.f * aspectRatio - 320.f) / 2.f;
          const float right = 320.f - left;
          const bool outside = projected[i].z <= 0 || projected[i].x < left ||
                               projected[i].x >= right || projected[i].y < 0 ||
                               projected[i].y >= 240;
          // A visible opaque leaf must never teleport. At >=8000 units,
          // native distance opacity is already zero before reseeding.
          assert(outside ||
                 Math_Vec3f_DistXYZ(&before[i], &play.view.eye) >= 8000.f);
          assert(p.unk_18 == 1 && submittedLeafAlpha[i] <= 9);
        }
      }
      assert(recycled < 16); // No synchronized whole-field restart.
      totalRecycled += recycled;
    }
    assert(totalRecycled >= 96);
    std::printf("PASS three-minute steep-down scene %d: 96 nonzero-alpha "
                "slots, %u bounded individual recycles\n",
                scene, totalRecycled);
  }
  aspectRatio = 4.f / 3.f;
  perspectiveProjection = false;
}

static int ActiveSeasonParticleOwners(const PlayState *play) {
  int active = 0;
  for (const Actor *actor =
           play->actorCtx.actorLists[ACTORCAT_ITEMACTION].first;
       actor; actor = actor->next) {
    active += actor->id == ACTOR_OBJECT_KANKYO && actor->params >= 1 &&
              actor->params <= 3 && actor->update;
  }
  return active;
}

struct XluMaterialState {
  Gfx texture{};
  Gfx combine{};
  void Apply(const Gfx &command) {
    const unsigned opcode = command.words.w0 >> 24;
    if (opcode == G_TEXTURE)
      texture = command;
    if (opcode == G_SETCOMBINE)
      combine = command;
    if (opcode == G_DL && command.words.w1 == (uintptr_t)sNativeParticleSetup) {
      for (const Gfx &setup : sNativeParticleSetup)
        Apply(setup);
    }
  }
  bool TextureOn() const { return ((texture.words.w0 >> 1) & 127) == G_ON; }
  bool SameMaterial(const XluMaterialState &other) const {
    return texture.words.w0 == other.texture.words.w0 &&
           texture.words.w1 == other.texture.words.w1 &&
           combine.words.w0 == other.combine.words.w0 &&
           combine.words.w1 == other.combine.words.w1;
  }
};

static void SpringXluMaterialRegression() {
  static PlayState play;
  Camera camera{};
  GraphicsContext gfx{};
  Reset(&play, &camera, &gfx);
  Confirm(&play, SEASON_SPRING);
  XluMaterialState standard;
  for (const Gfx &command : sNativeParticleSetup)
    standard.Apply(command);
  assert(standard.TextureOn());
  for (int mode : {0, 1, 2}) {
    // Full layer, all projected particles culled, and disabled/zero layer.
    forceParticleCulled = mode == 1;
    springBlossoms = mode == 2 ? 0 : 1;
    Gfx commands[4096];
    Gfx *cursor = Gfx_SetupDL(commands, SETUPDL_0);
    gfx.polyXlu.p = cursor;
    func_808DD3C8(play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first, &play);
    cursor = gfx.polyXlu.p;
    // This textured XLU consumer loads its own art but uses the surrounding
    // standard sampling/combiner state, as native shared render passes do.
    gDPLoadTextureBlock(cursor++, sAutumnLeafTextures[0], G_IM_FMT_RGBA,
                        G_IM_SIZ_32b, 32, 32, 0, G_TX_CLAMP, G_TX_CLAMP, 5, 5,
                        G_TX_NOLOD, G_TX_NOLOD);
    __gSPDisplayList(cursor++, sAutumnLeafGeometry);
    XluMaterialState state;
    unsigned petalMeshes = 0, layerSetups = 0, followingMeshes = 0;
    for (const Gfx *command = commands; command < cursor; ++command) {
      state.Apply(*command);
      if ((command->words.w0 >> 24) != G_DL)
        continue;
      if (command >= gfx.polyXlu.p) {
        assert(state.TextureOn() && state.SameMaterial(standard));
        ++followingMeshes;
      } else if (command->words.w1 == (uintptr_t)sNativeParticleSetup &&
                 command != commands) {
        ++layerSetups;
      } else if (command->words.w1 == (uintptr_t)sSpringPetalGeometry ||
                 command->words.w1 == (uintptr_t)sSpringBlossomGeometry) {
        assert(!state.TextureOn() && !state.SameMaterial(standard));
        ++petalMeshes;
      }
    }
    assert(petalMeshes == (mode == 0 ? 96 : 0) && followingMeshes == 1);
    assert(layerSetups ==
           (mode == 0 ? 2 : 0)); // One entry and one cleanup, none per petal.
  }
  springBlossoms = 1;
  forceParticleCulled = false;
  std::puts("PASS Spring restores textured XLU sampling/combiner once; "
            "culled/zero layers preserve state");
}

static void SpringVisibleParticleRegression() {
  static PlayState play;
  Camera camera{};
  GraphicsContext gfx{};
  for (int scene : {SCENE_00KEIKOKU, SCENE_TOWN, SCENE_ICHIBA, SCENE_BACKTOWN,
                    SCENE_CLOCKTOWER, SCENE_ALLEY}) {
    Reset(&play, &camera, &gfx);
    play.sceneId = scene;
    play.view.fovy = 60;
    play.view.up = {0, 1, 0};
    play.view.at = {0, 0, 1};
    perspectiveProjection = true;
    const int beforeRandom = gameplayRandomCalls;
    Confirm(&play, SEASON_SPRING);
    for (int i = 0; i < 180; ++i) {
      Frame(&play);
      assert(DrawSnow(&play) == 96 && petalDraws == 96 && blossomDraws == 24 && leafDraws == 0);
      assert(ActiveSeasonParticleOwners(&play) == 1);
      assert(gameplayRandomCalls == beforeRandom);
    }
    std::printf("PASS 96 moving Spring petal meshes in outdoor scene %d, "
                "isolated RNG and sole owner\n",
                scene);
  }
  perspectiveProjection = false;
}

static void SeasonalParticleToggleRegression() {
  static PlayState play;
  Camera camera{};
  GraphicsContext gfx{};
  Reset(&play, &camera, &gfx);
  const int beforeRandom = gameplayRandomCalls;
  Confirm(&play, SEASON_SPRING);
  assert(
      DrawSnow(&play) == 96 && petalDraws == 96 &&
      leafDraws ==
          0); // Spring owns the accepted leaf budget, with separate petal art.
  auto *petals =
      (ObjectKankyo *)play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first;
  const int owners = spawns;
  const f32 beforeY = petals->unk_14C[0].unk_10;
  DrawSnow(&play); // Paused redraw never advances falling motion.
  assert(petals->unk_14C[0].unk_10 == beforeY);
  Frame(&play);
  assert(petals->unk_14C[0].unk_10 < beforeY &&
         gameplayRandomCalls == beforeRandom);
  springBlossoms = 0;
  assert(DrawSnow(&play) ==
         0); // Cvar changes hide the layer on the very next render.
  EnvironmentFrame(&play);
  assert(!petals->actor.update && !petals->actor.draw);
  springBlossoms = 1;
  EnvironmentFrame(&play);
  assert(DrawSnow(&play) == 96 && spawns == owners + 1 &&
         ActiveSeasonParticleOwners(&play) == 1);
  const int currentOwners = spawns;
  Confirm(&play, SEASON_AUTUMN);
  assert(DrawSnow(&play) == 96 && leafDraws == 96 && spawns == currentOwners);
  autumnLeaves = 0;
  assert(DrawSnow(&play) == 0);
  EnvironmentFrame(&play);
  autumnLeaves = 1;
  EnvironmentFrame(&play);
  assert(DrawSnow(&play) == 96);
  Confirm(&play, SEASON_WINTER);
  Frame(&play);
  assert(DrawSnow(&play) == 64);
  const int winterOwners = spawns;
  const auto winterBefore =
      *(ObjectKankyo *)play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first;
  auto ordinarySnow = winterBefore, gustSnow = winterBefore;
  const int randomBeforeOrdinary = gameplayRandomCalls;
  ObjectKankyo_UpdateSeasonSnowParticles(&ordinarySnow, &play);
  const int ordinaryCalls = gameplayRandomCalls - randomBeforeOrdinary;
  winterFlurries = 1;
  const int randomBeforeGust = gameplayRandomCalls;
  ObjectKankyo_UpdateSeasonSnowParticles(&gustSnow, &play);
  assert(gameplayRandomCalls - randomBeforeGust == ordinaryCalls);
  assert(gustSnow.unk_14C[0].unk_0C != ordinarySnow.unk_14C[0].unk_0C);
  assert(gustSnow.unk_14C[0].unk_10 == ordinarySnow.unk_14C[0].unk_10);
  assert(gustSnow.unk_14C[0].unk_18 == ordinarySnow.unk_14C[0].unk_18);
  assert(play.envCtx.precipitation[PRECIP_SNOW_CUR] == 0);
  Frame(&play);
  assert(DrawSnow(&play) == 64 && spawns == winterOwners &&
         ActiveSeasonParticleOwners(&play) == 1);
  winterSnow = 0;
  assert(DrawSnow(&play) == 0);
  EnvironmentFrame(&play);
  winterSnow = 1;
  winterFlurries = 0;
  EnvironmentFrame(&play);
  assert(DrawSnow(&play) == 64);
  auto *previousRoomOwner = play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first;
  ++play.roomCtx.curRoom.num;
  EnvironmentFrame(&play);
  assert(!previousRoomOwner->update && !previousRoomOwner->draw &&
         ActiveSeasonParticleOwners(&play) == 1);
  Confirm(&play, SEASON_OFF);
  assert(DrawSnow(&play) == 0 && ActiveSeasonParticleOwners(&play) == 0);
  std::puts("PASS Spring petals, same bounded owner, live seasonal toggles and "
            "paused motion");

  Reset(&play, &camera, &gfx);
  ObjectKankyo native{};
  native.actor.id = ACTOR_OBJECT_KANKYO;
  native.actor.params = 1;
  native.actor.update = ObjectKankyo_Update;
  native.actor.draw = func_808DD3C8;
  ObjectKankyo_Init(&native.actor, &play);
  play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first = &native.actor;
  play.envCtx.precipitation[PRECIP_SNOW_CUR] =
      play.envCtx.precipitation[PRECIP_SNOW_MAX] = 17;
  const int beforeNativeSpawn = spawns;
  Confirm(&play, SEASON_WINTER);
  winterSnow = 0;
  Frame(&play);
  assert(DrawSnow(&play) == 0 && native.actor.update && native.actor.draw);
  assert(play.envCtx.precipitation[PRECIP_SNOW_CUR] == 17 &&
         spawns == beforeNativeSpawn);
  Confirm(&play, SEASON_OFF);
  assert(DrawSnow(&play) ==
         17); // Disabled seasonal snow cannot destroy live native snow.
  winterSnow = 1;
  std::puts("PASS disabled Winter snow preserves native actor, precipitation "
            "and Off restoration");
}

static void OpeningForestSeasonRegression() {
  static PlayState play;
  Camera camera{};
  GraphicsContext gfx{};
  for (bool solo : {false, true}) {
  gComboGameMode = solo ? COMBO_GAME_MM : COMBO_GAME_BOTH;
  for (int room : {0, 1}) {
    Reset(&play, &camera, &gfx);
    play.sceneId = solo ? COMBO_SOLO_FOREST_MM_SCENE : SCENE_LOST_WOODS;
    play.skyboxId = SKYBOX_2;
    play.roomCtx.curRoom.num = room;
    gSaveContext.fileNum = 0;
    gSaveContext.save.entrance = solo ? COMBO_SOLO_FOREST_MM_ENTRANCE : ENTRANCE(LOST_WOODS, 0);
    gSaveContext.save.day = 3;
    gSaveContext.save.time = CLOCK_TIME(21, 0);
    // The playable forest keeps its native room lighting and covered sky.
    play.envCtx.stormState = STORM_STATE_OFF;
    play.envCtx.lightMode = LIGHT_MODE_SETTINGS;
    const auto progress = gSaveContext.save.saveInfo;
    for (u8 season : {SEASON_SPRING, SEASON_WINTER, SEASON_AUTUMN,
                      SEASON_SUMMER, SEASON_OFF, SEASON_WINTER}) {
      Confirm(&play, season);
      Frame(&play);
      assert(MMWeather_Season() == (season == SEASON_OFF ? -1 : season));
      assert(MMWeather_FoliageSeasonForPlay(&play) == MMWeather_Season());
      assert(DrawSnow(&play) == (season == SEASON_WINTER ? 64 :
             (season == SEASON_SPRING || season == SEASON_AUTUMN) ? 96 : 0));
      if (season == SEASON_SPRING) {
        assert(petalDraws == 96 && blossomDraws == 24 && MMWeather_RainDensity() == 30 && rainGain == 1);
        const int before = rainDraws;
        DrawRainFromPlay(&play);
        assert(rainDraws == before + 1);
      } else if (season == SEASON_AUTUMN) {
        assert(leafDraws == 96);
      } else {
        assert(MMWeather_RainDensity() == 0 && rainGain == 0);
      }
      if (season == SEASON_WINTER)
        assert(MMWeather_Overcast() > 0);
      if (season == SEASON_SUMMER || season == SEASON_OFF)
        assert(MMWeather_Overcast() == 0 && ActiveSeasonParticleOwners(&play) == 0);
      assert(play.skyboxId == SKYBOX_2 && play.envCtx.lightMode == LIGHT_MODE_SETTINGS);
      assert(play.envCtx.precipitation[PRECIP_SNOW_CUR] == 0 &&
             play.envCtx.precipitation[PRECIP_SNOW_MAX] == 0 &&
             play.envCtx.precipitation[PRECIP_RAIN_CUR] == 0 &&
             play.envCtx.precipitation[PRECIP_RAIN_MAX] == 0);
      assert(std::memcmp(&progress, &gSaveContext.save.saveInfo, sizeof(progress)) == 0);
      assert(gSaveContext.save.day == 3 && gSaveContext.save.time == CLOCK_TIME(21, 0));
    }
    // The independent weather override also works with no Rod/season owned.
    Confirm(&play, SEASON_OFF);
    gSaveContext.save.shipSaveInfo.nei = {};
    gSaveContext.save.shipSaveInfo.nei.season = SEASON_OFF;
    const auto unowned = gSaveContext.save;
    independentRain = 1;
    for (int i = 0; i < 120; ++i)
      Frame(&play);
    assert(!Seasons_HasRod() && MMWeather_Season() == -1 &&
           MMWeather_RainDensity() > 0 && rainGain > 0 && MMWeather_Overcast() > 0);
    const int before = rainDraws;
    DrawRainFromPlay(&play);
    assert(rainDraws == before + 1 &&
           std::memcmp(&unowned, &gSaveContext.save, sizeof(unowned)) == 0);
    // Leaving for a real interior releases independent rain and audio too.
    play.sceneId = SCENE_SONCHONOIE;
    EnvironmentFrame(&play);
    assert(MMWeather_Season() == -1 && MMWeather_RainDensity() == 0 && rainGain == 0 && DrawSnow(&play) == 0);
    independentRain = 0;
  }
  }
  gComboGameMode = COMBO_GAME_BOTH;
  std::puts("PASS combined and solo Lost Woods SKYBOX_2: real Rod confirmation, rain draw/audio, "
            "96 petals/leaves, 64 snow, independent weather without a Rod, "
            "Off/re-entry and unchanged native save/sky/precipitation");
}

static void SoloForestWeatherGuards() {
  static PlayState play;
  Camera camera{};
  GraphicsContext gfx{};
  Reset(&play, &camera, &gfx);
  gComboGameMode = COMBO_GAME_MM;
  play.sceneId = COMBO_SOLO_FOREST_MM_SCENE;
  play.skyboxId = SKYBOX_2;
  play.roomCtx.curRoom.num = 0;
  gSaveContext.fileNum = 0;
  gSaveContext.sceneLayer = 0;
  gSaveContext.save.cutsceneIndex = 0;
  Confirm(&play, SEASON_WINTER);
  const auto eligible = [&] {
    return MMWeather_SeasonForPlay(&play) == SEASON_WINTER &&
           MMWeather_FoliageSeasonForPlay(&play) == SEASON_WINTER;
  };
  assert(eligible());
  for (int mode : {COMBO_GAME_BOTH, COMBO_GAME_OOT}) {
    gComboGameMode = mode;
    assert(!eligible());
  }
  gComboGameMode = COMBO_GAME_MM;
  play.roomCtx.curRoom.num = 2;
  assert(!eligible());
  play.roomCtx.curRoom.num = 1;
  assert(eligible());
  gSaveContext.sceneLayer = 1;
  assert(!eligible());
  gSaveContext.sceneLayer = 0;
  gSaveContext.save.cutsceneIndex = 0xFFF0;
  assert(!eligible());
  gSaveContext.save.cutsceneIndex = 0;
  gSaveContext.fileNum = 0xFF;
  assert(!eligible());
  gSaveContext.fileNum = 2;
  play.csCtx.state = 1;
  assert(!eligible());
  play.csCtx.state = CS_STATE_IDLE;
  assert(eligible());
  gComboGameMode = COMBO_GAME_BOTH;
  MMWeather_Reset();
  std::puts("PASS private solo forest weather guards: mode, room, layer, file, cutscene and native eligibility");
}

int main(int argc, char **argv) {
  if (argc == 2) {
    ExportAutumnPreview(argv[1]);
    return 0;
  }
  OpeningForestSeasonRegression();
  SoloForestWeatherGuards();
  SpringXluMaterialRegression();
  SpringVisibleParticleRegression();
  SeasonalParticleToggleRegression();
  SteepDownwardAutumnRegression();
  NativeProjectionHandednessRegression();
  AllAutumnSlotsVisibleRegression();
  VisibleAutumnRegression();
  AutumnCoverageRegression();
  static PlayState play;
  Camera camera{};
  GraphicsContext gfx{};
  Reset(&play, &camera, &gfx);
  Confirm(&play, SEASON_WINTER);
  assert(play.envCtx.precipitation[PRECIP_SNOW_CUR] == 0 &&
         play.envCtx.precipitation[PRECIP_SNOW_MAX] == 0);
  const int firstSpawn = spawns;
  Frame(&play);
  assert(DrawSnow(&play) == 64 && spawns == firstSpawn);
  Actor *winterActor = play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first;
  Confirm(&play, SEASON_SPRING);
  assert(play.envCtx.precipitation[PRECIP_SNOW_CUR] == 0 &&
         play.envCtx.precipitation[PRECIP_SNOW_MAX] == 0);
  assert(winterActor->update && winterActor->draw && DrawSnow(&play) == 96);
  assert(MMWeather_RainDensity() == 30 && rainGain == 1.0f);
  Frame(&play);
  assert(DrawSnow(&play) == 96);
  Confirm(&play, SEASON_WINTER);
  assert(spawns == firstSpawn &&
         play.envCtx.precipitation[PRECIP_SNOW_CUR] == 0);
  Frame(&play);
  assert(DrawSnow(&play) == 64);
  Confirm(&play, SEASON_AUTUMN);
  assert(DrawSnow(&play) == 96 &&
         play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first->update !=
             nullptr);
  assert(leafPalettes == 15 && leafDraws == 96);
  auto *leaves =
      (ObjectKankyo *)play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first;
  const f32 beforeLeafY = leaves->unk_14C[0].unk_10;
  const int beforeLeafRandom = gameplayRandomCalls;
  Frame(&play);
  assert(leaves->unk_14C[0].unk_10 < beforeLeafY &&
         gameplayRandomCalls == beforeLeafRandom);
  Confirm(&play, SEASON_WINTER);
  assert(
      spawns ==
      firstSpawn); // Spring/Autumn/Winter reuse the same native particle owner.
  const int beforeRestoreRandom = gameplayRandomCalls;
  assert(DrawSnow(&play) == 64);
  assert(leaves->unk_14C[0].unk_1C == 1 && firstLeafAlpha > 0);
  assert(gameplayRandomCalls == beforeRestoreRandom);
  Frame(&play);
  assert(DrawSnow(&play) == 64);
  assert(leafDraws == 0 && leafPalettes == 0);
  Confirm(&play, SEASON_OFF);
  assert(play.envCtx.precipitation[PRECIP_SNOW_CUR] == 0 &&
         play.envCtx.precipitation[PRECIP_SNOW_MAX] == 0);
  assert(DrawSnow(&play) == 0);
  std::puts("PASS real paused selector confirm: Winter/Spring/Autumn reuse one "
            "owner, Off removes it");

  Reset(&play, &camera, &gfx);
  play.envCtx.stormState = STORM_STATE_OFF;
  const int beforeAutumnSpawnRandom = gameplayRandomCalls;
  Confirm(&play, SEASON_AUTUMN);
  Frame(&play);
  assert(play.envCtx.stormState == STORM_STATE_OFF && DrawSnow(&play) == 96);
  assert(leafPalettes == 15 && leafDraws == 96);
  assert(gameplayRandomCalls == beforeAutumnSpawnRandom);

  // Scene-native snow actors must also initialize their autumn leaf phases
  // without advancing the random sequence used by gameplay.
  ObjectKankyo autumnNormal{}, autumnBlizzard{};
  autumnNormal.actor.params = 3;
  autumnBlizzard.actor.params = 2;
  play.envCtx.precipitation[PRECIP_SNOW_CUR] = 128;
  ObjectKankyo_Init(&autumnNormal.actor, &play);
  ObjectKankyo_Init(&autumnBlizzard.actor, &play);
  assert(gameplayRandomCalls == beforeAutumnSpawnRandom);
  std::puts("PASS clear-weather Autumn submits all four generated RGBA "
            "palettes, native fall and gameplay RNG isolation");

  // Snowhead's actual blizzard actors and environment remain the native
  // owners. Winter reuses them, clear seasons suppress all variants, Off
  // restores them.
  Reset(&play, &camera, &gfx);
  ObjectKankyo blizzard{}, normal{};
  blizzard.actor.id = normal.actor.id = ACTOR_OBJECT_KANKYO;
  blizzard.actor.params = 2;
  normal.actor.params = 3;
  blizzard.actor.update = normal.actor.update = ObjectKankyo_Update;
  blizzard.actor.draw = normal.actor.draw = func_808DD3C8;
  ObjectKankyo_Init(&blizzard.actor, &play);
  ObjectKankyo_Init(&normal.actor, &play);
  blizzard.actor.next = &normal.actor;
  play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first = &blizzard.actor;
  play.envCtx.precipitation[PRECIP_SNOW_MAX] = 128;
  play.envCtx.sandstormState = SANDSTORM_A;
  play.envCtx.sandstormPrimA = play.envCtx.sandstormEnvA = D_801F4E30 = 155;
  int count = spawns;
  Confirm(&play, SEASON_WINTER);
  Frame(&play);
  assert(spawns == count && play.envCtx.precipitation[PRECIP_SNOW_CUR] == 128);
  assert(DrawSnow(&play) > 0);
  for (u8 season : {SEASON_SPRING, SEASON_SUMMER}) {
    Confirm(&play, season);
    Frame(&play);
    assert(play.envCtx.precipitation[PRECIP_SNOW_CUR] == 128 &&
           DrawSnow(&play) == (season == SEASON_SPRING ? 96 : 0));
    assert(!NativeFogVisible(&play, SANDSTORM_A));
    assert(play.envCtx.sandstormState == SANDSTORM_A && D_801F4E30 == 155);
    assert(blizzard.actor.update && normal.actor.update && spawns == count);
  }
  Confirm(&play, SEASON_AUTUMN);
  Frame(&play);
  assert(play.envCtx.precipitation[PRECIP_SNOW_CUR] == 128 &&
         play.envCtx.precipitation[PRECIP_SNOW_MAX] == 128);
  assert(DrawSnow(&play) == 96 && NativeFogVisible(&play, SANDSTORM_A) &&
         D_801F4E30 == 155);
  const auto autumnSnowhead = play.envCtx;
  Confirm(&play, SEASON_WINTER);
  assert(DrawSnow(&play) == 128 &&
         play.envCtx.precipitation[PRECIP_SNOW_CUR] == 128);
  Confirm(&play, SEASON_AUTUMN);
  assert(DrawSnow(&play) == 96 && NativeFogVisible(&play, SANDSTORM_A));
  assert(std::memcmp(&autumnSnowhead, &play.envCtx, sizeof(autumnSnowhead)) ==
         0);
  Confirm(&play, SEASON_OFF);
  assert(play.envCtx.precipitation[PRECIP_SNOW_CUR] == 128 &&
         play.envCtx.precipitation[PRECIP_SNOW_MAX] == 128);
  assert(play.envCtx.sandstormState == SANDSTORM_A && D_801F4E30 == 155 &&
         DrawSnow(&play) > 0);
  assert(NativeFogVisible(&play, SANDSTORM_A));
  std::puts("PASS native Snowhead actor reuse, Spring petals/Summer masking, "
            "Autumn leaves/fog and live Off restoration");

  // Real Winter Fog tags write MAX only on exit. Native CUR/fog must never
  // become a saved seasonal count: continue its native decay and expose it on
  // Off.
  Reset(&play, &camera, &gfx);
  ObjectKankyo tagSnow{};
  tagSnow.actor.id = ACTOR_OBJECT_KANKYO;
  tagSnow.actor.params = 1;
  tagSnow.actor.update = ObjectKankyo_Update;
  tagSnow.actor.draw = func_808DD3C8;
  ObjectKankyo_Init(&tagSnow.actor, &play);
  play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first = &tagSnow.actor;
  play.envCtx.precipitation[PRECIP_SNOW_CUR] =
      play.envCtx.precipitation[PRECIP_SNOW_MAX] = 128;
  play.envCtx.sandstormState = SANDSTORM_A;
  play.envCtx.sandstormPrimA = play.envCtx.sandstormEnvA = D_801F4E30 = 155;
  Confirm(&play, SEASON_WINTER);
  Frame(&play);
  assert(DrawSnow(&play) == 64 &&
         play.envCtx.precipitation[PRECIP_SNOW_CUR] == 128);
  Player nativePlayer{};
  nativePlayer.actor.world.pos.x = 200;
  play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &nativePlayer.actor;
  EnWeatherTag tag{};
  tag.actor.params = (1 << 8) | WEATHERTAG_TYPE_WINTERFOG;
  play.envCtx.lightMode = LIGHT_MODE_TIME;
  play.envCtx.lightConfig = play.envCtx.changeLightNextConfig = 2;
  func_80966E84(&tag, &play);
  assert(play.envCtx.precipitation[PRECIP_SNOW_MAX] == 0 &&
         tag.actionFunc == func_80966E0C);
  assert(play.envCtx.precipitation[PRECIP_SNOW_CUR] == 128 &&
         D_801F4E30 == 155);
  play.state.frames = 16;
  ObjectKankyo_Update(&tagSnow.actor, &play);
  EnvironmentFrame(&play);
  assert(play.envCtx.precipitation[PRECIP_SNOW_CUR] == 126 &&
         DrawSnow(&play) == 64);
  const auto afterNativeTag = play.envCtx;
  Confirm(&play, SEASON_AUTUMN);
  assert(DrawSnow(&play) == 96 && NativeFogVisible(&play, SANDSTORM_A));
  assert(std::memcmp(&afterNativeTag, &play.envCtx, sizeof(afterNativeTag)) ==
         0);
  Confirm(&play, SEASON_OFF);
  assert(std::memcmp(&afterNativeTag, &play.envCtx, sizeof(afterNativeTag)) ==
         0);
  assert(play.envCtx.precipitation[PRECIP_SNOW_CUR] == 126 &&
         DrawSnow(&play) == 126 && D_801F4E30 == 155);
  for (int i = 0; i < 64; ++i) {
    play.state.frames += 16;
    ObjectKankyo_Update(&tagSnow.actor, &play);
  }
  assert(play.envCtx.precipitation[PRECIP_SNOW_CUR] == 0);
  std::puts("PASS real native Winter Fog tag MAX-only exit, independent "
            "CUR/fog decay and exact Off state");

  Reset(&play, &camera, &gfx);
  EnTest4 dayActor{};
  for (int day : {1, 2, 3}) {
    gSaveContext.save.day = day;
    play.envCtx.precipitation[PRECIP_RAIN_MAX] =
        play.envCtx.precipitation[PRECIP_RAIN_CUR] = 0;
    dayActor.weather = THREEDAY_WEATHER_CLEAR;
    Confirm(&play, SEASON_SPRING);
    Frame(&play, &dayActor);
    if (day == 2) {
      // Native rain ramps its CUR index every eight frames.
      for (int i = 0; i < 8; ++i)
        Frame(&play, &dayActor);
    }
    int draws = rainDraws;
    DrawRainFromPlay(&play);
    assert(rainDraws == draws + 1);
    if (day == 2) {
      assert(gWeatherMode == WEATHER_MODE_RAIN &&
             play.envCtx.precipitation[PRECIP_RAIN_MAX] == 60);
    }
    // Spring owns presentation on all three days while the native Day 2
    // schedule still advances its raw storm and precipitation targets.
    assert(MMWeather_RainDensity() == 30 && rainGain == 1 &&
           MMWeather_Overcast() > 0 && MMWeather_SeasonClearsRain());
    assert(nativeRainAmbience == 0 && nativeThunderAmbience == 0);
  }
  gSaveContext.save.day = 2;
  play.envCtx.precipitation[PRECIP_RAIN_MAX] =
      play.envCtx.precipitation[PRECIP_RAIN_CUR] = 60;
  play.envCtx.lightningState = LIGHTNING_ON;
  gWeatherMode = WEATHER_MODE_RAIN;
  Confirm(&play, SEASON_SUMMER);
  assert(nativeRainAmbience == 0 && nativeThunderAmbience == 0 &&
         MMWeather_SeasonClearsRain());
  int draws = rainDraws;
  DrawRainFromPlay(&play);
  assert(rainDraws == draws);
  u8 first = 1, second = 1, blend = 255;
  MMWeather_ApplySky(&first, &second, &blend);
  assert(first == 0 && second == 0 && blend == 0);
  AdjLightSettings light{};
  EnvLightSettings settings[24]{};
  func_800F6CEC(&play, 4, &light, settings);
  assert(light.ambientColor[0] == 0 && light.light1Color[0] == 0);
  Frame(&play, &dayActor);
  assert(gWeatherMode == WEATHER_MODE_RAIN &&
         play.envCtx.precipitation[PRECIP_RAIN_MAX] == 60);
  Confirm(&play, SEASON_AUTUMN);
  assert(nativeRainAmbience == 0 && nativeThunderAmbience == 0 &&
         MMWeather_RainDensity() == 0 && MMWeather_SeasonClearsRain());
  assert(gWeatherMode == WEATHER_MODE_RAIN &&
         play.envCtx.precipitation[PRECIP_RAIN_MAX] == 60);
  draws = rainDraws;
  DrawRainFromPlay(&play);
  assert(rainDraws == draws); // Autumn's own dry phase masks native rain.
  Confirm(&play, SEASON_OFF);
  assert(nativeRainAmbience == 1 && nativeThunderAmbience == 1 &&
         !MMWeather_SeasonClearsRain());
  DrawRainFromPlay(&play);
  assert(rainDraws == draws + 1);
  Confirm(&play, SEASON_SUMMER);
  gSaveContext.save.time = CLOCK_TIME(18, 0);
  Frame(&play, &dayActor); // real native schedule ends the storm
  assert(gWeatherMode == WEATHER_MODE_CLEAR &&
         dayActor.weather == THREEDAY_WEATHER_CLEAR);
  while (play.envCtx.precipitation[PRECIP_RAIN_MAX] > 8)
    Frame(&play, &dayActor);
  assert(play.envCtx.precipitation[PRECIP_RAIN_MAX] == 8 &&
         play.envCtx.precipitation[PRECIP_RAIN_CUR] > 0);
  assert(nativeRainAmbience == 0 && nativeThunderAmbience == 0);
  Confirm(&play, SEASON_OFF);
  assert(nativeRainAmbience == 0 &&
         nativeThunderAmbience ==
             0); // actual native stop, despite nonzero counters
  Confirm(&play, SEASON_AUTUMN);
  assert(nativeRainAmbience == 0 && nativeThunderAmbience == 0);
  func_800F6CEC(&play, 4, &light, settings);
  gWeatherMode = WEATHER_MODE_RAIN;
  func_800F6CEC(&play, 4, &light, settings);
  assert(light.ambientColor[0] == 0 && light.light1Color[0] == 0);
  Confirm(&play, SEASON_OFF);
  func_800F6CEC(&play, 4, &light, settings);
  assert(light.ambientColor[0] == -50 && light.light1Color[0] == -100);
  gSaveContext.save.day = 3;
  Confirm(&play, SEASON_SUMMER);
  assert(MMWeather_SeasonClearsRain());
  draws = rainDraws;
  DrawRainFromPlay(&play);
  assert(rainDraws ==
         draws); // Summer's clear policy applies on all three days.
  std::puts("PASS real native schedule: Spring all three days, Summer clear, "
            "Autumn dry presentation, preserved raw Day 2 storm and exact "
            "native rain/ambience/light restoration when Off");

  // A direct actor query must respect the current scene/story/camera state
  // even while the cached rendering bridge still contains last frame's season.
  Reset(&play, &camera, &gfx);
  Confirm(&play, SEASON_WINTER);
  for (int restriction = 0; restriction < 9; ++restriction) {
    switch (restriction) {
    case 0:
      play.csCtx.state = 1;
      break;
    case 1:
      play.envCtx.lightSettingOverride = 1;
      break;
    case 2:
      play.envCtx.customSkyboxFilter = true;
      break;
    case 3:
      camera.stateFlags = CAM_STATE_UNDERWATER;
      break;
    case 4:
      play.skyboxId = SKYBOX_NONE;
      break;
    case 5:
      // A real scene reload starts with fresh environment and actors.
      play.sceneId = SCENE_SONCHONOIE;
      play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first = nullptr;
      play.envCtx.precipitation[PRECIP_SNOW_CUR] =
          play.envCtx.precipitation[PRECIP_SNOW_MAX] = 0;
      break;
    case 6:
      play.envCtx.skyboxDisabled = true;
      break;
    case 7:
      gSaveContext.gameMode = GAMEMODE_END_CREDITS;
      break;
    case 8:
      play.gameOverCtx.state = GAMEOVER_DEATH_START;
      break;
    }
    assert(MMWeather_SeasonForPlay(&play) == -1);
    EnvironmentFrame(&play);
    assert(MMWeather_Season() == -1 && DrawSnow(&play) == 0);
    play.csCtx.state = CS_STATE_IDLE;
    play.envCtx.lightSettingOverride = LIGHT_SETTING_OVERRIDE_NONE;
    play.envCtx.customSkyboxFilter = play.envCtx.skyboxDisabled = false;
    camera.stateFlags = 0;
    play.skyboxId = SKYBOX_NORMAL_SKY;
    play.sceneId = SCENE_TOWN;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    play.gameOverCtx.state = GAMEOVER_INACTIVE;
    Frame(&play);
    assert(MMWeather_Season() == SEASON_WINTER && DrawSnow(&play) > 0);
  }
  Confirm(&play, SEASON_SPRING);
  Frame(&play);
  assert(DrawSnow(&play) == 96 && refreshes > 0);
  std::puts("PASS direct actor season query, story/interior/underwater "
            "restrictions and restoration");
}
