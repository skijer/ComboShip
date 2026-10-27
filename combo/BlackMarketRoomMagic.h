#pragma once

#include <libultraship/libultra/gbi.h>
#include <array>
#include <cstring>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace BlackMarketRoomMagic {
inline constexpr char kCvar[] = "gCombo.BlackMarket.MagicalRoomMaterials";
inline constexpr char kSourcePrefix[] = "custom/skiejer_market/";
inline constexpr char kPrefix[] = "custom/skiejer_market/room_magic_v1/";
inline constexpr char kRoom[] = "custom/skiejer_market/room_magic_v1/room";
inline constexpr char kTexture[] = "custom/skiejer_market/room_magic_v1/texture/arcane_stone";
inline constexpr std::array<const char*, 4> kCosmeticKeys = { "World.BlackMarketRoomViolet",
                                                              "World.BlackMarketRoomTeal", "World.BlackMarketRoomRose",
                                                              "World.BlackMarketRoomLavender" };
using Color = std::array<uint8_t, 4>;
using Palette = std::array<Color, 4>;
inline constexpr Palette kDefaultPalette = { Color{ 165, 148, 230, 255 }, Color{ 135, 194, 210, 255 },
                                             Color{ 204, 170, 215, 255 }, Color{ 175, 160, 216, 255 } };
inline std::string MaterialPath(size_t index, bool source = false) {
    return std::string(source ? kSourcePrefix : kPrefix) + "geometry/mat_mahouya_room_0DL_006548_GranmaShop" +
           static_cast<char>('0' + index / 10) + static_cast<char>('0' + index % 10);
}
inline const char* SourceRoom(bool oot) {
    return oot ? "custom/skiejer_market/room" : "custom/skiejer_market/mm_shop_presentation_v1/room";
}
inline bool MatchesSource(const Gfx* opaque, bool oot) {
    const auto* path = reinterpret_cast<const char*>(opaque);
    if (!path || std::strncmp(path, "__OTR__", 7) != 0)
        return false;
    path += 7;
    if (std::strncmp(path, "alt/", 4) == 0)
        path += 4;
    return std::strcmp(path, SourceRoom(oot)) == 0;
}
struct Display {
    const Gfx* data = nullptr;
    size_t count = 0;
    std::shared_ptr<void> owner;
};
struct Vertices {
    const Vtx* data = nullptr;
    size_t count = 0;
    std::shared_ptr<void> owner;
};
struct Provider {
    std::string game;
    std::function<Display(const std::string&)> display;
    std::function<Vertices(const std::string&)> vertices;
    std::function<std::shared_ptr<void>(const std::string&)> texture;
};

// Only private display lists are copied. Vertex bytes, texture pixels and every
// non-color command stay intact; the owning host's resources remain pinned.
class Room {
  public:
    Room() = default;
    Room(const Room&) = delete;
    Room& operator=(const Room&) = delete;
    bool Load(const Provider& provider, bool useSource = false) {
        Clear();
        mUseSource = useSource;
        try {
            if (provider.game != "oot" && provider.game != "mm")
                throw std::runtime_error("Invalid room material owner");
            // Material 17 is currently unused by the mesh, but belongs to the
            // same versioned contract and must also be compatible.
            for (size_t i = 0; i < mColors.size(); ++i)
                CopyList(MaterialPath(i), provider);
            mOpaque = CopyList(useSource ? SourceRoom(provider.game == "oot") : kRoom, provider);
            return true;
        } catch (const std::exception&) {
            Clear();
            return false;
        }
    }
    Gfx* Opaque() const {
        return mOpaque;
    }
    void SetPalette(const Palette& palette) {
        for (size_t i = 0; i < mColors.size(); ++i) {
            if (!mColors[i])
                continue;
            const auto& color = palette[i % palette.size()];
            mColors[i]->words.w1 =
                (uint32_t(color[0]) << 24) | (uint32_t(color[1]) << 16) | (uint32_t(color[2]) << 8) | 255;
        }
    }
    void Clear() {
        mOpaque = nullptr;
        mColors = {};
        mLists.clear();
        mActive.clear();
        mStrings.clear();
        mOwners.clear();
        mUseSource = false;
    }

  private:
    std::map<std::string, std::vector<Gfx>> mLists;
    std::set<std::string> mActive;
    std::deque<std::string> mStrings;
    std::vector<std::shared_ptr<void>> mOwners;
    std::array<Gfx*, 28> mColors{};
    Gfx* mOpaque = nullptr;
    bool mUseSource = false;

    static std::string Path(const char* value) {
        if (!value || value[0] == '>')
            throw std::runtime_error("Invalid room material reference");
        std::string path(value);
        if (path.starts_with("__OTR__"))
            path.erase(0, 7);
        return path;
    }
    Gfx* CopyList(const std::string& path, const Provider& provider) {
        std::string canonical = path.starts_with("alt/") ? path.substr(4) : path;
        if ((!canonical.starts_with(kPrefix) && !(mUseSource && canonical.starts_with(kSourcePrefix))) ||
            mActive.contains(path))
            throw std::runtime_error("Unsupported room material graph");
        // Unpaired Alt rooms retain their active mesh and UVs. Replace only
        // known material calls with the private, independently colored graph.
        if (mUseSource)
            for (size_t i = 0; i < mColors.size(); ++i)
                if (canonical == MaterialPath(i, true))
                    return CopyList(MaterialPath(i), provider);
        if (auto found = mLists.find(path); found != mLists.end())
            return found->second.data();
        if (mLists.size() >= 128)
            throw std::runtime_error("Room material graph exceeds expected size");
        const auto source = provider.display(path);
        if (!source.data || !source.owner || !source.count || source.count > 100000)
            throw std::runtime_error("Missing room material display list");
        mOwners.push_back(source.owner);
        mActive.insert(path);
        auto& list = mLists[path];
        list.reserve(source.count);
        size_t material = mColors.size(), primitive = source.count;
        for (size_t i = 0; i < mColors.size(); ++i)
            if (canonical == MaterialPath(i))
                material = i;
        for (size_t i = 0; i < source.count; ++i) {
            Gfx command = source.data[i];
            const auto opcode = command.words.w0 >> 24;
            if (opcode == G_DL_OTR_FILEPATH) {
                command.words.w0 = (command.words.w0 & 0x00ffffff) | (uint32_t(G_DL) << 24);
                command.words.w1 = reinterpret_cast<uintptr_t>(
                    CopyList(Path(reinterpret_cast<const char*>(command.words.w1)), provider));
            } else if (opcode == G_VTX_OTR_FILEPATH) {
                if (++i >= source.count)
                    throw std::runtime_error("Truncated room vertex command");
                const auto vertices = provider.vertices(Path(reinterpret_cast<const char*>(command.words.w1)));
                const auto& args = source.data[i].words;
                const size_t count = args.w0, offset = args.w1 & 0xffff, destination = args.w1 >> 16;
                if (!vertices.data || !vertices.owner || !count || count > 32 || destination + count > 32 ||
                    offset > vertices.count || count > vertices.count - offset)
                    throw std::runtime_error("Invalid room vertex range");
                mOwners.push_back(vertices.owner);
                Gfx load = { gsSPVertex(vertices.data + offset, count, destination) };
                list.push_back(load);
                command = { gsSPNoOp() };
            } else if (opcode == G_SETTIMG_OTR_FILEPATH) {
                const auto texture = Path(reinterpret_cast<const char*>(command.words.w1));
                if (texture != kTexture && texture != std::string("alt/") + kTexture)
                    throw std::runtime_error("Unexpected room material texture");
                auto owner = provider.texture(texture);
                if (!owner)
                    throw std::runtime_error("Missing room material texture");
                mOwners.push_back(std::move(owner));
                mStrings.push_back("__OTR__@" + provider.game + ":" + texture);
                command.words.w1 = reinterpret_cast<uintptr_t>(mStrings.back().c_str());
            } else if (opcode == G_SETPRIMCOLOR && material < mColors.size()) {
                if (primitive != source.count || mColors[material])
                    throw std::runtime_error("Ambiguous room palette command");
                primitive = list.size();
            } else if (opcode == G_DL || opcode == G_VTX || opcode == G_MTX || opcode == G_SETTIMG || opcode == 0x20 ||
                       opcode == 0x31 || opcode == 0x32 || opcode == 0x33 || opcode == 0x35 || opcode == 0x36 ||
                       opcode == 0x42) {
                throw std::runtime_error("Unsupported room material encoding");
            }
            list.push_back(command);
            if (opcode == G_ENDDL) {
                if (i + 1 != source.count || (material < mColors.size() && primitive == source.count))
                    throw std::runtime_error("Incompatible room material display list");
                if (material < mColors.size())
                    mColors[material] = &list[primitive];
                mActive.erase(path);
                return list.data();
            }
        }
        throw std::runtime_error("Unterminated room material display list");
    }
};
} // namespace BlackMarketRoomMagic
