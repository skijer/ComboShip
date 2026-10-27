#include "mods/transformation_masks/assets/mm_asset_loader.h"
#include "mods/transformation_masks/assets/mm_normal_actor_resource.h"
#include <cassert>
#include <cstring>
#include <fast/resource/type/DisplayList.h>
#include <string>
#include <unordered_map>
extern "C" {
#include "src/overlays/actors/ovl_En_Viewer/static_story_mm_actor.h"
}
#define MMASSETS_LOG(...) ((void)0)
static std::unordered_map<std::string, MmNormalActor::Resource> resources;
static MmNormalActor::Resource
MmAssets_LoadResourceObjectFromMmArchive(const char *p) {
  auto i = resources.find(p);
  return i == resources.end() ? nullptr : i->second;
}
static MmNormalActor::Resource MmAssets_LoadAnjuNativeResource(const char *p) {
  return MmAssets_LoadResourceObjectFromMmArchive(p);
}
/* PRODUCTION_LOADERS */
/* ARCHIVE_RESOURCES */
int main() {
  BuildResources();
  for (int type = 25; type <= 27; type++)
    for (int pose = 0; pose < (type == 25 ? 5 : 2); pose++) {
      MmNormalActorResources r{};
      fprintf(stderr, "Loading actor%d pose%d\n", type, pose);
      assert(MmAssets_LoadNormalActor(type, pose, &r));
      assert(r.owner && r.skeleton && r.animation);
      assert((r.propDisplayList != nullptr) == (type == 26 && pose == 1));
      MmAssets_ReleaseNormalActor(r.owner);
    }
  auto standard = std::static_pointer_cast<SOH::Skeleton>(
      resources.at("objects/object_skb/gStalchildSkel"));
  for (const auto &limb : standard->limbResources) {
    auto typedLimb = std::static_pointer_cast<SOH::SkeletonLimb>(limb);
    if (!typedLimb->limbData.standardLimb.dList)
      continue;
    auto list = typedLimb->limbData.standardLimb.dList;
    typedLimb->limbData.standardLimb.dList = nullptr;
    MmNormalActorResources result{};
    assert(!MmAssets_LoadNormalActor(25, 0, &result) && !result.owner);
    typedLimb->limbData.standardLimb.dList = list;
    break;
  }
  const char *prop = "objects/object_zog/object_zog_DL_0280A8";
  auto correct = resources.at(prop);
  MmNormalActorResources r{};
  resources[prop] = resources.at("objects/object_zog/object_zog_Tex_024750");
  assert(!MmAssets_LoadNormalActor(26, 1, &r) && !r.owner);
  resources.erase(prop);
  assert(!MmAssets_LoadNormalActor(26, 1, &r) && !r.owner);
  resources[prop] = correct;
  auto skeleton = resources.at("objects/object_zog/object_zog_Skel_029170");
  auto typed = std::static_pointer_cast<SOH::Skeleton>(skeleton);
  auto header = typed->skeletonData.flexSkeletonHeader;
  typed->skeletonData.flexSkeletonHeader.dListCount = 19;
  assert(!MmAssets_LoadNormalActor(26, 1, &r));
  typed->skeletonData.flexSkeletonHeader = header;
  auto child = typed->limbResources[0];
  typed->limbResources[0] = correct;
  assert(!MmAssets_LoadNormalActor(26, 1, &r));
  typed->limbResources[0] = child;
  assert(MmAssets_LoadNormalActor(26, 1, &r));
  resources.clear();
  assert(r.skeleton->sh.limbCount == 22 &&
         r.animation->common.frameCount == 12 && r.propDisplayList);
  MmAssets_ReleaseNormalActor(r.owner);
  assert(!MmAssets_LoadNormalActor(27, 0, &r) && !r.owner);
  puts("PASS real-archive typed resource loader: nine poses, missing/wrong "
       "prop, bad matrices/children, cache clear and release");
}
