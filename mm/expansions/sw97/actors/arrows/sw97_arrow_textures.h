#ifndef SW97_ARROW_TEXTURES_H
#define SW97_ARROW_TEXTURES_H

#include "global.h"
#include "2s2h/BenGui/CosmeticEditor.h"
#include "soh/ResourceManagerHelpers.h"
#include <libultraship/bridge/resourcebridge.h>

// The OoT companion is optional in MM. Preserve its existing effect geometry
// when both lists load; otherwise select the matching MM material/model pair.
static inline s32 Sw97_ArrowHasCompanionGeometry(const char* material, const char* model) {
    const s32 alt = ResourceMgr_IsAltAssetsEnabled();
    if ((!ResourceMgr_FileExists(material) && !(alt && ResourceMgr_FileAltExists(material))) ||
        (!ResourceMgr_FileExists(model) && !(alt && ResourceMgr_FileAltExists(model)))) {
        return false;
    }
    return ResourceGetDataByName(material) != NULL && ResourceGetDataByName(model) != NULL;
}

// The native material must run first. Replace only its two image slots, leaving
// render-tile shifts/masks/wrap, colors and combiner available to the usual scroll.
static inline Gfx* Sw97_ArrowLoadMedallionTextures(Gfx* gfx, const char* first, const char* second) {
    if (!ResourceMgr_IsAltAssetsEnabled()) {
        return gfx;
    }

    const u8 firstExists = ResourceMgr_FileAltExists(first);
    const u8 secondExists = ResourceMgr_FileAltExists(second);
    if (!firstExists || !secondExists || ResourceGetDataByName(first) == NULL ||
        ResourceGetDataByName(second) == NULL) {
        return gfx;
    }

    // Named references retain image metadata. Each 16-bit block transfer fills
    // one native 32x64 I8 slot (2048 bytes); tile 7 is only the transfer descriptor.
    gDPPipeSync(gfx++);
    gDPSetTextureImage(gfx++, G_IM_FMT_I, G_IM_SIZ_16b, 1, first);
    gDPSetTile(gfx++, G_IM_FMT_I, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, G_TX_WRAP, 0, 0, G_TX_WRAP, 0, 0);
    gDPLoadSync(gfx++);
    gDPLoadBlock(gfx++, G_TX_LOADTILE, 0, 0, ((32 * 64) / 2) - 1, CALC_DXT(32, G_IM_SIZ_8b_BYTES));
    gDPPipeSync(gfx++);
    gDPSetTextureImage(gfx++, G_IM_FMT_I, G_IM_SIZ_16b, 1, second);
    gDPSetTile(gfx++, G_IM_FMT_I, G_IM_SIZ_16b, 0, 0x100, G_TX_LOADTILE, 0, G_TX_WRAP, 0, 0, G_TX_WRAP, 0, 0);
    gDPLoadSync(gfx++);
    gDPLoadBlock(gfx++, G_TX_LOADTILE, 0, 0, ((32 * 64) / 2) - 1, CALC_DXT(32, G_IM_SIZ_8b_BYTES));
    gDPPipeSync(gfx++);
    return gfx;
}

#endif
