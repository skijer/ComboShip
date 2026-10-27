#include "global.h"
#include "../../combo/render/sword_element_aura_identity.h"
#include "biggoron_aliases.h"
#include "din_fire_sword.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef BIGGORON_TEST_MM
#include "mods/nei_oot_compat.h"
#include "objects/object_link_child/object_link_child.h"
#endif

#define REQUIRE(check)                                                  \
    do {                                                                \
        if (!(check)) {                                                 \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #check); \
            exit(1);                                                    \
        }                                                               \
    } while (0)
static Gfx* resolved = sModernNative;
static Gfx* sizeOwner = sModernNative;
static size_t reportedBytes = sizeof(sModernNative);
static int registeredModel = -1;
static void* registeredHand;
Gfx* ResourceMgr_LoadGfxByName(const char* path) {
    return resolved;
}
size_t ResourceGetSizeByName(const char* path) {
    REQUIRE(strstr(path, "gLinkAdultLeftHandHoldingBgsNearDL") != NULL);
    return reportedBytes;
}
void* ResourceGetDataByName(const char* path) {
    REQUIRE(strstr(path, "gLinkAdultLeftHandHoldingBgsNearDL") != NULL);
    return sizeOwner;
}
void DinFireSword_SetHandModel(Player* player, void* hand, int model) {
    if (hand == NULL)
        return;
    registeredModel = model;
    registeredHand = hand;
}
#ifdef BIGGORON_TEST_MM
static int sPlayerLod;
u8 WeaponUpgrade_HasGreatFairy(void) {
    return 0;
}
u8 ResourceMgr_FileExists(const char* path) {
    return resolved != NULL;
}
void ResourceMgr_PatchGfxByName(const char* path, const char* name, int index, Gfx value) {
}
#else
void* MmForm_KafeiWhistleHandDL(const char* path) {
    return NULL;
}
void* HarpoonSkinSync_ResolvePlayerLimbDL(const char* path) {
    return NULL;
}
void* CustomForms_ResolveVanillaResource(const char* path) {
    return NULL;
}
#endif
#include "biggoron_native_production.inc"

static void Resolve(void) {
    registeredModel = -1;
    registeredHand = NULL;
#ifdef BIGGORON_TEST_MM
    Player player = { 0 };
    player.heldItemId = ITEM_SWORD_BGS;
    player.transformation = PLAYER_FORM_HUMAN;
    Gfx* selected = NULL;
    BiggoronNativeImportedHand(&player, &selected);
    REQUIRE(selected == resolved);
#else
    REQUIRE(Player_ResolveLimbDLForDummyOrLocal("__OTR__objects/object_link_boy/"
                                                "gLinkAdultLeftHandHoldingBgsNearDL") == resolved);
#endif
    REQUIRE(registeredHand == resolved);
}

int main(void) {
    // A branch-only replacement is a complete two-command resource without an
    // EndDL. Its successor after an Alt switch has a different allocation and
    // a larger size. The imported hand may never keep the old raw pointer.
    Gfx* first = malloc(2 * sizeof(Gfx));
    REQUIRE(first != NULL);
    first[0] = (Gfx){ .words = { .w0 = 0x31010000u, .w1 = 0 } };
    first[1] = (Gfx){ .words = { .w0 = 0x12345678u, .w1 = 0x9ABCDEF0u } };
    resolved = sizeOwner = first;
    reportedBytes = 2 * sizeof(Gfx);
    Resolve();
    REQUIRE(registeredModel == SWORD_AURA_NATIVE_OOT_BGS);
    resolved = sizeOwner = sModernNative;
    reportedBytes = sizeof(sModernNative);
    free(first);
    // The actual selected resource is a short alias, not the native 80-command
    // hand its public name resembles. The native boundary must inspect its
    // supplied complete commands before selecting a native sword profile.
    Resolve();
    REQUIRE(registeredModel == SWORD_AURA_MODERN_DIN_BGS);
    reportedBytes = 1;
    Resolve();
    REQUIRE(registeredModel == SWORD_AURA_NATIVE_OOT_BGS);
    reportedBytes = 0;
    Resolve();
    REQUIRE(registeredModel == SWORD_AURA_NATIVE_OOT_BGS);
    // Reload the same name into another allocation, changing its blade target.
    Gfx* reload = malloc(sizeof(sModernNative));
    REQUIRE(reload != NULL);
    memcpy(reload, sModernNative, sizeof(sModernNative));
    reload[5].words.w0 = 0x68A55E4Bu;
    reload[5].words.w1 = 0x08BD368Cu;
    resolved = sizeOwner = reload;
    reportedBytes = sizeof(sModernNative);
    Resolve();
    REQUIRE(registeredModel == SWORD_AURA_MODERN_DIN_BROKEN);

    // The pointer returned for drawing and the current named size can belong
    // to different resources after a reload/owner change. Never inspect the
    // two-command allocation using the other owner's larger command count.
    Gfx* branch = malloc(2 * sizeof(Gfx));
    REQUIRE(branch != NULL);
    branch[0] = (Gfx){ .words = { .w0 = 0x31010000u, .w1 = 0 } };
    branch[1] = (Gfx){ .words = { .w0 = 0x12345678u, .w1 = 0x9ABCDEF0u } };
    resolved = branch;
    sizeOwner = reload;
    reportedBytes = sizeof(sModernNative);
    Resolve();
    REQUIRE(registeredModel == SWORD_AURA_NATIVE_OOT_BGS);
    sizeOwner = branch;
    reportedBytes = 2 * sizeof(Gfx);
    Resolve();
    REQUIRE(registeredModel == SWORD_AURA_NATIVE_OOT_BGS);
    resolved = sizeOwner = sModernNative;
    reportedBytes = sizeof(sModernNative);
    free(branch);
    free(reload);
    Resolve();
    REQUIRE(registeredModel == SWORD_AURA_MODERN_DIN_BGS);
    resolved = sizeOwner = NULL;
    reportedBytes = 0;
    Resolve();
    REQUIRE(registeredModel == -1);
    resolved = sizeOwner = sModernNative;
    reportedBytes = sizeof(sModernNative);
    Resolve();
    REQUIRE(registeredModel == SWORD_AURA_MODERN_DIN_BGS);
    puts("PASS native Biggoron identity, Alt switches, reloads and pointer/size "
         "owner boundaries");
    return 0;
}
