#ifndef MM_SEASON_PARTICLES_H
#define MM_SEASON_PARTICLES_H

#include "mods/nei_save.h"
#include "libultraship/bridge/consolevariablebridge.h"

// Shared by the supplemental owner and native particle renderer. These switches
// control presentation only; native precipitation targets remain live.
static inline int MMSeasonParticles_IsFallingSeason(int season) {
    return season == SEASON_SPRING || season == SEASON_AUTUMN;
}

static inline int MMSeasonParticles_IsEnabled(int season) {
    switch (season) {
        case SEASON_SPRING:
            return CVarGetInteger("gAudioEditor.MMWeather.Spring.Blossoms", 1) != 0;
        case SEASON_AUTUMN:
            return CVarGetInteger("gAudioEditor.MMWeather.Autumn.Leaves", 1) != 0;
        case SEASON_WINTER:
            return CVarGetInteger("gAudioEditor.MMWeather.Winter.Snow", 1) != 0;
        default:
            return 0;
    }
}

static inline int MMSeasonParticles_HasFlurries(int season) {
    return season == SEASON_WINTER && MMSeasonParticles_IsEnabled(season) &&
           CVarGetInteger("gAudioEditor.MMWeather.Winter.Flurries", 0) != 0;
}
#endif
