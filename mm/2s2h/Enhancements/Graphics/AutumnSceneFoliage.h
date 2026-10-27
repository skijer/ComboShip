#pragma once

struct PlayState;

// Reversible season variants of loaded foliage draws and exact mixed-image
// copies, including scene-owned Road vines and foliage-only randomizer grass.
void MMAutumnSceneFoliage_Update(const PlayState* play);
void MMAutumnSceneFoliage_Reset();
