#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "global.h"

#if __has_include("soh/Enhancements/cosmetics/NativeNpcModels.h")
#include "soh/Enhancements/cosmetics/NativeNpcModels.h"

static unsigned sFlags;
bool ResourceMgr_IsAltAssetsEnabled(void) {
    return (sFlags & 1) != 0;
}
uint8_t ResourceMgr_FileExists(const char* path) {
    return (sFlags & 2) && strstr(path, "/Skel") != NULL;
}
uint8_t ResourceMgr_FileAltExists(const char* path) {
    if (strstr(path, "/Skel") != NULL)
        return (sFlags & 4) != 0;
    if (strstr(path, "/Body/") != NULL)
        return (sFlags & 8) != 0;
    if (strstr(path, "/Faces/") != NULL)
        return (sFlags & 16) != 0;
    return (sFlags & 32) != 0;
}

int main(void) {
    const NativeNpcModel* models[] = { &sSariaNpcModel, &sKokiriGirlNpcModel, &sChildMalonNpcModel,
                                       &sAdultMalonNpcModel };
    const char* original = "__OTR__native/Skel";
    Gfx head[1];
    unsigned checks = 0;
    for (sFlags = 0; sFlags < 64; ++sFlags) {
        bool present = (sFlags & 14) == 14;
        bool active = present && (sFlags & 1);
        for (unsigned i = 0; i < 4; ++i) {
            const NativeNpcModel* model = models[i];
            assert(NativeNpcModel_IsPresent(model) == present);
            assert(NativeNpcModel_IsActive(model) == active);
            assert((NativeNpcModel_SelectSkeleton(model, original) != original) == present);
            for (int eye = -1; eye <= 6; ++eye) {
                for (int mouth = -1; mouth <= 6; ++mouth) {
                    Gfx* result = NativeNpcModel_SelectHead(model, head, eye, mouth);
                    assert((result != head) == active);
                    if (active)
                        assert((uintptr_t)result % 2 == 0);
                    ++checks;
                }
            }
        }
        if (active && (sFlags & 16)) {
            assert(strstr((const char*)NativeNpcModel_SelectEyes(&sSariaNpcModel, head, 1, 2, 0), "L1R2M0") != NULL);
        }
        for (int type = 0; type < 256; ++type) {
            bool girl = type == 1 || type == 5 || type == 6 || type == 9 || type == 10;
            assert((NativeNpcModel_Kokiri(type) != NULL) == girl);
            ++checks;
        }
        assert((NativeNpcModel_SariaOcarina(head) != head) == (active && (sFlags & 32)));
    }
    assert(NativeNpcModel_SelectSkeleton(NULL, original) == original);
    assert(NativeNpcModel_SelectHead(NULL, head, 0, 0) == head);
    printf("PASS %u model routing cases, all Kokiri variants, partial packs, faces, props and live Alt state.\n",
           checks);
    return 0;
}
#else
int main(void) {
    assert(!"Native NPC model routing is missing");
}
#endif
