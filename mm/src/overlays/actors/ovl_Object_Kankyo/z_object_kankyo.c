/*
 * File: z_object_kankyo.c
 * Overlay: ovl_Object_Kankyo
 * Description: Snow, rain in Skull Kid backstory cutscene, and bubbles deep in Pinnacle Rock
 */

#include "z_object_kankyo.h"
#include "2s2h/Enhancements/Audio/MMWeather.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include "BenPort.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include "mods/items/objects/object_autumn_leaves.h"
#include "mods/items/objects/object_spring_petals.h"
#include "mods/items/helpers/season_particles.h"

#define FLAGS (ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED | ACTOR_FLAG_UPDATE_DURING_OCARINA)

void ObjectKankyo_Init(Actor* thisx, PlayState* play);
void ObjectKankyo_Destroy(Actor* thisx, PlayState* play);
void ObjectKankyo_Update(Actor* thisx, PlayState* play);
void ObjectKankyo_Draw(Actor* thisx, PlayState* play);

void ObjectKankyo_SetupAction(ObjectKankyo* this, ObjectKankyoActionFunc actionFunc);
void func_808DC18C(ObjectKankyo* this, PlayState* play);
void func_808DCB7C(ObjectKankyo* this, PlayState* play);
void func_808DCBF8(ObjectKankyo* this, PlayState* play);
void func_808DCDB4(ObjectKankyo* this, PlayState* play);
void func_808DD3C8(Actor* thisx, PlayState* play2);
void func_808DD970(Actor* thisx, PlayState* play2);
void func_808DDE9C(Actor* thisx, PlayState* play2);

static f32 D_808DE5B0;

ActorProfile Object_Kankyo_Profile = {
    /**/ ACTOR_OBJECT_KANKYO,
    /**/ ACTORCAT_ITEMACTION,
    /**/ FLAGS,
    /**/ GAMEPLAY_KEEP,
    /**/ sizeof(ObjectKankyo),
    /**/ ObjectKankyo_Init,
    /**/ ObjectKankyo_Destroy,
    /**/ ObjectKankyo_Update,
    /**/ ObjectKankyo_Draw,
};

static u16 D_808DE340 = 0;

void ObjectKankyo_SetupAction(ObjectKankyo* this, ObjectKankyoActionFunc actionFunc) {
    this->actionFunc = actionFunc;
}

void func_808DBE8C(ObjectKankyo* this) {
    ObjectKankyo_SetupAction(this, func_808DC18C);
}

void func_808DBEB0(ObjectKankyo* this, PlayState* play) {
    s32 i;
    f32 (*particleRandom)(void) =
        MMSeasonParticles_IsFallingSeason(MMWeather_SeasonForPlay(play)) ? MMWeather_RandomFloat : Rand_ZeroOne;

    D_808DE5B0 = 0.0f;
    this->unk_144 = particleRandom() * 360.0f;
    this->unk_148 = particleRandom() * 360.0f;
    if (play->envCtx.precipitation[PRECIP_SNOW_CUR] == 128) {
        D_808DE5B0 = 1.0f;
        this->unk_114E = 1;

        for (i = 0; i < play->envCtx.precipitation[PRECIP_SNOW_CUR]; i++) {
            this->unk_14C[i].unk_10 = particleRandom() * -200.0f;
        }
    } else {
        this->unk_114E = 0;
    }
    ObjectKankyo_SetupAction(this, func_808DCB7C);
}

void func_808DBFB0(ObjectKankyo* this, PlayState* play) {
    f32 (*particleRandom)(void) =
        MMSeasonParticles_IsFallingSeason(MMWeather_SeasonForPlay(play)) ? MMWeather_RandomFloat : Rand_ZeroOne;
    D_808DE5B0 = 0.0f;
    this->unk_114E = 0;
    this->unk_144 = particleRandom() * 360.0f;
    this->unk_148 = particleRandom() * 360.0f;
    this->unk_114C = D_808DE340;
    D_808DE340++;
    ObjectKankyo_SetupAction(this, func_808DCBF8);
}

void func_808DC038(ObjectKankyo* this, PlayState* play) {
    s16 i;

    this->unk_144 = Rand_ZeroOne() * 360.0f;
    this->unk_148 = Rand_ZeroOne() * 360.0f;
    this->unk_114C = 0;

    for (i = 0; i < ARRAY_COUNT(this->unk_14C); i++) {
        this->unk_14C[i].unk_1C = 0;
    }

    ObjectKankyo_SetupAction(this, func_808DCDB4);
}

void ObjectKankyo_Init(Actor* thisx, PlayState* play) {
    ObjectKankyo* this = (ObjectKankyo*)thisx;
    s16 i;

    for (i = 0; i < ARRAY_COUNT(this->unk_14C); i++) {
        this->unk_14C[i].unk_1C = 0;
    }

    this->actor.room = -1;
    switch (this->actor.params) {
        case 0:
            func_808DBE8C(this);
            break;

        case 2:
            play->envCtx.precipitation[PRECIP_SNOW_CUR] = 128;
            func_808DBFB0(this, play);
            break;

        case 1:
        case 3:
            func_808DBEB0(this, play);
            break;

        case 4:
            func_808DC038(this, play);
            break;
    }
}

void ObjectKankyo_Destroy(Actor* thisx, PlayState* play) {
    ObjectKankyo* this = (ObjectKankyo*)thisx;

    Actor_Kill(&this->actor);
}

void func_808DC18C(ObjectKankyo* this, PlayState* play) {
    s32 pad;
    s32 pad2;
    s32 pad3;
    f32 magnitude;
    f32 y;
    f32 z;
    f32 temp_f18;
    Vec3f sp30;
    f32 x;
    f32 sp1C;

    x = play->view.at.x - play->view.eye.x;
    y = play->view.at.y - play->view.eye.y;
    z = play->view.at.z - play->view.eye.z;
    magnitude = sqrtf(SQ(x) + SQ(y) + SQ(z));

    temp_f18 = x / magnitude;
    x = z / magnitude;
    sp1C = (y / magnitude) * 120.0f;

    this->unk_14C[0].unk_00 = play->view.eye.x + (temp_f18 * 50.0f);
    this->unk_14C[0].unk_04 = play->view.eye.y + sp1C;
    this->unk_14C[0].unk_08 = play->view.eye.z + (x * 50.0f);
    this->unk_14C[1].unk_00 = play->view.eye.x + (temp_f18 * 220.0f);
    this->unk_14C[1].unk_08 = play->view.eye.z + (x * 220.0f);
    this->unk_114C = 0;
    this->unk_144 = 100.0f;

    if ((this->unk_14C[0].unk_00 < -252.0f) && (this->unk_14C[0].unk_00 > -500.0f)) {
        if ((this->unk_14C[0].unk_08 > 3820.0f) && (this->unk_14C[0].unk_08 < 4150.0f)) {
            this->unk_114C = 1;
            this->unk_144 = 400.0f;
            if (x < 0.0f) {
                this->unk_14C[0].unk_00 = -350.0f;
                this->unk_14C[0].unk_04 = play->view.eye.y + sp1C;
                this->unk_14C[0].unk_08 = 3680.0f;
                this->unk_14C[1].unk_00 = -350.0f;
                this->unk_14C[1].unk_08 = 3680.0f;
            } else {
                this->unk_14C[0].unk_00 = -350.0f;
                this->unk_14C[0].unk_04 = play->view.eye.y + sp1C;
                this->unk_14C[0].unk_08 = 4280.0f;
                this->unk_14C[1].unk_00 = -350.0f;
                this->unk_14C[1].unk_08 = 4280.0f;
            }
        }
    }

    magnitude = play->envCtx.windSpeed / 60.0f;
    magnitude = CLAMP(magnitude, 0.0f, 1.0f);

    sp30.x = play->envCtx.windDirection.x * magnitude;
    sp30.y = play->envCtx.windDirection.y + 100.0f;
    sp30.z = play->envCtx.windDirection.z * magnitude;
    this->unk_14C[2].unk_00 = 0x4000 - Math_Vec3f_Pitch(&gZeroVec3f, &sp30);
    this->unk_14C[2].unk_04 = Math_Vec3f_Yaw(&gZeroVec3f, &sp30) + 0x8000;
}

static s32 ObjectKankyo_IsAutumnOwner(ObjectKankyo* this, PlayState* play) {
    for (Actor* actor = play->actorCtx.actorLists[ACTORCAT_ITEMACTION].first; actor != NULL; actor = actor->next) {
        if (actor->id == ACTOR_OBJECT_KANKYO && actor->params >= 1 && actor->params <= 3 && actor->update != NULL) {
            return actor == &this->actor;
        }
    }
    return false;
}

static s32 ObjectKankyo_AutumnBand(s32 index) {
    return index < OBJECT_KANKYO_AUTUMN_NEAR_COUNT                                       ? 0
           : index < OBJECT_KANKYO_AUTUMN_NEAR_COUNT + OBJECT_KANKYO_AUTUMN_MIDDLE_COUNT ? 1
                                                                                         : 2;
}

static s32 ObjectKankyo_IsCompactAutumnScene(PlayState* play) {
    return play->sceneId == SCENE_TOWN || play->sceneId == SCENE_ICHIBA || play->sceneId == SCENE_BACKTOWN ||
           play->sceneId == SCENE_CLOCKTOWER || play->sceneId == SCENE_ALLEY;
}

static void ObjectKankyo_AutumnViewBasis(PlayState* play, Vec3f* forward, Vec3f* right, Vec3f* up) {
    *forward = (Vec3f){ play->view.at.x - play->view.eye.x, play->view.at.y - play->view.eye.y,
                        play->view.at.z - play->view.eye.z };
    f32 length = sqrtf(SQ(forward->x) + SQ(forward->y) + SQ(forward->z));
    if (length < 0.001f) {
        *forward = (Vec3f){ 0.0f, 0.0f, 1.0f };
    } else {
        forward->x /= length;
        forward->y /= length;
        forward->z /= length;
    }
    Vec3f viewUp = play->view.up;
    if (SQ(viewUp.x) + SQ(viewUp.y) + SQ(viewUp.z) < 0.001f)
        viewUp = (Vec3f){ 0.0f, 1.0f, 0.0f };
    *right = (Vec3f){ viewUp.y * forward->z - viewUp.z * forward->y, viewUp.z * forward->x - viewUp.x * forward->z,
                      viewUp.x * forward->y - viewUp.y * forward->x };
    length = sqrtf(SQ(right->x) + SQ(right->y) + SQ(right->z));
    if (length < 0.001f) {
        *right = (Vec3f){ 1.0f, 0.0f, 0.0f };
    } else {
        right->x /= length;
        right->y /= length;
        right->z /= length;
    }
    *up = (Vec3f){ forward->y * right->z - forward->z * right->y, forward->z * right->x - forward->x * right->z,
                   forward->x * right->y - forward->y * right->x };
}

static void ObjectKankyo_InitAutumnParticle(ObjectKankyo* this, PlayState* play, s32 index) {
    static const f32 minimum[] = { 700.0f, 1200.0f, 3200.0f };
    static const f32 maximum[] = { 1000.0f, 2800.0f, 6000.0f };
    const s32 band = ObjectKankyo_AutumnBand(index);
    ObjectKankyoStruct* particle = &this->unk_14C[index];
    Vec3f forward, right, up;
    ObjectKankyo_AutumnViewBasis(play, &forward, &right, &up);
    const Camera* camera = GET_ACTIVE_CAM(play);
    const f32 fov = play->view.fovy > 0.0f                 ? play->view.fovy
                    : camera != NULL && camera->fov > 0.0f ? camera->fov
                                                           : 60.0f;
    const f32 halfHeight = tanf(CLAMP(fov, 5.0f, 150.0f) * 0.00872665f);
    const f32 aspect = MAX(OTRGetAspectRatio(), 4.0f / 3.0f);
    // Permute a jittered 12x8 layout so every depth band covers the whole view.
    // The small near leaves stay farther from the camera; no budget sits behind it.
    const s32 cell = (index * 37) % OBJECT_KANKYO_AUTUMN_COUNT;
    const f32 x = (((cell % 12 + 0.15f + MMWeather_RandomFloat() * 0.7f) / 12.0f) * 2.0f - 1.0f) * 0.85f;
    const f32 y = (((cell / 12 + 0.15f + MMWeather_RandomFloat() * 0.7f) / 8.0f) * 2.0f - 1.0f) * 0.85f;
    f32 radius = minimum[band] + MMWeather_RandomFloat() * (maximum[band] - minimum[band]);
    if (band != 0 && ObjectKankyo_IsCompactAutumnScene(play)) {
        radius = MAX(radius * 0.3f, 600.0f);
    }
    Vec3f ray = { forward.x + right.x * x * halfHeight * aspect + up.x * y * halfHeight,
                  forward.y + right.y * x * halfHeight * aspect + up.y * y * halfHeight,
                  forward.z + right.z * x * halfHeight * aspect + up.z * y * halfHeight };
    radius /= sqrtf(SQ(ray.x) + SQ(ray.y) + SQ(ray.z));
    particle->unk_00 = play->view.eye.x + ray.x * radius;
    particle->unk_04 = play->view.eye.y + ray.y * radius;
    particle->unk_08 = play->view.eye.z + ray.z * radius;
    particle->unk_0C = particle->unk_10 = particle->unk_14 = 0.0f;
    particle->unk_18 = 1.0f; // Begin faintly, then fade in; every slot is submitted in view.
    particle->unk_1C = 2;
    particle->epoch++;
}

static void ObjectKankyo_UpdateAutumnParticles(ObjectKankyo* this, PlayState* play) {
    if (!ObjectKankyo_IsAutumnOwner(this, play)) {
        return;
    }
    const f32 windLength = sqrtf(SQ((f32)play->envCtx.windDirection.x) + SQ((f32)play->envCtx.windDirection.z));
    const f32 wind = CLAMP(play->envCtx.windSpeed / 120.0f, 0.0f, 1.0f);
    for (s32 i = 0; i < OBJECT_KANKYO_AUTUMN_COUNT; ++i) {
        ObjectKankyoStruct* particle = &this->unk_14C[i];
        const s32 band = ObjectKankyo_AutumnBand(i);
        if (particle->unk_1C != 2) {
            ObjectKankyo_InitAutumnParticle(this, play, i);
        }
        const f32 phase = play->gameplayFrames * 0.055f + i * 2.4f;
        const f32 flutter = 0.9f + band * 0.45f;
        particle->unk_0C += sinf(phase) * flutter;
        particle->unk_14 += cosf(phase * 0.7f) * flutter;
        if (windLength > 0.001f) {
            particle->unk_0C -= play->envCtx.windDirection.x / windLength * wind * (3.0f + band);
            particle->unk_14 -= play->envCtx.windDirection.z / windLength * wind * (3.0f + band);
        }
        particle->unk_10 -= 2.6f + band * 0.8f + (i & 7) * 0.24f;
        particle->unk_18 = MIN(particle->unk_18 + 1.0f, 24.0f);
    }
}

static void ObjectKankyo_RestoreAutumnParticle(ObjectKankyo* this, PlayState* play, s32 index) {
    ObjectKankyoStruct* particle = &this->unk_14C[index];
    Vec3f direction = { play->view.at.x - play->view.eye.x, play->view.at.y - play->view.eye.y,
                        play->view.at.z - play->view.eye.z };
    const f32 length = sqrtf(SQ(direction.x) + SQ(direction.y) + SQ(direction.z));
    const f32 scale = length > 0.001f ? 120.0f / length : 0.0f;
    // Match the native snow's initial volume/speed on the confirm render, which
    // can precede actor update. Cosmetic reinitialization uses the private RNG.
    particle->unk_00 = play->view.eye.x + direction.x * scale;
    particle->unk_04 = play->view.eye.y + direction.y * scale;
    particle->unk_08 = play->view.eye.z + (length > 0.001f ? direction.z * scale : 120.0f);
    particle->unk_0C = (MMWeather_RandomFloat() - 0.5f) * 240.0f;
    particle->unk_10 = MAX(Camera_GetCamDirPitch(GET_ACTIVE_CAM(play)) * 0.004f + 60.0f, 20.0f);
    particle->unk_14 = (MMWeather_RandomFloat() - 0.5f) * 240.0f;
    particle->unk_18 = MMWeather_RandomFloat() * 3.0f + (play->envCtx.precipitation[PRECIP_SOS_MAX] ? 8.0f : 1.0f);
    particle->unk_1C = 1;
    particle->epoch++;
}

void func_808DC454(ObjectKankyo* this, PlayState* play) {
    s16 i;
    s32 pad1;
    f32 phi_f20;
    f32 spD0;
    f32 spCC;
    f32 spC8;
    f32 spC4;
    f32 spC0;
    f32 spBC;
    f32 temp_f0_4;
    f32 temp_f22;
    f32 temp_f24;
    f32 temp_f28;
    f32 x = play->view.at.x - play->view.eye.x;
    f32 y = play->view.at.y - play->view.eye.y;
    f32 z = play->view.at.z - play->view.eye.z;
    f32 magnitude = sqrtf(SQ(x) + SQ(y) + SQ(z));
    f32 temp_120 = 120.0f;
    f32 temp_f30;
    Vec3f sp88;
    s32 pad;

    spD0 = x / magnitude;
    spCC = y / magnitude;
    spC8 = z / magnitude;

    f32 (*particleRandom)(void) =
        MMSeasonParticles_IsFallingSeason(MMWeather_SeasonForPlay(play)) ? MMWeather_RandomFloat : Rand_ZeroOne;
    for (i = 0; i < play->envCtx.precipitation[PRECIP_SNOW_CUR]; i++) {
        if (this->unk_14C[i].unk_1C == 2) {
            // Distant positions and fade age cannot become native snow speed.
            ObjectKankyo_RestoreAutumnParticle(this, play, i);
        }
        switch (this->unk_14C[i].unk_1C) {
            case 0:
                this->unk_14C[i].unk_00 = play->view.eye.x + (spD0 * 120.0f);
                this->unk_14C[i].unk_04 = play->view.eye.y + (spCC * 120.0f);
                this->unk_14C[i].unk_08 = play->view.eye.z + (spC8 * 120.0f);
                this->unk_14C[i].unk_0C = (particleRandom() - 0.5f) * (2.0f * temp_120);

                temp_f22 = (Camera_GetCamDirPitch(GET_ACTIVE_CAM(play)) * 0.004f) + 60.0f;
                if (temp_f22 < 20.0f) {
                    temp_f22 = 20.0f;
                }

                if (this->unk_114E == 0) {
                    this->unk_14C[i].unk_10 = temp_f22;
                } else {
                    this->unk_14C[i].unk_10 += temp_f22;
                    if (play->envCtx.precipitation[PRECIP_SNOW_CUR] == ((u32)i + 1)) {
                        this->unk_114E = 0;
                    }
                }

                this->unk_14C[i].unk_14 = (particleRandom() - 0.5f) * (2.0f * temp_120);
                if (play->envCtx.precipitation[PRECIP_SOS_MAX] == 0) {
                    this->unk_14C[i].unk_18 = (particleRandom() * 3.0f) + 1.0f;
                } else {
                    this->unk_14C[i].unk_18 = (particleRandom() * 3.0f) + 8.0f;
                }
                this->unk_14C[i].unk_1C++;
                this->unk_14C[i].epoch++;
                break;

            case 1:
                temp_f24 = play->view.eye.x + (spD0 * 120.0f);
                temp_f28 = play->view.eye.y + (spCC * 120.0f);
                temp_f30 = play->view.eye.z + (spC8 * 120.0f);

                magnitude = sqrtf((f32)SQ(play->envCtx.windDirection.x) + SQ(play->envCtx.windDirection.y) +
                                  SQ(play->envCtx.windDirection.z));
                if (magnitude == 0.0f) {
                    magnitude = 0.001f;
                }
                spC4 = -play->envCtx.windDirection.x / magnitude;
                spC0 = -play->envCtx.windDirection.y / magnitude;
                spBC = -play->envCtx.windDirection.z / magnitude;

                if (i == 0) {
                    this->unk_144 += 0.049999997f * particleRandom();
                    this->unk_148 += 0.049999997f * particleRandom();
                }

                phi_f20 = play->envCtx.windSpeed / 120.0f;
                phi_f20 = CLAMP(phi_f20, 0.0f, 1.0f);

                this->unk_14C[i].unk_0C += sinf((this->unk_144 + (i * 100.0f)) * 0.01f) + (spC4 * 10.0f * phi_f20);
                this->unk_14C[i].unk_14 += cosf((this->unk_148 + (i * 100.0f)) * 0.01f) + (spBC * 10.0f * phi_f20);
                this->unk_14C[i].unk_10 -= this->unk_14C[i].unk_18 - (spC0 * 3.0f * (play->envCtx.windSpeed / 100.0f));

                temp_f22 = (-Camera_GetCamDirPitch(GET_ACTIVE_CAM(play)) * 0.012f) + 40.0f;
                if (temp_f22 < -40.0f) {
                    temp_f22 = -40.0f;
                }

                if (((this->unk_14C[i].unk_00 + this->unk_14C[i].unk_0C) - temp_f24) > temp_120) {
                    this->unk_14C[i].unk_00 = temp_f24 - temp_120;
                    // 2S2H [Interpolation] Here and below, skip particle interp on next frame when position moves
                    this->unk_14C[i].epoch++;
                }

                if (((this->unk_14C[i].unk_00 + this->unk_14C[i].unk_0C) - temp_f24) < -temp_120) {
                    this->unk_14C[i].unk_00 = temp_f24 + temp_120;
                    this->unk_14C[i].epoch++;
                }

                sp88.x = this->unk_14C[i].unk_00 + this->unk_14C[i].unk_0C;
                sp88.y = this->unk_14C[i].unk_04 + this->unk_14C[i].unk_10;
                sp88.z = this->unk_14C[i].unk_08 + this->unk_14C[i].unk_14;

                phi_f20 = Math_Vec3f_DistXZ(&sp88, &play->view.eye) / 200.0f;
                phi_f20 = CLAMP(phi_f20, 0.0f, 1.0f);
                temp_f0_4 = 100.0f + phi_f20 + 60.0f;

                if (temp_f0_4 < (this->unk_14C[i].unk_04 + (this->unk_14C[i].unk_10) - temp_f28)) {
                    this->unk_14C[i].unk_04 = temp_f28 - temp_f0_4;
                    this->unk_14C[i].epoch++;
                }

                if (((this->unk_14C[i].unk_04 + this->unk_14C[i].unk_10) - temp_f28) < -temp_f0_4) {
                    this->unk_14C[i].unk_04 = temp_f28 + temp_f0_4;
                    this->unk_14C[i].epoch++;
                }

                if (((this->unk_14C[i].unk_08 + this->unk_14C[i].unk_14) - temp_f30) > temp_120) {
                    this->unk_14C[i].unk_08 = temp_f30 - temp_120;
                    this->unk_14C[i].epoch++;
                }

                if (((this->unk_14C[i].unk_08 + this->unk_14C[i].unk_14) - temp_f30) < -temp_120) {
                    this->unk_14C[i].unk_08 = temp_f30 + temp_120;
                    this->unk_14C[i].epoch++;
                }

                if ((this->unk_14C[i].unk_04 + this->unk_14C[i].unk_10) < ((play->view.eye.y - temp_f22) - 40.0f)) {
                    this->unk_14C[i].unk_1C = 0;
                }
                break;
        }
    }
}

static void ObjectKankyo_UpdateSnowTarget(PlayState* play) {
    if ((play->state.frames % 16) != 0) {
        return;
    }
    u8* count = &play->envCtx.precipitation[PRECIP_SNOW_CUR];
    u8 target = play->envCtx.precipitation[PRECIP_SNOW_MAX];
    // Clamp a final odd step so native target changes converge exactly.
    if (*count < target) {
        *count += MIN(2, target - *count);
    } else if (*count > target) {
        *count -= MIN(2, *count - target);
    }
}

static void ObjectKankyo_UpdateSeasonSnowParticles(ObjectKankyo* this, PlayState* play) {
    const int season = MMWeather_SeasonForPlay(play);
    const u8 nativeCount = play->envCtx.precipitation[PRECIP_SNOW_CUR];
    if (MMSeasonParticles_IsFallingSeason(season)) {
        if (MMSeasonParticles_IsEnabled(season)) {
            ObjectKankyo_UpdateAutumnParticles(this, play);
        }
        return;
    }
    if (season == SEASON_WINTER || season == SEASON_SUMMER) {
        // Particle positions are native actor state. Compose their count only
        // during motion; weather tags and the next actor see the live native count.
        play->envCtx.precipitation[PRECIP_SNOW_CUR] =
            season == SEASON_WINTER && MMSeasonParticles_IsEnabled(season) ? 64 : 0;
    }
    func_808DC454(this, play);
    if (MMSeasonParticles_HasFlurries(season)) {
        // Optional windblown motion reuses the same 64 native snow slots and
        // texture. Deterministic gusts never consume gameplay RNG or weather state.
        const f32 windLength = sqrtf(SQ((f32)play->envCtx.windDirection.x) + SQ((f32)play->envCtx.windDirection.z));
        const f32 windX = windLength > 0.001f ? -play->envCtx.windDirection.x / windLength : 1.0f;
        const f32 windZ = windLength > 0.001f ? -play->envCtx.windDirection.z / windLength : 0.0f;
        const f32 gust = 1.8f + sinf(play->gameplayFrames * 0.03f) * 0.8f;
        for (s32 i = 0; i < 64; ++i) {
            const f32 drift = gust + sinf(play->gameplayFrames * 0.07f + i * 1.7f) * 0.4f;
            this->unk_14C[i].unk_0C += windX * drift;
            this->unk_14C[i].unk_14 += windZ * drift;
        }
    }
    play->envCtx.precipitation[PRECIP_SNOW_CUR] = nativeCount;
}

void func_808DCB7C(ObjectKankyo* this, PlayState* play) {
    ObjectKankyo_UpdateSnowTarget(play);
    ObjectKankyo_UpdateSeasonSnowParticles(this, play);
}

void func_808DCBF8(ObjectKankyo* this, PlayState* play) {
    f32 temp_f0;

    if ((play->envCtx.precipitation[PRECIP_SNOW_CUR] > 0) && (this->unk_114C == 0)) {
        if ((play->state.frames % 16) == 0) {
            play->envCtx.precipitation[PRECIP_SNOW_CUR] -= 9;
            if ((s8)play->envCtx.precipitation[PRECIP_SNOW_CUR] < 0) {
                play->envCtx.precipitation[PRECIP_SNOW_CUR] = 0;
            }
        }
    }

    temp_f0 = (f32)play->envCtx.precipitation[PRECIP_SNOW_CUR] / 128;
    temp_f0 = CLAMP(temp_f0, 0.0f, 1.0f);

    if (temp_f0 > 0.01f) {
        D_801F4E30 = 155.0f * temp_f0;
        play->envCtx.sandstormState = SANDSTORM_A;
    } else {
        D_801F4E30 = 0;
        play->envCtx.sandstormState = SANDSTORM_A;
    }
    ObjectKankyo_UpdateSeasonSnowParticles(this, play);
}

void func_808DCDB4(ObjectKankyo* this, PlayState* play) {
    s16 i;
    f32 magnitude;
    f32 temp_80;
    f32 temp_120;
    f32 spAC;
    f32 spA8;
    f32 spA4;
    f32 spA0;
    f32 sp9C;
    f32 x;
    f32 y;
    f32 z;
    f32 temp_f18;
    f32 temp_f20;
    f32 temp_f26;
    f32 temp_f28;

    if (this->unk_114C < 0x80) {
        this->unk_114C++;
    }

    x = play->view.at.x - play->view.eye.x;
    y = play->view.at.y - play->view.eye.y;
    z = play->view.at.z - play->view.eye.z;

    magnitude = sqrtf(SQ(x) + SQ(y) + SQ(z));

    spAC = x / magnitude;
    spA8 = y / magnitude;
    spA4 = z / magnitude;

    temp_80 = 80.0f;
    temp_120 = 120.0f;

    for (i = 0; i < this->unk_114C; i++) {
        switch (this->unk_14C[i].unk_1C) {
            case 0:
                this->unk_14C[i].unk_00 = play->view.eye.x + (spAC * 120.0f);
                this->unk_14C[i].unk_04 = play->view.eye.y + (spA8 * 120.0f);
                this->unk_14C[i].unk_08 = play->view.eye.z + (spA4 * 120.0f);
                this->unk_14C[i].unk_0C = (Rand_ZeroOne() - 0.5f) * (temp_120 * 2.0f);
                if ((i % 2) == 0) {
                    this->unk_14C[i].unk_10 = -100.0f;
                } else {
                    this->unk_14C[i].unk_10 = 100.0f;
                }
                this->unk_14C[i].unk_14 = (Rand_ZeroOne() - 0.5f) * (temp_120 * 2.0f);
                this->unk_14C[i].unk_18 = Rand_ZeroOne() + 0.2f;
                this->unk_14C[i].unk_1C++;
                this->unk_14C[i].epoch++;
                break;

            case 1:

                temp_f26 = play->view.eye.x + (spAC * 120.0f);
                temp_f28 = play->view.eye.y + (spA8 * 120.0f);
                temp_f18 = play->view.eye.z + (spA4 * 120.0f);

                magnitude = sqrtf((f32)SQ(play->envCtx.windDirection.x) + SQ(play->envCtx.windDirection.y) +
                                  SQ(play->envCtx.windDirection.z));
                if (magnitude == 0.0f) {
                    magnitude = 0.001f;
                }

                spA0 = -play->envCtx.windDirection.x / magnitude;
                sp9C = -play->envCtx.windDirection.z / magnitude;

                if (i == 0) {
                    this->unk_144 += 0.049999997f * Rand_ZeroOne();
                    this->unk_148 += 0.049999997f * Rand_ZeroOne();
                }
                temp_f20 = play->envCtx.windSpeed / 120.0f;
                temp_f20 = CLAMP(temp_f20, 0.0f, 1.0f);

                this->unk_14C[i].unk_0C += sinf((this->unk_144 + i) * 0.01f) + (spA0 * 10.0f * temp_f20);
                this->unk_14C[i].unk_14 += cosf((this->unk_148 + i) * 0.01f) + (sp9C * 10.0f * temp_f20);

                if ((i % 2) == 0) {
                    this->unk_14C[i].unk_10 += this->unk_14C[i].unk_18;
                    if ((this->unk_14C[i].unk_04 + this->unk_14C[i].unk_10) > (play->view.eye.y + 100.0f)) {
                        this->unk_14C[i].unk_1C = 0;
                    }
                } else {
                    this->unk_14C[i].unk_10 -= this->unk_14C[i].unk_18;
                    if ((this->unk_14C[i].unk_04 + this->unk_14C[i].unk_10) < (play->view.eye.y - 100.0f)) {
                        this->unk_14C[i].unk_1C = 0;
                    }
                }

                if (((this->unk_14C[i].unk_00 + this->unk_14C[i].unk_0C) - temp_f26) > temp_80) {
                    this->unk_14C[i].unk_00 = temp_f26 - temp_80;
                    // 2S2H [Interpolation] Here and below, skip particle interp on next frame when position moves
                    this->unk_14C[i].epoch++;
                }

                if (((this->unk_14C[i].unk_00 + this->unk_14C[i].unk_0C) - temp_f26) < -temp_80) {
                    this->unk_14C[i].unk_00 = temp_f26 + temp_80;
                    this->unk_14C[i].epoch++;
                }

                if (((this->unk_14C[i].unk_04 + this->unk_14C[i].unk_10) - temp_f28) > temp_80) {
                    this->unk_14C[i].unk_04 = temp_f28 - temp_80;
                    this->unk_14C[i].epoch++;
                }

                if (((this->unk_14C[i].unk_04 + this->unk_14C[i].unk_10) - temp_f28) < -temp_80) {
                    this->unk_14C[i].unk_04 = temp_f28 + temp_80;
                    this->unk_14C[i].epoch++;
                }

                if (((this->unk_14C[i].unk_08 + this->unk_14C[i].unk_14) - temp_f18) > temp_80) {
                    this->unk_14C[i].unk_08 = temp_f18 - temp_80;
                    this->unk_14C[i].epoch++;
                }

                if (((this->unk_14C[i].unk_08 + this->unk_14C[i].unk_14) - temp_f18) < -temp_80) {
                    this->unk_14C[i].unk_08 = temp_f18 + temp_80;
                    this->unk_14C[i].epoch++;
                }
                break;
        }
    }
}

void ObjectKankyo_Update(Actor* thisx, PlayState* play) {
    ObjectKankyo* this = (ObjectKankyo*)thisx;

    this->actionFunc(this, play);
}

void ObjectKankyo_Draw(Actor* thisx, PlayState* play) {
    ObjectKankyo* this = (ObjectKankyo*)thisx;

    switch (this->actor.params) {
        case 0:
            func_808DDE9C(thisx, play);
            break;

        case 1:
        case 2:
        case 3:
            func_808DD3C8(thisx, play);
            break;

        case 4:
            func_808DD970(thisx, play);
            break;
    }
}

void func_808DD3C8(Actor* thisx, PlayState* play2) {
    PlayState* play = play2;
    ObjectKankyo* this = (ObjectKankyo*)thisx;
    Vec3f worldPos;
    Vec3f screenPos;
    s16 i;
    u8 pad2;
    u8 spB4;
    f32 temp_f0;
    u8 sp68;
    s32 pad;
    f32 temp_f2;
    f32 tempf;

    const int season = MMWeather_SeasonForPlay(play);
    const s32 fallingSeason = MMSeasonParticles_IsFallingSeason(season);
    const f32 autumnFadeDistance = 8000.0f;
    const u8 snowCount = season == SEASON_WINTER ? 64
                         : fallingSeason         ? OBJECT_KANKYO_AUTUMN_COUNT
                                                 : play->envCtx.precipitation[PRECIP_SNOW_CUR];
    if ((play->cameraPtrs[CAM_ID_MAIN]->stateFlags & CAM_STATE_UNDERWATER) || (season == SEASON_SUMMER) ||
        ((fallingSeason || season == SEASON_WINTER) && !MMSeasonParticles_IsEnabled(season)) ||
        ((u8)play->envCtx.stormState == STORM_STATE_OFF && season != SEASON_WINTER && !fallingSeason)) {
        return;
    }

    if (fallingSeason && !ObjectKankyo_IsAutumnOwner(this, play)) {
        return; // Multiple native blizzard actors share one bounded leaf/petal layer.
    }

    OPEN_DISPS(play->state.gfxCtx);

    spB4 = false;

    if (this->actor.params == 3 && !fallingSeason) {
        temp_f0 = func_80173B48(&play->state) / 1.4e7f;
        temp_f0 = CLAMP(temp_f0, 0.0f, 1.0f);
        Math_SmoothStepToF(&D_808DE5B0, temp_f0, 0.2f, 0.1f, 0.001f);

        sp68 = snowCount;
        sp68 *= D_808DE5B0;

        if ((snowCount >= 32) && (sp68 < 32)) {
            sp68 = 32;
        }
    } else {
        sp68 = snowCount;
    }

    for (i = 0; i < sp68; i++) {
        if (fallingSeason && this->unk_14C[i].unk_1C != 2) {
            // Paused confirmation can draw before the next actor update.
            ObjectKankyo_InitAutumnParticle(this, play, i);
        } else if (!fallingSeason && this->unk_14C[i].unk_1C == 2) {
            ObjectKankyo_RestoreAutumnParticle(this, play, i);
        }
        worldPos.x = this->unk_14C[i].unk_00 + this->unk_14C[i].unk_0C;
        worldPos.y = this->unk_14C[i].unk_04 + this->unk_14C[i].unk_10;
        worldPos.z = this->unk_14C[i].unk_08 + this->unk_14C[i].unk_14;

        Play_GetScreenPos(play, &worldPos, &screenPos);

        // #region 2S2H [Cosmetic] Increase snow render area for widescreen
        f32 xMin = 0.0f;
        f32 xMax = SCREEN_WIDTH;
        if (OTRGetAspectRatio() > 4.0f / 3.0f) {
            xMin = OTRGetDimensionFromLeftEdge(xMin);
            xMax = OTRGetDimensionFromRightEdge(xMax);
        }
        // #endregion

        if (fallingSeason &&
            (screenPos.z <= 0.0f || screenPos.x < xMin || screenPos.x >= xMax || screenPos.y < 0.0f ||
             screenPos.y >= SCREEN_HEIGHT || Math_Vec3f_DistXYZ(&worldPos, &play->view.eye) >= autumnFadeDistance)) {
            // Recycle only offscreen or fully distance-faded leaves. A steep
            // downward view can keep falling leaves onscreen past the fade range.
            // The new epoch skips interpolation; each replacement fades in alone.
            // Visible opaque leaves retain their world-space trajectories.
            ObjectKankyo_InitAutumnParticle(this, play, i);
            worldPos = (Vec3f){ this->unk_14C[i].unk_00, this->unk_14C[i].unk_04, this->unk_14C[i].unk_08 };
            Play_GetScreenPos(play, &worldPos, &screenPos);
        }

        if ((screenPos.x >= xMin) && (screenPos.x < xMax) && (screenPos.y >= 0.0f) && (screenPos.y < SCREEN_HEIGHT)) {
            FrameInterpolation_RecordOpenChild(&this->unk_14C[i], this->unk_14C[i].epoch);
            if (!spB4) {
                spB4 = true;

                gDPPipeSync(POLY_XLU_DISP++);
                gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
                gSPClearGeometryMode(POLY_XLU_DISP++, G_LIGHTING);

                POLY_XLU_DISP = Gfx_SetupDL(POLY_XLU_DISP, SETUPDL_0);

                gDPSetRenderMode(POLY_XLU_DISP++, G_RM_FOG_SHADE_A, G_RM_ZB_CLD_SURF2);
                gSPSetGeometryMode(POLY_XLU_DISP++, G_FOG);
                gSPSegment(POLY_XLU_DISP++, 0x08, Lib_SegmentedToVirtual(gEffDust5Tex));
            }

            Matrix_Translate(worldPos.x, worldPos.y, worldPos.z, MTXMODE_NEW);
            if (fallingSeason) {
                const s32 band = ObjectKankyo_AutumnBand(i);
                f32 leafScale = (0.035f + (i & 7) * 0.002f) * (1.0f + band * 0.7f);
                Matrix_Mult(&play->billboardMtxF, MTXMODE_APPLY);
                Matrix_RotateZS((s16)(play->gameplayFrames * 180 + i * 1300), MTXMODE_APPLY);
                Matrix_Scale(leafScale, leafScale, leafScale, MTXMODE_APPLY);
                const f32 distance = Math_Vec3f_DistXYZ(&worldPos, &play->view.eye);
                Actor* player = play->actorCtx.actorLists[ACTORCAT_PLAYER].first;
                const f32 nearDistance =
                    player != NULL ? MIN(distance, Math_Vec3f_DistXYZ(&worldPos, &player->world.pos)) : distance;
                temp_f2 = CLAMP((autumnFadeDistance - distance) / 2000.0f, 0.0f, 1.0f) *
                          CLAMP((nearDistance - 150.0f) / 200.0f, 0.0f, 1.0f) *
                          MIN(this->unk_14C[i].unk_18 / 24.0f, 1.0f);
                const f32 edgeFade =
                    CLAMP(MIN(screenPos.x - xMin, xMax - screenPos.x) / ((xMax - xMin) * 0.1f), 0.0f, 1.0f) *
                    CLAMP(MIN(screenPos.y, SCREEN_HEIGHT - screenPos.y) / (SCREEN_HEIGHT * 0.1f), 0.0f, 1.0f);
                u8 alpha = (u8)(220.0f * temp_f2 * edgeFade);
                // Keep edge-fading slots faintly present; the camera/Link clear
                // pocket and far-distance fade still suppress them completely.
                if (alpha == 0 && temp_f2 > 0.0f)
                    alpha = 1;
                if (season == SEASON_SPRING) {
                    // Petals rock about their own axis, while keeping the accepted
                    // density, clear pocket, lifetime and recycling volume.
                    const f32 flutter = 0.65f + sinf(play->gameplayFrames * 0.045f + i * 2.4f) * 0.25f;
                    Matrix_Scale(flutter, 1.0f, 1.0f, MTXMODE_APPLY);
                    SpringPetals_Draw(play, i, alpha);
                } else {
                    AutumnLeaves_Draw(play, i, alpha);
                }
                FrameInterpolation_RecordCloseChild();
                continue;
            }
            tempf = (i & 7) * 0.008f;
            Matrix_Scale(0.05f + tempf, 0.05f + tempf, 0.05f + tempf, MTXMODE_APPLY);
            temp_f2 = Math_Vec3f_DistXYZ(&worldPos, &play->view.eye) / 300.0f;
            temp_f2 = ((1.0f < temp_f2) ? 0.0f : (((1.0f - temp_f2) > 1.0f) ? 1.0f : 1.0f - temp_f2));

            gDPPipeSync(POLY_XLU_DISP++);
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, (u8)(160.0f * temp_f2));

            Matrix_Mult(&play->billboardMtxF, MTXMODE_APPLY);

            MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
            gSPDisplayList(POLY_XLU_DISP++, gEffDustDL);
            FrameInterpolation_RecordCloseChild();
        }
    }

    if (season == SEASON_SPRING && spB4) {
        // Petals use an untextured material. Restore the standard textured XLU
        // setup once for the next consumer; empty/culled layers change no state.
        POLY_XLU_DISP = Gfx_SetupDL(POLY_XLU_DISP, SETUPDL_0);
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

void func_808DD970(Actor* thisx, PlayState* play2) {
    f32 temp_f0;
    f32 temp_f20;
    Vec3f worldPos;
    Vec3f screenPos;
    f32 tempf;
    s16 i;
    f32 phi_f26;
    PlayState* play = play2;
    ObjectKankyo* this = (ObjectKankyo*)thisx;
    f32 tempA;

    if (play->sceneId == SCENE_KYOJINNOMA) {
        phi_f26 = 1.0f;
    } else {
        tempA = Camera_GetWaterYPos(GET_ACTIVE_CAM(play));
        if (tempA != BGCHECK_Y_MIN) {
            tempA -= play->view.eye.y;
            phi_f26 = tempA / 4000.0f;
        } else {
            phi_f26 = 0.0f;
        }

        phi_f26 = CLAMP_MAX(phi_f26, 1.0f);

        if (!(play->cameraPtrs[CAM_ID_MAIN]->stateFlags & CAM_STATE_UNDERWATER) || (phi_f26 == 0.0f)) {
            return;
        }
    }

    OPEN_DISPS(play->state.gfxCtx);

    for (i = 0; i < this->unk_114C; i++) {
        worldPos.x = this->unk_14C[i].unk_00 + this->unk_14C[i].unk_0C;
        worldPos.y = this->unk_14C[i].unk_04 + this->unk_14C[i].unk_10;
        worldPos.z = this->unk_14C[i].unk_08 + this->unk_14C[i].unk_14;

        Play_GetScreenPos(play, &worldPos, &screenPos);

        // #region 2S2H [Cosmetic] Increase deep underwater dust render area for widescreen
        f32 xMin = 0.0f;
        f32 xMax = SCREEN_WIDTH;
        if (OTRGetAspectRatio() > 4.0f / 3.0f) {
            xMin = OTRGetDimensionFromLeftEdge(xMin);
            xMax = OTRGetDimensionFromRightEdge(xMax);
        }
        // #endregion

        if ((screenPos.x >= xMin) && (screenPos.x < xMax) && (screenPos.y >= 0.0f) && (screenPos.y < SCREEN_HEIGHT)) {
            FrameInterpolation_RecordOpenChild(&this->unk_14C[i], this->unk_14C[i].epoch);
            Matrix_Translate(worldPos.x, worldPos.y, worldPos.z, MTXMODE_NEW);
            Matrix_Scale(0.03f, 0.03f, 0.03f, MTXMODE_APPLY);
            temp_f0 = Math_Vec3f_DistXYZ(&worldPos, &play->view.eye);
            temp_f0 = (u8)(255.0f * phi_f26) * (1.0f - (temp_f0 / 300.0f));

            gDPPipeSync(POLY_XLU_DISP++);
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 55, temp_f0);
            gDPSetEnvColor(POLY_XLU_DISP++, 55, 50, 255, temp_f0);

            Matrix_Mult(&play->billboardMtxF, MTXMODE_APPLY);

            MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
            gSPSegment(POLY_XLU_DISP++, 0x08, Lib_SegmentedToVirtual(gEffDust5Tex));
            gSPClearGeometryMode(POLY_XLU_DISP++, G_LIGHTING);

            POLY_XLU_DISP = Gfx_SetupDL(POLY_XLU_DISP, SETUPDL_0);

            gDPSetRenderMode(POLY_XLU_DISP++, G_RM_FOG_SHADE_A, G_RM_ZB_CLD_SURF2);
            gSPSetGeometryMode(POLY_XLU_DISP++, G_FOG);
            gSPDisplayList(POLY_XLU_DISP++, gEffDustDL);
            FrameInterpolation_RecordCloseChild();
        }
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

f32 func_808DDE74(void) {
    return Rand_ZeroOne() - 0.5f;
}

void func_808DDE9C(Actor* thisx, PlayState* play2) {
    PlayState* play = play2;
    ObjectKankyo* this = (ObjectKankyo*)thisx;
    Player* player = GET_PLAYER(play);
    s32 i;
    u8 phi_s5;
    u16 end = play->envCtx.precipitation[PRECIP_RAIN_CUR];
    f32 temp_f12;
    f32 temp_f20;
    f32 temp_f22;
    f32 temp_f2;

    OPEN_DISPS(play->state.gfxCtx);

    if (end) {
        gDPPipeSync(POLY_XLU_DISP++);
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 150, 255, 255, 25);
        POLY_XLU_DISP = Gfx_SetupDL(POLY_XLU_DISP, SETUPDL_20);
    }

    for (i = 0; i < end; i++) {
        temp_f20 = this->unk_14C[0].unk_00 + ((Rand_ZeroOne() - 0.7f) * this->unk_144);
        temp_f22 = this->unk_14C[0].unk_04 + ((Rand_ZeroOne() - 0.7f) * this->unk_144);
        temp_f2 = this->unk_14C[0].unk_08 + ((Rand_ZeroOne() - 0.7f) * this->unk_144);

        if (!((temp_f20 < -252.0f) && (temp_f20 > -500.0f) && (temp_f2 > 3820.0f) && (temp_f2 < 4150.0f))) {
            FrameInterpolation_RecordOpenChild(this, i);
            Matrix_Translate(temp_f20, temp_f22, temp_f2, MTXMODE_NEW);

            gSPMatrix(POLY_XLU_DISP++, D_01000000_TO_SEGMENTED, G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);

            Matrix_RotateYS(TRUNCF_BINANG(this->unk_14C[2].unk_04) + (s16)(i << 5), MTXMODE_APPLY);
            Matrix_RotateXS(TRUNCF_BINANG(this->unk_14C[2].unk_00) + (s16)(i << 5), MTXMODE_APPLY);

            if (this->unk_114C == 0) {
                Matrix_Scale(0.5f, 1.0f, 0.5f, MTXMODE_APPLY);
            } else {
                Matrix_Scale(2.0f, 4.0f, 2.0f, MTXMODE_APPLY);
            }

            MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
            gSPDisplayList(POLY_XLU_DISP++, gFallingRainDropDL);
            FrameInterpolation_RecordCloseChild();
        }
    }

    phi_s5 = false;
    if (player->actor.floorHeight < play->view.eye.y) {
        for (i = 0; i < end; i++) {
            if (!phi_s5) {
                Gfx_SetupDL25_Xlu(play->state.gfxCtx);

                gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
                gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 100);
                phi_s5++;
            }

            temp_f20 = this->unk_14C[1].unk_00 + (func_808DDE74() * 220.0f);
            temp_f22 = player->actor.floorHeight + 2.0f;
            temp_f2 = this->unk_14C[1].unk_08 + (func_808DDE74() * 220.0f);

            if (!((temp_f20 < -252.0f) && (temp_f20 > -500.0f) && (temp_f2 > 3820.0f) && (temp_f2 < 4150.0f))) {
                FrameInterpolation_RecordOpenChild(this, i + end);
                Matrix_Translate(temp_f20, temp_f22, temp_f2, MTXMODE_NEW);
                temp_f12 = (Rand_ZeroOne() * 0.05f) + 0.05f;
                Matrix_Scale(temp_f12, temp_f12, temp_f12, MTXMODE_APPLY);

                MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
                gSPDisplayList(POLY_XLU_DISP++, gEffShockwaveDL);
                FrameInterpolation_RecordCloseChild();
            }
        }
    }

    CLOSE_DISPS(play->state.gfxCtx);
}
