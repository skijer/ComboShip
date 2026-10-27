#pragma once

// Shared geometry/UVs for the native OoT/MM draw path and the icon/preview
// exporter. Coordinates are ordinary game units; Draw uses 1/16-unit Vtx precision.
#include <array>
#include <cmath>
#include <cstdint>

namespace ComboMagicMirrorGeometry {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kVertexPrecision = 16.0f;
constexpr uint32_t kAnimationFrames = 960;
constexpr int kRimSegments = 32;
constexpr int kGlassRings = 5;
// RGBA16: 64 * 32 * 2 = 4096 bytes, exactly one native texture tile.
constexpr int kReflectionWidth = 64, kReflectionHeight = 32;
constexpr int kMaxVertices = 1280;
constexpr int kMaxTriangles = 1280;
constexpr int kMaxBatches = 96;

struct Color {
    uint8_t r, g, b, a;
};
struct Vertex {
    float x, y, z;
    uint8_t r, g, b, a;
};
struct Triangle {
    uint8_t a, b, c;
};
struct Batch {
    uint16_t firstVertex, firstTriangle;
    uint8_t vertexCount, triangleCount;
};
struct Mesh {
    std::array<Vertex, kMaxVertices> vertices;
    std::array<Triangle, kMaxTriangles> triangles;
    std::array<Batch, kMaxBatches> batches;
    uint16_t vertexCount = 0, triangleCount = 0, batchCount = 0, opaqueBatchCount = 0, glassEndBatch = 0;
};

inline uint8_t Channel(float value) {
    return static_cast<uint8_t>(value < 0 ? 0 : value > 255 ? 255 : value + 0.5f);
}
inline Color Mix(Color a, Color b, float amount) {
    return { Channel(a.r + (b.r - a.r) * amount), Channel(a.g + (b.g - a.g) * amount),
             Channel(a.b + (b.b - a.b) * amount), Channel(a.a + (b.a - a.a) * amount) };
}
inline Vertex Point(float x, float y, float z, Color color) {
    return { x, y, z, color.r, color.g, color.b, color.a };
}
inline float ReflectionS(float x) {
    return x / 25.0f + 0.5f;
}
inline float ReflectionT(float y) {
    return 0.5f - (y - 6.0f) / 25.0f;
}
inline Color ReflectionModulation(Color) {
    // The scene carries its own lighting. Neutral opaque modulation preserves
    // detail; the separate restrained glints still identify polished glass.
    return { 255, 255, 255, 255 };
}

class Builder {
  public:
    explicit Builder(Mesh& mesh) : mesh(mesh) {
        mesh.vertexCount = mesh.triangleCount = mesh.batchCount = 0;
    }
    void Begin() {
        mesh.batches[mesh.batchCount++] = { mesh.vertexCount, mesh.triangleCount, 0, 0 };
    }
    void Add(Vertex vertex) {
        mesh.vertices[mesh.vertexCount++] = vertex;
        ++Current().vertexCount;
    }
    void Tri(int a, int b, int c) {
        mesh.triangles[mesh.triangleCount++] = { static_cast<uint8_t>(a), static_cast<uint8_t>(b),
                                                 static_cast<uint8_t>(c) };
        ++Current().triangleCount;
    }
    void Quad(int a, int b, int c, int d) {
        Tri(a, b, c);
        Tri(a, c, d);
    }

  private:
    Batch& Current() {
        return mesh.batches[mesh.batchCount - 1];
    }
    Mesh& mesh;
};

inline Color RimColor(float angle, int profile) {
    // ALTTP's gold inner lip and blue outer casing use separate bevel bands.
    constexpr Color colors[] = { { 126, 84, 31, 255 },  { 255, 235, 160, 255 }, { 220, 169, 72, 255 },
                                 { 48, 104, 158, 255 }, { 137, 181, 219, 255 }, { 39, 84, 137, 255 },
                                 { 23, 57, 98, 255 },   { 28, 66, 109, 255 } };
    const float light = 0.73f + 0.27f * (0.5f + 0.5f * (-0.65f * std::cos(angle) + 0.76f * std::sin(angle)));
    const auto color = colors[profile];
    return { Channel(color.r * light), Channel(color.g * light), Channel(color.b * light), 255 };
}

inline Vertex HandlePoint(float width, float y, int index) {
    const float angle = index * (2.0f * kPi / 8.0f);
    const float light = 0.66f + 0.34f * (0.5f + 0.5f * (-0.45f * std::cos(angle) + 0.89f * std::sin(angle)));
    return Point(width * std::cos(angle), y, 0.9f * std::sin(angle),
                 { Channel(78 * light), Channel(130 * light), Channel(181 * light), 255 });
}

inline Vertex GlassPoint(float radius, float angle, float phase) {
    const float nx = radius * std::cos(angle), ny = radius * std::sin(angle);
    const float ripple = 0.5f + 0.5f * std::sin(ny * 4 + nx * 2 + phase);
    Color color = Mix({ 111, 163, 190, 246 }, { 186, 220, 236, 246 }, 0.46f + ny * 0.2f + ripple * 0.12f);
    // Broad diagonal reflections read as ordinary polished glass while idle.
    // The proposed TP activation sigil belongs to a later use-sequence effect.
    const float diagonal = nx - ny * 0.38f;
    const float reflection = std::fmax(0.0f, 1.0f - std::fabs(diagonal + 0.12f) * 2.25f);
    const float second = std::fmax(0.0f, 1.0f - std::fabs(diagonal - 0.72f) * 7.0f) * 0.5f;
    color = Mix(color, { 249, 254, 255, 246 }, std::fmax(reflection * 0.88f, second));
    const float z = 0.15f + 0.22f * (1.0f - radius * radius) + 0.015f * std::sin(nx * 4 + ny * 7 - phase * 2);
    return Point(nx * 12.5f, 6.0f + ny * 12.5f, z, color);
}

// Geometry and color are pure functions of the bounded animation phase. No RNG,
// global matrix, persistent vertex storage, resource manager or save state is used.
inline void Build(uint32_t frame, Mesh& mesh) {
    Builder builder(mesh);
    const float phase = (frame % kAnimationFrames) * (2.0f * kPi / kAnimationFrames);
    constexpr float rimRadius[] = { 12.5f, 12.85f, 13.55f, 13.7f, 15.55f, 16.2f, 15.55f, 12.5f };
    constexpr float rimZ[] = { 1.05f, 1.7f, 1.3f, 1.25f, 1.45f, 0.0f, -1.2f, -1.2f };
    // The circular face and short rounded grip preserve ALTTP's silhouette.
    for (int segment = 0; segment < kRimSegments; ++segment) {
        builder.Begin();
        for (int end = 0; end < 2; ++end) {
            const float angle = ((segment + end) % kRimSegments) * (2 * kPi / kRimSegments);
            for (int p = 0; p < 8; ++p)
                builder.Add(Point(rimRadius[p] * std::cos(angle), 6 + rimRadius[p] * std::sin(angle), rimZ[p],
                                  RimColor(angle, p)));
        }
        for (int p = 0; p < 8; ++p)
            builder.Quad(p, (p + 1) % 8, 8 + (p + 1) % 8, 8 + p);
    }
    constexpr float handleY[] = { -7.8f, -9.2f, -10.6f, -11.6f, -14.2f, -15.5f, -17.0f, -18.6f, -19.0f };
    constexpr float handleX[] = { 1.6f, 2.4f, 2.0f, 1.35f, 1.45f, 1.85f, 2.05f, 1.45f, 0.65f };
    for (int level = 0; level < 8; ++level) {
        builder.Begin();
        for (int end = 0; end < 2; ++end)
            for (int side = 0; side < 8; ++side)
                builder.Add(HandlePoint(handleX[level + end], handleY[level + end], side));
        for (int side = 0; side < 8; ++side)
            builder.Quad(side, (side + 1) % 8, 8 + (side + 1) % 8, 8 + side);
    }
    builder.Begin();
    builder.Add(Point(0, -19.0f, 0, { 30, 70, 115, 255 }));
    for (int i = 0; i < 8; ++i)
        builder.Add(HandlePoint(0.65f, -19.0f, i));
    for (int i = 0; i < 8; ++i)
        builder.Tri(0, i + 1, (i + 1) % 8 + 1);
    mesh.opaqueBatchCount = mesh.batchCount;

    // One double-sided, nearly opaque reflecting pane. It is recessed inside
    // the gold inner rim; all highlights stay on the glass and casing.
    for (int segment = 0; segment < kRimSegments; ++segment) {
        builder.Begin();
        builder.Add(GlassPoint(0, 0, phase));
        for (int ring = 1; ring <= kGlassRings; ++ring)
            for (int end = 0; end < 2; ++end) {
                const float angle = ((segment + end) % kRimSegments) * (2 * kPi / kRimSegments);
                builder.Add(GlassPoint(ring / float(kGlassRings), angle, phase));
            }
        builder.Tri(0, 1, 2);
        for (int ring = 1; ring < kGlassRings; ++ring) {
            const int a = 1 + (ring - 1) * 2;
            builder.Quad(a, a + 2, a + 3, a + 1);
        }
    }
    mesh.glassEndBatch = mesh.batchCount;
    // A slender reflected arc and two softly pulsing pinpoints, with transparent
    // edges. Shimmer is intentionally confined to the object.
    for (int segment = 0; segment < 8; ++segment) {
        builder.Begin();
        for (int end = 0; end < 2; ++end) {
            const float angle = (0.55f + (segment + end) * 0.067f) * kPi;
            const float fade = std::sin((segment + end) * (kPi / 8.0f));
            for (int edge = 0; edge < 2; ++edge) {
                const float radius = edge ? 0.81f : 0.77f;
                builder.Add(Point(std::cos(angle) * 12.5f * radius, 6 + std::sin(angle) * 12.5f * radius, 0.55f,
                                  { 222, 246, 255, Channel(fade * (42 + 10 * std::sin(phase))) }));
            }
        }
        builder.Quad(0, 1, 3, 2);
    }
    for (int glint = 0; glint < 2; ++glint) {
        const float x = glint ? 11.4f : -9.8f, y = glint ? -2.6f : 17.5f;
        const float pulse = 0.5f + 0.5f * std::sin(phase * 2 + glint * kPi);
        builder.Begin();
        builder.Add(Point(x, y, 1.95f, { 255, 248, 196, Channel(45 + pulse * 100) }));
        builder.Add(Point(x - 0.55f, y, 1.95f, { 255, 238, 166, 0 }));
        builder.Add(Point(x, y + 1.1f, 1.95f, { 255, 238, 166, 0 }));
        builder.Add(Point(x + 0.55f, y, 1.95f, { 255, 238, 166, 0 }));
        builder.Add(Point(x, y - 1.1f, 1.95f, { 255, 238, 166, 0 }));
        for (int i = 0; i < 4; ++i)
            builder.Tri(0, i + 1, (i + 1) % 4 + 1);
    }
}
} // namespace ComboMagicMirrorGeometry
