#include "../../combo/ComboGameMode.h"
#ifdef COMBO_BUILD
#include "global.h"
#include "ComboExport.h"
#include "gameplay/ComboFaroresWind.h"

extern "C" int32_t Combo_SOH_SaveCrossDungeonCheckpoint(void);
extern "C" void Combo_SOH_RestoreCrossDungeonCheckpoint(int32_t index);

static ComboFwCallbacks sFwCallbacks{};
static ComboFwPoint sFwArrival{};
static int sFwArrivalSlot = -1;
static bool sFwValidateRoom = false;
extern "C" void SOH_QueueFwHandoff(int slot);

static bool NativePointValid(const ComboFwPoint* p) {
    // Native entrance selection adds age/night offsets (0..3) before indexing the table.
    if (!ComboFw_PointValid(p) || p->game != 0 || p->entrance + 3 >= ENTR_MAX)
        return false;
    for (int layer = 0; layer < 4; ++layer) {
        int scene = gEntranceTable[p->entrance + layer].scene;
        if (scene < 0 || scene >= SCENE_ID_MAX)
            return false;
    }
    return true;
}

extern "C" COMBO_EXPORT void SOH_SetFwCallbacks(const ComboFwCallbacks* callbacks) {
    sFwCallbacks = callbacks ? *callbacks : ComboFwCallbacks{};
}

static void PublishNativePoint(bool captureCheckpoint) {
    if (!Combo_IsCombinedGame() || !sFwCallbacks.write || !gSaveContext.fw.set)
        return;
    const auto& fw = gSaveContext.fw;
    ComboFwPoint p{ 0,
                    fw.entranceIndex,
                    fw.roomIndex,
                    fw.yaw,
                    (int32_t)gSaveContext.linkAge,
                    (float)fw.pos.x,
                    (float)fw.pos.y,
                    (float)fw.pos.z,
                    (uint32_t)fw.tempSwchFlags,
                    (uint32_t)fw.tempCollectFlags,
                    0,
                    captureCheckpoint ? Combo_SOH_SaveCrossDungeonCheckpoint() : -1 };
    sFwCallbacks.write(gSaveContext.fileNum, &p);
}

extern "C" void ComboFw_PublishPoint(void) {
    PublishNativePoint(true);
}

static int ReadPoint(ComboFwPoint* p) {
    if (!Combo_IsCombinedGame() || !sFwCallbacks.read)
        return -1;
    int result = sFwCallbacks.read(gSaveContext.fileNum, p);
    if (result < 0 && gSaveContext.fileNum <= 2 && sFwCallbacks.write) {
        if (gSaveContext.fw.set) {
            PublishNativePoint(false); // A legacy point has no reliable cast-time checkpoint.
            result = sFwCallbacks.read(gSaveContext.fileNum, p);
        } else {
            return 0; // Leave metadata absent so the other game's legacy point can still migrate.
        }
    }
    if (result == 1 && p->game == 0 && !NativePointValid(p))
        return 0;
    return result;
}

static void AdoptNativePoint(const ComboFwPoint& p) {
    auto& fw = gSaveContext.fw;
    fw.set = 1;
    fw.pos.x = (int32_t)p.x;
    fw.pos.y = (int32_t)p.y;
    fw.pos.z = (int32_t)p.z;
    fw.yaw = p.yaw;
    fw.playerParams = 0x6FF;
    fw.entranceIndex = p.entrance;
    fw.roomIndex = p.room;
    fw.tempSwchFlags = p.tempSwitchFlags;
    fw.tempCollectFlags = p.tempCollectFlags;
}

extern "C" int ComboFw_HasPoint(int nativeSet) {
    ComboFwPoint p{};
    int result = ReadPoint(&p);
    return result < 0 ? nativeSet : result == 1;
}

extern "C" void ComboFw_SyncPoint(void) {
    ComboFwPoint p{};
    int result = ReadPoint(&p);
    if (result == 1 && p.game == 0)
        AdoptNativePoint(p);
    else if (result >= 0)
        gSaveContext.fw.set = 0;
}

extern "C" void ComboFw_ClearPoint(void) {
    if (Combo_IsCombinedGame() && sFwCallbacks.write)
        sFwCallbacks.write(gSaveContext.fileNum, nullptr);
}

static void PrepareNativeReturn(const ComboFwPoint& p) {
    AdoptNativePoint(p);
    Combo_SOH_RestoreCrossDungeonCheckpoint(p.dungeonCheckpoint);
    auto& top = gSaveContext.respawn[RESPAWN_MODE_TOP];
    top.pos = { p.x, p.y, p.z };
    top.yaw = (int16_t)p.yaw;
    top.playerParams = 0x6FF;
    top.entranceIndex = (int16_t)p.entrance;
    top.roomIndex = (uint8_t)p.room;
    top.tempSwchFlags = p.tempSwitchFlags;
    top.tempCollectFlags = p.tempCollectFlags;
    top.data = 40;
    sFwValidateRoom = true;
}

extern "C" void ComboFw_ValidateArrivalRoom(int roomCount) {
    if (!sFwValidateRoom)
        return;
    sFwValidateRoom = false;
    if (gSaveContext.respawn[RESPAWN_MODE_TOP].roomIndex >= roomCount) {
        ComboFw_ClearPoint();
        gSaveContext.fw.set = 0;
        gSaveContext.respawn[RESPAWN_MODE_TOP].data = 0;
        gSaveContext.respawnFlag = 0;
    }
}

extern "C" void ComboFw_ClearLocalPoint(void) {
    ComboFwPoint p{};
    int result = ReadPoint(&p);
    if ((result == 1 && p.game == 0) || (result < 0 && gSaveContext.fw.set)) {
        ComboFw_ClearPoint();
        gSaveContext.fw.set = 0;
    }
    sFwArrivalSlot = -1;
    sFwValidateRoom = false;
}

extern "C" int ComboFw_RequestReturn(void) {
    ComboFwPoint p{};
    int result = ReadPoint(&p);
    if (result < 0)
        return 0; // Standalone/older launcher: preserve the native path.
    if (result != 1)
        return -1;
    if (p.game == 0) {
        PrepareNativeReturn(p);
        return 0;
    }
    if (!sFwCallbacks.requestReturn || !sFwCallbacks.requestReturn(0, gSaveContext.fileNum))
        return -1;
    SOH_QueueFwHandoff(gSaveContext.fileNum);
    return 1;
}

extern "C" COMBO_EXPORT int SOH_SetFwArrival(int slot, const ComboFwPoint* p) {
    if (slot < 0 || slot > 2 || !NativePointValid(p))
        return 0;
    sFwArrival = *p;
    sFwArrivalSlot = slot;
    return 1;
}

extern "C" int ComboFw_ApplyArrival(void) {
    int slot = sFwArrivalSlot;
    sFwArrivalSlot = -1;
    if (slot < 0 || slot != gSaveContext.fileNum)
        return 0;
    PrepareNativeReturn(sFwArrival);
    gSaveContext.entranceIndex = sFwArrival.entrance;
    gSaveContext.respawnFlag = 3;
    gSaveContext.cutsceneIndex = 0;
    return 1;
}

extern "C" int ComboFw_ConsumeArrivalAnimation(void) {
    return 0;
} // Native OOT start mode handles it.
#endif
