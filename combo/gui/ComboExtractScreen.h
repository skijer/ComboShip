// combo/gui/ComboExtractScreen.h
// ComboShip: the combo-owned ROM-extraction screen, hosted in comboui.dll. Runs the libultraship
// frame loop. Selection proceeds through OoT, an optional complementary OoT ROM,
// then MM, before extracting the selected archives with progress bars.
// See ComboExtract.h for the ABI and docs/UPSTREAM_MERGES.md for the rationale.
#pragma once

#include "ComboExtract.h"
#include "ComboExport.h"

extern "C" COMBO_EXPORT int ComboUI_RunExtraction(const ComboExtractCallbacks* cb);
extern "C" COMBO_EXPORT int ComboUI_SelectGame(int currentMode);
