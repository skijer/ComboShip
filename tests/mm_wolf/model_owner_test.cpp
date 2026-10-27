#include "mods/pak_loader/pak_loader.h"
#include "mods/items/logic/adult_link_render.h"
#include <cassert>
#include <cstdio>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Ship {
class Archive;
class IResource;
}
enum PakModelSource { PAK_SOURCE_PAK, PAK_SOURCE_ZOBJ, PAK_SOURCE_O2R };
constexpr unsigned PAK_MAX_LIMBS = 21;
struct PakModel;
static std::vector<PakModel> sModels;
static s32 sSelectedAdultIndex = -1, sSelectedChildIndex = -1, sSelectedEquipIndex = -1;
static s32 sForcedModelIndex = -1, sForcedEquipIndex = -1;
static bool adult, enabled, slotMix;
static std::map<u32, Gfx*> sCachedEquipDLs;
extern "C" {
SaveContext gSaveContext{};
s32 AdultLink_IsActive(void) { return adult; }
int32_t CVarGetInteger(const char*, int32_t fallback) { return enabled ? 1 : fallback; }
}
// Parsing and cache I/O are independent boundaries. The ownership predicates
// run unchanged against the complete native PakModel state below.
static void PakLoader_CheckMaskForce(void) {}
static void EnsureSlotMixLoaded(void) {}
static bool AnySlotMixActive(void) { return slotMix; }
static void sGetEquipDLs(void) {}
#include "pak-owner.inc"

int main() {
    // MM's legacy linkAge value is adult even when NEI's Time Gate selects child.
    gSaveContext.save.linkAge = 0;
    sModels.resize(3);
    sModels[0].isEquipmentOnly = 1;
    sModels[0].adultReady = sModels[0].childReady = 1;
    sModels[1].childReady = 1;
    sModels[2].adultReady = 1;
    sCachedEquipDLs[1] = reinterpret_cast<Gfx*>(uintptr_t(2));
    sSelectedEquipIndex = 0;
    if (!PakLoader_HasActiveModel() || PakLoader_HasActiveBodyModel()) {
        std::fputs("FAIL equipment-only selection must remain active for equipment without owning Wolf's body\n", stderr);
        return 1;
    }
    sSelectedEquipIndex = -1;
    sForcedEquipIndex = 0;
    assert(PakLoader_HasActiveModel() && !PakLoader_HasActiveBodyModel());
    sForcedEquipIndex = -1;
    slotMix = true;
    assert(PakLoader_HasActiveModel() && !PakLoader_HasActiveBodyModel());
    slotMix = false;
    sSelectedChildIndex = 1;
    assert(!PakLoader_HasActiveBodyModel()); // Body toggle disabled.
    enabled = true;
    assert(PakLoader_HasActiveBodyModel()); // Actual NEI age, not MM's pinned age.
    sSelectedChildIndex = 2;
    assert(!PakLoader_HasActiveBodyModel()); // Wrong-age body cannot own draw.
    adult = true;
    sSelectedAdultIndex = 2;
    assert(PakLoader_HasActiveBodyModel());
    sSelectedAdultIndex = 1;
    assert(!PakLoader_HasActiveBodyModel());
    enabled = false;
    sForcedModelIndex = 2;
    assert(PakLoader_HasActiveBodyModel()); // Forced body retains priority.
    sForcedModelIndex = 0;
    assert(!PakLoader_HasActiveBodyModel()); // Equipment is never a body owner.
    sForcedModelIndex = -1;
    sSelectedAdultIndex = -1;
    assert(!PakLoader_HasActiveBodyModel());
    std::puts("PASS production MM PAK ownership: equipment/slot mix, enabled/forced bodies and NEI ages");
}
