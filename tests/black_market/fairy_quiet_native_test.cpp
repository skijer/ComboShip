#include "ComboGameMode.h"
#include "entrances/BlackMarketEntrance.h"
#include "global.h"
#include "overlays/actors/ovl_En_Elf/z_en_elf.h"
#ifdef FAIRY_QUIET_OOT
#include "soh/Enhancements/audio/MidnaAudio.h"
#else
#include "2s2h/Enhancements/Companion/MidnaAudio.h"
#endif
#if __has_include("BlackMarketFairy.h")
#include "BlackMarketFairy.h"
#endif
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
SaveContext gSaveContext;
extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
}
static Vec3f movementTarget;
static int blinks, movementUpdates;
static void Capture(EnElf *, Vec3f *target) { movementTarget = *target; ++movementUpdates; }
static void Bob(EnElf *, PlayState *) {}
static void Draw(Actor *, PlayState *) {}
struct CUpCandidates {
  Actor *a;
  Actor *cup;
};
#ifdef FAIRY_QUIET_OOT
static GameInfo info;
GameInfo *gGameInfo = &info;
#define func_80A0461C(...) ((void)0)
#define func_80A03AB0(...) ((void)0)
#define func_80A01C38(f, state) ((f)->unk_2A8 = (state))
#define func_80A02C98(f, p, ...) Capture(f, p)
#define func_80A02EC0(f, p) Capture(f, p)
#define func_80A02E30(f, p) Capture(f, p)
#define func_80A03148(f, p, ...) Capture(f, p)
#define EnElf_UpdateLights(...) ((void)0)
#define EnElf_SpawnSparkles(...) ((void)0)
#define EnElf_GetCutsceneNextPos(...) ((void)0)
#define EnElf_PlayNaviSound(...) ((void)0)
#define func_80A029A8(...) ((void)0)
#define func_80A0299C(...) ((void)0)
#define ElfMessage_GetCUpText(...) 0
#define Actor_ProcessTalkRequest(...) false
#define MidnaAudio_TryPlay(...) true
#define MidnaAudio_UpdateIdle(...) ((void)0)
#define func_800F4524(...) ((void)0)
#define func_80A052F4 nullptr
#define func_80A04D90(...) ((void)0)
#define EnElf_UpdateMidnaBlink(...) (++blinks)
#define LOG_NUM(...) ((void)0)
#else
#define func_8088F214(...) ((void)0)
#define func_8088E5A8(...) ((void)0)
#define func_8088C51C(f, state) ((f)->unk_244 = (state))
#define func_8088D660(f, p, ...) Capture(f, p)
#define func_8088D864(f, p) Capture(f, p)
#define func_8088D7F8(f, p) Capture(f, p)
#define func_8088DB4C(f, p, ...) Capture(f, p)
#define func_8088E60C(...) ((void)0)
#define func_8088F5F4(...) ((void)0)
#define func_808908D0(...) ((void)0)
#define EnElf_PlayTatlSound(...) ((void)0)
#define QuestHint_GetTatlTextId(...) 0
#define Actor_TalkOfferAccepted(...) false
#define MMMidnaAudio_TryPlay(...) true
#define MMMidnaAudio_UpdateIdle(...) ((void)0)
#define Audio_PlaySfx_AtPosWithReverb(...) ((void)0)
#define func_8088FE64 nullptr
#define func_8088FD04(...) ((void)0)
#define Actor_ChangeFocus(...) ((void)0)
#define Cutscene_IsCueInChannel(...) false
#define Cutscene_GetCueChannel(...) 0
#define EnElf_UpdateMidnaBlink(...) (++blinks)
#endif
#define SkelAnime_Update(...) (0)
#define Lights_PointNoGlowSetInfo(...) ((void)0)
#define EnElf_UpdateMidnaIdleAudio(...) ((void)0)
#define Math_StepToF(...) (false)
#define Environment_AdjustLights(...) (false)
#define Rand_ZeroFloat(...) (0.f)
#define Play_InCsMode(...) (false)
#define Math_SinS(...) (0.f)
#define Math_CosS(...) (1.f)
#define Math_Vec3f_Copy(a, b) (*(a) = *(b))
#define Math_Vec3f_DistXYZ(a, b)                                               \
  (std::sqrt(std::pow((a)->x - (b)->x, 2) + std::pow((a)->y - (b)->y, 2) +     \
             std::pow((a)->z - (b)->z, 2)))
#define Math_SmoothStepToF(p, target, ...) (*(p) = (target))
/* PRODUCTION */
int main() {
  static PlayState play;
  static Player player;
  EnElf fairy{};
  Actor merchandise{}, keeper{};
  fairy.actor.id = ACTOR_EN_ELF;
  fairy.actor.params = 0;
  fairy.actor.draw = Draw;
  fairy.actor.scale = {.008f, .008f, .008f};
  player.actor.world.pos = {0, 0, 200};
  player.actor.focus.pos = {0, 60, 200};
  player.talkActor = &keeper;
  player.focusActor = &merchandise;
  merchandise.flags = ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_TALK_WITH_C_UP;
  keeper.flags = ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_FRIENDLY;
#ifdef FAIRY_QUIET_OOT
  play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
  player.naviActor = &fairy.actor;
  merchandise.naviEnemyId = 0x20;
  player.bodyPartsPos[8] = {0, 60, 200};
  play.actorCtx.targetCtx.arrowPointedActor = &merchandise;
  play.actorCtx.targetCtx.naviRefPos = {75, 76, 40};
  fairy.func_2C8 = Bob;
  fairy.actionFunc = func_80A03CF8;
#else
  play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
  player.tatlActor = &fairy.actor;
  merchandise.hintId = 0x20;
  gSaveContext.save.saveInfo.playerData.health = 48;
  player.bodyPartsPos[PLAYER_BODYPART_HAT] = {0, 60, 200};
  play.actorCtx.attention.tatlHoverActor = &merchandise;
  play.actorCtx.attention.tatlHoverPos = {75, 76, 40};
  fairy.unk_26C = Bob;
  fairy.actionFunc = func_8088E850;
#endif
  const auto flags = keeper.flags, stockFlags = merchandise.flags;
  for (int mode : {COMBO_GAME_BOTH,
#ifdef FAIRY_QUIET_OOT
                   COMBO_GAME_OOT, COMBO_GAME_MM
#else
                   COMBO_GAME_MM, COMBO_GAME_OOT
#endif
       })
    for (int inside : {0, 1, 0}) {
      gComboGameMode = mode;
#ifdef FAIRY_QUIET_OOT
      const bool quiet = inside && mode != COMBO_GAME_MM;
#else
      const bool quiet = inside && mode != COMBO_GAME_OOT;
#endif
#ifdef FAIRY_QUIET_OOT
      play.sceneNum = inside ? SCENE_SKIEJER_MARKET : SCENE_MARKET_DAY;
      player.naviTextId = -0x110;
#else
      play.sceneId = inside ? COMBO_BLACK_MARKET_SCENE : SCENE_00KEIKOKU;
      player.tatlTextId = -0x110;
#endif
      player.talkActor = &keeper;
      player.focusActor = &merchandise;
#ifdef FAIRY_QUIET_OOT
      player.stateFlags2 |= PLAYER_STATE2_NAVI_ALERT;
#else
      player.stateFlags2 |= PLAYER_STATE2_200000;
#endif
      const auto candidates = NativeCUpCandidates(&player, &play);
      if (quiet && !std::getenv("FAIRY_TEST_FOLLOW")) {
        assert(candidates.a == &keeper && !candidates.cup &&
               "shop fairy C-Up cannot steal the keeper's A talk offer");
#ifdef FAIRY_QUIET_OOT
        assert(!(player.stateFlags2 & PLAYER_STATE2_NAVI_ALERT) &&
               "stale Navi C-Up prompt must clear in the shop");
#else
        assert(!(player.stateFlags2 & PLAYER_STATE2_200000) &&
               "stale Tatl C-Up prompt must clear in the shop");
#endif
      } else if (!quiet)
        assert(candidates.cup == &fairy.actor &&
               "ordinary fairy interaction is preserved outside Black Market");
      const int previousMovementUpdates = movementUpdates, previousBlinks = blinks;
#ifdef FAIRY_QUIET_OOT
      func_80A053F0(&fairy.actor, &play);
#else
      func_8089010C(&fairy.actor, &play);
#endif
      if (quiet) {
        assert(movementUpdates == previousMovementUpdates && blinks == previousBlinks &&
               "hidden shop companion skips movement, targeting and blink updates");
        assert(!(fairy.actor.flags & ACTOR_FLAG_TALK_OFFER_AUTO_ACCEPTED));
        assert(fairy.actor.draw == Draw && fairy.actor.scale.x > 0 &&
               "shop keeps the draw callback and scale for ordinary behavior on exit; "
               "the native draw entry guard suppresses the shop model");
      } else
        assert(movementTarget.z == 40 &&
               "outside-shop attention retains its ordinary target position");
      assert(
          keeper.flags == flags && merchandise.flags == stockFlags &&
          "quiet fairy never disables keeper or merchandise actor attention");
    }
  assert(blinks == 7);
  std::puts("PASS native fairy policy: real C-Up candidate selection preserves "
            "keeper A; shop companion skips movement/blink; native draw-entry "
            "guards and zero-radius lights are covered by season_shop fixtures; "
            "normal attention restored outside shop");
}
