#include <fast/lus_gbi.h>
#undef GIMMCMD
#include "soh/Enhancements/cosmetics/PlacedSariaResources.h"
#include <cstdlib>
#include <unordered_map>
#include "soh/resource/type/Array.h"
#include <limits>
#include <cmath>
#include "tests/PlacedSariaABITest.h"
#include "ComboExport.h"
#define CHECK(x)                                                      \
    do {                                                              \
        if (!(x)) {                                                   \
            std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); \
            std::exit(1);                                             \
        }                                                             \
    } while (0)
using PlacedSaria::Resource;
using Store = std::unordered_map<std::string, Resource>;
// Controlled archive/RM boundary; the injected production exports below use
// the real snapshot builder and retain real resource objects.
static Store* providerAssets;
static bool providerAlt;
namespace Ship {
class TestPlacedSariaOwner {
  public:
    bool IsAltAssetsEnabled() {
        return providerAlt;
    }
    PlacedSaria::Resource LoadResource(const std::string& path, bool exact) {
        CHECK(exact);
        auto found = providerAssets->find(path);
        return found == providerAssets->end() ? nullptr : found->second;
    }
};
class CrossRMRegistry {
  public:
    static auto Get(const std::string& name) {
        CHECK(name == "oot");
        static auto owner = std::make_shared<TestPlacedSariaOwner>();
        return owner;
    }
};
class OwnRMScope {
  public:
    explicit OwnRMScope(const char* name) {
        CHECK(std::strcmp(name, "oot") == 0);
    }
};
} // namespace Ship

/* PRODUCTION_PLACED_SARIA_PROVIDER */

static auto Rig(const char* display) {
    auto s = std::make_shared<SOH::Skeleton>();
    s->type = SOH::SkeletonType::Flex;
    s->limbType = s->limbTableType = SOH::LimbType::Standard;
    s->limbCount = s->limbTableCount = 16;
    s->dListCount = 15;
    for (unsigned i = 0; i < 16; ++i) {
        auto limb = std::make_shared<SOH::SkeletonLimb>();
        limb->limbType = SOH::LimbType::Standard;
        limb->limbData.standardLimb = {};
        limb->limbData.standardLimb.child = i == 15 ? 255 : i + 1;
        limb->limbData.standardLimb.sibling = 255;
        limb->limbData.standardLimb.dList = i ? reinterpret_cast<Gfx*>(const_cast<char*>(display)) : nullptr;
        s->limbTable.push_back("limb" + std::to_string(i));
        s->limbResources.push_back(limb);
        s->skeletonHeaderSegments.push_back(limb->GetRawPointer());
    }
    auto& h = s->skeletonData.flexSkeletonHeader;
    h.sh.limbCount = 16;
    h.sh.skeletonType = 1;
    h.sh.segment = s->skeletonHeaderSegments.data();
    h.dListCount = 15;
    return s;
}
static auto Animation(int frames = 24) {
    auto a = std::make_shared<SOH::Animation>();
    a->type = SOH::AnimationType::Normal;
    a->rotationValues = { 1, 2, 3 };
    for (unsigned i = 0; i < 17; ++i)
        a->rotationIndices.emplace_back(0, 1, 2);
    auto& h = a->animationData.animationHeader;
    h.common.frameCount = frames;
    h.frameData = reinterpret_cast<s16*>(a->rotationValues.data());
    h.jointIndices = reinterpret_cast<decltype(h.jointIndices)>(a->rotationIndices.data());
    h.staticIndexMax = 3;
    return a;
}
static auto Texture() {
    auto t = std::make_shared<Fast::Texture>();
    t->Type = Fast::TextureType::RGBA16bpp;
    t->Width = t->Height = 16;
    t->ImageDataSize = 512;
    t->ImageData = new uint8_t[512]{};
    return t;
}
static auto List() {
    auto d = std::make_shared<Fast::DisplayList>();
    d->Instructions.resize(1);
    return d;
}
static auto DrawScale(float factor) {
    auto result = std::make_shared<SOH::Array>();
    result->ArrayType = SOH::ArrayResourceType::Scalar;
    result->ArrayScalarType = SOH::ScalarType::ZSCALAR_U16;
    uint32_t bits;
    std::memcpy(&bits, &factor, sizeof(bits));
    for (uint16_t word : { uint16_t(0x4e44), uint16_t(1), uint16_t(bits), uint16_t(bits >> 16) }) {
        SOH::ScalarData data{};
        data.u16 = word;
        result->Scalars.push_back(data);
    }
    result->ArrayCount = 4;
    return result;
}

int main() {
    Store assets;
    providerAssets = &assets;
    assets["objects/object_sa/gSariaSkel"] = Rig("__OTR__objects/object_sa/nativeBody");
    for (auto path : PlacedSaria::kAnimations)
        if (path)
            assets[path] = Animation();
    for (auto name : { "gSariaEyeOpenTex", "gSariaEyeHalfTex", "gSariaEyeClosedTex", "gSariaMouthClosed2Tex" })
        assets[std::string("objects/object_sa/") + name] = Texture();
    auto load = [&](const std::string& path) -> Resource {
        auto found = assets.find(path);
        return found == assets.end() ? nullptr : found->second;
    };
    auto native = PlacedSaria::Build(load, false, 0);
    PlacedSariaABITest::LegacyCaller oldCaller;
    CHECK(OOT_PlacedSariaLoad(0, reinterpret_cast<ComboPlacedSariaResources*>(&oldCaller.output)));
    OOT_PlacedSariaRelease(oldCaller.output.owner);
    CHECK(oldCaller.Intact());
    CHECK(PlacedSariaABITest::CheckV2(OOT_PlacedSariaLoadV2, OOT_PlacedSariaRelease, 1.f));
    CHECK(native && !native->result.custom && !native->result.altEnabled);
    CHECK(std::strstr(reinterpret_cast<const char*>(native->limbs[1].dList), "__OTR__@oot:") != nullptr);
    CHECK(PlacedSaria::Build(load, true, 15));
    auto seated = Animation();
    assets["objects/object_sa/gSariaSitting3Anim"] = seated;
    auto sitting = PlacedSaria::Build(load, false, 3);
    CHECK(sitting && sitting->result.animation == seated->GetRawPointer());
    auto playing = Animation(65);
    assets["objects/object_sa/gSariaPlayingOcarinaAnim"] = playing;
    assets["objects/object_sa/gSariaRightHandAndOcarinaDL"] = List();
    auto performer = PlacedSaria::Build(load, false, 2);
    CHECK(performer && performer->result.animation == playing->GetRawPointer());
    CHECK(std::strstr(reinterpret_cast<const char*>(performer->limbs[14].dList),
                      "__OTR__@oot:objects/object_sa/gSariaRightHandAndOcarinaDL"));
    assets.erase("objects/object_sa/gSariaRightHandAndOcarinaDL");
    CHECK(!PlacedSaria::Build(load, false, 2) && PlacedSaria::Build(load, false, 1));
    assets["objects/object_sa/gSariaRightHandAndOcarinaDL"] = List();
    const std::string ns = PlacedSaria::kNamespace, alt = "alt/" + ns;
    assets[ns + "/Skel"] = Rig("__OTR__objects/object_sa/nativeBody");
    assets[alt + "/Skel"] = Rig("__OTR__objects/object_sa/customBody");
    assets[alt + "/Body/Limb15"] = List();
    for (unsigned eye = 0; eye < 3; ++eye)
        assets[alt + "/Faces/L" + std::to_string(eye) + "R" + std::to_string(eye) + "M0"] = List();
    const std::string clip = alt + "/PoseAnimations/" + PlacedSaria::kAnimations[0];
    auto fitted = Animation();
    assets[clip] = fitted;
    const std::string seatedClip = alt + "/PoseAnimations/" + PlacedSaria::kAnimations[3];
    auto fittedSeated = Animation();
    assets[seatedClip] = fittedSeated;
    CHECK(PlacedSaria::Build(load, true, 3)->result.animation == fittedSeated->GetRawPointer());
    const std::string playingClip = alt + "/PoseAnimations/objects/object_sa/gSariaPlayingOcarinaAnim";
    auto fittedPlaying = Animation(65);
    assets[playingClip] = fittedPlaying;
    assets[alt + "/Props/RightHandOcarina"] = List();
    auto customPerformer = PlacedSaria::Build(load, true, 2);
    CHECK(customPerformer && customPerformer->result.animation == fittedPlaying->GetRawPointer());
    CHECK(std::strstr(reinterpret_cast<const char*>(customPerformer->limbs[14].dList), "/Props/RightHandOcarina"));
    assets[clip + "/Limbs/Limb12"] = List();
    auto custom = PlacedSaria::Build(load, true, 0);
    CHECK(custom && custom->result.custom && custom->result.animation == fitted->GetRawPointer());
    CHECK(custom->drawScale == 1.f);
    const std::string scalePath = alt + "/DrawScale";
    assets[scalePath] = DrawScale(1.10f);
    providerAlt = true;
    CHECK(PlacedSariaABITest::CheckV2(OOT_PlacedSariaLoadV2, OOT_PlacedSariaRelease, 1.10f));
    providerAlt = false;
    for (unsigned pose : { 0u, 1u, 2u, 3u }) {
        auto scaled = PlacedSaria::Build(load, true, pose);
        CHECK(scaled && std::abs(scaled->drawScale - 1.10f) < 1e-6f);
        CHECK(PlacedSaria::Build(load, false, pose)->drawScale == 1.f);
    }
    for (float invalid :
         { 0.f, -1.f, .49f, 2.01f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN() }) {
        assets[scalePath] = DrawScale(invalid);
        CHECK(PlacedSaria::Build(load, true, 0)->drawScale == 1.f);
    }
    assets[scalePath] = List();
    CHECK(PlacedSaria::Build(load, true, 0)->drawScale == 1.f);
    assets[scalePath] = DrawScale(1.10f);
    CHECK(custom->result.poseLimbs[13] && custom->result.heads[0] != custom->result.heads[2]);
    for (unsigned pose : { 0u, 1u, 3u }) {
        auto blink = PlacedSaria::Build(load, true, pose);
        CHECK(blink && blink->result.custom);
        for (unsigned eye = 0; eye < 3; ++eye) {
            const std::string expected = "/Faces/L" + std::to_string(eye) + "R" + std::to_string(eye) + "M0";
            CHECK(blink->result.heads[eye] && std::strstr(blink->result.heads[eye], expected.c_str()));
        }
        CHECK(!blink->result.poseLimbs[16]);
    }
    CHECK(!PlacedSaria::Build(load, false, 0)->result.custom);
    fitted->rotationValues[0] = 999;
    CHECK(PlacedSaria::Build(load, true, 0)->result.animation != fitted->GetRawPointer());
    assets[ns + "/Skel"] = Texture();
    CHECK(!PlacedSaria::Build(load, true, 0)->result.custom);
    CHECK(PlacedSaria::Build(load, true, 0)->drawScale == 1.f);
    assets[ns + "/Skel"] = assets["objects/object_sa/gSariaSkel"];
    auto malformed = Rig("__OTR__objects/object_sa/customBody");
    std::static_pointer_cast<SOH::SkeletonLimb>(malformed->limbResources[0])->limbData.standardLimb.child = 2;
    std::static_pointer_cast<SOH::SkeletonLimb>(malformed->limbResources[2])->limbData.standardLimb.child = 1;
    std::static_pointer_cast<SOH::SkeletonLimb>(malformed->limbResources[1])->limbData.standardLimb.child = 3;
    assets[alt + "/Skel"] = malformed;
    CHECK(!PlacedSaria::Build(load, true, 0)->result.custom);
    assets.erase("objects/object_sa/gSariaEyeHalfTex");
    CHECK(!PlacedSaria::Build(load, false, 0));
    ComboPlacedSariaResourcesV2 failed{};
    failed.drawScale = 123.f;
    CHECK(!OOT_PlacedSariaLoadV2(0, &failed, sizeof(failed)));
    CHECK(!failed.resources.owner && failed.drawScale == 1.f);
    auto* animation = static_cast<AnimationHeader*>(native->result.animation);
    assets.clear();
    CHECK(animation->common.frameCount == 24 && animation->frameData[0] == 1);
    CHECK(native->skeleton.sh.segment[1] == &native->limbs[1]);
    puts("PASS Saria resource snapshots: native/Alt, fitted clips/limbs/faces, malformed pack fallback, root "
         "protection, missing donors, retained lifetime");
    puts("PASS OoT exports: frozen V1 caller canary, V2 short-size/null rejection before writes and bounded successful "
         "output.");
}
