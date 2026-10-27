#!/usr/bin/env python3
"""Probe the native information option from generation CVar to saved Context.

This uses native Option/Settings/Context declarations, real Option construction,
the complete SetAllToContext and FinalizeSettings bodies, and SaveManager's real
JSON traversal/templates. Its optional route fixture uses native Entrance edges
and complete CreateEntranceOverrides/JSON dump bodies. The assigned edge pool is
an input fixture; graph fill, disk I/O and game loops are outside this boundary.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from


def extract(source, signature):
    pos = source.index(signature)
    return block_from(source, pos)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitizers", action="store_true")
    parser.add_argument("--mq-layouts", action="store_true",
                        help="exercise native vanilla/MQ selection, Combo prep, oracle reuse and save/reload")
    parser.add_argument("--ignore-recorded-quest-assets", action="store_true",
                        help="negative control: normalize replay against newly installed archives")
    parser.add_argument("--drop-appended-option", action="store_true",
                        help="negative control: omit the information option during Context copy")
    parser.add_argument("--dump-entrance-fixture", type=Path,
                        help="write native generated/dumped entrance tables for the receiver regression")
    parser.add_argument("--drop-grace-reset", action="store_true",
                        help="negative control: retain prior Grace policy on old native spoiler import")
    args = parser.parse_args()
    read = lambda path: (ROOT / path).read_text()
    settings = read("soh/soh/Enhancements/randomizer/settings.cpp")
    context = read("soh/soh/Enhancements/randomizer/SeedContext.cpp")
    item = read("soh/soh/Enhancements/randomizer/item_location.cpp")
    otr = read("soh/soh/OTRGlobals.cpp")
    manager = read("soh/soh/SaveManager.cpp")
    # Native option storage/construction only. Unexecuted AddWidget callbacks
    # belong to the GUI library and are outside this settings boundary.
    option = read("soh/soh/Enhancements/randomizer/option.cpp")
    # Native translated metadata remains outside this seed-storage boundary.
    # Include its production dispatch helpers; the fixture supplies the language service.
    metadata = option[option.index('const static std::string namePostfix'):
                      option.index('#pragma endregion')]
    functions = [metadata] + [extract(option, signature) for signature in (
        "Option Option::Bool(RandomizerSettingKey key_, std::string name_, std::string cvarName_",
        "Option Option::U8(RandomizerSettingKey key_, std::string name_, std::vector<std::string> options_",
        "Option::Option(size_t key_", "void Option::PopulateTextToNum()",
        "OptionValue::OptionValue(uint8_t val)", "uint8_t OptionValue::Get()",
        "void OptionValue::Set(uint8_t val)", "OptionValue::operator bool() const",
        "uint8_t Option::GetValueFromText", "void Option::SetContextIndex",
        "const std::string& Option::GetName() const", "const std::string& Option::GetDescription() const",
        "OptionCategory Option::GetCategory() const",
        "uint8_t Option::GetOptionIndex() const",
        "size_t Option::GetOptionCount() const",
        "uint8_t Option::GetMenuOptionDefault() const", "const std::string& Option::GetCVarName() const")]
    for signature in ("Settings::Settings()", "std::shared_ptr<Settings> Settings::GetInstance()",
                      "Option& Settings::GetOption", "const std::array<Option, RSK_MAX>& Settings::GetAllOptions()",
                      "std::unordered_map<std::string, RandomizerSettingKey> Settings::PopulateOptionNameToEnum()",
                      "TrickSetting& Settings::GetTrickSetting", "void Settings::ParseJson",
                      "void Settings::AssignContext", "void Settings::ClearContext", "void Settings::SetAllToContext()",
                      "void Context::ResetTrickOptions()", "void Context::FinalizeSettings"):
        if signature == "Settings::Settings()" and signature not in settings:
            continue  # 9.3 deliberately defers CreateOptions until service initialization.
        body = extract(settings, signature)
        if args.ignore_recorded_quest_assets and signature == "void Context::FinalizeSettings":
            body = body.replace("Combo_SOH_GenerationQuestAssets()",
                                "((OTRGlobals::Instance->HasOriginal() ? 1 : 0) | "
                                "(OTRGlobals::Instance->HasMasterQuest() ? 2 : 0))")
        if args.drop_appended_option and signature == "void Settings::SetAllToContext()":
            body = body.replace("mContext->GetOption(static_cast<RandomizerSettingKey>(i)).Set",
                                "if (i != RSK_MAPS_COMPASSES_GIVE_INFORMATION) mContext->GetOption(static_cast<RandomizerSettingKey>(i)).Set", 1)
        if args.drop_grace_reset and signature == "void Settings::ParseJson":
            body = body.replace("mContext->GetOption(RSK_HYLIAS_GRACE).Set(NEI_GRACE_ON);", "")
            body = body.replace("mContext->GetOption(RSK_HYLIAS_GRACE_REWARDS).Set(4);", "")
        functions.append(body)
    # Execute the exact target registration; no manual SetOption(On) substitutes
    # for the CVar-backed native boolean or its actual default/prefix.
    registration = re.search(r"^\s*OPT_BOOL\(RSK_MAPS_COMPASSES_GIVE_INFORMATION,[^\n]*;", settings, re.M).group(0)
    functions.append("void RegisterInformationOption(Settings& settings) {\n"
                     "#define OPT_BOOL(rsk, ...) settings.mOptions[rsk] = Option::Bool(rsk, __VA_ARGS__)\n"
                     "    auto& mOptionDescriptions = settings.mOptionDescriptions;\n" + registration + "\n#undef OPT_BOOL\n}")
    seasons_registration = settings[settings.index("    OPT_U8(RSK_ROD_OF_SEASONS,"):
                                    settings.index("    OPT_U8(RSK_HYLIAS_GRACE,")]
    functions.append("void RegisterSeasonsOptions(Settings& settings) {\n"
                     "#define OPT_U8(rsk, ...) settings.mOptions[rsk] = Option::U8(rsk, __VA_ARGS__)\n"
                     "#define OPT_BOOL(rsk, ...) settings.mOptions[rsk] = Option::Bool(rsk, __VA_ARGS__)\n" +
                     seasons_registration + "\n#undef OPT_U8\n#undef OPT_BOOL\n}")
    if args.mq_layouts:
        for signature in ("Option Option::Bool(RandomizerSettingKey key_, std::vector",
                          "Option Option::U8(RandomizerSettingKey key_, std::vector"):
            functions.append(extract(option, signature))
        # Its default parameters contain {}, before the function body's brace.
        functions.append(settings[settings.index("std::vector<std::string> NumOpts("):
                                  settings.index("std::vector<std::string> MultiVecOpts(")])
        mq_registration = "\n".join(re.findall(
            r"^\s*OPT_(?:U8|BOOL)\(RSK_(?:MQ_[A-Z_]+|GANONS_TRIALS|TRIAL_COUNT|KEYRINGS(?:_[A-Z_]+)?),[^\n]*;",
            settings, re.M))
        functions.append("void RegisterMqOptions(Settings& settings) {\n"
                         "#define OPT_U8(rsk, ...) settings.mOptions[rsk] = Option::U8(rsk, __VA_ARGS__)\n"
                         "#define OPT_BOOL(rsk, ...) settings.mOptions[rsk] = Option::Bool(rsk, __VA_ARGS__)\n" +
                         mq_registration + "\n#undef OPT_U8\n#undef OPT_BOOL\n}")
    for signature in ("Context::Context()", "std::shared_ptr<Context> Context::CreateInstance()",
                      "std::shared_ptr<Context> Context::GetInstance()", "ItemLocation* Context::GetItemLocation(size_t",
                      "ItemLocation* Context::GetItemLocation(const RandomizerCheck", "OptionValue& Context::GetOption",
                      "OptionValue& Context::GetTrickOption", "std::shared_ptr<Logic> Context::GetLogic()",
                      "std::shared_ptr<EntranceShuffler> Context::GetEntranceShuffler()",
                      "std::shared_ptr<Dungeons> Context::GetDungeons()", "DungeonInfo* Context::GetDungeon(size_t",
                      "std::shared_ptr<Trials> Context::GetTrials()", "bool Context::IsSeedGenerated()",
                      "void Context::SetSeedGenerated", "bool Context::IsSpoilerLoaded()", "void Context::SetSpoilerLoaded"):
        functions.append(extract(context, signature))
    for signature in ("void Context::SetSeedString", "void Context::SetSeed("):
        functions.append(extract(context, signature))
    if args.mq_layouts:
        functions.append(extract(context, "DungeonInfo* Context::GetDungeonFromScene"))
    for signature in ("ItemLocation::ItemLocation()", "ItemLocation::ItemLocation(const RandomizerCheck",
                      "RandomizerCheck ItemLocation::GetRandomizerCheck() const", "void ItemLocation::SetExcludedOption"):
        functions.append(extract(item, signature))
    functions.append(extract(read("soh/soh/Enhancements/randomizer/hint.cpp"), "Hint::Hint()"))
    entrance = read("soh/soh/Enhancements/randomizer/entrance.cpp")
    for signature in ("EntranceShuffler::EntranceShuffler()", "void EntranceShuffler::SetNoRandomEntrances",
                      "void EntranceShuffler::CreateEntranceOverrides()",
                      "Entrance::Entrance(RandomizerRegion", "void Entrance::SetParentRegion",
                      "RandomizerRegion Entrance::GetParentRegionKey() const", "void Entrance::SetAsShuffled()",
                      "bool Entrance::IsShuffled() const", "int16_t Entrance::GetIndex() const", "void Entrance::SetIndex",
                      "void Entrance::SetReplacement", "Entrance* Entrance::GetReplacement() const",
                      "EntranceType Entrance::GetType() const", "void Entrance::SetType",
                      "Entrance* Entrance::GetReverse() const", "void Entrance::BindTwoWay"):
        functions.append(extract(entrance, signature))
    logic = read("soh/soh/Enhancements/randomizer/logic.cpp")
    for signature in ("Logic::Logic()", "void Logic::SetContext"):
        functions.append(extract(logic, signature))
    fish = read("soh/soh/Enhancements/randomizer/fishsanity.cpp")
    for signature in ("Fishsanity::Fishsanity()", "Fishsanity::~Fishsanity()", "void Fishsanity::InitializeHelpers()"):
        if signature in fish:
            functions.append(extract(fish, signature))
    functions.extend(re.findall(r'^[^\n]*Fishsanity::(?:fishsanityHelpersInit|pondFishAgeMap|childPondFish|adultPondFish)[^\n]*;',
                                fish, re.M))
    functions = "namespace Rando {\n" + "\n\n".join(functions) + "\n}\n"
    if args.mq_layouts:
        seed_start = otr.index("static uint64_t sComboRandoSeed =")
        seed_end = otr.index("static const PriceSettingsStruct", seed_start)
        seed_state = otr[seed_start:seed_end]
        for state in ("sOracleInitialized", "sComboContextPrepared", "sComboRecordedMqDungeons",
                      "sComboRecordedQuestAssets", "sComboFinalizingQuestAssets", "sComboPreparedQuestAssets"):
            if state not in seed_state:
                seed_state += re.search(r"^static [^\n]*\b" + state + r"\b[^\n]*;", otr, re.M).group(0) + "\n"
        functions += seed_state
        for signature in ('static uint8_t Combo_InstalledQuestAssets',
                          'extern "C" int Combo_SOH_GenerationQuestAssets',
                          'extern "C" COMBO_EXPORT void SOH_SetComboRandoSeed',
                          'extern "C" COMBO_EXPORT const char* SOH_DumpDungeonLayouts',
                          'static bool Combo_DecodeDungeonLayouts',
                          'extern "C" COMBO_EXPORT int SOH_ValidateDungeonLayoutsForCombo',
                          'extern "C" COMBO_EXPORT int SOH_SetDungeonLayoutsForCombo',
                          'extern "C" COMBO_EXPORT void SOH_PrepRandoContext', 'static void EnsureOracleInit()'):
            # The file has a forward declaration of EnsureOracleInit as well.
            position = otr.index(signature + " {") if signature.endswith("()") else otr.index(signature)
            functions += block_from(otr, position) + "\n"
    if 'static bool Combo_ClearNumericExclusions() {' in otr:
        functions += extract(otr, 'static bool Combo_ClearNumericExclusions() {') + "\n"
    for signature in ('extern "C" COMBO_EXPORT const char* SOH_DumpRandoSettings',
                      'extern "C" COMBO_EXPORT void SOH_RestoreRandoSettings',
                      'extern "C" COMBO_EXPORT const char* SOH_DumpEntranceOverrides'):
        functions += extract(otr, signature) + "\n"
    for signature in ("void SaveManager::SaveArray", "void SaveManager::LoadArray"):
        functions += extract(manager, signature) + "\n"
    save_start = manager.index('SaveManager::Instance->SaveArray("randoSettings"')
    load_start = manager.index('SaveManager::Instance->LoadArray("randoSettings"')
    save_block = manager[save_start:manager.index("});", save_start) + 3]
    load_block = manager[load_start:manager.index("});", load_start) + 3]
    functions += "void SaveNativeSettings() { auto randoContext = Rando::Context::GetInstance();\n" + save_block + "\n}\n"
    functions += "void LoadNativeSettings() { auto randoContext = Rando::Context::GetInstance();\n" + load_block + "\n}\n"
    if args.mq_layouts:
        mq_save_start = manager.index('    SaveManager::Instance->SaveData("masterQuestDungeonCount"')
        mq_save_end = manager.index('    SaveManager::Instance->SaveArray("requiredTrials"', mq_save_start)
        mq_load_start = manager.index('    size_t mqDungeonCount;')
        mq_load_end = manager.index('    randoContext->GetTrials()->SkipAll();', mq_load_start)
        functions += "void SaveMqDungeons() { auto randoContext = Rando::Context::GetInstance();\n" + manager[mq_save_start:mq_save_end] + "\n}\n"
        functions += "void LoadMqDungeons() { auto randoContext = Rando::Context::GetInstance();\n" + manager[mq_load_start:mq_load_end] + "\n}\n"
    functions += extract(read("soh/soh/Enhancements/randomizer/NeiSeasons.cpp"),
                         'extern "C" uint8_t Seasons_RandoMode') + "\n"
    # Execute the complete native item-count helper and complete NEI source pool
    # block against the real saved Context, including pool-density behavior.
    pool = read("soh/soh/Enhancements/randomizer/3drando/item_pool.cpp")
    functions += "Rando::Context* ctx;\n"
    functions += pool[pool.index("std::vector<RandomizerGet> itemPool"):
                      pool.index("void AddItemToPool(")]
    functions += extract(pool, "void AddItemToPool(") + "\n"
    functions += "void GenerateNativeNeiPool() {\n" + extract(pool, "if (ctx->GetOption(RSK_SKIJER_CUSTOM_ITEMS)) {") + "\n}\n"
    starting = read("soh/soh/Enhancements/randomizer/3drando/starting_inventory.cpp")
    functions += "std::vector<RandomizerGet> StartingInventory;\n"
    functions += extract(starting, "static void AddItemToInventory(") + "\n"
    start_prefix = starting[starting.index("    auto ctx =", starting.index("void GenerateStartingInventory")):
                            starting.index("    if (ctx->GetOption(RSK_SHUFFLE_MAPANDCOMPASS)")]
    functions += "void GenerateNativeStartingRod() {\n" + start_prefix + "\n}\n"
    with tempfile.TemporaryDirectory(prefix="compass-seed-settings-") as directory:
        build = Path(directory)
        (build / "seed_settings_production.inc").write_text(functions)
        flags = ["-std=c++20", "-O1", "-g", "-ffunction-sections", "-fdata-sections",
                 "-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DIMGUI_DEFINE_MATH_OPERATORS=", "-DLOG_LEVEL_GAME_PRINTS=0",
                 "-DFMT_HEADER_ONLY"]
        if args.mq_layouts:
            flags += ["-DCOMBO_MQ_LAYOUT_TESTS"]
            flags[flags.index("-O1")] = "-O0"
        # Use the actual configured CVar prefixes from the production CMake file.
        prefixes = read("CMake/soh-cvars.cmake")
        flags += ['-D' + key + '="' + value + '"' for key, value in
                  re.findall(r'set\((CVAR_PREFIX_\w+)\s+"([^"]+)"\)', prefixes)]
        if args.sanitizers:
            flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer"]
        includes = ["-I" + str(p) for p in (build, ROOT, ROOT / "soh", ROOT / "soh/include",
                    ROOT / "soh/assets", ROOT / "libultraship/include", ROOT / "combo", ROOT / "combo/menu")]
        sources = [ROOT / "tests/item_receipts/seed_settings_test.cpp",
                   ROOT / "soh/soh/Enhancements/randomizer/dungeon.cpp",
                   ROOT / "soh/soh/Enhancements/randomizer/trial.cpp",
                   ROOT / "soh/soh/Enhancements/randomizer/location.cpp",
                   ROOT / "soh/soh/Enhancements/randomizer/randomizerEnumStrings.cpp",
                   ROOT / "soh/soh/ShipUtils.cpp"]
        binary = build / "seed_settings_test"
        subprocess.run([os.environ.get("CXX", "c++"), *flags, *includes, *map(str, sources),
                        "-Wl,--gc-sections", "-o", str(binary)], cwd=ROOT, check=True)
        command = [str(binary)]
        if args.dump_entrance_fixture:
            command.append(str(args.dump_entrance_fixture))
        return subprocess.run(command, cwd=ROOT).returncode


if __name__ == "__main__":
    sys.exit(main())
