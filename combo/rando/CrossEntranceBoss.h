#pragma once
#include <stdint.h>

// Stable physical boss identities shared by both hosts and the consolidated seed.
enum ComboCrossBossId {
    COMBO_BOSS_GOHMA,
    COMBO_BOSS_DODONGO,
    COMBO_BOSS_BARINADE,
    COMBO_BOSS_PHANTOM_GANON,
    COMBO_BOSS_VOLVAGIA,
    COMBO_BOSS_MORPHA,
    COMBO_BOSS_TWINROVA,
    COMBO_BOSS_BONGO_BONGO,
    COMBO_BOSS_ODOLWA,
    COMBO_BOSS_GOHT,
    COMBO_BOSS_GYORG,
    COMBO_BOSS_TWINMOLD,
    COMBO_BOSS_COUNT
};

#ifdef COMBO_BUILD
#ifdef __cplusplus
extern "C" {
#endif
int Combo_SOH_CrossBossEnabled(void);
int Combo_MM_CrossBossEnabled(void);
int Combo_MM_CrossBossLogicEnabled(void);
void Combo_MM_RequestCrossBossDeathRecovery(void);
int Combo_SOH_CrossBossLogicalClear(int id, int nativeValue);
int Combo_MM_CrossBossLogicalClear(int id, int nativeValue);
int Combo_SOH_CrossBossWarp(int scene);
int Combo_MM_CrossBossWarp(int scene);
int32_t Combo_SOH_CrossDungeonDeathCheckpoint(void);
#ifdef __cplusplus
}
#endif
#else
#define Combo_SOH_CrossBossLogicalClear(id, nativeValue) (nativeValue)
#define Combo_MM_CrossBossLogicalClear(id, nativeValue) (nativeValue)
#define Combo_SOH_CrossBossEnabled() 0
#define Combo_MM_CrossBossEnabled() 0
#define Combo_MM_CrossBossLogicEnabled() 0
#define Combo_MM_RequestCrossBossDeathRecovery() ((void)0)
#define Combo_SOH_CrossBossWarp(scene) 0
#define Combo_MM_CrossBossWarp(scene) 0
#endif

#ifdef __cplusplus
namespace ComboRando::CrossDungeon {
// Reward flags remain native. This state only owns the source dungeon completion aliases.
class BossCompletionState {
  public:
    void SetCompletedMask(uint16_t mask) {
        completed = mask & 0x0fff;
    }
    uint16_t CompletedMask() const {
        return completed;
    }
    void SetNotify(void (*callback)(int)) {
        notify = callback;
    }
    bool LogicalClear(int id, bool nativeValue, bool enabled) const {
        return !enabled ? nativeValue : id >= 0 && id < COMBO_BOSS_COUNT && (completed & (1u << id));
    }
    bool Warp(int id, bool enabled) const {
        if (!enabled || id < 0 || id >= COMBO_BOSS_COUNT)
            return false;
        // Notify on repeat use too: MM's cycle world effects may have reset since the last warp.
        if (notify)
            notify(id);
        return true;
    }

  private:
    uint16_t completed = 0;
    void (*notify)(int) = nullptr;
};
} // namespace ComboRando::CrossDungeon
#endif
