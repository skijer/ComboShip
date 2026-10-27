#include <stdio.h>
#include <stdlib.h>
#include "overlays/actors/ovl_En_Viewer/z_en_viewer.h"

static unsigned initialized, destroyed, updated, drawn;
static u8 D_8089F3E0, D_8089F3E4;
u32 D_8089F4D0;
void func_8089F17C(EnViewer* a, PlayState* p) {
}
void func_8089F218(EnViewer* a, PlayState* p) {
}
void func_8089F2C4(EnViewer* a, PlayState* p) {
}
int ComboSaria_IsParam(s16 p) {
    return ((u16)p & 0xFF0F) == 0x7F03 || p == 0x7E33;
}
void ComboSaria_Init(Actor* a, PlayState* p) {
    ++initialized;
}
void ComboSaria_Destroy(Actor* a, PlayState* p) {
    ++destroyed;
}
void ComboSaria_Update(Actor* a, PlayState* p) {
    ++updated;
}
void ComboSaria_Draw(Actor* a, PlayState* p) {
    ++drawn;
}

/* PRODUCTION_VIEWER_FUNCTIONS */

#define CHECK(x)                              \
    do {                                      \
        if (!(x)) {                           \
            fprintf(stderr, "FAIL %s\n", #x); \
            exit(1);                          \
        }                                     \
    } while (0)
int main(void) {
    static PlayState play;
    EnViewer saria = { 0 }, native = { 0 };
    saria.actor.params = 0x7F03;
    EnViewer_Init(&saria.actor, &play);
    CHECK(initialized == 1 && D_8089F3E0 == 0);
    EnViewer_Update(&saria.actor, &play);
    EnViewer_Draw(&saria.actor, &play);
    EnViewer_Destroy(&saria.actor, &play);
    CHECK(updated == 1 && drawn == 1 && destroyed == 1 && D_8089F3E0 == 0);
    saria.actor.params = 0x7E33;
    EnViewer_Init(&saria.actor, &play);
    EnViewer_Update(&saria.actor, &play);
    EnViewer_Draw(&saria.actor, &play);
    EnViewer_Destroy(&saria.actor, &play);
    CHECK(initialized == 2 && updated == 2 && drawn == 2 && destroyed == 2 && D_8089F3E0 == 0);
    for (int type = 0; type < 3; ++type) {
        native.actor.params = type;
        EnViewer_Init(&native.actor, &play);
        CHECK(D_8089F3E0 == 1 && initialized == 2 && native.actionFunc != NULL);
        EnViewer_Update(&native.actor, &play);
        EnViewer_Destroy(&native.actor, &play);
        CHECK(D_8089F3E0 == 0 && destroyed == 2);
    }
    puts("PASS MM Saria viewer dispatch: reserved NPC lifetime bypasses native lighting counters; three lighting modes "
         "preserved");
    return 0;
}
