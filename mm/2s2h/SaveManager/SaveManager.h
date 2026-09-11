
#ifndef SAVE_MANAGER_H
#define SAVE_MANAGER_H

#ifdef __cplusplus
#include <string>
#include <filesystem>
#include <nlohmann/json.hpp>
#ifdef COMBO_BUILD
#include "rando/CrossForeign.h" // ComboShip: merged-save IO callback typedefs
// ComboShip: register the launcher's .combosav IO callbacks (routes file{N}.json IO into the container).
void SaveManager_SetComboSaveIO(ComboRando::FnComboReadSave r, ComboRando::FnComboWriteSave w);
#endif
std::string SaveManager_GetFileName(int fileNum, bool isBackup = false);
bool SaveManager_HandleFileDropped(char* filePath);
bool BinarySaveConverter_HandleFileDropped(char* filePath);
int SaveManager_GetOpenFileSlot();
void SaveManager_WriteSaveFile(const std::filesystem::path& fileName, nlohmann::json j);
// Remove a save file from the saves folder (no-op when it isn't there). Used by the fleet combo to
// keep MM's derived files paired with OoT's: OoT erases a file, MM's half goes with it.
void SaveManager_DeleteSaveFile(const std::filesystem::path& fileName);
// Read a save file's raw json WITHOUT loading it into the game. 0 = ok, -1 = no such file,
// -2 = unreadable/not json. Used by the fleet combo to check a slot's seed before booting it.
int SaveManager_ReadSaveFile(const std::filesystem::path& fileName, nlohmann::json& j);
void SaveManager_PersistSariaHintsAvailable();
// ComboShip: cross-game save activation/persistence entry points
#ifdef COMBO_BUILD
// Playable combo MM save in gSaveContext, nothing written: post-first-cycle Human Link with the
// mid-playthrough kit. ootName8 (optional) is the OOT-entered file name.
void SaveManager_BuildComboBaseline(const unsigned char* ootName8);
#endif
void SaveManager_InitNewSaveForSlot(int mmFileNum, const unsigned char* ootName8 = nullptr);
// 0 = loaded; negative = nothing usable was loaded (codes at the definition). Never creates or persists:
// a failure is logged and leaves the fail-closed sentinel behind, and play still proceeds.
int SaveManager_LoadSaveFile(int mmFileNum);
void SaveManager_SaveCurrentForCombo();
#else
void SaveManager_SysFlashrom_WriteData(u8* addr, u32 pageNum, u32 pageCount);
s32 SaveManager_SysFlashrom_ReadData(void* addr, u32 pageNum, u32 pageCount);
#endif

#endif // SAVE_MANAGER_H
