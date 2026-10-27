// Compiles the production header-only fill, display, playthrough and hint code.
#include "rando/ComboPlaythrough.h"
#ifndef COMBO_SHARED_BASELINE
#include "rando/CrossHints.h"
#endif
#include "rando/SharedItems.h"
#include <cstdlib>
#include <functional>
#include <iostream>
#include <stdexcept>

using namespace ComboRando;
using json = nlohmann::json;
static std::vector<std::string> owned[2];
static std::vector<std::string> checks[2];
static std::string required[2];
static int requiredCount[2] = { 0, 0 };
static bool openPortal = true;
static std::string reachable[2];
static int failures = 0;
static void check(bool ok, const std::string& what) {
    if (!ok) {
        ++failures;
        std::cerr << "FAIL: " << what << '\n';
    }
}
static void reset() {
}
static void setOot(const char* s) {
    owned[0] = json::parse(s).get<std::vector<std::string>>();
}
static void setMm(const char* s) {
    owned[1] = json::parse(s).get<std::vector<std::string>>();
}
static const char* reach(int g) {
    json out = checks[g];
    if (std::count(owned[g].begin(), owned[g].end(), required[g]) >= requiredCount[g])
        out.push_back(g == 0 ? "Ganon" : "Moon Majora Pot 01");
    reachable[g] = out.dump();
    return reachable[g].c_str();
}
static const char* reachOot() {
    return reach(0);
}
static const char* reachMm() {
    return reach(1);
}
static void place(const char*, const char*) {
}
static uint8_t portal() {
    return openPortal;
}
static const OracleFns oot{ reset, setOot, reachOot, place, portal };
static const OracleFns mm{ reset, setMm, reachMm, place };
static json emptyDump() {
    return { { "items", json::array() },
             { "pool", json::array() },
             { "checks", json::array() },
             { "accessibility", { { "maskQuestShuffle", true } } } };
}
static void add(json& d, std::string name, int copies, bool advancement = true) {
    d["items"].push_back({ { "name", name }, { "token", "RI_" + name } });
    for (int i = 0; i < copies; ++i) {
        d["pool"].push_back(
            { { "name", name }, { "advancement", advancement }, { "category", advancement ? "major" : "junk" } });
        d["checks"].push_back({ { "name", "Check " + std::to_string(d["checks"].size()) } });
    }
}
static void configure(const json& o, const json& m, std::string oReq = "", int oCount = 0, std::string mReq = "",
                      int mCount = 0) {
    for (int g = 0; g < 2; ++g) {
        owned[g].clear();
        checks[g].clear();
        for (const auto& c : (g == 0 ? o : m)["checks"])
            checks[g].push_back(c["name"]);
    }
    required[0] = oReq;
    requiredCount[0] = oCount;
    required[1] = mReq;
    requiredCount[1] = mCount;
    openPortal = true;
}
static std::string pairsJson(std::initializer_list<CwSharedPair> pairs) {
    json out = json::array();
    for (const auto& p : pairs)
        out.push_back({ { "oot", p.ootName }, { "mm", p.mmName } });
    return out.dump();
}
static void forwardAndRequiredness() {
    json o = emptyDump(), m = emptyDump();
    add(o, "Whip", 1);
    add(o, "Unrelated advancement", 1);
    add(m, "Junk", 1, false);
    configure(o, m, "", 0, "Whip", 1);
    json seed = { { "oot", { { "Check 0", "Whip" }, { "Check 1", "Unrelated advancement" } } },
                  { "mm", { { "Check 0", "Junk" } } },
                  { "foreign", json::array() } };
    auto pairs = pairsJson({ { "Whip", "Whip" } });
    auto run = RunPlaythrough(seed.dump(), oot, mm, "combo_shared", nullptr, nullptr, o.dump(), m.dump(), true, false,
                              {}, false
#ifndef COMBO_SHARED_BASELINE
                              ,
                              0, pairs
#endif
    );
    check(run.beatable, "NEI pair credits MM in forward playthrough");
    auto req = PareDownPlaythrough(seed.dump(), oot, mm, nullptr, o.dump(), m.dump(), {}, {}, DefaultGanonMajoraGoal,
                                   true, false, nullptr
#ifndef COMBO_SHARED_BASELINE
                                   ,
                                   0, pairs
#endif
    );
    check(req.requiredByCheck["oot:Check 0"], "NEI shared gate item remains required");
    check(!req.requiredByCheck["oot:Check 1"], "unrelated advancement is not required by a missing NEI mirror");
#ifndef COMBO_SHARED_BASELINE
    seed["oot"]["Check 0"] = "Fire Arrows";
    seed["sharedItems"] = { "fireArrows" };
    required[1] = "Fire Arrows";
    req = PareDownPlaythrough(seed.dump(), oot, mm, nullptr, o.dump(), m.dump());
    check(req.requiredByCheck["oot:Check 0"] && !req.requiredByCheck["oot:Check 1"],
          "requiredness consumes the seed's effective family mask");
    // Explicit requested mask must not override an effective mask which rejected that family.
    seed["sharedItems"] = json::array();
    run = RunPlaythrough(seed.dump(), oot, mm, "ineffective", nullptr, nullptr, o.dump(), m.dump(), true, false, {},
                         false, 1u << SF_FIRE_ARROWS);
    check(!run.beatable, "seed effective mask takes precedence over the requested mask");
    openPortal = false;
    run = RunPlaythrough(seed.dump(), oot, mm, "no-logic", nullptr, nullptr, o.dump(), m.dump(), false, false, {},
                         false, 0, pairs);
    check(run.reachableMm == 1, "ungated ever-reachable inventory honors No Logic");
    openPortal = true;
#endif
}
static void parsedNamesAndDisplay() {
    json o = emptyDump(), m = emptyDump();
    add(o, "Goron Mask (MM)", 1);
    add(o, "Goron Mask", 1);
    add(m, "Goron Mask", 1);
    json seed = { { "oot", { { "native", "Goron Mask (MM)" }, { "tagged", "Goron Mask (OOT)" } } },
                  { "mm", { { "foreign", "Goron Mask (MM)" } } },
                  { "foreign", json::array({ { { "checkGame", "mm" },
                                               { "checkName", "foreign" },
                                               { "itemGame", "oot" },
                                               { "itemName", "Goron Mask (MM)" } } }) } };
    auto pl = ParseSpoilerPlacements(seed.dump(), o.dump(), m.dump());
    for (const auto& p : pl)
        check(p.item == (p.check == "tagged" ? "Goron Mask" : "Goron Mask (MM)"),
              "preserve intrinsic NEI mask name for " + p.check);
    auto foreign = json::array({ { { "checkGame", "mm" },
                                   { "checkName", "whip" },
                                   { "itemGame", "oot" },
                                   { "itemName", "Whip" },
                                   { "displayName", "Whip" } },
                                 { { "checkGame", "oot" },
                                   { "checkName", "junk" },
                                   { "itemGame", "mm" },
                                   { "itemName", "Arrows" },
                                   { "displayName", "Arrows" } } });
    auto enriched = BuildForeignArray(foreign
#ifndef COMBO_SHARED_BASELINE
                                      ,
                                      0, { { "Whip", "Whip" } }
#endif
    );
    check(enriched[0]["displayName"] == "Whip" && enriched[0].value("shared", false),
          "NEI foreign pickup is untagged and shared");
    check(enriched[1]["displayName"] == "Arrows (MM)" && !enriched[1].value("shared", false),
          "unshared foreign junk keeps origin");
#ifndef COMBO_SHARED_BASELINE
    auto names = SharedUntaggedNames(1u << SF_GORON_MASK, { { "Goron Mask (MM)", "Goron Mask" } });
    json op = { { "alias", "Goron Mask (MM)" }, { "native", "Goron Mask" } }, mp = { { "native", "Goron Mask" } };
    SuffixCrossGameItems(op, mp, json::array(), o.dump(), m.dump(), names);
    check(op["alias"] == "Goron Mask (MM)" && op["native"] == "Goron Mask" && mp["native"] == "Goron Mask",
          "both shared naming systems exempt native suffixes");
    ForeignItem fi;
    fi.shared = true;
    fi.itemGame = GAME_OOT;
    check(ShownForeignName(fi, "Whip") == "Whip", "live resolved shared names remain untagged");
#endif
}
#ifndef COMBO_SHARED_BASELINE
static int countPlaced(const CombinedFillResult& r, GameId game, const std::string& name) {
    int count = 0;
    for (const auto& p : ParseSpoilerPlacements(r.spoilerJson))
        if (p.itemGame == game && p.item == name)
            ++count;
    return count;
}
static CombinedFillResult fill(json o, json m, uint32_t mask, const std::string& pairs,
                               const std::string& forced = "") {
    configure(o, m);
    return CrossWorldCombinedFill(o.dump(), m.dump(), 42, oot, mm, nullptr, forced, OotAccess::ALL_REACHABLE, {},
                                  GAME_OOT, pairs, mask);
}
static void fillAndCredit() {
    auto pairs = pairsJson({ { "Progressive Bow", "Progressive Bow" }, { "Progressive Bow", "Progressive Bow" } });
    json o = emptyDump(), m = emptyDump();
    add(o, "Progressive Bow", 3);
    add(o, "Junk", 1, false);
    add(m, "Progressive Bow", 4);
    add(m, "Junk", 1, false);
    auto r = fill(o, m, 1u << SF_BOW, pairs);
    check(r.success, "overlapping family/NEI fill succeeds");
    if (r.success) {
        check(countPlaced(r, GAME_OOT, "Progressive Bow") == 3 && countPlaced(r, GAME_MM, "Progressive Bow") == 1,
              "overlap retains NEI's larger tier count exactly once");
        check(SharedMaskFromKeys(json::parse(r.spoilerJson)["sharedItems"]) == (1u << SF_BOW),
              "overlap still records the effective family");
    }
    o = emptyDump();
    m = emptyDump();
    add(o, "Goron Mask", 1);
    add(o, "Goron Mask (MM)", 1);
    add(o, "Junk", 1, false);
    add(m, "Goron Mask", 1);
    add(m, "Junk", 1, false);
    pairs = pairsJson({ { "Goron Mask (MM)", "Goron Mask" } });
    r = fill(o, m, 1u << SF_GORON_MASK, pairs);
    check(r.success, "native/imported mask fill succeeds");
    if (r.success)
        check(countPlaced(r, GAME_OOT, "Goron Mask") == 1 && countPlaced(r, GAME_OOT, "Goron Mask (MM)") == 0 &&
                  countPlaced(r, GAME_MM, "Goron Mask") == 0,
              "three shared mask representations retain one physical copy");
    o["accessibility"]["maskQuestShuffle"] = false;
    r = fill(o, m, 1u << SF_GORON_MASK, pairs);
    check(r.success, "mask shuffle disabled fill succeeds");
    if (r.success)
        check(json::parse(r.spoilerJson)["sharedItems"].empty() && countPlaced(r, GAME_OOT, "Goron Mask") == 1 &&
                  countPlaced(r, GAME_OOT, "Goron Mask (MM)") == 1,
              "disabled mask family preserves distinct native and NEI masks");
    o = emptyDump();
    m = emptyDump();
    add(o, "Fire Arrows", 1);
    add(o, "Junk", 1, false);
    add(m, "Fire Arrows", 1);
    add(m, "Junk", 1, false);
    r = fill(o, m, 1u << SF_FIRE_ARROWS, "", R"({"Link's Pocket":{"item":"Fire Arrows"}})");
    check(r.success && SharedMaskFromKeys(json::parse(r.spoilerJson)["sharedItems"]) == (1u << SF_FIRE_ARROWS),
          "forced starting copy activates the family before pool reservation");
    o = emptyDump();
    m = emptyDump();
    add(o, "Junk", 1, false);
    add(m, "Fire Arrows", 1);
    add(m, "Junk", 1, false);
    r = fill(o, m, 1u << SF_FIRE_ARROWS, "");
    check(r.success && json::parse(r.spoilerJson)["sharedItems"].empty() && countPlaced(r, GAME_MM, "Fire Arrows") == 1,
          "ineffective family preserves MM pool");
    o = emptyDump();
    m = emptyDump();
    add(o, "Junk", 1, false);
    add(m, "Whip", 1);
    add(m, "Junk", 1, false);
    o["fixed"] = json::array({ { { "check", "Fixed Whip" }, { "item", "Whip" }, { "advancement", true } } });
    r = fill(o, m, 0, pairsJson({ { "Whip", "Whip" } }));
    check(r.success && countPlaced(r, GAME_MM, "Whip") == 1,
          "NEI-only sharing preserves its existing pool-only dedup when OOT item is fixed");
    auto groups = BuildSharedItemGroups({ { "Progressive Bow", "Progressive Bow" } }, 1u << SF_BOW);
    std::vector<std::string> ov, mv;
    SharedOwnedViews(groups, { "Progressive Bow" }, {}, ov, mv);
    check(ov.size() == 1 && mv.size() == 1, "one shared bow never credits tier two");
    SharedOwnedViews(groups, { "Progressive Bow", "Progressive Bow" }, { "Progressive Bow" }, ov, mv);
    check(ov.size() == 3 && mv.size() == 3, "independent shared progressive pickups accumulate once each");
    groups = BuildSharedItemGroups({ { "Goron Mask (MM)", "Goron Mask" } }, 1u << SF_GORON_MASK);
    SharedOwnedViews(groups, { "Goron Mask" }, {}, ov, mv);
    check(std::count(ov.begin(), ov.end(), "Goron Mask (MM)") == 1 && mv.size() == 1,
          "native mask credits imported alias via shared component");
    groups = BuildSharedItemGroups({}, 1u << SF_HOOKSHOT);
    SharedOwnedViews(groups, { "Progressive Hookshot", "Progressive Hookshot" }, {}, ov, mv);
    check(mv.size() == 1, "family-only MM hookshot mirror honors its tier cap");
    auto unresolved = ResolveSharedPairs(R"([{"oot":"Unknown","mm":"RI_MISSING"}])", emptyDump().dump());
    check(unresolved.empty(), "unresolved RI token cannot become a false shared edge");
}
static void ganondorfHint() {
    json o = emptyDump(), m = emptyDump();
    add(o, "Progressive Master Sword", 1);
    add(o, "Light Arrows", 1);
    add(m, "Junk", 1, false);
    json seed = { { "oot", { { "Check 0", "Progressive Master Sword" }, { "Check 1", "Light Arrows" } } },
                  { "mm", json::object() },
                  { "foreign", json::array() } };
    json hints = { { "options", { { "ganondorfHint", 1 }, { "shuffleMasterSword", 1 } } },
                   { "checks", json::array({ { { "name", "Check 0" }, { "area", "Sword Area" } },
                                             { { "name", "Check 1" }, { "area", "Arrow Area" } } }) },
                   { "hintTextTable", json::object() } };
    for (const char* key :
         { "RHT_GANONDORF_HINT_LA_ONLY", "RHT_GANONDORF_HINT_MS_ONLY", "RHT_GANONDORF_HINT_LA_AND_MS" })
        hints["hintTextTable"][key] = {
            { "clear", { { "en", "arrows [[1]] sword [[2]]" }, { "de", "[[1]] [[2]]" }, { "fr", "[[1]] [[2]]" } } }
        };
    auto generated = Generate(42, o.dump(), hints.dump(), m.dump(), json::array(), seed.dump(), {});
    bool found = false;
    for (const auto& hint : generated["oot"]) {
        if (hint.value("checkName", "") != kGanondorfHintKey)
            continue;
        found = true;
        check(hint["messages"].size() == 3, "progressive Master Sword retains all Ganondorf message indices");
        check(hint["messages"][1]["en"].get<std::string>().find("Sword Area") != std::string::npos,
              "Ganondorf sword clause resolves the progressive chain location");
    }
    check(found, "Ganondorf hint survives NEI progressive Master Sword");
}
#endif
int main() {
    forwardAndRequiredness();
    parsedNamesAndDisplay();
#ifndef COMBO_SHARED_BASELINE
    fillAndCredit();
    ganondorfHint();
#endif
    if (failures) {
        std::cerr << failures << " combo shared checks failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "combo shared production behavior checks passed\n";
}
