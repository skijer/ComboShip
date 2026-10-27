/* Execute the real draw callbacks with a small affine-matrix fixture. */
#include <math.h>
#include <stdio.h>
#include <string.h>

#if MALON_TRACKING_ACTOR == 1
#include "../src/overlays/actors/ovl_En_Ma1/z_en_ma1.c"
#define TestActor EnMa1
#define TestHead EnMa1_OverrideLimbDraw
#define HEAD_LIMB 15
#define TEST_INFO(actor) ((actor).interactInfo)
#elif MALON_TRACKING_ACTOR == 2
#include "../src/overlays/actors/ovl_En_Ma2/z_en_ma2.c"
#define TestActor EnMa2
#define TestHead EnMa2_OverrideLimbDraw
#define HEAD_LIMB 18
#define TEST_INFO(actor) ((actor).interactInfo)
#elif MALON_TRACKING_ACTOR == 3
#include "../src/overlays/actors/ovl_En_Ma3/z_en_ma3.c"
#define TestActor EnMa3
#define TestHead EnMa3_OverrideLimbDraw
#define HEAD_LIMB 18
#define TEST_INFO(actor) ((actor).interactInfo)
#else
#include "../src/overlays/actors/ovl_En_Viewer/z_en_viewer.c"
#define TestActor EnViewer
#define TEST_INFO(actor) ((actor).staticState.interactInfo)
#endif

static unsigned sFlags;
static double sMatrix[4][4];

bool ResourceMgr_IsAltAssetsEnabled(void) {
    return (sFlags & 1) != 0;
}
uint8_t ResourceMgr_FileExists(const char* path) {
    return (sFlags & 2) != 0;
}
uint8_t ResourceMgr_FileAltExists(const char* path) {
    if (strstr(path, "/Skel"))
        return (sFlags & 4) != 0;
    if (strstr(path, "/Body/"))
        return (sFlags & 8) != 0;
    return 1;
}
Gfx* NativeNpcPose_SelectLimb(const SkelAnime* animation, s32 limb, Gfx* original) {
    return original;
}
f32 Math_SinS(s16 angle) {
    return sinf(angle * (M_PI / 32768.0f));
}
f32 Math_CosS(s16 angle) {
    return cosf(angle * (M_PI / 32768.0f));
}

static void Identity(void) {
    memset(sMatrix, 0, sizeof(sMatrix));
    for (int i = 0; i < 4; ++i)
        sMatrix[i][i] = 1;
}
static void Multiply(double right[4][4]) {
    double out[4][4] = { 0 };
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            for (int k = 0; k < 4; ++k)
                out[r][c] += sMatrix[r][k] * right[k][c];
    memcpy(sMatrix, out, sizeof(out));
}
void Matrix_Translate(f32 x, f32 y, f32 z, u8 mode) {
    double t[4][4] = { { 1, 0, 0, x }, { 0, 1, 0, y }, { 0, 0, 1, z }, { 0, 0, 0, 1 } };
    if (mode == MTXMODE_NEW)
        Identity();
    Multiply(t);
}
void Matrix_RotateX(f32 angle, u8 mode) {
    double c = cos(angle), s = sin(angle);
    double r[4][4] = { { 1, 0, 0, 0 }, { 0, c, -s, 0 }, { 0, s, c, 0 }, { 0, 0, 0, 1 } };
    if (mode == MTXMODE_NEW)
        Identity();
    Multiply(r);
}
void Matrix_RotateZ(f32 angle, u8 mode) {
    double c = cos(angle), s = sin(angle);
    double r[4][4] = { { c, -s, 0, 0 }, { s, c, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } };
    if (mode == MTXMODE_NEW)
        Identity();
    Multiply(r);
}
void Matrix_RotateY(f32 angle, u8 mode) {
    double c = cos(angle), s = sin(angle);
    double r[4][4] = { { c, 0, s, 0 }, { 0, 1, 0, 0 }, { -s, 0, c, 0 }, { 0, 0, 0, 1 } };
    if (mode == MTXMODE_NEW)
        Identity();
    Multiply(r);
}
static Vec3f Transform(Vec3f p) {
    Vec3f out;
    float* xyz = &out.x;
    for (int i = 0; i < 3; ++i)
        xyz[i] = sMatrix[i][0] * p.x + sMatrix[i][1] * p.y + sMatrix[i][2] * p.z + sMatrix[i][3];
    return out;
}

int main(void) {
    /* The first two are the actual accepted child/adult pack neck offsets.
     * A third covers a future fit with offsets on all three axes. */
    const Vec3f positions[] = { { 645, 0, 0 }, { 1340, 0, 0 }, { 645, 80, -45 } };
    const Vec3s angles[] = { { 0, 0, 0 },    { 4096, 0, 0 },  { -4096, 0, 0 },
                             { 0, 4096, 0 }, { 0, -4096, 0 }, { 4096, -4096, 0 } };
    unsigned checks = 0, failures = 0;
#if MALON_TRACKING_ACTOR == 0
    const int cases = 2;
#else
    const int cases = 1;
#endif
    for (int which = 0; which < cases; ++which) {
        TestActor actor = { 0 };
#if MALON_TRACKING_ACTOR == 0
        OverrideLimbDrawOpa callback =
            which ? EnViewer_StaticAdultMalonOverrideLimbDraw : EnViewer_StaticChildMalonOverrideLimbDraw;
        int head = which ? 18 : 15;
        actor.staticState.type = which ? STATIC_STORY_ACTOR_ADULT_MALON : STATIC_STORY_ACTOR_CHILD_MALON;
#else
        OverrideLimbDrawOpa callback = TestHead;
        int head = HEAD_LIMB;
#endif
        for (sFlags = 0; sFlags < 16; ++sFlags) {
            for (unsigned p = 0; p < ARRAY_COUNT(positions); ++p) {
                for (unsigned a = 0; a < ARRAY_COUNT(angles); ++a) {
                    Vec3f pos = positions[p], expected;
                    Vec3s rot = { 0 };
                    Gfx dummy[1], *display = dummy;
                    TEST_INFO(actor).headRot = angles[a];
                    /* Alt off and incomplete packs must keep the vanilla pivot. */
                    Identity();
                    if (sFlags != 15) {
                        Matrix_Translate(1400, 0, 0, MTXMODE_APPLY);
                        Matrix_RotateX(BINANG_TO_RAD(angles[a].y), MTXMODE_APPLY);
                        Matrix_RotateZ(BINANG_TO_RAD(angles[a].x), MTXMODE_APPLY);
                        Matrix_Translate(-1400, 0, 0, MTXMODE_APPLY);
                    }
                    expected = Transform(pos);
                    Identity();
                    callback(NULL, head, &display, &pos, &rot, &actor);
                    Vec3f actual = Transform(pos);
                    float error =
                        sqrtf(SQ(actual.x - expected.x) + SQ(actual.y - expected.y) + SQ(actual.z - expected.z));
                    if (error > 0.001f) {
                        if (!failures)
                            fprintf(stderr,
                                    "FAIL actor=%d case=%d flags=%u neck=(%.0f,%.0f,%.0f) tracking=(%d,%d): anchor "
                                    "error %.3f\n",
                                    MALON_TRACKING_ACTOR, which, sFlags, pos.x, pos.y, pos.z, angles[a].x, angles[a].y,
                                    error);
                        ++failures;
                    }
                    /* Keeping the anchor fixed must not disable or reverse the
                     * actual look rotation. Check an off-origin head point. */
                    const Vec3f direction = { 170, 80, -110 };
                    double rx = BINANG_TO_RAD(angles[a].y), rz = BINANG_TO_RAD(angles[a].x);
                    double zx = cos(rz) * direction.x - sin(rz) * direction.y;
                    double zy = sin(rz) * direction.x + cos(rz) * direction.y;
                    Vec3f turned = { zx, cos(rx) * zy - sin(rx) * direction.z, sin(rx) * zy + cos(rx) * direction.z };
                    Vec3f probe = { pos.x + direction.x, pos.y + direction.y, pos.z + direction.z };
                    Vec3f actualProbe = Transform(probe);
                    float directionError =
                        sqrtf(SQ(actualProbe.x - actual.x - turned.x) + SQ(actualProbe.y - actual.y - turned.y) +
                              SQ(actualProbe.z - actual.z - turned.z));
                    if (directionError > 0.001f) {
                        if (!failures)
                            fprintf(stderr, "FAIL actor=%d case=%d flags=%u: look direction error %.3f\n",
                                    MALON_TRACKING_ACTOR, which, sFlags, directionError);
                        ++failures;
                    }
                    ++checks;
                }
            }
        }
#if MALON_TRACKING_ACTOR == 0
        /* Singing placements intentionally suppress additive tracking. */
        sFlags = 15;
        actor.staticState.pose = which ? 2 : 1;
        TEST_INFO(actor).headRot = angles[5];
        Identity();
        Vec3f pos = positions[which];
        Vec3s rot = { 0 };
        Gfx dummy[1], *display = dummy;
        callback(NULL, head, &display, &pos, &rot, &actor);
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                if (fabs(sMatrix[r][c] - (r == c)) > 0.000001)
                    ++failures;
        ++checks;
#endif
    }
    printf("%s Malon actor %d: %u callback checks, %u failures.\n", failures ? "FAIL" : "PASS", MALON_TRACKING_ACTOR,
           checks, failures);
    return failures ? 1 : 0;
}
