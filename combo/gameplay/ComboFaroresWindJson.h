#pragma once
#include "ComboFaroresWind.h"
#include <nlohmann/json.hpp>
#include <stdexcept>

inline nlohmann::json ComboFw_EncodePoint(const ComboFwPoint& p) {
    return { { "version", 1 },
             { "game", p.game },
             { "entrance", p.entrance },
             { "room", p.room },
             { "yaw", p.yaw },
             { "age", p.age },
             { "x", p.x },
             { "y", p.y },
             { "z", p.z },
             { "tempSwitchFlags", p.tempSwitchFlags },
             { "tempCollectFlags", p.tempCollectFlags },
             { "tempCollectFlagsLow", p.tempCollectFlagsLow },
             { "dungeonCheckpoint", p.dungeonCheckpoint } };
}

inline bool ComboFw_DecodePoint(const nlohmann::json& j, ComboFwPoint& p) {
    try {
        if (!j.is_object() || j.at("version") != 1)
            return false;
        auto integer = [&](const char* key) -> int32_t {
            const auto& value = j.at(key);
            if (!value.is_number_integer())
                throw std::runtime_error("noninteger Farore field");
            // Read through double for range checks before narrowing, including unsigned JSON integers.
            double n = value.get<double>();
            if (n < INT32_MIN || n > INT32_MAX)
                throw std::runtime_error("Farore field overflow");
            return value.get<int32_t>();
        };
        auto flags = [&](const char* key) -> uint32_t {
            const auto& value = j.at(key);
            if (!value.is_number_integer() || value.get<double>() < 0 || value.get<double>() > UINT32_MAX)
                throw std::runtime_error("Farore flags overflow");
            return value.get<uint32_t>();
        };
        ComboFwPoint decoded{ integer("game"),
                              integer("entrance"),
                              integer("room"),
                              integer("yaw"),
                              integer("age"),
                              j.at("x").get<float>(),
                              j.at("y").get<float>(),
                              j.at("z").get<float>(),
                              flags("tempSwitchFlags"),
                              flags("tempCollectFlags"),
                              flags("tempCollectFlagsLow"),
                              j.contains("dungeonCheckpoint") ? integer("dungeonCheckpoint") : -1 };
        if (!ComboFw_PointValid(&decoded))
            return false;
        p = decoded;
        return true;
    } catch (...) { return false; }
}
