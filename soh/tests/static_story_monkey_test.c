#include <stdio.h>
#include <string.h>
#include "../src/overlays/actors/ovl_En_Viewer/static_story_mm_actor.h"
#include "../src/overlays/actors/ovl_En_Viewer/static_story_dialogue.h"
#define CHECK(x)                                                         \
    do {                                                                 \
        if (!(x)) {                                                      \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); \
            return 1;                                                    \
        }                                                                \
    } while (0)

int main(void) {
    StaticStoryActorType monkey = StaticStoryActor_GetType(0x7E0E);
    CHECK(monkey != STATIC_STORY_ACTOR_NONE);
    CHECK(StaticStoryActor_IsAvailable(monkey));
    CHECK(StaticStoryActor_GetType(0x7E1E) == monkey);
    CHECK(StaticStoryActor_GetType(0x7F0E) == STATIC_STORY_ACTOR_NONE);
    CHECK(StaticStoryActor_GetResourceSource(monkey) == STATIC_STORY_RESOURCE_MM_ARCHIVE);
    const StaticStoryActorDefinition* def = StaticStoryActor_GetDefinition(monkey);
    CHECK(def->scale == 0.012f && def->colliderRadius == 15 && def->colliderHeight == 30);
    const unsigned frames[] = { 12, 71, 16 };
    for (unsigned pose = 0; pose < 3; ++pose) {
        const StaticStoryMmPresentation* p = StaticStoryMm_GetPresentation(monkey, pose);
        CHECK(p && p->limbCount == 22 && p->matrixCount == 20);
        CHECK(p->frameCount == frames[pose]);
        CHECK(strcmp(p->skeletonPath, "objects/object_mnk/gMonkeySkel") == 0);
        CHECK(p->eyeCount == 3 && p->eyeSegment == 8 && p->mouthCount == 0);
        CHECK(!p->requiresSecondarySkeleton && !StaticStoryActor_CanTrack(monkey, pose));
        CHECK(!StaticStoryActor_LocksRootTranslation(monkey, pose));
    }
    CHECK(strstr(StaticStoryMm_GetPresentation(monkey, 2)->animationPath, "object_mnk_Anim_008814") != NULL);
    CHECK(StaticStoryMm_GetPresentation(monkey, 3) == NULL);
    CHECK(StaticStoryActor_SanitizePose(monkey, 15) == 0);
    CHECK(StaticStoryDialogue_GetText(StaticStoryActor_SelectTextId(monkey, NULL), 0) != NULL);
    CHECK(StaticStoryActor_GetType(0x7E0D) != monkey);
    puts("PASS placeable monkey: reserved params, native rig/clips/scale, blink, dialogue, invalid-pose fallback");
    return 0;
}
