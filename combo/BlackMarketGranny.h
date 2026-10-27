#pragma once

// Only paths and private counter/clearance recipes live in source. Room/texture data is
// supplied by the user's Djipi archive; no resource is written back or exported.
#include <libultraship/libultra/gbi.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace BlackMarketGranny {
inline constexpr char kCvar[] = "gCombo.BlackMarket.RegularGrannyInterior";
inline constexpr char kOpaque[] = "scenes/shared/mahouya_scene/mahouya_room_0DL_006548";
inline constexpr char kTranslucent[] = "scenes/shared/mahouya_scene/mahouya_room_0DL_006A78";
inline constexpr char kCounterVertices[] = "scenes/shared/mahouya_scene/mahouya_room_0DL_006548_vtx_2";
inline constexpr char kCounterDraw[] = "scenes/shared/mahouya_scene/mahouya_room_0DL_006548_tri_2";
inline constexpr char kScene[] = "scenes/shared/mahouya_scene/mahouya_scene";
inline constexpr char kRoom[] = "scenes/shared/mahouya_scene/mahouya_room_0";

inline std::string Exact(std::string path) {
    if (path.starts_with("__OTR__"))
        path.erase(0, 7);
    if (!path.starts_with("alt/"))
        path.insert(0, "alt/");
    return path;
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

} // namespace BlackMarketGranny
#include "BlackMarketGrannyRefinement.h"
namespace BlackMarketGranny {
class Room {
  public:
    Room() = default;
    Room(const Room&) = delete;
    Room& operator=(const Room&) = delete;
    Room(Room&&) = delete;
    Room& operator=(Room&&) = delete;

    bool Load(const Provider& provider, bool removeRearPots = false) {
        Clear();
        try {
            if (provider.game != "oot" && provider.game != "mm")
                Fail("Unknown resource owner");
            mRearPots.Reset(removeRearPots);
            auto* opaque = CopyList(Exact(kOpaque), provider);
            mRearPots.Validate();
            if (mCounter.size() != 1197 || !mLists.contains(Exact(kCounterDraw)))
                Fail("Incompatible Djipi Granny counter layout");
            mOpaque = opaque;
            return true;
        } catch (const std::exception& e) {
            const std::string error = e.what();
            Clear();
            mError = error;
            return false;
        }
    }
    Gfx* Opaque() const {
        return mOpaque;
    }
    const std::vector<Vtx>& Counter() const {
        return mCounter;
    }
    const std::string& Error() const {
        return mError;
    }

  private:
    std::map<std::string, std::vector<Gfx>> mLists;
    std::map<std::string, Vertices> mVertices;
    std::set<std::string> mActive;
    std::deque<std::string> mStrings;
    std::vector<std::shared_ptr<void>> mOwners;
    std::vector<Vtx> mCounter;
    Gfx* mOpaque = nullptr;
    std::string mError;
    RearPots mRearPots;

    [[noreturn]] static void Fail(const std::string& error) {
        throw std::runtime_error(error);
    }
    void Clear() {
        mOpaque = nullptr;
        mLists.clear();
        mVertices.clear();
        mActive.clear();
        mStrings.clear();
        mOwners.clear();
        mCounter.clear();
        mError.clear();
        mRearPots.Reset();
    }
    void RaiseCounter(const Vertices& source) {
        if (source.count != 1189)
            Fail("Incompatible Djipi Granny vertex count");
        constexpr unsigned indices[] = { 603,  604,  605,  606,  607,  634,  635, 636, 637, 655, 656, 657,  658,
                                         659,  660,  661,  662,  663,  664,  665, 666, 667, 668, 669, 670,  671,
                                         672,  673,  688,  689,  690,  691,  692, 693, 694, 695, 696, 697,  698,
                                         699,  700,  701,  702,  703,  704,  705, 706, 707, 708, 709, 710,  711,
                                         712,  713,  714,  715,  716,  717,  718, 719, 720, 721, 722, 723,  724,
                                         725,  726,  727,  728,  729,  730,  731, 732, 733, 734, 735, 1180, 1181,
                                         1182, 1183, 1184, 1185, 1186, 1187, 1188 };
        constexpr unsigned lowJoin[] = { 661, 726, 727, 730, 731, 734, 735 };
        mCounter.assign(source.data, source.data + source.count);
        unsigned moved = 0;
        for (unsigned index : indices) {
            auto& v = mCounter[index].v;
            if (v.ob[0] < -160 || v.ob[0] > 58 || v.ob[2] < 93 || v.ob[2] > 123 ||
                (v.ob[1] != 0 && v.ob[1] != 26 && v.ob[1] != 30))
                Fail("Incompatible Djipi Granny counter component");
            bool fixedJoin = false;
            for (unsigned fixed : lowJoin)
                fixedJoin |= index == fixed;
            if (v.ob[1] && v.ob[0] >= -51 && !fixedJoin) {
                v.ob[1] += 11;
                ++moved;
            }
        }
        if (moved != 46)
            Fail("Incompatible Djipi Granny counter shape");
        // Close the exposed step using the donor's own end-panel UVs and seam
        // vertex colors. The fixed left book station is never moved.
        const auto& end = source.data[606].v;
        const auto& trim = source.data[607].v;
        const short oldS = end.tc[0];
        const short raisedS = oldS - std::lround(11.0 * (trim.tc[0] - oldS) / 4.0);
        auto cap = [&](unsigned from, short y, short s, short t) {
            Vtx v = source.data[from];
            v.v.ob[1] = y;
            v.v.tc[0] = s;
            v.v.tc[1] = t;
            mCounter.push_back(v);
        };
        const auto& front = source.data[726].v;
        const auto& back = source.data[727].v;
        cap(726, front.ob[1], oldS, front.tc[1]);
        cap(726, front.ob[1] + 11, raisedS, front.tc[1]);
        cap(727, back.ob[1] + 11, raisedS, back.tc[1]);
        cap(727, back.ob[1], oldS, back.tc[1]);
        cap(735, source.data[735].v.ob[1], trim.tc[0], source.data[603].v.tc[1]);
        cap(735, front.ob[1], oldS, source.data[603].v.tc[1]);
        cap(734, back.ob[1], oldS, source.data[604].v.tc[1]);
        cap(734, source.data[734].v.ob[1], trim.tc[0], source.data[604].v.tc[1]);
    }

    Gfx* CopyList(const std::string& path, const Provider& provider) {
        if (mActive.contains(path))
            Fail("Recursive Djipi display list: " + path);
        if (auto found = mLists.find(path); found != mLists.end())
            return found->second.data();
        if (mLists.size() >= 256)
            Fail("Djipi room graph exceeds expected size");
        const auto source = provider.display(path);
        if (!source.data || !source.owner || !source.count || source.count > 100000)
            Fail("Missing Djipi display list: " + path);
        mOwners.push_back(source.owner);
        mActive.insert(path);
        auto& list = mLists[path];
        list.reserve(source.count + 4);
        std::array<int, 32> cache;
        cache.fill(-1);
        for (size_t i = 0; i < source.count; ++i) {
            Gfx command = source.data[i];
            const auto opcode = command.words.w0 >> 24;
            if (opcode == 0x27) { // G_DL_OTR_FILEPATH, preserving PUSH/NOPUSH.
                const auto* child = reinterpret_cast<const char*>(command.words.w1);
                if (!child || child[0] == '>')
                    Fail("Unsupported Djipi display-list reference");
                command.words.w0 = (command.words.w0 & 0x00ffffff) | (G_DL << 24);
                command.words.w1 = reinterpret_cast<uintptr_t>(CopyList(Exact(child), provider));
            } else if (opcode == 0x24) { // XML vertex commands occupy two Gfx entries.
                if (++i >= source.count)
                    Fail("Truncated Djipi vertex command");
                const auto* name = reinterpret_cast<const char*>(command.words.w1);
                if (!name)
                    Fail("Missing Djipi vertex path");
                const auto vertexPath = Exact(name);
                if (!mVertices.contains(vertexPath)) {
                    const auto data = provider.vertices(vertexPath);
                    if (!data.data || !data.owner)
                        Fail("Missing Djipi vertices: " + vertexPath);
                    mVertices[vertexPath] = data;
                    mOwners.push_back(data.owner);
                    if (vertexPath == Exact(kCounterVertices))
                        RaiseCounter(data);
                }
                const auto& vertices = mVertices.at(vertexPath);
                const auto& args = source.data[i].words;
                const size_t count = args.w0, offset = args.w1 & 0xffff, destination = args.w1 >> 16;
                if (!count || count > 32 || destination + count > 32 || offset > vertices.count ||
                    count > vertices.count - offset)
                    Fail("Invalid Djipi vertex range");
                mRearPots.VerticesFor(path, vertexPath, vertices, count, offset, destination, cache);
                const Vtx* data = vertexPath == Exact(kCounterVertices) ? mCounter.data() : vertices.data;
                Gfx load = { gsSPVertex(data + offset, count, destination) };
                list.push_back(load);
                command = { gsSPNoOp() };
            } else if (opcode == 0x25) { // Preserve every texture parameter; pin path/owner.
                const auto* name = reinterpret_cast<const char*>(command.words.w1);
                if (!name)
                    Fail("Missing Djipi texture path");
                const auto texture = Exact(name);
                auto resource = provider.texture(texture);
                if (!resource)
                    Fail("Missing Djipi 3DS texture: " + texture);
                mOwners.push_back(std::move(resource));
                mStrings.push_back("__OTR__@" + provider.game + ":" + texture);
                command.words.w1 = reinterpret_cast<uintptr_t>(mStrings.back().c_str());
            } else if (opcode == 0x20 || opcode == 0x31 || opcode == 0x32 || opcode == 0x33 || opcode == 0x35 ||
                       opcode == 0x36 || opcode == 0x42) {
                Fail("Unsupported Djipi map encoding; install the original scene maps");
            }
            if (opcode == G_ENDDL && path == Exact(kCounterDraw)) {
                if (mCounter.size() != 1197)
                    Fail("Missing Djipi counter vertices");
                Gfx cap[] = {
                    gsSPVertex(mCounter.data() + 1189, 4, 0), gsSP1Triangle(0, 2, 1, 0), gsSP1Triangle(0, 3, 2, 0),
                    gsSPVertex(mCounter.data() + 1193, 4, 0), gsSP1Triangle(0, 1, 2, 0), gsSP1Triangle(0, 2, 3, 0)
                };
                list.insert(list.end(), std::begin(cap), std::end(cap));
            }
            list.push_back(mRearPots.Filter(path, command, cache));
            if (opcode == G_ENDDL) {
                if (i + 1 != source.count)
                    Fail("Unexpected trailing Djipi commands");
                mActive.erase(path);
                return list.data();
            }
        }
        Fail("Unterminated Djipi display list: " + path);
    }
};
} // namespace BlackMarketGranny
