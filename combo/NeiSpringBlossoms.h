#ifndef NEI_SPRING_BLOSSOMS_H
#define NEI_SPRING_BLOSSOMS_H

// Shared native geometry for both games. Call after the host's Vtx/GBI
// definitions; no scene texture, pack override or runtime allocation is needed.
// Five rounded blush petals, a crimson core and tiny golden anthers follow cor's
// blossom reference. The single petal has one rounded end, without a heart cleft.
static Vtx sSpringBlossomVertices[] = {
    VTX(0, 0, 12, 0, 0, 250, 170, 196, 255),      VTX(-56, 78, 5, 0, 0, 255, 223, 235, 255),
    VTX(-57, 129, 3, 0, 0, 255, 225, 236, 255),   VTX(-33, 154, -2, 0, 0, 255, 245, 249, 255),
    VTX(0, 160, -6, 0, 0, 255, 250, 252, 255),    VTX(33, 154, -2, 0, 0, 255, 245, 249, 255),
    VTX(57, 129, 3, 0, 0, 255, 225, 236, 255),    VTX(56, 78, 5, 0, 0, 255, 223, 235, 255),
    VTX(105, 94, 3, 0, 0, 255, 225, 236, 255),    VTX(136, 79, -2, 0, 0, 255, 245, 249, 255),
    VTX(152, 49, -6, 0, 0, 255, 250, 252, 255),   VTX(156, 16, -2, 0, 0, 255, 245, 249, 255),
    VTX(140, -15, 3, 0, 0, 255, 225, 236, 255),   VTX(91, -30, 5, 0, 0, 255, 223, 235, 255),
    VTX(122, -70, 3, 0, 0, 255, 225, 236, 255),   VTX(117, -105, -2, 0, 0, 255, 245, 249, 255),
    VTX(94, -129, -6, 0, 0, 255, 250, 252, 255),  VTX(64, -143, -2, 0, 0, 255, 245, 249, 255),
    VTX(29, -138, 3, 0, 0, 255, 225, 236, 255),   VTX(0, -96, 5, 0, 0, 255, 223, 235, 255),
    VTX(-29, -138, 3, 0, 0, 255, 225, 236, 255),  VTX(-64, -143, -2, 0, 0, 255, 245, 249, 255),
    VTX(-94, -129, -6, 0, 0, 255, 250, 252, 255), VTX(-117, -105, -2, 0, 0, 255, 245, 249, 255),
    VTX(-122, -71, 3, 0, 0, 255, 225, 236, 255),  VTX(-91, -30, 5, 0, 0, 255, 223, 235, 255),
    VTX(-140, -15, 3, 0, 0, 255, 225, 236, 255),  VTX(-156, 16, -2, 0, 0, 255, 245, 249, 255),
    VTX(-152, 49, -6, 0, 0, 255, 250, 252, 255),  VTX(-136, 79, -2, 0, 0, 255, 245, 249, 255),
    VTX(-105, 94, 3, 0, 0, 255, 225, 236, 255),
};
// Localize the red to the flower's small core. Painting the large fan's
// center red would spread that color across most of each pale petal.
static Vtx sSpringBlossomCoreVertices[] = {
    VTX(0, 0, 14, 0, 0, 172, 14, 39, 255),      VTX(0, 23, 14, 0, 0, 251, 183, 207, 255),
    VTX(14, 19, 14, 0, 0, 251, 183, 207, 255),  VTX(22, 7, 14, 0, 0, 251, 183, 207, 255),
    VTX(22, -7, 14, 0, 0, 251, 183, 207, 255),  VTX(14, -19, 14, 0, 0, 251, 183, 207, 255),
    VTX(0, -23, 14, 0, 0, 251, 183, 207, 255),  VTX(-14, -19, 14, 0, 0, 251, 183, 207, 255),
    VTX(-22, -7, 14, 0, 0, 251, 183, 207, 255), VTX(-22, 7, 14, 0, 0, 251, 183, 207, 255),
    VTX(-14, 19, 14, 0, 0, 251, 183, 207, 255),
};
static Vtx sSpringBlossomStamens[] = {
    VTX(-1, 7, 15, 0, 0, 206, 26, 47, 255),     VTX(1, 7, 15, 0, 0, 206, 26, 47, 255),
    VTX(0, 36, 15, 0, 0, 231, 67, 75, 255),     VTX(-4, 32, 15, 0, 0, 255, 210, 74, 255),
    VTX(4, 32, 15, 0, 0, 255, 210, 74, 255),    VTX(0, 40, 15, 0, 0, 255, 230, 119, 255),
    VTX(6, 3, 15, 0, 0, 206, 26, 47, 255),      VTX(7, 1, 15, 0, 0, 206, 26, 47, 255),
    VTX(34, 11, 15, 0, 0, 231, 67, 75, 255),    VTX(29, 14, 15, 0, 0, 255, 210, 74, 255),
    VTX(32, 6, 15, 0, 0, 255, 210, 74, 255),    VTX(38, 12, 15, 0, 0, 255, 230, 119, 255),
    VTX(5, -5, 15, 0, 0, 206, 26, 47, 255),     VTX(3, -6, 15, 0, 0, 206, 26, 47, 255),
    VTX(21, -29, 15, 0, 0, 231, 67, 75, 255),   VTX(22, -24, 15, 0, 0, 255, 210, 74, 255),
    VTX(16, -28, 15, 0, 0, 255, 210, 74, 255),  VTX(24, -32, 15, 0, 0, 255, 230, 119, 255),
    VTX(-3, -6, 15, 0, 0, 206, 26, 47, 255),    VTX(-5, -5, 15, 0, 0, 206, 26, 47, 255),
    VTX(-21, -29, 15, 0, 0, 231, 67, 75, 255),  VTX(-16, -28, 15, 0, 0, 255, 210, 74, 255),
    VTX(-22, -24, 15, 0, 0, 255, 210, 74, 255), VTX(-24, -32, 15, 0, 0, 255, 230, 119, 255),
    VTX(-7, 1, 15, 0, 0, 206, 26, 47, 255),     VTX(-6, 3, 15, 0, 0, 206, 26, 47, 255),
    VTX(-34, 11, 15, 0, 0, 231, 67, 75, 255),   VTX(-32, 6, 15, 0, 0, 255, 210, 74, 255),
    VTX(-29, 14, 15, 0, 0, 255, 210, 74, 255),  VTX(-38, 12, 15, 0, 0, 255, 230, 119, 255),
};
static Gfx sSpringBlossomGeometry[] = {
    gsSPVertex(sSpringBlossomVertices, 31, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSP2Triangles(0, 3, 4, 0, 0, 4, 5, 0),
    gsSP2Triangles(0, 5, 6, 0, 0, 6, 7, 0),
    gsSP2Triangles(0, 7, 8, 0, 0, 8, 9, 0),
    gsSP2Triangles(0, 9, 10, 0, 0, 10, 11, 0),
    gsSP2Triangles(0, 11, 12, 0, 0, 12, 13, 0),
    gsSP2Triangles(0, 13, 14, 0, 0, 14, 15, 0),
    gsSP2Triangles(0, 15, 16, 0, 0, 16, 17, 0),
    gsSP2Triangles(0, 17, 18, 0, 0, 18, 19, 0),
    gsSP2Triangles(0, 19, 20, 0, 0, 20, 21, 0),
    gsSP2Triangles(0, 21, 22, 0, 0, 22, 23, 0),
    gsSP2Triangles(0, 23, 24, 0, 0, 24, 25, 0),
    gsSP2Triangles(0, 25, 26, 0, 0, 26, 27, 0),
    gsSP2Triangles(0, 27, 28, 0, 0, 28, 29, 0),
    gsSP2Triangles(0, 29, 30, 0, 0, 30, 1, 0),
    gsSPVertex(sSpringBlossomCoreVertices, 11, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSP2Triangles(0, 3, 4, 0, 0, 4, 5, 0),
    gsSP2Triangles(0, 5, 6, 0, 0, 6, 7, 0),
    gsSP2Triangles(0, 7, 8, 0, 0, 8, 9, 0),
    gsSP2Triangles(0, 9, 10, 0, 0, 10, 1, 0),
    gsSPVertex(sSpringBlossomStamens, 30, 0),
    gsSP2Triangles(0, 1, 2, 0, 3, 4, 5, 0),
    gsSP2Triangles(6, 7, 8, 0, 9, 10, 11, 0),
    gsSP2Triangles(12, 13, 14, 0, 15, 16, 17, 0),
    gsSP2Triangles(18, 19, 20, 0, 21, 22, 23, 0),
    gsSP2Triangles(24, 25, 26, 0, 27, 28, 29, 0),
    gsSPEndDisplayList(),
};

static Vtx sSpringPetalVertices[] = {
    VTX(0, 10, 8, 0, 0, 255, 244, 248, 255),     VTX(0, -150, 0, 0, 0, 247, 197, 216, 255),
    VTX(46, -130, 0, 0, 0, 251, 219, 231, 255),  VTX(80, -90, 5, 0, 0, 255, 232, 240, 255),
    VTX(94, -15, 7, 0, 0, 255, 242, 247, 255),   VTX(86, 73, 3, 0, 0, 255, 249, 251, 255),
    VTX(53, 130, -3, 0, 0, 255, 252, 253, 255),  VTX(0, 147, -6, 0, 0, 255, 253, 254, 255),
    VTX(-44, 131, -5, 0, 0, 255, 251, 253, 255), VTX(-74, 85, -1, 0, 0, 255, 247, 250, 255),
    VTX(-91, 0, 4, 0, 0, 255, 238, 245, 255),    VTX(-78, -78, 5, 0, 0, 253, 228, 239, 255),
    VTX(-40, -125, 1, 0, 0, 249, 211, 228, 255),
};
static Gfx sSpringPetalGeometry[] = {
    gsSPVertex(sSpringPetalVertices, 13, 0),   gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSP2Triangles(0, 3, 4, 0, 0, 4, 5, 0),    gsSP2Triangles(0, 5, 6, 0, 0, 6, 7, 0),
    gsSP2Triangles(0, 7, 8, 0, 0, 8, 9, 0),    gsSP2Triangles(0, 9, 10, 0, 0, 10, 11, 0),
    gsSP2Triangles(0, 11, 12, 0, 0, 12, 1, 0), gsSPEndDisplayList(),
};

static inline Gfx* NeiSpringBlossoms_Geometry(unsigned index) {
    // 24 intact blossoms and 72 smaller loose petals within the existing
    // 96-slot layer. Selection is stable across pause, redraw and recycling.
    return (index & 3) == 0 ? sSpringBlossomGeometry : sSpringPetalGeometry;
}

static inline const u8* NeiSpringBlossoms_Color(unsigned index) {
    static const u8 colors[][3] = {
        { 255, 229, 239 },
        { 255, 214, 230 },
        { 255, 238, 245 },
        { 255, 204, 225 },
    };
    // Intact flowers span every blush variant, rather than sharing one tint.
    return colors[(index + index / 4) & 3];
}
#endif
