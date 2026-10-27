#pragma once

#ifdef COMBO_BUILD
#include "WoodsMysteryMakeover.h"
#include <initializer_list>

extern "C" {
Gfx* ResourceMgr_LoadGfxByName(const char* path);
void ResourceMgr_PatchGfxByName(const char* path, const char* patchName, int index, Gfx instruction);
void ResourceMgr_UnpatchGfxByName(const char* path, const char* patchName);
size_t ResourceGetSizeByName(const char* path);
uint8_t ResourceGetIsCustomByName(const char* path);
}

namespace WoodsMysteryNativeBridge {
constexpr char kNativeWall[] = "__OTR__scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORI_room_01DL_000B90";
constexpr char kBakedWall[] = "__OTR__custom/cor_woods_bridge/wall";
constexpr char kBakedWallVertices[] = "__OTR__custom/cor_woods_bridge/wall_vtx";
constexpr char kBridge[] = "__OTR__custom/cor_woods_bridge/bridge";
constexpr char kBridgeVertices[] = "__OTR__custom/cor_woods_bridge/bridge_vtx";
constexpr char kBark[] = "__OTR__custom/cor_woods_bridge/textures/bark";
constexpr char kPlanks[] = "__OTR__custom/cor_woods_bridge/textures/planks";
constexpr char kBlack[] = "__OTR__custom/cor_woods_bridge/textures/black";
constexpr char kNativeCanopy[] = "__OTR__scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORI_room_01DL_002968";
inline constexpr char kCanopyVertices[] = "__OTR__custom/cor_woods_bridge/canopy_vtx";
constexpr char kPatchName[] = "ComboWoodsMysteryBakedBridge";
constexpr int kCanopyIndices[] = { 28, 29, 49, 50 };
constexpr const char* kCanopyPatchNames[] = { "ComboWoodsMysteryCanopyFirstPath", "ComboWoodsMysteryCanopyFirstRange",
                                              "ComboWoodsMysteryCanopySecondPath",
                                              "ComboWoodsMysteryCanopySecondRange" };
inline bool sActive = false;
inline bool sAltEnabled = false;

inline bool NativeRoom(const PlayState* play) {
    if (!play || gSaveContext.gameMode != GAMEMODE_NORMAL || play->sceneId != SCENE_26SARUNOMORI ||
        play->roomCtx.curRoom.num != 1 || WoodsMysteryMakeover::OwnsBridgeMesh(play))
        return false;
    const RoomShape* shape = play->roomCtx.curRoom.roomShape;
    if (!shape || shape->base.type != ROOM_SHAPE_TYPE_CULLABLE || !shape->cullable.entries)
        return false;
    for (int i = 0; i < shape->cullable.numEntries; ++i) {
        const auto* opa = shape->cullable.entries[i].opa;
        if (opa && std::strncmp(reinterpret_cast<const char*>(opa), "__OTR__", 7) == 0 &&
            std::strcmp(reinterpret_cast<const char*>(opa), kNativeWall) == 0)
            return true;
    }
    return false;
}

inline bool PatchedWall() {
    const Gfx expected = gsSPBranchList(reinterpret_cast<Gfx*>(const_cast<char*>(kBakedWall)));
    const Gfx* wall = ResourceMgr_LoadGfxByName(kNativeWall);
    return wall && wall[0].words.w0 == expected.words.w0 && wall[0].words.w1 == expected.words.w1;
}

inline Gfx CanopyVertexPath() {
    Gfx command = {};
    command.words.w0 = static_cast<uintptr_t>(G_VTX_OTR_FILEPATH) << 24;
    command.words.w1 = reinterpret_cast<uintptr_t>(kCanopyVertices);
    return command;
}

inline Gfx CanopyVertexRange(int count, int offset) {
    Gfx command = {};
    command.words.w0 = count;
    command.words.w1 = offset; // Vertex buffer index is zero; source offset is in vertices.
    return command;
}

inline bool SameCommand(const Gfx& a, const Gfx& b) {
    return a.words.w0 == b.words.w0 && a.words.w1 == b.words.w1;
}

inline const Gfx* NativeCanopy() {
    // Binary and XML replacements can be shorter than this native layout.
    // Resolve/preload Alt first, then size that selection before fixed reads.
    if (!ResourceMgr_FileExists(kNativeCanopy))
        return nullptr;
    const Gfx* canopy = ResourceMgr_LoadGfxByName(kNativeCanopy);
    if (!canopy || ResourceGetIsCustomByName(kNativeCanopy) || ResourceGetSizeByName(kNativeCanopy) < 51 * sizeof(Gfx))
        return nullptr;
    return canopy;
}

inline bool CanopyPatched(const Gfx* canopy) {
    return canopy && SameCommand(canopy[28], CanopyVertexPath()) && SameCommand(canopy[29], CanopyVertexRange(12, 0)) &&
           SameCommand(canopy[49], CanopyVertexPath()) && SameCommand(canopy[50], CanopyVertexRange(18, 12));
}

inline bool CanopyCanPatch() {
    const Gfx* canopy = NativeCanopy();
    if (!canopy)
        return false;
    if (CanopyPatched(canopy))
        return true;
    const auto nativeHash = [](const Gfx& command) {
        return command.words.w0 == 0x6D7AC1D5u && command.words.w1 == 0xC35928A3u;
    };
    // G_VTX_OTR_HASH caches the original resource's raw vertex pointer in w1.
    // Accept its original byte offset or that cached form; replace both words
    // of each expanded command with a filepath load that never caches a pointer.
    return canopy[28].words.w0 == 0x3200C018u && (canopy[28].words.w1 == 0 || canopy[28].words.w1 > 0xFFFFFu) &&
           canopy[49].words.w0 == 0x32012024u && (canopy[49].words.w1 == 0xC0 || canopy[49].words.w1 > 0xFFFFFu) &&
           nativeHash(canopy[29]) && nativeHash(canopy[50]);
}

inline bool PatchedCanopy() {
    return ResourceMgr_FileExists(kCanopyVertices) && ResourceGetSizeByName(kCanopyVertices) == 38 * sizeof(Vtx) &&
           CanopyPatched(NativeCanopy());
}

inline void Reset() {
    sActive = false;
    // The API restores only the resource instance that supplied the original
    // instruction, including after an Alt switch or a resource reload.
    ResourceMgr_UnpatchGfxByName(kNativeWall, kPatchName);
    for (const char* patchName : kCanopyPatchNames)
        ResourceMgr_UnpatchGfxByName(kNativeCanopy, patchName);
}

inline void Update(const PlayState* play, bool passageAvailable) {
    if (!passageAvailable || !NativeRoom(play)) {
        Reset();
        return;
    }
    for (const char* path :
         { kBakedWall, kBakedWallVertices, kBridge, kBridgeVertices, kBark, kPlanks, kBlack, kCanopyVertices }) {
        if (!ResourceMgr_FileExists(path)) {
            Reset();
            return;
        }
    }
    if (!ResourceMgr_LoadGfxByName(kBakedWall) || !ResourceMgr_LoadGfxByName(kBridge) ||
        ResourceGetSizeByName(kCanopyVertices) != 38 * sizeof(Vtx) || !CanopyCanPatch()) {
        Reset();
        return;
    }
    const bool altEnabled = ResourceMgr_IsAltAssetsEnabled();
    if (sActive && sAltEnabled != altEnabled)
        Reset();
    sActive = false;
    sAltEnabled = altEnabled;
    const Gfx canopyCommands[] = { CanopyVertexPath(), CanopyVertexRange(12, 0), CanopyVertexPath(),
                                   CanopyVertexRange(18, 12) };
    for (int i = 0; i < 4; ++i)
        ResourceMgr_PatchGfxByName(kNativeCanopy, kCanopyPatchNames[i], kCanopyIndices[i], canopyCommands[i]);
    if (!PatchedCanopy()) {
        Reset();
        return;
    }
    const Gfx branch = gsSPBranchList(reinterpret_cast<Gfx*>(const_cast<char*>(kBakedWall)));
    // This API rejects XML/custom Alt lists. The complete replacement list
    // owns both the aperture and the bridge, leaving room headers untouched.
    ResourceMgr_PatchGfxByName(kNativeWall, kPatchName, 0, branch);
    if (!PatchedWall() || !PatchedCanopy()) {
        Reset();
        return;
    }
    sActive = true;
}

inline bool Active(const PlayState* play) {
    return sActive && NativeRoom(play) && sAltEnabled == ResourceMgr_IsAltAssetsEnabled() && PatchedWall() &&
           PatchedCanopy();
}
} // namespace WoodsMysteryNativeBridge
#endif
