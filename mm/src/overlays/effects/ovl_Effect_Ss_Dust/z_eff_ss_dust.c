/*
 * File: z_eff_ss_dust.c
 * Overlay: ovl_Effect_Ss_Dust
 * Description: Dust Particle Effect
 */

#include "z_eff_ss_dust.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include "2s2h/BenPort.h"
#include "2s2h/BenGui/CosmeticEditor.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <libultraship/bridge/resourcebridge.h>

#define rPrimColorR regs[0]
#define rPrimColorG regs[1]
#define rPrimColorB regs[2]
#define rPrimColorA regs[3]
#define rEnvColorR regs[4]
#define rEnvColorG regs[5]
#define rEnvColorB regs[6]
#define rEnvColorA regs[7]
#define rTexIndex regs[8] // this reg is also used to set specific colors in the fire update function
#define rScale regs[9]
#define rScaleStep regs[10]
#define rDrawFlags regs[11]
#define rLifespan regs[12]

#define PARAMS ((EffectSsDustInitParams*)initParamsx)

u32 EffectSsDust_Init(PlayState* play, u32 index, EffectSs* this, void* initParamsx);
void EffectSsDust_Update(PlayState* play, u32 index, EffectSs* this);
void EffectSsDust_UpdateFire(PlayState* play, u32 index, EffectSs* this);
void EffectSsDust_Draw(PlayState* play, u32 index, EffectSs* this);

EffectSsProfile Effect_Ss_Dust_Profile = {
    EFFECT_SS_DUST,
    EffectSsDust_Init,
};

static EffectSsUpdateFunc sUpdateFuncs[] = {
    EffectSsDust_Update,
    EffectSsDust_UpdateFire,
};

static TexturePtr sDustTextures[] = {
    gEffDust1Tex, gEffDust2Tex, gEffDust3Tex, gEffDust4Tex, gEffDust5Tex, gEffDust6Tex, gEffDust7Tex, gEffDust8Tex,
};

u32 EffectSsDust_Init(PlayState* play, u32 index, EffectSs* this, void* initParamsx) {
    EffectSsDustInitParams* initParams = PARAMS;

    Math_Vec3f_Copy(&this->pos, &initParams->pos);
    Math_Vec3f_Copy(&this->velocity, &initParams->velocity);
    Math_Vec3f_Copy(&this->accel, &initParams->accel);
    this->gfx = gEffDustDL;
    this->life = initParams->life;
    this->update = sUpdateFuncs[initParams->updateMode];
    this->draw = EffectSsDust_Draw;

    if (initParams->drawFlags & DUST_DRAWFLAG_RAND_COLOR_OFFSET) {
        s32 randColorOffset = Rand_ZeroOne() * 20.0f - 10.0f;

        this->rPrimColorR = initParams->primColor.r + randColorOffset;
        this->rPrimColorG = initParams->primColor.g + randColorOffset;
        this->rPrimColorB = initParams->primColor.b + randColorOffset;
        this->rEnvColorR = initParams->envColor.r + randColorOffset;
        this->rEnvColorG = initParams->envColor.g + randColorOffset;
        this->rEnvColorB = initParams->envColor.b + randColorOffset;
    } else {
        this->rPrimColorR = initParams->primColor.r;
        this->rPrimColorG = initParams->primColor.g;
        this->rPrimColorB = initParams->primColor.b;
        this->rEnvColorR = initParams->envColor.r;
        this->rEnvColorG = initParams->envColor.g;
        this->rEnvColorB = initParams->envColor.b;
    }

    this->rPrimColorA = initParams->primColor.a;
    this->rEnvColorA = initParams->envColor.a;
    this->rTexIndex = 0;
    this->rScale = initParams->scale;
    this->rScaleStep = initParams->scaleStep;
    this->rLifespan = initParams->life;
    this->rDrawFlags = initParams->drawFlags;

    return 1;
}

// The eight frames are an optional set, resolved per draw so existing particles
// follow Alt toggles and archive changes without retaining a resource pointer.
static const void* EffectSsDust_SelectTexture(u16 drawFlags, s16 frame, const void* fallback) {
    static const char ALIGN_ASSET(2) forest1[] = "__OTR__custom/medallion_magic/spells/forest/dust1Tex";
    static const char ALIGN_ASSET(2) forest2[] = "__OTR__custom/medallion_magic/spells/forest/dust2Tex";
    static const char ALIGN_ASSET(2) forest3[] = "__OTR__custom/medallion_magic/spells/forest/dust3Tex";
    static const char ALIGN_ASSET(2) forest4[] = "__OTR__custom/medallion_magic/spells/forest/dust4Tex";
    static const char ALIGN_ASSET(2) forest5[] = "__OTR__custom/medallion_magic/spells/forest/dust5Tex";
    static const char ALIGN_ASSET(2) forest6[] = "__OTR__custom/medallion_magic/spells/forest/dust6Tex";
    static const char ALIGN_ASSET(2) forest7[] = "__OTR__custom/medallion_magic/spells/forest/dust7Tex";
    static const char ALIGN_ASSET(2) forest8[] = "__OTR__custom/medallion_magic/spells/forest/dust8Tex";
    static const char* forestTextures[] = { forest1, forest2, forest3, forest4, forest5, forest6, forest7, forest8 };
    if (!(drawFlags & EFFECT_SS_DUST_DRAW_SW97_FOREST) || !ResourceMgr_IsAltAssetsEnabled()) {
        return fallback;
    }
    for (s32 i = 0; i < ARRAY_COUNT(forestTextures); ++i) {
        if (!ResourceMgr_FileAltExists(forestTextures[i]) || ResourceGetDataByName(forestTextures[i]) == NULL) {
            return fallback;
        }
    }
    return forestTextures[frame];
}

void EffectSsDust_Draw(PlayState* play, u32 index, EffectSs* this) {
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    MtxF mfTrans;
    MtxF mfScale;
    MtxF mfResult;
    MtxF mfTransBillboard;
    s32 pad;
    Mtx* mtx;
    f32 scale;

    OPEN_DISPS(gfxCtx);

    scale = this->rScale * 0.0025f;
    SkinMatrix_SetTranslate(&mfTrans, this->pos.x, this->pos.y, this->pos.z);
    SkinMatrix_SetScale(&mfScale, scale, scale, 1.0f);
    SkinMatrix_MtxFMtxFMult(&mfTrans, &play->billboardMtxF, &mfTransBillboard);
    SkinMatrix_MtxFMtxFMult(&mfTransBillboard, &mfScale, &mfResult);
    gSPMatrix(POLY_XLU_DISP++, &gIdentityMtx, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

    mtx = SkinMatrix_MtxFToNewMtx(gfxCtx, &mfResult);

    if (mtx != NULL) {
        gSPMatrix(POLY_XLU_DISP++, mtx, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gDPPipeSync(POLY_XLU_DISP++);
        gSPSegment(POLY_XLU_DISP++, 0x08,
                   Lib_SegmentedToVirtual((void*)EffectSsDust_SelectTexture(this->rDrawFlags, this->rTexIndex,
                                                                            sDustTextures[this->rTexIndex])));
        POLY_XLU_DISP = Gfx_SetupDL(POLY_XLU_DISP, SETUPDL_0);
        gDPPipeSync(POLY_XLU_DISP++);

        if (this->rDrawFlags & DUST_DRAWFLAG1) {
            gDPSetCombineLERP(POLY_XLU_DISP++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, PRIMITIVE, 0, TEXEL0, 0,
                              COMBINED, 0, SHADE, 0, 0, 0, 0, COMBINED);
            gDPSetRenderMode(POLY_XLU_DISP++, G_RM_FOG_SHADE_A, G_RM_ZB_CLD_SURF2);
            gSPSetGeometryMode(POLY_XLU_DISP++, G_FOG | G_LIGHTING);
        } else if (this->rDrawFlags & DUST_DRAWFLAG2) {
            gDPSetRenderMode(POLY_XLU_DISP++, G_RM_PASS, G_RM_ZB_CLD_SURF2);
            gSPClearGeometryMode(POLY_XLU_DISP++, G_FOG | G_LIGHTING);
        } else { // DUST_DRAWFLAG0
            gSPClearGeometryMode(POLY_XLU_DISP++, G_LIGHTING);
        }

        Color_RGBA8 primary = { this->rPrimColorR, this->rPrimColorG, this->rPrimColorB, 255 };
        Color_RGBA8 secondary = { this->rEnvColorR, this->rEnvColorG, this->rEnvColorB, this->rEnvColorA };
        if (this->rDrawFlags & EFFECT_SS_DUST_DRAW_SW97_FOREST) {
            primary = CosmeticEditor_GetChangedColor(primary.r, primary.g, primary.b, primary.a,
                                                     COSMETIC_ID("Magic.MedallionForestPrimary"));
            secondary = CosmeticEditor_GetChangedColor(secondary.r, secondary.g, secondary.b, secondary.a,
                                                       COSMETIC_ID("Magic.MedallionForestSecondary"));
        }
        gDPPipeSync(POLY_XLU_DISP++);
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, primary.r, primary.g, primary.b, 255);
        gDPSetEnvColor(POLY_XLU_DISP++, secondary.r, secondary.g, secondary.b, this->rEnvColorA);
        gSPDisplayList(POLY_XLU_DISP++, this->gfx);
    }

    CLOSE_DISPS(gfxCtx);
}

void EffectSsDust_Update(PlayState* play, u32 index, EffectSs* this) {
    this->accel.x = (Rand_ZeroOne() * 0.4f) - 0.2f;
    this->accel.z = (Rand_ZeroOne() * 0.4f) - 0.2f;

    if ((this->life <= this->rLifespan) && (this->life >= (this->rLifespan - 7))) {
        if (this->rLifespan >= 5) {
            this->rTexIndex = this->rLifespan - this->life;
        } else {
            this->rTexIndex = ((this->rLifespan - this->life) * (ARRAY_COUNT(sDustTextures) / this->rLifespan));
        }
    } else {
        this->rTexIndex = ARRAY_COUNT(sDustTextures) - 1;
    }

    this->rScale += this->rScaleStep;
}

// this update mode is unused in the original game
void EffectSsDust_UpdateFire(PlayState* play, u32 index, EffectSs* this) {
    this->accel.x = (Rand_ZeroOne() * 0.4f) - 0.2f;
    this->accel.z = (Rand_ZeroOne() * 0.4f) - 0.2f;

    switch (this->rTexIndex) {
        case 0:
            this->rPrimColorR = 255;
            this->rPrimColorG = 150;
            this->rPrimColorB = 0;
            this->rEnvColorR = 150;
            this->rEnvColorG = 50;
            this->rEnvColorB = 0;
            break;
        case 1:
            this->rPrimColorR = 200;
            this->rPrimColorG = 50;
            this->rPrimColorB = 0;
            this->rEnvColorR = 100;
            this->rEnvColorG = 0;
            this->rEnvColorB = 0;
            break;
        case 2:
            this->rPrimColorR = 50;
            this->rPrimColorG = 0;
            this->rPrimColorB = 0;
            this->rEnvColorR = 0;
            this->rEnvColorG = 0;
            this->rEnvColorB = 0;
            break;
        case 3:
            this->rPrimColorR = 50;
            this->rEnvColorR = this->rPrimColorG = this->rEnvColorG = this->rPrimColorB = this->rEnvColorB = 0;
            break;
    }

    if (this->rTexIndex < (ARRAY_COUNT(sDustTextures) - 1)) {
        this->rTexIndex++;
    }

    this->rScale += this->rScaleStep;
}
