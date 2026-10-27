// MM's native draw identities for OoT-imported MM aliases. This changes only
// presentation: the foreign placement, grant owner and receipt name stay OoT.
#ifndef COMBO_MM_NATIVE_IMPORTS_H
#define COMBO_MM_NATIVE_IMPORTS_H

#include <cstdint>
#include <cstring>

inline int32_t ComboNativeMmImport(const char* name) {
    if (!name)
        return -1;
    struct Entry {
        const char* name;
        RandoItemId item;
    };
    static constexpr Entry entries[] = {
        { "Woodfall Map", RI_WOODFALL_MAP },
        { "Snowhead Map", RI_SNOWHEAD_MAP },
        { "Great Bay Map", RI_GREAT_BAY_MAP },
        { "Stone Tower Map", RI_STONE_TOWER_MAP },
        { "Woodfall Compass", RI_WOODFALL_COMPASS },
        { "Snowhead Compass", RI_SNOWHEAD_COMPASS },
        { "Great Bay Compass", RI_GREAT_BAY_COMPASS },
        { "Stone Tower Compass", RI_STONE_TOWER_COMPASS },
        { "Woodfall Small Key", RI_WOODFALL_SMALL_KEY },
        { "Snowhead Small Key", RI_SNOWHEAD_SMALL_KEY },
        { "Great Bay Small Key", RI_GREAT_BAY_SMALL_KEY },
        { "Stone Tower Small Key", RI_STONE_TOWER_SMALL_KEY },
        { "Woodfall Boss Key", RI_WOODFALL_BOSS_KEY },
        { "Snowhead Boss Key", RI_SNOWHEAD_BOSS_KEY },
        { "Great Bay Boss Key", RI_GREAT_BAY_BOSS_KEY },
        { "Stone Tower Boss Key", RI_STONE_TOWER_BOSS_KEY },
        { "Soul of Odolwa", RI_SOUL_BOSS_ODOLWA },
        { "Soul of Goht", RI_SOUL_BOSS_GOHT },
        { "Soul of Gyorg", RI_SOUL_BOSS_GYORG },
        { "Soul of Twinmold", RI_SOUL_BOSS_TWINMOLD },
        { "Soul of Majora", RI_SOUL_BOSS_MAJORA },
        { "Bottle With Gold Dust", RI_BOTTLE_GOLD_DUST },
        { "Bottle with Magic Mushroom", RI_OOT_BOTTLE_MAGIC_MUSHROOM },
        { "Room Key", RI_ROOM_KEY },
        { "Tingle's Clock Town Map", RI_TINGLE_MAP_CLOCK_TOWN },
        { "Tingle's Woodfall Map", RI_TINGLE_MAP_WOODFALL },
        { "Tingle's Snowhead Map", RI_TINGLE_MAP_SNOWHEAD },
        { "Tingle's Romani Ranch Map", RI_TINGLE_MAP_ROMANI_RANCH },
        { "Tingle's Great Bay Map", RI_TINGLE_MAP_GREAT_BAY },
        { "Tingle's Stone Tower Map", RI_TINGLE_MAP_STONE_TOWER },
    };
    for (const auto& entry : entries)
        if (std::strcmp(name, entry.name) == 0)
            return entry.item;
    return -1;
}
#endif
