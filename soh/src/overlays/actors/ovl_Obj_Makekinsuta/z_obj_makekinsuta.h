#ifndef Z_OBJ_MAKEKINSUTA_H
#define Z_OBJ_MAKEKINSUTA_H

#include <libultraship/libultra.h>
#include "global.h"

struct ObjMakekinsuta;

// Native bugs/shovel triggers use 1. Seasonal auto-reveal keeps the spawn but
// does not announce it as a solved puzzle.
#define SOFT_SOIL_TRIGGER_SEASONAL 2

typedef void (*ObjMakekinsutaActionFunc)(struct ObjMakekinsuta*, PlayState*);

typedef struct ObjMakekinsuta {
    /* 0x0000 */ Actor actor;
    /* 0x014C */ ObjMakekinsutaActionFunc actionFunc;
    /* 0x150  */ s16 timer;
    /* 0x152  */ s16 unk_152;
} ObjMakekinsuta; // size = 0x0154

#endif
