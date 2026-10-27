#pragma once
#include "NeiGiModelBounds.h"

namespace NeiGi {
// Copy the selected triangles, including replacement-resource matrices. The
// texture pass follows the actual emblem/facets rather than an invented disc.
template <class Load> class RewardSurfaceLoader {
  public:
    explicit RewardSurfaceLoader(Load& loader) : load(loader) {
    }
    template <class Key> auto operator()(Key key) {
        return load(key);
    }
    template <class Key> auto LoadMatrix(Key key) {
        if constexpr (requires { load.LoadMatrix(key); })
            return load.LoadMatrix(key);
        else
            return load(key);
    }
    void Enter(const std::shared_ptr<Fast::DisplayList>& resource) {
        if constexpr (requires { load.Enter(resource); })
            load.Enter(resource);
    }
    void Leave() {
        if constexpr (requires { load.Leave(); })
            load.Leave();
    }
    bool IsMaterialDisplayList(uintptr_t address) const {
        // Native jewel lists call their two geometry-free scrolling materials.
        const uintptr_t base = address & ~uintptr_t(1); // Native exporter tags segment addresses.
        return base == 0x08000000u || base == 0x09000000u;
    }
    bool SurfaceVertex(const float p[3], size_t index) {
        if (index >= cache.size())
            return false;
        cache[index] = { p[0], p[1], p[2] };
        valid[index] = true;
        return true;
    }
    bool SurfaceTriangle(size_t a, size_t b, size_t c) {
        if (a >= cache.size() || b >= cache.size() || c >= cache.size() || !valid[a] || !valid[b] || !valid[c] ||
            mesh.count + 3 > mesh.vertices.size())
            return false;
        const Point normal = Cross(cache[b] - cache[a], cache[c] - cache[a]);
        if (normal.x * normal.x + normal.y * normal.y + normal.z * normal.z < .000001f)
            return true;
        const Point bump = Unit(normal) * .12f;
        for (size_t index : { a, b, c })
            mesh.vertices[mesh.count++] = { cache[index] + bump, 0xffffff, 255, 0, 0 };
        return true;
    }
    Mesh mesh{};

  private:
    Load& load;
    std::array<Point, 32> cache{};
    std::array<bool, 32> valid{};
};

template <class Load> bool ReadRewardSurface(Load& load, const char* path, Mesh& out) {
    out = {};
    if (!path)
        return false;
    RewardSurfaceLoader<Load> surface(load);
    ModelBoundsReader<RewardSurfaceLoader<Load>, true> reader(surface, 0);
    FrameBounds bounds{};
    if (!reader.Read(path, bounds) || !reader.RestoresModelView() || !surface.mesh.count)
        return false;
    float low[3] = { INFINITY, INFINITY, INFINITY }, high[3] = { -INFINITY, -INFINITY, -INFINITY };
    for (size_t i = 0; i < surface.mesh.count; ++i) {
        const Point p = surface.mesh.vertices[i].p;
        const float axes[3] = { p.x, p.y, p.z };
        for (int axis = 0; axis < 3; ++axis) {
            low[axis] = std::min(low[axis], axes[axis]);
            high[axis] = std::max(high[axis], axes[axis]);
        }
    }
    // Project onto the two widest axes of this selected surface. Native
    // medallions and stones use different orientations before the host pose.
    std::array<int, 3> axes{ 0, 1, 2 };
    std::sort(axes.begin(), axes.end(), [&](int a, int b) { return high[a] - low[a] > high[b] - low[b]; });
    if (high[axes[1]] <= low[axes[1]])
        return false;
    for (size_t i = 0; i < surface.mesh.count; ++i) {
        auto& vertex = surface.mesh.vertices[i];
        const float p[3] = { vertex.p.x, vertex.p.y, vertex.p.z };
        vertex.u = (p[axes[0]] - low[axes[0]]) / (high[axes[0]] - low[axes[0]]);
        vertex.v = (p[axes[1]] - low[axes[1]]) / (high[axes[1]] - low[axes[1]]);
    }
    out = surface.mesh;
    return true;
}
} // namespace NeiGi
