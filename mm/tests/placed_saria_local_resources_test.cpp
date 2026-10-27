#include <fast/lus_gbi.h>
#undef GIMMCMD
#include "2s2h/Enhancements/Gameplay/MmPlacedSariaResources.h"
#include <cstdlib>
#include <unordered_map>
#include <stdexcept>
#include "2s2h/resource/type/Array.h"
#include <limits>
#include <cmath>
#include "tests/PlacedSariaABITest.h"
#define CHECK(x)                                                      \
    do {                                                              \
        if (!(x)) {                                                   \
            std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); \
            std::exit(1);                                             \
        }                                                             \
    } while (0)
using MmPlacedSaria::Resource;
using Store = std::unordered_map<std::string, Resource>;

#ifdef MM_SARIA_PACK_TEST
#include "2s2h/resource/importer/SkeletonFactory.h"
#include "2s2h/resource/importer/SkeletonLimbFactory.h"
#include "2s2h/resource/importer/ArrayFactory.h"
#include <ship/utils/binarytools/MemoryStream.h>
#include <filesystem>
#include <fstream>
static std::filesystem::path archiveRoot;
static Store cache;
static bool activeAlt;
static std::filesystem::path DiskPath(std::string path) {
    for (auto& c : path)
        if (c == '/')
            c = '!';
    return archiveRoot / path;
}
Resource FactoryLoad(const std::string& requested, bool exact = false);
std::shared_ptr<Ship::IResource> GetResourceByName(const char* path, bool exact) {
    CHECK(exact);
    return FactoryLoad(path, true);
}
extern "C" bool ResourceMgr_IsAltAssetsEnabled() {
    return activeAlt;
}
Resource FactoryLoad(const std::string& requested, bool exact) {
    std::string path = requested;
    if (path.starts_with("__OTR__"))
        path.erase(0, 7);
    if (!exact && activeAlt && !path.starts_with("alt/") && std::filesystem::exists(DiskPath("alt/" + path)))
        path = "alt/" + path;
    if (cache.contains(path))
        return cache.at(path);
    std::ifstream input(DiskPath(path), std::ios::binary);
    if (!input)
        return nullptr;
    auto bytes =
        std::make_shared<std::vector<char>>(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    CHECK(bytes->size() >= 64);
    auto metadata = std::make_shared<Ship::ResourceInitData>();
    metadata->Format = RESOURCE_FORMAT_BINARY;
    metadata->Path = path;
    auto file = std::make_shared<Ship::File>();
    auto reader = std::make_shared<Ship::BinaryReader>(std::make_shared<Ship::MemoryStream>(bytes, 64));
    reader->SetEndianness(Ship::Endianness::Little);
    file->Reader = reader;
    std::string type(bytes->data() + 4, 4);
    Resource result;
    if (type == "LKSO")
        result = SOH::ResourceFactoryBinarySkeletonV0().ReadResource(file, metadata);
    else if (type == "BLSO")
        result = SOH::ResourceFactoryBinarySkeletonLimbV0().ReadResource(file, metadata);
    else if (type == "RRAO")
        result = SOH::ResourceFactoryBinaryArrayV0().ReadResource(file, metadata);
    else if (type == "MNAO") {
        auto animation = std::make_shared<SOH::Animation>();
        animation->type = static_cast<SOH::AnimationType>(reader->ReadUInt32());
        CHECK(animation->type == SOH::AnimationType::Normal);
        auto& header = animation->animationData.animationHeader;
        header.common.frameCount = reader->ReadInt16();
        unsigned count = reader->ReadUInt32();
        while (count--)
            animation->rotationValues.push_back(reader->ReadUInt16());
        count = reader->ReadUInt32();
        while (count--) {
            auto x = reader->ReadUInt16(), y = reader->ReadUInt16(), z = reader->ReadUInt16();
            animation->rotationIndices.emplace_back(x, y, z);
        }
        header.frameData = reinterpret_cast<s16*>(animation->rotationValues.data());
        header.jointIndices = reinterpret_cast<SOH::JointIndex*>(animation->rotationIndices.data());
        header.staticIndexMax = reader->ReadInt16();
        result = animation;
    } else if (type == "XETO") {
        auto texture = std::make_shared<Fast::Texture>();
        texture->Type = static_cast<Fast::TextureType>(reader->ReadUInt32());
        texture->Width = reader->ReadUInt32();
        texture->Height = reader->ReadUInt32();
        texture->ImageDataSize = reader->ReadUInt32();
        CHECK(texture->ImageDataSize <= bytes->size() - 80);
        texture->ImageData = new uint8_t[texture->ImageDataSize];
        std::memcpy(texture->ImageData, bytes->data() + 80, texture->ImageDataSize);
        result = texture;
    } else if (type == "TLDO") {
        // Draw execution is outside this resource-ownership test. Preserve real
        // archive presence/size and use the real DisplayList owning class.
        auto display = std::make_shared<Fast::DisplayList>();
        display->Instructions.resize((bytes->size() - 64) / sizeof(Gfx));
        result = display;
    }
    if (result)
        cache[path] = result;
    return result;
}
int main(int argc, char** argv) {
    CHECK(argc == 2);
    archiveRoot = argv[1];
    for (unsigned cycle = 0; cycle < 4; ++cycle) {
        for (bool alt : { false, true }) {
            cache.clear();
            activeAlt = alt;
            for (unsigned pose : { 0u, 1u, 2u, 3u }) {
                ComboPlacedSariaResourcesV2 versioned{};
                auto& output = versioned.resources;
                CHECK(MM_PlacedSariaLoadV2(pose, &versioned, sizeof(versioned)));
                CHECK(output.custom == alt && output.altEnabled == alt);
                const bool scaled =
                    alt && std::filesystem::exists(DiskPath("alt/objects/object_sa/NativeNpcV1/DrawScale"));
                CHECK(std::abs(versioned.drawScale - (scaled ? 1.10f : 1.f)) < 1e-6f);
                if (alt) {
                    for (unsigned eye = 0; eye < 3; ++eye) {
                        auto suffix = "/Faces/L" + std::to_string(eye) + "R" + std::to_string(eye) + "M0";
                        CHECK(output.heads[eye] && std::strstr(output.heads[eye], suffix.c_str()));
                    }
                    auto rig =
                        std::dynamic_pointer_cast<SOH::Skeleton>(cache.at("alt/objects/object_sa/NativeNpcV1/Skel"));
                    CHECK(rig->limbTable[1] == "objects/object_sa/NativeNpcV1/Rig/Limb01");
                    CHECK(rig->skeletonHeaderSegments[1] == cache.at("alt/" + rig->limbTable[1])->GetRawPointer());
                }
                // Destroy all RM cache ownership while the snapshot remains usable.
                cache.clear();
                auto* animation = static_cast<SOH::AnimationHeader*>(output.animation);
                CHECK(animation->common.frameCount == (pose == 2 ? 65 : 24) && animation->jointIndices);
                auto* skeleton = static_cast<SOH::FlexSkeletonHeader*>(output.skeleton);
                CHECK(skeleton->sh.limbCount == 16 && skeleton->sh.segment[15]);
                if (pose == 2) {
                    auto* hand = static_cast<SOH::StandardLimb*>(skeleton->sh.segment[14]);
                    CHECK(std::strstr(reinterpret_cast<const char*>(hand->dList), "Ocarina"));
                }
                MM_PlacedSariaRelease(output.owner);
            }
        }
    }
    puts("PASS real Saria pack: production MM skeleton/limb factories, Alt-selected shared paths, four "
         "poses/ocarina/blink "
         "heads, repeated cache eviction");
}
/* PRODUCTION_MM_BINARY_FACTORIES */
#else
static Store rigAssets;
static Store* activeAssets;
static bool activeAlt, throwLoad;
std::shared_ptr<Ship::IResource> GetResourceByName(const char* path, bool loadExact) {
    CHECK(loadExact);
    if (throwLoad)
        throw std::runtime_error("missing archive");
    if (activeAssets) {
        auto found = activeAssets->find(path);
        if (found != activeAssets->end())
            return found->second;
    }
    auto found = rigAssets.find(path);
    return found == rigAssets.end() ? nullptr : found->second;
}
extern "C" bool ResourceMgr_IsAltAssetsEnabled() {
    return activeAlt;
}
static auto Child(const std::shared_ptr<SOH::Skeleton>& rig, unsigned i) {
    return std::static_pointer_cast<SOH::SkeletonLimb>(rigAssets.at(rig->limbTable[i]));
}
static auto Rig(const char* display) {
    static unsigned serial = 0;
    std::string prefix = "rig" + std::to_string(++serial) + "/";
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
        limb->dListPtr = i ? display : "";
        limb->limbData.standardLimb.dList = i ? reinterpret_cast<Gfx*>(limb->dListPtr.data()) : nullptr;
        s->limbTable.push_back(prefix + "limb" + std::to_string(i));
        rigAssets[s->limbTable.back()] = limb;
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
    activeAssets = &assets;
    assets["objects/object_sa/gSariaSkel"] = Rig("__OTR__objects/object_sa/nativeBody");
    for (auto path : MmPlacedSaria::kAnimations)
        if (path)
            assets[path] = Animation();
    for (auto name : { "gSariaEyeOpenTex", "gSariaEyeHalfTex", "gSariaEyeClosedTex", "gSariaMouthClosed2Tex" })
        assets[std::string("objects/object_sa/") + name] = Texture();
    auto load = [&](const std::string& path) -> Resource {
        auto found = assets.find(path);
        if (found != assets.end())
            return found->second;
        auto rig = rigAssets.find(path);
        return rig == rigAssets.end() ? nullptr : rig->second;
    };
    assets["objects/object_sa/nativeBody"] = List();
    assets["objects/object_sa/customBody"] = List();
    PlacedSariaABITest::LegacyCaller oldCaller;
    CHECK(MM_PlacedSariaLoad(0, reinterpret_cast<ComboPlacedSariaResources*>(&oldCaller.output)));
    MM_PlacedSariaRelease(oldCaller.output.owner);
    CHECK(oldCaller.Intact());
    CHECK(PlacedSariaABITest::CheckV2(MM_PlacedSariaLoadV2, MM_PlacedSariaRelease, 1.f));
    auto native = MmPlacedSaria::Build(load, false, 0);
    CHECK(native && !native->result.custom && !native->result.altEnabled);
    CHECK(std::strstr(reinterpret_cast<const char*>(native->limbs[1].dList), "__OTR__@mm:") != nullptr);
    CHECK(MmPlacedSaria::Build(load, true, 15));
    auto seated = Animation();
    assets["objects/object_sa/gSariaSitting3Anim"] = seated;
    auto sitting = MmPlacedSaria::Build(load, false, 3);
    CHECK(sitting && sitting->result.animation == seated->GetRawPointer());
    auto playing = Animation(65);
    assets["objects/object_sa/gSariaPlayingOcarinaAnim"] = playing;
    assets["objects/object_sa/gSariaRightHandAndOcarinaDL"] = List();
    auto performer = MmPlacedSaria::Build(load, false, 2);
    CHECK(performer && performer->result.animation == playing->GetRawPointer());
    CHECK(std::strstr(reinterpret_cast<const char*>(performer->limbs[14].dList),
                      "__OTR__@mm:objects/object_sa/gSariaRightHandAndOcarinaDL"));
    assets.erase("objects/object_sa/gSariaRightHandAndOcarinaDL");
    CHECK(!MmPlacedSaria::Build(load, false, 2) && MmPlacedSaria::Build(load, false, 1));
    assets["objects/object_sa/gSariaRightHandAndOcarinaDL"] = List();
    const std::string ns = MmPlacedSaria::kNamespace, alt = "alt/" + ns;
    assets[ns + "/Skel"] = Rig("__OTR__objects/object_sa/nativeBody");
    assets[alt + "/Skel"] = Rig("__OTR__objects/object_sa/customBody");
    assets[alt + "/Body/Limb15"] = List();
    for (unsigned eye = 0; eye < 3; ++eye)
        assets[alt + "/Faces/L" + std::to_string(eye) + "R" + std::to_string(eye) + "M0"] = List();
    const std::string clip = alt + "/PoseAnimations/" + MmPlacedSaria::kAnimations[0];
    auto fitted = Animation();
    assets[clip] = fitted;
    const std::string seatedClip = alt + "/PoseAnimations/" + MmPlacedSaria::kAnimations[3];
    auto fittedSeated = Animation();
    assets[seatedClip] = fittedSeated;
    CHECK(MmPlacedSaria::Build(load, true, 3)->result.animation == fittedSeated->GetRawPointer());
    const std::string playingClip = alt + "/PoseAnimations/objects/object_sa/gSariaPlayingOcarinaAnim";
    auto fittedPlaying = Animation(65);
    assets[playingClip] = fittedPlaying;
    assets[alt + "/Props/RightHandOcarina"] = List();
    auto customPerformer = MmPlacedSaria::Build(load, true, 2);
    CHECK(customPerformer && customPerformer->result.animation == fittedPlaying->GetRawPointer());
    CHECK(std::strstr(reinterpret_cast<const char*>(customPerformer->limbs[14].dList), "/Props/RightHandOcarina"));
    assets[clip + "/Limbs/Limb12"] = List();
    auto custom = MmPlacedSaria::Build(load, true, 0);
    CHECK(custom && custom->result.custom && custom->result.animation == fitted->GetRawPointer());
    CHECK(custom->drawScale == 1.f);
    const std::string scalePath = alt + "/DrawScale";
    assets[scalePath] = DrawScale(1.10f);
    activeAlt = true;
    CHECK(PlacedSariaABITest::CheckV2(MM_PlacedSariaLoadV2, MM_PlacedSariaRelease, 1.10f));
    activeAlt = false;
    for (unsigned pose : { 0u, 1u, 2u, 3u }) {
        auto scaled = MmPlacedSaria::Build(load, true, pose);
        CHECK(scaled && std::abs(scaled->drawScale - 1.10f) < 1e-6f);
        CHECK(MmPlacedSaria::Build(load, false, pose)->drawScale == 1.f);
    }
    for (float invalid :
         { 0.f, -1.f, .49f, 2.01f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN() }) {
        assets[scalePath] = DrawScale(invalid);
        CHECK(MmPlacedSaria::Build(load, true, 0)->drawScale == 1.f);
    }
    assets[scalePath] = List();
    CHECK(MmPlacedSaria::Build(load, true, 0)->drawScale == 1.f);
    assets[scalePath] = DrawScale(1.10f);
    CHECK(custom->result.poseLimbs[13] && custom->result.heads[0] != custom->result.heads[2]);
    for (unsigned pose : { 0u, 1u, 3u }) {
        auto blink = MmPlacedSaria::Build(load, true, pose);
        CHECK(blink && blink->result.custom);
        for (unsigned eye = 0; eye < 3; ++eye) {
            const std::string expected = "/Faces/L" + std::to_string(eye) + "R" + std::to_string(eye) + "M0";
            CHECK(blink->result.heads[eye] && std::strstr(blink->result.heads[eye], expected.c_str()));
        }
        CHECK(!blink->result.poseLimbs[16]);
    }
    CHECK(!MmPlacedSaria::Build(load, false, 0)->result.custom);
    fitted->rotationValues[0] = 999;
    CHECK(MmPlacedSaria::Build(load, true, 0)->result.animation != fitted->GetRawPointer());
    assets[ns + "/Skel"] = Texture();
    CHECK(!MmPlacedSaria::Build(load, true, 0)->result.custom);
    CHECK(MmPlacedSaria::Build(load, true, 0)->drawScale == 1.f);
    assets[ns + "/Skel"] = assets["objects/object_sa/gSariaSkel"];
    auto malformed = Rig("__OTR__objects/object_sa/customBody");
    Child(malformed, 0)->limbData.standardLimb.child = 2;
    Child(malformed, 2)->limbData.standardLimb.child = 1;
    Child(malformed, 1)->limbData.standardLimb.child = 3;
    assets[alt + "/Skel"] = malformed;
    CHECK(!MmPlacedSaria::Build(load, true, 0)->result.custom);
    assets.erase("objects/object_sa/gSariaEyeHalfTex");
    CHECK(!MmPlacedSaria::Build(load, false, 0));
    // The ABI wrapper borrows only native C headers and releases its own snapshot.
    ComboPlacedSariaResources output{};
    CHECK(!MM_PlacedSariaLoad(0, &output) && !output.owner);
    assets["objects/object_sa/gSariaEyeHalfTex"] = Texture();
    CHECK(!MM_PlacedSariaLoad(0, nullptr));
    throwLoad = true;
    CHECK(!MM_PlacedSariaLoad(0, &output) && !output.owner);
    ComboPlacedSariaResourcesV2 failed{};
    failed.drawScale = 123.f;
    CHECK(!MM_PlacedSariaLoadV2(0, &failed, sizeof(failed)));
    CHECK(!failed.resources.owner && failed.drawScale == 1.f);
    throwLoad = false;
    for (unsigned iteration = 0; iteration < 6; ++iteration) {
        activeAlt = iteration % 2;
        CHECK(MM_PlacedSariaLoad(iteration % 2 ? 3 : 0, &output));
        CHECK(output.altEnabled == activeAlt);
        MM_PlacedSariaRelease(output.owner);
    }
    // Simulate the factory's borrowed limb pointer surviving an RM cache replacement.
    auto originalChild = rigAssets.at("rig1/limb1");
    rigAssets["rig1/limb1"] = std::make_shared<SOH::SkeletonLimb>();
    CHECK(!MmPlacedSaria::Build(load, false, 0));
    rigAssets["rig1/limb1"] = originalChild;
    originalChild.reset();
    activeAlt = true;
    CHECK(MM_PlacedSariaAlt() && MM_PlacedSariaLoad(1, &output));
    CHECK(output.owner && output.altEnabled && !output.custom);
    auto* animation = static_cast<SOH::AnimationHeader*>(native->result.animation);
    std::weak_ptr<Ship::IResource> heldLimb = rigAssets.at("rig1/limb1");
    rigAssets.clear();
    assets.clear();
    CHECK(!heldLimb.expired());
    CHECK(animation->common.frameCount == 24 && animation->frameData[0] == 1);
    CHECK(native->skeleton.sh.segment[1] == &native->limbs[1]);
    native.reset();
    sitting.reset();
    custom.reset();
    performer.reset();
    customPerformer.reset();
    CHECK(!heldLimb.expired());
    CHECK(static_cast<SOH::AnimationHeader*>(output.animation)->common.frameCount == 24);
    MM_PlacedSariaRelease(output.owner);
    MM_PlacedSariaRelease(nullptr);
    CHECK(heldLimb.expired());
    puts("PASS MM Saria resource snapshots: native/Alt, fitted clips/limbs/faces, malformed pack fallback, root "
         "protection, missing donors, retained lifetime");
    puts("PASS MM exports: frozen V1 caller canary, V2 short-size/null rejection before writes and bounded successful "
         "output.");
}

#endif // MM_SARIA_PACK_TEST
