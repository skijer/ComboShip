static constexpr uint32_t kMoon = 1u << 11;

static json TradeState() {
    const auto* nei = Nei_Save();
    return {{"tradeAdultOwned", nei->tradeAdultOwned}, {"comboObtainedFc", nei->comboObtainedFc},
            {"comboAppliedFc", nei->comboAppliedFc}, {"extBootsLayoutVersion", nei->extBootsLayoutVersion},
            {"extTunicLayoutVersion", nei->extTunicLayoutVersion}};
}

static void CheckTrade(uint32_t expected, const char* context) {
    Check(Nei_Save()->tradeAdultOwned == expected, context);
    Check((Nei_Save()->comboObtainedFc[FCI_MM_MOONS_TEAR] != 0) == ((expected & kMoon) != 0),
          "Moon's Tear receipt must follow its real grant, not a soul/dungeon grant");
#if HOST_MM
    Check((Flags_GetRandoInf(RANDO_INF_OBTAINED_MOONS_TEAR) != 0) == ((expected & kMoon) != 0),
          "MM's native Moon's Tear flag must follow an actual Moon's Tear grant");
#endif
}

#if HOST_MM
static void FoldNativeTrades() {
    for (const auto slot : {SLOT_TRADE_DEED, SLOT_TRADE_KEY_MAMA, SLOT_TRADE_COUPLE})
        TradeAdult_FoldCurrent(gSaveContext.save.saveInfo.inventory.items[slot]);
}

static void ResetNative() {
    Reset();
    std::memset(gSaveContext.save.saveInfo.inventory.items, ITEM_NONE,
                sizeof(gSaveContext.save.saveInfo.inventory.items));
}

static json NativeTradeState() {
    return {{"items", gSaveContext.save.saveInfo.inventory.items},
            {"randoInf", gSaveContext.save.shipSaveInfo.rando.randoInf}};
}

static json AddStartingScenario(unsigned scenario, std::vector<RandoItemId> configured, unsigned optionBits) {
    ResetNative();
    auto& rando = gSaveContext.save.shipSaveInfo.rando;
    Rando::SetStartingItemsInSave(rando, configured);
    const RandoOptionId booleans[] = {
        RO_STARTING_ROD_OF_SEASONS, RO_STARTING_MAPS_AND_COMPASSES, RO_SHUFFLE_SWIM,
        RO_SHUFFLE_ENEMY_SOULS, RO_SHUFFLE_OCARINA_BUTTONS, RO_SHUFFLE_SONG_DOUBLE_TIME,
        RO_SHUFFLE_SONG_INVERTED_TIME, RO_CLOCK_SHUFFLE, RO_CLOCK_SHUFFLE_PROGRESSIVE,
    };
    for (unsigned bit = 0; bit < std::size(booleans); ++bit)
        rando.randoSaveOptions[booleans[bit]] = (optionBits >> bit) & 1u;
    const RandoOptionId dungeon[] = {RO_PLACEMENT_SMALL_KEYS, RO_PLACEMENT_BOSS_KEYS, RO_PLACEMENT_STRAY_FAIRIES};
    for (unsigned bit = 0; bit < std::size(dungeon); ++bit)
        rando.randoSaveOptions[dungeon[bit]] =
            (optionBits & (1u << (std::size(booleans) + bit))) ? RO_DUNGEON_ITEM_START_WITH : RO_DUNGEON_ITEM_VANILLA;
    rando.randoSaveOptions[RO_STARTING_HEALTH] = 3;
    const auto computed = Rando::GetComputedStartingItems(rando);
    Check(std::find(computed.begin(), computed.end(), RI_MOONS_TEAR) == computed.end(),
          "computed starting options must never select Moon's Tear");
    Rando::GrantStartingItems();
    FoldNativeTrades();
    uint32_t expected = 0;
    for (const auto item : configured) {
        const auto it = Rando::StaticData::Items.find(item);
        if (it != Rando::StaticData::Items.end()) {
            const int index = TradeAdult_IndexOfItem(it->second.itemId);
            Check(index >= 11 && index <= 19, "configured trade metadata must identify a real trade entry");
            expected |= 1u << index;
        }
    }
    CheckTrade(expected, "MM starting items must grant only their configured trade entries");
    const auto saved = SaveBlob();
    LoadBlob(saved);
    CheckTrade(expected, "MM starting trade ownership and receipts must survive the native serializer");
    return {{"scenario", scenario}, {"expected", expected}, {"shared", ExportTrade()},
            {"mm", TradeState()}, {"mmNative", NativeTradeState()}};
}

static json StartingScenarios() {
    json scenarios = json::array();
    unsigned count = 0;
    // Exhaust every combination of the twelve options that add computed items.
    for (unsigned bits = 0; bits < (1u << 12); ++bits)
        scenarios.push_back(AddStartingScenario(count++, {}, bits));
    // Every configured UI starting choice passes through the same native list and
    // record boundary. Unrelated local item effects are deliberately out of scope.
    for (const auto item : kStartingChoices)
        scenarios.push_back(AddStartingScenario(count++, {item}, 1u << 3));
    std::printf("PASS MM: %u computed-option combinations and %zu explicit starting choices\n", 1u << 12,
                std::size(kStartingChoices));
    return scenarios;
}
#else
static void CheckNativeGrants() {
    static_assert(std::size(kNoopItems) == 68);
    for (const auto item : kNoopItems) {
        Reset();
        GrantNative(item);
        if (Nei_Save()->tradeAdultOwned != 0) {
            std::fprintf(stderr, "FAIL: RG=%d (fcId=%d) granted trade bits=0x%08X without a trade pickup\n",
                         item, FcCombo_ItemForNative(item), Nei_Save()->tradeAdultOwned);
            std::exit(1);
        }
        const int fc = FcCombo_ItemForNative(item);
        Check(fc != FCI_NO_ITEM && Nei_Save()->comboObtainedFc[fc] == 1 && Nei_Save()->comboAppliedFc[fc] == 1,
              "a dungeon/soul grant must retain its own obtained/applied FC receipt");
        CheckTrade(0, "a dungeon/soul grant must leave the trade wheel empty");
        LoadBlob(SaveBlob());
        CheckTrade(0, "a dungeon/soul receipt must survive native save/reload without Moon's Tear");
        ApplyFcRegistryToNatives();
        CheckTrade(0, "an applied dungeon/soul receipt must not cause a second grant");
    }
    std::printf("PASS %zu MM dungeon/soul grant cases do not grant Moon's Tear\n", std::size(kNoopItems));
    const RandomizerGet trade[] = {RG_MM_MOONS_TEAR, RG_MM_DEED_LAND, RG_MM_DEED_SWAMP, RG_MM_DEED_MOUNTAIN,
        RG_MM_DEED_OCEAN, RG_MM_ROOM_KEY, RG_MM_LETTER_TO_KAFEI, RG_MM_LETTER_TO_MAMA, RG_MM_PENDANT_OF_MEMORIES};
    for (unsigned index = 0; index < std::size(trade); ++index) {
        Reset();
        GrantNative(trade[index]);
        const uint32_t expected = 1u << (11 + index);
        CheckTrade(expected, "a real OoT trade grant must retain its own wheel entry");
        LoadBlob(SaveBlob());
        CheckTrade(expected, "an earned OoT trade item must survive save/reload");
    }
    std::puts("PASS all nine real MM trade grants in OoT retain their ownership and receipts");
}
#endif

static void Receive(json& scenarios) {
    for (auto& scenario : scenarios) {
#if HOST_MM
        ResetNative();
        constexpr const char* host = "mm";
#else
        Reset();
        constexpr const char* host = "oot";
#endif
        if (scenario.contains(host))
            LoadBlob(scenario[host]);
#if HOST_MM
        if (scenario.contains("mmNative")) {
            scenario["mmNative"]["items"].get_to(gSaveContext.save.saveInfo.inventory.items);
            scenario["mmNative"]["randoInf"].get_to(gSaveContext.save.shipSaveInfo.rando.randoInf);
        }
#endif
        const uint32_t expected = scenario["expected"];
        // Real merge/materialization is repeated across a native save/reload.
        for (int round = 0; round < 2; ++round) {
            ImportTrade(scenario["shared"]);
            ApplyFcRegistryToNatives();
#if HOST_MM
            FoldNativeTrades();
#endif
            if (Nei_Save()->tradeAdultOwned != expected) {
                std::fprintf(stderr, "FAIL: %s scenario=%u expected=0x%08X actual=0x%08X after native sync grant\n",
                    host, scenario["scenario"].get<unsigned>(), expected, Nei_Save()->tradeAdultOwned);
                std::exit(1);
            }
            CheckTrade(expected, "paired sync must grant Moon's Tear only for an actual Moon's Tear start/pickup");
            const auto before = TradeState();
            LoadBlob(SaveBlob());
            Check(TradeState() == before, "native save/reload must preserve trade ownership, receipts and layout versions");
        }
        scenario[host] = TradeState();
        scenario["shared"] = ExportTrade();
#if HOST_MM
        scenario["mmNative"] = NativeTradeState();
#endif
    }
    std::printf("PASS %s paired sync/materialize/native save/reload: %zu scenarios\n",
                HOST_MM ? "MM" : "OoT", scenarios.size());
}

int main(int argc, char** argv) {
    Check(argc == 3, "usage: test <start|receive> <private pair JSON>");
#if !HOST_MM
    SaveManager manager;
    SaveManager::Instance = &manager;
    sFixtureSaveDirectory = std::getenv("NEI_SAVE_TEST_DIRECTORY");
    std::filesystem::create_directories(sFixtureSaveDirectory);
    if (!std::getenv("NEI_MOON_TEST_PAIR_ONLY"))
        CheckNativeGrants();
#endif
    json scenarios;
#if HOST_MM
    if (std::string(argv[1]) == "start") {
        scenarios = StartingScenarios();
    } else
#endif
    {
        std::ifstream input(argv[2]);
        Check(bool(input), "the paired host's private JSON output must be readable");
        input >> scenarios;
        Receive(scenarios);
    }
    std::ofstream output(argv[2]);
    output << scenarios;
    Check(bool(output), "the private paired JSON output must be written");
    return 0;
}
