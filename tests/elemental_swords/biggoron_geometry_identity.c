#include "global.h"
#include "../../combo/render/sword_element_aura_data.h"
#include "../../combo/render/sword_element_aura_identity.h"
#include "biggoron_aliases.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQUIRE(check)                                                  \
    do {                                                                \
        if (!(check)) {                                                 \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #check); \
            exit(1);                                                    \
        }                                                               \
    } while (0)
#include "aura_gbi_checks.h"

static void RecoveredFit(const char* path, Gfx* alias, size_t commands, int broken) {
    int model = SwordAura_ResourceModel(path, alias, commands);
    // An actual resolved ModernDin blade must not inherit a logical NEI/native
    // fit. Its source X tip is 6295 (full) or 2152 (broken), with its own socket.
    REQUIRE(model >= 0 && model < SWORD_AURA_MODEL_COUNT);
    REQUIRE(model != SWORD_AURA_BIGGORON_SWORD && model != SWORD_AURA_NATIVE_OOT_BGS);
    REQUIRE(model != SWORD_AURA_BROKEN_KNIFE && model != SWORD_AURA_NATIVE_OOT_BROKEN);
    for (int rig = 0; rig < 3; ++rig) {
        const SwordAuraGeometry* fit = &sSwordAuraGeometry[model][rig];
        const Gfx* layers[] = { fit->core, fit->flame, fit->tight };
        for (int layer = 0; layer < 3; ++layer) {
            AuraGbi geometry = { 0 };
            for (int axis = 0; axis < 3; ++axis) {
                geometry.min[axis] = 32767;
                geometry.max[axis] = -32768;
            }
            AuraWalk(&geometry, layers[layer], NULL, 0);
            REQUIRE(geometry.triangles > 20);
            REQUIRE(geometry.min[0] >= 900 && geometry.min[0] < 1000);
            REQUIRE(geometry.max[0] >= (broken ? 2150 : 6290));
            REQUIRE(geometry.max[0] < (broken ? 2300 : layer == 1 ? 6700 : 6350));
            // Main blade faces occupy Z=-45..69; the old native fallback has
            // its positive face behind Z=0 and cannot case the front surface.
            REQUIRE(geometry.min[2] <= -85 && geometry.max[2] >= 125);
        }
    }
}

int main(void) {
    RecoveredFit("__OTR__objects/object_custom_equip/gCustomLongswordDL", sModernFull,
                 sizeof(sModernFull) / sizeof(Gfx), 0);
    RecoveredFit("__OTR__objects/object_custom_equip/gCustomBreakableLongswordDL", sModernBreakable,
                 sizeof(sModernBreakable) / sizeof(Gfx), 0);
    RecoveredFit("__OTR__objects/object_custom_equip/gCustomBrokenLongswordDL", sModernBroken,
                 sizeof(sModernBroken) / sizeof(Gfx), 1);
    RecoveredFit("__OTR__objects/object_link_boy/gLinkAdultLeftHandHoldingBgsNearDL", sModernNative,
                 sizeof(sModernNative) / sizeof(Gfx), 0);
    Gfx shortAlias[] = { sModernFull[4] };
    REQUIRE(SwordAura_ResourceModel(NULL, shortAlias, 1) == -1);
    REQUIRE(SwordAura_ResourceModel(NULL, NULL, 0) == -1);
    puts("PASS exact recovered ModernDin Biggoron identities and generated "
         "native GBI in all rigs");
    return 0;
}
