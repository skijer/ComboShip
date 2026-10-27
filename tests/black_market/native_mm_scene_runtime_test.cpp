// Exact production scene-readiness guard, actual native resource classes,
// actual binary factories/readers, and actual native cutscene-chain lookup.
// Only ResourceLoad/OwnRMScope and destructor logging are controlled.
#include "global.h"
#include "overlays/actors/ovl_En_Ossan/z_en_ossan.h"
#include "combo/BlackMarketMmShopScene.h"
#include "2s2h/resource/type/Scene.h"
#include "2s2h/resource/type/scenecommand/SetCsCamera.h"
#include "2s2h/resource/type/scenecommand/SetActorCutsceneList.h"
#include "2s2h/resource/type/scenecommand/SetMesh.h"
#include "2s2h/resource/importer/scenecommand/SetMeshFactory.h"
#include "2s2h/resource/importer/scenecommand/SetCsCameraFactory.h"
#include "2s2h/resource/importer/scenecommand/SetActorCutsceneListFactory.h"
#include <libultraship/bridge/resourcebridge.h>
#include <ship/resource/CrossRMRegistry.h>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

extern "C" {
extern ActorCutscene* sSceneCutsceneList;
extern s16 sSceneCutsceneCount;
void EnOssan_GetCutscenes(EnOssan*, PlayState*);
}

static PlayState play;
static std::shared_ptr<Ship::IResource> baseResource, altResource;
static std::shared_ptr<SOH::Scene> roomResource;
static std::shared_ptr<SOH::SetMesh> roomMesh;
static bool altEnabled;
static std::string activeManager = "foreign";
static std::vector<std::string> previousManagers;
static int loads, enteredNativeCutscenes, cases;
static const char* cameraPath;
static const char* cutscenePath;
static const char* meshPath;

static void Require(bool condition, const std::string& reason) {
    if (!condition)
        throw std::runtime_error(reason);
}

namespace Ship {
OwnRMScope::OwnRMScope(const char* name) {
    previousManagers.push_back(activeManager);
    activeManager = name;
}
OwnRMScope::~OwnRMScope() {
    activeManager = previousManagers.back();
    previousManagers.pop_back();
}
} // namespace Ship

std::shared_ptr<Ship::IResource> ResourceLoad(const char* name) {
    Require(activeManager == "mm", "production guard did not acquire its own MM resource manager");
    if (std::strcmp(name, "scenes/nonmq/skiejer_market/skiejer_market_room_0") == 0) {
        ++loads;
        return roomResource;
    }
    Require(std::strcmp(name, "scenes/nonmq/skiejer_market/skiejer_market") == 0,
            "production guard requested a different scene");
    ++loads;
    return altEnabled && altResource ? altResource : baseResource;
}

static bool sGrannyActive;
static RoomShape sGrannyShape{};
static RoomShape* sGrannyOriginal;

/* PRODUCTION READINESS */

struct SceneResources {
    std::shared_ptr<SOH::Scene> scene;
    std::shared_ptr<SOH::SetCsCamera> cameras;
    std::shared_ptr<SOH::SetActorCutsceneList> cutscenes;
};

static std::shared_ptr<Ship::IResource> LoadCommand(const char* path, SOH::SceneCommandFactoryBinaryV0& factory) {
    std::ifstream file(path, std::ios::binary);
    std::vector<char> bytes((std::istreambuf_iterator<char>(file)), {});
    auto reader = std::make_shared<Ship::BinaryReader>(bytes.data(), bytes.size());
    reader->SetEndianness(Ship::Endianness::Little);
    auto resource = factory.ReadResource(std::make_shared<Ship::ResourceInitData>(), reader);
    Require(reader->GetBaseAddress() == bytes.size(), "native serialized factory did not consume EOF");
    return resource;
}

static SceneResources ReadScene() {
    SceneResources resources;
    SOH::SetCsCameraFactory cameraFactory;
    SOH::SetActorCutsceneListFactory cutsceneFactory;
    resources.scene = std::make_shared<SOH::Scene>(std::make_shared<Ship::ResourceInitData>());
    resources.cameras = std::dynamic_pointer_cast<SOH::SetCsCamera>(LoadCommand(cameraPath, cameraFactory));
    resources.cutscenes = std::dynamic_pointer_cast<SOH::SetActorCutsceneList>(LoadCommand(cutscenePath, cutsceneFactory));
    Require(resources.cameras && resources.cutscenes, "native command factory returned a different resource type");
    resources.scene->commands = { resources.cameras, resources.cutscenes };
    return resources;
}

static void Bind(const SceneResources& resources) {
    play.actorCsCamList = reinterpret_cast<ActorCsCamInfo*>(resources.cameras->GetPointer());
    sSceneCutsceneList = reinterpret_cast<ActorCutscene*>(resources.cutscenes->GetPointer());
    sSceneCutsceneCount = resources.cutscenes->entries.size();
}

static SceneResources Reset() {
    baseResource.reset();
    altResource.reset();
    sNativeShopSceneResource.reset();
    play = {};
    sSceneCutsceneList = nullptr;
    sSceneCutsceneCount = 0;
    altEnabled = false;
    sGrannyActive = false;
    sGrannyShape = {};
    sGrannyOriginal = nullptr;
    loads = enteredNativeCutscenes = 0;
    Require(activeManager == "foreign" && previousManagers.empty(), "resource manager scope leaked");
    auto resources = ReadScene();
    SOH::SetMeshFactory meshFactory;
    roomMesh = std::dynamic_pointer_cast<SOH::SetMesh>(LoadCommand(meshPath, meshFactory));
    roomResource = std::make_shared<SOH::Scene>(std::make_shared<Ship::ResourceInitData>());
    roomResource->commands = {roomMesh};
    play.roomCtx.curRoom.roomShape = reinterpret_cast<RoomShape*>(roomMesh->GetPointer());
    baseResource = resources.scene;
    Bind(resources);
    return resources;
}

static EnOssan GuardedNativeLookup(bool expected, const std::string& reason) {
    EnOssan shop{};
    shop.actor.csId = 0;
    shop.lookToShopkeeperCsId = shop.lookToLeftShelfCsId = shop.lookToRightShelfCsId =
        shop.lookToShopKeeperFromShelfCsId = -77;
    bool ready = NativeShopSceneReady(&play);
    if (ready) {
        ++enteredNativeCutscenes;
        EnOssan_GetCutscenes(&shop, &play);
    }
    Require(ready == expected, reason);
    Require(loads >= 1 && loads <= 2 && activeManager == "foreign" && previousManagers.empty(),
            reason + ": own-MM resource scope was not restored");
    if (expected) {
        Require(enteredNativeCutscenes == 1 && sNativeShopSceneResource,
                reason + ": accepted native scene was not retained");
        Require(shop.lookToShopkeeperCsId == 0 && shop.lookToLeftShelfCsId == 1 &&
                shop.lookToRightShelfCsId == 2 && shop.lookToShopKeeperFromShelfCsId == 3,
                reason + ": real unchecked native GetCutscenes followed the wrong chain");
    } else {
        Require(enteredNativeCutscenes == 0 && !sNativeShopSceneResource && shop.lookToShopkeeperCsId == -77 &&
                shop.lookToLeftShelfCsId == -77 && shop.lookToRightShelfCsId == -77 &&
                shop.lookToShopKeeperFromShelfCsId == -77,
                reason + ": invalid scene entered unchecked native GetCutscenes");
    }
    ++cases;
    return shop;
}

static void TrimCameras(SOH::SetCsCamera& cameras, size_t count) {
    for (size_t i = count; i < cameras.csCamera.size(); ++i)
        delete[] cameras.csCamera[i].actorCsCamFuncData;
    cameras.csCamera.resize(count);
}

static void ResourceTypeCases() {
    auto resources = Reset();
    baseResource.reset();
    GuardedNativeLookup(false, "missing typed scene");
    resources = Reset();
    baseResource = resources.cameras;
    GuardedNativeLookup(false, "non-scene resource");
    resources = Reset();
    resources.scene->commands.clear();
    GuardedNativeLookup(false, "scene without camera or cutscene commands");
    resources = Reset();
    resources.scene->commands.erase(resources.scene->commands.begin());
    GuardedNativeLookup(false, "missing typed camera command");
    resources = Reset();
    resources.scene->commands.pop_back();
    GuardedNativeLookup(false, "missing typed cutscene command");
    resources = Reset();
    resources.scene->commands.push_back(nullptr);
    GuardedNativeLookup(false, "null scene command");
    resources = Reset();
    auto wrongType = std::make_shared<SOH::SetActorCutsceneList>(std::make_shared<Ship::ResourceInitData>());
    wrongType->cmdId = SOH::SceneCommandID::SetCsCamera;
    resources.scene->commands[0] = wrongType;
    GuardedNativeLookup(false, "camera command ID with a different dynamic resource type");
    resources = Reset();
    auto wrongCutsceneType = std::make_shared<SOH::SetCsCamera>(std::make_shared<Ship::ResourceInitData>());
    wrongCutsceneType->cmdId = SOH::SceneCommandID::SetActorCutsceneList;
    resources.scene->commands[1] = wrongCutsceneType;
    GuardedNativeLookup(false, "cutscene command ID with a different dynamic resource type");
}

static void RecordCountAndPointerCases() {
    for (size_t count = 0; count < 4; ++count) {
        auto resources = Reset();
        TrimCameras(*resources.cameras, count);
        Bind(resources);
        GuardedNativeLookup(false, "old scene has fewer than four native camera records");
        resources = Reset();
        resources.cutscenes->entries.resize(count);
        Bind(resources);
        GuardedNativeLookup(false, "old scene has fewer than four native cutscene records");
    }
    auto resources = Reset();
    play.actorCsCamList = nullptr;
    GuardedNativeLookup(false, "missing loaded actor camera pointer");
    resources = Reset();
    auto other = ReadScene();
    play.actorCsCamList = reinterpret_cast<ActorCsCamInfo*>(other.cameras->GetPointer());
    GuardedNativeLookup(false, "equal camera values at a different loaded allocation");
    resources = Reset();
    sSceneCutsceneList = nullptr;
    GuardedNativeLookup(false, "missing actual manager cutscene pointer");
    resources = Reset();
    sSceneCutsceneList = reinterpret_cast<ActorCutscene*>(other.cutscenes->GetPointer());
    GuardedNativeLookup(false, "equal cutscene values at a different manager allocation");
    resources = Reset();
    ++sSceneCutsceneList;
    GuardedNativeLookup(false, "manager pointer starts at the wrong cutscene entry");
}

static void CameraAndCutsceneCases() {
    for (int camera = 0; camera < 4; ++camera) {
        for (int field = 0; field < 9; ++field) {
            auto resources = Reset();
            auto& point = resources.cameras->csCamera[camera].actorCsCamFuncData[field / 3];
            if (field % 3 == 0) ++point.x;
            if (field % 3 == 1) ++point.y;
            if (field % 3 == 2) ++point.z;
            GuardedNativeLookup(false, "wrong native camera pose/FOV/rate/unused field");
        }
        auto resources = Reset();
        resources.cameras->csCamera[camera].setting = CAM_SET_FIXED2;
        GuardedNativeLookup(false, "camera is not actual native Fixed1");
        resources = Reset();
        resources.cameras->csCamera[camera].count = 2;
        GuardedNativeLookup(false, "camera data has fewer than three vectors");
        resources = Reset();
        delete[] resources.cameras->csCamera[camera].actorCsCamFuncData;
        resources.cameras->csCamera[camera].actorCsCamFuncData = nullptr;
        GuardedNativeLookup(false, "missing actual native camera data");
    }
    const std::array<void (*)(SOH::CutsceneEntry&), 10> changes = {
        [](auto& cs) { ++cs.priority; }, [](auto& cs) { ++cs.length; }, [](auto& cs) { cs.csCamId = 99; },
        [](auto& cs) { ++cs.scriptIndex; }, [](auto& cs) { cs.additionalCsId = 32767; },
        [](auto& cs) { ++cs.endSfx; }, [](auto& cs) { --cs.customValue; }, [](auto& cs) { ++cs.hudVisibility; },
        [](auto& cs) { ++cs.endCam; }, [](auto& cs) { --cs.letterboxSize; },
    };
    for (int index = 0; index < 4; ++index)
        for (auto change : changes) {
            auto resources = Reset();
            change(resources.cutscenes->entries[index]);
            GuardedNativeLookup(false, "wrong native cutscene metadata or out-of-range linked ID");
        }
}

static void SelectedResourceAndLifetimeCases() {
    auto resources = Reset();
    GuardedNativeLookup(true, "valid base scene");
    resources = Reset();
    auto alternate = ReadScene();
    altResource = alternate.scene;
    altEnabled = true;
    Bind(alternate);
    GuardedNativeLookup(true, "selected valid Alt scene");
    resources = Reset();
    alternate = ReadScene();
    altResource = alternate.scene;
    altEnabled = true;
    GuardedNativeLookup(false, "selected Alt scene differs from loaded base pointers");
    resources = Reset();
    alternate = ReadScene();
    alternate.cutscenes->entries.resize(3);
    altResource = alternate.scene;
    altEnabled = true;
    Bind(alternate);
    GuardedNativeLookup(false, "selected old Alt scene must not fall back to a valid base scene");
    resources = Reset();
    alternate = ReadScene();
    resources.cutscenes->entries.resize(3);
    altResource = alternate.scene;
    altEnabled = true;
    Bind(alternate);
    GuardedNativeLookup(true, "selected valid Alt scene supersedes old base scene");
    resources = Reset();
    alternate = ReadScene();
    altResource = alternate.scene;
    Bind(alternate);
    GuardedNativeLookup(false, "disabled Alt assets must validate actual selected base pointers");
    resources = Reset();
    altEnabled = true;
    GuardedNativeLookup(true, "missing Alt resource uses selected valid base resource");
    std::weak_ptr<SOH::Scene> lifetime = resources.scene;
    resources = {};
    baseResource.reset();
    Require(!lifetime.expired(), "accepted scene and raw native arrays were not retained");
    EnOssan shop{};
    shop.actor.csId = 0;
    EnOssan_GetCutscenes(&shop, &play);
    Require(shop.lookToShopKeeperFromShelfCsId == 3, "retained scene lost native cutscene array lifetime");
    sNativeShopSceneResource.reset();
    Require(lifetime.expired(), "test retained an unexpected independent scene owner");
    ++cases;
}

static void RoomPresentationCases() {
    Reset(); roomResource.reset();
    GuardedNativeLookup(false, "missing selected MM room");
    Reset(); roomResource->commands.clear();
    GuardedNativeLookup(false, "missing typed room mesh");
    Reset(); roomMesh->opaPaths[0] = "__OTR__custom/skiejer_market/room";
    GuardedNativeLookup(false, "legacy or Alt basket mesh is not the private layout");
    Reset(); play.roomCtx.curRoom.roomShape = nullptr;
    GuardedNativeLookup(false, "selected room resource differs from the actually loaded mesh");
    Reset(); roomMesh->xluPaths.push_back("foreign/extra");
    GuardedNativeLookup(false, "unexpected room translucent graph");
    Reset(); altEnabled = true;
    GuardedNativeLookup(true, "native companion presentation with Alt enabled");
}
static void GrannyPresentationCases() {
    Reset();
    sGrannyOriginal = play.roomCtx.curRoom.roomShape;
    play.roomCtx.curRoom.roomShape = &sGrannyShape;
    sGrannyActive = true;
    GuardedNativeLookup(true, "owned Granny shape preserves validated native shop initialization");
    Reset();
    sGrannyOriginal = play.roomCtx.curRoom.roomShape;
    play.roomCtx.curRoom.roomShape = &sGrannyShape;
    GuardedNativeLookup(false, "inactive Granny replacement cannot bypass room identity");
    Reset();
    sGrannyActive = true;
    play.roomCtx.curRoom.roomShape = &sGrannyShape;
    GuardedNativeLookup(false, "Granny replacement without the original owner is rejected");
    Reset();
    RoomShape foreign{};
    sGrannyActive = true;
    sGrannyOriginal = &foreign;
    play.roomCtx.curRoom.roomShape = &sGrannyShape;
    GuardedNativeLookup(false, "Granny from another room is rejected");
    Reset();
    sGrannyActive = true;
    sGrannyOriginal = play.roomCtx.curRoom.roomShape;
    play.roomCtx.curRoom.roomShape = &foreign;
    GuardedNativeLookup(false, "foreign replacement after Granny activation is rejected");
}
int main(int argc, char** argv) {
    try {
        Require(argc == 4, "expected prepared native camera, cutscene and mesh command files");
        cameraPath = argv[1];
        cutscenePath = argv[2];
        meshPath = argv[3];
        ResourceTypeCases();
        RecordCountAndPointerCases();
        CameraAndCutsceneCases();
        SelectedResourceAndLifetimeCases();
        RoomPresentationCases();
        GrannyPresentationCases();
        std::cout << "PASS exact production MM scene readiness: " << cases
                  << " typed/count/metadata/pointer/Alt/lifetime cases before real native GetCutscenes\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
