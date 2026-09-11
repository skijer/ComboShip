/*
 * adult_link_render.cpp - render OoT adult Link over MM's human Link (Skijer's NEI).
 * See adult_link_render.h for the design.
 *
 * The skeleton is OoT's gLinkAdultSkel loaded pre-parsed from the un-indexed oot.o2r via
 * MmAssets_LoadFromOotArchive; each limb DL is deep-patched via OotAssets_LoadGfxDirect so its
 * vtx/tex are inlined and resolve at draw time. We draw through the ENGINE's real Player_DrawImpl +
 * Player_PostLimbDrawGameplay so held items (sword, shield, bow, hookshot, ...), trails, colliders
 * and the shield/hookshot logic all work — but with a CUSTOM overrideLimbDraw that wraps the vanilla
 * Player_OverrideLimbDrawGameplayDefault and re-points the four equipment limbs (both hands, the
 * sheath, the waist) to OoT-ADULT DLs chosen from player->leftHandType/rightHandType/sheath state.
 * (For the Human form the vanilla override reads hardcoded MM-child hand arrays, so repointing
 * player->*HandDLists alone is NOT enough — the override callback is the clean seam.)
 *
 * New .cpp -> registered in build/x64/mm/2ship.vcxproj directly (do NOT run cmake).
 */

#include "adult_link_render.h"

#include <libultraship/bridge.h>
#include <spdlog/spdlog.h>
#include <math.h>
#include <string>
#include <cstring>
#include <cstdlib>

extern "C" {
#include "macros.h"
#include "functions.h"
#include "variables.h"
#include "z64lib.h"                                 // Lib_SegmentedToVirtual
#include "mods/oot_asset_loader/oot_asset_loader.h" // OotAssets_LoadGfxDirect / LoadTexOrDList / Available
#include "mods/nei_save.h"                          // Nei_Save()->timeGateAdultMode

// Loads an OoT resource already-parsed straight off the oot.o2r archive handle, bypassing the
// (rejected) global index. Defined in mods/transformation_masks/assets/mm_asset_loader.cpp.
void* MmAssets_LoadFromOotArchive(const char* path, size_t* outSize);

// Alt-assets (mod / texture-pack) resolution so appearance mods can override the adult model. BenPort.
// ResourceMgr_LoadGfxByName dereferences a null resource -> only call it once FileExists confirms the
// "alt/" path is present.
bool ResourceMgr_IsAltAssetsEnabled(void);
uint8_t ResourceMgr_FileExists(const char* path);
Gfx* ResourceMgr_LoadGfxByName(const char* path);
// Loads a skeleton by name; with alt-assets on it resolves the "alt/" (mod) version first. Returns the
// header or NULL (e.g. an un-indexed oot.o2r-only base with no mod present). See HarpoonDummyPlayer.
SkeletonHeader* ResourceMgr_LoadSkeletonByName(const char* path, SkelAnime* skelAnime);
char* ResourceMgr_LoadTexOrDListByName(const char* path); // load an indexed texture (e.g. a mod's alt/ eye tex)
char** ResourceMgr_ListFiles(const char* searchMask, int* resultSize); // enumerate archive files (for mod discovery)

// MM per-form eye/mouth texture tables that Player_DrawImpl binds to segments 0x08/0x09. We swap the
// Human slots to the OoT-adult textures for the draw so the adult head samples the right eyes.
extern TexturePtr sPlayerEyesTextures[PLAYER_FORM_MAX][PLAYER_EYES_MAX];
extern TexturePtr sPlayerMouthTextures[PLAYER_FORM_MAX][PLAYER_MOUTH_MAX];

// Per-form GAMEPLAY attributes (z_player.c:787). player->ageProperties points at [transformation], so
// overriding the Human slot in place makes the running Human Link play tall — ledge-grab reach
// (unk_14/18/1C vs yDistToLedge), ceiling clearance, wall radius, body height. This is the GAMEPLAY half
// of a custom form (the visual half is the skeleton/eye swap); the collider HEIGHT auto-recomputes from
// the taller adult body-part positions, so only these scalars need scaling.
extern PlayerAgeProperties sPlayerAgeProperties[PLAYER_FORM_MAX];

// NEI custom-item in-hand draw dispatcher (beetle, cane of somaria, ball&chain, ...). In the vanilla
// path it runs AFTER the skeleton draw (which we skip via the early return), reading the world-space
// hand positions the skeleton draw wrote. custom_items.h:611, extern "C".
s32 CustomItems_OverrideDraw(Player* player, PlayState* play);

// NEI extended-equipment shield override: the OTR path of a custom shield's back DL (Divine/Kite/Ikana),
// or NULL for a vanilla shield. extended_equipment.h:392. Used so PostLimb keeps drawing ext shields on
// the back while we suppress the duplicate MM-CHILD shield the adult sheath DL already bakes in.
const char* ExtEquip_GetShieldDLOverride(void);

// Custom forms (Kafei/Keaton/Gerudo/Garo): each is an adult-rigged mirror of object_link_boy under
// objects/forms/<name>/, so this renderer draws them by swapping its model source to the form's base path.
#include "mods/forms/custom_forms.h"
// Garo has NO Link-rig mirror: it draws MM's native ghost-ninja skeleton (En_Jso's rig) over a
// null-DL walk of the Link skeleton that keeps positions/shadow/colliders alive.
#include "objects/object_jso/object_jso.h"

// OPEN_DISPS re-declares these at block scope with C++ linkage in a .cpp; the FIRST declaration wins,
// so pin them to C linkage here or the link fails with LNK2001 (see reference_open_disps_cpp_linkage).
void FrameInterpolation_RecordOpenChild(const void* a, int b);
void FrameInterpolation_RecordCloseChild(void);
}

#define ADULT_LINK_SCALE_DEFAULT 0.01f
#define ADULT_LINK_COLLIDER_HEIGHT_DEFAULT 90
#define ADULT_LINK_LIMB_COUNT 21

// OTR path helper (all adult DLs live under this object).
#define ALB(sym) "__OTR__objects/object_link_boy/" sym

// Limb index -> near display list OTR path. NULL = control bone (no geometry) -> drawn invisible.
static const char* sLimbNearDL[ADULT_LINK_LIMB_COUNT] = {
    /* 0  Root          */ NULL,
    /* 1  Waist         */ ALB("gLinkAdultWaistNearDL"),
    /* 2  LowerControl  */ NULL,
    /* 3  RightThigh    */ ALB("gLinkAdultRightThighNearDL"),
    /* 4  RightLeg      */ ALB("gLinkAdultRightLegNearDL"),
    /* 5  RightFoot     */ ALB("gLinkAdultRightFootNearDL"),
    /* 6  LeftThigh     */ ALB("gLinkAdultLeftThighNearDL"),
    /* 7  LeftLeg       */ ALB("gLinkAdultLeftLegNearDL"),
    /* 8  LeftFoot      */ ALB("gLinkAdultLeftFootNearDL"),
    /* 9  UpperControl  */ NULL,
    /* 10 Head          */ ALB("gLinkAdultHeadNearDL"),
    /* 11 Hat           */ ALB("gLinkAdultHatNearDL"),
    /* 12 Collar        */ ALB("gLinkAdultCollarNearDL"),
    /* 13 LeftShoulder  */ ALB("gLinkAdultLeftShoulderNearDL"),
    /* 14 LeftArm       */ ALB("gLinkAdultLeftArmNearDL"),
    /* 15 LeftHand      */ ALB("gLinkAdultLeftHandNearDL"),
    /* 16 RightShoulder */ ALB("gLinkAdultRightShoulderNearDL"),
    /* 17 RightArm      */ ALB("gLinkAdultRightArmNearDL"),
    /* 18 RightHand     */ ALB("gLinkAdultRightHandNearDL"),
    /* 19 SwordAndSheath*/ NULL, // driven by the override at PLAYER_LIMB_SHEATH instead
    /* 20 Torso         */ ALB("gLinkAdultTorsoNearDL"),
};

// ---- one-time-loaded state ----
static FlexSkeletonHeader* sSkel = NULL;
static u8 sReady = 0;
static u8 sSetupFailedPermanently = 0;
static u8 sIsMod = 0;      // 1 = a user alt-assets mod supplied the skeleton (full model replacement)
static u8 sIsForm = 0;     // 1 = a custom form (objects/forms/<name>/) supplied the skeleton
static u8 sIsChildRig = 0; // 1 = the form's object_link_child mirror is loaded (Time Gate off = child age)
static SkelAnime sGaroSkelAnime;
static Vec3s sGaroJointTable[GARO_LIMB_MAX];
static Vec3s sGaroMorphTable[GARO_LIMB_MAX];
static u8 sGaroReady = 0;

// Equipment DLs chosen at draw time by the override (deep-patched adult DLs).
static Gfx* sDL_LHOpen;
static Gfx* sDL_LHClosed;
static Gfx* sDL_LHSword; // holding one-hand sword (Master Sword)
static Gfx* sDL_LHBgs;   // two-hand (Biggoron/Great Fairy's)
static Gfx* sDL_RHOpen;
static Gfx* sDL_RHClosed;
static Gfx* sDL_RHShield; // shield raised in hand (Hylian)
static Gfx* sDL_RHBow;
static Gfx* sDL_RHOcarina;
static Gfx* sDL_RHHookshot;
static Gfx* sDL_Waist;
static Gfx* sDL_SheathEmpty;       // empty scabbard, no shield on back
static Gfx* sDL_SheathSword;       // sword in scabbard, no shield on back
static Gfx* sDL_SheathShield;      // empty scabbard + Hylian shield on back
static Gfx* sDL_SheathBoth;        // sword in scabbard + Hylian shield on back
static Gfx* sDL_RHMirrorShield;    // Mirror shield raised in hand
static Gfx* sDL_SheathMirror;      // empty scabbard + Mirror shield on back
static Gfx* sDL_SheathMirrorSword; // sword in scabbard + Mirror shield on back

// A MOD head mesh binds its own eye/mouth via segments 0x08/0x09 (recomp/Fast64 preserves the vanilla
// structure). We DISCOVER the mod's eye/mouth textures generically by matching each blink state as a
// substring of the mod's alt-object filenames — so any exporter's suffix works, not just one mod. The
// state order maps to MM's PlayerEyes index (open/half/closed used for blinking; the rest positional).
// Canonical OoT adult-Link eye/mouth resource names (object_link_boy). A mod that ships gLinkAdultSkel
// almost always ships these under the same object with the same names, so we can load each slot by NAME
// directly — robust across archive formats (the new .o2r doesn't match a ResourceMgr_ListFiles wildcard,
// which returned 0 and left every slot on the MM-child eye = garbage on the adult head).
static const char* sOotEyeName[PLAYER_EYES_MAX] = {
    "gLinkAdultEyesOpenTex",      "gLinkAdultEyesHalfTex",  "gLinkAdultEyesClosedfTex", "gLinkAdultEyesRollLeftTex",
    "gLinkAdultEyesRollRightTex", "gLinkAdultEyesShockTex", "gLinkAdultEyesUnk1Tex",    "gLinkAdultEyesUnk2Tex",
};
static const char* sOotMouthName[PLAYER_MOUTH_MAX] = {
    "gLinkAdultMouth1Tex",
    "gLinkAdultMouth2Tex",
    "gLinkAdultMouth3Tex",
    "gLinkAdultMouth4Tex",
};
// CHILD-rig twins (forms ship both ages; the Time Gate picks which, mirroring OoT's linkAge).
static const char* sOotChildEyeName[PLAYER_EYES_MAX] = {
    "gLinkChildEyesOpenTex",      "gLinkChildEyesHalfTex",  "gLinkChildEyesClosedfTex", "gLinkChildEyesRollLeftTex",
    "gLinkChildEyesRollRightTex", "gLinkChildEyesShockTex", "gLinkChildEyesUnk1Tex",    "gLinkChildEyesUnk2Tex",
};
static const char* sOotChildMouthName[PLAYER_MOUTH_MAX] = {
    "gLinkChildMouth1Tex",
    "gLinkChildMouth2Tex",
    "gLinkChildMouth3Tex",
    "gLinkChildMouth4Tex",
};
// sPlayerEyesTextures holds OTR PATH STRINGS (e.g. "__OTR__objects/.../gLinkHumanEyesOpenTex"), which
// the gfx interpreter resolves at draw time — NOT raw pixel pointers. So we store the mod eye/mouth
// PATHS (with the __OTR__ prefix) and hand those to Player_DrawImpl, letting it resolve the mod texture.
static std::string sModEyePath[PLAYER_EYES_MAX];
static std::string sModMouthPath[PLAYER_MOUTH_MAX];
// RAW pixel pointers for the mod eye/mouth. We bind these directly on segments 0x08/0x09 instead of a
// path string: MM's own eye PATHS resolve only because their base is indexed; a mod's texture exists
// ONLY as alt/... (no indexed base), and the interpreter's GetBaseTexturePath strips the "alt/" ->
// un-indexed base -> fails. Raw pixels (gfx_check_image_signature sees non-__OTR__ data) bypass all of
// that — exactly how a zobj/ML64 swap feeds the eye texels from the object.
static void* sModEye[PLAYER_EYES_MAX];
static void* sModMouth[PLAYER_MOUTH_MAX];

// Load an adult-Link DL, preferring a user mod / texture-pack: if the "alt/" override of this resource
// is present (indexed), use it so appearance mods apply; otherwise deep-patch the oot.o2r base (which
// the alt-assets index can't reach). Only mod DLs that ship the FULL display list get overridden here;
// texture-only packs don't apply because the deep-patch inlines oot.o2r's base textures.
// Mirror base of the rig the current setup is loading ("objects/forms/<name>/object_link_<age>/"),
// empty when no form is active. Set by AdultLink_Setup before the equipment loads run.
static std::string sRigBase;

static Gfx* AdultLink_LoadDL(const char* otrPath) {
    const char* p = otrPath;
    if (std::strncmp(p, "__OTR__", 7) == 0) {
        p += 7;
    }
    const char* sym = std::strrchr(p, '/');
    sym = (sym != NULL) ? sym + 1 : p;
    if (!sRigBase.empty()) {
        std::string formPath = sRigBase + sym;
        if (ResourceMgr_FileExists(formPath.c_str())) {
            Gfx* dl = ResourceMgr_LoadGfxByName(formPath.c_str());
            if (dl != NULL) {
                return dl;
            }
        }
    }
    if (ResourceMgr_IsAltAssetsEnabled()) {
        std::string altPath = std::string("alt/") + p; // gAltAssetPrefix == "alt/"
        if (ResourceMgr_FileExists(altPath.c_str())) {
            Gfx* mod = ResourceMgr_LoadGfxByName(altPath.c_str());
            if (mod != NULL) {
                return mod;
            }
        }
    }
    Gfx* dl = (Gfx*)OotAssets_LoadGfxDirect(otrPath);
    if (dl == NULL) {
        SPDLOG_WARN("[AdultLink] DL load FAILED: {}", otrPath);
    }
    return dl;
}

// A form/mode change invalidates every cached model resource; the next draw re-runs the setup with the
// new model source.
extern "C" void AdultLink_OnFormChanged(void) {
    sReady = 0;
    sSetupFailedPermanently = 0;
    sSkel = NULL;
    sIsMod = 0;
    sIsForm = 0;
    sIsChildRig = 0;
    sRigBase.clear();
    sGaroReady = 0;
    for (int i = 0; i < PLAYER_EYES_MAX; i++) {
        sModEyePath[i].clear();
        sModEye[i] = NULL;
    }
    for (int i = 0; i < PLAYER_MOUTH_MAX; i++) {
        sModMouthPath[i].clear();
        sModMouth[i] = NULL;
    }
}

static s32 AdultLink_Setup(void) {
    if (sReady) {
        return 1;
    }
    if (sSetupFailedPermanently) {
        return 0;
    }

    // Prefer a user MOD / alt-assets skeleton — a full appearance replacement (e.g. a recomp/Fast64
    // model whose .otr in the mods folder ships alt/objects/object_link_boy/gLinkAdultSkel + its own
    // limb meshes/textures). ResourceMgr_LoadSkeletonByName resolves the alt/ version through the normal
    // index; its limbs already reference the mod's indexed meshes, so nothing needs deep-patching. When
    // no mod is present it returns NULL (the oot.o2r base isn't indexed) and we deep-patch the base.
    FlexSkeletonHeader* skel = NULL;
    // Active custom form first: its self-contained 21-limb mirror skeleton (mods/nei_forms.o2r) already
    // references its own indexed limb meshes — same contract as a mod skeleton, nothing to deep-patch.
    const char* formBase = CustomForms_BasePath();
    std::string formRigBase = (formBase != NULL) ? formBase : "";
    if (formBase != NULL) {
        // The Time Gate is MM's linkAge: gate off = the form's object_link_child mirror (when shipped).
        if (!AdultLink_IsActive()) {
            std::string childBase = formRigBase;
            size_t boy = childBase.find("object_link_boy");
            if (boy != std::string::npos) {
                childBase.replace(boy, strlen("object_link_boy"), "object_link_child");
                if (ResourceMgr_FileExists((childBase + "gLinkChildSkel").c_str())) {
                    formRigBase = childBase;
                    sIsChildRig = 1;
                }
            }
        }
        std::string formSkel = formRigBase + (sIsChildRig ? "gLinkChildSkel" : "gLinkAdultSkel");
        skel = (FlexSkeletonHeader*)ResourceMgr_LoadSkeletonByName(formSkel.c_str(), NULL);
        sIsForm = (skel != NULL) ? 1 : 0;
        if (!sIsForm) {
            sIsChildRig = 0;
            static u8 loggedFormMiss = 0;
            if (!loggedFormMiss) {
                loggedFormMiss = 1;
                SPDLOG_WARN("[AdultLink] form skeleton {} not found — falling back to adult Link", formSkel);
            }
        }
    }
    if (skel == NULL && ResourceMgr_IsAltAssetsEnabled()) {
        skel = (FlexSkeletonHeader*)ResourceMgr_LoadSkeletonByName("objects/object_link_boy/gLinkAdultSkel", NULL);
    }
    sIsMod = (!sIsForm && skel != NULL) ? 1 : 0;

    if (!sIsMod && !sIsForm) {
        // Base skeleton from the un-indexed oot.o2r (self-recovering archive-scoped load).
        skel = (FlexSkeletonHeader*)MmAssets_LoadFromOotArchive("objects/object_link_boy/gLinkAdultSkel", NULL);
        if (skel == NULL) {
            static u8 logged = 0;
            if (!logged) {
                logged = 1;
                SPDLOG_WARN("[AdultLink] gLinkAdultSkel not found (mod alt + oot.o2r) — retrying");
            }
            return 0;
        }
    }

    SPDLOG_INFO("[AdultLink] skeleton {} (mod={}) -> limbCount={} dListCount={}", (void*)skel, (int)sIsMod,
                (int)skel->sh.limbCount, (int)skel->dListCount);

    if (skel->sh.limbCount != ADULT_LINK_LIMB_COUNT) {
        sSetupFailedPermanently = 1;
        SPDLOG_ERROR("[AdultLink] unexpected limbCount {} (expected {}) — adult mode disabled", (int)skel->sh.limbCount,
                     ADULT_LINK_LIMB_COUNT);
        return 0;
    }

    if (!sIsMod && !sIsForm) {
        // Base skeleton: its limb DLs point at un-indexed oot.o2r assets — swap each for a self-contained
        // deep-patched Gfx*. (A mod/form skeleton's limbs already point at indexed meshes — leave them.)
        int loaded = 0, failed = 0;
        for (int i = 0; i < ADULT_LINK_LIMB_COUNT; i++) {
            LodLimb* limb = (LodLimb*)skel->sh.segment[i];
            if (limb == NULL) {
                continue;
            }
            const char* nearPath = sLimbNearDL[i];
            if (nearPath == NULL) {
                limb->dLists[0] = NULL;
                limb->dLists[1] = NULL;
                continue;
            }
            Gfx* dl = AdultLink_LoadDL(nearPath);
            limb->dLists[0] = dl;
            limb->dLists[1] = dl;
            (dl != NULL) ? loaded++ : failed++;
        }
        // Eye/mouth off oot.o2r as RAW texels for ALL 8 eye + 4 mouth slots. The deep-patched base head DL
        // still references segments 0x08/0x09 (OoT's adult head, like MM, samples the eye/mouth from those
        // segments), so we MUST bind an adult eye there — otherwise the segments keep MM's CHILD eye/mouth
        // and the adult face renders garbled. Bound at draw time (see AdultLink_Draw).
        int nEye = 0, nMouth = 0;
        for (int i = 0; i < PLAYER_EYES_MAX; i++) {
            std::string base = std::string("objects/object_link_boy/") + sOotEyeName[i];
            sModEye[i] = MmAssets_LoadFromOotArchive(base.c_str(), NULL);
            if (sModEye[i] != NULL) {
                nEye++;
            }
        }
        for (int i = 0; i < PLAYER_MOUTH_MAX; i++) {
            std::string base = std::string("objects/object_link_boy/") + sOotMouthName[i];
            sModMouth[i] = MmAssets_LoadFromOotArchive(base.c_str(), NULL);
            if (sModMouth[i] != NULL) {
                nMouth++;
            }
        }
        SPDLOG_INFO("[AdultLink] base deep-patch: {} limb DLs loaded, {} failed; {}/{} eye, {}/{} mouth off oot.o2r",
                    loaded, failed, nEye, (int)PLAYER_EYES_MAX, nMouth, (int)PLAYER_MOUTH_MAX);
    } else {
        // Mod skeleton: load its eye/mouth textures BY CANONICAL NAME (the head materials bind them via
        // segments 0x08/0x09). We bind the BASE path on the segment and let the interpreter's alt-assets
        // redirect resolve it to the mod's alt/ texture — exactly how MM's own forms resolve their eyes,
        // and robust across archive formats (ResourceMgr_ListFiles returned 0 on the new .o2r format,
        // which left every slot on the MM-child eye). Any slot the mod lacks falls back to the oot.o2r
        // base adult eye (raw pixels) — a real adult eye, never the child garbage.
        int nEye = 0, nMouth = 0, firstEye = -1, firstMouth = -1;
        // A FORM ships its eyes at its own indexed mirror path (bound directly); a MOD ships them at
        // alt/ (probed there, bound as the base path so the interpreter's alt-redirect resolves them).
        const char* const* eyeNames = sIsChildRig ? sOotChildEyeName : sOotEyeName;
        const char* const* mouthNames = sIsChildRig ? sOotChildMouthName : sOotMouthName;
        std::string ootBase = sIsChildRig ? "objects/object_link_child/" : "objects/object_link_boy/";
        std::string probeBase = sIsForm ? formRigBase : std::string("alt/objects/object_link_boy/");
        std::string bindBase =
            sIsForm ? (std::string("__OTR__") + formRigBase) : std::string("__OTR__objects/object_link_boy/");
        for (int i = 0; i < PLAYER_EYES_MAX; i++) {
            std::string probe = probeBase + eyeNames[i];
            if (ResourceMgr_FileExists(probe.c_str())) {
                sModEyePath[i] = bindBase + eyeNames[i];
                if (firstEye < 0) {
                    firstEye = i;
                }
                nEye++;
            } else {
                // oot.o2r twin as raw texels (un-indexed archive; bound directly on the segment).
                sModEye[i] = MmAssets_LoadFromOotArchive((ootBase + eyeNames[i]).c_str(), NULL);
            }
        }
        for (int i = 0; i < PLAYER_MOUTH_MAX; i++) {
            std::string probe = probeBase + mouthNames[i];
            if (ResourceMgr_FileExists(probe.c_str())) {
                sModMouthPath[i] = bindBase + mouthNames[i];
                if (firstMouth < 0) {
                    firstMouth = i;
                }
                nMouth++;
            } else {
                sModMouth[i] = MmAssets_LoadFromOotArchive((ootBase + mouthNames[i]).c_str(), NULL);
            }
        }
        // Any still-empty slot (no mod texture, no oot.o2r base) -> reuse the first slot the mod DID ship,
        // so no eye/mouth index can ever fall through to the MM-child texture.
        if (firstEye >= 0) {
            for (int i = 0; i < PLAYER_EYES_MAX; i++) {
                if (sModEyePath[i].empty() && sModEye[i] == NULL) {
                    sModEyePath[i] = sModEyePath[firstEye];
                }
            }
        }
        if (firstMouth >= 0) {
            for (int i = 0; i < PLAYER_MOUTH_MAX; i++) {
                if (sModMouthPath[i].empty() && sModMouth[i] == NULL) {
                    sModMouthPath[i] = sModMouthPath[firstMouth];
                }
            }
        }
        SPDLOG_INFO("[AdultLink] mod textures (by-name): {}/{} eye, {}/{} mouth from mod alt (rest -> oot.o2r base)",
                    nEye, (int)PLAYER_EYES_MAX, nMouth, (int)PLAYER_MOUTH_MAX);

        // GROUND-TRUTH probe: this mod's CI8 skin/face materials load their TLUT (palette) from OoT base
        // resources (object_link_boyTLUT_*, gLinkAdultHeadTLUT) that live in oot.o2r, NOT in the mod. In soh
        // those are indexed (base game); in 2ship they resolve ONLY if oot.o2r is truly in the name index.
        // If they DON'T resolve, every CI8 skin/face texture renders palette-less = gray skin + black face
        // (exactly the reported symptom). Log whether each resolves so we know if that's the cause.
        static const char* sProbe[] = {
            "objects/object_link_boy/object_link_boyTLUT_005800", // boots/gauntlets/shoe palette
            "objects/object_link_boy/gLinkAdultHeadTLUT",         // FACE-skin palette (adult_ear)
            "objects/object_link_boy/sAdultEyes",                 // eye palette (mod-shipped, should exist)
            "objects/object_link_boy/gLinkHumanSkinTLUT",         // skin palette (mod-shipped)
        };
        for (int i = 0; i < (int)(sizeof(sProbe) / sizeof(sProbe[0])); i++) {
            std::string base = std::string("__OTR__") + sProbe[i];
            std::string alt = std::string("__OTR__alt/") + sProbe[i];
            void* basePtr =
                ResourceMgr_FileExists(sProbe[i]) ? (void*)ResourceMgr_LoadTexOrDListByName(base.c_str()) : NULL;
            std::string altc = std::string("alt/") + sProbe[i];
            int altExists = (int)ResourceMgr_FileExists(altc.c_str());
            void* ootPtr = MmAssets_LoadFromOotArchive(sProbe[i], NULL);
            SPDLOG_INFO("[AdultLink] TLUT probe '{}': baseIndexed={} basePtr={} altInMod={} ootArchive={}", sProbe[i],
                        (int)ResourceMgr_FileExists(sProbe[i]), basePtr, altExists, ootPtr);
        }
    }

    sRigBase = sIsForm ? formRigBase : "";

    if (sIsChildRig) {
        // Child rig: MM's own child equipment already fits, so the hand overrides stay off (NULL lets the
        // vanilla override result stand while holding); only the back setup uses the form's child combos.
        sDL_LHOpen = NULL;
        sDL_LHClosed = NULL;
        sDL_LHSword = NULL;
        sDL_LHBgs = NULL;
        sDL_RHOpen = NULL;
        sDL_RHClosed = NULL;
        sDL_RHShield = NULL;
        sDL_RHBow = NULL;
        sDL_RHOcarina = NULL;
        sDL_RHHookshot = NULL;
        sDL_Waist = NULL;
        sDL_RHMirrorShield = NULL;
        sDL_SheathMirror = NULL;
        sDL_SheathMirrorSword = NULL;
        sDL_SheathEmpty = AdultLink_LoadDL("__OTR__objects/object_link_child/gLinkChildSheathNearDL");
        sDL_SheathSword = AdultLink_LoadDL("__OTR__objects/object_link_child/gLinkChildSwordAndSheathNearDL");
        sDL_SheathShield = AdultLink_LoadDL("__OTR__objects/object_link_child/gLinkChildHylianShieldAndSheathNearDL");
        sDL_SheathBoth =
            AdultLink_LoadDL("__OTR__objects/object_link_child/gLinkChildHylianShieldSwordAndSheathNearDL");

        sSkel = skel;
        sReady = 1;
        SPDLOG_INFO("[AdultLink] setup OK (form child rig, base={})", formRigBase);
        return 1;
    }

    // Equipment DLs the override swaps in for held weapons (both paths). A mod usually lacks holding
    // variants, so these fall to the oot.o2r base; AdultLink_LoadDL prefers a mod alt/ when present.
    sDL_LHOpen = AdultLink_LoadDL(ALB("gLinkAdultLeftHandNearDL"));
    sDL_LHClosed = AdultLink_LoadDL(ALB("gLinkAdultLeftHandClosedNearDL"));
    sDL_LHSword = AdultLink_LoadDL(ALB("gLinkAdultLeftHandHoldingMasterSwordNearDL"));
    sDL_LHBgs = AdultLink_LoadDL(ALB("gLinkAdultLeftHandHoldingBgsNearDL"));
    sDL_RHOpen = AdultLink_LoadDL(ALB("gLinkAdultRightHandNearDL"));
    sDL_RHClosed = AdultLink_LoadDL(ALB("gLinkAdultRightHandClosedNearDL"));
    sDL_RHShield = AdultLink_LoadDL(ALB("gLinkAdultRightHandHoldingHylianShieldNearDL"));
    sDL_RHBow = AdultLink_LoadDL(ALB("gLinkAdultRightHandHoldingBowNearDL"));
    sDL_RHOcarina = AdultLink_LoadDL(ALB("gLinkAdultRightHandHoldingOotNearDL"));
    sDL_RHHookshot = AdultLink_LoadDL(ALB("gLinkAdultRightHandHoldingHookshotNearDL"));
    sDL_Waist = AdultLink_LoadDL(ALB("gLinkAdultWaistNearDL"));
    sDL_SheathEmpty = AdultLink_LoadDL(ALB("gLinkAdultSheathNearDL"));
    sDL_SheathSword = AdultLink_LoadDL(ALB("gLinkAdultMasterSwordAndSheathNearDL"));
    sDL_SheathShield = AdultLink_LoadDL(ALB("gLinkAdultHylianShieldAndSheathNearDL"));
    sDL_SheathBoth = AdultLink_LoadDL(ALB("gLinkAdultHylianShieldSwordAndSheathNearDL"));
    sDL_RHMirrorShield = AdultLink_LoadDL(ALB("gLinkAdultRightHandHoldingMirrorShieldNearDL"));
    sDL_SheathMirror = AdultLink_LoadDL(ALB("gLinkAdultMirrorShieldAndSheathNearDL"));
    sDL_SheathMirrorSword = AdultLink_LoadDL(ALB("gLinkAdultMirrorShieldSwordAndSheathNearDL"));

    sSkel = skel;
    sReady = 1;
    SPDLOG_INFO("[AdultLink] setup OK (mod={})", (int)sIsMod);
    return 1;
}

// Custom limb-draw override: run the vanilla logic (matrices, body-part tracking, hand-type caching,
// leg adjust, upper-limb rot) then re-point the four equipment limbs to adult DLs. Everything the
// PostLimb pass keys off (player->*Type) is untouched, so trails/colliders/reticle still work.
static s32 AdultLink_OverrideLimb(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot, Actor* actor) {
    // Snapshot the skeleton's OWN limb DL BEFORE the vanilla override runs. For a mod skeleton these are
    // the mod's meshes (waist / hands / sword+sheath); for the base they're the deep-patched adult DLs.
    Gfx* skelDL = *dList;
    s32 ret = Player_OverrideLimbDrawGameplayDefault(play, limbIndex, dList, pos, rot, actor);
    Player* p = (Player*)actor;

    // A short-legged form (Keaton 0.335) scales the ROOT joint translation so Link's animations sit at
    // the form's hip height — the mesh itself is authored at the right size (soh custom_forms rule).
    if (limbIndex == PLAYER_LIMB_ROOT) {
        f32 rootScale = CustomForms_RootScale();
        // Order matters: Gerudo drops BEFORE the multiply, Keaton AFTER (soh MmForm_OverrideLimbDraw).
        pos->y -= CustomForms_RootDropBefore();
        if (rootScale != 1.0f) {
            pos->x *= rootScale;
            pos->y *= rootScale;
            pos->z *= rootScale;
        }
        pos->y -= CustomForms_RootDropAfter();
    }

    // A form may claim a hand outright (Gerudo's scimitar in BOTH hands) or blank the sheath.
    if (limbIndex == PLAYER_LIMB_LEFT_HAND || limbIndex == PLAYER_LIMB_RIGHT_HAND) {
        u8 claimed = 0;
        Gfx* formDL = CustomForms_HandDL(p, limbIndex, &claimed);
        if (claimed) {
            *dList = formDL;
            return ret;
        }
    }
    if (limbIndex == PLAYER_LIMB_SHEATH && CustomForms_HidesSheath(p)) {
        *dList = NULL;
        return ret;
    }

    // The vanilla override CLOBBERS *dList for the waist/hands/sheath with MM-CHILD DLs (read from
    // player->waistDLists / leftHandDLists / ... — that's why child parts leaked through, gerudo has the
    // same issue). We RESTORE the skeleton's own mesh for those limbs, substituting an oot.o2r base DL
    // only when a WEAPON is actually held (a mod usually ships no in-hand holding variants).
    switch (limbIndex) {
        case PLAYER_LIMB_WAIST:
            *dList = skelDL; // adult/mod waist — never MM child
            break;

        case PLAYER_LIMB_LEFT_HAND:
            if (sIsChildRig) {
                // Held items keep the vanilla MM-child DL (right scale); empty hands are the form's own.
                if (p->leftHandType == PLAYER_MODELTYPE_LH_OPEN || p->leftHandType == PLAYER_MODELTYPE_LH_CLOSED) {
                    *dList = skelDL;
                }
                break;
            }
            if (p->leftHandType == PLAYER_MODELTYPE_LH_ONE_HAND_SWORD && sDL_LHSword != NULL) {
                *dList = sDL_LHSword;
            } else if (p->leftHandType == PLAYER_MODELTYPE_LH_TWO_HAND_SWORD && sDL_LHBgs != NULL) {
                *dList = sDL_LHBgs;
            } else if (!sIsMod && p->leftHandType == PLAYER_MODELTYPE_LH_CLOSED && sDL_LHClosed != NULL) {
                *dList = sDL_LHClosed;
            } else {
                *dList = skelDL; // open / bottle / mod hand -> the skeleton's own hand
            }
            break;

        case PLAYER_LIMB_RIGHT_HAND:
            if (sIsChildRig) {
                if (p->rightHandType == PLAYER_MODELTYPE_RH_OPEN || p->rightHandType == PLAYER_MODELTYPE_RH_CLOSED) {
                    *dList = skelDL;
                }
                break;
            }
            if (p->rightHandType == PLAYER_MODELTYPE_RH_SHIELD) {
                // Draw whichever shield is EQUIPPED in hand, not always Hylian.
                if (p->currentShield == PLAYER_SHIELD_MIRROR_SHIELD && sDL_RHMirrorShield != NULL) {
                    *dList = sDL_RHMirrorShield;
                } else if (sDL_RHShield != NULL) {
                    *dList = sDL_RHShield;
                } else {
                    *dList = skelDL;
                }
            } else if (p->rightHandType == PLAYER_MODELTYPE_RH_BOW && sDL_RHBow != NULL) {
                *dList = sDL_RHBow;
            } else if (p->rightHandType == PLAYER_MODELTYPE_RH_INSTRUMENT && sDL_RHOcarina != NULL) {
                *dList = sDL_RHOcarina;
            } else if (p->rightHandType == PLAYER_MODELTYPE_RH_HOOKSHOT && sDL_RHHookshot != NULL) {
                *dList = sDL_RHHookshot;
            } else if (!sIsMod && p->rightHandType == PLAYER_MODELTYPE_RH_CLOSED && sDL_RHClosed != NULL) {
                *dList = sDL_RHClosed;
            } else {
                *dList = skelDL;
            }
            break;

        case PLAYER_LIMB_SHEATH:
            if (sIsMod) {
                *dList = skelDL; // the mod's own sword+sheath mesh on the back
            } else {
                // Base skeleton has no sheath mesh (limb 19 = NULL) — pick the back setup from the EQUIPPED
                // sword/shield, mirroring MM's real sheath logic (z_player_lib.c:3233), NOT the hand-type
                // enum. Sword-on-back only if a sword is equipped AND sheathed (not in hand); shield-on-back
                // only if a shield is equipped AND not raised. This kills the old "always Master Sword +
                // Hylian at rest" default.
                u8 swordSheathed =
                    ((u8)GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) != EQUIP_VALUE_SWORD_NONE) &&
                    (p->sheathType == PLAYER_MODELTYPE_SHEATH_14 || p->sheathType == PLAYER_MODELTYPE_SHEATH_12);
                u8 shieldOnBack =
                    (p->currentShield != PLAYER_SHIELD_NONE) && (p->rightHandType != PLAYER_MODELTYPE_RH_SHIELD);
                // A NEI ext shield (Divine/Kite/Ikana) has no adult geometry — keep it OFF the sheath DL and
                // let PostLimb's ext-shield draw handle the back.
                u8 extShield = (ExtEquip_GetShieldDLOverride() != NULL);

                Gfx* d;
                if (!shieldOnBack || extShield) {
                    d = swordSheathed ? sDL_SheathSword : sDL_SheathEmpty;
                } else if (p->currentShield == PLAYER_SHIELD_MIRROR_SHIELD && sDL_SheathMirror != NULL) {
                    d = swordSheathed ? sDL_SheathMirrorSword : sDL_SheathMirror;
                } else { // Hero's/Hylian (also the mirror-DL-missing fallback)
                    d = swordSheathed ? sDL_SheathBoth : sDL_SheathShield;
                }
                if (d != NULL) {
                    *dList = d;
                }
            }
            break;
    }
    return ret;
}

// PostLimb wrapper: the adult sheath DLs already bake the shield-on-back at ADULT scale, but MM's
// Player_PostLimbDrawGameplay independently draws a SECOND shield at CHILD scale at the SHEATH limb
// (z_player_lib.c:4388). Suppress that duplicate for a VANILLA shield by hiding currentShield across the
// post-limb call; keep it for a NEI ext shield (Divine/Kite/Ikana) — our sheath geometry doesn't bake
// those, so PostLimb's ext-shield back draw is the only one. currentShield isn't read elsewhere in that
// post-limb block, so this only removes the duplicate.
static void AdultLink_PostLimb(PlayState* play, s32 limbIndex, Gfx** dList1, Gfx** dList2, Vec3s* rot, Actor* actor) {
    Player* p = (Player*)actor;
    if (limbIndex == PLAYER_LIMB_SHEATH && p->currentShield != PLAYER_SHIELD_NONE &&
        ExtEquip_GetShieldDLOverride() == NULL) {
        s8 saved = p->currentShield;
        p->currentShield = PLAYER_SHIELD_NONE;
        Player_PostLimbDrawGameplay(play, limbIndex, dList1, dList2, rot, actor);
        p->currentShield = saved;
    } else {
        Player_PostLimbDrawGameplay(play, limbIndex, dList1, dList2, rot, actor);
    }
    // Form extras that need THIS limb's matrix: Keaton's tails/reflector (waist), flute + fist quads (hands).
    CustomForms_PostLimb(play, p, limbIndex);
}

// ---- Garo form: native object_jso skeleton over a null-DL Link walk --------------------------
static s32 AdultLink_NullLimb(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot, Actor* actor) {
    s32 ret = Player_OverrideLimbDrawGameplayDefault(play, limbIndex, dList, pos, rot, actor);
    *dList = NULL;
    return ret;
}

static void AdultLink_DrawGaro(PlayState* play, Player* player) {
    if (!sGaroReady) {
        SkelAnime_InitFlex(play, &sGaroSkelAnime, (FlexSkeletonHeader*)gGaroSkel, (AnimationHeader*)gGaroIdleAnim,
                           sGaroJointTable, sGaroMorphTable, GARO_LIMB_MAX);
        Animation_PlayLoop(&sGaroSkelAnime, (AnimationHeader*)gGaroIdleAnim);
        sGaroReady = 1;
        SPDLOG_INFO("[AdultLink] Garo skeleton ready (native object_jso)");
    }

    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    gSPSegment(POLY_OPA_DISP++, 0x0C, (uintptr_t)gCullBackDList);
    CLOSE_DISPS(play->state.gfxCtx);

    f32 scale = CVarGetFloat("gAdultLink.Scale", ADULT_LINK_SCALE_DEFAULT);
    Matrix_Translate(player->actor.world.pos.x, player->actor.world.pos.y, player->actor.world.pos.z, MTXMODE_NEW);
    Matrix_RotateYS(player->actor.shape.rot.y, MTXMODE_APPLY);
    Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);

    // Null-DL walk of the Link rig: fills bodyPartsPos / hand + focus positions for items and lock-on.
    Player_DrawImpl(play, sSkel->sh.segment, player->skelAnime.jointTable, sSkel->dListCount, 0, PLAYER_FORM_HUMAN,
                    player->currentBoots, player->actor.shape.face, AdultLink_NullLimb, AdultLink_PostLimb,
                    &player->actor);

    // Hybrid body: Link's animation retargeted bone-by-bone onto the ghost rig (sword + robe bones keep
    // Garo's own idle), drawn at Link's 0.01 with the x3.5 subtree scale applied at the ROOT entry.
    CustomForms_GaroPose(player, sGaroSkelAnime.jointTable, GARO_LIMB_MAX);
    Matrix_Translate(player->actor.world.pos.x, player->actor.world.pos.y, player->actor.world.pos.z, MTXMODE_NEW);
    Matrix_RotateYS(player->actor.shape.rot.y, MTXMODE_APPLY);
    Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
    SkelAnime_DrawFlexOpa(play, sGaroSkelAnime.skeleton, sGaroSkelAnime.jointTable, sGaroSkelAnime.dListCount,
                          CustomForms_GaroOverrideLimb, CustomForms_GaroPostLimb, &player->actor);

    Math_Vec3f_Copy(&player->actor.shape.feetPos[0], &player->actor.world.pos);
    Math_Vec3f_Copy(&player->actor.shape.feetPos[1], &player->actor.world.pos);
    CustomItems_OverrideDraw(player, play);
    CustomForms_DrawWorld(play, player);
}

extern "C" s32 AdultLink_IsActive(void) {
    // gAdultLink.ForceOn (console) forces adult mode ON regardless of the Time Gate, for isolated tests.
    return Nei_Save()->timeGateAdultMode != 0 || CVarGetInteger("gAdultLink.ForceOn", 0) != 0;
}

extern "C" void AdultLink_Toggle(void) {
    NeiSaveData* n = Nei_Save();
    n->timeGateAdultMode = n->timeGateAdultMode ? 0 : 1;
    SPDLOG_INFO("[AdultLink] toggle -> adult mode = {}", (int)n->timeGateAdultMode);
    // The age flip also switches an active form between its child and adult mirrors.
    AdultLink_OnFormChanged();
    if (n->timeGateAdultMode || CustomForms_ActiveForm() != CUSTOM_FORM_NONE) {
        AdultLink_Setup();
    }
}

extern "C" s32 AdultLink_ShouldHide(void) {
    // The menu's Force Form combobox changes the active form without going through CustomForms_Toggle.
    static s32 sLastForm = CUSTOM_FORM_NONE;
    s32 form = CustomForms_ActiveForm();
    if (form != sLastForm) {
        sLastForm = form;
        AdultLink_OnFormChanged();
    }
    if (!AdultLink_IsActive() && form == CUSTOM_FORM_NONE) {
        return 0;
    }
    // Adult Link / a custom form only REPLACES the Human form. When the player is transformed (Deku /
    // Goron / Zora / Fierce Deity via a mask), draw that native form normally — otherwise the overlay is
    // drawn over every transformation and it looks like the mask "no me convierte".
    if (gSaveContext.save.playerForm != PLAYER_FORM_HUMAN) {
        return 0;
    }
    return AdultLink_Setup();
}

extern "C" void AdultLink_Draw(PlayState* play, Player* player) {
    if (!sReady || sSkel == NULL) {
        return;
    }

    if (CustomForms_ActiveForm() == CUSTOM_FORM_GARO) {
        AdultLink_DrawGaro(play, player);
        return;
    }

    static u8 loggedDraw = 0;
    if (!loggedDraw) {
        loggedDraw = 1;
        SPDLOG_INFO("[AdultLink] first AdultLink_Draw (engine path) at pos ({},{},{})", player->actor.world.pos.x,
                    player->actor.world.pos.y, player->actor.world.pos.z);
    }

    // Render-state + tunic ENV color. OoT adult Link's tunic/cap/collar material is grayscale, TINTED by
    // the ENV color (a mod that exports OoT adult Link keeps this — its tunic goes BLACK without a green
    // ENV, confirmed). So set the Kokiri-green ENV for both vanilla and mod. Player_DrawImpl opens its own
    // DISPS and appends after these commands.
    {
        OPEN_DISPS(play->state.gfxCtx);
        Gfx_SetupDL25_Opa(play->state.gfxCtx);
        gSPSegment(POLY_OPA_DISP++, 0x0C, (uintptr_t)gCullBackDList);
        u8 tr = (u8)CVarGetInteger("gAdultLink.TunicR", 30);
        u8 tg = (u8)CVarGetInteger("gAdultLink.TunicG", 105);
        u8 tb = (u8)CVarGetInteger("gAdultLink.TunicB", 27);
        gDPSetEnvColor(POLY_OPA_DISP++, tr, tg, tb, 255);
        // DIAGNOSTIC (gAdultLink.FullBright): bind a full-white ambient light so every G_LIGHTING
        // surface renders at TEXEL brightness regardless of its normals. If the black FACE lights up
        // with this on, the bug is lighting/SHADE (normals or light binding in this overlay); if it
        // STAYS black, the eye/face TEXTURE itself is the problem (geometry/UV/segment), not lighting.
        if (CVarGetInteger("gAdultLink.FullBright", 0)) {
            static Lights1 sFullBright = gdSPDefLights1(255, 255, 255, 255, 255, 255, 0, 0, 120);
            gSPSetLights1(POLY_OPA_DISP++, sFullBright);
        }
        CLOSE_DISPS(play->state.gfxCtx);
    }

    f32 scale = CVarGetFloat("gAdultLink.Scale", ADULT_LINK_SCALE_DEFAULT);
    f32 yOff = CVarGetFloat("gAdultLink.YOffset", 0.0f);

    Matrix_Translate(player->actor.world.pos.x, player->actor.world.pos.y + yOff, player->actor.world.pos.z,
                     MTXMODE_NEW);
    Matrix_RotateYS(player->actor.shape.rot.y, MTXMODE_APPLY);
    Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);

    // Neutralize the Human age scale so the adult root sits at its animation-native height (the shared
    // ageProperties table is restored right after — same save/mutate/restore pattern as item_minish_cap.c).
    // A child rig with no explicit form scale keeps MM's own child root scale (11/17, OoT's 0.64):
    // the child mirror is authored child-sized, so an adult root height leaves it hanging in the air.
    f32 savedAgeScale = 0.0f;
    if (player->ageProperties != NULL) {
        savedAgeScale = player->ageProperties->unk_08;
        if (!sIsChildRig || CustomForms_RootScale() != 1.0f) {
            player->ageProperties->unk_08 = 1.0f;
        }
    }

    // Point the Human eye/mouth arrays at the adult textures for this draw (Player_DrawImpl binds them
    // to segments 0x08/0x09). Restore afterward so nothing else is affected.
    TexturePtr savedEyes[PLAYER_EYES_MAX];
    TexturePtr savedMouth[PLAYER_MOUTH_MAX];
    // For a mod, bind the mod's eye/mouth OTR PATH STRING on the segment — the EXACT proven mechanism MM's
    // own forms + soh's transformation_masks (Zora/Goron) use. gfx_set_timg_handler_rdp then LoadResource-
    // Process()es it and populates the FULL texture metadata (width/height/type/HByteScale/VPixelScale)
    // from the resource, whereas a raw pixel pointer leaves those at defaults (scale=1) — which corrupts
    // any HD/upscaled mod eye. A direct "__OTR__alt/..." path resolves fine (GetBaseTexturePath only
    // affects HD-blend lookup, not loading). Fall back to raw texels only if the path never resolved.
    // Bind adult eye/mouth on segments 0x08/0x09 for BOTH vanilla (oot.o2r base texels) and mod (alt path).
    // A mod slot prefers its OTR path (full metadata); vanilla uses the oot.o2r raw texels. Either way the
    // adult head never keeps MM's child eye/mouth.
    for (int i = 0; i < PLAYER_EYES_MAX; i++) {
        savedEyes[i] = sPlayerEyesTextures[PLAYER_FORM_HUMAN][i];
        if (!sModEyePath[i].empty()) {
            sPlayerEyesTextures[PLAYER_FORM_HUMAN][i] = (TexturePtr)sModEyePath[i].c_str(); // mod eye OTR path
        } else if (sModEye[i] != NULL) {
            sPlayerEyesTextures[PLAYER_FORM_HUMAN][i] = (TexturePtr)sModEye[i]; // oot.o2r base / raw texels
        }
    }
    for (int i = 0; i < PLAYER_MOUTH_MAX; i++) {
        savedMouth[i] = sPlayerMouthTextures[PLAYER_FORM_HUMAN][i];
        if (!sModMouthPath[i].empty()) {
            sPlayerMouthTextures[PLAYER_FORM_HUMAN][i] = (TexturePtr)sModMouthPath[i].c_str();
        } else if (sModMouth[i] != NULL) {
            sPlayerMouthTextures[PLAYER_FORM_HUMAN][i] = (TexturePtr)sModMouth[i];
        }
    }

    // One-shot ground-truth diagnostic: what actually gets bound for the eye the animation selected.
    {
        static u8 loggedFace = 0;
        if (!loggedFace) {
            loggedFace = 1;
            s32 ei = GET_EYE_INDEX_FROM_JOINT_TABLE(player->skelAnime.jointTable);
            if (ei < 0) {
                ei = 0;
            }
            if (ei >= PLAYER_EYES_MAX) {
                ei = 0;
            }
            SPDLOG_INFO("[AdultLink] FACE bind (mod={}): eyeIndex={} eyePath='{}' rawPtr={} headLimbDL={}", (int)sIsMod,
                        ei, sModEyePath[ei].empty() ? "(none)" : sModEyePath[ei].c_str(), sModEye[ei],
                        (void*)((LodLimb*)sSkel->sh.segment[10])->dLists[0]);
        }
    }

    // Draw through the real engine path so held items / trails / colliders all work; our override
    // supplies adult equipment DLs. Feed MM's OWN jointTable (identical 21-limb hierarchy) 1:1.
    Player_DrawImpl(play, sSkel->sh.segment, player->skelAnime.jointTable, sSkel->dListCount, 0, PLAYER_FORM_HUMAN,
                    player->currentBoots, player->actor.shape.face, AdultLink_OverrideLimb, AdultLink_PostLimb,
                    &player->actor);

    for (int i = 0; i < PLAYER_EYES_MAX; i++) {
        sPlayerEyesTextures[PLAYER_FORM_HUMAN][i] = savedEyes[i];
    }
    for (int i = 0; i < PLAYER_MOUTH_MAX; i++) {
        sPlayerMouthTextures[PLAYER_FORM_HUMAN][i] = savedMouth[i];
    }
    if (player->ageProperties != NULL) {
        player->ageProperties->unk_08 = savedAgeScale;
    }

    // Feet shadow: the vanilla Player_Draw path (which our early return skips) seeds shape.feetPos, and
    // ActorShadow_DrawFeet (invoked from z_actor.c AFTER actor->draw) reads it — otherwise Link casts no
    // shadow. MM seeds both feet with the actor world pos (a single blob under Link); mirror that.
    Math_Vec3f_Copy(&player->actor.shape.feetPos[0], &player->actor.world.pos);
    Math_Vec3f_Copy(&player->actor.shape.feetPos[1], &player->actor.world.pos);

    // Held NEI custom items (beetle, cane of somaria, ball&chain, rods, ...) are drawn post-skeleton in
    // the vanilla path we skipped. bodyPartsPos/leftHandWorld/rightHandWorld were just filled by
    // Player_DrawImpl+PostLimb, so each custom item seats on the adult hands exactly like on child Link.
    // (Boomerang/Net already drew inside PostLimb — this does not double them.)
    CustomItems_OverrideDraw(player, play);
}

// ---- GAMEPLAY layer: adult-scale the Human form's height-dependent attributes -------------------
// Backed up once, applied on activate, restored on deactivate (in place on the shared table, mirroring
// PlayAsKafei's skeleton-slot swap). Scales ledge-grab / ceiling / wall / height by the same factor the
// adult skeleton is taller than child Link, so adult Link grabs higher ledges, clears ceilings at adult
// head height, and checks walls at adult reach — the part that makes the form PLAY tall, not just look it.
static PlayerAgeProperties sHumanAgePropsBackup;
static u8 sAgePropsSaved = 0;
static u8 sAgePropsActive = 0;

static void AdultLink_ApplyAgeProps(void) {
    PlayerAgeProperties* h = &sPlayerAgeProperties[PLAYER_FORM_HUMAN];
    if (!sAgePropsSaved) {
        sHumanAgePropsBackup = *h;
        sAgePropsSaved = 1;
    }
    if (sAgePropsActive) {
        return;
    }
    const PlayerAgeProperties* b = &sHumanAgePropsBackup;
    f32 s = CVarGetFloat("gAdultLink.HeightScale", 1.30f); // adult ~30% taller than MM child Link
    h->ceilingCheckHeight = b->ceilingCheckHeight * s;
    h->unk_0C = b->unk_0C * s;
    h->unk_10 = b->unk_10 * s;
    h->unk_14 = b->unk_14 * s; // ledge grab: max yDistToLedge to grab
    h->unk_18 = b->unk_18 * s; // ledge grab: hang
    h->unk_1C = b->unk_1C * s; // ledge grab: climb
    h->unk_34 = b->unk_34 * s; // body height
    h->unk_40 = b->unk_40 * s;
    h->wallCheckRadius = b->wallCheckRadius * CVarGetFloat("gAdultLink.WallScale", 1.0f);
    sAgePropsActive = 1;
}

static u8 sFormPropsApplied = 0;

static void AdultLink_RestoreAgeProps(void) {
    if (sAgePropsSaved && (sAgePropsActive || sFormPropsApplied)) {
        sPlayerAgeProperties[PLAYER_FORM_HUMAN] = sHumanAgePropsBackup;
        sAgePropsActive = 0;
        sFormPropsApplied = 0;
    }
}

// Per-form placement on top of the age layer (soh sFormProps): ceiling / wall reach / shadow and the
// collider cylinder. Kafei keeps the adult numbers so his running ledge vault survives.
static void AdultLink_ApplyFormProps(Player* player, const CustomFormProps* props) {
    PlayerAgeProperties* h = &sPlayerAgeProperties[PLAYER_FORM_HUMAN];
    if (!sAgePropsSaved) {
        sHumanAgePropsBackup = *h;
        sAgePropsSaved = 1;
    }
    if (props->overridesAgeProps) {
        h->ceilingCheckHeight = props->ceilingCheckHeight;
        h->wallCheckRadius = props->wallCheckRadius;
        h->shadowScale = props->shadowScale;
        sFormPropsApplied = 1;
    }
    player->cylinder.dim.radius = (s16)props->cylinderRadius;
    player->cylinder.dim.height = (s16)props->cylinderHeight;
}

extern "C" void AdultLink_UpdateCollider(Player* player) {
    if (sGaroReady && CustomForms_ActiveForm() == CUSTOM_FORM_GARO) {
        SkelAnime_Update(&sGaroSkelAnime);
    }
    const CustomFormProps* props = (sReady && player->transformation == PLAYER_FORM_HUMAN) ? CustomForms_Props() : NULL;
    // Adult gameplay attributes apply to adult mode AND to any ADULT-SIZED custom form (rootScale 1.0 —
    // Kafei/Gerudo). A short form (Keaton) keeps child Link's ledge grab / ceiling / collider.
    u8 adultSized = (AdultLink_IsActive() || CustomForms_ActiveForm() != CUSTOM_FORM_NONE) && sReady && !sIsChildRig &&
                    CustomForms_RootScale() == 1.0f;
    if (!adultSized && props == NULL) {
        AdultLink_RestoreAgeProps();
        return;
    }
    if (adultSized) {
        AdultLink_ApplyAgeProps();
        player->cylinder.dim.height =
            (s16)CVarGetInteger("gAdultLink.ColliderHeight", ADULT_LINK_COLLIDER_HEIGHT_DEFAULT);
    } else {
        AdultLink_RestoreAgeProps();
    }
    if (props != NULL) {
        AdultLink_ApplyFormProps(player, props);
    }
}
