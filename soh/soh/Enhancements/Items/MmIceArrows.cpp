#include "soh/ActorDB.h"
#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#ifdef COMBO_BUILD
#include <ship/resource/CrossRMRegistry.h>
#endif
#include <memory>
#include <new>

extern "C" {
#include "MmIceArrows.h"
#include "overlays/actors/ovl_Bg_MmIcefloe/z_bg_mm_icefloe.h"
#include "overlays/actors/ovl_Bg_MmIceblock/z_bg_mm_iceblock.h"
#include "objects/object_spot08_obj/object_spot08_obj.h"
#include "objects/object_demo_kekkai/object_demo_kekkai.h"
}

namespace {
struct FloeResources {
    std::shared_ptr<Ship::IResource> collision;
    std::shared_ptr<Ship::IResource> model;
};
} // namespace

extern "C" s32 MmIceArrows_GetActorId(void) {
    if (ActorDB::Instance == nullptr) {
        return -1;
    }
    // Retrieve by name rather than keeping an ID from a previous game session.
    int id = ActorDB::Instance->RetrieveId("Bg_MmIcefloe");
    if (id >= 0) {
        return id;
    }
    ActorDBInit profile;
    profile.name = "Bg_MmIcefloe";
    profile.desc = "Ice Arrow Temporary Water Platform";
    profile.category = ACTORCAT_BG;
    profile.flags = ACTOR_FLAG_UPDATE_CULLING_DISABLED;
    profile.objectId = OBJECT_GAMEPLAY_KEEP;
    profile.instanceSize = sizeof(BgMmIcefloe);
    profile.init = BgMmIcefloe_Init;
    profile.destroy = BgMmIcefloe_Destroy;
    profile.update = BgMmIcefloe_Update;
    profile.draw = BgMmIcefloe_Draw;
    return ActorDB::Instance->AddEntry(profile).entry.id;
}

extern "C" s32 MmIceArrows_GetOctorokBlockActorId(void) {
    if (ActorDB::Instance == nullptr) {
        return -1;
    }
    int id = ActorDB::Instance->RetrieveId("Bg_MmIceblock");
    if (id >= 0) {
        return id;
    }
    ActorDBInit profile;
    profile.name = "Bg_MmIceblock";
    profile.desc = "Ice Arrow Frozen Octorok Block";
    profile.category = ACTORCAT_BG;
    profile.flags = ACTOR_FLAG_UPDATE_CULLING_DISABLED;
    profile.objectId = OBJECT_GAMEPLAY_KEEP;
    profile.instanceSize = sizeof(BgMmIceblock);
    profile.init = BgMmIceblock_Init;
    profile.destroy = BgMmIceblock_Destroy;
    profile.update = BgMmIceblock_Update;
    profile.draw = BgMmIceblock_Draw;
    return ActorDB::Instance->AddEntry(profile).entry.id;
}

static void* AcquireNativeIceResources(const char* collisionName, const char* modelName, CollisionHeader** collision,
                                       Gfx** model) {
    *collision = nullptr;
    *model = nullptr;
#ifdef COMBO_BUILD
    auto owner = Ship::CrossRMRegistry::GetOrActive("oot");
#else
    auto context = Ship::Context::GetRawInstance();
    auto owner = context ? context->GetResourceManager() : nullptr;
#endif
    if (!owner) {
        return nullptr;
    }
    auto resources = std::unique_ptr<FloeResources>(new (std::nothrow) FloeResources);
    if (!resources) {
        return nullptr;
    }
    resources->collision = owner->LoadResource(collisionName);
    resources->model = owner->LoadResource(modelName);
    if (!resources->collision || !resources->model) {
        return nullptr;
    }
    *collision = static_cast<CollisionHeader*>(resources->collision->GetRawPointer());
    *model = static_cast<Gfx*>(resources->model->GetRawPointer());
    if (!*collision || !*model) {
        *collision = nullptr;
        *model = nullptr;
        return nullptr;
    }
    return resources.release();
}

extern "C" void* MmIceArrows_AcquireResources(CollisionHeader** collision, Gfx** model) {
    return AcquireNativeIceResources(gZorasFountainIcebergCol, gZorasFountainIcebergDL, collision, model);
}

extern "C" void* MmIceArrows_AcquireBlockResources(CollisionHeader** collision, Gfx** model) {
    return AcquireNativeIceResources(gWaterTrialIceBlockCol, gWaterTrialIceBlockDL, collision, model);
}

extern "C" void MmIceArrows_ReleaseResources(void* handle) {
    delete static_cast<FloeResources*>(handle);
}
