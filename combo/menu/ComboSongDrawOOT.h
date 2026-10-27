#pragma once
#include "ComboSongDraw.h"
// Native color lists retain the vanilla clef material and normal base/Alt/mod
// resolution. A grayscale replacement would bypass this authored material.
static inline const char* ComboSongOotColorDlist(int song) {
    static const char* colors[] = {
        "__OTR__objects/object_gi_melody/gGiMinuetColorDL",   "__OTR__objects/object_gi_melody/gGiBoleroColorDL",
        "__OTR__objects/object_gi_melody/gGiSerenadeColorDL", "__OTR__objects/object_gi_melody/gGiRequiemColorDL",
        "__OTR__objects/object_gi_melody/gGiNocturneColorDL", "__OTR__objects/object_gi_melody/gGiPreludeColorDL"
    };
    return song >= CW_SONG_OOT_MINUET && song <= CW_SONG_OOT_PRELUDE ? colors[song - CW_SONG_OOT_MINUET] : nullptr;
}
// Include after OoT's RandomizerGet declarations. Identity must not be inferred
// from GID aliases: Healing, Soaring and Oath all used the same purple note.
static inline int ComboSongForOotItem(int rg) {
    switch (rg) {
        case RG_MINUET_OF_FOREST:
            return CW_SONG_OOT_MINUET;
        case RG_BOLERO_OF_FIRE:
            return CW_SONG_OOT_BOLERO;
        case RG_SERENADE_OF_WATER:
            return CW_SONG_OOT_SERENADE;
        case RG_REQUIEM_OF_SPIRIT:
            return CW_SONG_OOT_REQUIEM;
        case RG_NOCTURNE_OF_SHADOW:
            return CW_SONG_OOT_NOCTURNE;
        case RG_PRELUDE_OF_LIGHT:
            return CW_SONG_OOT_PRELUDE;
        case RG_ZELDAS_LULLABY:
            return CW_SONG_OOT_ZELDA;
        case RG_EPONAS_SONG:
            return CW_SONG_OOT_EPONA;
        case RG_SARIAS_SONG:
            return CW_SONG_OOT_SARIA;
        case RG_SUNS_SONG:
            return CW_SONG_SUN;
        case RG_SONG_OF_TIME:
            return CW_SONG_TIME;
        case RG_SONG_OF_STORMS:
            return CW_SONG_STORMS;
        case RG_MM_SONG_DOUBLE_TIME:
            return CW_SONG_DOUBLE_TIME;
        case RG_MM_SONG_ELEGY:
            return CW_SONG_ELEGY;
        case RG_MM_SONG_EPONA:
            return CW_SONG_EPONA;
        case RG_MM_SONG_HEALING:
            return CW_SONG_HEALING;
        case RG_MM_SONG_INVERTED_TIME:
            return CW_SONG_INVERTED_TIME;
        case RG_MM_SONG_LULLABY_INTRO:
            return CW_SONG_LULLABY_INTRO;
        case RG_MM_SONG_LULLABY:
            return CW_SONG_LULLABY;
        case RG_MM_SONG_NOVA:
            return CW_SONG_NOVA;
        case RG_MM_SONG_OATH:
            return CW_SONG_OATH;
        case RG_MM_SONG_SARIA:
            return CW_SONG_SARIA;
        case RG_MM_SONG_SOARING:
            return CW_SONG_SOARING;
        case RG_MM_SONG_SONATA:
            return CW_SONG_SONATA;
        case RG_MM_SONG_STORMS:
            return CW_SONG_STORMS;
        case RG_MM_SONG_SUN:
            return CW_SONG_SUN;
        case RG_MM_SONG_TIME:
            return CW_SONG_TIME;
        default:
            return -1;
    }
}
