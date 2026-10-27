#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace NeiGi {
enum class Kind {
    Neutral,
    Fire,
    Ice,
    Light,
    Hylia,
    Zonai,
    Demise,
    Leaf,
    Sand,
    Tornado,
    Water,
    Meteor,
    Storm,
    Shadow,
    Slate,
    Hourglass,
    DarkCrystal,
    SwordAura,
    CaneBlue,
    SeasonCycle,
    SlateBomb,
    SlateCycle,
    SlateStasis,
    SlateCryonis,
    SlateSensor,
    KokiriSword,
    MmKokiriSword,
    RazorSword,
    GildedSword,
    MasterSword,
    BiggoronSword,
    GreatFairySword,
    FourSword,
    Somaria,
    Pacci,
    Pokeball,
    MarioMask,
    Gold,
    GiantsKnife,
    // Shimmer-only identities append without changing existing cross-engine IDs.
    DivineShield,
    SheikahShield,
    IkanaShield,
    MagicCape,
    SpiritBreastplate,
    SagesTunic,
    ChampionsTunic,
    PegasusAnklet,
    ClimbBoots,
    PendantOfMemories,
    IronKnuckleAxe,
    ChateauRomani,
    Last = ChateauRomani
};
constexpr float Tau = 6.28318530718f;
// The authored Four Sword's high center is lowered in GI space only. Its
// held model and source vertices keep their accepted placement.
constexpr float PresentationOffsetY(Kind kind) {
    return kind == Kind::FourSword ? -18.f : 0.f;
}
constexpr std::array<uint32_t, 4> FourSwordColors{ 0x315B2F, 0xD8232D, 0x2289CF, 0x6D3593 };
constexpr uint32_t ColorHex(Kind kind) {
    switch (kind) {
        case Kind::Somaria:
            return 0xFF3C3C;
        case Kind::Pacci:
            return 0xFFD746;
        case Kind::Pokeball:
            return 0xE73842;
        case Kind::MarioMask:
            return 0xE63C3C;
        case Kind::Fire:
            return 0xFA8B20;
        case Kind::Ice:
            return 0x357CFF;
        case Kind::Light:
            return 0xFDFF7B;
        case Kind::Hylia:
            return 0xFF96FF;
        case Kind::Zonai:
            return 0x64FFE6;
        case Kind::Demise:
            return 0x000000;
        case Kind::Leaf:
            return 0x5AC85A;
        case Kind::Sand:
            return 0xE8AC48;
        case Kind::Tornado:
            return 0x91EFBC;
        case Kind::Water:
            return 0x38A6FF;
        case Kind::Meteor:
            return 0xFF4818;
        case Kind::Storm:
            return 0xB5A8FF;
        case Kind::Shadow:
            return 0x9E38DA;
        case Kind::DarkCrystal:
            return 0x201C1B;
        case Kind::Slate:
            return 0x30CAFA;
        case Kind::SlateBomb:
            return 0x5FDCEB;
        case Kind::SlateCycle:
            return 0x64E6BE;
        case Kind::SlateStasis:
            return 0xFAC846;
        case Kind::SlateCryonis:
            return 0x96D7FF;
        case Kind::SlateSensor:
            return 0xC882FF;
        case Kind::Hourglass:
            return 0xF0C864;
        case Kind::SwordAura:
            return 0xFFF4D6;
        case Kind::KokiriSword:
            return 0x78C850;
        case Kind::MmKokiriSword:
            return 0xA46CFF;
        case Kind::RazorSword:
            return 0xBDD6EA;
        case Kind::GildedSword:
        case Kind::Gold:
            return 0xFFD45A;
        case Kind::MasterSword:
            return 0x6F8FFF;
        case Kind::BiggoronSword:
            return 0xFF9A42;
        case Kind::GiantsKnife:
            return 0xCFD9E6;
        case Kind::GreatFairySword:
            return 0x79BE84;
        case Kind::FourSword:
            return FourSwordColors[0];
        case Kind::CaneBlue:
            return 0x5A90FF;
        case Kind::DivineShield:
            return 0x875FCD;
        case Kind::SheikahShield:
            return 0xDC4141;
        case Kind::IkanaShield:
            return 0xAFB4DC;
        case Kind::MagicCape:
        case Kind::PegasusAnklet:
        case Kind::PendantOfMemories:
            return 0xFFD45A;
        case Kind::SpiritBreastplate:
            return 0xEB6E14;
        case Kind::SagesTunic:
            return 0xF0F4FA;
        case Kind::ChampionsTunic:
            return 0x30CAFA;
        case Kind::ClimbBoots:
            return 0xCFAC2E;
        case Kind::IronKnuckleAxe:
            return 0xCFD9E6;
        case Kind::ChateauRomani:
            return 0x4A0C41; // Dominant MM3D label background, sampled from the supplied atlas.
        default:
            return 0xE6E6EB;
    }
}
inline bool IsRod(Kind k) {
    return k == Kind::Fire || k == Kind::Ice || k == Kind::Light;
}
inline bool IsSpell(Kind k) {
    return k == Kind::Hylia || k == Kind::Zonai || k == Kind::Demise;
}
inline bool IsSlate(Kind k) {
    return k == Kind::Slate || (k >= Kind::SlateBomb && k <= Kind::SlateSensor);
}
inline bool IsSword(Kind k) {
    return k == Kind::SwordAura || k == Kind::GiantsKnife || (k >= Kind::KokiriSword && k <= Kind::FourSword);
}
inline bool IsEquipment(Kind k) {
    return (k >= Kind::DivineShield && k <= Kind::IronKnuckleAxe) || k == Kind::Gold || k == Kind::CaneBlue;
}
constexpr uint32_t EquipmentShimmerSecondary(Kind k) {
    switch (k) {
        case Kind::DivineShield:
        case Kind::ClimbBoots:
            return 0xCFD9E6;
        case Kind::SheikahShield:
        case Kind::PendantOfMemories:
            return 0x4090FF;
        case Kind::IkanaShield:
        case Kind::SagesTunic:
            return 0xFFD45A;
        case Kind::MagicCape:
            return 0xB80B0E; // Authored crimson cloth; retain the cape's existing red.
        case Kind::PegasusAnklet:
            return 0x16AB94;
        default:
            return ColorHex(k);
    }
}
inline bool IsSpecial(Kind k) {
    return (k >= Kind::Sand && k <= Kind::CaneBlue) || IsSlate(k) || IsSword(k);
}
struct Point {
    float x = 0, y = 0, z = 0;
    Point operator+(Point b) const {
        return { x + b.x, y + b.y, z + b.z };
    }
    Point operator-(Point b) const {
        return { x - b.x, y - b.y, z - b.z };
    }
    Point operator*(float s) const {
        return { x * s, y * s, z * s };
    }
};
inline Point Cross(Point a, Point b) {
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}
inline Point Unit(Point p) {
    float n = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
    return n > .0001f ? p * (1.f / n) : Point{ 1, 0, 0 };
}
// Camera axes expressed in the rotating GI's coordinates, with scale removed.
struct Basis {
    Point right{ 1, 0, 0 }, up{ 0, 1, 0 }, forward{ 0, 0, 1 };
};
struct EffectVertex {
    Point p;
    uint32_t rgb;
    uint8_t alpha;
    float u = 0, v = 0;
};
struct Mesh {
    // At most 512 triangles / 24 KiB of arena vertices, independent of frame count.
    std::array<EffectVertex, 1536> vertices{};
    size_t count = 0;
    void Tri(EffectVertex a, EffectVertex b, EffectVertex c) {
        if (count + 3 > vertices.size())
            return;
        vertices[count++] = a;
        vertices[count++] = b;
        vertices[count++] = c;
    }
};
inline float Time(uint32_t frame) {
    return (frame % 180u) * (Tau / 180.f);
}
inline Point Plane(const Basis& b, float x, float y, float z = 0) {
    return b.right * x + b.up * y + b.forward * z;
}
inline void Glow(Mesh& m, Point center, float radius, uint32_t color, uint8_t alpha, const Basis& b) {
    for (int j = 0; j < 16; ++j) {
        float a = Tau * j / 16, c = Tau * (j + 1) / 16;
        m.Tri({ center, color, alpha }, { center + Plane(b, std::cos(a) * radius, std::sin(a) * radius), color, 0 },
              { center + Plane(b, std::cos(c) * radius, std::sin(c) * radius), color, 0 });
    }
}
inline void Star(Mesh& m, Point p, float size, float spin, uint32_t color, const Basis& b, uint32_t core = 0xFFFFFF) {
    for (int j = 0; j < 8; ++j) {
        float a = spin + Tau * j / 8, c = spin + Tau * (j + 1) / 8;
        float r = j % 2 ? size * .19f : size, s = (j + 1) % 2 ? size * .19f : size;
        m.Tri({ p, core, 255 }, { p + Plane(b, std::cos(a) * r, std::sin(a) * r), color, 45 },
              { p + Plane(b, std::cos(c) * s, std::sin(c) * s), color, 45 });
    }
}
// Soft colored skirt and bright narrow center. All vertices are per-frame arena data.
inline void Band(Mesh& m, Point a, Point b, float width, uint32_t color, uint32_t core, const Basis& camera,
                 uint8_t alpha = 240) {
    Point side = Unit(Cross(b - a, camera.forward)) * width;
    m.Tri({ a, color, alpha }, { a - side, color, 0 }, { b - side, color, 0 });
    m.Tri({ a, color, alpha }, { b - side, color, 0 }, { b, color, alpha });
    m.Tri({ a, color, alpha }, { b, color, alpha }, { b + side, color, 0 });
    m.Tri({ a, color, alpha }, { b + side, color, 0 }, { a + side, color, 0 });
    side = side * .18f;
    m.Tri({ a - side, core, alpha }, { b - side, core, alpha }, { b + side, core, alpha });
    m.Tri({ a - side, core, alpha }, { b + side, core, alpha }, { a + side, core, alpha });
}
// A pointed hexagonal ice prism with hard planar faces. The effect pass does
// not write depth, so submit only faces directed toward the camera; otherwise
// a rear face could paint over the front and flatten the crystal's shape.
inline void IceCrystal(Mesh& m, Point center, Point direction, float length, float width, float roll, uint8_t alpha,
                       const Basis& camera) {
    const Point axis = Unit(direction);
    const Point side = Unit(Cross(axis, std::abs(axis.y) < .9f ? Point{ 0, 1, 0 } : Point{ 1, 0, 0 }));
    const Point across = Cross(axis, side);
    const Point light = Unit(camera.up * .6f - camera.right * .4f + camera.forward * .7f);
    auto dot = [](Point a, Point b) { return a.x * b.x + a.y * b.y + a.z * b.z; };
    auto face = [&](Point a, Point b, Point c) {
        const Point normal = Unit(Cross(b - a, c - a));
        if (dot(normal, camera.forward) <= .0001f)
            return;
        const float shade = .35f + .65f * std::fmax(0.f, dot(normal, light));
        const uint32_t color =
            (uint32_t(48 + 180 * shade) << 16) | (uint32_t(140 + 105 * shade) << 8) | uint32_t(240 + 15 * shade);
        m.Tri({ a, color, alpha }, { b, color, alpha }, { c, color, alpha });
    };
    auto ring = [&](int j, bool upper) {
        const float a = roll + Tau * (j % 6) / 6;
        return center + axis * (length * (upper ? .20f : -.28f)) +
               (side * std::cos(a) + across * std::sin(a)) * (width * (upper ? .85f : 1.f));
    };
    const Point tip = center + axis * (length * .62f), base = center - axis * (length * .58f);
    for (int j = 0; j < 6; ++j) {
        const Point lo = ring(j, false), nextLo = ring(j + 1, false);
        const Point hi = ring(j, true), nextHi = ring(j + 1, true);
        face(lo, nextLo, nextHi);
        face(lo, nextHi, hi);
        face(hi, nextHi, tip);
        face(nextLo, lo, base);
    }
}
// A pale reflection band clipped to the front facets. It follows the incoming
// GI camera/rotation and drifts gently; it never fills or leaves the crystal.
inline Mesh SampleCrystalSheen(uint32_t frame, const Basis& camera = {}, float radius = 22.2f,
                               float halfHeight = 37.f) {
    Mesh mesh;
    const Point points[] = { { 0, halfHeight, 0 }, { radius, 0, 0 },  { 0, 0, radius },
                             { -radius, 0, 0 },    { 0, 0, -radius }, { 0, -halfHeight, 0 } };
    auto dot = [](Point a, Point b) { return a.x * b.x + a.y * b.y + a.z * b.z; };
    const auto light = Unit(camera.forward + camera.up * .65f - camera.right * .45f);
    const auto halfway = Unit(camera.forward + light);
    const float phase = (frame % 360u) * (Tau / 360.f);
    const float center = radius * (-.18f + .48f * std::sin(phase));
    const float width = radius * .22f;
    const float offsets[] = { -1.f, -.3f, 0.f, .3f, 1.f };
    const float weights[] = { 0.f, .65f, 1.f, .65f, 0.f };
    auto coordinate = [&](Point p) { return dot(p, camera.right) + .28f * dot(p, camera.up) - center; };
    for (int j = 0; j < 4; ++j)
        for (int half = 0; half < 2; ++half) {
            const Point a = points[half ? 5 : 0], b = points[1 + (half ? (j + 1) % 4 : j)],
                        c = points[1 + (half ? j : (j + 1) % 4)];
            const auto normal = Unit(Cross(c - a, b - a));
            const float facing = dot(normal, camera.forward);
            if (facing <= .001f)
                continue;
            const float specular = std::pow(std::fmax(0.f, dot(normal, halfway)), 5.f);
            const float peak = 112.f * (.35f + .65f * specular);
            for (int strip = 0; strip < 4; ++strip) {
                std::array<Point, 8> polygon{ a, b, c };
                size_t count = 3;
                auto clip = [&](float boundary, bool greater) {
                    std::array<Point, 8> result{};
                    size_t n = 0;
                    for (size_t i = 0; i < count; ++i) {
                        const Point from = polygon[i], to = polygon[(i + 1) % count];
                        const float x = coordinate(from), y = coordinate(to);
                        const bool inside = greater ? x >= boundary : x <= boundary;
                        const bool next = greater ? y >= boundary : y <= boundary;
                        if (inside)
                            result[n++] = from;
                        if (inside != next)
                            result[n++] = from + (to - from) * ((boundary - x) / (y - x));
                    }
                    polygon = result;
                    count = n;
                };
                const float low = width * offsets[strip], high = width * offsets[strip + 1];
                clip(low, true);
                clip(high, false);
                auto vertex = [&](Point p) {
                    const float f = std::fmax(0.f, std::fmin(1.f, (coordinate(p) - low) / (high - low)));
                    const float strength = weights[strip] + f * (weights[strip + 1] - weights[strip]);
                    return EffectVertex{ p * 1.0005f, 0xEDF6FF, uint8_t(peak * strength) };
                };
                for (size_t i = 1; i + 1 < count; ++i)
                    mesh.Tri(vertex(polygon[0]), vertex(polygon[i]), vertex(polygon[i + 1]));
            }
        }
    return mesh;
}
// All swords point up +Y. Camera axes only shape the soft skirts; emission
// paths remain in the corrected model's own coordinates.
inline float SwordBladeTip(Kind kind) {
    switch (kind) {
        case Kind::KokiriSword:
            return 38.f;
        case Kind::MmKokiriSword:
            return 40.f;
        case Kind::RazorSword:
            return 35.f;
        case Kind::BiggoronSword:
            return 109.5f;
        case Kind::GiantsKnife:
            return 100.3f;
        case Kind::GildedSword:
            return 61.8f;
        case Kind::MasterSword:
            return 70.2f;
        case Kind::SwordAura:
            return 70.2f;
        case Kind::GreatFairySword:
            return 100.f;
        case Kind::FourSword:
            return 49.f;
        default:
            return 47.f;
    }
}
inline float SwordEmissionBase(Kind kind) {
    // Centered legacy meshes put their guards below the origin. These starts
    // lie just above the measured, exported blade seats, rather than halfway
    // up the longer blades. Forged meshes keep their guard at Y=0.
    return kind == Kind::RazorSword ? -9.f : 8.f;
}
inline Mesh SampleSword(Kind kind, uint32_t frame, const Basis& camera, float particleScale = 1.f) {
    Mesh m;
    const float t = Time(frame), top = SwordBladeTip(kind), bottom = SwordEmissionBase(kind);
    const uint32_t color = ColorHex(kind);
    const float size = std::isfinite(particleScale) && particleScale > 0.f ? particleScale : 1.f;
    if (kind == Kind::KokiriSword || kind == Kind::GreatFairySword) {
        const bool fairy = kind == Kind::GreatFairySword;
        for (int i = 0; i < 14; ++i) {
            const uint32_t age = frame % 360u + i * 31u;
            const float f = float(age % 180u) / 180.f;
            const float a = i * 2.399963f + f * Tau * (fairy ? 1.5f : .5f);
            const float radius = fairy ? 10.f : 4.8f;
            const float pad = (fairy ? 2.6f : 2.f) * size;
            const Point p{ radius * std::cos(a), bottom + pad + (top - bottom - 2.f * pad) * f, radius * std::sin(a) };
            const Point along = Point{ std::sin(a) * .6f, fairy ? 2.6f : 2.f, std::cos(a) * .6f } * size;
            const Point across =
                Point{ std::cos(a) * (fairy ? 1.35f : 1.1f), .1f, -std::sin(a) * (fairy ? 1.35f : 1.1f) } * size;
            const uint32_t hue = fairy && (age / 180u) % 2 ? 0x9382C4 : color;
            const uint8_t alpha = uint8_t(220 * std::sin(f * Tau * .5f));
            // Pointed local flecks/petals retain their shape while the GI spins.
            m.Tri({ p + along, hue, alpha }, { p + across, hue, alpha }, { p - along, hue, alpha });
            m.Tri({ p + along, hue, alpha }, { p - along, hue, alpha }, { p - across, hue, alpha });
        }
    } else if (kind == Kind::MmKokiriSword || kind == Kind::MasterSword || kind == Kind::SwordAura ||
               kind == Kind::FourSword) {
        const bool blessing = kind == Kind::SwordAura, four = kind == Kind::FourSword;
        const int strands = four ? 4 : 2;
        for (int strand = 0; strand < strands; ++strand) {
            const uint32_t hue = four ? FourSwordColors[strand] : color;
            auto p = [&](int j) {
                const float f = j / 14.f;
                const float a = t + f * Tau * .55f + strand * Tau / strands;
                const float radius = (four ? 5.4f : 4.8f) + .45f * std::sin(f * Tau + t);
                return Point{ radius * std::sin(a), bottom + (top - bottom) * f, radius * std::cos(a) };
            };
            for (int j = 0; j < 14; ++j)
                Band(m, p(j), p(j + 1), (blessing ? .9f : .65f) * size, hue, blessing ? 0xF4C95D : 0xFFFFFF, camera,
                     blessing ? 205 : 180);
        }
        if (blessing) {
            for (int i = 0; i < 6; ++i) {
                const float f = float((frame % 360u + i * 59u) % 360u) / 360.f;
                const float a = i * 2.399963f + t;
                Glow(m, { 5.5f * std::cos(a), bottom + (top - bottom) * f, 5.5f * std::sin(a) }, 1.2f * size, 0xF4C95D,
                     uint8_t(210 * std::sin(f * Tau * .5f)), camera);
            }
        }
    } else if (kind == Kind::RazorSword) {
        for (int i = 0; i < 12; ++i) {
            const float f = float((frame % 180u + i * 23u) % 180u) / 180.f;
            const float pad = 2.4f * size;
            const float y = bottom + pad + (top - bottom - 2.f * pad) * f;
            const float edge = (i % 2 ? -1.f : 1.f) * (5.8f - 1.7f * f);
            const Point p{ edge, y, 1.5f * std::sin(t + i) };
            const uint8_t alpha = uint8_t(225 * std::sin(f * Tau * .5f));
            Band(m, p, p + Point{ .35f, 2.4f, 0 } * size, .45f * size, color, 0xFFFFFF, camera, alpha);
            Glow(m, p, 1.1f * size, color, alpha, camera);
        }
    } else if (kind == Kind::GildedSword || kind == Kind::BiggoronSword || kind == Kind::GiantsKnife) {
        const bool forge = kind != Kind::GildedSword;
        const uint32_t lifetime = forge ? 90u : 360u;
        for (int i = 0; i < 12; ++i) {
            const float f = float((frame % lifetime + i * (forge ? 11u : 43u)) % lifetime) / lifetime;
            const float a = i * 2.399963f + (forge ? f * 1.6f : (frame % 360u) * (Tau / 360.f));
            const float pad = (forge ? 2.8f : 1.4f) * size;
            const Point p{ 5.2f * std::cos(a), bottom + pad + (top - bottom - (forge ? pad : 2.f * pad)) * f,
                           5.2f * std::sin(a) };
            const uint8_t alpha = uint8_t(220 * std::sin(f * Tau * .5f));
            Glow(m, p, (forge ? 1.25f : 1.4f) * size, color, alpha, camera);
            if (forge)
                Band(m, p, p - Point{ .25f, 2.8f, 0 } * size, .32f * size, color, 0xFFE6BA, camera, alpha);
        }
    }
    return m;
}
inline Mesh SampleSpecial(Kind kind, uint32_t frame, const Basis& camera = {}, float particleScale = 1.f) {
    Mesh m;
    const float t = Time(frame);
    const uint32_t color = ColorHex(kind);
    if (kind == Kind::Sand || kind == Kind::Water || kind == Kind::Meteor || kind == Kind::Hourglass) {
        for (int i = 0; i < 10; ++i) {
            const float f = float((frame + i * 11u) % 120u) / 120.f;
            const float a = i * 2.4f + t;
            Point p{ std::cos(a) * 8, 12 - 24 * f, std::sin(a) * 8 };
            if (kind == Kind::Meteor)
                p = { std::cos(a) * 7, -8 + 27 * f, std::sin(a) * 7 };
            if (kind == Kind::Hourglass)
                p = { std::sin(a) * .8f, 9 - 18 * f, std::cos(a) * .8f };
            const uint8_t alpha = uint8_t(210 * std::sin(f * Tau * .5f));
            if (kind == Kind::Water)
                Band(m, p, p + Point{ -.5f, -4, 0 }, .4f, color, 0xE4F8FF, camera, alpha);
            else
                Glow(m, p, kind == Kind::Hourglass ? .4f : .75f, color, alpha, camera);
        }
    } else if (kind == Kind::DarkCrystal) {
        // Thin charcoal wisps with copper rims leave the original silhouette
        // readable; staggered embers rise outside its black/orange body.
        for (int ring = 0; ring < 2; ++ring) {
            auto p = [&](int j) {
                const float f = j / 16.f, a = f * Tau * 1.3f + t * (ring ? -1.f : 1.f) + ring * Tau * .5f;
                const float r = 13.5f + .8f * std::sin(t * 2 + f * Tau);
                return Point{ r * std::cos(a), -19 + 38 * f, r * std::sin(a) };
            };
            for (int j = 0; j < 16; ++j)
                Band(m, p(j), p(j + 1), .42f, color, 0xA54B1B, camera, 105);
        }
        for (int i = 0; i < 8; ++i) {
            const float f = float((frame % 180u + i * 23u) % 180u) / 180.f;
            const float a = i * 2.399963f + f * 1.6f, r = 12.f + 3.f * f;
            const Point p{ r * std::cos(a), -25.f + 54.f * f, r * std::sin(a) };
            const uint8_t alpha = uint8_t(220 * std::sin(f * Tau * .5f));
            Glow(m, p, .8f, 0xD76B20, alpha, camera);
            Band(m, p, p - Point{ .15f, 1.4f, 0 }, .17f, 0xD76B20, 0xFFD19A, camera, alpha);
        }
    } else if (kind == Kind::Tornado || kind == Kind::Shadow || kind == Kind::CaneBlue) {
        for (int ring = 0; ring < 2; ++ring) {
            auto p = [&](int j) {
                const float f = j / 24.f, a = f * Tau * 1.6f + t * (ring ? -.9f : 1.4f) + ring * Tau * .5f;
                const float r = kind == Kind::Tornado ? 3 + 10 * f : 13;
                return Point{ r * std::cos(a), -13 + 26 * f, r * std::sin(a) };
            };
            for (int j = 0; j < 24; ++j)
                Band(m, p(j), p(j + 1), .45f, color, kind == Kind::Tornado ? 0xF0FFF7 : 0xD5B6FF, camera, 150);
        }
    } else if (kind == Kind::Storm) {
        for (int bolt = 0; bolt < 3; ++bolt) {
            auto p = [&](int j) {
                const float a = bolt * Tau / 3;
                return Point{ 9 * std::cos(a) + 2 * std::sin(j * 2.6f + t * 8), -14 + j * 4.f, 9 * std::sin(a) };
            };
            for (int j = 0; j < 7; ++j)
                Band(m, p(j), p(j + 1), .7f, color, 0xF5F3FF, camera, 200);
        }
    } else if (IsSlate(kind)) {
        // Front-mounted sigils rotate in the Slate's own frame. Its raised
        // eye reaches Z=5.04; with the (0,0,4) effect center, this local offset
        // leaves a .75 gap for the .22-wide camera-facing ribbon skirts.
        // The casing naturally hides these front-face rings from rear views.
        constexpr float depth = 1.79f;
        for (int ring = 0; ring < 2; ++ring)
            for (int j = 0; j < 24; ++j) {
                auto p = [&](int k) {
                    float a = k * Tau / 24 + t * (ring ? -1 : 1), r = 9 + ring * 5;
                    return Point{ r * std::cos(a), r * std::sin(a), depth };
                };
                Band(m, p(j), p(j + 1), .22f, color, 0xD1F8FF, camera, 120);
            }
    } else if (IsSword(kind)) {
        return SampleSword(kind, frame, camera, particleScale);
    }
    return m;
}
inline Mesh SampleShimmer(uint32_t frame, bool enabled, const Basis& camera = {}, Kind kind = Kind::Neutral) {
    Mesh m;
    if (!enabled)
        return m;
    float t = Time(frame);
    uint32_t fairyHue = ColorHex(Kind::GreatFairySword);
    if (kind == Kind::GreatFairySword) {
        // One shared 360-frame hue cycle keeps all five shimmer clusters green
        // at phase zero and violet halfway through, without a native-frame snap.
        const float blend = .5f - .5f * std::cos((frame % 360u) * (Tau / 360.f));
        fairyHue = 0;
        for (int shift : { 0, 8, 16 }) {
            const float green = float((0x79BE84u >> shift) & 255u);
            const float violet = float((0x9382C4u >> shift) & 255u);
            fairyHue |= uint32_t(green + (violet - green) * blend + .5f) << shift;
        }
    }
    for (int i = 0; i < 5; ++i) {
        float phase = t + i * Tau / 5;
        Point p = { 24 * std::cos(i * 2.4f + t), 24 * std::sin(phase), 24 * std::sin(i * 2.4f + t) };
        float pulse = .25f + .75f * std::pow(.5f + .5f * std::sin(t * 4 + i * 2.f), 2.f);
        // Wonder-item silhouette; Deku Leaf keeps green halos AND green glints.
        const bool leaf = kind == Kind::Leaf;
        const uint32_t hue = kind == Kind::Pokeball          ? (i % 2 ? 0xFFFFFF : 0xE73842)
                             : kind == Kind::FourSword       ? FourSwordColors[i % FourSwordColors.size()]
                             : kind == Kind::GreatFairySword ? fairyHue
                             : kind == Kind::DarkCrystal     ? (i % 3 == 0 ? ColorHex(kind) : 0xD76B20)
                             : kind == Kind::ChateauRomani   ? (i % 2 ? 0xFFD45A : ColorHex(kind))
                             : IsEquipment(kind)     ? (i % 2 ? EquipmentShimmerSecondary(kind) : ColorHex(kind))
                             : kind == Kind::Neutral ? 0xA8E9FF
                                                     : ColorHex(kind);
        Glow(m, p, 7 * pulse, hue, 140, camera);
        const uint32_t core = kind == Kind::DarkCrystal ? 0xFFD19A : leaf ? 0xB0FFB0 : 0xFFFFFF;
        Star(m, p, 6 * pulse, t * .5f, kind == Kind::Neutral ? 0xE6F8FF : hue, camera, core);
    }
    return m;
}
// Curved textured skin: a flat billboard would be hidden by the opaque core.
// The optional shimmer and the light rod deliberately do not use this pass.
inline Mesh SampleOrb(Kind kind, const Basis& camera = {}) {
    Mesh m;
    if (kind != Kind::Fire && kind != Kind::Ice && !IsSpell(kind))
        return m;
    constexpr int rings = 5, segments = 20;
    const float radius = IsSpell(kind) ? 16.3f : 15.f;
    auto vertex = [&](int ring, int j) {
        const float a = Tau * (j % segments) / segments;
        const float latitude = (Tau * .25f) * ring / rings;
        const float x = std::sin(latitude) * std::cos(a), y = std::sin(latitude) * std::sin(a);
        Point p = Plane(camera, radius * x, radius * y, radius * std::cos(latitude));
        if (IsSpell(kind)) {
            const float extent = (std::abs(p.x) + std::abs(p.z)) / 22.2f + std::abs(p.y) / 37.f;
            if (extent > .99f)
                p = p * (.99f / extent);
        }
        return EffectVertex{ p, 0xFFFFFF, 255, .5f + .5f * x, .5f - .5f * y };
    };
    for (int ring = 0; ring < rings; ++ring)
        for (int j = 0; j < segments; ++j) {
            m.Tri(vertex(ring, j), vertex(ring + 1, j), vertex(ring + 1, j + 1));
            if (ring)
                m.Tri(vertex(ring, j), vertex(ring + 1, j + 1), vertex(ring, j + 1));
        }
    return m;
}
inline Mesh SampleEnergy(Kind kind, uint32_t frame, const Basis& camera = {}) {
    Mesh m;
    const float t = Time(frame);
    const uint32_t color = ColorHex(kind);
    if (kind == Kind::Fire) {
        // The turbulent textured body supplies the flame; embers rise from it.
        for (int i = 0; i < 4; ++i) {
            float f = std::fmod((frame % 180u) / 45.f + i * .25f, 1.f);
            Star(m, { std::sin(t * 3 + i * 2.f) * 7, 10 + f * 21, std::cos(t * 3 + i * 2.f) * 7 }, 2.2f * (1 - f) + .4f,
                 t, 0xFFD274, camera);
        }
    } else if (kind == Kind::Ice) {
        // Small, tumbling ice fragments fall away and fade at both ends of
        // their lifetime. These are faceted solids, not four-point shimmers.
        for (int i = 0; i < 8; ++i) {
            const float f = std::fmod((frame % 180u) / 90.f + i / 8.f, 1.f);
            const float fade = std::sin(f * Tau * .5f);
            const float x = 16 * std::cos(i * 2.4f) + 2 * std::sin(t + i + f * 3);
            const float y = 20 - 43 * f;
            const float spin = t * (i % 2 ? -1 : 1) + i;
            IceCrystal(m, Plane(camera, x, y, 11 + 2 * std::sin(i * 1.4f)),
                       Plane(camera, std::sin(spin), std::cos(spin), .3f), 2.3f + .7f * std::sin(i * 2.f), .65f, spin,
                       static_cast<uint8_t>(225 * fade * fade), camera);
        }
    } else if (kind == Kind::Light) {
        Glow(m, {}, 15, color, 190, camera);
        Glow(m, {}, 8, 0xFFFFE0, 235, camera);
        for (int ring = 0; ring < 2; ++ring) {
            auto p = [&](int j) {
                float a = Tau * j / 16 + t * (ring ? -3 : 2), r = 12;
                return ring ? Point{ r * std::cos(a), r * .65f * std::sin(a), r * .75f * std::sin(a) }
                            : Point{ r * std::cos(a), r * std::sin(a), std::sin(a * 2 + t) * 2 };
            };
            for (int j = 0; j < 16; ++j)
                Band(m, p(j), p(j + 1), 3.5f, color, 0xFFFFE9, camera);
        }
        Star(m, camera.forward * 10, 16 + 3 * std::sin(t * 4), t * .5f, 0xFFF5A3, camera);
    } else if (IsSpell(kind)) {
        const bool dark = kind == Kind::Demise, jagged = kind == Kind::Zonai || dark;
        const uint32_t edge = dark ? 0xAEBBCB : color, hot = dark ? 0xE6E6EB : 0xFFF0FF;
        for (int ring = 0; ring < 3; ++ring) {
            auto p = [&](int j) {
                float a = Tau * .68f * j / 16 + t * (ring % 2 ? -2 : 3) + ring * 2.1f;
                float r = 15.1f + (jagged ? .7f * std::sin(j * 2.8f + t * 9) : .25f * std::sin(a * 3 + t));
                if (ring == 0)
                    return Point{ r * std::cos(a), r * .4f * std::sin(a), r * .92f * std::sin(a) };
                if (ring == 1)
                    return Point{ r * .45f * std::sin(a), r * std::cos(a), r * .9f * std::sin(a) };
                return Point{ r * .7f * std::cos(a), r * .7f * std::cos(a), r * std::sin(a) };
            };
            for (int j = 0; j < 16; ++j) {
                float trail = .3f + .7f * j / 15;
                Band(m, p(j), p(j + 1), (dark ? 2.f : 2.7f) * trail, edge, hot, camera,
                     static_cast<uint8_t>(90 + 150 * trail));
            }
            Star(m, p(16), 1.8f, t * 2, edge, camera);
        }
        // A wandering discharge reaches into the tips without crossing the shell.
        auto surge = [&](int j) {
            float y = (j / 8.f - .5f) * 54;
            return Point{ 2 * std::sin(t * 7 + j * (jagged ? 2.6f : .7f)), y, 10 * (1 - std::abs(y) / 30) };
        };
        for (int j = 0; j < 8; ++j)
            Band(m, surge(j), surge(j + 1), 1.7f, edge, hot, camera,
                 static_cast<uint8_t>(100 + 100 * (.5f + .5f * std::sin(t * 6))));
        // Project all energy strictly inside the existing octahedral shell.
        // This applies after band width and camera-facing glow construction.
        for (size_t i = 0; i < m.count; ++i) {
            auto& p = m.vertices[i].p;
            float extent = (std::abs(p.x) + std::abs(p.z)) / 22.2f + std::abs(p.y) / 37.f;
            if (extent > .97f)
                p = p * (.97f / extent);
        }
    }
    return m;
}
// 1 spring, 2 summer, 3 autumn, 4 winter; 5 cycles all four for the Rod.
// Coordinates are in the incoming GI pose before its mesh-specific scaling.
inline int SeasonProfile(uint32_t frame, int profile) {
    return profile == 5 ? 1 + (frame / 180u) % 4 : profile;
}
inline Point SeasonSunCenter(const Basis& camera, bool rod) {
    return rod ? Plane(camera, 16, 24) : Point{};
}
inline Mesh SampleSeason(uint32_t frame, int profile, const Basis& camera = {}) {
    Mesh m;
    const bool rod = profile == 5;
    profile = SeasonProfile(frame, profile);
    if (profile < 1 || profile > 4)
        return m;
    if (profile == 2) {
        // A steady round sun with a soft halo. The separate
        // textured corona scrolls slowly; no rising sparks or spell glints.
        const Point center = SeasonSunCenter(camera, rod);
        const float radius = rod ? 6.5f : 11.5f;
        Glow(m, center, rod ? 14.f : 26.f, 0xFFD16A, 95, camera);
        for (int j = 0; j < 24; ++j) {
            const float a = Tau * j / 24, b = Tau * (j + 1) / 24;
            m.Tri({ center, 0xFFF4B0, 245 },
                  { center + Plane(camera, radius * std::cos(a), radius * std::sin(a)), 0xFFC83E, 235 },
                  { center + Plane(camera, radius * std::cos(b), radius * std::sin(b)), 0xFFC83E, 235 });
        }
        return m;
    }
    const uint32_t lifetime = profile == 1 ? 80u : profile == 3 ? 210u : 240u;
    const float drift = (frame % 360u) * (Tau / 360.f);
    const float roll = (frame % (profile == 3 ? 240u : 1800u)) * (Tau / (profile == 3 ? 240.f : 1800.f));
    for (int i = 0; i < 12; ++i) {
        const float phase = float((frame % lifetime + i * 19u) % lifetime) / lifetime;
        const float angle = i * 2.399963f;
        const float x = (18 + i % 3 * 5) * std::cos(angle);
        const float z = (18 + i % 3 * 5) * std::sin(angle);
        const float fade = std::fmin(1.f, std::fmin(phase, 1 - phase) * 8.f);
        const uint8_t alpha = uint8_t(210 * fade);
        if (profile == 1) {
            Point p{ x, 32 - 64 * phase, z };
            Band(m, p, p + Point{ -1.2f, -6, 0 }, .55f, 0x5ABEFF, 0xDCF5FF, camera, alpha);
        } else if (profile == 3) {
            Point p{ x + 6 * std::sin(drift + i), 30 - 60 * phase, z };
            Point along = Plane(camera, std::cos(roll + i) * 3.2f, std::sin(roll + i) * 3.2f);
            Point across = Plane(camera, -std::sin(roll + i) * 1.6f, std::cos(roll + i) * 1.6f);
            uint32_t color = i % 2 ? 0xF0A02C : 0xC85022;
            m.Tri({ p + along, color, alpha }, { p + across, color, alpha }, { p - along, 0xFFD279, alpha });
            m.Tri({ p + along, color, alpha }, { p - along, 0xFFD279, alpha }, { p - across, color, alpha });
        } else {
            Point p{ x + 4 * std::sin(drift + i), 32 - 64 * phase, z };
            // Three thin crossing arms make a readable snowflake silhouette.
            for (int arm = 0; arm < 3; ++arm) {
                const float a = roll + i + arm * Tau / 6;
                const Point d = Plane(camera, std::cos(a) * 1.8f, std::sin(a) * 1.8f);
                Band(m, p - d, p + d, .24f, 0x8CCEFF, 0xF0FAFF, camera, alpha);
            }
        }
    }
    return m;
}
// Polar UVs put twelve soft rays around a circular sun. Scrolling the S tile
// moves those rays around its rim without moving or distorting the sun disk.
inline Mesh SampleSeasonSunRays(uint32_t frame, int profile, const Basis& camera = {}) {
    Mesh m;
    if (SeasonProfile(frame, profile) != 2)
        return m;
    const bool rod = profile == 5;
    const Point center = SeasonSunCenter(camera, rod);
    auto vertex = [&](int j, bool outer) {
        const float angle = Tau * (j % 48) / 48;
        const float radius = outer ? (rod ? 14.f : 24.f) : (rod ? 6.25f : 11.2f);
        return EffectVertex{ center + Plane(camera, radius * std::cos(angle), radius * std::sin(angle)), 0xFFD16A, 210,
                             j * (12.f / 48), outer ? 1.f : 0.f };
    };
    for (int j = 0; j < 48; ++j) {
        m.Tri(vertex(j, false), vertex(j, true), vertex(j + 1, true));
        m.Tri(vertex(j, false), vertex(j + 1, true), vertex(j + 1, false));
    }
    return m;
}
} // namespace NeiGi
