#pragma once
#ifdef COMBO_BUILD
#ifdef __cplusplus
#include <nlohmann/json.hpp>
void ComboSoloForestMm_SaveState(nlohmann::json& save, int slot);
extern "C" {
#endif
void ComboSoloForestMm_NormalizeEntry(void);
int ComboSoloForestMm_PassageAvailable(void);
#ifdef __cplusplus
}
#endif
#endif
