#pragma once
#include <cstdint>

namespace BlackMarketMmShopScene {
inline constexpr int16_t kKeeperActor = 42;
inline constexpr int16_t kKeeperParams = 1;
inline constexpr int16_t kItemParamBase = 28672;
inline constexpr int16_t kCameraSetting = 91;
inline constexpr int16_t kCutsceneCount = 4;
inline constexpr int16_t kFirstCutscene = 0;
inline constexpr int16_t kFirstPriority = 10;
inline constexpr int16_t kHudVisibility = 4;
inline constexpr int16_t kLetterboxSize = 32;
inline constexpr const char* kPresentationResources[] = {
    "custom/skiejer_market/mm_shop_presentation_v1/room",
    "custom/skiejer_market/mm_shop_presentation_v1/geometry/mahouya_room_0DL_006548",
    "custom/skiejer_market/mm_shop_presentation_v1/geometry/mahouya_room_0DL_006548_tri_0",
    "custom/skiejer_market/mm_shop_presentation_v1/geometry/mahouya_room_0DL_006548_vtx_0",
    "custom/skiejer_market/mm_shop_presentation_v1/geometry/mahouya_room_0DL_006548_tri_15",
    "custom/skiejer_market/mm_shop_presentation_v1/geometry/mahouya_room_0DL_006548_vtx_15",
    "custom/skiejer_market/mm_shop_presentation_v1/geometry/mahouya_room_0DL_006548_tri_17",
    "custom/skiejer_market/mm_shop_presentation_v1/geometry/mahouya_room_0DL_006548_vtx_17",
    "custom/skiejer_market/mm_shop_presentation_v1/geometry/mahouya_room_0DL_006548_tri_2",
    "custom/skiejer_market/mm_shop_presentation_v1/geometry/mahouya_room_0DL_006548_vtx_2",
    "custom/skiejer_market/mm_shop_presentation_v1/geometry/mahouya_room_0DL_006548_tri_6",
    "custom/skiejer_market/mm_shop_presentation_v1/geometry/mahouya_room_0DL_006548_vtx_6",
    "custom/skiejer_market/mm_shop_presentation_v1/geometry/mahouya_room_0DL_006548_tri_18",
    "custom/skiejer_market/mm_shop_presentation_v1/geometry/mahouya_room_0DL_006548_vtx_18",
};

// OoT Data4: one fixed eye and a +/-30-degree yaw pivot. The linked records
// are kept for native MM actor initialization/receipt continuation, but browsing
// retains the keeper cutscene instead of replacing its camera at each bank.
// Each row is position XYZ, rotation XYZ, then FOV / donor jfif axes / unused.
// Axes 3 makes Data4 use the scene pitch and yaw; FOV -1 selects native 60.
inline constexpr int16_t kCameraRecords[4][9] = {
    { 0, 43, 160, 728, -32768, 0, -1, 3, -1 },
    { 0, 43, 160, 728, -32768, 0, -1, 3, -1 },
    { 0, 43, 160, 728, -32768, 0, -1, 3, -1 },
    { 0, 43, 160, 728, -32768, 0, -1, 3, -1 },
};

// Native chain order: keeper -> left shelf -> right shelf -> return to keeper.
inline constexpr int16_t kCutsceneCameras[4] = { 0, 2, 3, 1 };

// Straight native shelf-to-selection motion: room endpoints only, right then
// left bank. Camera eye, pitch and FOV do not follow the selected item.
// Inspection temporarily suppresses native letterbox clipping; this endpoint
// retains the complete authored item/seal envelope above the unchanged textbox.
inline constexpr float kSelectedTargets[2][3] = {
    { 31.04f, 51.66f, 106.24f },
    { -31.04f, 51.66f, 106.24f },
};
} // namespace BlackMarketMmShopScene
