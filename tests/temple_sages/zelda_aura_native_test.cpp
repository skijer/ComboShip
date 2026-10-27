#include "ComboGameMode.h"
#include "actors/TempleSagesZeldaAura.h"
#include "entrances/TempleSagesPolicy.h"
#include "global.h"
#include "soh/Enhancements/Gameplay/ComboTempleSages.h"
#include "soh/Enhancements/randomizer/NeiGiRender.h"
#include <cassert>
#include <cstring>
#include <limits>

SaveContext gSaveContext{};
extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
}
static PlayState play;
static GraphicsContext gfx;
static Actor actor;
static int roomToken, sceneToken;
static unsigned draws, pushes, pops;
static NeiGi::Mesh submitted;
static Vec3f origin;
extern "C" void Matrix_Push() { ++pushes; }
extern "C" void Matrix_Pop() { ++pops; }
extern "C" void Matrix_Translate(float x, float y, float z, u8 mode) {
  assert(mode == MTXMODE_NEW);
  origin = {x, y, z};
}
extern "C" NeiGi::Basis NeiGi_CameraBasis(PlayState *p) {
  assert(p == &play);
  return {};
}
extern "C" void NeiGi_DrawWorldParticles(PlayState *p,
                                         const NeiGi::Mesh &mesh) {
  assert(p == &play);
  submitted = mesh;
  ++draws;
}
/* PRODUCTION_AURA_BRIDGE */

void Setup(int params = 0x7D23) {
  play = {};
  actor = {};
  gSaveContext = {};
  gfx = {};
  draws = pushes = pops = 0;
  submitted = {};
  gComboGameMode = COMBO_GAME_BOTH;
  play.state.running = true;
  play.state.gfxCtx = &gfx;
  play.sceneSegment = &sceneToken;
  play.roomCtx.curRoom.segment = &roomToken;
  play.roomCtx.curRoom.num = actor.room = 0;
  play.gameOverCtx.state = GAMEOVER_INACTIVE;
  play.csCtx.state = CS_STATE_IDLE;
  play.transitionTrigger = TRANS_TRIGGER_OFF;
  play.transitionMode = TRANS_MODE_OFF;
  gSaveContext.gameMode = GAMEMODE_NORMAL;
  actor.id = ACTOR_EN_VIEWER;
  actor.params = params;
  actor.world.pos = {0, -110, -2400};
  actor.xzDistToPlayer = 200;
}
int main() {
  for (int params : {0x7D23, 0x7F34})
    for (int mode : {COMBO_GAME_BOTH, COMBO_GAME_OOT}) {
      Setup(params);
      gComboGameMode = mode;
      const auto beforeActor = actor;
      const auto beforeSave = gSaveContext;
      ComboTempleSages_DrawZeldaAura(&actor, &play);
      assert(draws == 1 && pushes == 1 && pops == 1 && submitted.count > 0 &&
             submitted.count <= 720);
      assert(origin.x == 0 && origin.y == -110 && origin.z == -2400);
      assert(std::memcmp(&actor, &beforeActor, sizeof(actor)) == 0);
      assert(std::memcmp(&gSaveContext, &beforeSave, sizeof(gSaveContext)) ==
             0);
    }
  for (int params : {0, 2, 0x7D03, 0x7D13, 0x7F04, 0x7F14, 0x7F24}) {
    Setup(params);
    ComboTempleSages_DrawZeldaAura(&actor, &play);
    assert(draws == 0 && pushes == 0 && pops == 0);
  }
  Setup();
  ComboTempleSages_DrawZeldaAuraAt(&actor, &play, 15.7f, -110.f, -2400.f);
  assert(draws == 1 && pushes == 1 && pops == 1);
  assert(origin.x == 15.7f && origin.y == -110.f && origin.z == -2400.f);
  for (float bad : {std::numeric_limits<float>::quiet_NaN(),
                    std::numeric_limits<float>::infinity()}) {
    for (int axis = 0; axis < 3; ++axis) {
      Setup();
      float position[3]{15.7f, -110.f, -2400.f};
      position[axis] = bad;
      ComboTempleSages_DrawZeldaAuraAt(&actor, &play, position[0], position[1],
                                       position[2]);
      assert(draws == 0 && pushes == 0 && pops == 0);
    }
  }
  for (int bad = 0; bad < 24; ++bad) {
    Setup();
    switch (bad) {
    case 0:
      actor.id = ACTOR_EN_ZL4;
      break;
    case 1:
      actor.id = ACTOR_EN_ZL3;
      break;
    case 2:
      gComboGameMode = COMBO_GAME_MM;
      break;
    case 3:
      play.state.running = false;
      break;
    case 4:
      play.state.gfxCtx = nullptr;
      break;
    case 5:
      play.sceneSegment = nullptr;
      break;
    case 6:
      play.roomCtx.curRoom.segment = nullptr;
      break;
    case 7:
      play.roomCtx.status = 1;
      break;
    case 8:
      actor.room = 1;
      break;
    case 9:
      play.pauseCtx.state = 1;
      break;
    case 10:
      play.csCtx.state = 1;
      break;
    case 11:
      play.gameOverCtx.state = 1;
      break;
    case 12:
      play.transitionTrigger = TRANS_TRIGGER_START;
      break;
    case 13:
      play.transitionMode = 1;
      break;
    case 14:
      gSaveContext.cutsceneTrigger = 1;
      break;
    case 15:
      gSaveContext.gameMode = GAMEMODE_TITLE_SCREEN;
      break;
    case 16:
      gSaveContext.sceneLayer = 4;
      break;
    case 17:
      actor.xzDistToPlayer = 1801;
      break;
    case 18:
      actor.yDistToPlayer = -401;
      break;
    case 19:
      actor.xzDistToPlayer = std::numeric_limits<float>::quiet_NaN();
      break;
    case 20:
      actor.yDistToPlayer = std::numeric_limits<float>::quiet_NaN();
      break;
    case 21:
      actor.world.pos.x = std::numeric_limits<float>::infinity();
      break;
    case 22:
      actor.world.pos.y = std::numeric_limits<float>::quiet_NaN();
      break;
    case 23:
      actor.world.pos.z = std::numeric_limits<float>::infinity();
      break;
    }
    ComboTempleSages_DrawZeldaAura(&actor, &play);
    assert(draws == 0 && pushes == 0 && pops == 0);
  }
  Setup();
  ComboTempleSages_DrawZeldaAura(nullptr, &play);
  ComboTempleSages_DrawZeldaAura(&actor, nullptr);
  assert(draws == 0 && pushes == 0 && pops == 0);
  puts("PASS native Zelda aura bridge: exact opt-in actor scope, normal/solo "
       "guards, balanced local pose, finite bounds and no actor/save writes");
}
