#pragma once
#include "BlackMarketEntry.h"
#include "BlackMarketNativeResources.h"
#include "../../mm/2s2h/Enhancements/Gameplay/BlackMarketResources.h"

inline bool destination = true;
inline std::vector<nlohmann::json> diagnostics;
inline bool DestinationAvailable() { return destination; }
inline void CaptureEntryLog(const char*, const std::string& payload) {
    diagnostics.push_back(nlohmann::json::parse(payload));
}
#define SPDLOG_INFO(pattern, payload) CaptureEntryLog(pattern, payload)
void LogDoorAttempt(PlayState*, Actor*, int, const char*);
void LogEntryRecovery();
