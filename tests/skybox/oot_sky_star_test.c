#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Only engine service/type boundaries are supplied here. The included bodies
// are extracted from z_vr_box.c and z_kankyo.c on every run, never copied.
typedef int32_t s32;
typedef int16_t s16;
typedef uint16_t u16;
typedef uint8_t u8;
typedef uint32_t u32;
typedef float f32;
typedef struct { int opcode; } Gfx;
typedef struct { Gfx* polyOpa; } GraphicsContext;
typedef struct {
    Gfx (*ootSkyDLists[2])[150];
    u8 ootSkyPhases[2];
    Gfx (*dListBuf)[150];
    void* staticSegments[2][6];
} SkyboxContext;
typedef struct { u8 skybox1Index; u8 skyboxDisabled; int skyboxConfig; } EnvironmentContext;
typedef struct {
    EnvironmentContext envCtx;
    SkyboxContext skyboxCtx;
    s16 skyboxId;
    struct { GraphicsContext* gfxCtx; } state;
} PlayState;

enum { SKYBOX_NONE, SKYBOX_NORMAL_SKY, SKYBOX_2, SKYBOX_3, SKYBOX_CONFIG_24 = 24 };
#define CLOCK_TIME(hour, minute) ((u16)(((hour) * 60 + (minute)) * 65536 / (24 * 60)))
#define CURRENT_TIME sCurrentTime
#define OPEN_DISPS(ctx) Gfx* polyOpa = (ctx)->polyOpa
#define POLY_OPA_DISP polyOpa
#define CLOSE_DISPS(ctx) ((ctx)->polyOpa = polyOpa)
#define gSPNoOp(command) ((command)->opcode = 1, ++sNoOps)

static u16 sCurrentTime;
static int sEnabled = 1, sAlt = 1, sFailures, sChecks, sNoOps, sSettingReads, sBuilds;
static float sOvercast;
static u16 gSkyboxNumStars = 128;
static s32 sEnvSkyboxNumStars;
static f32 D_801F4F28;
static Gfx* sSkyboxStarsDList;
static const char sOotSkyTextures[2][4][5][96] = { 0 };

static int CVarGetInteger(const char* key, int fallback) {
    ++sSettingReads;
    return strcmp(key, "gEnhancements.Graphics.UseOotSkyTextures") ? fallback : sEnabled;
}
static bool ResourceMgr_IsAltAssetsEnabled(void) { return sAlt != 0; }
static float MMWeather_Overcast(void) { return sOvercast; }
static void Skybox_Calculate128(SkyboxContext* context, int faces) {
    (void)context;
    (void)faces;
    ++sBuilds;
}

#include "production_sky.inc"

static void Check(bool ok, const char* message) {
    ++sChecks;
    if (!ok) {
        ++sFailures;
        fprintf(stderr, "FAIL: %s\n", message);
    }
}

static void Verify(PlayState* play, bool expectOot, bool expectNativeStars, const char* description) {
    Gfx commands[4] = { 0 };
    play->state.gfxCtx->polyOpa = commands;
    u8 blend = 255;
    int builds = sBuilds;
    bool usingOot = Skybox_PrepareOot(&play->skyboxCtx, play->skyboxId, sCurrentTime, &blend) != 0;
    Check(usingOot == expectOot, description);
    Check(sBuilds == builds, "stable sky does not rebuild display lists");
    if (usingOot) {
        Check(blend == 0, "native-star suppression preserves night texture blending");
    }
    // Seed stale data to ensure suppressed and fallback frames reset it.
    sSkyboxStarsDList = &commands[3];
    sEnvSkyboxNumStars = 17;
    D_801F4F28 = 0.123f;
    sNoOps = 0;
    Environment_SetupSkyboxStars(play);
    Check((sSkyboxStarsDList != NULL) == expectNativeStars, description);
    Check(sNoOps == (expectNativeStars ? 1 : 0), "only visible native stars reserve draw commands");
    Check(play->state.gfxCtx->polyOpa == commands + (expectNativeStars ? 1 : 0),
          "suppressed stars leave the display-list cursor unchanged");
    if (expectNativeStars) {
        Check(sEnvSkyboxNumStars == gSkyboxNumStars, "native fallback keeps the configured star count");
        Check(D_801F4F28 > 0.0f, "native fallback keeps the time/weather alpha");
    } else if ((expectOot && !play->envCtx.skyboxDisabled) || play->envCtx.skybox1Index != 0 ||
               play->skyboxId != SKYBOX_NORMAL_SKY) {
        Check(sEnvSkyboxNumStars == 0 && D_801F4F28 == 0.0f, "inactive native-star state is fully reset");
    }
}

int main(void) {
    GraphicsContext graphics = { 0 };
    Gfx clear[12][150] = { 0 }, cloudy[12][150] = { 0 };
    PlayState play = { .skyboxId = SKYBOX_NORMAL_SKY, .state.gfxCtx = &graphics };
    play.skyboxCtx.ootSkyDLists[0] = clear;
    play.skyboxCtx.ootSkyDLists[1] = cloudy;
    play.skyboxCtx.ootSkyPhases[0] = play.skyboxCtx.ootSkyPhases[1] = 3;
    sCurrentTime = CLOCK_TIME(23, 0);

    Verify(&play, true, false, "active OoT night sky suppresses duplicate native pink stars");
    play.envCtx.skyboxDisabled = 1;
    Verify(&play, true, true, "disabled skybox keeps original star behavior when no OoT sky is drawn");
    play.envCtx.skyboxDisabled = 0;
    sEnabled = 0;
    Verify(&play, false, true, "disabling OoT sky immediately restores native stars");
    sEnabled = 1;
    Verify(&play, true, false, "reenabling OoT sky suppresses native stars again");
    sAlt = 0;
    Verify(&play, false, true, "Alt Assets off preserves native stars");
    sAlt = 1;
    Verify(&play, true, false, "Alt Assets on restores OoT star ownership");

    play.skyboxCtx.ootSkyDLists[0] = NULL;
    Verify(&play, false, true, "missing clear sky lists retain native stars");
    play.skyboxCtx.ootSkyDLists[0] = clear;
    play.skyboxCtx.ootSkyDLists[1] = NULL;
    Verify(&play, false, true, "missing overcast sky lists retain native stars");
    play.skyboxCtx.ootSkyDLists[0] = NULL;
    Verify(&play, false, true, "absent/incomplete/uninitialized pack retains native stars");
    play.skyboxCtx.ootSkyDLists[0] = clear;
    play.skyboxCtx.ootSkyDLists[1] = cloudy;
    Verify(&play, true, false, "scene reentry with complete pack suppresses native stars");

    play.envCtx.skybox1Index = 1;
    sOvercast = 0.65f;
    Verify(&play, true, false, "winter cloudy sky still suppresses native stars");
    sEnabled = 0;
    Verify(&play, false, false, "native winter cloudy sky retains its existing star suppression");
    play.envCtx.skybox1Index = 0;
    sOvercast = 0.75f;
    Verify(&play, false, true, "native partial-overcast stars keep their alpha fade");
    Check(D_801F4F28 == 0.25f, "native partial-overcast star alpha remains exactly 25 percent");
    sOvercast = 1.0f;
    Verify(&play, false, false, "native full overcast fades stars completely");
    sOvercast = 0.0f;

    play.skyboxId = SKYBOX_2;
    sEnabled = 1;
    Verify(&play, false, false, "special skies retain native behavior");
    play.skyboxId = SKYBOX_3;
    Verify(&play, true, false, "regional OoT sky retains native star gating");
    play.skyboxId = SKYBOX_NORMAL_SKY;
    sEnabled = 0;
    sCurrentTime = CLOCK_TIME(12, 0);
    Verify(&play, false, false, "native daylight remains star free");
    sCurrentTime = CLOCK_TIME(20, 0);
    Verify(&play, false, true, "native dusk star fade remains visible");
    Check(D_801F4F28 > 0.0f && D_801F4F28 < 1.0f, "native dusk keeps fractional alpha");
    sCurrentTime = CLOCK_TIME(2, 30);
    Verify(&play, false, true, "native dawn star fade remains visible");
    Check(D_801F4F28 > 0.0f && D_801F4F28 < 1.0f, "native dawn keeps fractional alpha");
    printf("MM OoT sky native-star ownership: %d checks, %d failures\n", sChecks, sFailures);
    return sFailures != 0;
}
