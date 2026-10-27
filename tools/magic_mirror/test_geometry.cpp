// Standalone behavioral checks for the same mesh submitted by both native engines.
#include "../../combo/gameplay/MagicMirrorGeometry.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace ComboMagicMirrorGeometry;

int main() {
    Mesh first{}, same{}, cycle{}, animated{};
    Build(0, first);
    Build(0, same);
    Build(kAnimationFrames, cycle);
    Build(kAnimationFrames / 4, animated);
    assert(first.vertexCount > 0 && first.vertexCount <= kMaxVertices);
    assert(first.triangleCount > 0 && first.triangleCount <= kMaxTriangles);
    assert(first.batchCount > first.opaqueBatchCount && first.opaqueBatchCount > 0);
    assert(first.glassEndBatch == first.opaqueBatchCount + kRimSegments);
    assert(first.glassEndBatch < first.batchCount);
    assert(ReflectionS(-12.5f) == 0 && ReflectionS(12.5f) == 1);
    assert(ReflectionT(-6.5f) == 1 && ReflectionT(18.5f) == 0);
    bool changes = false;
    for (int i = 0; i < first.vertexCount; ++i) {
        const auto& a = first.vertices[i];
        const auto& b = same.vertices[i];
        const auto& c = cycle.vertices[i];
        const auto& d = animated.vertices[i];
        assert(a.x == b.x && a.y == b.y && a.z == b.z);
        assert(a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a);
        assert(a.x == c.x && a.y == c.y && a.z == c.z);
        assert(a.r == c.r && a.g == c.g && a.b == c.b && a.a == c.a);
        changes |= a.r != d.r || a.g != d.g || a.b != d.b || a.a != d.a || a.z != d.z;
    }
    assert(changes);
    std::vector<uint32_t> frames;
    for (uint32_t f = 0; f <= kAnimationFrames; ++f) frames.push_back(f);
    frames.push_back(1000000); frames.push_back(UINT32_MAX);
    for (uint32_t frame : frames) {
        Mesh mesh{};
        Build(frame, mesh);
        assert(mesh.vertexCount == first.vertexCount && mesh.triangleCount == first.triangleCount);
        float minX = 100, maxX = -100, minY = 100, maxY = -100;
        bool blueFrame = false, goldRim = false;
        for (int i = 0; i < mesh.vertexCount; ++i) {
            const auto& v = mesh.vertices[i];
            assert(std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z));
            assert(std::abs(v.x) <= 16.25f && v.y >= -19.1f && v.y <= 22.3f && std::abs(v.z) <= 2.1f);
            if (v.a == 255) {
                blueFrame |= v.b > v.r * 1.2f && v.b > v.g * 1.1f;
                goldRim |= v.r > v.b * 1.5f && v.r > v.g + 15;
            }
            minX = std::min(minX, v.x); maxX = std::max(maxX, v.x);
            minY = std::min(minY, v.y); maxY = std::max(maxY, v.y);
        }
        assert(minX < -16 && maxX > 16 && minY < -18 && maxY > 22);
        assert(blueFrame && goldRim);
        bool translucent = false, feathered = false;
        int vertices = 0, triangles = 0;
        for (int b = 0; b < mesh.batchCount; ++b) {
            const auto& batch = mesh.batches[b];
            assert(batch.vertexCount > 0 && batch.vertexCount <= 32);
            assert(batch.firstVertex == vertices && batch.firstTriangle == triangles);
            assert(batch.firstVertex + batch.vertexCount <= mesh.vertexCount);
            assert(batch.firstTriangle + batch.triangleCount <= mesh.triangleCount);
            for (int i = 0; i < batch.vertexCount; ++i) {
                const auto& v = mesh.vertices[batch.firstVertex + i];
                if (b < mesh.opaqueBatchCount) assert(v.a == 255);
                else { translucent |= v.a > 0 && v.a < 255; feathered |= v.a == 0; }
            }
            for (int i = 0; i < batch.triangleCount; ++i) {
                const auto& tri = mesh.triangles[batch.firstTriangle + i];
                assert(tri.a < batch.vertexCount && tri.b < batch.vertexCount && tri.c < batch.vertexCount);
                const auto& a = mesh.vertices[batch.firstVertex + tri.a];
                const auto& c = mesh.vertices[batch.firstVertex + tri.b];
                const auto& d = mesh.vertices[batch.firstVertex + tri.c];
                // Match the native 1/16-unit quantization, so a thin ornament cannot collapse in game.
                auto q = [](float n) { return std::round(n * kVertexPrecision) / kVertexPrecision; };
                float ux = q(c.x)-q(a.x), uy = q(c.y)-q(a.y), uz = q(c.z)-q(a.z);
                float vx = q(d.x)-q(a.x), vy = q(d.y)-q(a.y), vz = q(d.z)-q(a.z);
                float nx = uy*vz-uz*vy, ny = uz*vx-ux*vz, nz = ux*vy-uy*vx;
                assert(nx*nx + ny*ny + nz*nz > 0.000001f);
            }
            vertices += batch.vertexCount; triangles += batch.triangleCount;
        }
        assert(vertices == mesh.vertexCount && triangles == mesh.triangleCount);
        assert(translucent && feathered);
    }
    std::cout << "Magic Mirror geometry: deterministic cycle, finite bounds, native topology and alpha pass ("
              << first.vertexCount << " vertices, " << first.triangleCount << " triangles, "
              << first.batchCount << " batches).\n";
}
