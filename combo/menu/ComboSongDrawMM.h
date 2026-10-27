#pragma once
#include "ComboSongDraw.h"
// Include after MM's RandoItemId declarations.
static inline int ComboSongForMmItem(int id) {
    if (id >= RI_SONG_DOUBLE_TIME && id <= RI_SONG_TIME)
        return id - RI_SONG_DOUBLE_TIME;
    switch (id) {
        case RI_OOT_SONG_MINUET_OF_FOREST:
            return CW_SONG_OOT_MINUET;
        case RI_OOT_SONG_BOLERO_OF_FIRE:
            return CW_SONG_OOT_BOLERO;
        case RI_OOT_SONG_SERENADE_OF_WATER:
            return CW_SONG_OOT_SERENADE;
        case RI_OOT_SONG_REQUIEM_OF_SPIRIT:
            return CW_SONG_OOT_REQUIEM;
        case RI_OOT_SONG_NOCTURNE_OF_SHADOW:
            return CW_SONG_OOT_NOCTURNE;
        case RI_OOT_SONG_PRELUDE_OF_LIGHT:
            return CW_SONG_OOT_PRELUDE;
        case RI_OOT_SONG_ZELDAS_LULLABY:
            return CW_SONG_OOT_ZELDA;
        default:
            return -1;
    }
}
