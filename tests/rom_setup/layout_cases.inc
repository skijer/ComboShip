// Included by the existing native seed/settings fixture in its MQ test mode.
static int RunMqLayoutTests() {
    using namespace Rando;
    SaveManager storage;
    SaveManager::Instance = &storage;
    auto settings = Settings::GetInstance();
    RegisterInformationOption(*settings);
    RegisterSeasonsOptions(*settings);
    RegisterMqOptions(*settings);
    auto context = Context::CreateInstance();
    settings->AssignContext(context);
    globals.gRandoContext = context;
    auto mask = [&]() {
        unsigned result = 0;
        for (size_t i = 0; i < 12; ++i)
            if (context->GetDungeon(i)->IsMQ()) result |= 1u << i;
        return result;
    };
    auto set = [](const char* suffix, int value) {
        CVarSetInteger((std::string("gRandoSettings.") + suffix).c_str(), value);
    };
    auto trials = [&]() {
        unsigned result = 0;
        for (auto* trial : context->GetTrials()->GetTrialList())
            if (trial->IsRequired()) result |= 1u << trial->GetTrialKey();
        return result;
    };
    auto keyrings = [&]() {
        unsigned result = 0;
        for (size_t i = 0; i < 12; ++i)
            if (context->GetDungeon(i)->HasKeyRing()) result |= 1u << i;
        return result;
    };
    set("MQDungeons", RO_MQ_DUNGEONS_SET_NUMBER);
    set("MQDungeonCount", 6);
    SOH_SetComboRandoSeed(0x12345678);
    Random_Init(1);
    SOH_PrepRandoContext();
    const unsigned first = mask();
    Check(context->GetDungeons()->CountMQ() == 6, "native Set Number creates six MQ and six vanilla dungeons");
    Random_Init(99999);
    SOH_PrepRandoContext();
    Check(mask() == first, "same Combo seed reproduces MQ layouts independently of prior RNG state");
    Random_Init(44444);
    EnsureOracleInit();
    Check(mask() == first, "oracle initialization must retain layouts used to build the item/check pools");
    const std::string snapshot = SOH_DumpRandoSettings();
    set("MQDungeons", RO_MQ_DUNGEONS_NONE);
    SOH_RestoreRandoSettings(snapshot.c_str());
    SOH_SetComboRandoSeed(0x12345678);
    Random_Init(7777);
    SOH_PrepRandoContext();
    Check(mask() == first, "consolidated settings replay plus master seed restores the same MQ layout");

    // Native per-dungeon selection is used by both OoT-start and MM-start Combo.
    for (int mmStart : {0, 1}) {
        gComboStartingGameMM = mmStart;
        set("MQDungeons", RO_MQ_DUNGEONS_SELECTION);
        for (auto* dungeon : context->GetDungeons()->GetDungeonList())
            CVarSetInteger(settings->GetOption(dungeon->GetMQSetting()).GetCVarName().c_str(), RO_MQ_SET_VANILLA);
        set("MQDungeonsWaterTemple", RO_MQ_SET_MQ);
        set("MQDungeonsJabuJabu", RO_MQ_SET_MQ);
        set("MQDungeonsGanonsCastle", RO_MQ_SET_MQ);
        SOH_PrepRandoContext();
        Check(mask() == 0x824 && context->GetDungeons()->CountMQ() == 3,
              "explicit vanilla/MQ dungeon choices survive either starting game");
        Check(context->GetDungeon(WATER_TEMPLE)->GetSmallKeyCount() == 2 &&
                  context->GetDungeon(FOREST_TEMPLE)->GetSmallKeyCount() == 5,
              "small-key counts follow each dungeon's selected quest");
    }
    json saved = json::object();
    storage.currentJsonContext = &saved;
    SaveNativeSettings();
    SaveMqDungeons();
    Check(saved.at("masterQuestDungeons") == json::array({2, 5, 11}), "native save records exact mixed dungeon IDs");
    ResetContext(context);
    globals.gRandoContext = context;
    storage.currentJsonContext = &saved;
    LoadNativeSettings();
    LoadMqDungeons();
    Check(mask() == 0x824, "native save/reload restores mixed layouts without rolling again");

    globals.masterQuest = false;
    SOH_PrepRandoContext();
    Check(mask() == 0, "vanilla-only installations cannot select unavailable MQ dungeons");
    globals.original = false; globals.masterQuest = true;
    SOH_PrepRandoContext();
    Check(mask() == 0xfff, "MQ-only installations retain the native all-MQ fallback");
    globals.original = true;
    for (int count : {0, 6, 12}) {
        set("MQDungeons", RO_MQ_DUNGEONS_SET_NUMBER);
        set("MQDungeonCount", count);
        set("MQDungeonsSelection", 0);
        SOH_PrepRandoContext();
        Check(context->GetDungeons()->CountMQ() == size_t(count), "native exact-count control includes both endpoints");
    }
    // A new installation may have only MQ, while its disabled UI preference is
    // None. Adding vanilla later must not change this seed on spoiler replay.
    globals.original = false;
    set("MQDungeons", RO_MQ_DUNGEONS_NONE);
    set("GanonTrial", RO_GANONS_TRIALS_SET_NUMBER);
    set("GanonTrialCount", 3);
    set("ShuffleKeyRings", RO_KEYRINGS_RANDOM);
    SOH_SetComboRandoSeed(1234);
    SOH_PrepRandoContext();
    Check(mask() == 0xfff, "MQ-only baseline resolves all dungeons to MQ");
    const auto originalTrials = trials(), originalKeyrings = keyrings();
    json consolidated{{"settings", json::parse(SOH_DumpRandoSettings())}};
    ComboRando::RecordOotDungeonLayouts(consolidated, SOH_DumpDungeonLayouts);
    Check(consolidated.at("dungeonLayoutAssets") == 2, "snapshot records original MQ-only normalization inputs");
    globals.original = true;
    SOH_RestoreRandoSettings(consolidated["settings"].dump().c_str());
    SOH_SetComboRandoSeed(1234);
    Check(ComboRando::RestoreOotDungeonLayouts(consolidated, SOH_SetDungeonLayoutsForCombo), "recorded layouts accepted after upgrade");
    SOH_PrepRandoContext();
    EnsureOracleInit();
    Check(mask() == 0xfff, "spoiler replay preserves MQ-only seed after vanilla assets are added");
    Check(trials() == originalTrials && keyrings() == originalKeyrings,
          "archive upgrade replay preserves later random trials and keyrings");
    Check(integerCVars["gRandoSettings.MQDungeons"] == RO_MQ_DUNGEONS_NONE,
          "resolved layout replay does not overwrite editable menu preferences");
    Check(Combo_SOH_GenerationQuestAssets() == 3, "generation override leaves actual archive availability intact");
    const auto acceptedLayouts = sComboRecordedMqDungeons;
    Check(!SOH_ValidateDungeonLayoutsForCombo("{\"masterQuestDungeons\":[\"not a dungeon\"]}") &&
              sComboContextPrepared && sOracleInitialized && sComboRecordedMqDungeons == acceptedLayouts,
          "layout preflight is pure and preserves the prepared current seed");
    Check(!SOH_SetDungeonLayoutsForCombo("{\"masterQuestDungeons\":[\"not a dungeon\"]}") &&
              !SOH_SetDungeonLayoutsForCombo("{\"masterQuestDungeons\":[1]}") &&
              !SOH_SetDungeonLayoutsForCombo("{\"masterQuestDungeons\":[],\"dungeonLayoutAssets\":2}") &&
              !SOH_SetDungeonLayoutsForCombo("{\"masterQuestDungeons\":[],\"dungeonLayoutAssets\":0}") &&
              !SOH_SetDungeonLayoutsForCombo("{\"masterQuestDungeons\":[],\"dungeonLayoutAssets\":\"3\"}") &&
              !SOH_SetDungeonLayoutsForCombo("{}"),
          "malformed and unknown layout identities are rejected atomically");
    SOH_PrepRandoContext();
    Check(mask() == 0xfff, "invalid replacement leaves prior accepted layout unchanged");
    globals.masterQuest = false;
    Check(!ComboRando::RestoreOotDungeonLayouts(consolidated, SOH_SetDungeonLayoutsForCombo),
          "seed requiring MQ rejects a vanilla-only installation");
    globals.masterQuest = true;
    Check(SOH_SetDungeonLayoutsForCombo("{\"masterQuestDungeons\":[]}"), "explicit all-vanilla array accepted");
    SOH_PrepRandoContext();
    Check(mask() == 0, "empty recorded array overrides an all-MQ previous seed");
    SOH_SetComboRandoSeed(5678);
    SOH_PrepRandoContext();
    Check(!sComboRecordedMqDungeons.has_value() && !sComboRecordedQuestAssets.has_value(),
          "new seed clears prior recorded layout and normalization overrides");

    // Native random-count and independent per-dungeon Random remain available.
    set("MQDungeons", RO_MQ_DUNGEONS_RANDOM_NUMBER);
    std::set<unsigned> randomMasks;
    for (unsigned seed = 1; seed <= 8; ++seed) {
        SOH_SetComboRandoSeed(seed);
        SOH_PrepRandoContext();
        const auto expected = mask();
        randomMasks.insert(expected);
        Random_Init(999);
        SOH_PrepRandoContext();
        Check(mask() == expected, "random MQ count and layouts reproduce for a fixed Combo seed");
    }
    Check(randomMasks.size() > 1, "random count mode responds to different seeds");
    set("MQDungeons", RO_MQ_DUNGEONS_SELECTION);
    for (auto* dungeon : context->GetDungeons()->GetDungeonList())
        CVarSetInteger(settings->GetOption(dungeon->GetMQSetting()).GetCVarName().c_str(), RO_MQ_SET_VANILLA);
    set("MQDungeonsDekuTree", RO_MQ_SET_RANDOM);
    randomMasks.clear();
    for (unsigned seed = 100; seed < 116; ++seed) {
        SOH_SetComboRandoSeed(seed);
        SOH_PrepRandoContext();
        const auto expected = mask();
        Check(expected <= 1, "individual Random changes only the selected dungeon");
        randomMasks.insert(expected);
        Random_Init(998);
        SOH_PrepRandoContext();
        Check(mask() == expected, "individual Random reproduces its choice for a fixed Combo seed");
    }
    Check(randomMasks.size() == 2, "individual Random can resolve either vanilla or MQ");
    context->GetLogic()->SetContext(nullptr);
    std::puts("PASS native MQ modes, mixed key counts, deterministic Combo prep/oracle/replay, and native save/reload");
    return 0;
}
