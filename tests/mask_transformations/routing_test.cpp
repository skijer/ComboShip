// Production mask lookup and item-use dispatcher; runtime resource/config seams only.
#include "z64item.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_map>

using u8 = uint8_t;
using s8 = int8_t;
using s32 = int32_t;
struct PlayState {};
struct Player {};
/* PRODUCTION_FORM_ENUM */
/* PRODUCTION_SHADOW_CRYSTAL_ID */

// Only state fields read by the extracted production lookup are needed here.
static struct {
    MmFormStateId state = MMFORM_STATE_INACTIVE;
    MmPlayerTransformation currentForm = MM_PLAYER_FORM_HUMAN;
} gFormState;

static std::unordered_map<std::string, int> cvars;
static bool mmAssets = true;
static int handledItem = ITEM_NONE;
static bool progressionUnlocked = true;
static s8 sFleetPendingForm = -1;
u8 MaskProgression_CanTransform(s32 item) {
    switch (item) {
        case ITEM_MM_MASK_DEKU:
        case ITEM_MM_MASK_GORON:
        case ITEM_MM_MASK_ZORA:
        case ITEM_MM_MASK_KEATON:
        case ITEM_MASK_GORON:
        case ITEM_MASK_ZORA:
        case ITEM_MASK_KEATON:
            return progressionUnlocked;
        default:
            return 1;
    }
}
int CVarGetInteger(const char* name, int fallback) {
    const auto it = cvars.find(name);
    return it == cvars.end() ? fallback : it->second;
}
u8 MmAssets_IsAvailable() { return mmAssets; }
void TransformMasks_HandleMaskUse(PlayState*, Player*, s32 item) { handledItem = item; }
u8 CustomForms_TrySkinItem(PlayState*, Player*, s32) { return 0; }
/* PRODUCTION_MASK_ROUTING */

static void expectUse(int item, TransformMaskId form, bool consumed) {
    PlayState play;
    Player player;
    handledItem = ITEM_NONE;
    assert(MmForm_GetMaskType(item) == form);
    assert(bool(TransformMasks_TryFormFromItem(&play, &player, item)) == consumed);
    assert(handledItem == (consumed ? item : ITEM_NONE));
}

int main() {
    cvars["gMods.TransformMasks.Enabled"] = 1;
    // Missing/new config must retain the behavior used by existing scene placements.
    expectUse(ITEM_MASK_GORON, TRANSFORM_MASK_GORON, true);
    expectUse(ITEM_MASK_ZORA, TRANSFORM_MASK_ZORA, true);
    for (int enabled : {0, 1}) {
        cvars["gMods.TransformMasks.OotGoronZora"] = enabled;
        expectUse(ITEM_MASK_GORON, enabled ? TRANSFORM_MASK_GORON : TRANSFORM_MASK_NONE, enabled);
        expectUse(ITEM_MASK_ZORA, enabled ? TRANSFORM_MASK_ZORA : TRANSFORM_MASK_NONE, enabled);
        expectUse(ITEM_MM_MASK_GORON, TRANSFORM_MASK_GORON, true);
        expectUse(ITEM_MM_MASK_ZORA, TRANSFORM_MASK_ZORA, true);
        expectUse(ITEM_MM_MASK_DEKU, TRANSFORM_MASK_DEKU, true);
        expectUse(ITEM_MM_MASK_FIERCE_DEITY, TRANSFORM_MASK_FIERCE_DEITY, true);
        expectUse(ITEM_MASK_KEATON, TRANSFORM_MASK_KEATON_FORM, true);
        expectUse(ITEM_MM_MASK_KEATON, TRANSFORM_MASK_KEATON_FORM, true);
        cvars["gMods.GerudoMaskTransform"] = 1;
        expectUse(ITEM_MASK_GERUDO, TRANSFORM_MASK_GERUDO, true);
        for (int item : {ITEM_MASK_SKULL, ITEM_MASK_SPOOKY, ITEM_MASK_TRUTH, ITEM_MASK_BUNNY})
            expectUse(item, TRANSFORM_MASK_NONE, false);
    }
    // Original MM forms still need the existing master toggle and resource pack.
    for (bool available : {false, true}) {
        mmAssets = available;
        for (int master : {0, 1}) {
            cvars["gMods.TransformMasks.Enabled"] = master;
            cvars["gMods.TransformMasks.OotGoronZora"] = 1;
            expectUse(ITEM_MASK_GORON, TRANSFORM_MASK_GORON, master && available);
            expectUse(ITEM_MASK_ZORA, TRANSFORM_MASK_ZORA, master && available);
            expectUse(ITEM_MM_MASK_GORON, TRANSFORM_MASK_GORON, master && available);
            expectUse(ITEM_MM_MASK_ZORA, TRANSFORM_MASK_ZORA, master && available);
            // Both Keaton copies keep their independent existing opt-out.
            for (int keaton : {0, 1}) {
                cvars["gMods.KeatonMaskTransform"] = keaton;
                expectUse(ITEM_MASK_KEATON, keaton ? TRANSFORM_MASK_KEATON_FORM : TRANSFORM_MASK_NONE, keaton);
                expectUse(ITEM_MM_MASK_KEATON, keaton ? TRANSFORM_MASK_KEATON_FORM : TRANSFORM_MASK_NONE, keaton);
            }
        }
    }
    // A live opt-out must still let the same OoT mask exit its active form.
    mmAssets = true;
    cvars["gMods.TransformMasks.Enabled"] = 1;
    cvars["gMods.TransformMasks.OotGoronZora"] = 0;
    gFormState.state = MMFORM_STATE_ACTIVE;
    gFormState.currentForm = MM_PLAYER_FORM_GORON;
    expectUse(ITEM_MASK_GORON, TRANSFORM_MASK_GORON, true);
    expectUse(ITEM_MASK_ZORA, TRANSFORM_MASK_NONE, false);
    expectUse(ITEM_MM_MASK_ZORA, TRANSFORM_MASK_ZORA, true);
    gFormState.currentForm = MM_PLAYER_FORM_ZORA;
    expectUse(ITEM_MASK_ZORA, TRANSFORM_MASK_ZORA, true);
    expectUse(ITEM_MASK_GORON, TRANSFORM_MASK_NONE, false);
    expectUse(ITEM_MM_MASK_GORON, TRANSFORM_MASK_GORON, true);
    for (MmFormStateId state : {MMFORM_STATE_INACTIVE, MMFORM_STATE_TRANSFORMING, MMFORM_STATE_DETRANSFORMING}) {
        gFormState.state = state;
        expectUse(ITEM_MASK_GORON, TRANSFORM_MASK_NONE, false);
        expectUse(ITEM_MASK_ZORA, TRANSFORM_MASK_NONE, false);
    }
    gFormState.state = MMFORM_STATE_ACTIVE;
    gFormState.currentForm = MM_PLAYER_FORM_DEKU;
    expectUse(ITEM_MASK_GORON, TRANSFORM_MASK_NONE, false);
    expectUse(ITEM_MASK_ZORA, TRANSFORM_MASK_NONE, false);
    // Locked progression affects the intended item families, including early pickups.
    cvars["gMods.TransformMasks.OotGoronZora"] = 1;
    cvars["gMods.KeatonMaskTransform"] = 1;
    progressionUnlocked = false;
    gFormState.state = MMFORM_STATE_INACTIVE;
    for (int item : {ITEM_MM_MASK_DEKU, ITEM_MM_MASK_GORON, ITEM_MM_MASK_ZORA, ITEM_MM_MASK_KEATON,
                     ITEM_MASK_GORON, ITEM_MASK_ZORA, ITEM_MASK_KEATON})
        expectUse(item, TRANSFORM_MASK_NONE, false);
    expectUse(ITEM_MM_MASK_FIERCE_DEITY, TRANSFORM_MASK_FIERCE_DEITY, true);
    expectUse(ITEM_MASK_GERUDO, TRANSFORM_MASK_GERUDO, true);
    // Enabling a gate mid-form cannot strand the player or allow a locked form switch.
    const struct { MmPlayerTransformation form; int mmItem; int ootItem; TransformMaskId mask; } families[] = {
        {MM_PLAYER_FORM_DEKU, ITEM_MM_MASK_DEKU, ITEM_MM_MASK_DEKU, TRANSFORM_MASK_DEKU},
        {MM_PLAYER_FORM_GORON, ITEM_MM_MASK_GORON, ITEM_MASK_GORON, TRANSFORM_MASK_GORON},
        {MM_PLAYER_FORM_ZORA, ITEM_MM_MASK_ZORA, ITEM_MASK_ZORA, TRANSFORM_MASK_ZORA},
        {MM_PLAYER_FORM_KEATON, ITEM_MM_MASK_KEATON, ITEM_MASK_KEATON, TRANSFORM_MASK_KEATON_FORM},
    };
    for (const auto& family : families) {
        assert(MmForm_MaskIdToForm(family.mask) == family.form);
        gFormState.state = MMFORM_STATE_ACTIVE;
        gFormState.currentForm = family.form;
        expectUse(family.mmItem, family.mask, true);
        expectUse(family.ootItem, family.mask, true);
        if (family.form != MM_PLAYER_FORM_GORON)
            expectUse(ITEM_MM_MASK_GORON, TRANSFORM_MASK_NONE, false);
        if (family.form != MM_PLAYER_FORM_ZORA)
            expectUse(ITEM_MASK_ZORA, TRANSFORM_MASK_NONE, false);
    }
    // Entering OoT in an MM native form must respect the same regional unlock.
    gFormState.state = MMFORM_STATE_INACTIVE;
    for (auto form : {MM_PLAYER_FORM_DEKU, MM_PLAYER_FORM_GORON, MM_PLAYER_FORM_ZORA}) {
        MmForm_FleetApplyForm(form);
        assert(sFleetPendingForm == MM_PLAYER_FORM_HUMAN && "MM arrival bypasses the dungeon gate");
    }
    MmForm_FleetApplyForm(MM_PLAYER_FORM_FIERCE_DEITY);
    assert(sFleetPendingForm == MM_PLAYER_FORM_FIERCE_DEITY);
    progressionUnlocked = true;
    for (int form = 0; form <= MM_PLAYER_FORM_HUMAN; ++form) {
        MmForm_FleetApplyForm(form);
        assert(sFleetPendingForm == form);
    }
    for (int form : {-1, 99}) {
        MmForm_FleetApplyForm(form);
        assert(sFleetPendingForm == MM_PLAYER_FORM_HUMAN);
    }
    std::cout << "PASS mask routing: optional OoT aliases, child-dungeon entry gates, live-toggle exit, MM forms, shared Keaton and resource gates\n";
}
