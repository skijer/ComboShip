#pragma once
#include "BlackMarketRoomMagic.h"
#include <fast/resource/type/DisplayList.h>
#include <fast/resource/type/Vertex.h>
#include <fast/resource/type/Texture.h>
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManager.h>
#include <libultraship/bridge/consolevariablebridge.h>

namespace BlackMarketRoomMagic {
inline Palette ReadPalette(bool oot) {
    Palette palette = kDefaultPalette;
    for (size_t i = 0; i < palette.size(); ++i) {
        const auto key = std::string(oot ? "gCosmetics." : "gCosmetic.") + kCosmeticKeys[i];
        if (!CVarGetInteger((key + ".Changed").c_str(), 0))
            continue;
        const auto& fallback = kDefaultPalette[i];
        const auto color = CVarGetColor((key + (oot ? ".Value" : ".Color")).c_str(),
                                        Color_RGBA8{ fallback[0], fallback[1], fallback[2], 255 });
        palette[i] = { color.r, color.g, color.b, 255 };
    }
    return palette;
}

class Resources {
  public:
    struct PreparationTrace {
        const char* reason = "skipped";
        const void* owner = nullptr;
        const void* root = nullptr;
        bool alt = false;
    };
    const PreparationTrace& LastPreparation() const {
        return mPreparation;
    }
    bool Prepare(const char* host, const Gfx* opaque = nullptr) {
        mPreparation = {};
        try {
            // Copied textures use @host routes, which require a registered owner.
            const auto owner = Ship::CrossRMRegistry::Get(host);
            mPreparation.owner = owner.get();
            if (!owner)
                return Unavailable("no-owner");
            const bool alt = owner->IsAltAssetsEnabled();
            mPreparation.alt = alt;
            const bool oot = std::strcmp(host, "oot") == 0;
            if (opaque && !MatchesSource(opaque, oot))
                return Unavailable("source-mismatch");
            const std::string sourceName = SourceRoom(oot);
            const std::string sourcePath = opaque ? reinterpret_cast<const char*>(opaque) + 7 : sourceName;
            const auto archive = owner->GetArchiveManager();
            // Reuse an unpaired Alt room's active geometry and replace only
            // its known material calls. Explicit Alt roots retain their exact
            // identity even with global Alt off; children keep normal routing.
            const bool useSource =
                sourcePath.starts_with("alt/") || (alt && archive->HasFile(std::string("alt/") + sourceName) &&
                                                   !archive->HasFile(std::string("alt/") + kRoom));
            if (!archive->HasFile(kRoom) && !(alt && archive->HasFile(std::string("alt/") + kRoom)))
                return Unavailable("graph-absent");
            const std::string rootPath = useSource ? sourcePath : kRoom;
            auto root = std::dynamic_pointer_cast<Fast::DisplayList>(owner->LoadResource(rootPath));
            mPreparation.root = root.get();
            if (!root)
                return Unavailable("root-not-display-list");
            mPreparation.reason = "ready";
            if (mRoom && mOwner == owner && mRoot == root && mAlt == alt && mUseSource == useSource &&
                mRootPath == rootPath && mHost == host)
                return true;
            Clear();
            Provider provider;
            provider.game = host;
            provider.display = [owner, sourceName, sourcePath](const std::string& path) -> Display {
                auto resource = std::dynamic_pointer_cast<Fast::DisplayList>(
                    owner->LoadResource(path == sourceName ? sourcePath : path));
                return resource ? Display{ resource->Instructions.data(), resource->Instructions.size(), resource }
                                : Display{};
            };
            provider.vertices = [owner](const std::string& path) -> Vertices {
                auto resource = std::dynamic_pointer_cast<Fast::Vertex>(owner->LoadResource(path));
                return resource ? Vertices{ resource->VertexList.data(), resource->VertexList.size(), resource }
                                : Vertices{};
            };
            provider.texture = [owner](const std::string& path) -> std::shared_ptr<void> {
                return std::dynamic_pointer_cast<Fast::Texture>(owner->LoadResource(path));
            };
            auto room = std::make_unique<Room>();
            if (!room->Load(provider, useSource))
                return Unavailable("graph-invalid");
            mRoom = std::move(room);
            mOwner = owner;
            mRoot = std::move(root);
            mAlt = alt;
            mUseSource = useSource;
            mRootPath = rootPath;
            mHost = host;
            return true;
        } catch (const std::exception&) { return Unavailable("exception"); }
    }
    Gfx* Opaque() const {
        return mRoom ? mRoom->Opaque() : nullptr;
    }
    void SetPalette(const Palette& palette) {
        if (mRoom)
            mRoom->SetPalette(palette);
    }
    void Clear() {
        mRoom.reset();
        mRoot.reset();
        mOwner.reset();
        mHost.clear();
        mRootPath.clear();
        mAlt = false;
        mUseSource = false;
    }

  private:
    PreparationTrace mPreparation;
    bool Unavailable(const char* reason) {
        mPreparation.reason = reason;
        Clear();
        return false;
    }
    std::unique_ptr<Room> mRoom;
    std::shared_ptr<Ship::ResourceManager> mOwner;
    std::shared_ptr<Fast::DisplayList> mRoot;
    std::string mHost;
    std::string mRootPath;
    bool mAlt = false;
    bool mUseSource = false;
};
} // namespace BlackMarketRoomMagic
