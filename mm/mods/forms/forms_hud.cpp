/*
 * forms_hud.cpp - ImGui overlays for the custom forms (Skijer's NEI): Kafei's stamina wheels over his
 * head and Gerudo's rage bar. Same lazily-registered GuiWindow pattern as CaneWheelHud.cpp.
 */
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <memory>

#include <ship/Context.h>
#include <ship/window/Window.h>
#include <ship/window/gui/Gui.h>
#include <ship/window/gui/GuiWindow.h>
#include <libultraship/libultraship.h>

#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"

extern "C" {
#include "z64.h"
#include "macros.h"
#include "mods/forms/custom_forms.h"
void Actor_GetProjectedPos(PlayState* play, Vec3f* worldPos, Vec3f* projectedPos, f32* invW);
}

namespace {
constexpr float kPi = 3.14159265f;
constexpr float kHeadOffset = 62.0f;

bool ProjectHead(PlayState* play, Player* player, const ImVec2& disp, ImVec2* out, float* invW) {
    Vec3f world = player->actor.world.pos;
    world.y += kHeadOffset;
    Vec3f proj;
    Actor_GetProjectedPos(play, &world, &proj, invW);
    if (*invW <= 0.0f) {
        return false;
    }
    ImGuiViewport* vp = ImGui::GetMainViewport();
    out->x = vp->Pos.x + (proj.x * *invW * 0.5f + 0.5f) * disp.x;
    out->y = vp->Pos.y + (proj.y * *invW * -0.5f + 0.5f) * disp.y;
    return out->x >= vp->Pos.x && out->x <= vp->Pos.x + disp.x && out->y >= vp->Pos.y && out->y <= vp->Pos.y + disp.y;
}

void DrawKafeiStamina(ImDrawList* dl, PlayState* play, Player* player, const ImVec2& disp) {
    ImVec2 center;
    float invW;
    if (!ProjectHead(play, player, disp, &center, &invW)) {
        return;
    }
    const float scale = disp.y / 720.0f;
    float radius = 26.0f * scale * (invW * 220.0f);
    radius = std::clamp(radius, 9.0f * scale, 34.0f * scale);
    const float thickness = radius * 0.28f;
    const bool winded = KafeiForm_IsWinded() != 0;
    const ImU32 fillColor = winded ? IM_COL32(230, 70, 60, 235) : IM_COL32(120, 230, 110, 235);
    for (int w = 0; w < KafeiForm_WheelCount(); w++) {
        const float r = radius - w * (thickness + 2.0f * scale);
        if (r <= 2.0f) {
            break;
        }
        dl->AddCircle(center, r, IM_COL32(0, 0, 0, 140), 64, thickness);
        const float fill = KafeiForm_WheelFill(w);
        if (fill <= 0.001f) {
            continue;
        }
        const float a0 = -kPi * 0.5f;
        dl->PathArcTo(center, r, a0, a0 + fill * 2.0f * kPi, 64);
        dl->PathStroke(fillColor, 0, thickness);
    }
}

void DrawGerudoRage(ImDrawList* dl, const ImVec2& disp) {
    const float scale = disp.y / 720.0f;
    const ImVec2 vp = ImGui::GetMainViewport()->Pos;
    const float w = 180.0f * scale;
    const float h = 12.0f * scale;
    const ImVec2 p0(vp.x + 40.0f * scale, vp.y + disp.y - 70.0f * scale);
    const ImVec2 p1(p0.x + w, p0.y + h);
    const float fill = std::clamp(GerudoForm_RageFill(), 0.0f, 1.0f);
    const ImU32 col = GerudoForm_RageReady() ? IM_COL32(255, 200, 40, 235) : IM_COL32(200, 80, 60, 220);
    dl->AddRectFilled(p0, p1, IM_COL32(0, 0, 0, 140), 3.0f);
    dl->AddRectFilled(p0, ImVec2(p0.x + w * fill, p1.y), col, 3.0f);
    dl->AddRect(p0, p1, IM_COL32(255, 255, 255, 120), 3.0f);
}

class FormsHudWindow : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    void InitElement() override {
    }
    void UpdateElement() override {
    }
    void DrawElement() override {
    }
    // Draw() itself is overridden: the base only reaches DrawElement for windows toggled visible.
    void Draw() override;
};

void FormsHudWindow::Draw() {
    if (gPlayState == nullptr || gPlayState->pauseCtx.state != 0) {
        return;
    }
    const bool kafei = KafeiForm_MeterVisible() != 0;
    const bool gerudo = GerudoForm_RageVisible() != 0;
    if (!kafei && !gerudo) {
        return;
    }
    auto gui = Ship::Context::GetRawInstance()->GetWindow()->GetGui();
    if (gui == nullptr || gui->GetMenuOrMenubarVisible() || ImGui::GetCurrentContext() == nullptr) {
        return;
    }
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImDrawList* dl = (viewport != nullptr) ? ImGui::GetForegroundDrawList(viewport) : nullptr;
    if (dl == nullptr) {
        return;
    }
    const ImVec2 disp = ImGui::GetIO().DisplaySize;
    if (disp.x < 1.0f || disp.y < 1.0f) {
        return;
    }
    Player* player = GET_PLAYER(gPlayState);
    if (kafei && player != nullptr) {
        DrawKafeiStamina(dl, gPlayState, player, disp);
    }
    if (gerudo) {
        DrawGerudoRage(dl, disp);
    }
}

std::shared_ptr<FormsHudWindow> sHudWindow = nullptr;

void EnsureRegistered() {
    if (sHudWindow != nullptr) {
        return;
    }
    auto ctx = Ship::Context::GetRawInstance();
    if (ctx == nullptr || ctx->GetWindow() == nullptr) {
        return;
    }
    auto gui = ctx->GetWindow()->GetGui();
    if (gui == nullptr) {
        return;
    }
    sHudWindow = std::make_shared<FormsHudWindow>("gFormsHud", "Custom Forms HUD");
    gui->AddGuiWindow(sHudWindow);
}

void RegisterFormsHud() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnInterfaceDrawStart>([]() { EnsureRegistered(); });
}
} // namespace

static RegisterShipInitFunc initFunc(RegisterFormsHud, {});
