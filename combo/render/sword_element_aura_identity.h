#ifndef SWORD_ELEMENT_AURA_IDENTITY_H
#define SWORD_ELEMENT_AURA_IDENTITY_H
#include "sword_element_aura_profiles.h"
#include <string.h>

// A resolved Din alias names its actual blade. Pack availability and a sword
// upgrade flag cannot establish which equipment model won the limb compositor.
static inline int SwordAura_ResourceModel(const char* path, const Gfx* source, size_t commands) {
    static const struct {
        u32 hi, lo;
        int model;
    } din[] = {
        { 0xB91A300A, 0xF55ED642, SWORD_AURA_DIN_ADULT },
        { 0x14F171F6, 0xA52C587B, SWORD_AURA_DIN_CHILD },
        { 0x72E2A885, 0x10DE5020, SWORD_AURA_DIN_BGS },
        { 0x3D9F2549, 0xED5D838C, SWORD_AURA_DIN_BROKEN },
        // Original adult Din equipment routes both custom and combined native
        // hands through these blade leaves, in its own authored held basis.
        { 0x0AC6FFD3, 0xF0AE6266, SWORD_AURA_MODERN_DIN_BGS },
        { 0x68A55E4B, 0x08BD368C, SWORD_AURA_MODERN_DIN_BROKEN },
    };
    // Only inspect complete commands supplied by the resource owner. An alias
    // can carry a marker first; the 128-bit hash command owns the next slot.
    for (size_t i = 0; source != NULL && i + 1 < commands && i < 12; ++i) {
        if ((source[i].words.w0 >> 24) == G_DL_OTR_HASH) {
            for (size_t j = 0; j < sizeof(din) / sizeof(din[0]); ++j)
                if (source[i + 1].words.w0 == din[j].hi && source[i + 1].words.w1 == din[j].lo)
                    return din[j].model;
        }
        if ((source[i].words.w0 >> 24) == G_ENDDL)
            break;
    }
    if (path == NULL)
        return -1;
    if (strstr(path, "gLinkAdultLeftHandHoldingMasterSword"))
        return SWORD_AURA_NATIVE_OOT_MASTER;
    if (strstr(path, "gLinkChildLeftFistAndKokiriSword"))
        return SWORD_AURA_NATIVE_OOT_KOKIRI;
    if (strstr(path, "gLinkAdultHandHoldingBrokenGiantsKnife"))
        return SWORD_AURA_NATIVE_OOT_BROKEN;
    if (strstr(path, "gLinkAdultLeftHandHoldingBgs"))
        return SWORD_AURA_NATIVE_OOT_BGS;
    if (strstr(path, "gLinkChildLeftHandHoldingMasterSwordDL"))
        return SWORD_AURA_NATIVE_OOT_CHILD_MASTER;
    if (strstr(path, "gLinkHumanLeftHandHoldingKokiriSword"))
        return SWORD_AURA_NATIVE_MM_KOKIRI;
    if (strstr(path, "gLinkHumanLeftHandHoldingRazorSword"))
        return SWORD_AURA_NATIVE_MM_RAZOR;
    if (strstr(path, "gLinkHumanLeftHandHoldingGildedSword"))
        return SWORD_AURA_NATIVE_MM_GILDED;
    if (strstr(path, "gLinkHumanLeftHandHoldingGreatFairysSword"))
        return SWORD_AURA_NATIVE_MM_FAIRY;
    return -1; // Unknown replacement models retain the documented logical fit.
}
#endif
