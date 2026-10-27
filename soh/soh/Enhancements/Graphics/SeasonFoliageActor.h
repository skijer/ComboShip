#pragma once

#include "soh/Enhancements/audio/SeasonWeather.h"

// Scope the renderer treatment to the selected foliage draw. This also reaches
// native and Alt model replacements without recoloring trunks or cut stumps.
static inline int SeasonFoliage_ActorBegin(Gfx** display, const PlayState* play, int canopy) {
    const int season = SeasonWeather_SeasonForPlay(play);
    unsigned red, green, blue, blend, mode;
    switch (season) {
        case 0:
            red = 246;
            green = 193;
            blue = 217;
            blend = 255;
            mode = 3; // Readable blossom canopies and grass blush; summer restores native color.
            break;
        case 2:
            red = canopy ? 239 : 242;
            green = canopy ? 72 : 104;
            blue = canopy ? 76 : 70;
            blend = 255;
            mode = 2; // Preserve the foliage's mean intensity while applying the autumn hue.
            break;
        case 3:
            red = canopy ? 226 : 220;
            green = canopy ? 238 : 236;
            blue = canopy ? 233 : 245;
            blend = 255;
            mode = 3; // Existing frost normalization preserves texture detail.
            break;
        default:
            return 0;
    }
    gDPSetGrayscaleColor((*display)++, red, green, blue, blend);
    gSPGrayscale((*display)++, mode);
    return 1;
}

static inline void SeasonFoliage_ActorEnd(Gfx** display, int active) {
    if (active) {
        gSPGrayscale((*display)++, false);
    }
}
