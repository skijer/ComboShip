// combo/gui/ComboSettingsSync.h
//
// ComboShip: keeps the settings that mean the same thing in both games on the same value. The two
// DLLs share one ConsoleVariables store, so ~110 keys are already one setting; this covers the rest,
// where OOT and 2Ship happened to name the same option differently (gCheats.InfiniteMoney vs
// gCheats.InfiniteRupees, gSettings.FreeLook.* vs gEnhancements.Camera.FreeLook.*, ...).
//
// libultraship has no CVar change notification, and settings also move outside the combo menu
// (popout editors, presets, the console), so the reconcile is a per-frame poll from a logic-only
// GuiWindow rather than a hook at each write site.
#pragma once

namespace ComboSync {

// Register the per-frame reconcile window into the shared Gui.
void RegisterSync();

// ComboShip Settings -> Sync panel.
void DrawSyncSharedPanel();

} // namespace ComboSync
