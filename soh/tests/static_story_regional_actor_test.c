#include <assert.h>
#include <string.h>
#include "../src/overlays/actors/ovl_En_Viewer/static_story_mm_actor.h"
int main(void) {
    const unsigned params[] = { 0x7E0F, 0x7D01, 0x7D02 };
    const unsigned poses[] = { 5, 2, 2 };
    const unsigned limbs[] = { 19, 22, 19 };
    const unsigned matrices[] = { 0, 20, 17 };
    const unsigned frames[][5] = { { 30, 20, 20, 30, 34 }, { 30, 12 }, { 29, 39 } };
    for (unsigned i = 0; i < 3; i++) {
        for (unsigned p = 0; p < poses[i]; p++) {
            assert(StaticStoryActor_IsParam(params[i] | (p << 4)));
            StaticStoryActorType type = StaticStoryActor_GetType(params[i] | (p << 4));
            assert(type == 25 + i);
            assert(StaticStoryActor_IsAvailable(type));
            assert(StaticStoryActor_GetResourceSource(type) == STATIC_STORY_RESOURCE_MM_ARCHIVE);
            const StaticStoryMmPresentation* row = StaticStoryMm_GetPresentation(type, p);
            assert(row && row->limbCount == limbs[i] && row->matrixCount == matrices[i]);
            assert(row->frameCount == frames[i][p]);
            assert(!StaticStoryActor_CanTrack(type, p));
            assert(StaticStoryActor_ResolvePose(type, p));
            assert(StaticStoryMm_ResourcesComplete(row, true, true, false));
            assert(!StaticStoryMm_ResourcesComplete(row, false, true, false));
            assert(!StaticStoryMm_ResourcesComplete(row, true, false, false));
        }
        assert(!StaticStoryMm_GetPresentation(25 + i, poses[i]));
    }
    assert(StaticStoryActor_GetType(0x7E0E) == 24);
    assert(StaticStoryActor_GetType(0x7D00) == 0);
    assert(StaticStoryActor_GetType(0x7D04) == 0);
    assert(StaticStoryActor_GetType(0x0010) == 0);
    return 0;
}
