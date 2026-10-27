#ifdef COMBO_BUILD
#include "global.h"
#include "mods/nei_save.h"
#include "ComboExport.h"
#include "gameplay/ComboFaroresWind.h"

extern "C" int32_t Combo_MM_SaveCrossDungeonCheckpoint(void);
extern "C" void Combo_MM_RestoreCrossDungeonCheckpoint(int32_t index);

static ComboFwCallbacks sFwCallbacks{};
static ComboFwPoint sFwArrival{};
static int sFwArrivalSlot = -1;
static bool sFwArrivalAnimation = false;
static bool sFwValidateRoom = false;
extern "C" void MM_QueueFwHandoff(void);
extern "C" {
extern s32 gSceneEntranceTableCount;
extern SceneEntranceTableEntry sSceneEntranceTable[];
}

static bool NativePointValid(const ComboFwPoint* p) {
    if (!ComboFw_PointValid(p) || p->game != 1 || (p->entrance & 0xF) != 0)
        return false;
    auto scene = p->entrance >> 9;
    auto spawn = (p->entrance >> 4) & 0x1F;
    return scene < gSceneEntranceTableCount && sSceneEntranceTable[scene].table &&
           spawn < sSceneEntranceTable[scene].tableCount && sSceneEntranceTable[scene].table[spawn];
}

extern "C" void MM_CaptureFwDeparture(void) {
    if (!gPlayState)
        return;
    Play_SaveCycleSceneFlags(gPlayState);
    // SaveCurrentForCombo writes Save, and title_setup reconstructs cycle flags from this array.
    // Copy the same five fields as MM's native owl-save writer; keep permanent-only flags intact.
    for (size_t i = 0; i < ARRAY_COUNT(gSaveContext.cycleSceneFlags); ++i) {
        const auto& cycle = gSaveContext.cycleSceneFlags[i];
        auto& saved = gSaveContext.save.saveInfo.permanentSceneFlags[i];
        saved.chest = cycle.chest;
        saved.switch0 = cycle.switch0;
        saved.switch1 = cycle.switch1;
        saved.clearedRoom = cycle.clearedRoom;
        saved.collectible = cycle.collectible;
    }
    gSaveContext.save.saveInfo.playerData.savedSceneId = gPlayState->sceneId;
}

extern "C" COMBO_EXPORT void MM_SetFwCallbacks(const ComboFwCallbacks* callbacks) {
    sFwCallbacks = callbacks ? *callbacks : ComboFwCallbacks{};
}

static void PublishNativePoint(bool captureCheckpoint) {
    const auto* fw = Nei_Save();
    if (!sFwCallbacks.write || !fw->fwSet)
        return;
    ComboFwPoint p{ 1,
                    fw->fwEntrance,
                    fw->fwRoomIndex,
                    fw->fwYaw,
                    -1,
                    fw->fwPosX,
                    fw->fwPosY,
                    fw->fwPosZ,
                    fw->fwTempSwitchFlags,
                    fw->fwTempCollectFlags,
                    gSaveContext.respawn[RESPAWN_MODE_DOWN].unk_18,
                    captureCheckpoint ? Combo_MM_SaveCrossDungeonCheckpoint() : -1 };
    sFwCallbacks.write(gSaveContext.fileNum, &p);
}

extern "C" void ComboFw_PublishPoint(void) {
    PublishNativePoint(true);
}

static int ReadPoint(ComboFwPoint* p) {
    if (!sFwCallbacks.read)
        return -1;
    int result = sFwCallbacks.read(gSaveContext.fileNum, p);
    if (result < 0 && gSaveContext.fileNum >= 0 && gSaveContext.fileNum <= 2 && sFwCallbacks.write) {
        if (Nei_Save()->fwSet) {
            PublishNativePoint(false); // A legacy point has no reliable cast-time checkpoint.
            result = sFwCallbacks.read(gSaveContext.fileNum, p);
        } else {
            return 0; // Leave metadata absent so the other game's legacy point can still migrate.
        }
    }
    if (result == 1 && p->game == 1 && !NativePointValid(p))
        return 0;
    return result;
}

static void AdoptNativePoint(const ComboFwPoint& p) {
    auto* fw = Nei_Save();
    fw->fwSet = 1;
    fw->fwPosX = p.x;
    fw->fwPosY = p.y;
    fw->fwPosZ = p.z;
    fw->fwYaw = (int16_t)p.yaw;
    fw->fwPlayerParams = PLAYER_PARAMS(0xFF, PLAYER_START_MODE_D);
    fw->fwEntrance = (uint16_t)p.entrance;
    fw->fwRoomIndex = (uint8_t)p.room;
    fw->fwTempSwitchFlags = p.tempSwitchFlags;
    fw->fwTempCollectFlags = p.tempCollectFlags;
}

extern "C" int ComboFw_HasPoint(int nativeSet) {
    ComboFwPoint p{};
    int result = ReadPoint(&p);
    return result < 0 ? nativeSet : result == 1;
}

extern "C" void ComboFw_SyncPoint(void) {
    ComboFwPoint p{};
    int result = ReadPoint(&p);
    if (result == 1 && p.game == 1)
        AdoptNativePoint(p);
    else if (result >= 0)
        Nei_Save()->fwSet = 0;
}

extern "C" void ComboFw_ClearPoint(void) {
    if (sFwCallbacks.write)
        sFwCallbacks.write(gSaveContext.fileNum, nullptr);
}

extern "C" int ComboFw_RequestReturn(void) {
    ComboFwPoint p{};
    int result = ReadPoint(&p);
    if (result < 0)
        return 0;
    if (result != 1)
        return -1;
    if (p.game == 1) {
        AdoptNativePoint(p);
        Combo_MM_RestoreCrossDungeonCheckpoint(p.dungeonCheckpoint);
        sFwValidateRoom = true;
        return 0;
    }
    if (!sFwCallbacks.requestReturn || !sFwCallbacks.requestReturn(1, gSaveContext.fileNum))
        return -1;
    MM_QueueFwHandoff();
    return 1;
}

extern "C" void ComboFw_ClearLocalPoint(void) {
    ComboFwPoint p{};
    int result = ReadPoint(&p);
    if ((result == 1 && p.game == 1) || (result < 0 && Nei_Save()->fwSet)) {
        ComboFw_ClearPoint();
        Nei_Save()->fwSet = 0;
    }
    sFwArrivalSlot = -1;
    sFwArrivalAnimation = false;
    sFwValidateRoom = false;
}

extern "C" void ComboFw_ValidateArrivalRoom(int roomCount) {
    if (!sFwValidateRoom)
        return;
    sFwValidateRoom = false;
    if (gSaveContext.respawn[RESPAWN_MODE_TOP].roomIndex >= roomCount) {
        ComboFw_ClearPoint();
        Nei_Save()->fwSet = 0;
        gSaveContext.respawn[RESPAWN_MODE_TOP].data = 0;
        gSaveContext.respawnFlag = 0;
        sFwArrivalAnimation = false;
    }
}

extern "C" COMBO_EXPORT int MM_SetFwArrival(int slot, const ComboFwPoint* p) {
    if (slot < 0 || slot > 2 || !NativePointValid(p))
        return 0;
    sFwArrival = *p;
    sFwArrivalSlot = slot;
    sFwArrivalAnimation = false;
    return 1;
}

extern "C" int ComboFw_ApplyArrival(void) {
    int slot = sFwArrivalSlot;
    sFwArrivalSlot = -1;
    if (slot < 0 || slot != gSaveContext.fileNum)
        return 0;
    AdoptNativePoint(sFwArrival);
    Combo_MM_RestoreCrossDungeonCheckpoint(sFwArrival.dungeonCheckpoint);
    auto& top = gSaveContext.respawn[RESPAWN_MODE_TOP];
    top.pos = { sFwArrival.x, sFwArrival.y, sFwArrival.z };
    top.yaw = (int16_t)sFwArrival.yaw;
    top.playerParams = PLAYER_PARAMS(0xFF, PLAYER_START_MODE_D);
    top.entrance = (uint16_t)sFwArrival.entrance;
    top.roomIndex = (uint8_t)sFwArrival.room;
    top.data = 1;
    top.tempSwitchFlags = sFwArrival.tempSwitchFlags;
    top.unk_18 = sFwArrival.tempCollectFlagsLow;
    top.tempCollectFlags = sFwArrival.tempCollectFlags;
    gSaveContext.save.entrance = sFwArrival.entrance;
    gSaveContext.save.cutsceneIndex = 0;
    gSaveContext.respawnFlag = 3;
    sFwArrivalAnimation = true;
    sFwValidateRoom = true;
    return 1;
}

extern "C" int ComboFw_ConsumeArrivalAnimation(void) {
    bool pending = sFwArrivalAnimation;
    sFwArrivalAnimation = false;
    return pending;
}
#endif
