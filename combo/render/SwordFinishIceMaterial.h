#pragma once
#include <stdint.h>
#include <libultraship/libultra/gbi.h>

#ifdef __cplusplus
extern "C" {
#endif
// NULL preserves the original native/Alt material.
Gfx* SwordFinishIce_GetDisplayList(uint32_t frame);
#ifdef __cplusplus
}
#endif
