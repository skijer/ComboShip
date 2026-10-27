#include "overlays/actors/ovl_En_Viewer/z_en_viewer.h"
#include "objects/object_sa/object_sa.h"
#include "overlays/actors/ovl_En_Viewer/static_story_actor.h"
#include <assert.h>
#include <stdio.h>

static int models, particles, seated;
void ComboForestActors_DrawSariaParticles(Actor*, PlayState*, int sitting) {
    ++particles; seated = sitting;
}
#define StaticStoryActor_GetInteractionEyeIndex(...) 0
#undef OPEN_DISPS
#define OPEN_DISPS(...) ((void)0)
#undef CLOSE_DISPS
#define CLOSE_DISPS(...) ((void)0)
#undef gSPSegment
#define gSPSegment(...) ((void)0)
#define SkelAnime_DrawSkeletonOpa(...) (++models)
/* PRODUCTION */
int main() {
    PlayState play = {0}; EnViewer saria = {0};
    saria.staticState.staticMode = true;
    saria.staticState.type = STATIC_STORY_ACTOR_SARIA;
    for (int pose = 0; pose < 4; ++pose) {
        models = particles = 0; saria.staticState.pose = pose;
        EnViewer_DrawStaticSaria(&saria, &play);
        assert(models == 1 && particles == 1 && "every placed Saria pose must call the shared GI particle bridge");
        assert(seated == (pose == 3));
    }
    models = particles = 0; saria.staticState.staticMode = false;
    EnViewer_DrawStaticSaria(&saria, &play);
    assert(models == 1 && particles == 0);
    puts("PASS OoT C viewer draw bridge: four placed poses keep native model draw and select seated particles; story cutscene unchanged");
}
