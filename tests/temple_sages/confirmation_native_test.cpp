#include "entrances/TempleSagesConfirmationNative.h"
#include <cassert>
#include <cstdio>
#include <cstring>
PlayState* gPlayState;
GameState* gGameState;
SaveContext gSaveContext{};
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
static GameInteractor interactor;
GameInteractor* GameInteractor::Instance = &interactor;
static PlayState play;
static Player player;
static int mode = 1, opened, closed, acquired, cleared, failCamera;
static bool advance;
static std::string loaded;
static Vec3f framedAt, framedEye;
static bool transformed;
static bool TransformMasks_IsTransformed() { return transformed; }
static void TransformMasks_FilterB(Input*) {}
static bool MmMaskWear_IsKamaroDancing() { return false; }
static Input NativePlayerInput(Player* self, PlayState* play) {
    Input input{};
    /* ACTUAL PLAYER INPUT */
    return input;
}
CustomMessage::CustomMessage(std::string text, TextBoxType, TextBoxPosition) { loaded = text; }
void CustomMessage::AutoFormat() {}
void CustomMessage::LoadIntoFont() {}
bool GameInteractor::IsPlayerInControl() { return true; }
extern "C" int32_t CVarGetInteger(const char*, int32_t) { return mode; }
/* ACTUAL MESSAGE STATE */
static void SetMessageState(u8 state) {
    play.msgCtx.msgLength = state == TEXT_STATE_NONE ? 0 : 1;
    play.msgCtx.msgMode = state == TEXT_STATE_CHOICE ? MSGMODE_TEXT_DONE : MSGMODE_TEXT_CLOSING;
    play.msgCtx.textboxEndType = TEXTBOX_ENDTYPE_2_CHOICE;
    play.msgCtx.stateTimer = 1;
}
extern "C" u8 Message_ShouldAdvance(PlayState*) { return advance; }
extern "C" void Message_StartTextbox(PlayState* p, u16 id, Actor*) {
    ++opened; p->msgCtx.textId = id; p->msgCtx.msgMode = MSGMODE_TEXT_START;
    bool fromTable = true;
    TempleSages::ConfirmationNative::OpenText(&id, &fromTable);
    assert(!fromTable);
}
extern "C" void Message_CloseTextbox(PlayState*) { ++closed; SetMessageState(TEXT_STATE_CLOSING); }
extern "C" s16 Play_CreateSubCamera(PlayState* p) {
    ++acquired;
    if (failCamera) return SUBCAM_NONE;
    p->cameraPtrs[1] = &p->subCameras[0]; p->cameraPtrs[1]->uid = 42;
    return 1;
}
extern "C" s16 Play_GetActiveCamId(PlayState* p) { return p->activeCamera; }
extern "C" s16 Play_ChangeCameraStatus(PlayState* p, s16 id, s16 status) {
    if (status == CAM_STAT_ACTIVE) p->activeCamera = id;
    return 1;
}
extern "C" void Play_ClearCamera(PlayState* p, s16 id) { ++cleared; p->cameraPtrs[id] = nullptr; }
extern "C" void Camera_InitPlayerSettings(Camera*, Player*) {}
extern "C" s32 Camera_RequestSetting(Camera*, s16) { return 1; }
extern "C" s32 Play_CameraSetAtEye(PlayState*, s16, Vec3f* at, Vec3f* eye) {
    assert(std::isfinite(at->x) && std::isfinite(eye->z));
    framedAt = *at; framedEye = *eye;
    assert(eye->y > at->y); return 1;
}
extern "C" s32 Play_CameraSetFov(PlayState*, s16, f32 fov) { assert(fov == 55); return 1; }
using namespace TempleSages::ConfirmationNative;
static TempleSages::Pad portal{{0,0,0},0};
static void Setup() {
    Reset(gPlayState);
    play = {}; player = {}; gSaveContext = {};
    gPlayState = &play; gGameState = &play.state;
    play.state.running = true;
    play.sceneSegment = &play;
    play.roomCtx.curRoom.segment = &player;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
    play.cameraPtrs[0] = &play.mainCamera;
    play.mainCamera.uid = 9;
    play.mainCamera.childCamIdx = SUBCAM_FREE;
    mode = 1; failCamera = 0; SetMessageState(TEXT_STATE_NONE); play.msgCtx.msgMode = 0; advance = false;
    opened = closed = acquired = cleared = 0;
}
static void CheckProjectedFraming() {
    // Project the actual production camera into a conservative 4:3 viewport.
    // These are the P3 portal+top-mote bounds and an adult Link at the front
    // trigger edge. Reserve 3 degrees of vertical margin rather than merely
    // testing whether the origin or the visual's center is on screen.
    auto dot = [](Vec3f a, Vec3f b) { return a.x*b.x + a.y*b.y + a.z*b.z; };
    auto cross = [](Vec3f a, Vec3f b) {
        return Vec3f{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};
    };
    auto unit = [&dot](Vec3f a) {
        const float length = std::sqrt(dot(a,a));
        return Vec3f{a.x/length,a.y/length,a.z/length};
    };
    for (int rotation=0; rotation<16; ++rotation) {
        Setup();
        TempleSages::Pad pose{{1000,200,-400},static_cast<s16>(rotation*4096)};
        const float yaw = pose.yaw * (6.28318530718f/65536.0f);
        auto point = [pose,yaw](float x,float y,float z) {
            return Vec3f{pose.position.x+std::cos(yaw)*x+std::sin(yaw)*z,
                         pose.position.y+y,
                         pose.position.z-std::sin(yaw)*x+std::cos(yaw)*z};
        };
        player.actor.world.pos = point(0,0,15);
        assert(Start(&play,rotation%5,false,pose));
        const auto forward = unit({framedAt.x-framedEye.x,framedAt.y-framedEye.y,framedAt.z-framedEye.z});
        const auto right = unit(cross(forward,{0,1,0}));
        const auto up = cross(right,forward);
        auto visible = [&](Vec3f world) {
            const Vec3f relative{world.x-framedEye.x,world.y-framedEye.y,world.z-framedEye.z};
            const float depth = dot(relative,forward);
            assert(depth > 0);
            assert(std::fabs(dot(relative,up))/depth < std::tan(24.5f*3.14159265359f/180.0f));
            assert(std::fabs(dot(relative,right))/depth < std::tan(27.5f*3.14159265359f/180.0f)*4.0f/3.0f*0.9f);
        };
        for (float x : {-60.0f,60.0f})
            for (float y : {0.0f,151.0f}) visible(point(x,y,0));
        for (float x : {-16.0f,16.0f})
            for (float y : {0.0f,70.0f}) visible(point(x,y,15));
    }
    Reset(&play);
}
int main() {
    Setup();
    play.state.input[0].cur.stick_y = 80;
    play.state.input[0].press.button = BTN_A;
    player.linearVelocity = 3.0f; player.actor.speedXZ = 3.0f;
    assert(Start(&play,0,false,portal));
    assert(player.stateFlags1 & PLAYER_STATE1_INPUT_DISABLED);
    assert(player.linearVelocity == 0 && player.actor.speedXZ == 0);
    for (bool form : {false,true}) {
        transformed = form;
        const auto actorInput = NativePlayerInput(&player,&play);
        assert(actorInput.cur.stick_y == 0 && actorInput.press.button == 0);
        assert(play.state.input[0].cur.stick_y == 80 && play.state.input[0].press.button == BTN_A);
    }
    Reset(&play);
    assert(!(player.stateFlags1 & PLAYER_STATE1_INPUT_DISABLED));
    CheckProjectedFraming();
    // Text output is production bytes consumed by native Message_Decode.
    constexpr char colors[] = {0x42,0x41,0x43,0x46,0x45};
    for (int route=0; route<5; ++route) {
        const auto out = TempleSages::ConfirmationNative::Text(route,false), back = TempleSages::ConfirmationNative::Text(route,true);
        assert(out.find(kDestinations[route]) != std::string::npos);
        assert(out[out.find('\x05')+1] == colors[route]);
        assert(back == "Teleport back to the \x05\x44" "Chamber of Sages\x05\x40?\x01\x1BYes\x01No\x02");
        assert(out.ends_with("?\x01\x1BYes\x01No\x02"));
    }
    Setup(); assert(Start(&play,0,false,portal));
    assert(loaded == TempleSages::ConfirmationNative::Text(0,false) && opened == 1 && acquired == 1 && play.activeCamera == 1);
    SetMessageState(TEXT_STATE_CHOICE); advance = true;
    assert(Update(&play) == Result::Pending); // no A is never Yes
    assert(Update(&play) == Result::Pending);
    play.state.input[0].press.button = BTN_A;
    assert(Update(&play) == Result::Yes && Holding() && closed == 1 && cleared == 0 && play.activeCamera == 1);
    assert(!(player.stateFlags1 & PLAYER_STATE1_INPUT_DISABLED));
    play.transitionTrigger = TRANS_TRIGGER_START;
    assert(Update(&play) == Result::Pending && Holding() && cleared == 0);
    Reset(&play,true);
    assert(!Active() && cleared == 1 && play.activeCamera == 0);
    Setup(); assert(Start(&play,2,true,portal));
    SetMessageState(TEXT_STATE_CHOICE); advance = true; play.msgCtx.choiceIndex = 1;
    assert(Update(&play) == Result::Pending);
    play.state.input[0].press.button = BTN_A;
    assert(Update(&play) == Result::No && cleared == 1);
    Setup(); failCamera = 1; assert(Start(&play,0,false,portal));
    play.state.input[0].press.button = BTN_B;
    assert(Update(&play) == Result::No && closed == 1 && cleared == 0);
    Setup(); play.activeCamera = 2;
    assert(Start(&play,0,false,portal) && acquired == 0);
    Reset(&play); assert(play.activeCamera == 2);
    Setup(); assert(Start(&play,0,false,portal));
    player.stateFlags1 |= PLAYER_STATE1_CARRYING_ACTOR;
    ++play.roomCtx.curRoom.num;
    assert(Update(&play) == Result::Abort && cleared == 1 && closed == 1);
    assert(!(player.stateFlags1 & PLAYER_STATE1_INPUT_DISABLED));
    assert(player.stateFlags1 & PLAYER_STATE1_CARRYING_ACTOR);
    Setup(); player.stateFlags1 = PLAYER_STATE1_INPUT_DISABLED;
    assert(!Start(&play,0,false,portal) && (player.stateFlags1 & PLAYER_STATE1_INPUT_DISABLED));
    Setup(); assert(Start(&play,0,false,portal));
    static Player replacement;
    replacement = {}; replacement.stateFlags1 = PLAYER_STATE1_INPUT_DISABLED | PLAYER_STATE1_CARRYING_ACTOR;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &replacement.actor;
    assert(Update(&play) == Result::Abort);
    assert(replacement.stateFlags1 == (PLAYER_STATE1_INPUT_DISABLED | PLAYER_STATE1_CARRYING_ACTOR));
    Setup(); assert(Start(&play,0,false,portal));
    static int replacementScene;
    play.sceneSegment = &replacementScene;
    assert(Update(&play) == Result::Abort);
    assert(player.stateFlags1 & PLAYER_STATE1_INPUT_DISABLED); // recycled scene identity is not ours
    Setup(); assert(Start(&play,0,false,portal));
    player.csAction = PLAYER_CSACTION_8;
    player.stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
    assert(Update(&play) == Result::Abort);
    assert(player.csAction == PLAYER_CSACTION_8 && (player.stateFlags1 & PLAYER_STATE1_IN_CUTSCENE));
    assert(!(player.stateFlags1 & PLAYER_STATE1_INPUT_DISABLED));
    Setup(); assert(Start(&play,0,false,portal)); ++mode;
    assert(Update(&play) == Result::Abort && cleared == 1);
    Setup(); assert(Start(&play,0,false,portal));
    play.msgCtx.textId = 0x1234;
    assert(Update(&play) == Result::Abort && closed == 0 && cleared == 1);
    Setup(); assert(Start(&play,0,false,portal));
    play.cameraPtrs[1]->uid = 99;
    assert(Update(&play) == Result::Abort && cleared == 0); // reused camera is not ours
    Setup(); assert(Start(&play,0,false,portal));
    Reset(&play,true); assert(closed == 1 && cleared == 1);
    Setup(); assert(Start(&play,0,false,portal));
    SetMessageState(TEXT_STATE_CLOSING);
    assert(Update(&play) == Result::No);
    Setup(); assert(Start(&play,0,false,portal));
    SetMessageState(TEXT_STATE_NONE);
    for (int i = 0; i < 15; ++i) assert(Update(&play) == Result::Pending);
    assert(Update(&play) == Result::No && cleared == 1); // failed early textbox never confirms
    Setup(); assert(Start(&play,0,false,portal));
    assert(!Start(&play,1,true,portal) && opened == 1); // one owner, no nested acquisition
    ++gSaveContext.sceneLayer;
    assert(Update(&play) == Result::Abort && cleared == 1);
    Setup(); assert(Start(&play,0,false,portal));
    player.stateFlags1 = PLAYER_STATE1_GETTING_ITEM;
    assert(Update(&play) == Result::Abort && cleared == 1);
    std::puts("PASS native production confirmation: explicit choice, five colors, projected framing, native player input hold and guarded cleanup");
}
