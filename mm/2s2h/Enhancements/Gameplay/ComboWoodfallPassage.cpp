#ifdef COMBO_BUILD
#include "ComboWoodfallPassage.h"
#include "ComboGameMode.h"
#include "entrances/LostWoodsEntrance.h"
#include <cmath>
#include <initializer_list>

namespace {
struct Passage {
    PlayState* play = nullptr;
    Vec3f entry{}, anchor{};
    s16 entryYaw = 0, yaw = 0;
    int room = -1, slot = -1;
    u32 seed = 0;
    bool queried = false, ready = false;
} sPassage;
constexpr float kAngle = 6.28318530718f / 65536.0f;

bool Floor(PlayState* play, Vec3f point, float expected, float* height) {
    point.y = expected + 100;
    CollisionPoly* floor = nullptr;
    s32 bg = BGCHECK_SCENE;
    const float y = BgCheck_EntityRaycastFloor5_3(play, &play->colCtx, &floor, &bg, &GET_PLAYER(play)->actor, &point);
    if (!floor || bg != BGCHECK_SCENE || !std::isfinite(y) || y == BGCHECK_Y_MIN || std::fabs(y - expected) > 35 ||
        floor->normal.y < 23170 || SurfaceType_GetSceneExitIndex(&play->colCtx, floor, bg) != 0)
        return false;
    WaterBox* water = nullptr;
    float waterY;
    if (WaterBox_GetSurface1_2(play, &play->colCtx, point.x, point.z, &waterY, &water) && waterY >= y - 0.5f)
        return false;
    *height = y;
    return true;
}

bool FindWall(PlayState* play, Vec3f* anchor, s16* yaw) {
    const float sine = std::sin(sPassage.entryYaw * kAngle), cosine = std::cos(sPassage.entryYaw * kAngle);
    // The short left/back wall beside spawn zero has a dry apron. Forward of
    // that spawn is the basin, so searching farther ahead would skip this wall.
    // Native midpoint (-200.5, 12, -1483), relative to entry (-41, 12, -1353).
    const Vec3f center{ sPassage.entry.x - cosine * 159.5f - sine * 130.0f, sPassage.entry.y,
                        sPassage.entry.z + sine * 159.5f - cosine * 130.0f };
    const float expectedX = cosine * -0.681291f + sine * 0.732013f;
    const float expectedZ = -sine * -0.681291f + cosine * 0.732013f;
    Vec3f from{ center.x + expectedX * 22, center.y, center.z + expectedZ * 22 };
    float floorY;
    if (!Floor(play, from, sPassage.entry.y, &floorY))
        return false;
    from.y = floorY + 60;
    Vec3f to{ center.x - expectedX * 40, from.y, center.z - expectedZ * 40 }, hit{};
    CollisionPoly* wall = nullptr;
    s32 bg = BGCHECK_SCENE;
    if (!BgCheck_EntityLineTest1(&play->colCtx, &from, &to, &hit, &wall, true, false, false, true, &bg) || !wall ||
        bg != BGCHECK_SCENE || std::abs(wall->normal.y) > 4096)
        return false;
    float nx = wall->normal.x / 32767.0f, nz = wall->normal.z / 32767.0f;
    const float length = std::sqrt(nx * nx + nz * nz);
    if (length < 0.9f)
        return false;
    nx /= length;
    nz /= length;
    if ((from.x - hit.x) * nx + (from.z - hit.z) * nz < 0) {
        nx = -nx;
        nz = -nz;
    }
    const float wallDistance = (from.x - hit.x) * nx + (from.z - hit.z) * nz;
    if (wallDistance < 18 || wallDistance > 26 || nx * expectedX + nz * expectedZ < 0.99f)
        return false;
    Vec3f mouth{ hit.x + nx * 2, floorY, hit.z + nz * 2 };
    // Check the whole frame against the static wall, including its top corners.
    // A changed scene fails closed instead of drawing over water or an exit.
    for (const float width : { -COMBO_WOODFALL_PASSAGE_HALF_WIDTH, 0.0f, COMBO_WOODFALL_PASSAGE_HALF_WIDTH }) {
        for (const float height : { 60.0f, 136.0f }) {
            Vec3f a{ mouth.x + nx * 20 + nz * width, floorY + height, mouth.z + nz * 20 - nx * width };
            Vec3f b{ a.x - nx * 60, a.y, a.z - nz * 60 }, wallHit{};
            CollisionPoly* frameWall = nullptr;
            if (!BgCheck_EntityLineTest1(&play->colCtx, &a, &b, &wallHit, &frameWall, true, false, false, true, &bg) ||
                !frameWall || bg != BGCHECK_SCENE || std::fabs((wallHit.x - hit.x) * nx + (wallHit.z - hit.z) * nz) > 2)
                return false;
        }
        float y;
        const Vec3f foot{ mouth.x + nz * width, floorY, mouth.z - nx * width };
        if (!Floor(play, foot, floorY, &y) || std::fabs(y - floorY) > 15)
            return false;
    }
    // The native apron is shallow. This narrow approach reaches beyond Fierce
    // Deity's 27-unit wall radius while staying on dry, non-exit floor.
    for (const float depth : { 16.0f, 30.0f }) {
        for (const float width : { -20.0f, 0.0f, 20.0f }) {
            const Vec3f sample{ mouth.x + nx * depth + nz * width, floorY, mouth.z + nz * depth - nx * width };
            float y;
            if (!Floor(play, sample, floorY, &y) || std::fabs(y - floorY) > 15)
                return false;
        }
    }
    *anchor = mouth;
    *yaw = static_cast<s16>(std::lround(std::atan2(nx, nz) / kAngle));
    return true;
}
} // namespace

extern "C" void ComboWoodfallPassage_Reset(void) {
    sPassage = {};
}

extern "C" void ComboWoodfallPassage_ReadSpawns(PlayState* play, const ActorEntry* entries, unsigned count) {
    sPassage = {};
    if (!Combo_IsCombinedGame() || !play || play->sceneId != SCENE_21MITURINMAE || !entries ||
        !play->setupEntranceList || !count)
        return;
    const auto entry = play->setupEntranceList[0];
    if (entry.spawn >= count)
        return;
    const auto& source = entries[entry.spawn];
    sPassage.play = play;
    sPassage.entry = { static_cast<float>(source.pos.x), static_cast<float>(source.pos.y),
                       static_cast<float>(source.pos.z) };
    // ActorEntry rotations pack degree angles above seven cutscene bits.
    // Match Actor_SpawnEntry instead of treating this resource value as binang.
    s16 yaw = (source.rot.y >> 7) & 0x1FF;
    if (!(source.id & 0x8000))
        yaw = DEG_TO_BINANG(yaw);
    else if (yaw > 180)
        yaw -= 360;
    sPassage.entryYaw = yaw;
    sPassage.room = entry.room;
    sPassage.slot = gSaveContext.fileNum;
    sPassage.seed = gSaveContext.save.shipSaveInfo.rando.finalSeed;
}

extern "C" int ComboWoodfallPassage_Get(PlayState* play, Vec3f* anchor, s16* yaw) {
    if (!Combo_IsCombinedGame() || !play || play != sPassage.play || play->sceneId != SCENE_21MITURINMAE ||
        play->roomCtx.curRoom.num != sPassage.room || gSaveContext.gameMode != GAMEMODE_NORMAL ||
        !ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) || sPassage.slot != gSaveContext.fileNum ||
        sPassage.seed != gSaveContext.save.shipSaveInfo.rando.finalSeed || !play->colCtx.colHeader || !GET_PLAYER(play))
        return false;
    if (!sPassage.queried) {
        sPassage.queried = true;
        sPassage.ready = FindWall(play, &sPassage.anchor, &sPassage.yaw);
    }
    if (!sPassage.ready)
        return false;
    if (anchor)
        *anchor = sPassage.anchor;
    if (yaw)
        *yaw = sPassage.yaw;
    return true;
}

extern "C" int ComboWoodfallPassage_AtReturn(PlayState* play, const Vec3f* position) {
    Vec3f anchor;
    s16 yaw;
    if (!position || !ComboWoodfallPassage_Get(play, &anchor, &yaw))
        return false;
    const float x = position->x - anchor.x, z = position->z - anchor.z;
    const float sine = std::sin(yaw * kAngle), cosine = std::cos(yaw * kAngle);
    const float width = x * cosine - z * sine, front = x * sine + z * cosine;
    // Trigger just before native wall collision. Passing along the original
    // approach stays outside this depth and never becomes a Hyrule exit.
    return std::fabs(width) < 20 && front > -8 && front < 30 && position->y > anchor.y - 20 &&
           position->y < anchor.y + 120;
}
#endif
