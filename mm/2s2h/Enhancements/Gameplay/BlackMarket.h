#pragma once
#ifdef COMBO_BUILD
struct PlayState;
struct Actor;
struct CollisionContext;
enum {
    COMBO_BLACK_MARKET_DOOR_UNHANDLED,
    COMBO_BLACK_MARKET_DOOR_ENTERED,
    COMBO_BLACK_MARKET_DOOR_BLOCKED,
};
#ifdef __cplusplus
#include <nlohmann/json.hpp>
void ComboBlackMarket_SaveState(nlohmann::json& save, int slot);
void ComboBlackMarket_LoadState(const nlohmann::json& save, int slot);
extern "C" {
#endif
int ComboBlackMarket_IsInterior(void);
int ComboBlackMarket_IsCounterBarrier(const struct CollisionContext* context, int bgId);
void ComboBlackMarket_ResetState(void);
void ComboBlackMarket_NormalizeEntry(void);
void ComboBlackMarket_RestoreRoomAfterDraw(struct PlayState* play);
void ComboBlackMarket_ApplyResume(int resume);
int ComboBlackMarket_TryDoorEntrance(struct PlayState* play, struct Actor* door);
int ComboBlackMarket_IsAuthoredDoor(struct PlayState* play, struct Actor* door);
#ifdef __cplusplus
}
#endif
#endif
