#include "ItemReceiptText.h"
#include "../../../combo/ComboGameMode.h"
#include "2s2h/Rando/StaticData/StaticData.h"
#include "2s2h/FleetShipCombo/FleetComboItemsGlue.h"
#include "2s2h/FleetShipCombo/FleetComboItems.h"
#include "ComboItemReceiptText.h"
#include "ComboDungeonKeyReceipt.h"
#include "ComboBossSoulReceiptText.h"
#include "ComboCapeReceiptChoice.h"
#include "ComboVanillaItemReceiptText.h"
#include "ComboSongReceiptText.h"
#include "ComboMagicItemReceiptText.h"
#include "ComboToolReceiptText.h"
#include "ComboMaskReceiptText.h"
#include "ComboSongDrawMM.h"
#include "ComboRupeeNames.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <iterator>
#include <random>
#ifdef COMBO_BUILD
#include "ComboExport.h"
#include "ComboResolve.h"
#include "rando/CrossForeign.h"
#include "rando/CrossDungeonSeed.h"
#include <unordered_map>
namespace Rando::MiscBehavior {
uint64_t ComboRandoGen();
const ComboRando::ForeignItem* MM_LookupForeign(RandoCheckId check);
} // namespace Rando::MiscBehavior
#endif

extern "C" {
#include "mods/extended_inventory.h"
#include "message_data_static.h"
extern float sNESFontWidths[160];
u16 Player_GetItemReceiptTextId(s16 getItemId, s16 itemId);
}

namespace {
struct DungeonInformation {
    const char* temple;
    const char* boss;
    const char* entrance;
    RandoCheckId reward;
};

// Native identities remain the fallback for solo and legacy seeds. Combined
// hints resolve saved cross routes before choosing a physical boss reward.
constexpr DungeonInformation kDungeonInformation[] = {
    { "Woodfall Temple", "Odolwa", "Woodfall", RC_WOODFALL_TEMPLE_BOSS_WARP },
    { "Snowhead Temple", "Goht", "Snowhead", RC_SNOWHEAD_TEMPLE_BOSS_WARP },
    { "Great Bay Temple", "Gyorg", "Zora Cape's turtle", RC_GREAT_BAY_TEMPLE_BOSS_WARP },
    { "Stone Tower Temple", "Twinmold", "Stone Tower", RC_STONE_TOWER_TEMPLE_INVERTED_BOSS_WARP },
};

#ifdef COMBO_BUILD
std::string ForeignRewardName(const ComboRando::ForeignItem& item) {
    return ComboRando::StripGameSuffix(item.itemName) + (item.shared                             ? ""
                                                         : item.itemGame == ComboRando::GAME_OOT ? " (OOT)"
                                                                                                 : " (MM)");
}

struct RewardIdentity {
    std::string name;
    int game = ComboRando::GAME_MM;
    bool imported = false;
    bool shared = false;
};

RewardIdentity LoadedSeedDungeonReward(int32_t dungeon) {
    static uint64_t generation = static_cast<uint64_t>(-1);
    static int slot = -1;
    static std::array<RewardIdentity, 4> names;
    const uint64_t current = Rando::MiscBehavior::ComboRandoGen();
    if (generation != current || slot != gSaveContext.fileNum) {
        generation = current;
        slot = gSaveContext.fileNum;
        names = {};
        try {
            const auto seed = nlohmann::json::parse(ComboRando::g_comboForeignJson);
            const auto placements =
                seed.value("mm", nlohmann::json::object()).value("placements", nlohmann::json::object());
            const auto foreign = ComboRando::LoadForeignForGame(0, ComboRando::GAME_MM);
            for (int i = 0; i < 4; ++i) {
                const auto& checkName = Rando::StaticData::GetCheckDisplayName(kDungeonInformation[i].reward);
                const auto placement = placements.find(checkName);
                if (placement == placements.end() || !placement->is_string())
                    continue;
                const auto imported = foreign.find(checkName);
                if (imported != foreign.end()) {
                    names[i] = { ComboRando::StripGameSuffix(imported->second.itemName), imported->second.itemGame,
                                 true, imported->second.shared };
                } else {
                    names[i].name = placement->get<std::string>();
                }
                if (names[i].name == ComboRando::kForeignSentinelNameMM)
                    names[i] = {};
            }
        } catch (...) { names = {}; }
    }
    return names[dungeon];
}
#endif

std::string DungeonRewardName(int32_t dungeon) {
    if (dungeon < 0 || dungeon >= 4)
        return {};
#ifdef COMBO_BUILD
    // The launcher pushes the selected seed before MM gameplay/save hydration.
    // Prefer it over dormant save data, which may still belong to a prior slot.
    if (Combo_IsCombinedGame() && !ComboRando::g_comboForeignJson.empty()) {
        const auto reward = LoadedSeedDungeonReward(dungeon);
        return reward.name.empty() ? std::string{}
                                   : reward.name + (reward.imported && !reward.shared
                                                        ? (reward.game == ComboRando::GAME_OOT ? " (OOT)" : " (MM)")
                                                        : "");
    }
#endif
    if (gSaveContext.fileNum == 0xFF || gSaveContext.save.shipSaveInfo.saveType != SAVETYPE_RANDO)
        return {};
    const auto check = kDungeonInformation[dungeon].reward;
    const auto id = gSaveContext.save.shipSaveInfo.rando.randoSaveChecks[check].randoItemId;
#ifdef COMBO_BUILD
    if (id == RI_COMBO_FOREIGN) {
        const auto* foreign = Rando::MiscBehavior::MM_LookupForeign(check);
        if (!foreign || foreign->itemName.empty())
            return {};
        return ForeignRewardName(*foreign);
    }
#endif
    const auto item = Rando::StaticData::Items.find(id);
    return id != RI_UNKNOWN && item != Rando::StaticData::Items.end() && item->second.name ? item->second.name
                                                                                           : std::string{};
}

#ifdef COMBO_BUILD
struct SavedInformationRoutes {
    bool valid = true;
    std::vector<ComboRando::CrossDungeon::Dungeon> dungeons;
    std::vector<ComboRando::CrossDungeon::Route> routes;
};

const SavedInformationRoutes& InformationRoutes() {
    static bool initialized = false;
    static std::string previous;
    static SavedInformationRoutes cached;
    const auto& raw = ComboRando::g_comboForeignJson;
    if (!initialized || previous != raw) {
        initialized = true;
        previous = raw;
        cached = {};
        try {
            if (!raw.empty()) {
                const auto seed = nlohmann::json::parse(raw);
                const auto cross = seed.value("crossDungeonEntrances", nlohmann::json{});
                cached.routes = ComboRando::CrossDungeon::Decode(cross);
                if (!cached.routes.empty())
                    for (const auto& entry : cross.at("dungeons"))
                        cached.dungeons.push_back(ComboRando::CrossDungeon::ParseDungeon(entry));
            }
        } catch (...) {
            cached = {};
            cached.valid = false;
        }
    }
    return cached;
}

std::string InformationEntranceSource(const ComboRando::CrossDungeon::Dungeon& source) {
    if (source.entrance.game == ComboRando::GAME_OOT) {
        using GetSource = int32_t (*)(int32_t, char*, uint32_t);
        const auto getSource = reinterpret_cast<GetSource>(Combo_ResolveSym("soh", "OOT_GetDungeonEntranceSourceName"));
        char name[512]{};
        const int32_t size = getSource ? getSource(source.entrance.index, name, sizeof(name)) : 0;
        return size > 0 && static_cast<size_t>(size) < sizeof(name) ? std::string(name, size) : std::string{};
    }
    static const std::map<std::string, const char*> names = {
        { "mm:woodfall", "Woodfall" },
        { "mm:snowhead", "Snowhead" },
        { "mm:greatbay", "Zora Cape's turtle" },
        { "mm:stonetower", "Stone Tower (normal)" },
        { "mm:inverted", "Stone Tower (inverted)" },
        { "mm:swamp-spider-house", "Southern Swamp's Spider House" },
        { "mm:ocean-spider-house", "Great Bay's Spider House" },
        { "mm:ikana-castle", "Ikana Castle" },
        { "mm:secret-shrine", "Ikana Canyon's Secret Shrine" },
        { "mm:beneath-well-front", "Beneath the Well's front entrance" },
        { "mm:beneath-well-back", "Beneath the Well's castle entrance" },
        { "mm:pirates-fortress", "Great Bay's Pirates' Fortress" }
    };
    const auto found = names.find(source.id);
    return found == names.end() ? std::string{} : found->second;
}

// nullopt means this endpoint was excluded from cross shuffle. An empty name
// means the saved route is unusable, so callers must not invent a native hint.
std::optional<std::string> CrossInformationEntranceSource(ComboRando::CrossDungeon::Endpoint entrance) {
    if (!Combo_IsCombinedGame())
        return std::nullopt;
    using namespace ComboRando::CrossDungeon;
    const auto& saved = InformationRoutes();
    if (!saved.valid)
        return std::string{};
    for (const auto& target : saved.dungeons) {
        if (target.entrance != entrance)
            continue;
        for (const auto& route : saved.routes) {
            if (route.phase != Phase::AfterNative || route.to != entrance)
                continue;
            for (const auto& source : saved.dungeons)
                if (source.entrance == route.from)
                    return InformationEntranceSource(source);
        }
        return std::string{};
    }
    return std::nullopt;
}

int InformationBossDestination(int boss) {
    if (boss < 0 || boss >= 12)
        return -1;
    if (!Combo_IsCombinedGame())
        return boss;
    using namespace ComboRando::CrossDungeon;
    const auto& saved = InformationRoutes();
    if (!saved.valid)
        return -1;
    using NativeTarget = int32_t (*)(int32_t, int32_t*, int32_t*);
    using DungeonBoss = int32_t (*)(int32_t);
    const auto nativeTarget = reinterpret_cast<NativeTarget>(Combo_ResolveSym("soh", "OOT_GetNativeBossHintTarget"));
    const auto dungeonBoss = reinterpret_cast<DungeonBoss>(Combo_ResolveSym("soh", "OOT_GetDungeonBossIndex"));
    unsigned visited = 0;
    while (boss >= 0 && boss < 12 && !(visited & (1u << boss))) {
        visited |= 1u << boss;
        Endpoint entrance{ boss < 8 ? ComboRando::GAME_OOT : ComboRando::GAME_MM, -1 };
        int32_t next = -1;
        int physical = boss;
        if (boss < 8) {
            if (!nativeTarget)
                return -1;
            physical = nativeTarget(boss, &entrance.index, &next);
        } else {
            for (const auto& source : saved.dungeons)
                if (source.pool == Pool::Boss && source.boss == boss)
                    entrance = source.entrance;
        }
        const Dungeon* target = nullptr;
        for (const auto& route : saved.routes) {
            if (route.phase != Phase::AfterNative || route.from != entrance)
                continue;
            for (const auto& candidate : saved.dungeons)
                if (candidate.entrance == route.to)
                    target = &candidate;
            if (!target)
                return -1;
        }
        if (!target) {
            if (physical >= 0)
                return physical;
            boss = next;
            continue;
        }
        if (target->pool == Pool::Boss)
            return target->boss;
        // Mixed pools can land in another dungeon. Follow its boss ingress,
        // applying native then cross exactly once at every transition.
        if (target->pool != Pool::Dungeon)
            return -1;
        if (target->entrance.game == ComboRando::GAME_OOT) {
            boss = dungeonBoss ? dungeonBoss(target->entrance.index) : -1;
        } else {
            const char* owners[] = { "mm:woodfall", "mm:snowhead", "mm:greatbay", "mm:inverted" };
            boss = -1;
            for (int i = 0; i < 4; ++i)
                if (target->id == owners[i])
                    boss = i + 8;
        }
    }
    return -1;
}
#endif

std::string InformationEntranceName(int32_t dungeon) {
    if (dungeon < 0 || dungeon >= 4)
        return {};
#ifdef COMBO_BUILD
    if (Combo_IsCombinedGame()) {
        using namespace ComboRando::CrossDungeon;
        const auto& saved = InformationRoutes();
        if (!saved.valid)
            return {};
        const char* ids[] = { "mm:woodfall", "mm:snowhead", "mm:greatbay", "mm:stonetower" };
        std::vector<std::string> locations;
        bool shuffled = false;
        for (int orientation = 0; orientation < (dungeon == 3 ? 2 : 1); ++orientation) {
            const std::string id = orientation ? "mm:inverted" : ids[dungeon];
            std::string location = dungeon == 3 ? (orientation ? "Stone Tower (inverted)" : "Stone Tower (normal)")
                                                : kDungeonInformation[dungeon].entrance;
            for (const auto& target : saved.dungeons)
                if (target.pool == Pool::Dungeon && target.id == id) {
                    shuffled = true;
                    location = CrossInformationEntranceSource(target.entrance).value_or("");
                }
            if (location.empty())
                return {};
            locations.push_back(std::move(location));
        }
        if (shuffled) {
            if (dungeon == 3)
                return "normal: " + locations[0] + "; inverted: " + locations[1];
            return locations[0];
        }
    }
#endif
    return kDungeonInformation[dungeon].entrance;
}

int InformationBoss(int32_t dungeon) {
    if (dungeon < 0 || dungeon >= 4)
        return -1;
#ifdef COMBO_BUILD
    return InformationBossDestination(dungeon + 8);
#else
    return dungeon + 8;
#endif
}

std::string InformationBossName(int32_t dungeon) {
    constexpr const char* names[] = {
        "Queen Gohma", "King Dodongo", "Barinade", "Phantom Ganon", "Volvagia", "Morpha",
        "Twinrova",    "Bongo Bongo",  "Odolwa",   "Goht",          "Gyorg",    "Twinmold"
    };
    const int boss = InformationBoss(dungeon);
    return boss >= 0 && boss < 12 ? names[boss] : std::string{};
}

std::string InformationRewardName(int32_t dungeon) {
    const int boss = InformationBoss(dungeon);
    if (boss >= 8)
        return DungeonRewardName(boss - 8);
#ifdef COMBO_BUILD
    if (boss >= 0) {
        using GetReward = int32_t (*)(int32_t, char*, uint32_t);
        const auto getReward = reinterpret_cast<GetReward>(Combo_ResolveSym("soh", "OOT_GetPhysicalBossRewardName"));
        char name[512]{};
        const int32_t size = getReward ? getReward(boss, name, sizeof(name)) : 0;
        if (size > 0 && static_cast<size_t>(size) < sizeof(name))
            return std::string(name, size);
    }
#endif
    return {};
}

int32_t CopyInformationText(const std::string& text, char* buffer, uint32_t capacity) {
    if (!buffer || !capacity)
        return 0;
    buffer[0] = '\0';
    if (text.empty() || text.size() >= capacity)
        return 0;
    std::memcpy(buffer, text.c_str(), text.size() + 1);
    return static_cast<int32_t>(text.size());
}

int MapCompassDungeon(RandoItemId id, bool& compass) {
    compass = true;
    switch (id) {
        case RI_WOODFALL_COMPASS:
            return 0;
        case RI_SNOWHEAD_COMPASS:
            return 1;
        case RI_GREAT_BAY_COMPASS:
            return 2;
        case RI_STONE_TOWER_COMPASS:
            return 3;
        default:
            break;
    }
    compass = false;
    switch (id) {
        case RI_WOODFALL_MAP:
            return 0;
        case RI_SNOWHEAD_MAP:
            return 1;
        case RI_GREAT_BAY_MAP:
            return 2;
        case RI_STONE_TOWER_MAP:
            return 3;
        default:
            return -1;
    }
}

// ConvertItem already resolved these identities using MM's save. Never ask the
// donor to choose a tier again: its state may differ or already include this grant.
const char* ConcreteReceiptName(RandoItemId id) {
    switch (id) {
        case RI_SINGLE_MAGIC:
            return "Magic Meter";
        case RI_DOUBLE_MAGIC:
            return "Enhanced Magic Meter";
        // These FC chains count identical keys, rather than equipment tiers.
        // Each local key already has the concrete identity the donor needs.
        case RI_OOT_SMALL_KEY_BOTTOM_OF_THE_WELL:
            return "Bottom of the Well Small Key";
        case RI_OOT_SMALL_KEY_FIRE_TEMPLE:
            return "Fire Temple Small Key";
        case RI_OOT_SMALL_KEY_FOREST_TEMPLE:
            return "Forest Temple Small Key";
        case RI_OOT_SMALL_KEY_GANONS_CASTLE:
            return "Ganon's Castle Small Key";
        case RI_OOT_SMALL_KEY_GERUDO_FORTRESS:
            return "Gerudo Fortress Small Key";
        case RI_OOT_SMALL_KEY_GERUDO_TRAINING_GROUND:
            return "Training Ground Small Key";
        case RI_OOT_SMALL_KEY_SHADOW_TEMPLE:
            return "Shadow Temple Small Key";
        case RI_OOT_SMALL_KEY_SPIRIT_TEMPLE:
            return "Spirit Temple Small Key";
        case RI_OOT_SMALL_KEY_TREASURE_GAME:
            return "Chest Game Small Key";
        case RI_OOT_SMALL_KEY_WATER_TEMPLE:
            return "Water Temple Small Key";
        // Shared songs still need MM's description of their use in Termina.
        // These are concrete identities, including each Goron Lullaby tier.
        case RI_SONG_SONATA:
            return "Sonata of Awakening";
        case RI_SONG_LULLABY:
            return "Goron Lullaby";
        case RI_SONG_LULLABY_INTRO:
            return "Goron Lullaby Intro";
        case RI_SONG_NOVA:
            return "New Wave Bossa Nova";
        case RI_SONG_ELEGY:
            return "Elegy of Emptiness";
        case RI_SONG_OATH:
            return "Oath to Order";
        case RI_SONG_HEALING:
            return "Song of Healing";
        case RI_SONG_SOARING:
            return "Song of Soaring";
        case RI_SONG_TIME:
            return "Song of Time (MM)";
        case RI_SONG_STORMS:
            return "Song of Storms (MM)";
        case RI_SONG_SUN:
            return "Sun's Song (MM)";
        case RI_SONG_EPONA:
            return "Epona's Song (MM)";
        case RI_SONG_SARIA:
            return "Saria's Song (MM)";
        case RI_SONG_DOUBLE_TIME:
            return "Song of Double Time";
        case RI_SONG_INVERTED_TIME:
            return "Inverted Song of Time";
        case RI_CLAWSHOT:
            return "Clawshot";
        case RI_HOOKSHOT: {
            int level = Nei_Save()->ootHookshotLevel;
            if (!level && INV_CONTENT(ITEM_HOOKSHOT) == ITEM_HOOKSHOT)
                level = 1;
            return level == 0 ? "Hookshot" : level == 1 ? "Longshot" : "Ultrashot";
        }
        case RI_FAIRY_SLINGSHOT:
            if (!Nei_Save()->slingshotOwned || Nei_BulletBagLevel() == 0)
                return "Fairy Slingshot";
            return Nei_BulletBagLevel() < 2 ? "Big Deku Seed Bullet Bag" : "Biggest Deku Seed Bullet Bag";
        case RI_OOT_PROGRESSIVE_STICK_CAPACITY: {
            const int level = CUR_UPG_VALUE(UPG_DEKU_STICKS);
            return level == 0 ? "Deku Stick Bag" : level == 1 ? "Deku Stick Capacity (20)" : "Deku Stick Capacity (30)";
        }
        case RI_OOT_PROGRESSIVE_NUT_CAPACITY: {
            const int level = CUR_UPG_VALUE(UPG_DEKU_NUTS);
            return level == 0 ? "Deku Nut Bag" : level == 1 ? "Deku Nut Capacity (30)" : "Deku Nut Capacity (40)";
        }
        case RI_OOT_HAMMER:
            return "Megaton Hammer";
        case RI_OOT_IRON_KNUCKLE_AXE:
            return "Iron Knuckle's Axe";
        case RI_OOT_MASTER_SWORD:
            return "Master Sword";
        case RI_OOT_TRUE_MASTER_SWORD:
            return "True Master Sword";
        case RI_OOT_BIGGORON_SWORD:
            return "Biggoron's Sword";
        case RI_OOT_STONE_OF_AGONY:
            return Nei_Save()->ootQuestItems & (1u << OOT_QUEST_STONE_OF_AGONY) ? "Quartz of Motion" : "Stone of Agony";
        case RI_OOT_QUARTZ_OF_MOTION:
            return "Quartz of Motion";
        case RI_OOT_GORONS_BRACELET:
            return "Goron's Bracelet";
        case RI_OOT_SILVER_GAUNTLETS:
            return "Silver Gauntlets";
        case RI_OOT_GOLDEN_GAUNTLETS:
            return "Golden Gauntlets";
        case RI_OOT_NEI_ROCS_FEATHER:
            return "Progressive Roc"; // donor's Skijer feather row
        case RI_OOT_NEI_ROCS_CAPE:
            return "Roc's Cape";
        case RI_OOT_NEI_CANE_PACCI_FLIP:
            return "Cane of Pacci";
        case RI_OOT_NEI_CANE_PACCI_STONE:
            return "Pacci Stone Skill";
        case RI_OOT_NEI_CANE_PACCI_ULTRAHAND:
            return "Ultrahand";
        case RI_OOT_NEI_CANE_SOMARIA_BLOCK:
            return "Somaria Block Skill";
        case RI_OOT_NEI_CANE_SOMARIA_PLATFORM:
            return "Somaria Platform Skill";
        default:
            return nullptr;
    }
}

void SetReceiptBody(CustomMessage::Entry& entry, std::string body) {
    if (entry.receiptPresentation.singleBox) {
        entry.icon = 0xFE; // The reward is a final-line sprite, not a header icon.
    } else {
        ComboItemReceiptText::Wrap(body, sNESFontWidths, 160, entry.icon == 0xFE ? 300.0f : 240.0f);
    }
    entry.msg = std::move(body);
    entry.autoFormat = false;
}

bool RandomRupeeReceipt(const char* name, CustomMessage::Entry& entry) {
    if (!name || gSaveContext.fileNum == 0xFF || gSaveContext.save.shipSaveInfo.saveType != SAVETYPE_RANDO ||
        !CVarGetInteger("gRandoEnhancements.RandomizeRupeeNames", 1))
        return false;
    struct Currency {
        const char* name;
        const char* amount;
        const char* color;
    };
    static constexpr Currency currencies[] = {
        { "Green Rupee", "1", "%g" },  { "Blue Rupee", "5", "%b" },    { "10 Rupees", "10", "%b" },
        { "Red Rupee", "20", "%r" },   { "Purple Rupee", "50", "%p" }, { "Silver Rupee", "100", "%w" },
        { "Huge Rupee", "200", "%y" },
    };
    for (const auto& currency : currencies) {
        if (std::strcmp(name, currency.name))
            continue;
        const char* const* names = ComboRupeeNames::englishRupeeNames;
        size_t count = std::size(ComboRupeeNames::englishRupeeNames);
        std::string prefix = "You found ";
        std::string suffix = "%w!";
        if (gSaveContext.options.language == LANGUAGE_GER) {
            names = ComboRupeeNames::germanRupeeNames;
            count = std::size(ComboRupeeNames::germanRupeeNames);
            prefix = "Du hast ";
            suffix = "%w gefunden!";
        } else if (gSaveContext.options.language == LANGUAGE_FRE) {
            names = ComboRupeeNames::frenchRupeeNames;
            count = std::size(ComboRupeeNames::frenchRupeeNames);
            prefix = "Vous obtenez ";
        }
        // Cosmetic randomness stays independent of the gameplay/seed generators.
        static std::mt19937 random(std::random_device{}());
        const size_t choice = std::uniform_int_distribution<size_t>(0, count - 1)(random);
        std::string nickname = names[choice];
        if (nickname == "[P]")
            nickname.assign(1, '\x16'); // MM's player-name command.
        entry.receiptPresentation = {};
        SetReceiptBody(entry, ComboItemReceiptText::FromNeiMarkup(prefix + currency.color + currency.amount + " " +
                                                                  nickname + suffix));
        return true;
    }
    return false;
}

bool NativeReceipt(GetItemId gi, ItemId itemId, CustomMessage::Entry& entry, const char* shieldName = nullptr) {
    if (!gPlayState || !gPlayState->msgCtx.messageTableNES)
        return false;
    const u16 textId = Player_GetItemReceiptTextId(gi, itemId);
    if (!textId)
        return false;
    for (const auto* text = gPlayState->msgCtx.messageTableNES; text->textId != 0xFFFF; ++text) {
        if (text->textId != textId)
            continue;
        if (!text->segment || text->msgSize <= MESSAGE_HEADER_SIZE)
            return false;
        std::string body;
        if (!ComboItemReceiptText::FromMMMessage(
                std::string_view(text->segment + MESSAGE_HEADER_SIZE, text->msgSize - MESSAGE_HEADER_SIZE), body)) {
            return false;
        }
        if (shieldName) {
            const auto name = body.find("Mirror Shield");
            if (name != std::string::npos)
                body.replace(name, std::strlen("Mirror Shield"), shieldName);
        }
        SetReceiptBody(entry, std::move(body));
        return true;
    }
    return false;
}
} // namespace

static bool BossSoulItemReceipt(const char* itemName, CustomMessage::Entry& entry) {
    const auto* soul = itemName ? ComboBossSoulReceiptText::Find(itemName) : nullptr;
    if (!soul)
        return false;
    using ComboBossSoulReceiptText::Language;
    const auto language = gSaveContext.options.language == LANGUAGE_GER   ? Language::German
                          : gSaveContext.options.language == LANGUAGE_FRE ? Language::French
                                                                          : Language::English;
    entry.receiptPresentation = {};
    SetReceiptBody(entry, ComboItemReceiptText::FromNeiMarkup(ComboBossSoulReceiptText::Body(*soul, language)));
    return true;
}

static bool MaskItemReceipt(const char* itemName, CustomMessage::Entry& entry) {
    if (!itemName)
        return false;
    const auto* mask = ComboMaskReceiptText::Find(itemName);
    if (!mask)
        return false;
    const char* body = gSaveContext.options.language == LANGUAGE_GER   ? mask->german
                       : gSaveContext.options.language == LANGUAGE_FRE ? mask->french
                                                                       : mask->english;
    entry.receiptPresentation = {};
    SetReceiptBody(entry, ComboItemReceiptText::FromNeiMarkup(body));
    return true;
}

static bool VanillaItemFallback(const char* itemName, CustomMessage::Entry& entry) {
    const auto* receipt = itemName ? ComboVanillaItemReceiptText::Find(itemName) : nullptr;
    if (!receipt)
        return false;
    entry.receiptPresentation = {};
    using ComboVanillaItemReceiptText::Language;
    const auto language = gSaveContext.options.language == LANGUAGE_GER   ? Language::German
                          : gSaveContext.options.language == LANGUAGE_FRE ? Language::French
                                                                          : Language::English;
    SetReceiptBody(entry, ComboItemReceiptText::FromNeiMarkup(ComboVanillaItemReceiptText::Body(*receipt, language)));
    return true;
}

static bool ToolItemReceipt(const char* itemName, CustomMessage::Entry& entry) {
    if (!itemName)
        return false;
    const auto* tool = ComboToolReceiptText::Find(itemName);
    if (!tool)
        return false;
    const char* body = gSaveContext.options.language == LANGUAGE_GER   ? tool->german
                       : gSaveContext.options.language == LANGUAGE_FRE ? tool->french
                                                                       : tool->english;
    entry.receiptPresentation = {};
    SetReceiptBody(entry, ComboItemReceiptText::FromNeiMarkup(body));
    return true;
}

static bool MagicItemReceipt(const char* itemName, CustomMessage::Entry& entry) {
    if (!itemName)
        return false;
    const auto* magic = ComboMagicItemReceiptText::Find(itemName, Wand_RandoMode());
    const auto* medallion = CVarGetInteger("gRando.Options.RO_SHUFFLE_NEI_ITEMS", 0)
                                ? ComboMagicItemReceiptText::FindMedallion(itemName, Wand_RandoMode())
                                : nullptr;
    if (!magic && !medallion)
        return false;
    using ComboMagicItemReceiptText::Language;
    const auto language = gSaveContext.options.language == LANGUAGE_GER   ? Language::German
                          : gSaveContext.options.language == LANGUAGE_FRE ? Language::French
                                                                          : Language::English;
    entry.receiptPresentation = {};
    const auto body = medallion ? ComboMagicItemReceiptText::MedallionReceipt(*medallion, language)
                                : ComboMagicItemReceiptText::Body(*magic, language);
    SetReceiptBody(entry, ComboItemReceiptText::FromNeiMarkup(body));
    return true;
}

bool Rando::ApplyNativeRandomRupeeReceipt(uint16_t textId, CustomMessage::Entry& entry) {
    const char* name;
    switch (textId) {
        case 0xC4:
            name = "Green Rupee";
            break;
        case 0x2:
            name = "Blue Rupee";
            break;
        case 0x3:
            name = "10 Rupees";
            break;
        case 0x4:
            name = "Red Rupee";
            break;
        case 0x5:
            name = "Purple Rupee";
            break;
        case 0x6:
            name = "Silver Rupee";
            break;
        case 0x7:
            name = "Huge Rupee";
            break;
        default:
            return false;
    }
    if (!RandomRupeeReceipt(name, entry))
        return false;
    entry.msg += '\xBF'; // Direct native boxes do not use the queued finalizer.
    return true;
}

bool Rando::MapCompassInfoEnabled() {
    if (gSaveContext.fileNum == 0xFF || gSaveContext.save.shipSaveInfo.saveType != SAVETYPE_RANDO)
        return false;
#ifdef COMBO_BUILD
    if (Combo_IsCombinedGame()) {
        using GetEnabled = int32_t (*)();
        static GetEnabled getEnabled = nullptr;
        if (!getEnabled)
            getEnabled = reinterpret_cast<GetEnabled>(Combo_ResolveSym("soh", "OOT_MapCompassInfoEnabled"));
        return getEnabled && getEnabled() != 0;
    }
#endif
    return gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_MAPS_COMPASSES_GIVE_INFORMATION] != 0;
}

std::string Rando::GetDungeonMapCompassInfo(int32_t dungeon, bool compass) {
    if (dungeon < 0 || dungeon >= 4 || !MapCompassInfoEnabled())
        return {};
    const auto& info = kDungeonInformation[dungeon];
    if (!compass) {
        const auto entrance = InformationEntranceName(dungeon);
        return entrance.empty() ? std::string{}
                                : std::string("The entrance to ") + info.temple + " is at " + entrance + ".";
    }
    const std::string boss = InformationBossName(dungeon);
    const std::string reward = InformationRewardName(dungeon);
    if (boss.empty() || reward.empty())
        return {};
    return std::string("The boss of ") + info.temple + " is " + boss + ". Defeat " + boss + " for " + reward + ".";
}

#ifdef COMBO_BUILD
extern "C" COMBO_EXPORT int32_t MM_GetSeedItemIconInfo(const char* name, CwItemIconInfo* out) {
    try {
        if (!name || !out)
            return 0;
        *out = CwItemIconInfo{};
        const std::string key = ComboRando::StripGameSuffix(name);
        for (const auto& [id, item] : Rando::StaticData::Items) {
            if (id == RI_UNKNOWN || id == RI_COMBO_FOREIGN || !item.name || key != item.name)
                continue;
            out->path = Rando::StaticData::GetIconTexturePath(id);
            if (!out->path || std::strncmp(out->path, "__OTR__", 7))
                return 0;
            out->width = out->height = std::strstr(out->path, "icon_item_24_static") ? 24 : 32;
            const int song =
                id == RI_PROGRESSIVE_LULLABY ? ComboSongForMmItem(RI_SONG_LULLABY) : ComboSongForMmItem(id);
            if (song >= 0 || std::strstr(out->path, "SongNote")) {
                out->width = 16;
                out->height = 24;
                out->isIA8 = 1;
                out->hasColor = ComboSongShimmerColor(song, out->color);
            } else if (std::strstr(out->path, "gOcarinaBtnIcon")) {
                out->width = out->height = 16;
                out->isIA8 = 1;
            } else if (std::strstr(out->path, "gHeartPieceIcon")) {
                out->width = out->height = 48;
                out->isIA8 = 1;
            }
            return 1;
        }
        return 0;
    } catch (...) { return 0; }
}

extern "C" COMBO_EXPORT int32_t MM_GetPhysicalBossRewardIconInfo(int32_t boss, CwItemIconInfo* out) {
    if (!out)
        return 0;
    *out = {};
    if (boss < 8 || boss >= 12)
        return 0;
    const int dungeon = boss - 8;
    try {
        RewardIdentity reward;
        if (Combo_IsCombinedGame() && !ComboRando::g_comboForeignJson.empty()) {
            reward = LoadedSeedDungeonReward(dungeon);
        } else {
            if (gSaveContext.fileNum == 0xFF || gSaveContext.save.shipSaveInfo.saveType != SAVETYPE_RANDO)
                return 0;
            const auto check = kDungeonInformation[dungeon].reward;
            const auto id = gSaveContext.save.shipSaveInfo.rando.randoSaveChecks[check].randoItemId;
            if (id == RI_COMBO_FOREIGN) {
                const auto* foreign = Rando::MiscBehavior::MM_LookupForeign(check);
                if (foreign)
                    reward = { ComboRando::StripGameSuffix(foreign->itemName), foreign->itemGame, true,
                               foreign->shared };
            } else {
                const auto item = Rando::StaticData::Items.find(id);
                if (id != RI_UNKNOWN && item != Rando::StaticData::Items.end() && item->second.name)
                    reward.name = item->second.name;
            }
        }
        if (reward.name.empty())
            return 0;
        if (reward.game == ComboRando::GAME_OOT) {
            auto getIcon = reinterpret_cast<Fn_GetItemIconInfo>(Combo_ResolveSym("soh", "OOT_GetSeedItemIconInfo"));
            if (!getIcon || getIcon(reward.name.c_str(), out) != 1)
                return 0;
            // Preserve the owner in the path when this result is consumed in OoT.
            static std::unordered_map<std::string, std::string> paths;
            CwItemReceiptPresentation p{};
            p.singleBox = 1;
            p.rewardLine = 2;
            if (!ComboReceipt_CopyIcon(&p, out, "oot"))
                return 0;
            out->path = paths.emplace(p.iconPath, p.iconPath).first->second.c_str();
            return 1;
        }
        return MM_GetSeedItemIconInfo(reward.name.c_str(), out);
    } catch (...) { return 0; }
}

extern "C" COMBO_EXPORT int32_t MM_GetDungeonRewardIconInfo(int32_t dungeon, CwItemIconInfo* out) {
    try {
        if (!out || dungeon < 0 || dungeon >= 4)
            return 0;
        *out = CwItemIconInfo{};
        const int boss = InformationBoss(dungeon);
        if (boss < 0)
            return 0;
        if (boss < 8) {
            using GetPresentation = int32_t (*)(int32_t, CwItemReceiptPresentation*);
            const auto getPresentation =
                reinterpret_cast<GetPresentation>(Combo_ResolveSym("soh", "OOT_GetPhysicalBossRewardPresentation"));
            CwItemReceiptPresentation presentation{};
            if (!getPresentation || getPresentation(boss, &presentation) != 1 || !ComboReceipt_HasIcon(&presentation))
                return 0;
            static std::unordered_map<std::string, std::string> paths;
            out->path = paths.emplace(presentation.iconPath, presentation.iconPath).first->second.c_str();
            out->width = presentation.iconWidth;
            out->height = presentation.iconHeight;
            out->isIA8 = presentation.iconIA8;
            out->hasColor = presentation.iconHasColor;
            std::memcpy(out->color, presentation.iconColor, sizeof(out->color));
            return 1;
        }
        return MM_GetPhysicalBossRewardIconInfo(boss, out);
    } catch (...) { return 0; }
}

extern "C" COMBO_EXPORT int32_t MM_GetDungeonRewardName(int32_t dungeon, char* buffer, uint32_t capacity) {
    if (!buffer || !capacity)
        return 0;
    buffer[0] = '\0';
    try {
        const std::string name = InformationRewardName(dungeon);
        if (name.empty() || name.size() >= capacity)
            return 0;
        std::memcpy(buffer, name.c_str(), name.size() + 1);
        return static_cast<int32_t>(name.size());
    } catch (...) { return 0; }
}

extern "C" COMBO_EXPORT int32_t MM_GetDungeonEntranceName(int32_t dungeon, char* buffer, uint32_t capacity) {
    try {
        return CopyInformationText(InformationEntranceName(dungeon), buffer, capacity);
    } catch (...) { return 0; }
}

extern "C" COMBO_EXPORT int32_t MM_GetDungeonBossName(int32_t dungeon, char* buffer, uint32_t capacity) {
    try {
        return CopyInformationText(InformationBossName(dungeon), buffer, capacity);
    } catch (...) { return 0; }
}

extern "C" COMBO_EXPORT int32_t MM_GetCrossDungeonEntranceSourceName(int32_t game, int32_t entrance, char* buffer,
                                                                     uint32_t capacity) {
    if (buffer && capacity)
        buffer[0] = '\0';
    if (!buffer || !capacity || game < 0 || game > 1 || entrance < 0 || entrance > 0xFFFF)
        return -1;
    try {
        const auto source = CrossInformationEntranceSource({ game, entrance });
        if (!source)
            return 0;
        const int32_t size = CopyInformationText(*source, buffer, capacity);
        return size ? size : -1;
    } catch (...) { return -1; }
}

extern "C" COMBO_EXPORT int32_t MM_GetInformationBossDestination(int32_t boss) {
    if (!Combo_IsCombinedGame())
        return -2; // The caller's native game owns solo entrance settings.
    try {
        return InformationBossDestination(boss);
    } catch (...) { return -1; }
}

extern "C" COMBO_EXPORT int32_t MM_GetPhysicalBossRewardName(int32_t boss, char* buffer, uint32_t capacity) {
    if (buffer && capacity)
        buffer[0] = '\0';
    if (boss < 8 || boss >= 12)
        return 0;
    try {
        return CopyInformationText(DungeonRewardName(boss - 8), buffer, capacity);
    } catch (...) { return 0; }
}

bool Rando::ApplyForeignItemReceiptText(const char* itemName, CustomMessage::Entry& entry, RandoCheckId check) {
    entry.capeVisibilityChoice = ComboCapeReceiptChoice::IsCape(itemName);
    if (!itemName || !*itemName)
        return false;
    if (BossSoulItemReceipt(itemName, entry) || MaskItemReceipt(itemName, entry) || ToolItemReceipt(itemName, entry) ||
        MagicItemReceipt(itemName, entry))
        return true;
    if (RandomRupeeReceipt(itemName, entry))
        return true;
    // The donor ABI exports English bytes only. Native MM bottles are handled
    // before this route; imported/cold-bank bottle lessons use MM's locale.
    if (gSaveContext.options.language != LANGUAGE_ENG) {
        const auto* receipt = ComboVanillaItemReceiptText::Find(itemName);
        if (receipt && receipt->kind >= ComboVanillaItemReceiptText::Kind::Fish)
            return VanillaItemFallback(itemName, entry);
    }
    const auto keyBody = ComboDungeonKeyReceipt::Markup(itemName);
    if (!keyBody.empty()) {
        entry.receiptPresentation = {};
        SetReceiptBody(entry, ComboItemReceiptText::FromNeiMarkup(keyBody));
        return true;
    }
    // Capture dynamic descriptions before the cross grant. Cycle recollection
    // must keep that receipt's counters as well as its resolved item identity.
    struct SavedReceipt {
        std::string body;
        CwItemReceiptPresentation presentation{};
    };
    static std::unordered_map<int, SavedReceipt> bodies;
    static int slot = -1;
    static uint64_t generation = static_cast<uint64_t>(-1);
    const uint64_t currentGeneration = MiscBehavior::ComboRandoGen();
    if (slot != gSaveContext.fileNum || generation != currentGeneration) {
        bodies.clear();
        slot = gSaveContext.fileNum;
        generation = currentGeneration;
    }
    if (check != RC_UNKNOWN) {
        const auto saved = bodies.find(check);
        if (saved != bodies.end()) {
            entry.receiptPresentation = saved->second.presentation;
            SetReceiptBody(entry, saved->second.body);
            return true;
        }
    }
    static Fn_OOT_GetItemReceiptText getText = nullptr;
    if (!getText)
        getText = reinterpret_cast<Fn_OOT_GetItemReceiptText>(Combo_ResolveSym("soh", "OOT_GetItemReceiptText"));
    if (!getText || !itemName || !*itemName)
        return VanillaItemFallback(itemName, entry);
    char buffer[BUFFER_SIZE - MESSAGE_HEADER_SIZE]{};
    const int32_t size = getText(itemName, buffer, sizeof(buffer));
    if (size <= 0 || static_cast<size_t>(size) > sizeof(buffer))
        return VanillaItemFallback(itemName, entry);
    std::string body(buffer, size); // explicit length retains color byte 0
    entry.receiptPresentation = {};
    static Fn_GetDungeonItemReceiptPresentation getPresentation = nullptr;
    if (!getPresentation)
        getPresentation = reinterpret_cast<Fn_GetDungeonItemReceiptPresentation>(
            Combo_ResolveSym("soh", "OOT_GetDungeonItemReceiptPresentation"));
    if (getPresentation && getPresentation(itemName, &entry.receiptPresentation) != 1)
        entry.receiptPresentation = {};
    if (check != RC_UNKNOWN)
        bodies[check] = { body, entry.receiptPresentation };
    SetReceiptBody(entry, std::move(body));
    return true;
}
#endif

bool Rando::ApplyItemReceiptText(RandoItemId id, CustomMessage::Entry& entry) {
    entry.capeVisibilityChoice = id == RI_OOT_EXT_MAGIC_CAPE;
    entry.receiptPresentation = {};
    if (id == RI_TRAP)
        return false;
    const auto it = StaticData::Items.find(id);
    if (it == StaticData::Items.end())
        return false;
    const auto& item = it->second;
    if (id == RI_OOT_NEI_MAGIC_MIRROR) {
        SetReceiptBody(
            entry,
            "You found the %gMagic Mirror%w!\nEquip it to a C Button to return\nto the Lost Woods from outdoors.");
        return true;
    }
    if (BossSoulItemReceipt(item.name, entry) || MaskItemReceipt(item.name, entry) ||
        ToolItemReceipt(item.name, entry) || MagicItemReceipt(item.name, entry))
        return true;
    if (RandomRupeeReceipt(item.name, entry))
        return true;
    const auto keyBody = item.name ? ComboDungeonKeyReceipt::Markup(item.name) : std::string{};
    if (!keyBody.empty()) {
        SetReceiptBody(entry, ComboItemReceiptText::FromNeiMarkup(keyBody));
        return true;
    }
    bool compass;
    const int dungeon = MapCompassDungeon(id, compass);
    if (dungeon >= 0) {
        if (MapCompassInfoEnabled()) {
            entry.receiptPresentation.singleBox = 1;
            entry.receiptPresentation.rewardLine = 2;
            std::string body = (compass ? "You received a %g" : "You found the %g") + std::string(item.name) + "%w!";
            if (compass) {
                const auto boss = InformationBossName(dungeon);
                if (!boss.empty())
                    body += "&It points to %r" + boss + "%w";
#ifdef COMBO_BUILD
                CwItemIconInfo icon{};
                if (MM_GetDungeonRewardIconInfo(dungeon, &icon) == 1)
                    ComboReceipt_CopyIcon(&entry.receiptPresentation, &icon, "mm");
#endif
                entry.receiptPresentation.rewardLine = 1;
            } else {
                const auto entrance = InformationEntranceName(dungeon);
                if (!entrance.empty())
                    body += "&It seems the entrance is at %c" + entrance + "%w.";
            }
            SetReceiptBody(entry, ComboItemReceiptText::FromNeiMarkup(body));
            return true;
        }
        // MM owns its ordinary map/compass tutorial too. A dormant OoT donor
        // can have a different saved information rule, including in solo MM.
        if (!Combo_IsCombinedGame())
            return NativeReceipt(item.getItemId, item.itemId, entry);
    }
    if (item.randoItemType == RITYPE_STRAY_FAIRY) {
        // The actor's native textbox is suppressed in rando. As with skull
        // tokens, CheckQueue builds this before GiveItem increments the count.
        // Use the shuffled item's dungeon, independently of the pickup scene.
        int fairyDungeon;
        switch (id) {
            case RI_WOODFALL_STRAY_FAIRY:
                fairyDungeon = DUNGEON_SCENE_INDEX_WOODFALL_TEMPLE;
                break;
            case RI_SNOWHEAD_STRAY_FAIRY:
                fairyDungeon = DUNGEON_SCENE_INDEX_SNOWHEAD_TEMPLE;
                break;
            case RI_GREAT_BAY_STRAY_FAIRY:
                fairyDungeon = DUNGEON_SCENE_INDEX_GREAT_BAY_TEMPLE;
                break;
            case RI_STONE_TOWER_STRAY_FAIRY:
                fairyDungeon = DUNGEON_SCENE_INDEX_STONE_TOWER_TEMPLE;
                break;
            default:
                return false; // Clock Town has one fairy and its own dialogue.
        }
        const unsigned count = gSaveContext.save.saveInfo.inventory.strayFairies[fairyDungeon] + 1;
        // Standard native message colors match the fairy's dungeon palette.
        constexpr const char* colors[] = { "%p", "%g", "%b", "%y" };
        const std::string color = colors[fairyDungeon];
        SetReceiptBody(entry, ComboItemReceiptText::FromNeiMarkup(
                                  "You found a " + color + item.name + "%w!^You've collected " + color +
                                  std::to_string(count) + "%w " + (count == 1 ? "stray fairy" : "stray fairies") +
                                  " in " + color + kDungeonInformation[fairyDungeon].temple + "%w."));
        return true;
    }
    if (item.randoItemType == RITYPE_SKULLTULA_TOKEN) {
        // CheckQueue composes this before GiveItem. Native MESSAGE_TOKENS
        // instead reads the current scene, which is wrong for shuffled tokens.
        unsigned count;
        std::string place;
        switch (id) {
            case RI_GS_TOKEN_SWAMP:
                count = (gSaveContext.save.saveInfo.skullTokenCount >> 16) + 1;
                place = " in the Swamp Spider House";
                break;
            case RI_GS_TOKEN_OCEAN:
                count = (gSaveContext.save.saveInfo.skullTokenCount & 0xFFFF) + 1;
                place = " in the Ocean Spider House";
                break;
            case RI_OOT_GS_TOKEN:
                // The FC total includes newly found tokens pending the next
                // shared-state projection into MM's OoT quest-page counter.
                count = std::max<unsigned>(Nei_Save()->ootGsCount, Nei_Save()->comboObtainedFc[FCI_OOT_GS_TOKEN]) + 1;
                place = " in Hyrule";
                break;
            default:
                return false;
        }
        const std::string article = item.article && *item.article ? std::string(item.article) + " " : "";
        const std::string color = id == RI_OOT_GS_TOKEN ? "%r" : "%c";
        SetReceiptBody(entry, ComboItemReceiptText::FromNeiMarkup(
                                  "You got " + article + color + item.name + "%w!^You've collected " + color +
                                  std::to_string(count) + "%w " + (count == 1 ? "token" : "tokens") + place + "."));
        return true;
    }
    // A junk reward still owns a native receipt. Prefer that brief body over
    // the queue's generic name/icon text, then allow imported ammo/refills to
    // use their donor or local registry when MM has no matching native GI.
    if (item.randoItemType == RITYPE_JUNK && NativeReceipt(item.getItemId, item.itemId, entry))
        return true;
    switch (id) {
        case RI_BOTTLE_RED_POTION:
            // GI_POTION_RED_BOTTLE is a model-only alias with ITEM_LONGSHOT.
            if (NativeReceipt(GI_POTION_RED, ITEM_POTION_RED, entry))
                return true;
            break;
        case RI_BOTTLE_CHATEAU_ROMANI:
            // GI_CHATEAU describes the refill (ITEM_CHATEAU_2), not this bottle.
            if (NativeReceipt(GI_CHATEAU_BOTTLE, ITEM_CHATEAU, entry))
                return true;
            break;
        case RI_SWORD_KOKIRI:
        case RI_SWORD_RAZOR:
        case RI_SWORD_GILDED:
        case RI_GREAT_FAIRY_SWORD:
        case RI_SHIELD_HERO:
        case RI_SHIELD_MIRROR:
        case RI_BOTTLE_EMPTY:
        case RI_BOTTLE_MILK:
        case RI_BOTTLE_GOLD_DUST:
        case RI_GOLD_DUST_REFILL:
        case RI_MUSHROOM:
            // MM owns these equipment and bottle tutorials. Shared OoT names
            // must not replace their native lessons or content mechanics.
            if (NativeReceipt(item.getItemId, item.itemId, entry,
                              id == RI_SHIELD_MIRROR && gSaveContext.options.language == LANGUAGE_ENG ? item.name
                                                                                                      : nullptr)) {
                return true;
            }
            break;
        default:
            break;
    }
    if (item.randoItemType != RITYPE_MAJOR && item.randoItemType != RITYPE_MASK &&
        item.randoItemType != RITYPE_LESSER && item.randoItemType != RITYPE_HEALTH && item.randoItemType != RITYPE_JUNK)
        return false;
    const char* concrete = ConcreteReceiptName(id);
    // Cross-game text export carries English bytes only. MM owns the meaning
    // and locale of its songs, including shared songs and both lullaby tiers.
    if (concrete) {
        if (const auto* song = ComboSongReceiptText::Find(concrete)) {
            const char* text = gSaveContext.options.language == LANGUAGE_GER   ? song->german
                               : gSaveContext.options.language == LANGUAGE_FRE ? song->french
                                                                               : song->english;
            SetReceiptBody(entry, ComboItemReceiptText::FromNeiMarkup(text));
            return true;
        }
    }
#ifdef COMBO_BUILD
    if (concrete && ApplyForeignItemReceiptText(concrete, entry))
        return true;
    // Single-identity catalog rows can use the full OoT description directly.
    // Progressive pool rows need concrete mapping above or MM's own native text.
    const int fc = FcCombo_ItemForNative(id);
    if (!concrete && fc >= 0 && fc < FC_COMBO_ITEM_COUNT && gFcComboItems[fc].chainLen == 1 &&
        *gFcComboItems[fc].ootName && ApplyForeignItemReceiptText(gFcComboItems[fc].ootName, entry))
        return true;
#endif
    if (VanillaItemFallback(concrete ? concrete : item.name, entry))
        return true;
    // MM's registry uses RI IDs in its rg column, including rows with ITEM_NONE
    // in StaticData. This lookup also works in standalone 2Ship without OoT.
    const NeiItem* nei = id == RI_OOT_NEI_ROCS_CAPE ? Nei_FindByItem(ITEM_ROCS_CAPE) : Nei_FindByRg(id);
    if (!nei && item.itemId != ITEM_NONE && !concrete)
        nei = Nei_FindByItem(item.itemId);
    if (nei && nei->nameEn && *nei->nameEn) {
        SetReceiptBody(entry, ComboItemReceiptText::FromNeiMarkup(nei->nameEn));
        return true;
    }
    // Clawshot and upper Hookshot tiers reuse a vanilla GI/model but aren't that
    // vanilla item. A missing donor must fall back safely, not describe Hookshot.
    if (id == RI_CLAWSHOT || (id == RI_HOOKSHOT && std::string_view(concrete) != "Hookshot"))
        return false;
    if (NativeReceipt(item.getItemId, item.itemId, entry))
        return true;
    return false;
}

void Rando::AppendReceiptSource(CustomMessage::Entry& entry, const std::string& source) {
    if (entry.autoFormat) {
        entry.msg += source;
        return;
    }
    if (!source.empty())
        entry.msg += "\x10" + source;
    // The queue/custom-item path needs a normal END; vanilla EVENT, FADE and
    // next-textbox behavior must never escape into this receipt.
    entry.msg += '\xBF';
}
