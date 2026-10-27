# SoH Integration Port Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans for checkpointed execution.

**Goal:** Carry the approved SoH stabilization and final Skull Kid texture experiment into the OoT side of ComboShip.

**Architecture:** Apply the integration delta from NEI c29262b76 to runtime-tested fcc86528612ecabb192fe3642acaacf7c2766869 onto ComboShip's existing NEI port. Retain ComboShip-specific cross-game code, then layer the user-approved uncommitted Skull Kid direct-image patch. Do not replace the whole soh tree.

**Tech Stack:** C/C++, CMake, native focused tests, Windows Fleet build.

**Spec:** User-approved OoT-only port, preserving ced161fd scrolling probe and all proven integration features; final texture patch explicitly included on user approval.

## Global Constraints

- Work only on integration/soh-stabilized-20260912 in this separate clone.
- Preserve source integration branch and its four uncommitted files.
- Base: 62d1555e37071dcb113c1192b23a87cb2e6ddd35, child of ced161fd72cafd1fa154bb868d33a8222093ec7d.
- No mm/, combo/, libultraship/, save-schema, upstream-pin or root build replacement.
- Preserve owning-game resource selection, cross-game rendering, and NEI adaptations.
- Skull Kid texture patch is runtime-unverified, not a confirmed visual fix.

## Task 1: Transfer compatible delta and focused regression suite

- [ ] Check delta with `git diff c29262b76 reference/soh-runtime-fcc8652 -- soh | git apply --check -`.
- [ ] Apply compatible files, excluding soh/CMakeLists.txt and the five failed paths below.
- [ ] Transfer the final mm_display_list_patch.cpp, mm_asset_loader.cpp and mm_display_list_patch_test.cpp changes and archive diagnostic from the donor working copy without modifying it.
- [ ] Compile and run the focused actor, rain, display-list, and archive tests against candidate sources.

## Task 2: Resolve cross-game overlaps

Files: soh/soh/Enhancements/audio/AudioEditor.cpp, soh/soh/OTRGlobals.cpp, soh/soh/z_play_otr.cpp, soh/src/code/audio_heap.c, soh/src/code/audio_load.c.

- [ ] Compare each path at c29262b76, donor fcc8652, and candidate HEAD.
- [ ] Apply each integration hunk without removing ComboShip scopes, logger initialization, audio threading, or lifecycle hooks.
- [ ] Read docs/deviations/boot-shutdown.md and the affected subsystem notes completely before editing lifecycle code.
- [ ] Run night handoff, streamed audio capacity and weather/audio runtime harnesses; preserve existing ComboShip audio owner selection.

## Task 3: Build integration and verification

- [ ] Integrate only the focused-test targets from donor soh/CMakeLists.txt, preserving ComboShip DLL targets and top-level project ownership.
- [ ] Run `python3 scripts/check-asset-collisions.py` and `git diff --check`.
- [ ] Verify `git diff --exit-code 62d1555e3 -- mm combo libultraship upstream-pins.json CMakeLists.txt`.
- [ ] Compile the Windows Fleet target and regenerate port archives through existing ComboShip build instructions.
- [ ] Require runtime tests: native OoT, Prelude reload, Skull Kid variants, Impa face, Ruto poses, Navi priority, rain/dry cycling, night/dawn/enemy handoffs, large streamed music stack, and OoT/MM round trip.
- [ ] Do not label candidate stable or merge into nei until build and runtime gates pass.
