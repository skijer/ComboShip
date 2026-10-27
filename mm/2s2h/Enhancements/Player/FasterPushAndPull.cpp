#include <libultraship/bridge/consolevariablebridge.h>
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"
#include "mods/combo_rpg.h"

extern "C" {
#include "overlays/actors/ovl_Bg_Dblue_Movebg/z_bg_dblue_movebg.h"
#include "overlays/actors/ovl_Bg_Ikana_Block/z_bg_ikana_block.h"
#include "overlays/actors/ovl_Obj_Oshihiki/z_obj_oshihiki.h"
#include "overlays/actors/ovl_Obj_Skateblock/z_obj_skateblock.h"
}

#define CVAR_NAME "gEnhancements.Player.FasterPushAndPull"
#define CVAR CVarGetInteger(CVAR_NAME, 0)

void RegisterFasterPushAndPull() {
    // Register once and read save state in each callback: handoffs and file
    // loads can change RPG settings without changing the enhancement CVar.
    COND_VB_SHOULD(VB_GREAT_BAY_GEAR_CLAMP_PUSH_SPEED, true, {
        const bool rpg = ComboRpg_IsEnabled(COMBO_RPG_PUSH);
        if (!rpg && !CVAR)
            return;
        BgDblueMovebg* bgDblueMovebg = va_arg(args, BgDblueMovebg*);
        *should = false;
        bgDblueMovebg->unk_188 = rpg ? MIN(bgDblueMovebg->unk_188, (s16)(5 + 3 * ComboRpg_PushBonus())) : 20;
    });

    COND_VB_SHOULD(VB_PUSH_BLOCK_SET_SPEED, true, {
        const bool rpg = ComboRpg_IsEnabled(COMBO_RPG_PUSH);
        if (!rpg && !CVAR)
            return;
        ObjOshihiki* objOshihiki = va_arg(args, ObjOshihiki*);
        objOshihiki->pushSpeed = rpg ? 2.0f + 0.5f * ComboRpg_PushBonus() : 5.0f;
        *should = false;
    });

    COND_VB_SHOULD(VB_PUSH_BLOCK_SET_TIMER, true, {
        const bool rpg = ComboRpg_IsEnabled(COMBO_RPG_PUSH);
        if (!rpg && !CVAR)
            return;
        Actor* actor = va_arg(args, Actor*);
        if (actor->id == ACTOR_OBJ_OSHIHIKI) {
            ((ObjOshihiki*)actor)->timer = rpg ? (s16)(10 - 1.5f * ComboRpg_PushBonus()) : 2;
        } else if (actor->id == ACTOR_BG_IKANA_BLOCK) {
            BgIkanaBlock* block = (BgIkanaBlock*)actor;
            block->unk_17B = rpg ? MIN(127, block->unk_17B + 1 + (s16)ComboRpg_PushBonus()) : 11;
        }
        *should = false;
    });

    COND_VB_SHOULD(VB_SKATE_BLOCK_BEGIN_MOVE, true, {
        const bool rpg = ComboRpg_IsEnabled(COMBO_RPG_PUSH);
        if (!rpg && !CVAR)
            return;
        // These blocks can only be pushed, not pulled
        ObjSkateblock* objSkateblock = va_arg(args, ObjSkateblock*);
        s32 directionIndex = va_arg(args, s32);
        *should = objSkateblock->unk_172[directionIndex] > (rpg ? (s16)(10 - 2 * ComboRpg_PushBonus()) : 0);
    });

    COND_VB_SHOULD(VB_BLOCK_BEGIN_MOVE, true, {
        if (!ComboRpg_IsEnabled(COMBO_RPG_PUSH) && CVAR)
            *should = true;
    });

    COND_VB_SHOULD(VB_BLOCK_BE_FINISHED_PULLING, true, {
        const bool rpg = ComboRpg_IsEnabled(COMBO_RPG_PUSH);
        if (rpg ? ComboRpg_PushBonus() == 0.0f : !CVAR)
            return;
        f32* pValue = va_arg(args, f32*);
        f32 target = (f32)va_arg(args, f64);
        f32 step = (f32)va_arg(args, f64);
        f32 maxStep = (f32)va_arg(args, f64);
        step = CLAMP_MAX(step, maxStep);
        if (rpg)
            step *= ComboRpg_PushBonus() / 5.0f;
        // This is actually the same exact condition, but because we're hooking and running it here it effectively
        // doubles the speed
        *should = Math_StepToF(pValue, target, step);
    });
}

static RegisterShipInitFunc initFunc(RegisterFasterPushAndPull, { CVAR_NAME });
