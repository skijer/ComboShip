# MM dungeon items and bottles implementation plan

> **For agentic workers:** Use superpowers:executing-plans to implement this plan. Steps use checkbox syntax.

**Goal:** Selected TP model ports with native MM dungeon shading and bottle shimmer.

**Architecture:** Resolve owner IDs before randomizer dispatch; wrap only the opaque dungeon-item draw in a scoped luminance tint. Add deterministic, bounded sparkle draws around the existing bottle functions. Build a separate O2R dependency closure and split TP compass glass into MM's translucent pass.

**Tech Stack:** C/C++, Fast3D, Python, XML resources, O2R ZIP.

**Spec:** docs/superpowers/specs/2026-09-22-mm-dungeon-bottles.md

## Global Constraints

- Four dungeon defaults: #EC78BA, #81AD46, #635AB7, #B1A553.
- Separate switches, default off; selected items only.
- Original donor files unchanged; candidate branch only.
- Runtime proof and acceptance pending until tested in game.

## Review Focus

- Shuffled items keep their owner colour in another dungeon.
- Tint ends before glass, gems and the next item.
- Missing resources produce no invalid display-list commands.
- Repeated render calls at one gameplay frame cannot accelerate shimmer or consume RNG.
- TP compass glass remains translucent and Poe contents retain scrolling/billboarding.

### Task 1: Native draw enhancements

**Files:** mm/2s2h/Rando/DungeonItemVisuals.h; mm/2s2h/Enhancements/ItemVisuals.h; mm/src/code/z_draw.c; mm/2s2h/Rando/DrawItem.cpp; mm/2s2h/BenGui/CosmeticEditor.{h,cpp}; mm/tests/item_visuals_test.c; scripts/diagnostics/run_mm_item_visuals_tests.py.

**Interfaces:** `int DungeonItem_GetOwner(RandoItemId id)` returns 0..3 or -1. `s32 GetItem_DrawDungeonItem(PlayState*, s16 drawId, s32 owner)` returns whether it drew; `void GetItem_DrawBottleShimmer(PlayState*, s16)` preserves incoming matrix state.

- [ ] Write fixture tests using production function bodies and actual GBI headers. Assert the 16 MM IDs map to four owners; foreign and unrelated items return -1. Assert a live edited colour is emitted only in the OPA scope and the scope closes before the next draw. Assert disabled/missing resources emit nothing. Assert three motes, balanced matrix stack and stable transforms on repeated frames.
  ```c
  assert(DungeonItem_GetOwner(RI_GREAT_BAY_MAP) == 2);
  assert(DungeonItem_GetOwner(RI_OOT_MAP_WATER_TEMPLE) == -1);
  ```
- [ ] Run `python3 scripts/diagnostics/run_mm_item_visuals_tests.py`. Expected: failure on absent implementation before production edits.
- [ ] Implement the interfaces and UI switches. Restrict accepted draw IDs to GID_KEY_SMALL, GID_KEY_BOSS, GID_DUNGEON_MAP, GID_COMPASS. Query CosmeticEditor_GetChangedColor for Items.Woodfall, Items.Snowhead, Items.GreatBay, Items.StoneTower. Shimmer accepts three potions, both fairies and both Poe variants only.
- [ ] Run the new harness and `python3 scripts/diagnostics/run_mm_custom_item_color_tests.py`. Expected: emitted-command tests and real-header syntax checks pass.
- [ ] Commit the runtime changes and record verification.

### Task 2: TP model dependency closure

**Files:** scripts/mods/build_mm_tp_dungeon_bottles.py; generated pack, manifest and README outside the git checkout.

**Interfaces:** Map root stays gGiDungeonMapDL; compass body/glass become separate MM roots; donor GhostContainerLid/Glass become MM PoeContainerLid/Glass. Dependencies live under objects/tp_dungeon_bottles_poc1/.

- [ ] Add explicit validation for complete dependency closure, supported XML commands, vertex/triangle references and texture payloads. Preserve original texture and vertex bytes. Check opaque compass body and translucent glass root disjointness.
- [ ] Build with the supplied TP model/texture archives and native mm.o2r; assert each target root exists in native MM and is used by z_draw.c.
- [ ] Read the produced archive back and verify ZIP CRCs, resource counts, unchanged donor hashes and a second deterministic build hash. Expected: all pass.
- [ ] Package source, manifest, installation details and configuration-specific runtime checklist; save deliverables. Commit the builder. Build candidate if CI is available; report exact evidence and remaining runtime limitations.
