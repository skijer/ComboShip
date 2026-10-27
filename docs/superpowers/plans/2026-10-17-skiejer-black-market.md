# Skiejer Black Market Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement task-by-task. cor explicitly requested implementation after approving the in-chat design; continue inline without another design/permission loop.

**Goal:** Add the seed-fixed eight-item magical shop in MM Lost Woods, with a correct local return.

**Architecture:** A shared header catalog and deterministic seed/state policy feed the MM-native shop adapter. A private scene/entrance and portable asset builder isolate the room, shopkeeper, and scrolling materials from normal scenes and shops.

**Tech Stack:** C/C++20 engine/launcher, nlohmann JSON, native GameInteractor hooks, Python asset conversion/verification, O2R resources.

**Spec:** `docs/superpowers/specs/2026-10-17-skiejer-black-market.md`.

## Global Constraints

Eight fixed offers: six NEI plus two wildcard; two/four/two price tiers; tier prices 30–90 / 100–150 / 160–200. Independent seed-bound dungeon checkbox uses 0/1/2 unique clears. Full agreed catalog stays in scope. No solver/item-pool dependency. Preserve POC9 stump, scenery and native routes. Use the accepted Granny donor alternative because the retained Zora room lacks its full mesh; every interior material magical and scrolling; goth shopkeeper. Centered OoT-style place cards for both destinations. New exit returns to MM Lost Woods. No master promotion or automatic runtime claim.

## Review Focus

- An old or malformed seed never invents/rerolls stock or makes an unusable exit.
- A failed, repeated, or already-owned purchase never spends rupees or grants the wrong progressive power.
- A time reset, save reload, or paired-game change never loses purchases or counts shuffled rewards as dungeon clears.
- Normal shop entrances, cutscenes, NPCs and the accepted corner-stump route remain unchanged.
- Incomplete packs/Alt settings never activate an invisible doorway or load a resource outside its owner.

## Tasks

### 1. Catalog, generation and persistence policy

Create `combo/rando/BlackMarket.h` and `tests/black_market/stock_test.cpp`; a Python runner compiles real production policy. Interfaces: `Generate(seed, options) -> json`, `Validate(json) -> bool`, exact string item keys, pure tier/unlock/purchase helpers. Add regression cases before implementation, watch failure, implement, and pass. Cover full catalog and quotas over thousands of seeds, independent RNG/pricing, malformed JSON and purchase state. Commit the tested policy.

### 2. Bind settings, seed JSON and durable state

Add shared menu controls in `combo/gui/ComboMenu.cpp`, seed generation/reload/save binding in `combo/ComboShip.cpp` and `combo/ComboRandoHeadless.cpp`, and required MM/SoH bridge exports. Interfaces consume task 1's JSON and report seed-stable settings/stock plus native unique-clear masks. Extend real host/seed/save fixtures RED→GREEN; old seeds stay opt-out. Commit the tested seed and save plumbing.

### 3. Native MM scene, shelves, keeper and local return

Create `mm/2s2h/Enhancements/Gameplay/BlackMarket.h/.cpp` and dedicated route policy `combo/entrances/BlackMarketEntrance.h`. Register unused scene `SCENE_UNSET_02` and entrance `ENTR_SCENE_UNSET_09` after checking no owner; consume private scene resources. Use a dedicated keeper with eight visible resolved item models and native Buy / Next offer / Leave messages; use a privately routed goth skeleton with isolated idle/blink. This candidate leaves `EnOssan` unchanged. Add actor/transition fixtures before implementation. Exit, void and reload return to the new Lost Woods doorway safely without cross-portal flags or Farore TOP corruption. Commit the tested native integration.

### 4. Portable donor conversion and magical scene assets

Create `tools/black_market/` builder and `tests/black_market/` asset checks using the exact retained donors. Extract and scope the complete approved Djipi Granny geometry and derived collision and external dependencies, materialize all replacement textures, bind scrolling independently from native scene draw assumptions, scope goth assets, draw a private exterior shell separately in Lost Woods, leaving the accepted archive unchanged. Preserve unchanged bytes. Produce editable/game candidates, material inventory/receipts and a central preview. Run asset contract tests and inspect the render; iterate only within the approved visual scope. Commit source tools and appropriate bundled resources.

### 5. Integration verification and recovery checkpoint

Run focused production/native tests, touched-unit compile checks and existing affected entrance/save/item/scroll tests. Record unavailable full build/gameplay explicitly. Review the complete diff in a fresh context using the requesting-code-review skill, fix important findings with regressions, and preserve source commits/bundle/patch plus durable asset deliverables. No merge or master promotion.
