#include "gameplay/ComboFaroresWindJson.h"
#include <cassert>
#include <iostream>

int main() {
    ComboFwPoint original{1, 0x1230, 3, -300, -1, 111.5f, 222, -333, 0xFFFFFFFF, 0xDEADBEEF, 0x12345678};
    assert(ComboFw_PointValid(&original));
    auto encoded = ComboFw_EncodePoint(original);
    ComboFwPoint restored{};
    assert(ComboFw_DecodePoint(nlohmann::json::parse(encoded.dump()), restored));
    assert(restored.game == 1 && restored.room == 3 && restored.x == 111.5f);
    assert(restored.tempSwitchFlags == 0xFFFFFFFF && restored.tempCollectFlags == 0xDEADBEEF);
    assert(restored.tempCollectFlagsLow == 0x12345678);
    // A recalled point restores its original dungeon rather than the current one.
    auto checkpointPoint = encoded;
    checkpointPoint["dungeonCheckpoint"] = 0x1230;
    assert(ComboFw_DecodePoint(checkpointPoint, restored));
    auto checkpointRoundtrip = ComboFw_EncodePoint(restored);
    assert(checkpointRoundtrip.contains("dungeonCheckpoint"));
    assert(checkpointRoundtrip["dungeonCheckpoint"] == 0x1230);
    for (int checkpoint : {-1, 0, 65535}) {
        checkpointPoint["dungeonCheckpoint"] = checkpoint;
        assert(ComboFw_DecodePoint(checkpointPoint, restored));
        assert(ComboFw_EncodePoint(restored)["dungeonCheckpoint"] == checkpoint);
    }
    checkpointPoint.erase("dungeonCheckpoint");
    assert(ComboFw_DecodePoint(checkpointPoint, restored));
    assert(ComboFw_EncodePoint(restored)["dungeonCheckpoint"] == -1);
    for (auto value : {nlohmann::json(nullptr), nlohmann::json("1"), nlohmann::json(1.0),
                       nlohmann::json(true), nlohmann::json(-2), nlohmann::json(65536),
                       nlohmann::json(UINT64_MAX)}) {
        checkpointPoint["dungeonCheckpoint"] = value;
        auto before = ComboFw_EncodePoint(restored);
        assert(!ComboFw_DecodePoint(checkpointPoint, restored));
        assert(ComboFw_EncodePoint(restored) == before);
    }
    for (auto key : {"game", "entrance", "room", "yaw", "age"}) {
        auto invalid = encoded;
        invalid[key] = uint64_t{0x100000001};
        assert(!ComboFw_DecodePoint(invalid, restored));
        invalid[key] = "1";
        assert(!ComboFw_DecodePoint(invalid, restored));
    }
    auto invalid = encoded;
    invalid["game"] = 99;
    assert(!ComboFw_DecodePoint(invalid, restored));
    invalid = encoded;
    invalid["tempSwitchFlags"] = -1;
    assert(!ComboFw_DecodePoint(invalid, restored));
    invalid = encoded;
    invalid["x"] = 1e100;
    assert(!ComboFw_DecodePoint(invalid, restored));
    assert(!ComboFw_DecodePoint(nullptr, restored));
    assert(!ComboFw_DecodePoint(nlohmann::json::object(), restored));
    std::cout << "save point JSON: roundtrip, MM flag words, unset/old/malformed records passed\n";
}
