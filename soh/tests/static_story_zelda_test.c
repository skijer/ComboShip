#include <stdio.h>
#include "tests/test_require.h"
#include "z64object.h"
#include "src/overlays/actors/ovl_En_Viewer/static_story_actor.h"

int main(void) {
    /* Public params are the catalogue API; the new actor ID is append-only. */
    const StaticStoryActorType child = StaticStoryActor_GetType(0x7D03);
    REQUIRE(child == 28);
    REQUIRE(StaticStoryActor_GetType(0x7D13) == child && StaticStoryActor_GetPose(0x7D13) == 1);
    REQUIRE(StaticStoryActor_IsAvailable(child));
    const StaticStoryObjectRequirements objects = StaticStoryActor_GetObjectRequirements(child);
    REQUIRE(objects.modelObjectId == OBJECT_ZL4 && objects.animationObjectId == OBJECT_ZL4);
    REQUIRE(StaticStoryActor_GetResourceSource(child) == STATIC_STORY_RESOURCE_OOT_OBJECT);
    REQUIRE(StaticStoryActor_CanTalk(child));
    REQUIRE(StaticStoryActor_SelectTextId(child, NULL) == 0x8F2D);
    const StaticStoryPoseDescriptor* childIdle = StaticStoryActor_ResolvePose(child, 0);
    const StaticStoryPoseDescriptor* childClasp = StaticStoryActor_ResolvePose(child, 1);
    REQUIRE(childIdle && childClasp && childIdle->animation != childClasp->animation);
    REQUIRE(childIdle->skeletonFamily == childClasp->skeletonFamily);
    REQUIRE(!StaticStoryActor_CanTrack(child, 0) && !StaticStoryActor_CanTrack(child, 1));
    const StaticStoryPoseDescriptor* childGlow = StaticStoryActor_ResolvePose(child, 2);
    REQUIRE(StaticStoryActor_SanitizePose(child, 2) == 2);
    REQUIRE(childGlow->animation == childClasp->animation && (childGlow->flags & (1u << 7)));
    REQUIRE(!(childClasp->flags & (1u << 7)) && !(childIdle->flags & (1u << 7)));
    REQUIRE(StaticStoryActor_SanitizePose(child, 3) == 0);
    REQUIRE(StaticStoryActor_GetType(0x7D04) == STATIC_STORY_ACTOR_NONE);
    REQUIRE(StaticStoryActor_GetType(0x7D01) == STATIC_STORY_ACTOR_MIKAU);
    REQUIRE(StaticStoryActor_GetType(0x7D02) == STATIC_STORY_ACTOR_DARMANI);

    const StaticStoryActorType adult = StaticStoryActor_GetType(0x7F24);
    REQUIRE(adult == STATIC_STORY_ACTOR_ADULT_ZELDA);
    REQUIRE(StaticStoryActor_GetPose(0x7F24) == 2);
    const StaticStoryPoseDescriptor* old0 = StaticStoryActor_ResolvePose(adult, 0);
    const StaticStoryPoseDescriptor* old1 = StaticStoryActor_ResolvePose(adult, 1);
    const StaticStoryPoseDescriptor* clasp = StaticStoryActor_ResolvePose(adult, 2);
    REQUIRE(old0->animation == STATIC_ANIM_ADULT_ZELDA_IDLE && old1->animation == old0->animation);
    REQUIRE(clasp->animation != old0->animation);
    REQUIRE(clasp->skeletonFamily == old0->skeletonFamily && !StaticStoryActor_CanTrack(adult, 2));
    REQUIRE(StaticStoryActor_GetAnimationObjectId(adult) == OBJECT_ZL2_ANIME2);
    REQUIRE(StaticStoryActor_GetDefinition(adult)->drawContract == STATIC_DRAW_CONTRACT_FACE_FLEX);
    const StaticStoryPoseDescriptor* adultGlow = StaticStoryActor_ResolvePose(adult, 3);
    REQUIRE(StaticStoryActor_SanitizePose(adult, 3) == 3);
    REQUIRE(adultGlow->animation == clasp->animation && (adultGlow->flags & (1u << 7)));
    REQUIRE(!(old0->flags & (1u << 7)) && !(old1->flags & (1u << 7)) && !(clasp->flags & (1u << 7)));
    REQUIRE(StaticStoryActor_SanitizePose(adult, 4) == 0);
    puts("PASS Zelda catalogue: independent child actor, distinct poses, native/custom resource identities, preserved "
         "aliases");
    return 0;
}
