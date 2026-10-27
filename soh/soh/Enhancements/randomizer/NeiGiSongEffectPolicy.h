#pragma once
#include "NeiGiEffectPolicy.h"
#include "../../../../combo/menu/ComboSongDraw.h"

namespace NeiGi {
// A curved vane with a raised rachis and tapered, notched barbs. It is a
// triangulated feather silhouette, independently of any archive model.
inline void SongFeather(Mesh& mesh, Point center, float roll, uint32_t color, uint8_t alpha, const Basis& camera) {
    const auto point = [&](float u, float side) {
        const float bend = .65f * std::sin(u * Tau * .5f);
        const float width = 1.8f * std::sin(u * Tau * .5f) * (1.f - .22f * u);
        const float x = bend + side * width;
        const float y = 12.f * (u - .5f);
        return center + Plane(camera, x * std::cos(roll) - y * std::sin(roll), x * std::sin(roll) + y * std::cos(roll),
                              .6f * std::sin(u * Tau * .5f) - .35f * std::abs(side));
    };
    for (int j = 0; j < 6; ++j) {
        const float u = j / 6.f, next = (j + 1) / 6.f;
        for (float side : { -1.f, 1.f }) {
            const Point a = point(u, 0), b = point(u, side), c = point(next, side), d = point(next, 0);
            mesh.Tri({ a, 0xFFF9FF, alpha }, { b, color, alpha }, { c, color, alpha });
            mesh.Tri({ a, 0xFFF9FF, alpha }, { c, color, alpha }, { d, 0xFFF9FF, alpha });
            // Individual barb veins make the two vanes readable as a feather.
            const Point mid = point((u + next) * .5f, side * .88f);
            mesh.Tri({ a, color, alpha }, { mid, 0xEEE1FF, alpha }, { d, color, alpha });
        }
        Band(mesh, point(u, 0), point(next, 0), .12f, color, 0xFFF9FF, camera, alpha);
    }
}

inline void SongHeart(Mesh& mesh, Point center, float radius, uint32_t color, uint8_t alpha, const Basis& camera) {
    for (int j = 0; j < 20; ++j) {
        const auto point = [&](int k) {
            const float a = k * Tau / 20, s = std::sin(a);
            return center + Plane(camera, radius * s * s * s,
                                  radius * (.8125f * std::cos(a) - .3125f * std::cos(2 * a) - .125f * std::cos(3 * a) -
                                            .0625f * std::cos(4 * a)));
        };
        mesh.Tri({ center, 0xFFF0FB, alpha }, { point(j), color, alpha }, { point(j + 1), color, alpha });
    }
}

inline void SongArc(Mesh& mesh, Point center, float radius, float start, float sweep, uint32_t color, uint8_t alpha,
                    const Basis& camera, int segments = 8) {
    // Bubbles and short ripple bands fit in the shared 512-triangle arena.
    for (int j = 0; j < segments; ++j) {
        const float a = start + sweep * j / float(segments), b = start + sweep * (j + 1) / float(segments);
        Band(mesh, center + Plane(camera, radius * std::cos(a), radius * std::sin(a)),
             center + Plane(camera, radius * std::cos(b), radius * std::sin(b)), .35f, color, 0xEFFFFF, camera, alpha);
    }
}

inline void SongMusicalNote(Mesh& mesh, Point center, int variant, float roll, uint32_t color, uint8_t alpha,
                            const Basis& camera) {
    // Small camera-facing notation particles; the native clef model is untouched.
    const auto p = [&](float x, float y) {
        return center + Plane(camera, x * std::cos(roll) - y * std::sin(roll), x * std::sin(roll) + y * std::cos(roll));
    };
    const auto quad = [&](float x0, float y0, float x1, float y1) {
        mesh.Tri({ p(x0, y0), color, alpha }, { p(x1, y0), color, alpha }, { p(x1, y1), color, alpha });
        mesh.Tri({ p(x0, y0), color, alpha }, { p(x1, y1), color, alpha }, { p(x0, y1), color, alpha });
    };
    const auto head = [&](float x, float y) {
        for (int j = 0; j < 12; ++j) {
            const auto edge = [&](int k) {
                const float a = k * Tau / 12;
                return p(x + 1.9f * std::cos(a), y + 1.2f * std::sin(a) + .45f * std::cos(a));
            };
            mesh.Tri({ p(x, y), color, alpha }, { edge(j), color, alpha }, { edge(j + 1), color, alpha });
        }
    };
    Glow(mesh, center, 3.5f, color, alpha / 4, camera);
    if (variant == 2) {
        // Beamed eighth-note pair, with a sloped beam and solid oval heads.
        head(-3, -3);
        head(2, -2);
        quad(-1.5f, -3, -.7f, 4);
        quad(3.5f, -2, 4.3f, 5);
        mesh.Tri({ p(-1.5f, 4), color, alpha }, { p(4.3f, 5), color, alpha }, { p(4.3f, 3.9f), color, alpha });
        mesh.Tri({ p(-1.5f, 4), color, alpha }, { p(4.3f, 3.9f), color, alpha }, { p(-1.5f, 2.9f), color, alpha });
    } else {
        // Quarter note or a flagged eighth note.
        head(-.7f, -3);
        quad(.8f, -3, 1.6f, 5);
        if (variant == 1) {
            mesh.Tri({ p(1.6f, 5), color, alpha }, { p(4.7f, 2.7f), color, alpha }, { p(1.6f, 2.7f), color, alpha });
            mesh.Tri({ p(1.6f, 2.7f), color, alpha }, { p(4.7f, 2.7f), color, alpha }, { p(3.2f, .5f), color, alpha });
        }
    }
}

inline void SongHourglass(Mesh& mesh, Point center, float phase, uint32_t color, const Basis& camera) {
    const auto p = [&](float x, float y) { return center + Plane(camera, x, y); };
    for (float side : { -1.f, 1.f }) {
        Band(mesh, p(side * 3.5f, 7), center, .4f, color, 0xEFFFFF, camera, 215);
        Band(mesh, center, p(side * 3.5f, -7), .4f, color, 0xEFFFFF, camera, 215);
    }
    Band(mesh, p(-4, 7), p(4, 7), .6f, color, 0xFFF5D6, camera, 235);
    Band(mesh, p(-4, -7), p(4, -7), .6f, color, 0xFFF5D6, camera, 235);
    mesh.Tri({ p(-2.8f, -5.8f), color, 150 }, { p(2.8f, -5.8f), color, 150 },
             { p(0, -5.8f + 4 * phase), 0xFFF5D6, 190 });
    for (int i = 0; i < 3; ++i)
        Glow(mesh, p(0, 5 - 10 * std::fmod(phase + i / 3.f, 1.f)), .5f, 0xFFF5D6, 210, camera);
}

inline Mesh SampleMmSong(int song, uint32_t frame, const Basis& camera) {
    Mesh mesh;
    const uint32_t color = ComboSongColorHex(song);
    const float t = (frame % 720u) * (Tau / 360.f);
    if (song == CW_SONG_INVERTED_TIME) {
        // A partial clock dial turns backwards. The native note stays visible.
        const float clock = (frame % 1440u) * (Tau / 1440.f);
        SongArc(mesh, {}, 25, clock, Tau * .72f, color, 180, camera, 24);
        for (int i = 0; i < 12; ++i) {
            const float a = i * Tau / 12 + clock;
            Band(mesh, Plane(camera, 23 * std::cos(a), 23 * std::sin(a)),
                 Plane(camera, 26 * std::cos(a), 26 * std::sin(a)), .4f, color, 0xDCE9FF, camera, 185);
        }
        for (int i = 0; i < 6; ++i) {
            const float a = clock - i * .2f;
            Glow(mesh, Plane(camera, 25 * std::cos(a), 25 * std::sin(a)), 1.8f, color, 210 - i * 25, camera);
        }
    } else if (song == CW_SONG_DOUBLE_TIME) {
        for (int i = 0; i < 2; ++i) {
            const float phase = float((frame % 120u + i * 60u) % 120u) / 120.f;
            const Point p = Plane(camera, i ? 19 : -19, i ? 5 : -5);
            SongHourglass(mesh, p, phase, color, camera);
            for (int j = 0; j < 3; ++j) {
                const float x = -26 + 52 * std::fmod(phase + j / 3.f, 1.f);
                const uint8_t alpha = uint8_t(170 * std::sin(std::fmod(phase + j / 3.f, 1.f) * Tau * .5f));
                Band(mesh, Plane(camera, x - 4, i ? 20 : -20), Plane(camera, x + 2, i ? 20 : -20), .45f, color,
                     0xFFFFFF, camera, alpha);
            }
        }
    } else if (song == CW_SONG_NOVA) {
        for (int i = 0; i < 7; ++i) {
            const float phase = float((frame % 240u + i * 34u) % 240u) / 240.f;
            const float angle = i * 2.399963f + t * .2f;
            const Point p{ 23 * std::cos(angle), -25 + 50 * phase, 23 * std::sin(angle) };
            SongArc(mesh, p, 2 + .8f * std::sin(t + i), 0, Tau, color, uint8_t(190 * std::sin(phase * Tau * .5f)),
                    camera);
        }
        // Keep the two lower ripples; add a shorter, staggered wave above them.
        for (int i = 0; i < 3; ++i) {
            const int segments = i == 2 ? 6 : 8;
            const auto point = [&](int j) {
                const float x = (i == 2 ? -9.f : -16.f) + j * 4.f;
                return Plane(camera, x, -18 + i * 6 + 1.8f * std::sin(x * .3f + t * 1.2f + i));
            };
            for (int j = 0; j < segments; ++j)
                Band(mesh, point(j), point(j + 1), .35f, color, 0xB0E8FF, camera, 140);
        }
    } else if (song == CW_SONG_LULLABY || song == CW_SONG_LULLABY_INTRO) {
        const int count = song == CW_SONG_LULLABY_INTRO ? 3 : 6;
        for (int i = 0; i < count; ++i) {
            const float a = i * Tau / count + t * .18f;
            const Point p{ 24 * std::cos(a), 14 * std::sin(t * .5f + i), 24 * std::sin(a) };
            SongMusicalNote(mesh, p, i % 3, .12f * std::sin(t * .5f + i), color, 225, camera);
        }
    } else if (song == CW_SONG_ELEGY) {
        // Fine amber dust falls slowly, without effigy heads or limbs.
        for (int i = 0; i < 8; ++i) {
            const float phase = float((frame % 480u + i * 60u) % 480u) / 480.f;
            // Sway shares the mote's fade period, so clock wraps stay continuous.
            const float angle = i * 2.399963f + .12f * std::sin(phase * Tau);
            const float radius = 18.f + (i % 3) * 3.f;
            const Point p{ radius * std::cos(angle), 26 - 52 * phase, radius * std::sin(angle) };
            const uint8_t alpha = uint8_t(190 * std::sin(phase * Tau * .5f));
            Glow(mesh, p, 1.5f, color, alpha / 2, camera);
            Glow(mesh, p, .65f, 0xFFD9A6, alpha, camera);
        }
    } else if (song == CW_SONG_OATH) {
        // Four widely spaced violet glints gently twinkle, without giant runes.
        const Point locations[] = { { -25, 15, 8 }, { 24, -4, -8 }, { -18, -23, -6 }, { 15, 25, -7 } };
        for (int i = 0; i < 4; ++i) {
            const float phase = float((frame % 240u + i * 60u) % 240u) / 240.f;
            const float wave = std::sin(phase * Tau * .5f), pulse = wave * wave;
            const size_t first = mesh.count;
            Star(mesh, locations[i], 1.6f + .8f * pulse, .1f * std::sin(t * .5f + i), color, camera, 0xE6C5ED);
            for (size_t j = first; j < mesh.count; ++j)
                mesh.vertices[j].alpha = uint8_t(mesh.vertices[j].alpha * pulse);
            Glow(mesh, locations[i], 2.3f, color, uint8_t(105 * pulse), camera);
        }
    } else if (song == CW_SONG_HEALING || song == CW_SONG_SONATA) {
        for (int i = 0; i < 6; ++i) {
            const float phase = float((frame % 240u + i * 40u) % 240u) / 240.f;
            const float angle = i * Tau / 6 + t * .3f + phase * Tau * .35f;
            const Point p{ 22 * std::cos(angle), -25 + 50 * phase, 22 * std::sin(angle) };
            const uint8_t alpha = uint8_t(215 * std::sin(phase * Tau * .5f));
            if (song == CW_SONG_HEALING) {
                SongHeart(mesh, p, 2.4f, color, alpha, camera);
                Glow(mesh, p + Plane(camera, 0, -4), 1.4f, color, alpha / 2, camera);
            } else {
                const float roll = t * .6f + i;
                const Point along = Plane(camera, 4 * std::cos(roll), 4 * std::sin(roll));
                const Point across = Plane(camera, -1.8f * std::sin(roll), 1.8f * std::cos(roll));
                mesh.Tri({ p + along, color, alpha }, { p + across, color, alpha }, { p - along, 0xDBFFBA, alpha });
                mesh.Tri({ p + along, color, alpha }, { p - along, 0xDBFFBA, alpha }, { p - across, color, alpha });
                Band(mesh, p - along, p + along, .15f, color, 0xDBFFBA, camera, alpha);
                Glow(mesh, p + Plane(camera, 0, -5), .8f, color, alpha / 2, camera);
            }
        }
    }
    return mesh;
}

inline Mesh SampleSong(int song, uint32_t frame, const Basis& camera = {}) {
    if (song == CW_SONG_HEALING || song == CW_SONG_SONATA || song == CW_SONG_LULLABY_INTRO || song == CW_SONG_LULLABY ||
        song == CW_SONG_NOVA || song == CW_SONG_ELEGY || song == CW_SONG_OATH || song == CW_SONG_INVERTED_TIME ||
        song == CW_SONG_DOUBLE_TIME)
        return SampleMmSong(song, frame, camera);
    Mesh mesh;
    const bool warp = song >= CW_SONG_OOT_MINUET && song <= CW_SONG_OOT_PRELUDE;
    const bool approved =
        song == CW_SONG_OOT_ZELDA || song == CW_SONG_SARIA || song == CW_SONG_OOT_SARIA || song == CW_SONG_SOARING;
    if (!warp && !approved)
        return mesh; // Plain regular notes; Epona/Sun use only the shared shimmer, Storms uses rain.
    const uint32_t color = ComboSongColorHex(song);
    const float t = (frame % 720u) * (Tau / 360.f);
    if (song == CW_SONG_OOT_BOLERO || song == CW_SONG_OOT_SERENADE)
        return mesh; // Keep the native note and shared shimmer; no encircling overlay.
    if (song == CW_SONG_SOARING) {
        for (int i = 0; i < 6; ++i) {
            const float phase = float((frame % 240u + i * 40u) % 240u) / 240.f;
            const float angle = i * Tau / 6 + t * .25f;
            const Point p{ 23 * std::cos(angle), 28 - 56 * phase, 23 * std::sin(angle) };
            SongFeather(mesh, p, t * .4f + i, color, uint8_t(210 * std::sin(phase * Tau * .5f)), camera);
        }
        return mesh;
    }
    const bool light = song == CW_SONG_OOT_PRELUDE;
    const bool leaves = song == CW_SONG_SARIA || song == CW_SONG_OOT_SARIA || song == CW_SONG_OOT_MINUET;
    for (int i = 0; i < 9; ++i) {
        const float phase = float((frame % 240u + i * 27u) % 240u) / 240.f;
        const float angle = i * 2.399963f + t * .4f;
        Point p{ 23 * std::cos(angle), light ? -28 + 56 * phase : 28 - 56 * phase, 23 * std::sin(angle) };
        const uint8_t alpha = uint8_t(210 * std::sin(phase * Tau * .5f));
        if (leaves) {
            const float roll = t + i;
            const Point along = Plane(camera, std::cos(roll) * 3, std::sin(roll) * 3);
            const Point across = Plane(camera, -std::sin(roll) * 1.4f, std::cos(roll) * 1.4f);
            mesh.Tri({ p + along, color, alpha }, { p + across, color, alpha }, { p - along, 0xD8FFD0, alpha });
            mesh.Tri({ p + along, color, alpha }, { p - along, 0xD8FFD0, alpha }, { p - across, color, alpha });
        } else {
            // Prelude has rising light motes; Zelda/Nocturne/Requiem keep their
            // existing bounded particles.
            Glow(mesh, p, 2.5f, color, alpha, camera);
        }
    }
    return mesh;
}
} // namespace NeiGi
