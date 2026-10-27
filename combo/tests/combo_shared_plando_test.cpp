#include "rando/ComboPlaythrough.h"
#include <iostream>
#include <stdexcept>
using json = nlohmann::json;
static std::string pairData;
static const char* dumpPairs() {
    return pairData.c_str();
}
using FnDump = const char* (*)();
static FnDump sSharedPairs = dumpPairs;
#include "combo_shared_plando_functions.h"
static void check(bool ok, const char* what) {
    if (!ok)
        throw std::runtime_error(what);
}
int main() {
    pairData =
        R"json([{"oot":"Goron Mask (MM)","mm":"RI_MASK_GORON"},{"oot":"Progressive Magic Meter","mm":"RI_MAGIC"}])json";
    sPlando.sohDump =
        R"json({"items":[{"name":"Goron Mask (MM)"},{"name":"Goron Mask"},{"name":"Progressive Magic Meter"},{"name":"Unique OOT"}]})json";
    sPlando.mmDump =
        R"json({"items":[{"name":"Goron Mask","token":"RI_MASK_GORON"},{"name":"Progressive Magic","token":"RI_MAGIC"},{"name":"Unique MM"}]})json";
    sPlando.loadedJson = R"json({"sharedItems":["goronMask","magic"]})json";
    PlandoBuildItems();
    check(sPlando.items.size() == 4,
          "picker contains one entry per shared component, including differently named NEI aliases");
    bool goron = false, magic = false, local = false;
    for (const auto& item : sPlando.items) {
        if (item.name == "Goron Mask")
            goron = item.game == ComboRando::GAME_OOT && item.display == "Goron Mask";
        if (item.name == "Progressive Magic Meter")
            magic = item.display == "Progressive Magic Meter";
        if (item.name == "Unique MM")
            local = item.display == "Unique MM (MM)";
        check(item.name != "Goron Mask (MM)", "enabled mask picker selects native OOT representation");
    }
    check(goron && magic && local, "picker canonical identity and shared/local tags match serialization");
    sPlando.loadedJson = R"json({"sharedItems":[]})json";
    PlandoBuildItems();
    check(sPlando.items.size() == 5, "disabled native mask remains separate from existing NEI mask pair");
    std::cout << "combo shared production plando picker checks passed\n";
}
