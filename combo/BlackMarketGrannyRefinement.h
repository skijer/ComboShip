#pragma once
// Included after the Granny Provider types. Recipes only; donor bytes stay owned
// by their installed archives and the RAM copies preserve every other command.
namespace BlackMarketGranny {
class RearPots {
  public:
    void Reset(bool enabled = false) {
        mEnabled = enabled;
        mSeen = {};
        mFaces = {};
    }
    bool Selected(const std::string& list) const {
        if (mEnabled)
            for (const auto& piece : kPieces)
                if (list == List(piece.suffix))
                    return true;
        return false;
    }
    void VerticesFor(const std::string& list, const std::string& path, const Vertices& vertices, size_t count,
                     size_t offset, size_t destination, std::array<int, 32>& cache) {
        if (!Selected(list))
            return;
        for (size_t n = 0; n < count; ++n) {
            cache[destination + n] = 0;
            for (size_t i = 0; i < kPieces.size(); ++i) {
                const auto& piece = kPieces[i];
                if (list != List(piece.suffix))
                    continue;
                if (path != VertexPath(piece.suffix))
                    throw std::runtime_error("Changed Granny pot vertex owner");
                const auto& v = vertices.data[offset + n].v.ob;
                bool inside = true;
                for (int axis = 0; axis < 3; ++axis)
                    inside &= v[axis] >= piece.low[axis] && v[axis] <= piece.high[axis];
                if (inside) {
                    if (cache[destination + n])
                        throw std::runtime_error("Overlapping Granny pot components");
                    cache[destination + n] = i + 1;
                    mSeen[i].insert(offset + n);
                }
            }
        }
    }
    Gfx Filter(const std::string& list, Gfx command, const std::array<int, 32>& cache) {
        const auto op = command.words.w0 >> 24;
        if (!Selected(list) || (op != 5 && op != 6 && op != G_TRI1_OTR))
            return command;
        const uintptr_t words[2] = { command.words.w0, command.words.w1 };
        int keep[2]{}, kept = 0;
        for (int face = 0; face < (op == 6 ? 2 : 1); ++face) {
            int selected = 0, labels[3];
            for (int v = 0; v < 3; ++v) {
                // XML Triangle1 stores raw indices across both words, unlike
                // native packed triangles' doubled byte-sized indices.
                size_t index;
                if (op == G_TRI1_OTR)
                    index = v == 0 ? words[0] & 0xffffff : (words[1] >> (v == 1 ? 16 : 0)) & 0xffff;
                else {
                    const auto packed = (words[face] >> (16 - 8 * v)) & 255;
                    if (packed & 1)
                        throw std::runtime_error("Changed Granny pot triangle encoding");
                    index = packed / 2;
                }
                if (index >= cache.size() || cache[index] < 0)
                    throw std::runtime_error("Changed Granny pot triangle encoding");
                labels[v] = cache[index];
                if (labels[v])
                    selected = labels[v];
            }
            if (selected) {
                if (labels[0] != selected || labels[1] != selected || labels[2] != selected)
                    throw std::runtime_error("Granny pot triangle crosses an unaudited component");
                ++mFaces[selected - 1];
            } else
                keep[kept++] = face;
        }
        if (!kept)
            return { gsSPNoOp() };
        if (kept == 1 && op == 6) {
            // Flags have already been folded into the factory's vertex order.
            command.words.w0 = (5u << 24) | (words[keep[0]] & 0xffffff);
            command.words.w1 = 0;
        }
        return command;
    }
    void Validate() const {
        if (mEnabled)
            for (size_t i = 0; i < kPieces.size(); ++i)
                if (mSeen[i].size() != kPieces[i].vertices || mFaces[i] != kPieces[i].faces)
                    throw std::runtime_error("Incompatible Granny rear pot components");
    }

  private:
    struct Piece {
        int suffix;
        std::array<short, 3> low, high;
        size_t vertices, faces;
    };
    inline static constexpr std::array<Piece, 6> kPieces{ { { 0, { 49, 37, 24 }, { 97, 37, 72 }, 25, 36 },
                                                            { 2, { -120, 0, 0 }, { -62, 54, 54 }, 116, 106 },
                                                            { 2, { -103, 60, 42 }, { -92, 62, 53 }, 20, 16 },
                                                            { 2, { 42, 0, 20 }, { 105, 39, 79 }, 94, 88 },
                                                            { 6, { -104, 51, 27 }, { -53, 62, 53 }, 80, 60 },
                                                            { 18, { -114, 51, 2 }, { -69, 51, 47 }, 30, 36 } } };
    static std::string List(int suffix) {
        return Exact(std::string(kOpaque) + "_tri_" + std::to_string(suffix));
    }
    static std::string VertexPath(int suffix) {
        return Exact(std::string(kOpaque) + "_vtx_" + std::to_string(suffix));
    }
    bool mEnabled = false;
    std::array<std::set<size_t>, 6> mSeen;
    std::array<size_t, 6> mFaces{};
};

inline constexpr char kVaporLeaf[] = "scenes/shared/mahouya_scene/mahouya_room_0DL_006940";
inline constexpr char kVaporVertices[] = "scenes/shared/mahouya_scene/mahouya_room_0Vtx_0065C0";
class Vapor {
  public:
    Vapor() = default;
    Vapor(const Vapor&) = delete;
    Vapor& operator=(const Vapor&) = delete;
    bool Load(const Display& root, const Display& leaf, const Vertices& vertices) {
        mRoot.clear();
        mLeaf.clear();
        mOwners = {};
        if (!root.data || !leaf.data || !vertices.data || !root.owner || !leaf.owner || !vertices.owner ||
            root.count != 5 || leaf.count != 45 || vertices.count != 56)
            return false;
        // Exact original native root, vertex loads and twelve independent steam
        // quads. The six rear quads are vertex24..47; vertex0..23 is entrance steam.
        if (root.data[0].words.w0 != 0x33000000 || root.data[2].words.w0 != 0x31000000 ||
            root.data[3].words.w0 != 0x9db10eea || root.data[3].words.w1 != 0x0253d372 ||
            root.data[4].words.w0 != 0xdf000000 || leaf.data[4].words.w0 != 0x32008010 ||
            leaf.data[4].words.w1 != 0x300 || leaf.data[28].words.w0 != 0x32020040 || leaf.data[28].words.w1 != 0 ||
            leaf.data[38].words.w0 != 0x32010020 || leaf.data[38].words.w1 != 0x200 ||
            leaf.data[44].words.w0 != 0xdf000000)
            return false;
        for (int command : { 5, 29, 39 })
            if (leaf.data[command].words.w0 != 0xfc4b43fe || leaf.data[command].words.w1 != 0xf8565a38)
                return false;
        constexpr short bounds[6][6] = { { 71, 37, 42, 98, 200, 45 },    { 45, 37, 47, 73, 200, 48 },
                                         { 59, 37, 66, 86, 200, 67 },    { -104, 51, 43, -79, 218, 43 },
                                         { -117, 51, 24, -91, 218, 25 }, { -93, 51, 20, -68, 218, 22 } };
        for (int quad = 0; quad < 6; ++quad) {
            short low[3]{ 32767, 32767, 32767 }, high[3]{ -32767, -32767, -32767 };
            for (int vertex = 0; vertex < 4; ++vertex)
                for (int axis = 0; axis < 3; ++axis) {
                    const auto value = vertices.data[24 + quad * 4 + vertex].v.ob[axis];
                    low[axis] = std::min(low[axis], value);
                    high[axis] = std::max(high[axis], value);
                }
            for (int axis = 0; axis < 3; ++axis)
                if (low[axis] != bounds[quad][axis] || high[axis] != bounds[quad][axis + 3])
                    return false;
        }
        mRoot.assign(root.data, root.data + root.count);
        mLeaf.assign(leaf.data, leaf.data + leaf.count);
        for (int quad = 0; quad < 12; ++quad) {
            const int first = quad < 8 ? quad * 4 : (quad - 8) * 4;
            const int command = quad < 8 ? 30 + quad : 40 + quad - 8;
            const auto& g = leaf.data[command];
            const uint32_t expected0 = (6u << 24) | ((first * 2) << 16) | ((first + 1) * 2 << 8) | ((first + 2) * 2);
            const uint32_t expected1 = ((first * 2) << 16) | ((first + 2) * 2 << 8) | ((first + 3) * 2);
            if (g.words.w0 != expected0 || g.words.w1 != expected1) {
                mRoot.clear();
                mLeaf.clear();
                return false;
            }
            if (quad >= 6)
                mLeaf[command] = { gsSPNoOp() };
        }
        mRoot[2] = { gsSPDisplayList(mLeaf.data()) };
        mRoot[3] = { gsSPNoOp() };
        mOwners = { root.owner, leaf.owner, vertices.owner };
        return true;
    }
    Gfx* Root() {
        return mRoot.empty() ? nullptr : mRoot.data();
    }

  private:
    std::vector<Gfx> mRoot, mLeaf;
    std::array<std::shared_ptr<void>, 3> mOwners;
};
} // namespace BlackMarketGranny
