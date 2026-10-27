#include "../../combo/NeiAssetPriorityResource.h"
#include "../../combo/NeiWolfAssetResource.h"
#include "../../combo/menu/ComboIkanaShieldResources.h"
#include "../../combo/ComboGameMode.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <fast/interpreter.h>
#include <fast/resource/ResourceType.h>
#include <fast/resource/type/DisplayList.h>
#include <libultraship/bridge/resourcebridge.h>
#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include <stb_image.h>

#include "ResourceManagerHelpers.h"
#include "OTRGlobals.h"
#include "cvar_prefixes.h"
#include "Enhancements/enhancementTypes.h"
#include "Enhancements/Graphics/PreludeNativeMaterialScroll.h"
#include "Enhancements/randomizer/dungeon.h"
#include "soh/Enhancements/randomizer/SeedContext.h"
#include <soh/GameVersions.h>
#include "resource/type/SohResourceType.h"
#include "resource/type/Array.h"
#include "resource/type/Skeleton.h"
#include "resource/type/PlayerAnimation.h"
#include "mods/transformation_masks/assets/mm_asset_loader.h"

extern "C" {
#include "variables.h"
#include "z64.h"
#include "macros.h"
}

#ifdef COMBO_BUILD
#include "ComboExport.h"
// ComboShip: audio loads pinned to OOT's own RM — the audio thread races active-RM swaps
// (ResourceManagerScope) made on other threads.
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManagerScope.h>
#define COMBO_OWN_RM() (Ship::CrossRMRegistry::GetOrActive("oot"))
#else
#define COMBO_OWN_RM() (Ship::Context::GetRawInstance()->GetResourceManager())
#endif

#include <algorithm>
#include <cstring>
#include <map>
#include <string>
#include <vector>

extern "C" PlayState* gPlayState;

struct LinkTunicDListCacheKey {
    size_t operator()(const std::pair<std::string, const char*>& key) const {
        return std::hash<std::string>{}(key.first) ^ std::hash<const char*>{}(key.second);
    }
};

static const char* ResourceMgr_ResolveLinkTunicDListPath(const char* path) {
    if (path == nullptr) {
        return nullptr;
    }

    const char* originalPath = path;
    constexpr std::string_view adultPrefix = "__OTR__objects/object_link_boy/";
    constexpr std::string_view childPrefix = "__OTR__objects/object_link_child/";

    std::string_view objectPrefix;
    const char* objectFolder;

    if (std::string_view(originalPath).starts_with(adultPrefix)) {
        objectPrefix = adultPrefix;
        objectFolder = "object_link_boy";
    } else if (std::string_view(originalPath).starts_with(childPrefix)) {
        objectPrefix = childPrefix;
        objectFolder = "object_link_child";
    } else {
        return path;
    }

    const char* tunicSuffix = nullptr;
    switch (TUNIC_EQUIP_TO_PLAYER(CUR_EQUIP_VALUE(EQUIP_TYPE_TUNIC))) {
        case PLAYER_TUNIC_KOKIRI:
            tunicSuffix = "kokiri";
            break;
        case PLAYER_TUNIC_GORON:
            tunicSuffix = "goron";
            break;
        case PLAYER_TUNIC_ZORA:
            tunicSuffix = "zora";
            break;
        default:
            return path;
    }

    static std::unordered_map<std::pair<std::string, const char*>, std::string, LinkTunicDListCacheKey>
        sResolvedLinkTunicDListPaths;
    std::pair<std::string, const char*> cacheKey{ originalPath, tunicSuffix };
    if (auto it = sResolvedLinkTunicDListPaths.find(cacheKey); it != sResolvedLinkTunicDListPaths.end()) {
        return it->second.c_str();
    }

    const std::string candidate = spdlog::fmt_lib::format("__OTR__objects/{}_{}/{}", objectFolder, tunicSuffix,
                                                          originalPath + objectPrefix.size());

    if (!ResourceMgr_IsAltAssetsEnabled() || !ResourceMgr_FileAltExists(candidate.c_str()) ||
        !ResourceGetIsCustomByName(candidate.c_str())) {
        return path;
    }

    auto it = sResolvedLinkTunicDListPaths.emplace(std::move(cacheKey), candidate).first;
    return it->second.c_str();
}

extern "C" uint32_t ResourceMgr_GetNumGameVersions() {
    return static_cast<u32>(
        Ship::Context::GetRawInstance()->GetResourceManager()->GetArchiveManager()->GetGameVersions().size());
}

// Helper: returns true if the version is any recognized OOT (NOT mm.o2r / mods).
static bool IsOotVersion(uint32_t version) {
    switch (version) {
        case OOT_NTSC_US_10:
        case OOT_NTSC_US_11:
        case OOT_NTSC_US_12:
        case OOT_NTSC_JP_GC:
        case OOT_NTSC_JP_GC_CE:
        case OOT_NTSC_US_GC:
        case OOT_NTSC_JP_MQ:
        case OOT_NTSC_US_MQ:
        case OOT_PAL_10:
        case OOT_PAL_11:
        case OOT_PAL_GC:
        case OOT_PAL_MQ:
        case OOT_PAL_GC_DBG1:
        case OOT_PAL_GC_DBG2:
        case OOT_PAL_GC_MQ_DBG:
            return true;
        default:
            return false;
    }
}

// Returns the version of the index-th OOT archive, skipping non-OOT (mm.o2r, mods).
extern "C" uint32_t ResourceMgr_GetGameVersion(int index) {
    auto versions = Ship::Context::GetRawInstance()->GetResourceManager()->GetArchiveManager()->GetGameVersions();
    int ootIndex = 0;
    for (uint32_t version : versions) {
        if (IsOotVersion(version)) {
            if (ootIndex == index)
                return version;
            ootIndex++;
        }
    }
    return 0;
}

extern "C" uint32_t ResourceMgr_GetGamePlatform(int index) {
    uint32_t version = ResourceMgr_GetGameVersion(index);

    switch (version) {
        case OOT_NTSC_US_10:
        case OOT_NTSC_US_11:
        case OOT_NTSC_US_12:
        case OOT_PAL_10:
        case OOT_PAL_11:
            return GAME_PLATFORM_N64;
        case OOT_NTSC_JP_GC:
        case OOT_NTSC_JP_GC_CE:
        case OOT_NTSC_US_GC:
        case OOT_PAL_GC:
        case OOT_NTSC_JP_MQ:
        case OOT_NTSC_US_MQ:
        case OOT_PAL_MQ:
        case OOT_PAL_GC_DBG1:
        case OOT_PAL_GC_DBG2:
        case OOT_PAL_GC_MQ_DBG:
            return GAME_PLATFORM_GC;
    }
    // No OOT found at this index — default to N64 for backwards compat
    return GAME_PLATFORM_N64;
}

extern "C" uint32_t ResourceMgr_GetGameRegion(int index) {
    uint32_t version = ResourceMgr_GetGameVersion(index);

    switch (version) {
        case OOT_NTSC_US_10:
        case OOT_NTSC_US_11:
        case OOT_NTSC_US_12:
        case OOT_NTSC_JP_GC:
        case OOT_NTSC_JP_GC_CE:
        case OOT_NTSC_US_GC:
        case OOT_NTSC_JP_MQ:
        case OOT_NTSC_US_MQ:
            return GAME_REGION_NTSC;
        case OOT_PAL_10:
        case OOT_PAL_11:
        case OOT_PAL_GC:
        case OOT_PAL_MQ:
        case OOT_PAL_GC_DBG1:
        case OOT_PAL_GC_DBG2:
        case OOT_PAL_GC_MQ_DBG:
            return GAME_REGION_PAL;
    }
    return GAME_REGION_NTSC;
}

extern "C" char* _message_0xFFFC_nes;
extern "C" bool ResourceMgr_IsPalLoaded() {
    return _message_0xFFFC_nes != NULL;
}

u32 IsSceneMasterQuest(s16 sceneNum) {
    u8 mqMode = CVarGetInteger(CVAR_GENERAL("BetterDebugWarpScreenMQMode"), WARP_MODE_OVERRIDE_OFF);
    if (mqMode == WARP_MODE_OVERRIDE_MQ_AS_VANILLA) {
        return true;
    }

    if (mqMode == WARP_MODE_OVERRIDE_VANILLA_AS_MQ) {
        return false;
    }

    if (OTRGlobals::Instance->HasMasterQuest()) {
        if (!OTRGlobals::Instance->HasOriginal()) {
            return true;
        }

        if (IS_MASTER_QUEST) {
            return true;
        }

        if (IS_RANDO) {
            auto dungeon = OTRGlobals::Instance->gRandoContext->GetDungeonFromScene((SceneID)sceneNum);
            if (dungeon != nullptr && dungeon->IsMQ()) {
                return true;
            }
        }
    }

    return false;
}

extern "C" uint32_t ResourceMgr_GameHasMasterQuest() {
    return OTRGlobals::Instance->HasMasterQuest();
}

extern "C" uint32_t ResourceMgr_GameHasOriginal() {
    return OTRGlobals::Instance->HasOriginal();
}

extern "C" uint32_t ResourceMgr_IsSceneMasterQuest(s16 sceneNum) {
    return IsSceneMasterQuest(sceneNum);
}

extern "C" uint32_t ResourceMgr_IsGameMasterQuest() {
    return gPlayState != NULL ? IsSceneMasterQuest(gPlayState->sceneNum) : 0;
}

extern "C" void ResourceMgr_LoadDirectory(const char* resName) {
    Ship::Context::GetRawInstance()->GetResourceManager()->LoadResources(resName);
}

extern "C" void ResourceMgr_DirtyDirectory(const char* resName) {
    PreludeNativeMaterialScroll_InvalidateMetadata("dirty directory");
    Ship::Context::GetRawInstance()->GetResourceManager()->DirtyResources(resName);
}

extern "C" void ResourceMgr_UnloadResource(const char* resName) {
    std::string path = resName;
    if (path.substr(0, 7) == "__OTR__") {
        path = path.substr(7);
    }
    if (path == "prelude/project/edits.json") {
        PreludeNativeMaterialScroll_InvalidateMetadata("metadata resource unload");
    }
    auto res = Ship::Context::GetRawInstance()->GetResourceManager()->UnloadResource(path);
}

// OTRTODO: There is probably a more elegant way to go about this...
// Caller must free each string and the array itself when done.
extern "C" char** ResourceMgr_ListFiles(const char* searchMask, int* resultSize) {
    auto lst = Ship::Context::GetRawInstance()->GetResourceManager()->GetArchiveManager()->ListFiles(searchMask);
    char** result = (char**)malloc(lst->size() * sizeof(char*));

    for (size_t i = 0; i < lst->size(); i++) {
        char* str = (char*)malloc(lst.get()[0][i].size() + 1);
        memcpy(str, lst.get()[0][i].data(), lst.get()[0][i].size());
        str[lst.get()[0][i].size()] = '\0';
        result[i] = str;
    }
    *resultSize = static_cast<int>(lst->size());

    return result;
}

extern "C" uint8_t ResourceMgr_FileExists(const char* filePath) {
    std::string path = filePath;
    if (path.substr(0, 7) == "__OTR__") {
        path = path.substr(7);
    }

    return ExtensionCache.contains(path);
}

extern "C" uint8_t ResourceMgr_FileAltExists(const char* filePath) {
    std::string path = filePath;
    if (path.substr(0, 7) == "__OTR__") {
        path = path.substr(7);
    }

    if (path.substr(0, 4) != "alt/") {
        path = "alt/" + path;
    }

    return ExtensionCache.contains(path);
}

extern "C" bool ResourceMgr_IsAltAssetsEnabled() {
    return Ship::Context::GetRawInstance()->GetResourceManager()->IsAltAssetsEnabled();
}

// Unloads a resource if an alternate version exists when alt assets are enabled
// The resource is only removed from the internal cache to prevent it from used in the next resource lookup
extern "C" void ResourceMgr_UnloadOriginalWhenAltExists(const char* resName) {
    if (ResourceMgr_IsAltAssetsEnabled() && ResourceMgr_FileAltExists((char*)resName)) {
        ResourceMgr_UnloadResource((char*)resName);
    }
}

static void ResourceMgr_PreloadAltWhenItExists(const char* resName) {
    if (!ResourceMgr_IsAltAssetsEnabled() || !ResourceMgr_FileAltExists(resName)) {
        return;
    }
    std::string path = resName;
    if (path.starts_with("__OTR__")) {
        path = path.substr(7);
    }
    if (!path.starts_with(Ship::IResource::gAltAssetPrefix)) {
        path = Ship::IResource::gAltAssetPrefix + path;
    }
    // Preload the exact Alt entry without evicting a native resource still used by submitted frames.
    Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(path, true);
}

std::shared_ptr<Ship::IResource> ResourceMgr_GetResourceByNameHandlingMQ(const char* path) {
    std::string Path = path;
    if (ResourceMgr_IsGameMasterQuest()) {
        size_t pos = 0;
        if ((pos = Path.find("/nonmq/", 0)) != std::string::npos) {
            Path.replace(pos, 7, "/mq/");
        }
    }
    return Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(Path.c_str());
}

extern "C" char* ResourceMgr_GetResourceDataByNameHandlingMQ(const char* path) {
    auto res = ResourceMgr_GetResourceByNameHandlingMQ(path);

    if (res == nullptr) {
        return nullptr;
    }

    return (char*)res->GetRawPointer();
}

extern "C" uint8_t ResourceMgr_TexIsRaw(const char* texPath) {
    auto res = std::static_pointer_cast<Fast::Texture>(ResourceMgr_GetResourceByNameHandlingMQ(texPath));
    return res->Flags & TEX_FLAG_LOAD_AS_RAW;
}

extern "C" uint8_t ResourceMgr_ResourceIsBackground(char* texPath) {
    auto res = ResourceMgr_GetResourceByNameHandlingMQ(texPath);
    return res->GetInitData()->Type == static_cast<uint32_t>(SOH::ResourceType::SOH_Background);
}

extern "C" char* ResourceMgr_LoadJPEG(char* data, size_t dataSize) {
    static char* finalBuffer = 0;

    if (finalBuffer == 0) {
        finalBuffer = (char*)malloc(dataSize);
    }

    int w;
    int h;
    int comp;

    unsigned char* pixels =
        stbi_load_from_memory((const unsigned char*)data, 320 * 240 * 2, &w, &h, &comp, STBI_rgb_alpha);
    // unsigned char* pixels = stbi_load_from_memory((const unsigned char*)data, 480 * 240 * 2, &w, &h, &comp,
    // STBI_rgb_alpha);
    int idx = 0;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            uint16_t* bufferTest = (uint16_t*)finalBuffer;
            int pixelIdx = ((y * w) + x) * 4;

            uint8_t r = pixels[pixelIdx + 0] / 8;
            uint8_t g = pixels[pixelIdx + 1] / 8;
            uint8_t b = pixels[pixelIdx + 2] / 8;

            uint8_t alphaBit = pixels[pixelIdx + 3] != 0;

            uint16_t data = (r << 11) + (g << 6) + (b << 1) + alphaBit;

            finalBuffer[idx++] = (data & 0xFF00) >> 8;
            finalBuffer[idx++] = (data & 0x00FF);
        }
    }

    return (char*)finalBuffer;
}

extern "C" char* ResourceMgr_LoadTexOrDListByName(const char* filePath) {
    if (filePath == nullptr) {
        return nullptr;
    }

    std::shared_ptr<Ship::IResource> res;
#ifdef COMBO_BUILD
    // Pause-menu preflight must resolve the same owner as the renderer. A
    // routed Ikana icon is not an OoT archive name, even when MM owns it and
    // the following SETTIMG command could draw it correctly.
    if (std::strncmp(filePath, "__OTR__@", 8) == 0) {
        const char* gameStart = filePath + 8;
        const char* colon = std::strchr(gameStart, ':');
        if (colon == nullptr || colon == gameStart) {
            return nullptr;
        }
        const auto owner = Ship::CrossRMRegistry::Get(std::string(gameStart, colon - gameStart));
        if (owner == nullptr) {
            return nullptr;
        }
        Ship::ResourceManagerScope scope(owner);
        res = owner->LoadResource(std::string("__OTR__") + (colon + 1));
    } else
#endif
    {
        res = ResourceMgr_GetResourceByNameHandlingMQ(filePath);
    }

    // The GBI wrappers reaching this (gSPInvalidateTexCache / gSPSegmentLoadRes) tolerate a null
    // address, so a resolve miss becomes a no-op instead of an AV on GetInitData(). pak_loader
    // hot-swapping equipment textures mid-kaleido can race-return null here.
    if (res == nullptr) {
        return nullptr;
    }

    if (res->GetInitData()->Type == static_cast<uint32_t>(Fast::ResourceType::DisplayList)) {
        return (char*)&((std::static_pointer_cast<Fast::DisplayList>(res))->Instructions[0]);
    }

    if (res->GetInitData()->Type == static_cast<uint32_t>(SOH::ResourceType::SOH_Array)) {
        return (char*)(std::static_pointer_cast<SOH::Array>(res))->Vertices.data();
    }

    // Retain the already resolved owner; a second local lookup would reject
    // the routed texture again. The draw still receives its original path.
    return (char*)res->GetRawPointer();
}

extern "C" char* ResourceMgr_LoadIfDListByName(const char* filePath) {
    if (filePath == nullptr) {
        return nullptr;
    }
    // A cold Alt material DL can replace a cached native texture at this path.
    // Resolve it before the regular lookup can return that texture from cache.
    ResourceMgr_PreloadAltWhenItExists(filePath);
    auto res = std::dynamic_pointer_cast<Fast::DisplayList>(ResourceMgr_GetResourceByNameHandlingMQ(filePath));
    if (res == nullptr || res->Instructions.empty()) {
        return nullptr;
    }
    return reinterpret_cast<char*>(res->Instructions.data());
}

extern "C" char* ResourceMgr_LoadPlayerAnimByName(const char* animPath) {
    auto anim = std::static_pointer_cast<SOH::PlayerAnimation>(ResourceMgr_GetResourceByNameHandlingMQ(animPath));

    return (char*)&anim->limbRotData[0];
}

// Wrap a raw PlayerAnimation resource (the kind in misc/link_animetion/, which
// is a raw s16 payload with no LinkAnimationHeader struct attached) in a
// LinkAnimationHeader so it can be passed to Player_AnimPlayLoop/PlayOnce.
// Mirrors the animation viewer's wrapping logic at animationViewer.cpp:131-138.
// Cached by path so repeated calls return the same pointer.
extern "C" LinkAnimationHeader* ResourceMgr_LoadPlayerAnimAsHeader(const char* animPath) {
    if (animPath == nullptr)
        return nullptr;
    auto res = ResourceMgr_GetResourceByNameHandlingMQ(animPath);
    if (res == nullptr)
        return nullptr;
    if (res->GetInitData()->Type != static_cast<uint32_t>(SOH::ResourceType::SOH_PlayerAnimation)) {
        return nullptr;
    }
    auto playerAnim = std::static_pointer_cast<SOH::PlayerAnimation>(res);

    constexpr size_t kS16PerFrame = 67; // matches animationViewer.cpp::PLAYER_ANIM_S16_PER_FRAME

    static std::map<std::string, LinkAnimationHeader> sPlayerAnimWrappers;
    LinkAnimationHeader& wrapper = sPlayerAnimWrappers[animPath];
    size_t totalS16 = playerAnim->GetPointerSize() / sizeof(int16_t);
    wrapper.common.frameCount = (s16)(totalS16 / kS16PerFrame);
    wrapper.segment = (void*)playerAnim->GetPointer();
    return &wrapper;
}

// Same as above, but the returned clip is IN PLACE: every frame keeps frame 0's
// root translation, so the animation no longer walks the body across the floor.
//
// Needed because the MHR dual-blade clips carry huge baked root motion (a run
// cycle nets ~20000 units, a dash ~22000). That is harmless while a form drives
// the clip by hand, but the moment such a clip is installed into OOT's own
// animation tables, OOT integrates the root delta through
// Player_StartAnimMovement/AnimationContext_SetMoveActor ON TOP of linearVelocity
// and the player double-moves — Link skates forward while running in place.
// Stripping the delta (not the offset — zeroing it would yank the body to the
// skeleton origin) leaves the pose intact and hands all travel back to OOT.
//
// stripY additionally pins the vertical root, which is what makes an otherwise
// grounded clip float or sink once OOT owns floor height.
// firstFrame/lastFrame (inclusive, -1 = the clip's own bounds) cut a SUB-RANGE out
// before the resample, so one packed clip can serve several engine slots. That is
// how Gerudo's guard works: DemonModeActivationFlourish is a single 46-frame
// flourish and OOT wants three separate animations for a shield (defense,
// defense_wait, defense_end), so the same file is sliced 1-20 / 21-30 / 31-45.
//
// Combined with targetFrames this is also the playback-speed knob: OOT plays these
// slots at a fixed 1.0, so a 20-frame range asked for as 10 frames simply runs at
// double speed. Doing it here instead of at the LinkAnimation_Change callsite keeps
// every engine path (raise, loop, release, and the interrupt path in
// Player_Action_808435C4) at the same speed without touching any of them.
extern "C" LinkAnimationHeader* ResourceMgr_LoadPlayerAnimAsHeaderInPlaceRange(const char* animPath, uint8_t stripY,
                                                                               int16_t firstFrame, int16_t lastFrame,
                                                                               int16_t targetFrames) {
    if (animPath == nullptr)
        return nullptr;
    auto res = ResourceMgr_GetResourceByNameHandlingMQ(animPath);
    if (res == nullptr)
        return nullptr;
    if (res->GetInitData()->Type != static_cast<uint32_t>(SOH::ResourceType::SOH_PlayerAnimation)) {
        return nullptr;
    }
    auto playerAnim = std::static_pointer_cast<SOH::PlayerAnimation>(res);

    constexpr size_t kS16PerFrame = 67; // 3 root translation + 64 limb rotation values

    // Keyed by path AND flag: the same clip can legitimately be wanted both with
    // and without its vertical root.
    struct InPlaceAnim {
        LinkAnimationHeader header;
        std::vector<int16_t> data;
    };
    static std::map<std::string, InPlaceAnim> sInPlaceAnims;

    const std::string key = std::string(animPath) + (stripY ? "#xyz" : "#xz") + "#" + std::to_string(targetFrames) +
                            "#" + std::to_string(firstFrame) + "-" + std::to_string(lastFrame);
    auto it = sInPlaceAnims.find(key);
    if (it != sInPlaceAnims.end()) {
        return &it->second.header;
    }

    const size_t totalS16 = playerAnim->GetPointerSize() / sizeof(int16_t);
    const size_t clipFrames = totalS16 / kS16PerFrame;
    if (clipFrames == 0)
        return nullptr;

    // Clamp the requested range into the clip. A range that lands entirely past the
    // end collapses to the last frame rather than returning null, so a mis-typed
    // window shows a frozen pose instead of silently reverting the slot to vanilla.
    size_t rangeBegin = (firstFrame > 0) ? (size_t)firstFrame : 0;
    if (rangeBegin >= clipFrames)
        rangeBegin = clipFrames - 1;
    size_t rangeEnd = ((lastFrame >= 0) && ((size_t)lastFrame < clipFrames)) ? (size_t)lastFrame : (clipFrames - 1);
    if (rangeEnd < rangeBegin)
        rangeEnd = rangeBegin;
    const size_t frameCount = rangeEnd - rangeBegin + 1;

    InPlaceAnim& entry = sInPlaceAnims[key];
    const int16_t* src = (const int16_t*)playerAnim->GetPointer() + rangeBegin * kS16PerFrame;

    // Resample to a fixed length when asked. OOT's locomotion is not a plain
    // playback: Player_Action_80840DE4 hard-sets animLength to 29 and the walk/run
    // blend rigs sample both clips at fixed frame RATIOS (16/29). Feed them clips
    // of 31 and 39 frames and the two are sampled out of phase with each other,
    // which is what throws a limb to a completely wrong angle mid-stride.
    // Nearest-frame resampling on purpose: these are packed s16 angles, and
    // interpolating them would smear any value that crosses the +-180 wrap.
    const size_t outFrames = (targetFrames > 0) ? (size_t)targetFrames : frameCount;
    entry.data.resize(outFrames * kS16PerFrame);
    for (size_t f = 0; f < outFrames; ++f) {
        size_t srcFrame = (outFrames == frameCount) ? f : (f * frameCount) / outFrames;
        if (srcFrame >= frameCount)
            srcFrame = frameCount - 1;
        std::copy(src + srcFrame * kS16PerFrame, src + (srcFrame + 1) * kS16PerFrame,
                  entry.data.begin() + f * kS16PerFrame);
    }

    const int16_t baseX = entry.data[0];
    const int16_t baseY = entry.data[1];
    const int16_t baseZ = entry.data[2];
    for (size_t f = 0; f < outFrames; ++f) {
        int16_t* frame = &entry.data[f * kS16PerFrame];
        frame[0] = baseX;
        frame[2] = baseZ;
        if (stripY) {
            frame[1] = baseY;
        }
    }

    entry.header.common.frameCount = (s16)outFrames;
    entry.header.segment = (void*)entry.data.data();
    return &entry.header;
}

extern "C" LinkAnimationHeader* ResourceMgr_LoadPlayerAnimAsHeaderInPlaceResampled(const char* animPath, uint8_t stripY,
                                                                                   int16_t targetFrames) {
    return ResourceMgr_LoadPlayerAnimAsHeaderInPlaceRange(animPath, stripY, -1, -1, targetFrames);
}

extern "C" LinkAnimationHeader* ResourceMgr_LoadPlayerAnimAsHeaderInPlace(const char* animPath, uint8_t stripY) {
    return ResourceMgr_LoadPlayerAnimAsHeaderInPlaceRange(animPath, stripY, -1, -1, 0);
}

extern "C" void ResourceMgr_PushCurrentDirectory(char* path) {
    Fast::gfx_push_current_dir(path);
}

extern "C" Gfx* ResourceMgr_LoadGfxByName(const char* path) {
    path = ResourceMgr_ResolveLinkTunicDListPath(path);
    // When an alt resource exists for the DL, we need to unload the original asset
    // to clear the cache so the alt asset will be loaded instead
    // OTRTODO: If Alt loading over original cache is fixed, this line can most likely be removed
    ResourceMgr_UnloadOriginalWhenAltExists(path);

    auto res = std::static_pointer_cast<Fast::DisplayList>(ResourceMgr_GetResourceByNameHandlingMQ(path));
    if (!res)
        return nullptr;
    return (Gfx*)&res->Instructions[0];
}

extern "C" uint8_t ResourceMgr_FileIsCustomByName(const char* path) {
    auto res = std::static_pointer_cast<Fast::DisplayList>(ResourceMgr_GetResourceByNameHandlingMQ(path));
    return res->GetInitData()->IsCustom;
}

typedef struct {
    int index;
    Gfx instruction;
    const void* instructionsPtr;
    size_t instructionCount;
    bool isCustom;
} GfxPatch;

std::unordered_map<std::string, std::unordered_map<std::string, GfxPatch>> originalGfx;

// Attention! This is primarily for cosmetics & bug fixes. For things like mods and model replacement you should be
// using OTRs instead (When that is available). Index can be found using the commented out section below.
extern "C" void ResourceMgr_PatchGfxByName(const char* path, const char* patchName, int index, Gfx instruction) {
    auto res = std::static_pointer_cast<Fast::DisplayList>(
        Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(path));

    if (res == nullptr || static_cast<size_t>(index) >= res->Instructions.size()) {
        return;
    }

    // Leaving this here for people attempting to find the correct Dlist index to patch
    /*if (strcmp("__OTR__objects/object_gi_longsword/gGiBiggoronSwordDL", path) == 0) {
        for (int i = 0; i < res->instructions.size(); i++) {
            Gfx* gfx = (Gfx*)&res->instructions[i];
            // Log all commands
            // SPDLOG_INFO("index:{} command:{}", i, gfx->words.w0 >> 24);
            // Log only SetPrimColors
            if (gfx->words.w0 >> 24 == 250) {
                SPDLOG_INFO("index:{} r:{} g:{} b:{} a:{}", i, _SHIFTR(gfx->words.w1, 24, 8), _SHIFTR(gfx->words.w1, 16,
    8), _SHIFTR(gfx->words.w1, 8, 8), _SHIFTR(gfx->words.w1, 0, 8));
            }
        }
    }*/

    // Index refers to individual gfx words, which are half the size on 32-bit
    // if (sizeof(uintptr_t) < 8) {
    // index /= 2;
    // }

    // Do not patch custom assets as they most likely do not have the same instructions as authentic assets
    if (res->GetInitData()->IsCustom) {
        return;
    }

    Gfx* gfx = (Gfx*)&res->Instructions[index];

    if (!originalGfx.contains(path) || !originalGfx[path].contains(patchName)) {
        originalGfx[path][patchName] = { index, *gfx, res->Instructions.data(), res->Instructions.size(),
                                         res->GetInitData()->IsCustom };
    }

    *gfx = instruction;
}

extern "C" void ResourceMgr_PatchGfxCopyCommandByName(const char* path, const char* patchName, int destinationIndex,
                                                      int sourceIndex) {
    auto res = std::static_pointer_cast<Fast::DisplayList>(
        Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(path));

    if (res == nullptr || static_cast<size_t>(destinationIndex) >= res->Instructions.size() ||
        static_cast<size_t>(sourceIndex) >= res->Instructions.size()) {
        return;
    }

    // Do not patch custom assets as they most likely do not have the same instructions as authentic assets
    if (res->GetInitData()->IsCustom) {
        return;
    }

    Gfx* destinationGfx = (Gfx*)&res->Instructions[destinationIndex];
    Gfx sourceGfx = *(Gfx*)&res->Instructions[sourceIndex];

    if (!originalGfx.contains(path) || !originalGfx[path].contains(patchName)) {
        originalGfx[path][patchName] = { destinationIndex, *destinationGfx, res->Instructions.data(),
                                         res->Instructions.size(), res->GetInitData()->IsCustom };
    }

    *destinationGfx = sourceGfx;
}

extern "C" void ResourceMgr_PatchCustomGfxByName(const char* path, const char* patchName, int index, Gfx instruction) {
    auto res = std::static_pointer_cast<Fast::DisplayList>(
        Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(path));

    if (res == nullptr || static_cast<size_t>(index) >= res->Instructions.size()) {
        return;
    }

    Gfx* gfx = (Gfx*)&res->Instructions[index];

    if (!originalGfx.contains(path) || !originalGfx[path].contains(patchName)) {
        originalGfx[path][patchName] = { index, *gfx, res->Instructions.data(), res->Instructions.size(),
                                         res->GetInitData()->IsCustom };
    }

    *gfx = instruction;
}

extern "C" void ResourceMgr_UnpatchGfxByName(const char* path, const char* patchName) {
    if (originalGfx.contains(path) && originalGfx[path].contains(patchName)) {
        auto res = std::static_pointer_cast<Fast::DisplayList>(
            Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(path));

        // If the resource is unavailable (e.g. swapped out when toggling alt assets), clean up the record and bail.
        if (res == nullptr) {
            ResourceMgr_UnloadResource(path);
            originalGfx[path].erase(patchName);
            return;
        }

        const GfxPatch& patch = originalGfx[path][patchName];
        // Skip and clean up if the backing resource changed since we recorded the patch (e.g. alt<->vanilla swap)
        // to avoid writing instructions from a different asset onto the current one.
        if (res->Instructions.data() != patch.instructionsPtr || res->Instructions.size() != patch.instructionCount ||
            res->GetInitData()->IsCustom != patch.isCustom) {
            ResourceMgr_UnloadResource(path);
            originalGfx[path].erase(patchName);
            return;
        }

        // Skip and clean up if the loaded resource is smaller than the recorded patch index (can happen when alt assets
        // swap in shorter display lists).
        if (static_cast<size_t>(patch.index) >= res->Instructions.size()) {
            originalGfx[path].erase(patchName);
            return;
        }

        Gfx* gfx = (Gfx*)&res->Instructions[patch.index];
        *gfx = patch.instruction;

        originalGfx[path].erase(patchName);
    }
}

extern "C" char* ResourceMgr_LoadArrayByName(const char* path) {
    auto res = std::static_pointer_cast<SOH::Array>(ResourceMgr_GetResourceByNameHandlingMQ(path));

    return (char*)res->Scalars.data();
}

// Return of LoadArrayByNameAsVec3s must be freed by the caller
extern "C" char* ResourceMgr_LoadArrayByNameAsVec3s(const char* path) {
    auto res = std::static_pointer_cast<SOH::Array>(ResourceMgr_GetResourceByNameHandlingMQ(path));

    // if (res->CachedGameAsset != nullptr)
    //     return (char*)res->CachedGameAsset;
    // else
    // {
    Vec3s* data = (Vec3s*)malloc(sizeof(Vec3s) * res->Scalars.size());

    for (size_t i = 0; i < res->Scalars.size(); i += 3) {
        data[(i / 3)].x = res->Scalars[i + 0].s16;
        data[(i / 3)].y = res->Scalars[i + 1].s16;
        data[(i / 3)].z = res->Scalars[i + 2].s16;
    }

    // res->CachedGameAsset = data;

    return (char*)data;
    // }
}

extern "C" CollisionHeader* ResourceMgr_LoadColByName(const char* path) {
    return (CollisionHeader*)ResourceGetDataByName(path);
}

extern "C" Vtx* ResourceMgr_LoadVtxByName(char* path) {
    return (Vtx*)ResourceGetDataByName(path);
}

extern "C" SequenceData ResourceMgr_LoadSeqByName(const char* path) {
    SequenceData* sequence = (SequenceData*)COMBO_OWN_RM()->GetResourceRawPointer(path);
    return *sequence;
}

extern "C" SequenceData* ResourceMgr_LoadSeqPtrByName(const char* path) {
    SequenceData* sequence = (SequenceData*)COMBO_OWN_RM()->GetResourceRawPointer(path);
    return sequence;
}

extern "C" SoundFontSample* ResourceMgr_LoadAudioSample(const char* path) {
    return (SoundFontSample*)COMBO_OWN_RM()->GetResourceRawPointer(path);
}

extern "C" SoundFont* ResourceMgr_LoadAudioSoundFontByName(const char* path) {
    return (SoundFont*)COMBO_OWN_RM()->GetResourceRawPointer(path);
}

extern "C" int ResourceMgr_OTRSigCheck(char* imgData) {
    uintptr_t i = (uintptr_t)(imgData);

    // if (i == 0xD9000000 || i == 0xE7000000 || (i & 1) == 1)
    if ((i & 1) == 1)
        return 0;

    // if ((i & 0xFF000000) != 0xAB000000 && (i & 0xFF000000) != 0xCD000000 && i != 0) {
    if (i != 0) {
        if (imgData[0] == '_' && imgData[1] == '_' && imgData[2] == 'O' && imgData[3] == 'T' && imgData[4] == 'R' &&
            imgData[5] == '_' && imgData[6] == '_') {
            return 1;
        }
    }

    return 0;
}

// Load animation with explicit alt asset path checking.
// When Alt Assets is OFF: use original path directly (O2R or vanilla)
// When Alt Assets is ON: try alt/ prefix first, fall back to regular path if not found or invalid
extern "C" AnimationHeaderCommon* ResourceMgr_LoadAnimByName(const char* path) {
    bool isAlt = ResourceMgr_IsAltAssetsEnabled();

    if (isAlt) {
        if (ResourceMgr_FileAltExists(path)) {
            std::string pathStr = std::string(path);
            static const std::string sOtr = "__OTR__";

            if (pathStr.starts_with(sOtr)) {
                pathStr = pathStr.substr(sOtr.length());
            }

            // Try alt/ first
            pathStr = Ship::IResource::gAltAssetPrefix + pathStr;

            AnimationHeaderCommon* animHeader = (AnimationHeaderCommon*)ResourceGetDataByName(pathStr.c_str());

            // If alt loaded successfully, verify it has valid data
            if (animHeader != NULL) {
                // Check for valid frame count (> 0)
                if (animHeader->frameCount > 0) {
                    // For Normal animations: check frameData (comes after frameCount in AnimationHeader)
                    // For Link animations: check segment (comes after frameCount in LinkAnimationHeader)
                    // We check both to be safe - if either is valid, the animation is usable
                    AnimationHeader* normalAnim = (AnimationHeader*)animHeader;
                    LinkAnimationHeader* linkAnim = (LinkAnimationHeader*)animHeader;

                    // Valid if Normal animation has frameData OR Link animation has segment
                    if (normalAnim->frameData != NULL || linkAnim->segment != NULL) {
                        return animHeader;
                    }
                }
                // Alt loaded but is invalid (broken), fall through to original path
            }
        }

        // Fall back to original path
        return (AnimationHeaderCommon*)ResourceGetDataByName(path);
    }

    // Alt OFF: use original path directly
    return (AnimationHeaderCommon*)ResourceGetDataByName(path);
}

extern "C" SkeletonHeader* ResourceMgr_LoadSkeletonByName(const char* path, SkelAnime* skelAnime) {
    std::string pathStr = std::string(path);
    static const std::string sOtr = "__OTR__";

    if (pathStr.starts_with(sOtr)) {
        pathStr = pathStr.substr(sOtr.length());
    }

    bool isAlt = ResourceMgr_IsAltAssetsEnabled();

    if (isAlt) {
        pathStr = Ship::IResource::gAltAssetPrefix + pathStr;
    }

    SkeletonHeader* skelHeader = (SkeletonHeader*)ResourceGetDataByName(pathStr.c_str());

    // If there isn't an alternate model, load the regular one
    if (isAlt && skelHeader == NULL) {
        skelHeader = (SkeletonHeader*)ResourceGetDataByName(path);
    }

    // This function is only called when a skeleton is initialized.
    // Therefore we can take this opportunity to take note of the Skeleton that is created...
    if (skelAnime != nullptr) {
        auto stringPath = std::string(path);
        SOH::SkeletonPatcher::RegisterSkeleton(stringPath, skelAnime);
    }

    return skelHeader;
}

extern "C" void ResourceMgr_UnregisterSkeleton(SkelAnime* skelAnime) {
    if (skelAnime != nullptr) {
        SOH::SkeletonPatcher::UnregisterSkeleton(skelAnime);
    }
}

extern "C" void ResourceMgr_ClearSkeletons() {
    SOH::SkeletonPatcher::ClearSkeletons();
}

extern "C" s32* ResourceMgr_LoadCSByName(const char* path) {
    return (s32*)ResourceMgr_GetResourceDataByNameHandlingMQ(path);
}

#ifdef COMBO_BUILD
// Dormant-owner queries: host MM's Alt toggle and resource cache never select OoT equipment.
extern "C" COMBO_EXPORT int32_t OOT_NeiAltAssetsEnabled(void) {
    const auto owner = Ship::CrossRMRegistry::Get("oot");
    return owner && owner->IsAltAssetsEnabled();
}

extern "C" COMBO_EXPORT int32_t OOT_MagicJarUsesCustomAsset(const char* path) {
    const auto owner = Ship::CrossRMRegistry::Get("oot");
    if (!path || !owner)
        return 0;
    // Factories may load dependencies through Context's active resource manager.
    // Keep those nested loads with the root owner, including Alt fallback.
    Ship::ResourceManagerScope scope(owner);
    std::string base = path;
    if (base.compare(0, 7, "__OTR__") == 0)
        base.erase(0, 7);
    std::string selected = base;
    if (owner->IsAltAssetsEnabled() && base.compare(0, 4, "alt/") != 0 &&
        (owner->GetArchiveManager()->HasFile("alt/" + base) ||
         owner->GetArchiveManager()->HasFile("alt/" + base + ".meta"))) {
        selected = "alt/" + base;
    }
    // Exact registered-owner loads avoid dormant save/MQ helpers and native cache eviction.
    // Match deferred resource fallback if a present Alt entry cannot actually be loaded.
    auto resource = owner->LoadResource(selected, true);
    if (!resource && selected != base)
        resource = owner->LoadResource(base, true);
    return resource && resource->GetInitData() && resource->GetInitData()->IsCustom;
}

// Native heart material offsets are owned by the Cosmetic Editor. Custom
// geometry receives a draw tint instead, so those offsets never touch an Alt DL.
extern "C" COMBO_EXPORT int32_t OOT_ApplyGiHeartCosmetics(const char* path, int32_t piece, uint8_t r, uint8_t g,
                                                          uint8_t b, int32_t changed) {
    const auto owner = Ship::CrossRMRegistry::Get("oot");
    if (!path || !owner)
        return 0;
    if (OOT_MagicJarUsesCustomAsset(path))
        return 1;
    Ship::ResourceManagerScope scope(owner);
    const char* primPatch = piece ? "Consumable_Hearts5" : "Consumable_Hearts7";
    const char* envPatch = piece ? "Consumable_Hearts6" : "Consumable_Hearts8";
    if (changed) {
        ResourceMgr_PatchGfxByName(path, primPatch, 2, gsDPSetPrimColor(0, 0, r, g, b, 255));
        ResourceMgr_PatchGfxByName(path, envPatch, 6, gsDPSetEnvColor(r / 2, g / 2, b / 2, 255));
    } else {
        ResourceMgr_UnpatchGfxByName(path, primPatch);
        ResourceMgr_UnpatchGfxByName(path, envPatch);
    }
    return 0;
}

// Typed resource-only query for native MM. The owner may be inactive; never
// consult OoT save/player state. Match deferred rendering using the registered
// owner's Alt mode, which can differ from the currently active MM mode.
extern "C" COMBO_EXPORT int32_t OOT_NeiResourceExists(const char* path) {
    constexpr char basePrefix[] = "__OTR__@oot-gi-base:";
    if (path && std::strncmp(path, basePrefix, sizeof(basePrefix) - 1) == 0) {
        const auto baseOwner = Ship::CrossRMRegistry::Get("oot-gi-base");
        return baseOwner && bool(baseOwner->LoadResource(path + sizeof(basePrefix) - 1, true));
    }
    const auto owner = Ship::CrossRMRegistry::Get("oot");
    if (!path || !owner)
        return 0;
    std::string resource = path;
    if (resource.compare(0, 7, "__OTR__") == 0)
        resource.erase(0, 7);
    if (resource.empty() || resource[0] == '@')
        return 0;
    // ExtensionCache is initialized by OoT's engine startup. In an MM-first
    // session the resident owner archives can be ready before that cache.
    const auto archives = owner->GetArchiveManager();
    if (!archives)
        return 0;
    // The resource loader also resolves alias-only .meta entries. Keep the
    // availability gate consistent with both base and selected Alt loading.
    const auto exists = [&](const std::string& name) {
        return archives->HasFile(name) || archives->HasFile(name + ".meta");
    };
    return exists(resource) || (owner->IsAltAssetsEnabled() && exists("alt/" + resource));
}

extern "C" COMBO_EXPORT int32_t OOT_NeiEnsureGiBaseOwner(void) {
    const auto owner = Ship::CrossRMRegistry::Get("oot");
    if (!owner || !owner->GetArchiveManager())
        return 0;
    if (Ship::CrossRMRegistry::Get("oot-gi-base"))
        return 1;
    const std::vector<std::string> shipped{ Ship::Context::LocateFileAcrossAppDirs("soh.o2r", "soh"),
                                            Ship::Context::LocateFileAcrossAppDirs("soh.o2r") };
    const auto archives = owner->GetArchiveManager()->GetArchives();
    if (!archives)
        return 0;
    for (const auto& archive : *archives) {
        if (!archive)
            continue;
        const auto path = NeiAssetPriority::NormalizeArchive(archive->GetPath());
        for (const auto& stock : shipped) {
            if (stock.empty() || path != NeiAssetPriority::NormalizeArchive(stock))
                continue;
            const auto view = owner->CreateResourceView(archive);
            if (!view)
                return 0;
            Ship::CrossRMRegistry::Register("oot-gi-base", view);
            return 1;
        }
    }
    return 0; // Missing shipped GI data must never borrow a mod archive.
}

#endif

#include "../../combo/NeiRewardGiResource.h"
extern "C" int ResourceMgr_GetRewardSurfaceForGame(const char* game, const char* path, NeiGi::Mesh* mesh) {
    return NeiGi::GetRewardSurface("oot", game, path, mesh);
}

extern "C" int ResourceMgr_GetGiModelFitForGame(const char* game, const char* path, float scale, float tilt, int shop,
                                                float fit[2]) {
    const int din =
        NeiAssetPriority::GetDinSwordGiProfile("oot", game, path, CVarGetInteger(CVAR_ENHANCEMENT("DinFireSword"), 0));
    return NeiAssetPriority::GetGiModelFit("oot", game, path, scale, tilt, shop, fit, din);
}
extern "C" int ResourceMgr_GetDinSwordGiProfileForGame(const char* game, const char* path) {
    return NeiAssetPriority::GetDinSwordGiProfile("oot", game, path,
                                                  CVarGetInteger(CVAR_ENHANCEMENT("DinFireSword"), 0));
}

extern "C" int ResourceMgr_GetGiModelsFitForGame(const char* game, const char* const* paths, int count, float scale,
                                                 float tilt, int presentation, float fit[2]) {
    return NeiAssetPriority::GetGiModelsFit("oot", game, paths, count, scale, tilt, presentation, fit,
                                            CVarGetInteger(CVAR_ENHANCEMENT("DinFireSword"), 0));
}

extern "C" int ResourceMgr_GetGiModelBoundsForGame(const char* game, const char* const* paths, int count,
                                                   float minimum[3], float maximum[3]) {
    return NeiAssetPriority::GetGiModelBounds("oot", game, paths, count, minimum, maximum);
}

extern "C" int ResourceMgr_IsCustomAssetForGame(const char* game, const char* path) {
    return NeiAssetPriority::IsCustomAsset("oot", game, path);
}

extern "C" int ResourceMgr_GetIkanaShieldGiPoseForGame(const char* game, const char* path, float* tilt, float* roll) {
    return NeiAssetPriority::GetIkanaShieldGiPose("oot", game, path, tilt, roll);
}

extern "C" int ResourceMgr_GetIkanaShieldGiTiltXForGame(const char* game, const char* path, float* tilt) {
    return NeiAssetPriority::GetIkanaShieldGiTiltX("oot", game, path, tilt);
}

extern "C" int ResourceMgr_IsModAsset(const char* path) {
    return NeiAssetPriority::IsModAsset("oot", "oot", path);
}

extern "C" int ResourceMgr_IsModAssetForGame(const char* game, const char* path) {
    return NeiAssetPriority::IsModAsset("oot", game, path);
}

extern "C" int ResourceMgr_IsGiModelAvailableForGame(const char* game, const char* path) {
    if (!game || !path || (std::strcmp(game, "oot") && std::strcmp(game, "mm")))
        return 0;
    auto owner = Ship::CrossRMRegistry::Get(game);
#ifndef COMBO_BUILD
    if (!owner && std::strcmp(game, "oot") == 0)
        if (const auto context = Ship::Context::GetRawInstance())
            owner = context->GetResourceManager();
#endif
    if (!owner)
        return 0;
    // XML factories may resolve dependencies through the active Context. The
    // selected owner's normal load also retains its Alt/.meta/cache policy.
    Ship::ResourceManagerScope scope(owner);
    const auto model = std::dynamic_pointer_cast<Fast::DisplayList>(owner->LoadResource(path));
    return model && !model->Instructions.empty();
}

extern "C" int ResourceMgr_IsIkanaShieldGiAvailable(void) {
    if (Ship::CrossRMRegistry::Get("mm")) {
        return ResourceMgr_IsGiModelAvailableForGame("mm", COMBO_IKANA_GI_OPA) &&
               ResourceMgr_IsGiModelAvailableForGame("mm", COMBO_IKANA_GI_XLU);
    }
#ifdef COMBO_BUILD
    if (gComboGameMode != COMBO_GAME_OOT)
        return 0;
#endif
    // Solo OoT uses the optional MM archive without starting the other game.
    // Strict graph resolution validates every nested DL, vertex and texture.
    return MmAssets_IsAvailable() && MmAssets_LoadDisplayListGraphStrict(COMBO_IKANA_GI_OPA) &&
           MmAssets_LoadDisplayListGraphStrict(COMBO_IKANA_GI_XLU);
}

#ifdef COMBO_BUILD
extern "C" COMBO_EXPORT int OOT_NeiResourceIsMod(const char* path) {
    return ResourceMgr_IsModAsset(path);
}
#endif

extern "C"
#ifdef COMBO_BUILD
    COMBO_EXPORT
#endif
    int
    OOT_CopyWolfLinkResource(uint8_t* destination, size_t capacity, size_t* size, const char** owner) {
    return NeiWolfAsset::CopyResource("oot", destination, capacity, size, owner);
}

extern "C"
#ifdef COMBO_BUILD
    COMBO_EXPORT
#endif
    int
    OOT_CopyWolfLinkModelResource(int useHD, uint8_t* destination, size_t capacity, size_t* size, const char** owner,
                                  const char** path) {
    return NeiWolfAsset::CopyResource("oot", destination, capacity, size, owner, useHD != 0, path);
}
