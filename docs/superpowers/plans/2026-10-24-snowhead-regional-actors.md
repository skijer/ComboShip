# Snowhead Regional Actors Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development. Steps use checkbox syntax for tracking.

**Goal:** Complete the shared open Snowhead valley with ordinary both-age access, three regional actor presentations, replaceable bridge textures and investigate Woodfall portal opacity.

**Architecture:** Four isolated worker branches own independent files. Integrate onto the restored overnight candidate, review each owned diff and then the combined branch. Task2 supplies portable valley data to Task3; Task3 keeps endpoint APIs stable for Task1 and supplies material selection to Task2.

**Tech Stack:** C/C++, native SoH/2Ship render and actor APIs, Python asset/fixture tools, O2R resource packs.

**Spec:** docs/superpowers/specs/2026-10-24-snowhead-regional-actors-design.md

## Global Constraints

Preserve recovered source tree fce96d959ecbeaf37b0e49b05a90684549084e9f (local checkpoint 9347ae570ee1921b55f1eb1952f901afb38370be, original candidate 729e3826db8d3adc04dd60ec668823b9ab230ede). Never merge develop/master. Preserve original archives unchanged. No original game archives or private evidence in public source. One shared doorway-less Snowhead valley in vanilla and Djipi. One separate right-hand ascent usable by child and adult, preserving adult moving icebergs and native Ice Cavern behavior. Child Ice Cavern access is NOT a goal. Ice-blue shimmer in the valley opening, local falling snow and icy blue drift/chill remain required. Post-build .o2r replacements must retexture all added Snowhead/Lake Hylia surfaces without code or recompilation. Preserve all overnight shop/title/spin/actor/weather fixes and original/combined/solo guards. Static/native checks are not configured gameplay proof.

## Review Focus

- Re-entry/resource reload: no stale dynamic collision, actor duplicates or render pointers.
- Existing placed actors and solo modes: stable IDs, native quest behavior and cross-game guards preserved.
- Moving adult ice plus ordinary child swimming: the whole new route stays usable without special equipment or removed native obstacles.
- Active native/Alt material graphs: only the backdrop/valley changes, no missing terrain, water or unexpected texture spill.
- Replacement packs and returns: post-build texture changes retain metadata/UVs; grounded return does not bounce through the entrance.

---

### Task 1: Regional cutscene actors

Own the existing actor catalog/presentation/importer paths, their tests and docs. Start with soh/src/overlays/actors/ovl_En_Viewer/{static_story_actor.*,static_story_mm_actor.*,z_en_viewer.*}, soh/mods/transformation_masks/assets/mm_asset_loader.*, corresponding native MM actor hooks, and catalog/preset tooling discovered with rg. Add Stalchildren, Mikau and Darmani as specified; append IDs, don't renumber. Read actual mm.o2r skeletons/animations/textures to derive limb/matrix/frame counts and resource closure. Native Mikau's extra object_zog_DL_0280A8 attaches at OBJECT_ZOG_LIMB_11; no separate bank blocker was established. Darmani object_gg_Skel_00F6C0 with wait object_gg_Anim_00F578 and fly object_gg_Anim_00AF40 requires explicit translucent drawing; the ordinary imported MM path is opaque. Its placed copy should be visible independently of native Lens/healed flags, with native quest actors preserved. Add supported native MM placed presentations too, using current conventions. Default regional placement belongs in NEW host-specific ComboRegionalActors.cpp/.h modules; consume existing ComboRegionalPassagesOot_Get / MM endpoint APIs so geometry changes do not require hardcoded NPC positions. Keep the three portal approaches clear; explicit authored rows suppress duplicates. New placement modules may query endpoint readiness each frame and own lifecycle/cleanup, not mutate the bridge modules. Coordinate stable new params through your report. Do not edit geometry, materials, generic regional trigger code or Woodfall portal files.

Interfaces: actor modules consume the existing regional endpoint API (same signature), which remains authoritative after Task3. The shared catalog exports append-only actor/pose identifiers and presets; report exact values. No other task owns actor catalog/importer/native actor sources.

Test first using real descriptors, archive resources and native draw/callback fixtures; include unavailable resources, wrong/absent prop, ghost XLU, original actor params, authored/default suppression and combo/solo modes. Run the existing applicable soh static-story and native MM tests. Stage/commit only your paths. Write full commands/RED/GREEN/proof limits to task-1-report.md.

- [x] Write and run a focused failing behavioral regression before implementation.
- [x] Implement only the owned production paths and required test support.
- [x] Run the named native/static/sanitized checks, record failures and exact proof boundaries.
- [x] Commit bounded owned changes and self-review.
- [x] Obtain an independent spec-and-quality task review; fix required findings.

### Task 2: Shared Fountain valley and removed backdrop

Own NEW combo/entrances/FountainValley.h, NEW soh/soh/Enhancements/Gameplay/ComboFountainValley.cpp/.h, any narrowly necessary new runtime resource-hook module, tests/regional_passages/*valley* and a POC note. Do not edit existing FountainSnowheadBridge.* or ComboRegionalPassages.cpp or shared presentation/material header; Task3 owns those. Do not edit actor catalog or Woodfall files.

From the read-only supplied archive, identify the exact active native backdrop triangles/display-list path; remove only those, accounting for native/Alt scene resource resolution and setups. Inspect the actual graph, not every loose XML leaf. Add missing valley floor/wall collision using matching visible geometry; do not wholesale-copy donor collision, which has unrelated edits. Compare all untouched waterboxes, native exit surfaces, camera data and actors. Prefer a source-contained process-owned collision/draw actor and targeted resource hooks following existing production patterns; no additional scene archive required. Snow floor should follow existing valley surfaces without cracks, doubled geometry/Z fighting or visual/collision mismatch. All snow/stone material calls must use Task3's replaceable material API; keep native/Djipi walls and UVs intact.

Produce portable namespace ComboFountainValley with Point {float x,y,z;}, triangles/mesh data as needed, and inline bool FloorAt(float x,float z,float* y) returning the actual walkable valley floor or false outside. Both variants must give the same walkable corridor. NEW host module exposes extern C int ComboFountainValley_Ready(PlayState*) and owns initialization/destruction; Task3 calls it before enabling the Snowhead endpoint. Define these exact interfaces and tell the controller as soon as the header is committed. Select only ordinary valid collision surfaces; preserve/re-map donor surface metadata deliberately. Task3 produces combo/render/RegionalBridgeMaterials.h; communicate required snow draw integration directly if needed, without editing that owned file.

Test first: old native missing valley floor and backdrop obstruction reproduce; corrected native/Alt graphs/geometry and player body prisms pass; original exits/water/actor resources unchanged; repeated scene load and resource reload lifetime correct. Include portable derived fixtures for normal CI (not a required private archive). Run actual archive checks when possible. Stage/commit only owned paths; task-2-report.md must include source identities and exact checks.

- [x] Write and run a focused failing behavioral regression before implementation.
- [x] Implement only the owned production paths and required test support.
- [x] Run the named native/static/sanitized checks, record failures and exact proof boundaries.
- [x] Commit bounded owned changes and self-review.
- [x] Obtain an independent spec-and-quality task review; fix required findings.

### Task 3: Both-age ascent, Snowhead effects and replaceable bridge materials

Own existing combo/entrances/FountainSnowheadBridge.h, soh/soh/Enhancements/Gameplay/ComboFountainSnowheadBridge.*, soh/soh/Enhancements/Gameplay/ComboLakeHyliaBridge.cpp (material calls only), combo/actors/RegionalPassagePresentation.h (Snowhead fitting only), combo/entrances/RegionalPassages.h / soh/soh/Enhancements/Gameplay/ComboRegionalPassages.cpp only as needed for the new grounded Snowhead trigger/return. Own NEW combo/render/RegionalBridgeMaterials.h, corresponding tests and replacement-pack tooling/docs. Do not edit Task2's new FountainValley.* modules/header, actor catalog/importer or Woodfall files.

Implement one separate natural right-side ascent usable by ordinary child and adult, starting low enough for normal swimming-to-walking. Do not use Ice Cavern child access as a requirement. Match the user's screenshot orientation and actual scene terrain. Preserve all native adult ice actors and verify complete moving/tilting/orbit envelopes against the whole new path/supports AND required player standing space. Existing ice_platform_clearance_test.py has measured native actors and continuous separation helpers; re-run against the exact NEW mesh, not old results. Preserve native Ice Cavern and ensure its entry does not intercept the Snowhead route. Remove the artificial Snowhead cave/cap; join the newly supported natural valley, with a ground-height transition in the valley opening and a valid grounded return outside retrigger bounds. Resolve floor at threshold AND return using Task2's ComboFountainValley::FloorAt(float,float,float*) and require ComboFountainValley_Ready(PlayState*) before enabling it. Existing endpoint API signature remains stable for Task1 actor placements. Tell Task2/actor worker the selected mouth/return semantics.

Keep ice-blue NEI shimmer covering the valley opening, local actual falling-snow presentation (current KiraKira sparkle call is not enough), icy-blue drifting wind/chill and smooth cleanup on pause/scene/mode transitions. Use local emitters, not scene-wide precipitation changes. Protect the grave/Lake shimmer implementation and existing NEI art.

Post-build retexture: create RegionalBridgeMaterials with Material enum values LakeStone, SnowheadStone, SnowheadSnow, SnowheadIce, ValleySnow; function const char* OverridePath(Material), const char* FallbackPath(Material), and template/helper Resolve(Material, exists) returning a replacement path when present, otherwise native fallback. Use stable paths under textures/comboship/regional/<lake_hylia|snowhead>/... with documented actual native dimensions/formats, preserving replacement metadata and HD scaling in engine loading. No CVar/code edits/recompile to install a pack. Give top/side/cave surfaces sensible continuous UVs and distinct material IDs. Task2's valley snow draw consumes this header/API. Include deterministic sample .o2r replacement-pack recipe/tool, using supplied user files or generated debug swatches rather than distributing whole native archives.

Test first: child swim/ground handoff and full route, adult ice clearances, native/Alt geometry control, endpoint/return/re-entry, shimmer fit and local effect lifetime, and drop-in replacement resolution/metadata/UVs. Run regional sanitized/native/presentation syntax suites and appropriate Lake checks. Commit only owned paths; task-3-report.md includes exact code checks and runtime limits.

- [x] Write and run a focused failing behavioral regression before implementation.
- [x] Implement only the owned production paths and required test support.
- [x] Run the named native/static/sanitized checks, record failures and exact proof boundaries.
- [x] Commit bounded owned changes and self-review.
- [x] Obtain an independent spec-and-quality task review; fix required findings.

### Task 4: Woodfall return portal transparency

Trace the MM Woodfall-to-OoT return portal that the user reports as an opaque magic/scroll surface. Own the precise Woodfall return rendering/material paths and their tests, likely mm/2s2h/Enhancements/Gameplay/ComboWoodfallPassage.*, relevant forest return graphics/WoodsMystery material module and combo/entrances/WoodfallPassage.h if needed. Do not edit Task1 actor sources, Task2 valley modules, Task3 generic RegionalPassagePresentation/material/endpoint files. If the exact rendering path is shared, report ownership before mutation so it can be assigned deliberately.

Read the actual native/MM + donor/archive material state and draw route. Establish a failing test for the reported opacity/scroll issue. Determine whether an opaque backing is intentional separately from the animated magic overlay; fix unintended opaque overlay blending/alpha/OPA-vs-XLU/material binding at its source. Preserve geometry, portal trigger/returns, scrolling continuity and other portals. Use normal texture replacement paths rather than baking pixels. Tests should exercise actual emitted commands or the native draw/material boundary, not only grep source text. If the current native material is correct and the problem is a specific pack override, identify and correct the relevant delivered resource with a minimal deterministic patch and source recipe, without changing unrelated packs. Run existing Woodfall/vertical-scroll/native graphics checks appropriate to the change and report exact evidence; task-4-report.md.

- [x] Characterize actual native draw behavior; no unintended overlay was reproduced and no production render change was made. The report explicitly distinguishes characterization from behavioral RED/GREEN.
- [x] Implement only the owned production paths and required test support.
- [x] Run the named native/static/sanitized checks, record failures and exact proof boundaries.
- [x] Commit bounded owned changes and self-review.
- [x] Obtain an independent spec-and-quality task review; fix required findings.

### Task 5: Controller integration and durable handoff

- [x] Integrate reviewed worker commits, resolve interface dependencies and add CI entries for new meaningful regression commands.
- [x] Verify all original 128-file changes remain in the cumulative candidate; run relevant combined suites and native translation-unit checks.
- [x] Independently review the full scoped diff and correct material issues.
- [x] Save source checkpoints after independently verified milestones, with exact refs/trees/asset identities and independent restore proof. Package only source/required generated overrides, not original archives.
- [x] Deliver source/build evidence separately from configured gameplay proof; keep develop/master unmerged. Full Linux combo build passes; configured gameplay remains unrun and explicitly excluded from acceptance claims.
