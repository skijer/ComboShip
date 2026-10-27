#ifndef COMBO_BOTTLE_CONTENTS_H
#define COMBO_BOTTLE_CONTENTS_H
#include <string.h>
#include "ComboBottleShimmer.h"

enum { CW_BOTTLE_MUSHROOM = 1, CW_BOTTLE_PRINCESS, CW_BOTTLE_GOLD_DUST, CW_BOTTLE_SEAHORSE };

static inline int ComboBottleContents_Profile(const char* path) {
    if (!path)
        return 0;
    if (strstr(path, "objects/combo_bottle_gi/MushroomBottle"))
        return CW_BOTTLE_MUSHROOM;
    if (strstr(path, "objects/combo_bottle_gi/PrincessBottle"))
        return CW_BOTTLE_PRINCESS;
    if (strstr(path, "objects/combo_bottle_gi/GoldDustBottle"))
        return CW_BOTTLE_GOLD_DUST;
    if (strstr(path, "objects/combo_bottle_gi/SeahorseBottle"))
        return CW_BOTTLE_SEAHORSE;
    return 0;
}

// Draw IDs and shimmer policy IDs are separate contracts, even where numbers coincide.
static inline int ComboBottleContents_ShimmerProfile(int content) {
    switch (content) {
        case CW_BOTTLE_MUSHROOM:
            return CW_SHIMMER_MUSHROOM;
        case CW_BOTTLE_PRINCESS:
            return CW_SHIMMER_PRINCESS;
        case CW_BOTTLE_GOLD_DUST:
            return CW_SHIMMER_GOLD_DUST;
        case CW_BOTTLE_SEAHORSE:
            return CW_SHIMMER_SEAHORSE;
        default:
            return 0;
    }
}

static inline const char* ComboBottleContents_MeshPath(int content) {
    switch (content) {
        case CW_BOTTLE_MUSHROOM:
            return "__OTR__objects/combo_bottle_gi/polish/mushroom/gi_dl";
        case CW_BOTTLE_PRINCESS:
            return "__OTR__objects/combo_bottle_gi/polish/princess/gi_dl";
        case CW_BOTTLE_SEAHORSE:
            return "__OTR__objects/combo_bottle_gi/polish/seahorse/gi_dl";
        default:
            return 0;
    }
}

// A root display list defers its vertex/matrix/texture loads to the GPU.
// Reject a partial bundled installation before queuing that deferred graph.
static inline const char* ComboBottleContents_Dependency(int content, int index) {
    static const char* mushroom[] = {
        "__OTR__objects/combo_bottle_gi/polish/mushroom/scale_mtx",
        "__OTR__objects/combo_bottle_gi/polish/mushroom/mesh_opa_vtx",
        "__OTR__objects/combo_bottle_gi/polish/mushroom/Mushroom_tex",
        0,
    };
    static const char* princess[] = {
        "__OTR__objects/combo_bottle_gi/polish/princess/scale_mtx",
        "__OTR__objects/combo_bottle_gi/polish/princess/mesh_opa_vtx",
        "__OTR__objects/combo_bottle_gi/polish/princess/material-0138A9AD_tex",
        "__OTR__objects/combo_bottle_gi/polish/princess/material-02FDF718_tex",
    };
    static const char* seahorse[] = {
        "__OTR__objects/combo_bottle_gi/polish/seahorse/scale_mtx",
        "__OTR__objects/combo_bottle_gi/polish/seahorse/mesh_opa_vtx",
        "__OTR__objects/combo_bottle_gi/polish/seahorse/material2_tex",
        "__OTR__objects/combo_bottle_gi/polish/seahorse/material3_tex",
    };
    if (index < 0 || index >= 4)
        return 0;
    switch (content) {
        case CW_BOTTLE_MUSHROOM:
            return mushroom[index];
        case CW_BOTTLE_PRINCESS:
            return princess[index];
        case CW_BOTTLE_SEAHORSE:
            return seahorse[index];
        default:
            return 0;
    }
}

static inline const char* ComboBottleContents_Marker(int content) {
    switch (content) {
        case CW_BOTTLE_MUSHROOM:
            return "__OTR__objects/combo_bottle_gi/MushroomBottle";
        case CW_BOTTLE_PRINCESS:
            return "__OTR__objects/combo_bottle_gi/PrincessBottle";
        case CW_BOTTLE_GOLD_DUST:
            return "__OTR__objects/combo_bottle_gi/GoldDustBottle";
        case CW_BOTTLE_SEAHORSE:
            return "__OTR__objects/combo_bottle_gi/SeahorseBottle";
        default:
            return 0;
    }
}

static inline int ComboBottleContents_Color(int content, unsigned char color[4]) {
    return ComboBottleShimmer_Color(ComboBottleContents_ShimmerProfile(content), color);
}

#ifdef __cplusplus
extern "C" {
#endif
struct PlayState;
int ComboBottleContents_Draw(struct PlayState* play, int content);
#ifdef __cplusplus
}
#endif
#endif
