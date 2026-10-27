#ifndef SOH_NATIVE_NPC_MODELS_H
#define SOH_NATIVE_NPC_MODELS_H

#include "soh/ResourceManagerHelpers.h"
#include "align_asset_macro.h"
#include "NativeNpcPose.h"

/* Optional paired private rigs follow the existing Young Fado NPC contract.
 * Base and Alt use native joint order/count, so SkeletonPatcher can switch an
 * already loaded actor without replacing animation, tracking or story logic. */
#define SARIA_NPC_NS "__OTR__objects/object_sa/NativeNpcV1"
#define KOKIRI_GIRL_NPC_NS "__OTR__objects/object_kw1/NativeNpcV1"
#define CHILD_MALON_NPC_NS "__OTR__objects/object_ma1/NativeNpcV1"
#define ADULT_MALON_NPC_NS "__OTR__objects/object_ma2/NativeNpcV1"
#define NPC_FACE_ROW(ns, left, right)                                                                                  \
    {                                                                                                                  \
        ns "/Faces/L" #left "R" #right "M0", ns "/Faces/L" #left "R" #right "M1", ns "/Faces/L" #left "R" #right "M2", \
            ns "/Faces/L" #left "R" #right "M3", ns "/Faces/L" #left "R" #right "M4"                                   \
    }
#define NPC_FACE_RIGHT(ns, left)                                                                                    \
    {                                                                                                               \
        NPC_FACE_ROW(ns, left, 0), NPC_FACE_ROW(ns, left, 1), NPC_FACE_ROW(ns, left, 2), NPC_FACE_ROW(ns, left, 3), \
            NPC_FACE_ROW(ns, left, 4)                                                                               \
    }
#define NPC_FACES(ns)                                                                               \
    {                                                                                               \
        NPC_FACE_RIGHT(ns, 0), NPC_FACE_RIGHT(ns, 1), NPC_FACE_RIGHT(ns, 2), NPC_FACE_RIGHT(ns, 3), \
            NPC_FACE_RIGHT(ns, 4)                                                                   \
    }

static const ALIGN_ASSET(2) char sSariaNpcSkel[] = SARIA_NPC_NS "/Skel";
static const ALIGN_ASSET(2) char sSariaNpcHead[] = SARIA_NPC_NS "/Body/Limb15";
static const ALIGN_ASSET(2) char sSariaNpcFaces[5][5][5][64] = NPC_FACES(SARIA_NPC_NS);
static const ALIGN_ASSET(2) char sSariaNpcOcarina[] = SARIA_NPC_NS "/Props/RightHandOcarina";
static const ALIGN_ASSET(2) char sKokiriGirlNpcSkel[] = KOKIRI_GIRL_NPC_NS "/Skel";
static const ALIGN_ASSET(2) char sKokiriGirlNpcHead[] = KOKIRI_GIRL_NPC_NS "/Body/Limb14";
static const ALIGN_ASSET(2) char sKokiriGirlNpcFaces[5][5][5][64] = NPC_FACES(KOKIRI_GIRL_NPC_NS);
static const ALIGN_ASSET(2) char sChildMalonNpcSkel[] = CHILD_MALON_NPC_NS "/Skel";
static const ALIGN_ASSET(2) char sChildMalonNpcHead[] = CHILD_MALON_NPC_NS "/Body/Limb14";
static const ALIGN_ASSET(2) char sChildMalonNpcFaces[5][5][5][64] = NPC_FACES(CHILD_MALON_NPC_NS);
static const ALIGN_ASSET(2) char sAdultMalonNpcSkel[] = ADULT_MALON_NPC_NS "/Skel";
static const ALIGN_ASSET(2) char sAdultMalonNpcHead[] = ADULT_MALON_NPC_NS "/Body/Limb17";
static const ALIGN_ASSET(2) char sAdultMalonNpcFaces[5][5][5][64] = NPC_FACES(ADULT_MALON_NPC_NS);

typedef struct {
    const char* skeleton;
    const char* head;
    const char (*faces)[5][5][64];
} NativeNpcModel;

static const NativeNpcModel sSariaNpcModel = { sSariaNpcSkel, sSariaNpcHead, sSariaNpcFaces };
static const NativeNpcModel sKokiriGirlNpcModel = { sKokiriGirlNpcSkel, sKokiriGirlNpcHead, sKokiriGirlNpcFaces };
static const NativeNpcModel sChildMalonNpcModel = { sChildMalonNpcSkel, sChildMalonNpcHead, sChildMalonNpcFaces };
static const NativeNpcModel sAdultMalonNpcModel = { sAdultMalonNpcSkel, sAdultMalonNpcHead, sAdultMalonNpcFaces };

static inline bool NativeNpcModel_IsPresent(const NativeNpcModel* model) {
    return model != NULL && ResourceMgr_FileExists(model->skeleton) && ResourceMgr_FileAltExists(model->skeleton) &&
           ResourceMgr_FileAltExists(model->head);
}

static inline bool NativeNpcModel_IsActive(const NativeNpcModel* model) {
    return ResourceMgr_IsAltAssetsEnabled() && NativeNpcModel_IsPresent(model);
}

static inline const char* NativeNpcModel_SelectSkeleton(const NativeNpcModel* model, const char* nativeSkeleton) {
    return NativeNpcModel_IsPresent(model) ? model->skeleton : nativeSkeleton;
}

static inline const NativeNpcModel* NativeNpcModel_Kokiri(u8 type) {
    /* All ordinary Kokiri-girl placements: twins' house, forest, shop and
     * story-gated awning. Fado (12) retains her separate adapter/resources. */
    return type == 1 || type == 5 || type == 6 || type == 9 || type == 10 ? &sKokiriGirlNpcModel : NULL;
}

static inline Gfx* NativeNpcModel_SelectEyes(const NativeNpcModel* model, Gfx* nativeHead, s32 leftEye, s32 rightEye,
                                             s32 mouth) {
    if (!NativeNpcModel_IsActive(model)) {
        return nativeHead;
    }
    leftEye = leftEye >= 0 && leftEye < 5 ? leftEye : 0;
    rightEye = rightEye >= 0 && rightEye < 5 ? rightEye : 0;
    mouth = mouth >= 0 && mouth < 5 ? mouth : 0;
    const char* path = model->faces[leftEye][rightEye][mouth];
    return (Gfx*)(ResourceMgr_FileAltExists(path) ? path : model->head);
}

static inline Gfx* NativeNpcModel_SelectHead(const NativeNpcModel* model, Gfx* nativeHead, s32 eye, s32 mouth) {
    return NativeNpcModel_SelectEyes(model, nativeHead, eye, eye, mouth);
}

static inline Gfx* NativeNpcModel_SariaOcarina(Gfx* nativeHandAndOcarina) {
    return NativeNpcModel_IsActive(&sSariaNpcModel) && ResourceMgr_FileAltExists(sSariaNpcOcarina)
               ? (Gfx*)sSariaNpcOcarina
               : nativeHandAndOcarina;
}

#undef NPC_FACES
#undef NPC_FACE_RIGHT
#undef NPC_FACE_ROW
#undef SARIA_NPC_NS
#undef KOKIRI_GIRL_NPC_NS
#undef CHILD_MALON_NPC_NS
#undef ADULT_MALON_NPC_NS
#endif
