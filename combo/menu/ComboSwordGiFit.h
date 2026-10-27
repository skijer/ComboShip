#ifndef COMBO_SWORD_GI_FIT_H
#define COMBO_SWORD_GI_FIT_H
#include "ComboSwordGiEffectFit.h"

extern "C" int ResourceMgr_GetGiModelFitForGame(const char* owner, const char* path, float scale, float tilt, int shop,
                                                float fit[2]);
extern "C" int ResourceMgr_GetGiModelsFitForGame(const char* owner, const char* const* paths, int count, float scale,
                                                 float tilt, int presentation, float fit[2]);

static inline void ComboSwordGi_ApplyModelsFit(const char* owner, const char* const* paths, int count, float scale,
                                               float tilt, bool shop = false, int mmPickup = 0) {
    float fit[2] = { 1.f, 0.f };
    const int presentation = shop ? 1 : mmPickup ? 1 + mmPickup : 0;
    const bool fitted = ResourceMgr_GetGiModelsFitForGame(owner, paths, count, scale, tilt, presentation, fit) > 0;
#ifdef COMBO_GI_RECEIPT_TRACE
    ComboGiReceiptTrace::Fit(owner, paths, count, scale, tilt, presentation, fitted, fit);
#endif
    if (fitted) {
        Matrix_Translate(0.f, fit[1], 0.f, MTXMODE_APPLY);
        Matrix_Scale(fit[0], fit[0], fit[0], MTXMODE_APPLY);
    }
}

static inline void ComboSwordGi_ApplyFit(const char* owner, const char* path, float scale, float tilt,
                                         bool shop = false, int mmPickup = 0) {
    float fit[2] = { 1.f, 0.f };
    // Preserve the C integer seam: world 0, shop 1, MM receipt 2, Goron 3.
    const int presentation = shop ? 1 : mmPickup ? 1 + mmPickup : 0;
    const bool fitted = ResourceMgr_GetGiModelFitForGame(owner, path, scale, tilt, presentation, fit) > 0;
#ifdef COMBO_GI_RECEIPT_TRACE
    ComboGiReceiptTrace::Fit(owner, &path, 1, scale, tilt, presentation, fitted, fit);
#endif
    if (fitted) {
        Matrix_Translate(0.f, fit[1], 0.f, MTXMODE_APPLY);
        Matrix_Scale(fit[0], fit[0], fit[0], MTXMODE_APPLY);
    }
}

#endif
