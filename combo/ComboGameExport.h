#pragma once

// Shared declarations match the owning DLL's exported definitions. Consumers
// do not acquire its implementation export macro or export the other game's ABI.
#if defined(COMBO_BUILD) && (defined(SOH_BUILD_DLL) || defined(MM_BUILD_DLL))
#include "ComboExport.h"
#endif

#if defined(COMBO_BUILD) && defined(SOH_BUILD_DLL)
#define COMBO_OOT_EXPORT COMBO_EXPORT
#else
#define COMBO_OOT_EXPORT
#endif
#if defined(COMBO_BUILD) && defined(MM_BUILD_DLL)
#define COMBO_MM_EXPORT COMBO_EXPORT
#else
#define COMBO_MM_EXPORT
#endif
