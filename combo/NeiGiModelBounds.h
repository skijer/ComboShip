#pragma once

#include "../soh/soh/Enhancements/randomizer/NeiGiFrameFit.h"
#include "DinSwordGiResources.h"
#include <fast/resource/type/DisplayList.h>
#include <fast/resource/type/Matrix.h>
#include <fast/resource/type/Vertex.h>
#include <fast/lus_gbi.h>
#ifdef MM_BUILD_DLL
#include "../mm/2s2h/resource/type/Array.h"
#else
#include "../soh/soh/resource/type/Array.h"
#endif
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <vector>

namespace NeiGi {
// Read the selected resource graph, including XML filepath and binary hash
// calls. This deliberately does not substitute the authored GI catalog bounds.
// Resource modelview matrices are part of the selected geometry. Dynamic
// segment geometry or matrices that replace the caller's pose remain unsupported.
template <class Load, bool TrackDepth = false> class ModelBoundsReader {
  public:
    ModelBoundsReader(Load& load, float tilt) : mLoad(load), mSin(std::sin(tilt)), mCos(std::cos(tilt)) {
    }

    bool Read(const char* path, FrameBounds& out, bool allowEmpty = false) {
        mComplete = false;
        const auto root = std::dynamic_pointer_cast<Fast::DisplayList>(mLoad(path));
        if (!Walk(root, 0) || !mStack.empty())
            return false;
        if (!mVertices) {
            out = {};
            return allowEmpty; // Native sword detail/color passes can be geometry-free.
        }
        out = { "selected_sword", { 0, mLow, 0 }, { 0, mHigh, 0 }, 2.f * mRadius, Kind::Neutral, {} };
        return mComplete = (mHigh > mLow || (TrackDepth && mDepthHigh > mDepthLow)) && mRadius > 0.f;
    }

    // Observational XYZ accessor: retain Read's established radial/Y fit output.
    bool ExactBounds(Point& minimum, Point& maximum) const {
        static_assert(TrackDepth, "Exact frame queries track all three axes");
        if (!mComplete || !mVertices)
            return false;
        minimum = { mWidthLow, mLow, mDepthLow };
        maximum = { mWidthHigh, mHigh, mDepthHigh };
        return true;
    }

    float ShieldTiltX() const {
        static_assert(TrackDepth);
        // Correct an XZ-oriented shield without tilting an already upright
        // replacement. The margin leaves ordinary thick/angled meshes alone.
        return mDepthHigh - mDepthLow > 2.f * (mHigh - mLow) ? 1.5707963267948966f : 0.f;
    }

    float ShieldRollZ() const {
        static_assert(TrackDepth);
        // Fados' worn Ikana mesh has a -X chin inside a nearly round rim.
        // Match its selected vertex stream, not a shared filename or relaxed
        // span threshold. Resource transforms and edited meshes stay generic.
        if (!mHasModelMatrix && mShieldVertexHashValid && mVertices == 2290 &&
            mShieldVertexHash == 0x015b5334888c6583ull)
            return 1.5707963267948966f;
        // After standing an arm-local XZ mesh up, its face can still point
        // sideways. Use spans, not distance from the off-center grip origin.
        // Bounds cannot identify a sideways face inside a square rim. Leave
        // ambiguous frames alone; correct only a clearly longer X axis.
        const float height = ShieldTiltX() != 0.f ? mDepthHigh - mDepthLow : mHigh - mLow;
        // The worn MM shield's face is authored with its chin toward -X.
        return mWidthHigh - mWidthLow > 1.05f * height ? 1.5707963267948966f : 0.f;
    }

    bool RestoresModelView() const {
        for (int row = 0; row < 4; ++row)
            for (int column = 0; column < 4; ++column)
                if (std::abs(mMatrix[row][column] - (row == column ? 1.f : 0.f)) > .00001f)
                    return false;
        return true;
    }

  private:
    template <class Visit> bool WithVertexData(const std::shared_ptr<Ship::IResource>& resource, Visit visit) {
        if (const auto vertices = std::dynamic_pointer_cast<Fast::Vertex>(resource))
            return visit(vertices->VertexList);
        // Binary sword packs use the host's SOH_Array factory; XML vertices
        // use Fast::Vertex. Scalar arrays are not geometry, and the declared
        // count must match the storage filled by the binary factory.
        if (const auto array = std::dynamic_pointer_cast<SOH::Array>(resource);
            array && array->ArrayType == SOH::ArrayResourceType::Vertex && array->ArrayCount == array->Vertices.size())
            return visit(array->Vertices);
        return false;
    }

    bool Vertices(const std::shared_ptr<Ship::IResource>& resource, size_t offset, size_t count,
                  bool byteOffset = false, size_t cacheIndex = 0) {
        return WithVertexData(resource, [&](const auto& vertices) {
            if (byteOffset) {
                const size_t stride = sizeof(vertices[0]);
                // The interpreter caches a vertex pointer in hash packets.
                // Validate it against this exact selected resource's storage.
                if (offset > 0xfffffu) {
                    const uintptr_t base = reinterpret_cast<uintptr_t>(vertices.data());
                    if (offset < base || offset - base >= vertices.size() * stride)
                        return false;
                    offset -= base;
                }
                if (offset % stride)
                    return false;
                offset /= stride;
            }
            if (count == 0 || offset > vertices.size() || count > vertices.size() - offset ||
                (mVertices += count) > 65536)
                return false;
            for (size_t i = offset; i < offset + count; ++i) {
                const auto& p = vertices[i].v.ob;
                float transformed[3]{};
                for (int axis = 0; axis < 3; ++axis) {
                    transformed[axis] = mMatrix[3][axis];
                    for (int input = 0; input < 3; ++input)
                        transformed[axis] += p[input] * mMatrix[input][axis];
                    if (!std::isfinite(transformed[axis]))
                        return false;
                }
                const float x = transformed[0] * mCos - transformed[1] * mSin;
                const float y = transformed[0] * mSin + transformed[1] * mCos;
                if constexpr (requires { mLoad.SurfaceVertex(transformed, cacheIndex); }) {
                    if (!mLoad.SurfaceVertex(transformed, cacheIndex + i - offset))
                        return false;
                }
                mLow = std::min(mLow, y);
                mHigh = std::max(mHigh, y);
                mRadius = std::max(mRadius, std::hypot(x, transformed[2]));
                if constexpr (TrackDepth) {
                    if (mShieldVertexHashValid) {
                        for (int axis = 0; axis < 3; ++axis) {
                            const float coordinate = p[axis];
                            if (coordinate < -32768.f || coordinate > 32767.f || std::trunc(coordinate) != coordinate) {
                                mShieldVertexHashValid = false;
                                break;
                            }
                            const uint16_t word = static_cast<uint16_t>(static_cast<int16_t>(coordinate));
                            mShieldVertexHash = (mShieldVertexHash ^ (word & 0xffu)) * 0x100000001b3ull;
                            mShieldVertexHash = (mShieldVertexHash ^ (word >> 8)) * 0x100000001b3ull;
                        }
                    }
                    mWidthLow = std::min(mWidthLow, x);
                    mWidthHigh = std::max(mWidthHigh, x);
                    mDepthLow = std::min(mDepthLow, transformed[2]);
                    mDepthHigh = std::max(mDepthHigh, transformed[2]);
                }
            }
            return true;
        });
    }

    using Matrix = std::array<std::array<float, 4>, 4>;
    template <class Key> std::shared_ptr<Ship::IResource> MatrixResource(Key key) {
        if constexpr (requires { mLoad.LoadMatrix(key); })
            return mLoad.LoadMatrix(key);
        else
            return mLoad(key);
    }
    bool ApplyMatrix(const std::shared_ptr<Ship::IResource>& resource, uint32_t flags) {
        const auto matrix = std::dynamic_pointer_cast<Fast::Matrix>(resource);
        // LOAD/projection matrices replace the host pose; an outer GI fit
        // cannot reliably contain such geometry without changing the asset.
        if (!matrix || (flags & ~(uint32_t)G_MTX_PUSH) || mStack.size() >= 32)
            return false;
        mHasModelMatrix = true;
        Matrix decoded{};
#ifdef GBI_FLOATS
        for (int row = 0; row < 4; ++row)
            for (int column = 0; column < 4; ++column)
                decoded[row][column] = matrix->Matrx.mf[row][column];
#else
        const auto* words = reinterpret_cast<const uint32_t*>(&matrix->Matrx);
        for (int row = 0; row < 4; ++row) {
            for (int column = 0; column < 4; column += 2) {
                const int index = row * 2 + column / 2;
                decoded[row][column] = int32_t((words[index] & 0xffff0000u) | (words[index + 8] >> 16)) / 65536.f;
                decoded[row][column + 1] = int32_t((words[index] << 16) | (words[index + 8] & 0xffffu)) / 65536.f;
            }
        }
#endif
        if (decoded[0][3] != 0 || decoded[1][3] != 0 || decoded[2][3] != 0 || decoded[3][3] != 1)
            return false;
        for (const auto& row : decoded)
            for (float value : row)
                if (!std::isfinite(value))
                    return false;
        if (flags & G_MTX_PUSH)
            mStack.push_back(mMatrix);
        Matrix combined{};
        // Match the interpreter's row-vector resource * current modelview.
        for (int row = 0; row < 4; ++row)
            for (int column = 0; column < 4; ++column)
                for (int input = 0; input < 4; ++input)
                    combined[row][column] += decoded[row][input] * mMatrix[input][column];
        mMatrix = combined;
        return true;
    }

    bool Walk(const std::shared_ptr<Fast::DisplayList>& list, unsigned depth) {
        if (!list || list->UCode != ucode_f3dex2 || depth >= 32)
            return false;
        struct Scope {
            Load& load;
            Scope(Load& loader, const std::shared_ptr<Fast::DisplayList>& resource) : load(loader) {
                if constexpr (requires {
                                  load.Enter(resource);
                                  load.Leave();
                              })
                    load.Enter(resource);
            }
            ~Scope() {
                if constexpr (requires {
                                  load.Enter(std::shared_ptr<Fast::DisplayList>{});
                                  load.Leave();
                              })
                    load.Leave();
            }
        } scope(mLoad, list);
        const auto& commands = list->Instructions;
        for (size_t i = 0; i < commands.size(); ++i) {
            if (++mCommands > 16384)
                return false;
            const auto& command = commands[i];
            const uint32_t op = command.words.w0 >> 24;
            if (op == G_ENDDL)
                return true;
            if (op == G_DL_OTR_FILEPATH) {
                if (!Walk(std::dynamic_pointer_cast<Fast::DisplayList>(
                              mLoad(reinterpret_cast<const char*>(command.words.w1))),
                          depth + 1))
                    return false;
                if ((command.words.w0 >> 16) & 1)
                    return true;
            } else if (op == G_VTX_OTR_FILEPATH) {
                if (++i == commands.size())
                    return false;
                const auto& args = commands[i];
                if (!Vertices(mLoad(reinterpret_cast<const char*>(command.words.w1)), args.words.w1 & 0xffffu,
                              args.words.w0, false, (args.words.w1 >> 16) & 0xffffu))
                    return false;
            } else if (op == G_MTX_OTR_FILEPATH) {
                if (!ApplyMatrix(MatrixResource(reinterpret_cast<const char*>(command.words.w1)),
                                 (command.words.w0 & 0xffu) ^ G_MTX_PUSH))
                    return false;
            } else if (op == G_POPMTX) {
                const size_t count = command.words.w1 / 64;
                if (!count || command.words.w1 % 64 || count > mStack.size())
                    return false;
                mMatrix = mStack[mStack.size() - count];
                mStack.resize(mStack.size() - count);
            } else if (op == G_DL_OTR_HASH || op == G_VTX_OTR_HASH || op == G_MTX_OTR) {
                if (++i == commands.size())
                    return false;
                const uint64_t hash = (uint64_t(commands[i].words.w0) << 32) | uint32_t(commands[i].words.w1);
                const auto resource = op == G_MTX_OTR ? MatrixResource(hash) : mLoad(hash);
                if (op == G_DL_OTR_HASH) {
                    if (!Walk(std::dynamic_pointer_cast<Fast::DisplayList>(resource), depth + 1))
                        return false;
                    if ((command.words.w0 >> 16) & 1)
                        return true;
                } else if (op == G_MTX_OTR) {
                    if (!ApplyMatrix(resource, (command.words.w0 & 0xffu) ^ G_MTX_PUSH))
                        return false;
                } else {
                    const size_t count = (command.words.w0 >> 12) & 0xff;
                    const size_t end = (command.words.w0 >> 1) & 0x7f;
                    if (!Vertices(resource, command.words.w1, count, true, end - count))
                        return false;
                }
            } else if (op == G_TRI1_OTR) {
                if constexpr (requires { mLoad.SurfaceTriangle(size_t{}, size_t{}, size_t{}); }) {
                    // XML Triangle1 uses direct indices in separate words;
                    // the native packed TRI1/TRI2 encoding below uses index*2.
                    if (!mLoad.SurfaceTriangle(command.words.w0 & 0xffffffu, (command.words.w1 >> 16) & 0xffffu,
                                               command.words.w1 & 0xffffu))
                        return false;
                }
            } else if (op == G_TRI1 || op == G_TRI2 || op == G_QUAD) {
                if constexpr (requires { mLoad.SurfaceTriangle(size_t{}, size_t{}, size_t{}); }) {
                    const auto triangle = [&](uintptr_t word) {
                        const size_t a = (word >> 16) & 0xff, b = (word >> 8) & 0xff, c = word & 0xff;
                        return !(a & 1 || b & 1 || c & 1) && mLoad.SurfaceTriangle(a / 2, b / 2, c / 2);
                    };
                    if (!triangle(command.words.w0) || ((op == G_TRI2 || op == G_QUAD) && !triangle(command.words.w1)))
                        return false;
                }
            } else if (op == G_DL) {
                // Only a caller that installs a known geometry-free material
                // segment may admit its raw call. Other raw geometry fails.
                if constexpr (requires { mLoad.IsMaterialDisplayList(command.words.w1); }) {
                    if (mLoad.IsMaterialDisplayList(command.words.w1))
                        continue;
                }
                return false;
            } else if (op == G_MTX || op == G_VTX || op == G_BRANCH_Z_OTR || op == G_BRANCH_Z) {
                return false;
            } else if (op == G_SETTIMG_OTR_HASH || op == G_MARKER || op == G_MOVEMEM_OTR) {
                if (++i == commands.size())
                    return false;
            }
        }
        return false;
    }

    Load& mLoad;
    float mSin, mCos, mLow = std::numeric_limits<float>::max(), mHigh = -std::numeric_limits<float>::max();
    float mRadius = 0;
    float mWidthLow = std::numeric_limits<float>::max(), mWidthHigh = -std::numeric_limits<float>::max();
    float mDepthLow = std::numeric_limits<float>::max(), mDepthHigh = -std::numeric_limits<float>::max();
    size_t mVertices = 0, mCommands = 0;
    uint64_t mShieldVertexHash = 0xcbf29ce484222325ull;
    bool mShieldVertexHashValid = true, mHasModelMatrix = false, mComplete = false;
    Matrix mMatrix{ { { { 1, 0, 0, 0 } }, { { 0, 1, 0, 0 } }, { { 0, 0, 1, 0 } }, { { 0, 0, 0, 1 } } } };
    std::vector<Matrix> mStack;
};

template <class Load>
bool SelectedModelBounds(Load& load, const char* path, float tilt, int dinProfile, FrameBounds& bounds,
                         bool allowEmpty = false, bool requireRestoredModelView = false) {
    ModelBoundsReader<Load> reader(load, tilt);
    if (!reader.Read(path, bounds, allowEmpty) || (requireRestoredModelView && !reader.RestoresModelView()))
        return false;
    if (dinProfile >= 1 && dinProfile <= 3) {
        const auto& layers = DinSwordGi::profiles[dinProfile - 1];
        for (const auto* layer : { layers.core, layers.flame }) {
            FrameBounds layerBounds{};
            ModelBoundsReader<Load> layerReader(load, tilt);
            if (!layerReader.Read(layer, layerBounds))
                return false; // Preserve the draw rather than claim a partial fit.
            bounds.minimum.y = std::min(bounds.minimum.y, layerBounds.minimum.y);
            bounds.maximum.y = std::max(bounds.maximum.y, layerBounds.maximum.y);
            bounds.spinningWidth = std::max(bounds.spinningWidth, layerBounds.spinningWidth);
        }
    }
    return true;
}

inline void MergeModelBounds(FrameBounds& bounds, const FrameBounds& part) {
    if (!part.slug)
        return; // A successfully traversed, geometry-free material/detail pass.
    if (!bounds.slug) {
        bounds = part;
        return;
    }
    bounds.minimum.y = std::min(bounds.minimum.y, part.minimum.y);
    bounds.maximum.y = std::max(bounds.maximum.y, part.maximum.y);
    bounds.spinningWidth = std::max(bounds.spinningWidth, part.spinningWidth);
}

template <class Load>
bool SelectedModelsFit(Load& load, const char* const* paths, int count, float scale, float tilt, int presentation,
                       int dinProfile, ShopFit& out) {
    if (!paths || count < 1 || count > 16)
        return false;
    FrameBounds bounds{};
    for (int i = 0; i < count; ++i) {
        FrameBounds part{};
        if (!paths[i] || !SelectedModelBounds(load, paths[i], tilt, i == 0 ? dinProfile : 0, part, true, count > 1))
            return false;
        MergeModelBounds(bounds, part);
    }
    if (!bounds.slug)
        return false;
    out = FrameFit(bounds, scale, presentation == 1, presentation >= 2 ? presentation - 1 : 0);
    return true;
}

template <class Load>
bool SelectedModelFit(Load& load, const char* path, float scale, float tilt, int presentation, int dinProfile,
                      ShopFit& out) {
    return SelectedModelsFit(load, &path, 1, scale, tilt, presentation, dinProfile, out);
}
} // namespace NeiGi
