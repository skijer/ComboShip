#pragma once
#include "CrossDungeonEntrances.h"
#include <array>
#include <utility>

// Include after the native MM entrance and region enums. These are logical portal identities;
// virtual grotto returns must pass through ComboMmMaterializeGrotto before native scene loading.
namespace ComboCrossMmCatalogue {
using ComboRando::CrossDungeon::Dungeon;
using ComboRando::CrossDungeon::InteriorKind;
using ComboRando::CrossDungeon::InteriorPool;
using ComboRando::CrossDungeon::Pool;
using ComboRando::CrossDungeon::PoolOptions;

struct RespawnDescriptor {
    int32_t entrance;
    int room, data, yaw;
    int x, y, z;
};
struct GrottoDescriptor {
    int logicalSpawn, physicalSpawn, room;
    RandoRegionId region, sourceRegion;
    int32_t nativeReturn;
    bool holeReturn;
    RespawnDescriptor hole, down;
    int32_t Entry() const {
        return ENTRANCE(GROTTOS, logicalSpawn);
    }
    int32_t Exit() const {
        return 0xF000 + logicalSpawn;
    }
    int32_t PhysicalEntry() const {
        return ENTRANCE(GROTTOS, physicalSpawn);
    }
};

inline const std::array<GrottoDescriptor, 26>& Grottos() {
    // Native content, yaw, XYZ and separate void data: DeveloperTools/BetterMapSelect.c:76–165.
    // Region and native portal pairing: Rando/Logic/Regions; reused identities: Logic.cpp:18–106.
    static const std::array<GrottoDescriptor, 26> grottos = { {
        { 0,
          0,
          0,
          RR_TERMINA_FIELD_GOSSIP_STONE_GROTTO_3,
          RR_TERMINA_FIELD,
          ENTRANCE(TERMINA_FIELD, 0),
          true,
          { ENTRANCE(TERMINA_FIELD, 0), 0, 0x1F, -5644, -2782, 48, -1654 },
          { ENTRANCE(TERMINA_FIELD, 0), 0, 0x01, -16384, -2400, 68, -400 } },
        { 1,
          1,
          1,
          RR_TERMINA_FIELD_GOSSIP_STONE_GROTTO_1,
          RR_TERMINA_FIELD,
          ENTRANCE(TERMINA_FIELD, 0),
          true,
          { ENTRANCE(TERMINA_FIELD, 0), 0, 0x1F, -21118, -1592, -222, 4622 },
          { ENTRANCE(TERMINA_FIELD, 6), 0, 0x01, 0, -412, -77, 1681 } },
        { 2,
          2,
          2,
          RR_TERMINA_FIELD_GOSSIP_STONE_GROTTO_4,
          RR_TERMINA_FIELD,
          ENTRANCE(TERMINA_FIELD, 0),
          true,
          { ENTRANCE(TERMINA_FIELD, 0), 0, 0x1F, -27125, 4450, 254, 925 },
          { ENTRANCE(TERMINA_FIELD, 7), 0, 0x01, 16384, 1672, 68, -394 } },
        { 3,
          3,
          3,
          RR_TERMINA_FIELD_GOSSIP_STONE_GROTTO_2,
          RR_TERMINA_FIELD,
          ENTRANCE(TERMINA_FIELD, 0),
          true,
          { ENTRANCE(TERMINA_FIELD, 0), 0, 0x1F, -11287, 192, 48, -3138 },
          { ENTRANCE(TERMINA_FIELD, 8), 0, 0x01, -32768, -400, 48, -2520 } },
        { 5,
          5,
          14,
          RR_PATH_TO_GORON_VILLAGE_HOT_SPRING_GROTTO,
          RR_PATH_TO_GORON_VILLAGE,
          ENTRANCE(PATH_TO_GORON_VILLAGE_WINTER, 0),
          true,
          { ENTRANCE(PATH_TO_GORON_VILLAGE_WINTER, 0), 0, 0xFF, -21846, 589, 195, 53 },
          { ENTRANCE(PATH_TO_GORON_VILLAGE_WINTER, 0), 0, 0x01, 20024, -2044, 200, 1288 } },
        { 7,
          7,
          7,
          RR_TERMINA_FIELD_DODONGO_GROTTO,
          RR_TERMINA_FIELD,
          ENTRANCE(TERMINA_FIELD, 0),
          true,
          { ENTRANCE(TERMINA_FIELD, 0), 0, 0x1F, 10558, -2425, -281, -3291 },
          { ENTRANCE(TERMINA_FIELD, 8), 0, 0x01, -32768, -400, 48, -2520 } },
        { 9,
          9,
          9,
          RR_TERMINA_FIELD_SCRUB_GROTTO,
          RR_TERMINA_FIELD,
          ENTRANCE(TERMINA_FIELD, 0),
          true,
          { ENTRANCE(TERMINA_FIELD, 0), 0, 0x1F, -19479, 3223, 219, 1417 },
          { ENTRANCE(TERMINA_FIELD, 7), 0, 0x01, 16384, 1672, 68, -394 } },
        { 11,
          11,
          11,
          RR_TERMINA_FIELD_BIO_BABA_GROTTO,
          RR_TERMINA_FIELD,
          ENTRANCE(TERMINA_FIELD, 0),
          true,
          { ENTRANCE(TERMINA_FIELD, 0), 0, 0x1F, 0, -5159, -281, -571 },
          { ENTRANCE(TERMINA_FIELD, 0), 0, 0x01, -16384, -2400, 68, -400 } },
        { 12,
          12,
          12,
          RR_DEKU_PALACE_BEAN_SALESMAN_GROTTO,
          RR_DEKU_PALACE_INSIDE_LOWER,
          ENTRANCE(DEKU_PALACE, 9),
          false,
          { 0, 0, 0xFF, 0, 0, 0, 0 },
          { ENTRANCE(DEKU_PALACE, 2), 0, 0x01, 0, 5, 0, 1433 } },
        { 13,
          13,
          13,
          RR_TERMINA_FIELD_PEAHAT_GROTTO,
          RR_TERMINA_FIELD,
          ENTRANCE(TERMINA_FIELD, 0),
          true,
          { ENTRANCE(TERMINA_FIELD, 0), 0, 0x1F, 21663, -2317, -221, 3418 },
          { ENTRANCE(TERMINA_FIELD, 6), 0, 0x01, 0, -412, -77, 1681 } },
        { 16,
          16,
          5,
          RR_LONE_PEAK_SHRINE,
          RR_LONE_PEAK_SHRINE_ENTRANCE,
          ENTRANCE(GORON_VILLAGE_WINTER, 3),
          false,
          { 0, 0, 0xFF, 0, 0, 0, 0 },
          { ENTRANCE(GORON_VILLAGE_WINTER, 3), 0, 0x01, -12015, 2621, -200, -1389 } },
        { 17,
          10,
          10,
          RR_TERMINA_FIELD_COW_GROTTO,
          RR_TERMINA_FIELD,
          ENTRANCE(TERMINA_FIELD, 0),
          true,
          { ENTRANCE(TERMINA_FIELD, 0), 0, 0x1F, -32768, -375, -222, 3976 },
          { ENTRANCE(TERMINA_FIELD, 6), 0, 0x01, 0, -412, -77, 1681 } },
        { 18,
          4,
          4,
          RR_TERMINA_FIELD_PILLAR_GROTTO,
          RR_TERMINA_FIELD,
          ENTRANCE(TERMINA_FIELD, 0),
          true,
          { ENTRANCE(TERMINA_FIELD, 0), 0, 0x9A, 0, 2367, 315, -192 },
          { ENTRANCE(TERMINA_FIELD, 7), 0, 0x01, 16384, 1672, 68, -394 } },
        { 19,
          4,
          4,
          RR_TERMINA_FIELD_TALL_GRASS_GROTTO,
          RR_TERMINA_FIELD,
          ENTRANCE(TERMINA_FIELD, 0),
          true,
          { ENTRANCE(TERMINA_FIELD, 0), 0, 0x3F, -22028, 1012, -221, 3642 },
          { ENTRANCE(TERMINA_FIELD, 6), 0, 0x01, 0, -412, -77, 1681 } },
        { 20,
          4,
          4,
          RR_ROAD_TO_SOUTHERN_SWAMP_GROTTO,
          RR_ROAD_TO_SOUTHERN_SWAMP,
          ENTRANCE(ROAD_TO_SOUTHERN_SWAMP, 0),
          true,
          { ENTRANCE(ROAD_TO_SOUTHERN_SWAMP, 0), 0, 0x3E, 4187, 104, -182, 2202 },
          { ENTRANCE(ROAD_TO_SOUTHERN_SWAMP, 0), 0, 0x01, -3641, 331, -143, 245 } },
        { 21,
          4,
          4,
          RR_SOUTHERN_SWAMP_GROTTO,
          RR_SOUTHERN_SWAMP_SOUTH,
          ENTRANCE(SOUTHERN_SWAMP_POISONED, 3),
          true,
          { ENTRANCE(SOUTHERN_SWAMP_POISONED, 0), 1, 0x3D, -5462, -1700, 38, 1800 },
          { ENTRANCE(SOUTHERN_SWAMP_POISONED, 8), 1, 0x01, -2731, -1049, 12, 2042 } },
        { 22,
          4,
          4,
          RR_WOODS_OF_MYSTERY_GROTTO,
          RR_WOODS_OF_MYSTERY,
          ENTRANCE(WOODS_OF_MYSTERY, 0),
          true,
          { ENTRANCE(WOODS_OF_MYSTERY, 0), 2, 0x5C, 0, 2, 0, -889 },
          { ENTRANCE(WOODS_OF_MYSTERY, 0), 1, 0x01, -16384, 274, 0, 0 } },
        { 23,
          4,
          4,
          RR_MOUNTAIN_VILLAGE_TUNNEL_GROTTO,
          RR_MOUNTAIN_VILLAGE,
          ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 0),
          true,
          { ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 0), 1, 0x3B, -10377, 2406, 1168, -1197 },
          { ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 2), 0, 0x01, -22210, 2089, 15, 939 } },
        { 24,
          4,
          4,
          RR_PATH_TO_GORON_VILLAGE_RAMP_GROTTO,
          RR_PATH_TO_GORON_VILLAGE,
          ENTRANCE(PATH_TO_GORON_VILLAGE_WINTER, 0),
          true,
          { ENTRANCE(PATH_TO_GORON_VILLAGE_WINTER, 0), 0, 0x99, 12379, -1309, 320, 143 },
          { ENTRANCE(PATH_TO_GORON_VILLAGE_WINTER, 0), 0, 0x01, 20024, -2044, 200, 1288 } },
        { 25,
          4,
          4,
          RR_PATH_TO_SNOWHEAD_GROTTO,
          RR_PATH_TO_SNOWHEAD_UPPER,
          ENTRANCE(PATH_TO_SNOWHEAD, 1),
          true,
          { ENTRANCE(PATH_TO_SNOWHEAD, 0), 0, 0x33, -19479, -987, 360, -2339 },
          { ENTRANCE(PATH_TO_SNOWHEAD, 1), 0, 0x01, 8192, -2518, 550, -3441 } },
        { 26,
          4,
          4,
          RR_GREAT_BAY_COAST_FISHERMAN_GROTTO,
          RR_GREAT_BAY_COAST,
          ENTRANCE(GREAT_BAY_COAST, 0),
          true,
          { ENTRANCE(GREAT_BAY_COAST, 0), 0, 0x37, -11287, 1359, 80, 5018 },
          { ENTRANCE(GREAT_BAY_COAST, 4), 0, 0x01, -28217, 1137, 92, 4635 } },
        { 27,
          10,
          10,
          RR_GREAT_BAY_COAST_COW_GROTTO,
          RR_GREAT_BAY_COAST_CLIFFSIDE,
          ENTRANCE(GREAT_BAY_COAST, 0),
          true,
          { ENTRANCE(GREAT_BAY_COAST, 0), 0, 0xFF, -13654, 2077, 333, -215 },
          { ENTRANCE(GREAT_BAY_COAST, 6), 0, 0x01, 16019, 1321, -16, -1029 } },
        { 28,
          4,
          4,
          RR_ZORA_CAPE_GROTTO,
          RR_ZORA_CAPE,
          ENTRANCE(ZORA_CAPE, 0),
          true,
          { ENTRANCE(ZORA_CAPE, 0), 0, 0x95, -14018, -562, 80, 2707 },
          { ENTRANCE(ZORA_CAPE, 0), 0, 0x01, 3583, 92, 12, 333 } },
        { 29,
          4,
          4,
          RR_IKANA_CANYON_GROTTO,
          RR_IKANA_CANYON_LOWER,
          ENTRANCE(IKANA_CANYON, 0),
          true,
          { ENTRANCE(IKANA_CANYON, 0), 2, 0xB4, -28217, -2475, -505, 2475 },
          { ENTRANCE(IKANA_CANYON, 12), 2, 0x01, 13653, -3068, -505, 2690 } },
        { 30,
          4,
          4,
          RR_IKANA_GRAVEYARD_GROTTO,
          RR_IKANA_GRAVEYARD_LOWER,
          ENTRANCE(IKANA_GRAVEYARD, 0),
          true,
          { ENTRANCE(IKANA_GRAVEYARD, 0), 1, 0xB8, -28217, 106, 314, -1777 },
          { ENTRANCE(IKANA_GRAVEYARD, 0), 0, 0x01, -32768, -45, -49, 864 } },
        { 31,
          4,
          4,
          RR_ROAD_TO_IKANA_GROTTO,
          RR_ROAD_TO_IKANA_FIELD_SIDE,
          ENTRANCE(ROAD_TO_IKANA, 0),
          true,
          { ENTRANCE(ROAD_TO_IKANA, 0), 0, 0x96, -19479, -428, 200, -335 },
          { ENTRANCE(ROAD_TO_IKANA, 0), 0, 0x01, 14563, -3006, 0, -305 } },
    } };
    return grottos;
}
inline const GrottoDescriptor* FindGrottoByEntry(int32_t logical) {
    for (const auto& grotto : Grottos())
        if (grotto.Entry() == logical)
            return &grotto;
    return nullptr;
}
inline const GrottoDescriptor* FindGrottoByExit(int32_t logical) {
    for (const auto& grotto : Grottos())
        if (grotto.Exit() == logical)
            return &grotto;
    return nullptr;
}
inline int32_t CanonicalGrottoLoad(int32_t physical, uint8_t content) {
    for (const auto& grotto : Grottos())
        if (grotto.PhysicalEntry() == physical &&
            ((grotto.physicalSpawn != 4 && grotto.physicalSpawn != 10) || grotto.hole.data == content))
            return grotto.Entry();
    return physical;
}
inline int32_t LogicalGrottoForRoom(int room, uint8_t content) {
    for (const auto& grotto : Grottos())
        if (grotto.room == room && ((room != 4 && room != 10) || grotto.hole.data == content))
            return grotto.Entry();
    return -1;
}
inline int32_t GrottoExitForRegion(RandoRegionId region) {
    for (const auto& grotto : Grottos())
        if (grotto.region == region)
            return grotto.Exit();
    return -1;
}
inline RandoRegionId RegionForGrottoReturn(int32_t logical) {
    const auto* grotto = FindGrottoByExit(logical);
    return grotto ? grotto->sourceRegion : RR_MAX;
}
inline RandoRegionId RegionForGrottoEntry(int32_t logical) {
    const auto* grotto = FindGrottoByEntry(logical);
    return grotto ? grotto->region : RR_MAX;
}

struct PortalDescriptor {
    const char* id;
    int32_t entrance, exit;
    RandoRegionId region, sourceRegion;
    InteriorKind kind = InteriorKind::Passage;
};
inline const std::array<PortalDescriptor, 42>& Interiors() {
    // Explicit native reversible ingress set; source conditions stay in the native graph.
    // Clock Tower Interior remains owned by the original cross-game link.
    // Kinds describe the physical ingress: native En_Door transitions and their
    // retail use. Open tunnels/caves remain Passage, including Observatory spawn 0.
    // Zora Hall rooms and Observatory spawn 1 have actual opening doors.
    static const std::array<PortalDescriptor, 42> interiors = { {
        { "mm:interior:fishermans_hut:0", ENTRANCE(FISHERMANS_HUT, 0), ENTRANCE(GREAT_BAY_COAST, 4), RR_FISHERMANS_HUT,
          RR_GREAT_BAY_COAST, InteriorKind::Passage },
        { "mm:interior:marine_research_lab:0", ENTRANCE(MARINE_RESEARCH_LAB, 0), ENTRANCE(GREAT_BAY_COAST, 7),
          RR_MARINE_RESEARCH_LAB, RR_GREAT_BAY_COAST_MARINE_LAB_EXTERIOR, InteriorKind::Door },
        { "mm:interior:fairy_fountain:3", ENTRANCE(FAIRY_FOUNTAIN, 3), ENTRANCE(ZORA_CAPE, 5),
          RR_GREAT_BAY_GREAT_FAIRY_FOUNTAIN, RR_ZORA_CAPE_OUTSIDE_FAIRY_FOUNTAIN, InteriorKind::Passage },
        { "mm:interior:zora_hall_rooms:5", ENTRANCE(ZORA_HALL_ROOMS, 5), ENTRANCE(ZORA_HALL, 2), RR_ZORA_HALL_SHOP,
          RR_ZORA_HALL, InteriorKind::ShopDoor },
        { "mm:interior:zora_hall_rooms:2", ENTRANCE(ZORA_HALL_ROOMS, 2), ENTRANCE(ZORA_HALL, 3),
          RR_ZORA_HALL_LULUS_ROOM, RR_ZORA_HALL, InteriorKind::Door },
        { "mm:interior:zora_hall_rooms:3", ENTRANCE(ZORA_HALL_ROOMS, 3), ENTRANCE(ZORA_HALL, 4),
          RR_ZORA_HALL_EVANS_ROOM, RR_ZORA_HALL, InteriorKind::Door },
        { "mm:interior:zora_hall_rooms:1", ENTRANCE(ZORA_HALL_ROOMS, 1), ENTRANCE(ZORA_HALL, 5),
          RR_ZORA_HALL_JAPAS_ROOM, RR_ZORA_HALL, InteriorKind::Door },
        { "mm:interior:zora_hall_rooms:0", ENTRANCE(ZORA_HALL_ROOMS, 0), ENTRANCE(ZORA_HALL, 6),
          RR_ZORA_HALL_MIKAUS_ROOM, RR_ZORA_HALL, InteriorKind::Door },
        { "mm:interior:swamp_shooting_gallery:0", ENTRANCE(SWAMP_SHOOTING_GALLERY, 0),
          ENTRANCE(ROAD_TO_SOUTHERN_SWAMP, 2), RR_SWAMP_SHOOTING_GALLERY, RR_ROAD_TO_SOUTHERN_SWAMP,
          InteriorKind::Door },
        { "mm:interior:tourist_information:0", ENTRANCE(TOURIST_INFORMATION, 0), ENTRANCE(SOUTHERN_SWAMP_POISONED, 1),
          RR_TOURIST_INFORMATION, RR_SOUTHERN_SWAMP_NORTH, InteriorKind::Door },
        { "mm:interior:magic_hags_potion_shop:0", ENTRANCE(MAGIC_HAGS_POTION_SHOP, 0),
          ENTRANCE(SOUTHERN_SWAMP_POISONED, 5), RR_MAGIC_HAGS_POTION_SHOP, RR_SOUTHERN_SWAMP_NEAR_WOODS,
          InteriorKind::ShopDoor },
        { "mm:interior:fairy_fountain:1", ENTRANCE(FAIRY_FOUNTAIN, 1), ENTRANCE(WOODFALL, 2),
          RR_WOODFALL_GREAT_FAIRY_FOUNTAIN, RR_WOODFALL, InteriorKind::Passage },
        { "mm:interior:ghost_hut:0", ENTRANCE(GHOST_HUT, 0), ENTRANCE(IKANA_CANYON, 1), RR_GHOST_HUT,
          RR_IKANA_CANYON_UPPER, InteriorKind::Passage },
        { "mm:interior:music_box_house:0", ENTRANCE(MUSIC_BOX_HOUSE, 0), ENTRANCE(IKANA_CANYON, 2), RR_MUSIC_BOX_HOUSE,
          RR_IKANA_CANYON_UPPER, InteriorKind::Door },
        { "mm:interior:fairy_fountain:4", ENTRANCE(FAIRY_FOUNTAIN, 4), ENTRANCE(IKANA_CANYON, 11),
          RR_IKANA_GREAT_FAIRY_FOUNTAIN, RR_IKANA_CANYON_UPPER, InteriorKind::Passage },
        { "mm:interior:ranch_house:0", ENTRANCE(RANCH_HOUSE, 0), ENTRANCE(ROMANI_RANCH, 2), RR_RANCH_BARN,
          RR_ROMANI_RANCH, InteriorKind::Door },
        { "mm:interior:ranch_house:1", ENTRANCE(RANCH_HOUSE, 1), ENTRANCE(ROMANI_RANCH, 3), RR_RANCH_HOUSE,
          RR_ROMANI_RANCH, InteriorKind::Door },
        { "mm:interior:cucco_shack:0", ENTRANCE(CUCCO_SHACK, 0), ENTRANCE(ROMANI_RANCH, 4), RR_CUCCO_SHACK,
          RR_ROMANI_RANCH, InteriorKind::Door },
        { "mm:interior:doggy_racetrack:0", ENTRANCE(DOGGY_RACETRACK, 0), ENTRANCE(ROMANI_RANCH, 5), RR_DOGGY_RACETRACK,
          RR_ROMANI_RANCH, InteriorKind::Door },
        { "mm:interior:astral_observatory:1", ENTRANCE(ASTRAL_OBSERVATORY, 1), ENTRANCE(TERMINA_FIELD, 9),
          RR_ASTRAL_OBSERVATORY, RR_ASTRAL_OBSERVATORY_OUTSIDE, InteriorKind::Door },
        { "mm:interior:astral_observatory:0", ENTRANCE(ASTRAL_OBSERVATORY, 0), ENTRANCE(EAST_CLOCK_TOWN, 2),
          RR_ASTRAL_OBSERVATORY_PASSAGE, RR_CLOCK_TOWN_EAST, InteriorKind::Passage },
        { "mm:interior:treasure_chest_shop:0", ENTRANCE(TREASURE_CHEST_SHOP, 0), ENTRANCE(EAST_CLOCK_TOWN, 4),
          RR_TREASURE_SHOP, RR_CLOCK_TOWN_EAST, InteriorKind::Door },
        { "mm:interior:honey_and_darlings_shop:0", ENTRANCE(HONEY_AND_DARLINGS_SHOP, 0), ENTRANCE(EAST_CLOCK_TOWN, 6),
          RR_HONEY_AND_DARLING, RR_CLOCK_TOWN_EAST, InteriorKind::Door },
        { "mm:interior:mayors_residence:0", ENTRANCE(MAYORS_RESIDENCE, 0), ENTRANCE(EAST_CLOCK_TOWN, 7),
          RR_MAYOR_RESIDENCE, RR_CLOCK_TOWN_EAST, InteriorKind::Door },
        { "mm:interior:town_shooting_gallery:0", ENTRANCE(TOWN_SHOOTING_GALLERY, 0), ENTRANCE(EAST_CLOCK_TOWN, 8),
          RR_TOWN_SHOOTING_GALLERY, RR_CLOCK_TOWN_EAST, InteriorKind::Door },
        { "mm:interior:stock_pot_inn:0", ENTRANCE(STOCK_POT_INN, 0), ENTRANCE(EAST_CLOCK_TOWN, 9), RR_INN,
          RR_CLOCK_TOWN_EAST, InteriorKind::Door },
        { "mm:interior:stock_pot_inn:1", ENTRANCE(STOCK_POT_INN, 1), ENTRANCE(EAST_CLOCK_TOWN, 10), RR_INN,
          RR_CLOCK_TOWN_EAST, InteriorKind::Door },
        { "mm:interior:milk_bar:0", ENTRANCE(MILK_BAR, 0), ENTRANCE(EAST_CLOCK_TOWN, 11), RR_MILK_BAR,
          RR_CLOCK_TOWN_EAST, InteriorKind::ShopDoor },
        { "mm:interior:curiosity_shop:1", ENTRANCE(CURIOSITY_SHOP, 1), ENTRANCE(LAUNDRY_POOL, 1),
          RR_CURIOSITY_SHOP_BACK, RR_CLOCK_TOWN_LAUNDRY, InteriorKind::Door },
        { "mm:interior:fairy_fountain:0", ENTRANCE(FAIRY_FOUNTAIN, 0), ENTRANCE(NORTH_CLOCK_TOWN, 3),
          RR_CLOCK_TOWN_GREAT_FAIRY_FOUNTAIN, RR_CLOCK_TOWN_NORTH, InteriorKind::Passage },
        { "mm:interior:deku_scrub_playground:0", ENTRANCE(DEKU_SCRUB_PLAYGROUND, 0), ENTRANCE(NORTH_CLOCK_TOWN, 4),
          RR_TOWN_DEKU_PLAYGROUND, RR_CLOCK_TOWN_NORTH, InteriorKind::Passage },
        { "mm:interior:swordmans_school:0", ENTRANCE(SWORDMANS_SCHOOL, 0), ENTRANCE(WEST_CLOCK_TOWN, 3),
          RR_SWORDSMAN_SCHOOL, RR_CLOCK_TOWN_WEST, InteriorKind::Door },
        { "mm:interior:curiosity_shop:0", ENTRANCE(CURIOSITY_SHOP, 0), ENTRANCE(WEST_CLOCK_TOWN, 4),
          RR_CURIOSITY_SHOP_FRONT, RR_CLOCK_TOWN_WEST, InteriorKind::ShopDoor },
        { "mm:interior:trading_post:0", ENTRANCE(TRADING_POST, 0), ENTRANCE(WEST_CLOCK_TOWN, 5), RR_TRADING_POST,
          RR_CLOCK_TOWN_WEST, InteriorKind::ShopDoor },
        { "mm:interior:bomb_shop:0", ENTRANCE(BOMB_SHOP, 0), ENTRANCE(WEST_CLOCK_TOWN, 6), RR_BOMB_SHOP,
          RR_CLOCK_TOWN_WEST, InteriorKind::ShopDoor },
        { "mm:interior:post_office:0", ENTRANCE(POST_OFFICE, 0), ENTRANCE(WEST_CLOCK_TOWN, 7), RR_POST_OFFICE,
          RR_CLOCK_TOWN_WEST, InteriorKind::Door },
        { "mm:interior:lottery_shop:0", ENTRANCE(LOTTERY_SHOP, 0), ENTRANCE(WEST_CLOCK_TOWN, 8), RR_LOTTERY_SHOP,
          RR_CLOCK_TOWN_WEST, InteriorKind::Door },
        { "mm:interior:goron_shop:0", ENTRANCE(GORON_SHOP, 0), ENTRANCE(GORON_SHRINE, 1), RR_GORON_SHOP,
          RR_GORON_SHRINE, InteriorKind::ShopPassage },
        { "mm:interior:goron_shrine:0", ENTRANCE(GORON_SHRINE, 0), ENTRANCE(GORON_VILLAGE_WINTER, 2), RR_GORON_SHRINE,
          RR_GORON_VILLAGE, InteriorKind::Passage },
        { "mm:interior:mountain_smithy:0", ENTRANCE(MOUNTAIN_SMITHY, 0), ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 1),
          RR_MOUNTAIN_SMITHY, RR_MOUNTAIN_VILLAGE, InteriorKind::Door },
        { "mm:interior:goron_graveryard:0", ENTRANCE(GORON_GRAVERYARD, 0), ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 3),
          RR_GORON_GRAVEYARD, RR_MOUNTAIN_VILLAGE, InteriorKind::Passage },
        { "mm:interior:fairy_fountain:2", ENTRANCE(FAIRY_FOUNTAIN, 2), ENTRANCE(SNOWHEAD, 2),
          RR_SNOWHEAD_GREAT_FAIRY_FOUNTAIN, RR_SNOWHEAD_OUTSIDE_FOUNTAIN, InteriorKind::Passage },
    } };
    return interiors;
}
inline const std::array<PortalDescriptor, 7>& ExpandedDungeons() {
    static const std::array<PortalDescriptor, 7> expanded = { {
        { "mm:swamp-spider-house", ENTRANCE(SWAMP_SPIDER_HOUSE, 0), ENTRANCE(SOUTHERN_SWAMP_POISONED, 8),
          RR_SWAMP_SPIDER_HOUSE_ENTRANCE, RR_SOUTHERN_SWAMP_SOUTH },
        { "mm:ocean-spider-house", ENTRANCE(OCEANSIDE_SPIDER_HOUSE, 0), ENTRANCE(GREAT_BAY_COAST, 8),
          RR_OCEAN_SPIDER_HOUSE_ENTRANCE_UPPER, RR_GREAT_BAY_COAST },
        { "mm:ikana-castle", ENTRANCE(IKANA_CASTLE, 1), ENTRANCE(IKANA_CANYON, 8), RR_IKANA_CASTLE_FRONT_ENTRANCE,
          RR_IKANA_CANYON_UPPER },
        { "mm:secret-shrine", ENTRANCE(SECRET_SHRINE, 0), ENTRANCE(IKANA_CANYON, 12), RR_SECRET_SHRINE_ENTRANCE,
          RR_IKANA_CANYON_LOWER },
        { "mm:beneath-well-front", ENTRANCE(BENEATH_THE_WELL, 0), ENTRANCE(IKANA_CANYON, 5),
          RR_BENEATH_THE_WELL_ENTRANCE, RR_IKANA_CANYON_UPPER },
        { "mm:beneath-well-back", ENTRANCE(BENEATH_THE_WELL, 1), ENTRANCE(IKANA_CASTLE, 0),
          RR_BENEATH_THE_WELL_BACK_EXIT, RR_IKANA_CASTLE_COURTYARD },
        { "mm:pirates-fortress", ENTRANCE(PIRATES_FORTRESS_EXTERIOR, 0), ENTRANCE(GREAT_BAY_COAST, 5),
          RR_PIRATES_FORTRESS_MOAT_LOWER, RR_GREAT_BAY_COAST },
    } };
    return expanded;
}
inline void Append(std::vector<Dungeon>& out, const PoolOptions& options) {
    auto appendPortal = [&](const PortalDescriptor& portal, Pool pool, bool expanded) {
        Dungeon dungeon;
        dungeon.id = portal.id;
        dungeon.entrance = { 1, portal.entrance };
        dungeon.exit = dungeon.returnTo = { 1, portal.exit };
        dungeon.pool = pool;
        dungeon.expanded = expanded;
        out.push_back(std::move(dungeon));
    };
    if (options.dungeons && options.expanded)
        for (const auto& portal : ExpandedDungeons())
            appendPortal(portal, Pool::Dungeon, true);
    for (const auto& portal : Interiors())
        if (const auto pool = InteriorPool(portal.kind, options))
            appendPortal(portal, *pool, false);
    if (options.grottos)
        for (const auto& grotto : Grottos()) {
            Dungeon dungeon;
            dungeon.id = "mm:grotto:" + std::to_string(grotto.logicalSpawn);
            dungeon.entrance = { 1, grotto.Entry() };
            dungeon.exit = dungeon.returnTo = { 1, grotto.Exit() };
            dungeon.pool = Pool::Grotto;
            out.push_back(std::move(dungeon));
        }
}
} // namespace ComboCrossMmCatalogue
