#ifndef COMBO_SPIN_ATTACK_STYLE_RESOURCES_H
#define COMBO_SPIN_ATTACK_STYLE_RESOURCES_H
#include <stddef.h>
#include "SpinAttackStyle.h"

#ifdef CW_SPIN_HOST_MM
#define CW_SPIN_ASTRAL_NAME "astral_mm"
#else
#define CW_SPIN_ASTRAL_NAME "astral_soh"
#endif

typedef struct {
    const char* charge;
    const char* disk;
    const char* cylinder;
    const char* burstTex[2];
    const char* chargeTex[2];
} CwSpinResourceRecipe;

#define CW_SPIN_RECIPE(name, level)                                                           \
    {                                                                                         \
        "objects/combo_spin/" name "/charge", "objects/combo_spin/" name "/disk_" level,      \
            "objects/combo_spin/" name "/cylinder_" level,                                    \
            { "objects/combo_spin/" name "/burst0", "objects/combo_spin/" name "/burst1" }, { \
            "objects/combo_spin/" name "/charge0", "objects/combo_spin/" name "/charge1"      \
        }                                                                                     \
    }

static inline CwSpinResourceRecipe CwSpinStyle_Resources(CwSpinStyle style, uint8_t nativeElement, int great) {
    static const CwSpinResourceRecipe recipes[7][2] = {
        { CW_SPIN_RECIPE("fire", "regular"), CW_SPIN_RECIPE("fire", "great") },
        { CW_SPIN_RECIPE("ice", "regular"), CW_SPIN_RECIPE("ice", "great") },
        { CW_SPIN_RECIPE("light", "regular"), CW_SPIN_RECIPE("light", "great") },
        { CW_SPIN_RECIPE("lightning", "regular"), CW_SPIN_RECIPE("lightning", "great") },
        { CW_SPIN_RECIPE("arcane", "regular"), CW_SPIN_RECIPE("arcane", "great") },
        { CW_SPIN_RECIPE(CW_SPIN_ASTRAL_NAME, "regular"), CW_SPIN_RECIPE(CW_SPIN_ASTRAL_NAME, "great") },
        { CW_SPIN_RECIPE("dark_energy", "regular"), CW_SPIN_RECIPE("dark_energy", "great") },
    };
    CwSpinResourceRecipe result = { NULL, NULL, NULL, { NULL, NULL }, { NULL, NULL } };
    if (style > CW_SPIN_STYLE_NATIVE && style < CW_SPIN_STYLE_COUNT)
        return recipes[style - 1][great != 0];
    if (style == CW_SPIN_STYLE_NATIVE && nativeElement >= 1 && nativeElement <= 3) {
        result = recipes[nativeElement - 1][great != 0];
        result.charge = result.disk = result.cylinder = NULL;
    }
    return result;
}

#undef CW_SPIN_RECIPE
#undef CW_SPIN_ASTRAL_NAME
#endif
