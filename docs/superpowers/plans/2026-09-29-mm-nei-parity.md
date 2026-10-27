# Native MM NEI Presentation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Complete the accepted NEI held models, grips, effects and associated item-use corrections in ComboShip's native MM engine.

**Architecture:** Share accepted pure mesh policies and existing assets, with MM-native render adapters and player/state hooks. Route deferred resource paths to their owning OoT archive with owner-aware availability checks and process-lifetime strings. Keep MM gameplay plumbing and the existing OoT implementation intact.

**Tech Stack:** C/C++20, native MM GBI/matrix/frame interpolation, Python production-function fixtures, GitHub Actions.

**Spec:** `docs/superpowers/specs/2026-09-29-mm-nei-parity.md`

## Global Constraints

- Baseline is `7e3b036502291cc86405c64e3b90fcb57b6e4eab`; the accepted donor is its `soh/` tree from SoH PR 17 `2ce2e5a28098261943c25f8a1ecebe730c1d06ea`.
- Reuse accepted artwork/geometry/poses; do not regenerate art, change Din's rig, or replace vendored dependencies.
- Preserve OoT behavior, MM GI/icon/progression corrections, PAK/voice, performance, weather and audio.
- Never pass MM player/play/save structures to OoT engine functions. Cross-engine resource queries use a small typed C ABI only.
- All deferred paths must outlive submitted frames; Alt on/off and missing/partial resources must fall back safely.
- Only the local held rod/tip is hidden during first-person aiming; projectile/trail submission and remote/clone models remain.
- Tests must execute actual native MM production paths, not only the shared policies or OoT drawers.
- Work on the isolated candidate; do not push from a worker. Root publishes the reviewed result to PR 24 under existing authorization. No master promotion.

## Review Focus

- MM and OoT use different player layouts, hand names, matrix/graphics functions and global resource ownership; compile against MM headers and submit through MM APIs.
- Captured wrists must belong to the currently drawn player, preserve roll, and expire before another actor can inherit them; cover missing limbs, first person, both ages and transformed/remote states.
- An inactive donor engine still owns the shared asset namespace; availability and deferred display-list/texture routing must agree, including Alt and missing bundles.
- Volley removal, projectile fade and reused slots must retain geometry/heading without interpolating another projectile's previous frame.
- Associated stow/Leaf/switch/whip changes must retain native MM input, collision, audio, progression and cleanup behavior.

### Task 1: Native MM rod presentation and shared rendering foundation

**Files:**
- Create native adapters in `mm/2s2h/Rando/NeiHeldPresentation.cpp/.h`, `NeiUsedMagicPresentation.cpp/.h`, and a small NEI resource routing helper if needed.
- Modify `mm/2s2h/Rando/NeiGiPresentation.cpp` to share its existing native mesh submission primitives and correct owner-aware texture availability.
- Add the smallest owner-resource ABI export to `soh/soh/Enhancements/randomizer/NeiGiPresentation.cpp` and declare it beside the existing NEI ABI in `combo/menu/ComboItemDrawABI.h` if needed.
- Modify `mm/mods/items/helpers/equip_helper.c/.h`, `mm/src/code/z_player_lib.c`, and `mm/mods/items/custom_items_common.c` for current-player wrist capture/pose/release.
- Modify `mm/mods/items/objects/object_firerod.c`, `object_icerod.c`, `object_lightrod.c`; `mm/mods/items/logic/item_rod_common.c/.h`, `item_rod_fire.c/.h`, `item_rod_ice.c/.h`, `item_rod_light.c/.h`; add the donor `rod_visual.h` and `ice_trail.h` helpers as required.
- Add native MM tests under `tests/mm_nei/` and runner `scripts/diagnostics/run_mm_nei_tests.py`; add it to `scripts/diagnostics/run_combo_nei_regressions.sh`.

**Interfaces:**
- Native APIs mirror the accepted `NeiHeld_HasResources`, `NeiHeld_DrawModel`, `NeiHeld_DrawRod`, `NeiUsedMagic_DrawProjectile/Trail/Charge/ChargeFocus/Spin/Burst/Portal` signatures, compiled against MM's `PlayState`, `Player`, `Vec3f` and graphics context.
- Produce `ItemHandPose`, `ItemEquip_CaptureHandMatrix`, `ItemEquip_CaptureLeftHandMatrix`, `ItemEquip_ReleaseHandMatrix`, `ItemEquip_ApplyHandPose`, `ItemEquip_ApplyLeftHandPose` for later native held drawers. Match the donor conventions; preserve MM-specific existing helpers.
- Expose `NeiHeld_DrawMitts` and `NeiHeld_DrawGustJar` adapters for Task 2. Resource routing helper names/signatures must be recorded in the report for Task 2.

- [ ] Write native MM drawer/geometry assertions that fail on baseline: replacement held paths, retained wrist roll, first-person held suppression with live shots, actual accepted charge/release/projectile output.
- [ ] Connect native MM rod drawers and draw-phase effects to the accepted shared policies. Preserve all host gameplay updates except the presentation epoch needed on set launch/reuse; match the donor's retained impact heading.
- [ ] Implement native hand capture at the unmodified wrist matrices, release after all current-player custom drawers, and safe missing-capture fallback. Cover MM form/age data explicitly; never assume OoT's Player layout.
- [ ] Route accepted model/texture resources to the bundled OoT owner, with matching native availability and atomic fallback. Exercise late resource absence, partial rod bundle, Alt on/off, stable path lifetime and returned MM display-list commands.
- [ ] Run native MM rod/interpolation tests covering both ages, all three elements, local/remote, eight wrist poses, launch, volley disappearance, reuse and impact; compare accepted Light/Fire/Ice policy output across 180 phases. Keep existing SoH tests green and compile changed production C/C++ units against MM headers.
- [ ] Commit the reviewed implementation inputs and write a report with commands, outcomes, exact adapter interfaces, unresolved runtime limits and commits. Do not spawn helpers or reviewers.

### Task 2: Complete remaining native MM held/used models

**Files:**
- Create `mm/2s2h/Rando/NeiArticulatedPresentation.cpp/.h` and `NeiLanternPresentation.cpp/.h` adapted from accepted SoH counterparts.
- Modify `mm/mods/items/objects/object_ballchain.c`, `object_beetle.c`, `object_cane_of_somaria.c`, `object_dekuleaf.c`, `object_gustjar_pot.c`, `object_mogma_mitts.c`, `object_shovel.c`, `object_spinner.c`, `object_timegate.c`, `object_whip.c` and native Switch Hook draw locations identified from `item_switchhook.c`/`mm/src/overlays/actors/ovl_Arms_Hook/z_arms_hook.c`.
- Modify the relevant hand selectors in `mm/src/code/z_player_lib.c`, `mm/mods/items/logic/item_lantern.c`, `mm/mods/items/custom_items_common.c`, and native MM tests/runner from Task 1.

**Interfaces:**
- Consume Task 1's MM `NeiHeld_*`, `NeiUsedMagic_DrawPortal`, owner-routing helper and `ItemEquip_*HandPose` APIs.
- Produce native `NeiArticulated_UsesSwitchHook/ApplySwitchHookHand/DrawSwitchHookTip/HasWhip/DrawWhipGrip` and `NeiLantern_DrawHeld` with the accepted donor signatures.

- [ ] Add failing native MM coverage for each accepted held replacement and active component, including missing/partial resources, human child/adult and other supported form guards.
- [ ] Connect all listed MM held/used paths to the accepted meshes and transforms; preserve native fallback DLs. Somaria replacement applies only to original Somaria; preserve Pacci/Trirod and empty-handed Ultrahand behavior. Spinner dimensions/rotation and child/adult shovel scale match the accepted donor.
- [ ] Keep Switch Hook docked body, extended/retracting head and native fist ownership consistent, and Whip coil/handle/rope/tip socket continuity across its native state machine. Adapt MM hand selectors using native form-specific hands.
- [ ] Use the accepted Lantern core/glass/accents and clasp; hide the Time Gate prop during use while rendering the accepted portal. No held model is introduced for GI-only items.
- [ ] Run actual native MM component tests, existing GI/icon/progression and SoH preservation tests, real-header compilation and asset owner/parity checks. Record per-item connected call sites so omissions are reviewable.
- [ ] Commit and report exact changes, commands/results and runtime limits. Do not spawn helpers or reviewers.

### Task 3: Associated MM item-use corrections and cumulative verification

**Files:**
- Inspect and selectively modify `mm/mods/items/custom_items.h`, `custom_items_common.c`, `custom_items_stow.c`, `helpers/equip_helper.c/.h`, `logic/item_ballchain.c`, `item_dekuleaf.c`, `item_lantern.c`, `item_whip.c`, `item_switchhook.c` and affected native MM player stow/input hooks in `mm/src/overlays/actors/ovl_player_actor/z_player.c`.
- Extend `tests/mm_nei/` and `scripts/diagnostics/run_mm_nei_tests.py`; update `docs/poc/nei-gi-combined-20260929.md` with the native-MM scope/evidence and keep the CI runner wired.

**Interfaces:**
- Consume the native presentation/capture APIs from Tasks 1/2. Preserve existing exported item APIs and native MM control/data layouts; add only the corresponding donor stow/cleanup entry points actually needed by MM.

- [ ] Compare accepted SoH use fixes to MM and write failing regressions only for missing native behavior: one put-away sound, rod/lantern stow, Ball and Chain inactive/roll input, Leaf safe stop/completion, raised Whip arm through first-person release, instant Switch Hook and Time Gate prop suppression.
- [ ] Apply each missing correction at the native MM integration point, retaining its existing save/form/input/audio/collision behavior. If MM already satisfies a donor fix, record the evidence instead of changing it. Do not transplant the OoT MM-audio shim into native MM.
- [ ] Run the complete native MM NEI suite, accepted SoH suites, MM pickup/icon/progression and item-visual tests, PAK/voice/shared-item checks, asset collision, formatting and production syntax checks. Exercise interruption, repeat use, no magic, absent assets, local/remote and lifetime transitions relevant to each correction.
- [ ] Document remaining in-game checks accurately and commit. Root performs final independent review, publishes the exact tested tree to PR 24 and waits for the complete gate plus Windows/Linux build start.

### Task 4: Diagnose and correct native MM sword damage regression

User steering on September 29: Din fire sword cannot damage Chuchus in Termina Field or any enemies, with the checkbox enabled or disabled; grass cutting still works. Complete this focused correction before the combined PR24 publication, while preserving all reviewed NEI work.

- [ ] Trace the native sword collider/damage path with both fire visual and damage options enabled/disabled, including the actual Din/adult/custom form update/draw and any damage modifier path. Establish a concrete failing case before changing code; existing pure damage fixtures currently pass and do not reproduce the report.
- [ ] Correct the smallest proven native integration fault. Preserve normal enemy damage, grass cutting, native elemental reactions, sword upgrades/forms and remote ownership. Din model orientation/artwork remains outside scope.
- [ ] Add a meaningful native production-path regression that fails before the correction and covers Chuchu plus an ordinary enemy, checkbox off/on and retained grass behavior. Run the affected existing MM Din suites and real-header compilation; do not rerun unrelated full suites without a concrete reason.
- [ ] Document evidence and remaining runtime limits, commit the correction and report for independent review. If source/fixtures cannot reproduce the user's failure, provide the precise missing runtime observation rather than inventing a fix; the controller will resolve it while preserving the ready NEI candidate.
