#ifndef COMBO_SONG_DRAW_H
#define COMBO_SONG_DRAW_H
#include <stdint.h>

// Stable presentation indices follow MM's native song block, independently
// of either game's item/GI numeric values.
enum {
    CW_SONG_DOUBLE_TIME,
    CW_SONG_ELEGY,
    CW_SONG_EPONA,
    CW_SONG_HEALING,
    CW_SONG_INVERTED_TIME,
    CW_SONG_LULLABY_INTRO,
    CW_SONG_LULLABY,
    CW_SONG_NOVA,
    CW_SONG_OATH,
    CW_SONG_SARIA,
    CW_SONG_SOARING,
    CW_SONG_SONATA,
    CW_SONG_STORMS,
    CW_SONG_SUN,
    CW_SONG_TIME,
    CW_SONG_OOT_MINUET,
    CW_SONG_OOT_BOLERO,
    CW_SONG_OOT_SERENADE,
    CW_SONG_OOT_REQUIEM,
    CW_SONG_OOT_NOCTURNE,
    CW_SONG_OOT_PRELUDE,
    CW_SONG_OOT_ZELDA,
    CW_SONG_OOT_EPONA,
    CW_SONG_OOT_SARIA,
    CW_SONG_COUNT
};
// OoT's native item song block is Minuet..Storms. Keep its established model
// colors as the source for receipt tint, without importing either game's enums.
static inline int ComboSongForOotSongIndex(int index) {
    static const int songs[] = { CW_SONG_OOT_MINUET,   CW_SONG_OOT_BOLERO,  CW_SONG_OOT_SERENADE, CW_SONG_OOT_REQUIEM,
                                 CW_SONG_OOT_NOCTURNE, CW_SONG_OOT_PRELUDE, CW_SONG_OOT_ZELDA,    CW_SONG_OOT_EPONA,
                                 CW_SONG_OOT_SARIA,    CW_SONG_SUN,         CW_SONG_TIME,         CW_SONG_STORMS };
    return index >= 0 && index < 12 ? songs[index] : -1;
}
// Hex themes are shared by native notes, exported recipes and their particles.
// Related songs retain a family hue while each MM song has its own shade.
static inline uint32_t ComboSongColorHex(int song) {
    static const uint32_t colors[CW_SONG_COUNT] = { 0x80D8F0, 0xFF6200, 0xD96E30, 0xFF96E6, 0x4A70CA, 0xFF6464,
                                                    0xFF1414, 0x1414FF, 0x620062, 0x6ACB62, 0xC8A0FF, 0x62FF62,
                                                    0x929292, 0xEDE73E, 0x62B1D3, 0x62FF62, 0xFF3C00, 0x55B4DF,
                                                    0xDE9E2F, 0xA028D2, 0xEDE73E, 0x6D498F, 0xD96E30, 0x3E6D17 };
    return song >= 0 && song < CW_SONG_COUNT ? colors[song] : 0;
}
static inline int ComboSongAlwaysShimmers(int song) {
    return (song >= CW_SONG_OOT_MINUET && song <= CW_SONG_OOT_PRELUDE) || song == CW_SONG_EPONA ||
           song == CW_SONG_OOT_EPONA || song == CW_SONG_SUN || song == CW_SONG_HEALING || song == CW_SONG_SONATA ||
           song == CW_SONG_LULLABY_INTRO || song == CW_SONG_LULLABY || song == CW_SONG_NOVA || song == CW_SONG_ELEGY ||
           song == CW_SONG_OATH || song == CW_SONG_INVERTED_TIME || song == CW_SONG_DOUBLE_TIME;
}
// Portable itemShimmer also gates song-local overlays, including Soaring's
// authored feathers. Other regular songs keep their plain note.
static inline int ComboSongHasOverlay(int song) {
    return ComboSongAlwaysShimmers(song) || song == CW_SONG_STORMS || song == CW_SONG_OOT_ZELDA ||
           song == CW_SONG_SARIA || song == CW_SONG_OOT_SARIA || song == CW_SONG_SOARING;
}
static inline int ComboSongShimmerColor(int song, uint8_t color[4]) {
    if (song < 0 || song >= CW_SONG_COUNT || !color)
        return 0;
    const uint32_t rgb = ComboSongColorHex(song);
    color[0] = (uint8_t)(rgb >> 16);
    color[1] = (uint8_t)(rgb >> 8);
    color[2] = (uint8_t)rgb;
    color[3] = 255;
    return 1;
}
#endif
