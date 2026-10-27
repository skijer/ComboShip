#include <cassert>
#include <cstdio>

static void ExpectSelections(const json& saved, const json& values, unsigned mask) {
    assert(saved.contains("swordElements") && "element selections must be serialized");
    assert(saved.at("swordElements").is_array());
    assert(saved.at("swordElements").size() == 8);
    assert(saved.at("swordElements") == values);
    assert(saved.at("swordElementSet") == mask);
}

int main() {
    const json all = { 0, 1, 2, 3, 3, 2, 1, 0 };
    const json none = { 0, 0, 0, 0, 0, 0, 0, 0 };
    // JSON text parse and dump exercise the actual persisted wire format.
    Load(json::parse(json{{"swordElements", all}, {"swordElementSet", 255}}.dump()));
    const auto fileOne = json::parse(Persist().dump());
    ExpectSelections(fileOne, all, 255);
    Load(fileOne);
    ExpectSelections(Persist(), all, 255);

    // Legacy files have no explicit choices; loading another file must clear prior selections.
    Load(json::object());
    ExpectSelections(Persist(), none, 0);
    Load({{"swordElements", {3, 2}}, {"swordElementSet", 511}});
    ExpectSelections(Persist(), json({3, 2, 0, 0, 0, 0, 0, 0}), 255);
    Load({{"swordElements", {0, 1, -1, 4, 256, "fire", nullptr, true, 3}},
          {"swordElementSet", 257}});
    ExpectSelections(Persist(), json({0, 1, 0, 0, 0, 0, 0, 0}), 1);
    for (const auto& malformed : {json(nullptr), json("ice"), json(2), json::object()}) {
        Load({{"swordElements", malformed}, {"swordElementSet", malformed}});
        ExpectSelections(Persist(), none, malformed == json(2) ? 2 : 0);
    }
    for (const auto& mask : {json(-1), json(2.5), json(true), json(18446744073709551615ull)}) {
        Load({{"swordElementSet", mask}});
        ExpectSelections(Persist(), none, mask.is_number_unsigned() ? 255 : 0);
    }

    Load(fileOne);
    Init();
    ExpectSelections(Persist(), none, 0);
    Load(fileOne);
    ExpectSelections(Persist(), all, 255);
    std::puts("PASS sword save: eight-slot JSON roundtrip, explicit None, legacy defaults, malformed/short data, file and new-game reset");
}
