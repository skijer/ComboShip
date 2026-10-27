#ifndef COMBO_BOTTLE_CONTENTS_DRAW_H
#define COMBO_BOTTLE_CONTENTS_DRAW_H
// Include after the host engine, shared mesh renderer and ComboForeignAnim glue.
#include "ComboBottleContents.h"
#include "../../soh/soh/Enhancements/randomizer/NeiGiRender.h"

static NeiGi::Mesh ComboBottleContents_Gold(uint32_t frame, const NeiGi::Basis& camera) {
    NeiGi::Mesh mesh;
    const NeiGi::Point tip{ 0, -16, 0 }, base{ 0, -23, 0 };
    for (int i = 0; i < 16; ++i) {
        const float a = i * NeiGi::Tau / 16, b = (i + 1) * NeiGi::Tau / 16;
        const NeiGi::Point p{ 12 * std::cos(a), -23, 12 * std::sin(a) };
        const NeiGi::Point q{ 12 * std::cos(b), -23, 12 * std::sin(b) };
        mesh.Tri({ tip, 0xFFD45A, 255 }, { p, 0xA96D16, 255 }, { q, 0xECAE32, 255 });
        mesh.Tri({ base, 0xA96D16, 255 }, { q, 0xECAE32, 255 }, { p, 0xA96D16, 255 });
    }
    for (int i = 0; i < 12; ++i) {
        const float phase = float((frame % 240u + i * 20u) % 240u) / 240.f;
        const float angle = i * 2.399963f + (frame % 720u) * .004f;
        const NeiGi::Point p{ 9 * std::cos(angle), -21 + 24 * phase, 9 * std::sin(angle) };
        NeiGi::Glow(mesh, p, .8f, 0xFFD45A, uint8_t(210 * std::sin(phase * NeiGi::Tau * .5f)), camera);
    }
    return mesh;
}

extern "C" int ComboBottleContents_Draw(PlayState* play, int content) {
    if (!play || !play->state.gfxCtx || content < CW_BOTTLE_MUSHROOM || content > CW_BOTTLE_SEAHORSE)
        return 0;
#ifdef COMBO_BOTTLE_HOST_MM
    const char* owner = "mm";
#else
    const char* owner = "oot";
#endif
    // All accepted fitted meshes and the preserved casing are in both hosts.
    // Active-host selection also honors a selected replacement of a marker or
    // private mesh; no resource load consults the dormant game's Alt setting.
    const char* marker = ComboBottleContents_Marker(content);
    const char* shell = "__OTR__objects/combo_bottle_gi/BottleShell";
    const char* mesh = ComboBottleContents_MeshPath(content);
    // Check the complete static footprint before a resource failure could
    // hand this exhausted arena to a native fallback.
    if (!NeiGi_CanDrawLayers(play, mesh ? 2 : 1, 32, 40))
        return 1;
    try {
        const bool replacement = ResourceMgr_IsModAsset(marker) != 0;
        const char* root = replacement ? marker : mesh;
        if (!replacement && mesh) {
            for (int i = 0; const char* dependency = ComboBottleContents_Dependency(content, i); ++i)
                if (!ResourceMgr_FileExists(dependency) &&
                    !(ResourceMgr_IsAltAssetsEnabled() && ResourceMgr_FileAltExists(dependency)))
                    return 0;
        }
        if ((!replacement && !ResourceMgr_LoadGfxByName(shell)) || (root && !ResourceMgr_LoadGfxByName(root)))
            return 0;
        // A suppressed draw must not fall back to an unchecked native allocator
        // in the same exhausted arena. Resource failures still permit fallback.
        if (root) {
            OPEN_DISPS(play->state.gfxCtx);
            if (replacement || content == CW_BOTTLE_PRINCESS) {
                CFA_SETUP_XLU(play->state.gfxCtx);
                gSPGrayscale(POLY_XLU_DISP++, false);
                gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
                gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
                gSPComboRMPush(POLY_XLU_DISP++, owner);
                CFA_LOAD_MTX(POLY_XLU_DISP++, play->state.gfxCtx);
                gSPDisplayList(POLY_XLU_DISP++, (Gfx*)root);
                gSPComboRMPop(POLY_XLU_DISP++);
                CFA_SETUP_XLU(play->state.gfxCtx);
                gSPGrayscale(POLY_XLU_DISP++, false);
                gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
                gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
            } else {
                CFA_SETUP_OPA(play->state.gfxCtx);
                gSPGrayscale(POLY_OPA_DISP++, false);
                gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, 255);
                gDPSetEnvColor(POLY_OPA_DISP++, 255, 255, 255, 255);
                gSPComboRMPush(POLY_OPA_DISP++, owner);
                CFA_LOAD_MTX(POLY_OPA_DISP++, play->state.gfxCtx);
                gSPDisplayList(POLY_OPA_DISP++, (Gfx*)root);
                gSPComboRMPop(POLY_OPA_DISP++);
                CFA_SETUP_OPA(play->state.gfxCtx);
                gSPGrayscale(POLY_OPA_DISP++, false);
                gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, 255);
                gDPSetEnvColor(POLY_OPA_DISP++, 255, 255, 255, 255);
            }
            CLOSE_DISPS(play->state.gfxCtx);
        } else {
            // Gold geometry, clock and reset rules are the accepted production
            // implementation. Reserve its later casing and state restoration.
            NeiGi_DrawMeshWithTail(play, ComboBottleContents_Gold(play->gameplayFrames, NeiGi_CameraBasis(play)), owner,
                                   1, 32, 40);
        }
        if (!replacement) {
            // Neutral glass is last at the incoming GI pose; each mesh's own
            // resource matrix pushes/pops its coordinate conversion internally.
            OPEN_DISPS(play->state.gfxCtx);
            CFA_SETUP_XLU(play->state.gfxCtx);
            gSPGrayscale(POLY_XLU_DISP++, false);
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
            gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
            gSPComboRMPush(POLY_XLU_DISP++, owner);
            CFA_LOAD_MTX(POLY_XLU_DISP++, play->state.gfxCtx);
            gSPDisplayList(POLY_XLU_DISP++, (Gfx*)shell);
            gSPComboRMPop(POLY_XLU_DISP++);
            CFA_SETUP_XLU(play->state.gfxCtx);
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);
            gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);
            CLOSE_DISPS(play->state.gfxCtx);
        }
        return 1;
    } catch (...) {
        return 0; // Resource exceptions must not cross the native C draw ABI.
    }
}
#endif
