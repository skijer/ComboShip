#pragma once
#include "Rando/Types.h"
#include "2s2h/CustomMessage/CustomMessage.h"
#ifdef COMBO_BUILD
#include "ComboExport.h"
#endif

namespace Rando {
bool ApplyItemReceiptText(RandoItemId id, CustomMessage::Entry& entry);
// Native repeat/minigame awards bypass the randomizer check receipt builders.
bool ApplyNativeRandomRupeeReceipt(uint16_t textId, CustomMessage::Entry& entry);
// Read-only seed information; returns no text while the shared setting is disabled.
bool MapCompassInfoEnabled();
std::string GetDungeonMapCompassInfo(int32_t dungeon, bool compass);
#ifdef COMBO_BUILD
bool ApplyForeignItemReceiptText(const char* itemName, CustomMessage::Entry& entry, RandoCheckId check = RC_UNKNOWN);
#endif
// Append attribution after the description while retaining its encoding.
void AppendReceiptSource(CustomMessage::Entry& entry, const std::string& source);
} // namespace Rando

#ifdef COMBO_BUILD
extern "C" COMBO_EXPORT int32_t MM_GetSeedItemIconInfo(const char* name, CwItemIconInfo* out);
extern "C" COMBO_EXPORT int32_t MM_GetDungeonRewardIconInfo(int32_t dungeon, CwItemIconInfo* out);
extern "C" COMBO_EXPORT int32_t MM_GetDungeonRewardName(int32_t dungeon, char* buffer, uint32_t capacity);
extern "C" COMBO_EXPORT int32_t MM_GetDungeonEntranceName(int32_t dungeon, char* buffer, uint32_t capacity);
extern "C" COMBO_EXPORT int32_t MM_GetDungeonBossName(int32_t dungeon, char* buffer, uint32_t capacity);
// Cross source: positive byte count, zero when excluded, -1 for unusable saved data.
extern "C" COMBO_EXPORT int32_t MM_GetCrossDungeonEntranceSourceName(int32_t game, int32_t entrance, char* buffer,
                                                                     uint32_t capacity);
extern "C" COMBO_EXPORT int32_t MM_GetInformationBossDestination(int32_t boss);
// Physical reward services never apply an entrance permutation a second time.
extern "C" COMBO_EXPORT int32_t MM_GetPhysicalBossRewardName(int32_t boss, char* buffer, uint32_t capacity);
extern "C" COMBO_EXPORT int32_t MM_GetPhysicalBossRewardIconInfo(int32_t boss, CwItemIconInfo* out);
#endif
