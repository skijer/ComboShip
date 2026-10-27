#ifndef CW_SWORD_ELEMENT_OVERRIDE_H
#define CW_SWORD_ELEMENT_OVERRIDE_H

#include <string.h>
#include <libultraship/bridge/consolevariablebridge.h>

// These keep their saved setting identities and menu locations. Enabling a
// cheat replaces the prior cheat; disabling it leaves normal sword selections
// and progression untouched. Called after the checkbox changes its CVar.
static inline void CwSwordElementOverride_Changed(const char* changed) {
    static const char* keys[] = { "gEnhancements.DinFireSwordDamage", "gEnhancements.IceSwordDamage",
                                  "gEnhancements.LightSwordDamage" };
    if (changed == NULL)
        return;
    for (int selected = 0; selected < 3; ++selected) {
        if (strcmp(changed, keys[selected]) == 0) {
            for (int other = 0; other < 3; ++other)
                if (other != selected)
                    CVarSetInteger(keys[other], 0);
            return;
        }
    }
}
#endif
