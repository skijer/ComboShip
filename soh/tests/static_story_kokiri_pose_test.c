#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "tests/test_require.h"
#include "src/overlays/actors/ovl_En_Viewer/z_en_viewer.h"
#include "src/overlays/actors/ovl_En_Viewer/static_story_actor.h"
#include "src/overlays/actors/ovl_En_Viewer/static_story_kokiri.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/Enhancements/cosmetics/NativeNpcModels.h"
#include "objects/object_dy_obj/object_dy_obj.h"
#include "objects/object_kw1/object_kw1.h"
#include "objects/object_os_anime/object_os_anime.h"
#include "kokiri_pose_fixture.h"

static bool donorAvailable = true;
static unsigned donorLoads;
static bool modelPackAvailable;
static bool fadoPackAvailable;
static bool fadoSeatedAvailable;
static const SkelAnime* fittedPoseTarget;
static const char* expectedSkeleton;
static const char* expectedAnimation;
static const char fadoSkeleton[] = "__OTR__objects/object_fa/YoungFadoNpcPOC1/Skel";
static const char fadoHead[] = "__OTR__objects/object_fa/YoungFadoNpcPOC1/Body/Limb14";
static const char fadoSeated[] = "__OTR__objects/object_fa/YoungFadoNpcPOC1/Animations/Seated5";
uintptr_t gSegments[16];

uint8_t ResourceMgr_FileExists(const char* path) {
    return (modelPackAvailable && strcmp(path, sKokiriGirlNpcSkel) == 0) ||
           (fadoPackAvailable && strcmp(path, fadoSkeleton) == 0) ||
           (fadoSeatedAvailable && strcmp(path, fadoSeated) == 0);
}
uint8_t ResourceMgr_FileAltExists(const char* path) {
    return (modelPackAvailable && (strcmp(path, sKokiriGirlNpcSkel) == 0 || strcmp(path, sKokiriGirlNpcHead) == 0)) ||
           (fadoPackAvailable && (strcmp(path, fadoSkeleton) == 0 || strcmp(path, fadoHead) == 0)) ||
           (fadoSeatedAvailable && strcmp(path, fadoSeated) == 0);
}

/* Exercise the production init path while retaining caller-owned test tables. */
s32 __wrap_SkelAnime_InitFlex(PlayState* play, SkelAnime* skel, FlexSkeletonHeader* skeleton,
                              AnimationHeader* animation, Vec3s* joints, Vec3s* morph, s32 count) {
    REQUIRE(strcmp((const char*)skeleton, expectedSkeleton != NULL ? expectedSkeleton : gKw1Skel) == 0);
    REQUIRE(animation == NULL && joints == NULL && morph == NULL && count == 0);
    REQUIRE(skel->jointTable != NULL);
    skel->limbCount = 16;
    return 0;
}
void __wrap_Animation_PlayLoopSetSpeed(SkelAnime* skel, AnimationHeader* animation, f32 speed) {
    REQUIRE(strcmp((const char*)animation,
                   expectedAnimation != NULL ? expectedAnimation : gKokiriSittingCrossedLegsAnim) == 0);
    skel->animation = animation;
    memcpy(skel->jointTable, fixtureSeated, sizeof(fixtureSeated));
}

AnimationHeaderCommon* ResourceMgr_LoadAnimByName(const char* path) {
    REQUIRE(strcmp(path, gGreatFairySittingAnim) == 0);
    ++donorLoads;
    return donorAvailable ? &fixtureDonor.common : NULL;
}
AnimationHeader* NativeNpcPose_ResolveAnimation(const SkelAnime* skel, AnimationHeader* animation) {
    return skel == fittedPoseTarget ? &fixtureDonor : animation;
}
int ResourceMgr_OTRSigCheck(char* value) {
    return 0;
}
void osSyncPrintfUnused(const char* format, ...) {
}
void LogUtils_LogThreadId(const char* name, s32 line) {
}
void lusprintf(const char* file, int32_t line, int32_t level, const char* format, ...) {
}

static void Pose(EnViewer* self, Vec3s* joints, uint8_t type, uint8_t pose) {
    memset(self, 0, sizeof(*self));
    self->staticState.type = type;
    self->staticState.pose = pose;
    self->skin.skelAnime.jointTable = joints;
    self->skin.skelAnime.limbCount = 16;
    memcpy(joints, pose == 5 ? fixtureArmsCrossed : fixtureSeated, sizeof(fixtureSeated));
}

static void Update(EnViewer* self, float step) {
    StaticStoryKokiri_UpdatePose(self, step);
}

/* Independent full Z-Y-X matrix composition, not the correction formula. */
static void Rotate(const Vec3s* rot, double v[3]) {
    const double unit = 3.14159265358979323846 / 32768.0;
    double x = rot->x * unit, y = rot->y * unit, z = rot->z * unit;
    double sx = sin(x), cx = cos(x), sy = sin(y), cy = cos(y), sz = sin(z), cz = cos(z);
    double a = v[0], b = v[1], c = v[2];
    v[0] = cz * cy * a + (cz * sy * sx - sz * cx) * b + (cz * sy * cx + sz * sx) * c;
    v[1] = sz * cy * a + (sz * sy * sx + cz * cx) * b + (sz * sy * cx - cz * sx) * c;
    v[2] = -sy * a + cy * sx * b + cy * cx * c;
}

static void RequireLevelHead(Vec3s* joints) {
    double forward[3] = { 0, 1, 0 }, up[3] = { 1, 0, 0 };
    for (int n = 0; n < 3; ++n) {
        int joint = n == 0 ? 15 : n == 1 ? 8 : 1;
        Rotate(&joints[joint], forward);
        Rotate(&joints[joint], up);
    }
    REQUIRE(fabs(forward[1]) < 0.002);
    REQUIRE(forward[2] > 0.999);
    REQUIRE(up[1] > 0.999);
}

int main(void) {
    EnViewer girl, fado;
    Vec3s girlJoints[16], fadoJoints[16], expected[8], next[8];
    Pose(&girl, girlJoints, STATIC_STORY_ACTOR_KOKIRI_GIRL, 4);
    Pose(&fado, fadoJoints, STATIC_STORY_ACTOR_FADO, 4);
    PlayState* play = calloc(1, sizeof(*play));
    REQUIRE(play != NULL);
    girl.staticState.kokiriLegFrame = 23.5f;
    fado.staticState.kokiriLegFrame = 49.5f;
    StaticStoryKokiri_Init(&girl, play);
    StaticStoryKokiri_Init(&fado, play);
    REQUIRE(girl.staticState.kokiriLegFrame == 0 && fado.staticState.kokiriLegFrame == 0);
    SkelAnime_GetFrameData(&fixtureDonor, 0, 8, expected);
    REQUIRE(memcmp(girlJoints + 2, expected + 2, sizeof(Vec3s) * 6) == 0);
    REQUIRE(memcmp(girlJoints, fadoJoints, sizeof(girlJoints)) == 0);
    RequireLevelHead(girlJoints);
    RequireLevelHead(fadoJoints);

    /* The private girl rig retains exactly the same native sampler output.
     * The separate Fado control never selects this model, even with the pack. */
    modelPackAvailable = true;
    expectedSkeleton = sKokiriGirlNpcSkel;
    StaticStoryKokiri_Init(&girl, play);
    expectedSkeleton = gKw1Skel;
    StaticStoryKokiri_Init(&fado, play);
    REQUIRE(memcmp(girlJoints, fadoJoints, sizeof(girlJoints)) == 0);
    RequireLevelHead(girlJoints);
    modelPackAvailable = false;
    expectedSkeleton = NULL;

    /* Real static Fado initialization must register her private rig. Recognizing
     * her namespace in the resolver alone cannot make seated corrections load. */
    Pose(&fado, fadoJoints, STATIC_STORY_ACTOR_FADO, 5);
    fadoPackAvailable = true;
    expectedSkeleton = fadoSkeleton;
    expectedAnimation = gKokiriSittingCrossedArmsLegsAnim;
    StaticStoryKokiri_Init(&fado, play);
    REQUIRE(strcmp((const char*)fado.skin.skelAnime.animation, gKokiriSittingCrossedArmsLegsAnim) == 0);
    fadoSeatedAvailable = true;
    expectedAnimation = fadoSeated;
    StaticStoryKokiri_Init(&fado, play);
    REQUIRE(strcmp((const char*)fado.skin.skelAnime.animation, fadoSeated) == 0);
    fadoPackAvailable = false;
    expectedSkeleton = gKw1Skel;
    expectedAnimation = gKokiriSittingCrossedArmsLegsAnim;
    StaticStoryKokiri_Init(&fado, play);
    fadoSeatedAvailable = false;
    expectedSkeleton = expectedAnimation = NULL;
    Pose(&fado, fadoJoints, STATIC_STORY_ACTOR_FADO, 4);

    /* Two complete loops plus fractional updates and the seam. */
    float frame = 0;
    for (int tick = 0; tick < fixtureDonor.common.frameCount * 4 + 3; ++tick) {
        frame = fmodf(frame + 0.5f, fixtureDonor.common.frameCount);
        memcpy(girlJoints, fixtureSeated, sizeof(fixtureSeated));
        memcpy(fadoJoints, fixtureSeated, sizeof(fixtureSeated));
        Update(&girl, 0.5f);
        Update(&fado, 0.5f);
        int current = (int)frame;
        int following = (current + 1) % fixtureDonor.common.frameCount;
        SkelAnime_GetFrameData(&fixtureDonor, current, 8, expected);
        SkelAnime_GetFrameData(&fixtureDonor, following, 8, next);
        SkelAnime_InterpFrameTable(8, expected, expected, next, frame - current);
        REQUIRE(memcmp(girlJoints + 2, expected + 2, sizeof(Vec3s) * 6) == 0);
        REQUIRE(memcmp(girlJoints, fadoJoints, sizeof(girlJoints)) == 0);
        REQUIRE(memcmp(girlJoints, fixtureSeated, sizeof(Vec3s) * 2) == 0);
        REQUIRE(memcmp(girlJoints + 8, fixtureSeated + 8, sizeof(Vec3s) * 7) == 0);
        REQUIRE(girlJoints[15].x == fixtureSeated[15].x && girlJoints[15].y == fixtureSeated[15].y);
        RequireLevelHead(girlJoints);
    }
    /* A fresh actor starts independently rather than inheriting another cursor. */
    Pose(&fado, fadoJoints, STATIC_STORY_ACTOR_FADO, 4);
    Update(&fado, 0);
    SkelAnime_GetFrameData(&fixtureDonor, 0, 8, expected);
    REQUIRE(memcmp(fadoJoints + 2, expected + 2, sizeof(Vec3s) * 6) == 0);
    RequireLevelHead(fadoJoints);

    for (int type = STATIC_STORY_ACTOR_KOKIRI_GIRL; type <= STATIC_STORY_ACTOR_FADO; ++type) {
        for (int pose = 0; pose <= 5; ++pose) {
            if (pose == 4)
                continue;
            Pose(&girl, girlJoints, type, pose);
            unsigned loads = donorLoads;
            Update(&girl, 1);
            REQUIRE(memcmp(girlJoints, pose == 5 ? fixtureArmsCrossed : fixtureSeated, sizeof(girlJoints)) == 0);
            REQUIRE(donorLoads == loads);
        }
        REQUIRE(!StaticStoryActor_CanTrack(type, 4));
        REQUIRE(!StaticStoryActor_CanTrack(type, 5));
    }
    /* An archive-provided complete pose must retain its own head and legs. */
    Pose(&girl, girlJoints, STATIC_STORY_ACTOR_KOKIRI_GIRL, 4);
    fittedPoseTarget = &girl.skin.skelAnime;
    unsigned loadsBeforeFit = donorLoads;
    Update(&girl, 1);
    REQUIRE(memcmp(girlJoints, fixtureSeated, sizeof(girlJoints)) == 0);
    REQUIRE(girl.staticState.kokiriLegFrame == 0 && donorLoads == loadsBeforeFit);
    fittedPoseTarget = NULL;
    donorAvailable = false;
    Pose(&girl, girlJoints, STATIC_STORY_ACTOR_KOKIRI_GIRL, 4);
    Update(&girl, 1);
    RequireLevelHead(girlJoints);
    REQUIRE(memcmp(girlJoints + 2, fixtureSeated + 2, sizeof(Vec3s) * 6) == 0);

    /* Every cutscene entry selects the paired girl rig, including the three
     * entries absent from the old preview sheet. Missing packs stay native. */
    const char* allAnimations[] = { gKokiriIdleAnim,
                                    gKokiriStandingArmsBehindBackAnim,
                                    gKokiriStandingHandsOnHipsAnim,
                                    gKokiriSittingHeadOnHandAnim,
                                    gKokiriSittingCrossedLegsAnim,
                                    gKokiriSittingCrossedArmsLegsAnim };
    EnViewer entry;
    Vec3s entryJoints[16];
    for (int available = 0; available < 2; ++available) {
        modelPackAvailable = available;
        expectedSkeleton = available ? sKokiriGirlNpcSkel : gKw1Skel;
        for (int pose = 0; pose < 6; ++pose) {
            Pose(&entry, entryJoints, STATIC_STORY_ACTOR_KOKIRI_GIRL, pose);
            expectedAnimation = allAnimations[pose];
            StaticStoryKokiri_Init(&entry, play);
            REQUIRE(strcmp((const char*)entry.skin.skelAnime.animation, allAnimations[pose]) == 0);
        }
    }
    free(play);
    puts("PASS both characters: level heads from init, donor leg motion, two loops, fractional seam, independent "
         "restart, unchanged other poses, fitted-clip ownership and missing-donor fallback");
    return 0;
}
