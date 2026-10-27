#include "gameplay/ComboFaroresWindJson.h"
#include <cassert>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <vector>
#define COMBO_RELEASE_VERSION "0.3.0"
namespace ComboRando { enum { GAME_OOT, GAME_MM }; }
static std::mutex g_containerMutex;
static std::map<int, nlohmann::json> g_containerCache;
static std::vector<int> g_evictedSlots;
static int g_MmSaveInMemorySlot = -1;
static void (*MM_InvalidateOwlBlobSlot)() = nullptr;
static int g_PendingMMFileNum = -1;
static int arrivalGame = -1, arrivalSlot = -1, resumeEntry = 0;
static ComboFwPoint arrival{};
static int setOot(int slot, const ComboFwPoint* p) {
    arrivalGame = 0; arrivalSlot = slot; arrival = *p; return 1;
}
static int setMm(int slot, const ComboFwPoint* p) {
    arrivalGame = 1; arrivalSlot = slot; arrival = *p; return 1;
}
static int (*SOH_SetFwArrival)(int, const ComboFwPoint*) = setOot;
static int (*MM_SetFwArrival)(int, const ComboFwPoint*) = setMm;
static void (*SOH_ResumeGame)(int) = [](int) {};
static void (*MM_ResumeGame)(int) = [](int) {};
static void (*MM_SetComboEntryIsResume)(int) = [](int value) { resumeEntry = value; };

// PRODUCTION_CONTAINER_FUNCTIONS

int main() {
    ComboFwPoint p{1, 0x1230, 3, -300, -1, 111.5f, 222, -333, 55, 77, 123};
    p.dungeonCheckpoint = 0x1230;
    ComboFwPoint read{};
    assert(Combo_ReadFwPoint(1, &read) == -1);
    Combo_WriteGameSave(0, 1, R"({"progress":17})");
    Combo_WriteGameSave(1, 1, R"({"progress":23})");
    assert(Combo_WriteFwPoint(1, &p));
    Combo_WriteGameSave(0, 1, R"({"progress":31})");
    g_containerCache.clear(); // Real disk round-trip, independent of cached objects.
    assert(Combo_ReadFwPoint(1, &read) == 1 && read.game == 1 && read.x == 111.5f);
    assert(read.tempCollectFlagsLow == 123 && read.dungeonCheckpoint == 0x1230);
    assert(nlohmann::json::parse(Combo_ReadGameSave(0, 1))["progress"] == 31);
    assert(nlohmann::json::parse(Combo_ReadGameSave(1, 1))["progress"] == 23);
    assert(Combo_RequestFwReturn(0, 1) && arrivalGame == 1 && arrivalSlot == 1);
    assert(g_PendingMMFileNum == 1 && resumeEntry == 1 && arrival.entrance == p.entrance);
    assert(arrival.dungeonCheckpoint == p.dungeonCheckpoint);
    assert(!Combo_RequestFwReturn(1, 1) && !Combo_RequestFwReturn(99, 1));
    MM_ResumeGame = nullptr;
    arrivalGame = -1;
    assert(!Combo_RequestFwReturn(0, 1) && arrivalGame == -1);
    assert(Combo_ReadFwPoint(1, &read) == 1);
    Combo_CopyContainer(1, 2);
    g_containerCache.clear();
    assert(Combo_ReadFwPoint(2, &read) == 1 && read.room == p.room);
    assert(nlohmann::json::parse(Combo_ReadGameSave(0, 2))["progress"] == 31);
    p.game = 0; p.age = 1; p.entrance = 0x123;
    assert(Combo_WriteFwPoint(1, &p));
    assert(Combo_RequestFwReturn(1, 1) && arrivalGame == 0 && arrivalSlot == 1);
    assert(Combo_ReadFwPoint(2, &read) == 1 && read.game == 1); // Files stay independent.
    assert(Combo_WriteFwPoint(1, nullptr));
    g_containerCache.clear();
    assert(Combo_ReadFwPoint(1, &read) == 0 && !Combo_RequestFwReturn(1, 1));
    assert(Combo_ReadFwPoint(2, &read) == 1);
    assert(!Combo_WriteFwPoint(255, &p) && !Combo_WriteFwPoint(-1, &p));
    assert(!Combo_ReadFwPoint(255, &read));
    assert(!std::filesystem::exists(ComboContainerPath(255)));
    auto before = LoadOrCreateContainer(2);
    p.yaw = 100000;
    assert(!Combo_WriteFwPoint(2, &p) && LoadOrCreateContainer(2) == before);
    LoadOrCreateContainer(1)["combo"]["faroresWind"] = "malformed";
    assert(!Combo_ReadFwPoint(1, &read));
    LoadOrCreateContainer(1)["combo"] = "malformed";
    assert(!Combo_ReadFwPoint(1, &read) && !Combo_WriteFwPoint(1, nullptr));
    std::cout << "container: disk reload, both saves, both handoffs, copy, clear, rejected writes passed\n";
}
