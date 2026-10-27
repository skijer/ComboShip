#ifndef COMBO_SPIN_ATTACK_ACCENTS_H
#define COMBO_SPIN_ATTACK_ACCENTS_H
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    float x, y, z, size;
    uint8_t r, g, b, a;
} CwSpinAccentParticle;

// Pure lifetime-derived motion: drawing particles never advances gameplay RNG.
static inline size_t CwSpinAccents_Build(uint8_t element, int great, float phase, float alpha,
                                         CwSpinAccentParticle* particles, size_t capacity) {
    if (element < 1 || element > 3 || particles == NULL || !isfinite(phase) || !isfinite(alpha) || alpha <= 0)
        return 0;
    if (alpha > 1)
        alpha = 1;
    size_t count = great ? 24 : 12;
    if (count > capacity)
        count = capacity;
    for (size_t i = 0; i < count; ++i) {
        CwSpinAccentParticle* p = &particles[i];
        memset(p, 0, sizeof(*p));
        float angle = (float)i * 2.39996323f + phase * (element == 2 ? -2.2f : 3.1f);
        float radius = 1300 + 450 * sinf((float)i * 1.7f + phase * 2);
        p->x = cosf(angle) * radius;
        p->z = sinf(angle) * radius;
        p->y = 160 + 650 * (0.5f + 0.5f * sinf(phase * 2.8f + (float)i));
        p->size = (element == 2 ? 60 : 38) + (float)(i % 3) * 10;
        p->r = element == 1 ? 255 : element == 2 ? 145 : 255;
        p->g = element == 1 ? 140 : element == 2 ? 225 : 242;
        p->b = element == 1 ? 45 : element == 2 ? 255 : 170;
        p->a = (uint8_t)(alpha * (170 + (i % 4) * 20));
    }
    return count;
}

// The native charge mesh runs along negative X (-3725..411), with a narrow
// Y/Z extent. Preview accents stay on that blade/glow frame, not the burst ring.
static inline size_t CwSpinAccents_BuildCharge(uint8_t element, int great, float phase, float alpha,
                                               CwSpinAccentParticle* particles, size_t capacity) {
    size_t limit = great ? 8 : 4;
    if (capacity > limit)
        capacity = limit;
    size_t count = CwSpinAccents_Build(element, great, phase, alpha, particles, capacity);
    for (size_t i = 0; i < count; ++i) {
        CwSpinAccentParticle* p = &particles[i];
        p->x = -500 - 2500 * ((float)i + 0.5f) / count;
        p->y = (p->y - 480) * 0.1f;
        p->z *= 0.06f;
        p->size *= 0.65f;
    }
    return count;
}
#endif
