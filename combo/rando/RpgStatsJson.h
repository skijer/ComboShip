#pragma once

#include "RpgStats.h"
#include <algorithm>
#include <nlohmann/json.hpp>

namespace ComboRpg {
inline constexpr const char* Names[COMBO_RPG_COUNT] = {
    "defense", "speed", "power", "magic", "crawl", "climb", "push"
};

inline int ReadInteger(const nlohmann::json& value, int fallback, int low, int high) {
    if (!value.is_number_integer())
        return fallback;
    return static_cast<int>(std::clamp(value.get<double>(), static_cast<double>(low), static_cast<double>(high)));
}

inline nlohmann::json ToJson(const ComboRpgState& state) {
    nlohmann::json out = { { "magicTotal", state.magicTotal == 100 ? 100 : 96 },
                           { "quarterHearts", state.quarterHeartsEnabled != 0 },
                           { "nativeMagicLevel", state.nativeMagicLevel } };
    for (int i = 0; i < COMBO_RPG_COUNT; ++i) {
        auto& stat = out[Names[i]];
        stat["level"] = state.level[i];
        if (state.knownMask & (1 << i))
            stat["enabled"] = (state.enabledMask & (1 << i)) != 0;
        if (state.required[i])
            stat["required"] = state.required[i];
    }
    return out;
}

// Wire messages can contain one flattened leaf. Read rules before levels and
// preserve absent members; do not truncate an early count before its cap arrives.
// OoT passes acceptRules=false: only the saved seed may author its rules.
inline void MergeJson(ComboRpgState& state, const nlohmann::json& in, bool acceptRules = true) {
    if (!in.is_object())
        return;
    auto nativeMagic = in.find("nativeMagicLevel");
    if (nativeMagic != in.end())
        state.nativeMagicLevel = std::max<int>(state.nativeMagicLevel, ReadInteger(*nativeMagic, 0, 0, 2));
    if (acceptRules) {
        auto total = in.find("magicTotal");
        if (total != in.end() && total->is_number_integer())
            state.magicTotal = ReadInteger(*total, 96, 96, 100) == 100 ? 100 : 96;
        auto quarter = in.find("quarterHearts");
        if (quarter != in.end() && quarter->is_boolean())
            state.quarterHeartsEnabled = quarter->get<bool>();
    }
    for (int i = 0; i < COMBO_RPG_COUNT; ++i) {
        auto row = in.find(Names[i]);
        if (row == in.end() || !row->is_object())
            continue;
        if (acceptRules) {
            auto enabled = row->find("enabled");
            if (enabled != row->end() && enabled->is_boolean()) {
                state.knownMask |= 1 << i;
                if (enabled->get<bool>())
                    state.enabledMask |= 1 << i;
                else
                    state.enabledMask &= ~(1 << i);
            }
            auto required = row->find("required");
            if (required != row->end() && required->is_number_integer())
                state.required[i] = ReadInteger(*required, state.required[i], 1, 100);
        }
        auto level = row->find("level");
        if (level != row->end())
            state.level[i] = std::max<int>(state.level[i], ReadInteger(*level, state.level[i], 0, 100));
        if (state.required[i])
            state.level[i] = std::min(state.level[i], state.required[i]);
    }
}

inline void MigrateLegacySpeed(ComboRpgState& state, uint8_t level, uint8_t required) {
    if ((state.knownMask & (1 << COMBO_RPG_SPEED)) || level == 0)
        return;
    state.knownMask |= 1 << COMBO_RPG_SPEED;
    state.enabledMask |= 1 << COMBO_RPG_SPEED;
    state.required[COMBO_RPG_SPEED] = required ? std::min<int>(required, 100) : 5;
    state.level[COMBO_RPG_SPEED] = std::min(level, state.required[COMBO_RPG_SPEED]);
}
} // namespace ComboRpg
