#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_set>
using u8 = uint8_t;
static const char* forced;
static bool available = true;
static int rawLoads;
static char rawPixels[32];
const char* O2rLoader_GetForcedName() { return forced; }
u8 ResourceMgr_FileExists(const char*) { return available; }
u8 ResourceMgr_FileAltExists(const char*) { return false; }
char* ResourceMgr_LoadTexOrDListByName(const char*) { ++rawLoads; return rawPixels; }
static const char kOtrPrefix[] = "__OTR__";
static const char kVanillaObjects[] = "objects/object_link_";
/* PRODUCTION_FACE_RESOLVER */

int main() {
    const char* symbols[] = { "__OTR__objects/object_link_boy/gLinkAdultEyesOpenTex",
                             "__OTR__objects/object_link_boy/gLinkAdultEyesHalfTex",
                             "__OTR__objects/object_link_child/gLinkChildEyesClosedfTex" };
    assert(!CustomForms_ResolveVanillaTexture(symbols[0]) && !CustomForms_PreferFaceTextures());
    forced = "gerudo";
    assert(CustomForms_PreferFaceTextures());
    void* stable = CustomForms_ResolveVanillaTexture(symbols[0]);
    assert(!std::strcmp(static_cast<const char*>(stable),
                        "__OTR__objects/forms/gerudo/object_link_boy/gLinkAdultEyesOpenTex"));
    for (const auto* symbol : symbols) {
        const char* path = static_cast<const char*>(CustomForms_ResolveVanillaTexture(symbol));
        assert(path && std::strstr(path, "__OTR__objects/forms/gerudo/object_link_"));
    }
    assert(stable == CustomForms_ResolveVanillaTexture(symbols[0]) && rawLoads == 0);
    assert(!CustomForms_ResolveVanillaTexture(nullptr));
    assert(!CustomForms_ResolveVanillaTexture("__OTR__objects/object_geldb/gGerudoRedEyeOpenTex"));
    available = false;
    assert(!CustomForms_ResolveVanillaTexture(symbols[0]));
    forced = "kafei";
    assert(!CustomForms_PreferFaceTextures() && CustomForms_ResolveVanillaTexture(symbols[0]) == rawPixels);
    assert(rawLoads == 1);
    puts("PASS actual OoT face resolver: both ages, stable HD resource paths, missing-asset fallback and other-form precedence");
}
