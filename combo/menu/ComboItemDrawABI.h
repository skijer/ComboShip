/* combo/menu/ComboItemDrawABI.h — ComboShip: cross-game item draw info (C ABI, POD only).
 * The owning game returns its sDrawItemTable data for one item so the OTHER game can render
 * the model via "@<game>:"-routed resource paths. Strings point into the owning game's static
 * storage (path literals from asset headers — process-lifetime). */
#ifndef COMBO_ITEM_DRAW_ABI_H
#define COMBO_ITEM_DRAW_ABI_H
#include "ComboElementalArrowGi.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "../ComboGameExport.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CW_DRAW_MAX_DLISTS 8

/* Producer return code: state the recipe needs isn't available yet (the other game is dormant /
 * not yet initialised). Consumers must RETRY next frame, never negative-cache. */
#define CW_DRAW_NOT_READY (-1)

/* ComboShip: which OOT get-item draw func the consumer must replicate. SIMPLE (0) = the plain
 * OPA-then-XLU submission the consumer already does (self-contained funcs + rupees/wallets/etc). The
 * rest are the non-portable OOT funcs (segment-8/9 texture scrolls, billboard rotation, per-instance
 * prim/env color) that draw as the sentinel today; each has a 1:1 handler in ComboForeignDrawMM.h
 * that re-binds the segment(s) in the host frame before submitting the routed @oot: DLs. When drawKind
 * != SIMPLE the consumer ignores xluStartIndex and dlists[] are carried in the OOT TABLE order (the
 * handler reorders exactly as the OOT func does). */
typedef enum {
    CW_DRAW_KIND_SIMPLE = 0,
    CW_DRAW_KIND_GORON_SWORD,    /* Biggoron/Broken sword: seg8 OPA scroll, dl0 */
    CW_DRAW_KIND_DEKU_NUTS,      /* seg8 OPA scroll, dl0 */
    CW_DRAW_KIND_RECOVERY_HEART, /* seg8 XLU scroll, dl0 */
    CW_DRAW_KIND_FISH,           /* seg8 XLU scroll, dl0 */
    CW_DRAW_KIND_POTION,         /* seg8 OPA scroll, OPA dl[1,0,2,3] + XLU dl[4,5] */
    CW_DRAW_KIND_MIRROR_SHIELD,  /* seg8 OPA scroll, OPA dl0 + XLU dl1 */
    CW_DRAW_KIND_BLUE_FIRE,      /* OPA dl0; XLU seg8 scroll + billboard flame dl1 */
    CW_DRAW_KIND_POES,           /* OPA dl0; XLU dl1; seg8 scroll; billboard dl3,dl2 */
    CW_DRAW_KIND_FAIRY,          /* OPA dl0; XLU dl1; seg8 scroll; billboard dl2 */
    CW_DRAW_KIND_JEWEL,          /* spiritual stones: seg9 XLU + seg8 OPA + rotate + per-layer colors */
    CW_DRAW_KIND_MAGIC_SPELL,    /* Din/Farore/Nayru: XLU seg8 scroll, dl0,1,2 */
    CW_DRAW_KIND_SCALE,          /* silver/gold scale: XLU seg8 scroll, dl2,3,1,0 */
    CW_DRAW_KIND_SKULL_TOKEN,    /* body OPA dl0 + XLU seg8 flame dl1 (full, flame included) */
    CW_DRAW_KIND_MUSIC_NOTE,     /* generic rando song note: grayscale tint (primColorXlu), dl0 */
    CW_DRAW_KIND_BOSS_SOUL, /* OOT boss soul: seg8 flame scroll (grayscale primColorXlu) dl0 + skull dl1 (envColorXlu)
                             */
    /* Triforce Piece / Fishing Pole are plain scaled-OPA draws with no extra GPU state, so they ride
     * the SIMPLE path (dlists[0] + scale) rather than a dedicated kind. */

    /* ComboShip: the bespoke Randomizer_Draw* funcs (Item::SetCustomDrawFunc), which the gid-keyed
     * sDrawItemTable is blind to. Appended so the values above keep their ABI numbers. */
    CW_DRAW_KIND_COLOR_LAYERS,   /* per-DL prim/env color then dlists[i]; xluStartIndex splits OPA/XLU */
    CW_DRAW_KIND_GRAYSCALE_XLU,  /* XLU: grayscale tint (primColorXlu) around dl0 */
    CW_DRAW_KIND_DOUBLE_DEFENSE, /* XLU: grayscale-white heart border dl0, then plain container dl1 */
    CW_DRAW_KIND_MASTER_SWORD,   /* seg8 OPA scroll + scale 0.05 + rotate Z 2.1, dl0 */
    CW_DRAW_KIND_BRONZE_SCALE,   /* XLU seg8 scroll + constant prim/env pairs around scale dl0 / water dl1 */

    /* ComboShip: MM->OOT additions. The kinds above are OOT funcs replicated by MM; these are MM
     * funcs replicated by OOT. Where an MM func is byte-for-byte the same as its OOT twin
     * (DekuNuts/RecoveryHeart/Fish/Potion/Poes/GoronSword) the kind above is reused instead. */
    CW_DRAW_KIND_MM_FAIRY_BOTTLE,           /* OPA dl0; XLU dl1; seg8 scroll (32x320 layer 2); billboard dl2 */
    CW_DRAW_KIND_MM_SOUL_FLAME,             /* MM enemy soul: billboard seg8 flame (primColorXlu), dl0 */
    CW_DRAW_KIND_OPS,                       /* ops[] bytecode below (transforms/colors/DLs); see CwDrawOpCode */
    CW_DRAW_KIND_NEI_CANE,                  /* concrete legacy cane skill rendered by the MM host */
    CW_DRAW_KIND_NEI_GI,                    /* replacement mesh plus host-rendered elemental energy */
    CW_DRAW_KIND_MM_MASK,                   /* imported MM mask: native MM resources and palette-free split passes */
    CW_DRAW_KIND_MM_REMAINS,                /* imported boss remains: native MM OPA resource, scale 0.02 */
    CW_DRAW_KIND_MM_SPIN_ATTACK,            /* MM Great Spin: native disk/cylinder, seg8 scroll, live burst color */
    CW_DRAW_KIND_OOT_MORPHA_SOUL = 32,      /* native MM flame dl0 + OoT Morpha membrane/nucleus dl1/2 */
    CW_DRAW_KIND_MAGIC_JAR = 33,            /* OoT magic jar: custom Alt grayscale tint carried in primColorOpa */
    CW_DRAW_KIND_CUSTOM_GI = 31,            /* OoT custom: spin, ops transforms, scale, OPA/XLU; primColorOpa grayscale,
                                         primColorXlu weapon flame; alpha zero disables each independent tint/effect. */
    CW_DRAW_KIND_SEASON_GI = 36,            /* 1..4: seasons; 5: rod + cycling weather; 6: song rain */
    CW_DRAW_KIND_GRAYSCALE_LAYERS = 35,     /* per-DL grayscale layerPrimMask/colors; OPA/XLU split */
    CW_DRAW_KIND_OOT_NATIVE_EQUIPMENT = 34, /* concrete static award; ops[0] native equipment selector */
    CW_DRAW_KIND_SONG_GI = 38,              /* note plus song profile in neiEffect; native env or grayscale tint */
    CW_DRAW_KIND_MM_FAIRY_CONTAINER = 37,   /* OPA shell0, XLU glass1/contents2; dl3 is its native Mtx path */
    CW_DRAW_KIND_ELEMENTAL_ARROW = 39,      /* owner arrow core dl0; host aura profile 1..3 in neiEffect */
    CW_DRAW_KIND_BOTTLE_CONTENTS = 40,      /* shared casing; dl0 marker selects mushroom/princess/gold dust */
    CW_DRAW_KIND_SILVER_RUPEE = 41,         /* OPA seg8 = segTexPath (owner RM), dl0; optional grayscale */
    CW_DRAW_KIND_MAGIC_MIRROR = 42,         /* receiving host draws authored procedural Mirror + leaves */
} CwDrawKind;

#define CW_DRAW_MAX_OPS 20

/* ComboShip: draw bytecode for funcs shaped as per-DL transforms/colors rather than one flat
 * OPA/XLU submission (MM's DrawClock, DrawOwlStatue, ...). See docs/deviations/rando.md. */
typedef enum {
    CW_OP_END = 0,
    CW_OP_SETUP_OPA,   /* Gfx_SetupDL_25Opa; later ops target the OPA stream */
    CW_OP_SETUP_XLU,   /* Gfx_SetupDL_25Xlu; later ops target the XLU stream */
    CW_OP_LOAD_MATRIX, /* load the current model matrix into the active stream */
    CW_OP_PUSH,        /* Matrix_Push */
    CW_OP_POP,         /* Matrix_Pop */
    CW_OP_TRANSLATE,   /* a,b,c = x,y,z (MTXMODE_APPLY) */
    CW_OP_SCALE,       /* a,b,c = x,y,z (MTXMODE_APPLY) */
    CW_OP_ROTATE_X,    /* a = s16 binang */
    CW_OP_ROTATE_Y,
    CW_OP_ROTATE_Z,
    CW_OP_BILLBOARD,       /* Matrix_ReplaceRotation(billboardMtxF) */
    CW_OP_PRIM_COLOR,      /* rgba; a = lodFrac */
    CW_OP_ENV_COLOR,       /* rgba */
    CW_OP_GRAYSCALE_COLOR, /* rgba */
    CW_OP_GRAYSCALE_ON,
    CW_OP_GRAYSCALE_OFF,
    CW_OP_DLIST,            /* a = index into dlists[] */
    CW_OP_FRAME_PAIR,       /* only CUSTOM_GI: choose one of two OPA DLs; a = frame bit (0..30) */
    CW_OP_NATIVE_EQUIPMENT, /* only OOT_NATIVE_EQUIPMENT: a = CwOotNativeEquipment */
    CW_OP_NO_CULL,          /* only CUSTOM_GI: draw a legacy single-sided plate from either side */
} CwDrawOpCode;

// Existing MM-native renderers for inline/palette-remapped OoT equipment. These concrete
// selectors carry no progression logic, item grants or gameplay pointers across the ABI.
typedef enum {
    CW_OOT_EQUIP_AXE = 1,
    CW_OOT_EQUIP_SPIRIT_TUNIC,
    CW_OOT_EQUIP_CHAMPIONS_TUNIC,
    CW_OOT_EQUIP_SAGES_TUNIC,
    CW_OOT_EQUIP_PEGASUS_BOOTS,
    CW_OOT_EQUIP_TRIDENT,
    CW_OOT_EQUIP_CLIMB_BOOTS,
    CW_OOT_EQUIP_ROC_BOOTS,
} CwOotNativeEquipment;

typedef struct {
    int32_t op; /* CwDrawOpCode */
    float a, b, c;
    uint8_t rgba[4];
} CwDrawOp;

typedef struct {
    const char* dlists[CW_DRAW_MAX_DLISTS]; /* OTR path strings, in SUBMISSION order */
    int32_t dlistCount;
    int32_t xluStartIndex;    /* dlists[0..xluStart-1] are OPA layers, rest XLU; -1 = all OPA */
    float scale;              /* extra model scale; 0 = none (e.g. MM boss remains: 0.02f) */
    int32_t hasEnvColor;      /* 1 = emit envColor before the DLs (e.g. MM song notes) */
    uint8_t envColor[4];      /* RGBA */
    int32_t xluSeg8TexScroll; /* 1 = bind seg 8 to the animated flame texscroll before the XLU layer
                                 (skulltula token); the consumer replays the owner's Gfx_TwoTexScrollEx */
    /* Resource-driven animated material (generalizes xluSeg8TexScroll; MM's Moon's Tear). The
     * consumer loads it from the owning game's RM and binds the segment — ComboForeignTexAnim_Run. */
    const char* matAnimPath;  /* owning game's own (unrouted) TextureAnimation path, or NULL */
    int32_t matAnimBindOpa;   /* 1 = also bind the animated segment on the OPA layer (item body samples it) */
    int32_t matAnimBillboard; /* 1 = Matrix_ReplaceRotation(billboardMtxF) before the XLU layer (glow) */

    /* Kind-tagged recipe for the non-portable funcs: drawKind picks the consumer handler; when
     * != SIMPLE, dlists[] is the raw table row and xluStartIndex is ignored. Colors are RGBA. */
    int32_t drawKind;        /* CwDrawKind */
    uint8_t primColorXlu[4]; /* JEWEL gem (XLU) prim; MUSIC_NOTE grayscale tint */
    uint8_t envColorXlu[4];  /* JEWEL gem (XLU) env */
    uint8_t primColorOpa[4]; /* JEWEL setting (OPA) prim */
    uint8_t envColorOpa[4];  /* JEWEL setting (OPA) env */

    /* 1 = live save-state model (progressive tier, Triforce shard, junk/trap); freezes on grant.
     * 2 = live appearance (dungeon palette/Alt selection); remains live after grant.
     * Both values require re-querying instead of caching. No struct layout change. */
    int32_t stateDependent;

    /* ComboShip: CW_DRAW_KIND_COLOR_LAYERS payload — the per-DL prim/env colors the rando key/map/
     * compass/jabber-nut funcs set before each display list. Bit i of each mask = dlists[i] sets it. */
    uint8_t layerPrimColor[CW_DRAW_MAX_DLISTS][4];
    uint8_t layerEnvColor[CW_DRAW_MAX_DLISTS][4];
    int32_t layerPrimMask;
    int32_t layerEnvMask;

    /* ComboShip: CW_DRAW_KIND_OPS payload. */
    int32_t opCount;
    CwDrawOp ops[CW_DRAW_MAX_OPS];

    /* ComboShip: the owner's setup DL for each stream (raw Gfx* in the owner's module — state-only
     * commands, no resource refs), or NULL for the consumer's own 25 Opa/Xlu. Items authored for a
     * different setup (OOT's 26 = 1-cycle, no fog) render through the wrong combiner under 25: the
     * dead second cycle wins and samples TEXEL1, i.e. whatever tile the HOST last left bound. */
    const void* setupDlOpa;
    const void* setupDlXlu;

    /* Resolved tier name when a progressive placeholder converted (e.g. "Large Quiver"), or NULL. */
    const char* resolvedName;

    /* NEI presentation metadata; no owning-game pointers to gameplay state. */
    int32_t neiEffect;
    float neiEffectCenter[3];
    int32_t neiSomariaUpgrade;
    int32_t neiLegacyCane; /* 1=Statue, 2=Flip, 3=Block, 4=Stone, 5=Platform, 6=Ultrahand */
    /* Independent effect pass; never tint the mask model or change its material routing. */
    int32_t itemShimmer;
    uint8_t itemShimmerColor[4];
    /* Optional model-independent NEI sampler: Kind + 1, or zero for the color overlay.
     * Carries shimmer only; never apply authored mesh-local energy to a mod model. */
    int32_t neiShimmer;
    /* ComboShip: owner's own (unrouted) texture path for seg 8; can't ride dlists[], which get routed. */
    const char* segTexPath;
} CwItemDrawInfo;

typedef struct {
    const char* path;      /* owning archive's process-lifetime texture path */
    int32_t width, height; /* logical pixels, independent of high-resolution replacements */
    int32_t isIA8;         /* zero = RGBA32 */
    int32_t hasColor;      /* append-only; zero preserves the default white */
    uint8_t color[4];
} CwItemIconInfo;
// Resource-only editor sampling for foreign GI presentation. Host frames advance only
// the sampling clock; no dormant native frame, game state or cosmetic patch loop runs.
COMBO_OOT_EXPORT void OOT_SetGiCosmeticFrame(uint32_t hostFrame);
COMBO_OOT_EXPORT void OOT_SampleGiCosmeticColor(const char* valueCvar, uint8_t fallbackR, uint8_t fallbackG,
                                                uint8_t fallbackB, uint8_t* outRGB);
COMBO_MM_EXPORT void MM_SetGiCosmeticFrame(uint32_t hostFrame);
COMBO_MM_EXPORT void MM_SampleGiCosmeticColor(const char* valueCvar, uint8_t fallbackR, uint8_t fallbackG,
                                              uint8_t fallbackB, uint8_t* outRGB);
typedef void (*Fn_SetGiCosmeticFrame)(uint32_t hostFrame);

typedef int32_t (*Fn_GetItemIconInfo)(const char* itemName, CwItemIconInfo* out);
// Uses the registered asset owner and its Alt mode; no engine state crosses.
typedef int32_t (*Fn_NeiResourceExists)(const char* path);
COMBO_OOT_EXPORT int32_t OOT_NeiResourceExists(const char* path);
COMBO_OOT_EXPORT int32_t OOT_NeiAltAssetsEnabled(void);
COMBO_OOT_EXPORT int32_t OOT_NeiEnsureGiBaseOwner(void);
COMBO_OOT_EXPORT int32_t OOT_GetNeiGiDrawInfo(const char* slug, CwItemDrawInfo* out);
typedef int32_t (*Fn_GetNeiGiDrawInfo)(const char* slug, CwItemDrawInfo* out);
// The active host supplies its sword asset mode without changing the donor's
// resource manager or per-game setting. Zero selects the authored sword GI.
COMBO_OOT_EXPORT int32_t OOT_GetNeiGiDrawInfoForAssets(const char* slug, int32_t altAssets, CwItemDrawInfo* out);
typedef int32_t (*Fn_GetNeiGiDrawInfoForAssets)(const char* slug, int32_t altAssets, CwItemDrawInfo* out);

/* Returns 1 and fills out on success; 0 if the item is unknown/undrawable; CW_DRAW_NOT_READY if the
 * producer's state isn't up yet. itemName is in the owning game's namespace (MM: RI_* spoilerName). */
typedef int32_t (*Fn_GetItemDrawInfo)(const char* itemName, CwItemDrawInfo* out);

static inline int32_t CwDescribeMagicMirror(CwItemDrawInfo* out) {
    if (out == NULL)
        return 0;
    memset(out, 0, sizeof(*out));
    out->drawKind = CW_DRAW_KIND_MAGIC_MIRROR;
    out->dlistCount = 0;
    out->opCount = 0;
    out->scale = 0.0f;    /* native callback owns its receiving host's existing GI scale */
    out->itemShimmer = 0; /* the authored callback supplies Saria leaves */
    out->resolvedName = "Magic Mirror";
    return 1;
}

/* Lowest dlists[] slot index each kind's handler blind-indexes, +1. Both resolvers reject a recipe
 * below this before caching it, so a handler can never submit a null display list. */
static inline int32_t CwMinDlistsForKind(int32_t kind) {
    switch (kind) {
        case CW_DRAW_KIND_POTION:
            return 6;
        case CW_DRAW_KIND_POES:
        case CW_DRAW_KIND_SCALE:
        case CW_DRAW_KIND_MM_FAIRY_CONTAINER:
            return 4;
        case CW_DRAW_KIND_FAIRY:
        case CW_DRAW_KIND_MAGIC_SPELL:
        case CW_DRAW_KIND_MM_FAIRY_BOTTLE:
        case CW_DRAW_KIND_OOT_MORPHA_SOUL:
            return 3;
        case CW_DRAW_KIND_MIRROR_SHIELD:
        case CW_DRAW_KIND_BLUE_FIRE:
        case CW_DRAW_KIND_JEWEL:
        case CW_DRAW_KIND_SKULL_TOKEN:
        case CW_DRAW_KIND_BOSS_SOUL:
        case CW_DRAW_KIND_DOUBLE_DEFENSE:
        case CW_DRAW_KIND_BRONZE_SCALE:
        case CW_DRAW_KIND_MM_MASK:
        case CW_DRAW_KIND_MM_SPIN_ATTACK:
            return 2;
        case CW_DRAW_KIND_OPS:
        case CW_DRAW_KIND_NEI_CANE:
        case CW_DRAW_KIND_MAGIC_MIRROR:
        case CW_DRAW_KIND_OOT_NATIVE_EQUIPMENT:
            return 0; /* native kinds carry no resource lists; OPS bounds-checks every DL index */
        case CW_DRAW_KIND_SEASON_GI:
            return 0; /* weather-only profiles; resolvers separately require a model for profile 5 */
        case CW_DRAW_KIND_SILVER_RUPEE:
            return 1;
        default:
            return 1;
    }
}

/* ComboShip: one segment bind a foreign animated draw needs in the host frame. Every bind is
 * restored to an empty DL after the draw (segment hygiene); a bind we cannot express means the
 * whole recipe is rejected rather than submitting a DL against an unbound segment. */
typedef enum {
    CW_ANIM_SEG_EMPTY_DL = 0,      /* host-allocated gsSPEndDisplayList (MM D_801AEFA0 / OOT GetEmptyDlist) */
    CW_ANIM_SEG_PATH,              /* the owning game's own "__OTR__..." texture path (resolved under the RM bracket) */
    CW_ANIM_SEG_TEXSCROLL,         /* Gfx_TwoTexScrollEx / Gfx_TexScrollEx built from the params below */
    CW_ANIM_SEG_XLU_RENDERMODE_1CY /* MM Scene_SetRenderModeXlu index 1 (single-cycle XLU render mode DL) */
} CwAnimSegKind;

typedef struct {
    int32_t kind;     /* CwAnimSegKind */
    int32_t segment;  /* 8..13 */
    int32_t onOpa;    /* bind on the OPA stream */
    int32_t onXlu;    /* bind on the XLU stream */
    const char* path; /* CW_ANIM_SEG_PATH only */
    /* CW_ANIM_SEG_TEXSCROLL: each layer's offset is (base + step * frames) & mask (mask 0 = none). */
    int32_t singleLayer; /* 1 = Gfx_TexScrollEx (layer 1 only) */
    int32_t xBase1, yBase1, xStep1, yStep1, xMask1, yMask1, width1, height1;
    int32_t xBase2, yBase2, xStep2, yStep2, xMask2, yMask2, width2, height2;
} CwAnimSegBind;

/* Per-limb env colour override (OverrideLimbDraw* in the real funcs). Ranges are inclusive and
 * applied in order, so a broad default can be followed by narrower overrides (OOT's Gohma). */
typedef struct {
    int32_t limbFrom, limbTo;
    uint8_t env[4];
} CwAnimLimbColor;

/* Per-limb display-list surgery: hide a limb, swap its DL, nudge its position, and/or submit an
 * extra DL in the post-limb pass (OOT Twinrova's ice hair, MM minifrog's billboarded eyes). */
typedef struct {
    int32_t limbIndex;
    int32_t hide;       /* 1 = *dList = NULL */
    const char* dlPath; /* replace this limb's DL with the owning game's path, or NULL */
    float posDx, posDy, posDz;
    int32_t postSelf;       /* 1 = re-submit the limb's ORIGINAL DL in the post-limb pass */
    const char* postDlPath; /* extra DL in the post-limb pass, or NULL */
    int32_t postBillboard;  /* 1 = Matrix_ReplaceRotation before the post-limb submission */
    int32_t postXlu;        /* 1 = post-limb submission goes on the XLU stream (else OPA) */
} CwAnimLimbDL;

#define CW_ANIM_MAX_SEGS 6
#define CW_ANIM_MAX_LIMB_COLORS 6
#define CW_ANIM_MAX_LIMB_DLS 4

typedef enum {
    CW_ANIM_PROFILE_NONE = 0,
    CW_ANIM_PROFILE_OOT_BARINADE,
    CW_ANIM_PROFILE_MM_TWINMOLD,
} CwAnimProceduralProfile;

/* ComboShip: animated variant — the owner describes a skeletal item and ComboForeignAnim.h loads it
 * via the owner's RM and drives the host's SkelAnime. APPEND-ONLY (POD C ABI, two DLLs). */
typedef struct {
    const char* skelPath;    /* FlexSkeleton resource (OTR path) */
    const char* animPath;    /* Animation resource */
    const char* texAnimPath; /* AnimatedMaterial resource, or NULL */
    float scale;             /* model scale (stray fairy: 0.03f) */
    int32_t billboard;       /* 1 = face camera (Matrix_ReplaceRotation on billboard mtx) */
    int32_t xlu;             /* 1 = draw on XLU layer with 25Xlu setup */
    int32_t limbCount;       /* skeleton limb count (jointTable sizing) */
    int32_t hiddenLimb;      /* limb index to null out (stray fairy: right-facing head), -1 none */

    /* --- ComboShip appended: the OPA/skeletal class (OOT boss souls, MM enemy souls + minifrogs).
     * The fields above alone could only express an XLU flex-skeleton item with one hidden limb. */
    int32_t opa;             /* 1 = 25Opa setup + SkelAnime_Draw*Opa (mutually exclusive with xlu) */
    int32_t nonFlexSkeleton; /* 1 = SkeletonHeader + SkelAnime_Init (0 = FlexSkeletonHeader/InitFlex) */
    float playSpeed;         /* animation speed; 0 = leave at the engine default (MM Leever: -1) */
    float translatePre[3];   /* Matrix_Translate BEFORE the scale (OOT boss souls: world units) */
    float translatePost[3];  /* Matrix_Translate AFTER the scale (MM enemy souls: model units) */
    int32_t hasPrimColor;
    uint8_t primColor[4];
    int32_t primLodFrac;
    int32_t hasModelEnvColor; /* env colour emitted once before the skeleton */
    uint8_t modelEnvColor[4];
    int32_t segCount;
    CwAnimSegBind segs[CW_ANIM_MAX_SEGS];
    int32_t limbColorCount;
    CwAnimLimbColor limbColors[CW_ANIM_MAX_LIMB_COLORS];
    int32_t limbDLCount;
    CwAnimLimbDL limbDLs[CW_ANIM_MAX_LIMB_DLS];
    /* Secondary composite pass: the soul flame around the model (OOT Randomizer_DrawBossSoul's
     * grayscale-tinted blue fire, MM DrawEnLight's prim/env-coloured flame). NULL = no flame. */
    const char* flameDlPath;
    int32_t flameAfter;          /* 1 = after the model, inheriting its transform (MM); 0 = before (OOT) */
    int32_t flameGrayscale;      /* 1 = gDPSetGrayscaleColor + gSPGrayscale (OOT); 0 = prim+env (MM) */
    int32_t flameBillboardFirst; /* 1 = billboard before translate/scale (MM DrawEnLight) */
    uint8_t flameColor[4];
    float flameTranslate[3];
    float flameScale[3];
    int32_t flameHasSeg;
    CwAnimSegBind flameSeg;
    /* 1 = recipe depends on live state (e.g. OOT's SimplerBossSoulModels CVar) — do not cache. */
    int32_t stateDependent;
    /* Native procedural limb surgery; appended without changing existing record offsets. */
    int32_t proceduralProfile;        /* CwAnimProceduralProfile */
    const char* proceduralDlPaths[2]; /* Barinade: limb-25 ring and limbs-10..19 electric XLU DLs */
    int32_t freezeLastFrame;          /* 1 = initialize the selected animation at its last frame, once mode */
} CwItemAnimDrawInfo;
typedef int32_t (*Fn_GetItemAnimDrawInfo)(const char* itemName, CwItemAnimDrawInfo* out);

#ifdef __cplusplus
}
#endif
#endif /* COMBO_ITEM_DRAW_ABI_H */
