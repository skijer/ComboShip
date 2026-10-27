#pragma once
#include "BlackMarketGranny.h"
#include "BlackMarketGrannyScene.h"
#include "BlackMarketShelves.h"
#include <fast/resource/type/DisplayList.h>
#include <fast/resource/type/Vertex.h>
#include <fast/resource/type/Texture.h>
#include <ship/Context.h>
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManager.h>
#include <libultraship/bridge/consolevariablebridge.h>
#include <spdlog/spdlog.h>

namespace BlackMarketGranny {
inline constexpr char kErrorCvar[] = "gCombo.BlackMarket.GrannyLoadError";

inline Vertices NativeVaporVertices(const std::shared_ptr<Ship::IResource>& resource) {
    if (!resource || !resource->GetInitData())
        return {};
    const auto type = resource->GetInitData()->Type;
    // Original OoT vertices are OARR/Vertex, newer archives may use OVTX.
    // Consume only the pinned owner's public byte interface: game-specific
    // Array RTTI must not cross the OoT/MM DLL boundary. Copy for validation;
    // rendering still uses the unchanged native vertex hash in the private view.
    if ((type != 0x4f415252 && type != 0x4f565458) || resource->GetPointerSize() != 56 * sizeof(Vtx))
        return {};
    const auto* bytes = resource->GetRawPointer();
    if (!bytes)
        return {};
    auto vertices = std::make_shared<std::vector<Vtx>>(56);
    std::memcpy(vertices->data(), bytes, 56 * sizeof(Vtx));
    return { vertices->data(), vertices->size(), vertices };
}

inline bool MapsDependenciesAvailable(const std::shared_ptr<Ship::ResourceManager>& maps) {
    const auto archive = maps ? maps->GetArchiveManager() : nullptr;
    return archive && archive->HasFile(Exact(kOpaque)) && archive->HasFile(Exact(kCounterVertices)) &&
           archive->HasFile("alt/scenes/shared/mahouya_scene/sh_maho_book_01__UDIM_");
}
inline std::pair<std::string, std::shared_ptr<Ship::ResourceManager>> MapsOwner() {
    const auto context = Ship::Context::GetRawInstance();
    if (!context)
        return {};
    const auto active = context->GetResourceManager();
    for (const char* name : { "oot", "mm" }) {
        auto rm = Ship::CrossRMRegistry::Get(name);
        if (rm == active && MapsDependenciesAvailable(rm))
            return { name, rm };
    }
    for (const char* name : { "oot", "mm" }) {
        auto rm = Ship::CrossRMRegistry::Get(name);
        if (MapsDependenciesAvailable(rm))
            return { name, rm };
    }
    return {};
}

inline std::pair<std::shared_ptr<Ship::ResourceManager>, std::shared_ptr<Ship::Archive>> NativeOwner() {
    // MM-only already mounts the user's optional oot.o2r companion. Its native
    // scene parser differs, so use only common render factories and raw metadata.
    const auto complete = [](const std::shared_ptr<Ship::Archive>& archive) {
        return archive && archive->HasFile(kScene) && archive->HasFile(kRoom) && archive->HasFile(kTranslucent);
    };
    for (const char* name : { "oot", "mm" }) {
        auto rm = Ship::CrossRMRegistry::Get(name);
        if (!rm)
            continue;
        const auto manager = rm->GetArchiveManager();
        if (!manager)
            continue;
        auto archive = manager->GetArchiveFromFile(kScene);
        if (complete(archive))
            return { rm, archive };
        // A scene-only mod can own the winning header while the complete base
        // archive remains mounted. Pin one complete donor, never a mixed graph.
        const auto archives = manager->GetArchives();
        if (archives)
            for (auto it = archives->rbegin(); it != archives->rend(); ++it)
                if (complete(*it))
                    return { rm, *it };
    }
    return {};
}
inline bool DependenciesAvailable() {
    const auto [game, maps] = MapsOwner();
    const auto [native, archive] = NativeOwner();
    return maps && native;
}

// The native actor has a different ID and switch-bit encoding in MM.
inline int16_t MmFlameParams(int16_t oot) {
    return (oot & 0x3ff) | ((oot & 0x400) << 1) | ((oot & 0x800) << 1) | (oot & 0x8000);
}

class Resources {
  public:
    bool Prepare(const char* host) {
        try {
            auto [game, maps] = MapsOwner();
            if (!maps)
                return Error(
                    "Djipi Granny room, counter vertices or 3DS textures are unavailable in a registered game.");
            const auto [native, archive] = NativeOwner();
            if (!native)
                return Error(
                    "A complete original OoT Granny scene, room and translucent-resource archive is unavailable.");
            if (mRoom) {
                CVarSetString(kErrorCvar, "");
                return true;
            }
            Provider provider;
            provider.game = game;
            provider.display = [maps](const std::string& path) -> Display {
                auto res = std::dynamic_pointer_cast<Fast::DisplayList>(maps->LoadResource(path, true));
                return res ? Display{ res->Instructions.data(), res->Instructions.size(), res } : Display{};
            };
            provider.vertices = [maps](const std::string& path) -> Vertices {
                auto res = std::dynamic_pointer_cast<Fast::Vertex>(maps->LoadResource(path, true));
                return res ? Vertices{ res->VertexList.data(), res->VertexList.size(), res } : Vertices{};
            };
            provider.texture = [maps](const std::string& path) -> std::shared_ptr<void> {
                return std::dynamic_pointer_cast<Fast::Texture>(maps->LoadResource(path, true));
            };
            auto room = std::make_unique<Room>();
            if (!room->Load(provider, true))
                return Error(room->Error());
            auto view = native->CreateResourceView(archive);
            if (!view)
                return Error("Original Granny archive is unavailable.");
            auto xlu = std::dynamic_pointer_cast<Fast::DisplayList>(view->LoadResource(kTranslucent, true));
            auto sceneFile = archive->LoadFile(kScene), roomFile = archive->LoadFile(kRoom);
            NativeMetadata metadata;
            if (!xlu || xlu->Instructions.empty() || !sceneFile || !roomFile || !sceneFile->Buffer ||
                !roomFile->Buffer || !metadata.ReadScene(*sceneFile->Buffer) || !metadata.ReadRoom(*roomFile->Buffer))
                return Error("Unsupported original Granny metadata or translucent resources.");
            if (std::strcmp(host, "oot") && std::strcmp(host, "mm"))
                return Error("Invalid Granny room owner.");
            mRoute = std::string("granny_") + host;
            mShelfPaths = { "__OTR__@" + game + ":" + BlackMarketShelves::kWoodTexture,
                            "__OTR__@" + game + ":" + BlackMarketShelves::kAtlasTexture };
            auto vapor = std::make_unique<Vapor>();
            auto vaporLeaf = std::dynamic_pointer_cast<Fast::DisplayList>(view->LoadResource(kVaporLeaf, true));
            auto vaporVertices = NativeVaporVertices(view->LoadResource(kVaporVertices, true));
            if (!vaporLeaf || !vaporVertices.data ||
                !vapor->Load({ xlu->Instructions.data(), xlu->Instructions.size(), xlu },
                             { vaporLeaf->Instructions.data(), vaporLeaf->Instructions.size(), vaporLeaf },
                             vaporVertices))
                return Error("Unsupported original Granny vapor layout.");
            mRoom = std::move(room);
            mVapor = std::move(vapor);
            mXlu = std::move(xlu);
            mLights = std::move(metadata.lights);
            mDecorations = std::move(metadata.decorations);
            mNativeView = std::move(view);
            // Publish only the complete view; native XLU cannot borrow host paths.
            Ship::CrossRMRegistry::Register(mRoute, mNativeView);
            CVarSetString(kErrorCvar, "");
            return true;
        } catch (const std::exception& e) { return Error(e.what()); }
    }
    Gfx* Opaque() const {
        return mRoom ? mRoom->Opaque() : nullptr;
    }
    Gfx* Translucent() const {
        return mVapor ? mVapor->Root() : nullptr;
    }
    const char* Route() const {
        return mRoute.c_str();
    }
    const auto& Lights() const {
        return mLights;
    }
    const auto& Decorations() const {
        return mDecorations;
    }
    std::array<const char*, 2> ShelfTextures() const {
        return mRoom ? std::array<const char*, 2>{ mShelfPaths[0].c_str(), mShelfPaths[1].c_str() }
                     : std::array<const char*, 2>{};
    }
    void Clear() {
        // Unregister invokes every teardown listener. Move the owned view out
        // and guard recursion before unregistering its private renderer route.
        if (mClearing)
            return;
        mClearing = true;
        auto view = std::move(mNativeView);
        mRoom.reset();
        mVapor.reset();
        mXlu.reset();
        mLights.clear();
        mDecorations.clear();
        mShelfPaths = {};
        if (view && Ship::CrossRMRegistry::Get(mRoute) == view)
            Ship::CrossRMRegistry::Unregister(mRoute);
        mRoute.clear();
        mClearing = false;
    }

  private:
    std::unique_ptr<Room> mRoom;
    std::unique_ptr<Vapor> mVapor;
    std::shared_ptr<Fast::DisplayList> mXlu;
    std::shared_ptr<Ship::ResourceManager> mNativeView;
    std::string mRoute;
    std::array<std::string, 2> mShelfPaths;
    bool mClearing = false;
    std::vector<std::array<uint8_t, 22>> mLights;
    std::vector<Decoration> mDecorations;
    static bool Error(const std::string& message) {
        SPDLOG_WARN("BlackMarketGranny: {}", message);
        CVarSetString(kErrorCvar, message.c_str());
        return false;
    }
};

// A new small wrapper is allocated in the current frame's graphics arena.
// This reproduces native Granny draw-config 46, after default scene setup has
// bound its empty segments. Native XLU dependencies use the pinned base view.
inline void Wrap(Gfx* opa, Gfx* xlu, Gfx* opaque, Gfx* translucent, Gfx* scroll, Gfx* smoke, const char* owner) {
    const Gfx opaqueCommands[] = { gsSPSegment(0x08, scroll), gsDPPipeSync(), gsDPSetEnvColor(128, 128, 128, 128),
                                   gsSPDisplayList(opaque), gsSPEndDisplayList() };
    const Gfx translucentCommands[] = { gsSPComboRMPush(owner),
                                        gsSPSegment(0x09, smoke),
                                        gsDPPipeSync(),
                                        gsDPSetEnvColor(128, 128, 128, 128),
                                        gsSPDisplayList(translucent),
                                        gsSPComboRMPop(),
                                        gsSPEndDisplayList() };
    std::memcpy(opa, opaqueCommands, sizeof(opaqueCommands));
    std::memcpy(xlu, translucentCommands, sizeof(translucentCommands));
}
} // namespace BlackMarketGranny
