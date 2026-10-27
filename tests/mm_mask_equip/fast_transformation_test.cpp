#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <libultraship/bridge/consolevariablebridge.h>

extern "C" {
#include "global.h"
#include "2s2h/GameInteractor/GameInteractor_VanillaBehavior.h"
}
#include "2s2h/Enhancements/Enhancements.h"

#define REQUIRE(condition)                                                                                       \
    do {                                                                                                         \
        if (!(condition)) {                                                                                      \
            std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition);                                     \
            std::exit(1);                                                                                        \
        }                                                                                                        \
    } while (0)

SaveContext gSaveContext{};
PlayState* gPlayState = nullptr;
static PlayState play;
static Player player;
static int fastMode;
static int appliedSounds;
static u16 appliedSfx;

extern "C" int32_t CVarGetInteger(const char* name, int32_t) {
    REQUIRE(std::strcmp(name, "gEnhancements.Masks.FastTransformation") == 0);
    return fastMode;
}

extern "C" void func_8082E1F0(Player* target, u16 sfx) {
    REQUIRE(target == &player);
    ++appliedSounds;
    appliedSfx = sfx;
}

// Pending-completion cases do not start a new item action. Unexpected engine
// service calls fail instead of silently simulating a successful action.
extern "C" void Player_UseItem(PlayState*, Player*, ItemId) { std::abort(); }
extern "C" PlayerItemAction Player_ItemToItemAction(Player*, ItemId) { std::abort(); }
extern "C" u8 Player_MaskIdToItemId(s32) { std::abort(); }
extern "C" void Audio_PlaySfx(u16) { std::abort(); }
extern "C" bool GameInteractor_Should(GIVanillaBehavior, uint32_t, ...) { std::abort(); }

#include "fast_transformation_production.inc"
#include "easy_mask_equip_production.inc"

static void Reset(int mode, int currentForm, int targetForm) {
    play = {};
    player = {};
    gSaveContext = {};
    gPlayState = &play;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    player.transformation = currentForm;
    gSaveContext.save.playerForm = targetForm;
    fastMode = mode;
    appliedSounds = 0;
    appliedSfx = 0;
    sPendingAction = {};
    sVisualMaskOverride = {};
    sMaskEquippedWithoutButton = PLAYER_MASK_NONE;
}

static void TestPolicy() {
    // The new EME completion caller reaches this settled human state. Before
    // the fix, the actual production table lookup trips UBSan at index 4.
    Reset(FAST_TRANSFORM_AFTER_FIRST, PLAYER_FORM_HUMAN, PLAYER_FORM_HUMAN);
    REQUIRE(ShouldFastTransform());

    const u16 firstUseFlags[] = { WEEKEVENTREG_30_80, WEEKEVENTREG_30_20,
                                 WEEKEVENTREG_30_40, WEEKEVENTREG_30_10 };
    for (int mode = FAST_TRANSFORM_OFF; mode <= FAST_TRANSFORM_AFTER_FIRST; ++mode) {
        for (int current = 0; current < PLAYER_FORM_MAX; ++current) {
            for (int target = 0; target < PLAYER_FORM_MAX; ++target) {
                Reset(mode, current, target);
                for (unsigned completed = 0; completed < 16; ++completed) {
                    gSaveContext.save.saveInfo.weekEventReg[30] = 0;
                    for (unsigned form = 0; form < 4; ++form) {
                        if (completed & (1u << form)) {
                            gSaveContext.save.saveInfo.weekEventReg[firstUseFlags[form] >> 8] |= firstUseFlags[form] & 0xFF;
                        }
                    }
                    const auto before = gSaveContext.save.saveInfo;
                    bool expected = mode == FAST_TRANSFORM_ON ||
                                    (mode == FAST_TRANSFORM_AFTER_FIRST &&
                                     (current != PLAYER_FORM_HUMAN || target == PLAYER_FORM_HUMAN ||
                                      (completed & (1u << target))));
                    REQUIRE(ShouldFastTransform() == expected);
                    REQUIRE(std::memcmp(before.weekEventReg, gSaveContext.save.saveInfo.weekEventReg,
                                        sizeof(before.weekEventReg)) == 0);
                }
            }
        }
    }
    for (int mode = FAST_TRANSFORM_OFF; mode <= FAST_TRANSFORM_AFTER_FIRST; ++mode) {
        for (int invalid : { static_cast<int>(PLAYER_FORM_MAX), 255 }) {
            Reset(mode, PLAYER_FORM_HUMAN, invalid);
            REQUIRE(!ShouldFastTransform());
            Reset(mode, invalid, PLAYER_FORM_HUMAN);
            REQUIRE(!ShouldFastTransform());
        }
        Reset(mode, PLAYER_FORM_HUMAN, PLAYER_FORM_DEKU);
        gPlayState = nullptr;
        REQUIRE(!ShouldFastTransform());
        gPlayState = &play;
        play.actorCtx.actorLists[ACTORCAT_PLAYER].first = nullptr;
        REQUIRE(!ShouldFastTransform());
    }
    for (int mode : { -1, 3 }) {
        Reset(mode, PLAYER_FORM_HUMAN, PLAYER_FORM_HUMAN);
        REQUIRE(!ShouldFastTransform());
    }
    puts("PASS production fast-transform policy: modes 0/1/2, all five current/target forms, all completion flags and invalid/context bounds");
}

static void TestRegularReturn() {
    for (int mode = FAST_TRANSFORM_OFF; mode <= FAST_TRANSFORM_AFTER_FIRST; ++mode) {
        for (int form = 0; form < PLAYER_FORM_HUMAN; ++form) {
            for (int mask = PLAYER_MASK_TRUTH; mask < PLAYER_MASK_GIANT; ++mask) {
                for (bool alreadyApplied : { false, true }) {
                    Reset(mode, form, PLAYER_FORM_HUMAN);
                    player.currentMask = form + PLAYER_MASK_FIERCE_DEITY;
                    sPendingAction.mode = PENDING_RETURN_TO_HUMAN_THEN_REGULAR;
                    sPendingAction.targetMask = static_cast<PlayerMask>(mask);
                    sVisualMaskOverride = { static_cast<PlayerMask>(mask), true };
                    ProcessPendingMaskEquip(&player.actor);
                    REQUIRE(sPendingAction.mode == PENDING_RETURN_TO_HUMAN_THEN_REGULAR);
                    REQUIRE(player.transformation == form && appliedSounds == 0);
                    player.transformation = PLAYER_FORM_HUMAN;
                    player.currentMask = alreadyApplied ? mask : PLAYER_MASK_NONE;
                    gSaveContext.save.equippedMask = player.currentMask;
                    ProcessPendingMaskEquip(&player.actor);
                    REQUIRE(player.currentMask == mask && gSaveContext.save.equippedMask == mask);
                    REQUIRE(sMaskEquippedWithoutButton == mask);
                    REQUIRE(sPendingAction.mode == PENDING_NONE && !sVisualMaskOverride.active);
                    REQUIRE(appliedSounds == (mode != FAST_TRANSFORM_OFF));
                    if (appliedSounds) {
                        REQUIRE(appliedSfx == NA_SE_PL_CHANGE_ARMS);
                    }
                    REQUIRE(gSaveContext.save.saveInfo.weekEventReg[30] == 0);
                    REQUIRE(gSaveContext.save.saveInfo.weekEventReg[16] == 0);
                }
            }
        }
    }
    puts("PASS actual Easy Mask Equip caller: every regular mask after all transformed forms, native/fast modes, both 3DS application orders");
}

static void TestCompletedPlayerAction() {
    for (int mode = FAST_TRANSFORM_OFF; mode <= FAST_TRANSFORM_AFTER_FIRST; ++mode) {
        for (int form = 0; form < PLAYER_FORM_MAX; ++form) {
            Reset(mode, form, form);
            const auto mask = form == PLAYER_FORM_HUMAN ? PLAYER_MASK_NONE
                                                       : static_cast<PlayerMask>(PLAYER_MASK_FIERCE_DEITY + form);
            player.currentMask = mask;
            gSaveContext.save.equippedMask = mask;
            sPendingAction.mode = PENDING_PLAYER_ACTION;
            sPendingAction.targetMask = mask;
            ProcessPendingMaskEquip(&player.actor);
            REQUIRE(sPendingAction.mode == PENDING_NONE && !sVisualMaskOverride.active);
            REQUIRE(player.currentMask == mask && player.transformation == form);
            REQUIRE(appliedSounds == (mode != FAST_TRANSFORM_OFF));
        }
        Reset(mode, PLAYER_FORM_HUMAN, PLAYER_FORM_HUMAN);
        player.currentMask = PLAYER_MASK_GIANT;
        sPendingAction.mode = PENDING_PLAYER_ACTION;
        sPendingAction.targetMask = PLAYER_MASK_GIANT;
        ProcessPendingMaskEquip(&player.actor);
        REQUIRE(sPendingAction.mode == PENDING_NONE && sMaskEquippedWithoutButton == PLAYER_MASK_GIANT);
        REQUIRE(appliedSounds == 0);
    }
    puts("PASS actual Easy Mask Equip completed-action caller: all five forms, human return and Giant sound exclusion");
}

int main(int argc, char** argv) {
    if (argc > 1 && std::strcmp(argv[1], "caller") == 0) {
        TestRegularReturn();
        TestCompletedPlayerAction();
    } else {
        TestPolicy();
    }
}
