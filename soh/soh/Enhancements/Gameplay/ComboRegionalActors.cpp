#ifdef COMBO_BUILD
#include "ComboRegionalActors.h"

// Regional catalogue actors are placed explicitly in Prelude. Retain this
// entry point for existing callers; scene/player hooks must not add defaults.
extern "C" void ComboRegionalActorsOot_Update(void) {
}
#endif
