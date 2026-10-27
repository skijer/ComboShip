# ComboShip chests and opt-in Midna implementation plan

> **For agentic workers:** Use focused parallel tasks with disjoint ownership, then review the combined branch.

**Goal:** Package the approved six chest models and bring contents-based chest rendering and optional Midna to both engines in ComboShip.

**Architecture:** Preserve the cumulative ComboShip candidate at `39b63af006ed3157177a501ee2cab1e9af064159`. Port the existing SoH chest and Midna behavior additively; adapt MM actor drawing and companion sound callsites to its own resource manager and lifecycle. Keep model assets in optional packs, with native fallback when disabled or unavailable.

**Tech stack:** C/C++, XML display lists, O2R ZIP archives, existing focused CPU harnesses.

**Spec:** User requests in this conversation: fixed chest pack, ComboShip compatibility, and default-off Midna checkboxes for both OoT and MM.

## Constraints and review focus

- Preserve all cumulative integration features; do not replace either engine wholesale.
- Chest geometry, UVs, textures and normals must match the approved pack. Boss-key chest resources remain provided by the active game pack.
- Keep actor chest types, flags, rewards and minigame concealment intact.
- Midna replaces only Navi/Tatl companions; healing, collectible and story fairies retain native behavior.
- Independent per-engine Midna defaults are off. Disable must restore model and sound immediately; scene and game transitions clear private audio.
- Optional-resource failure retains native rendering and sound. No stale raw-resource pointer or fixed-index patch of custom chest display lists.
- CPU checks and builds do not constitute in-game acceptance.

## Tasks

- [x] Chest task: port the SoH size change from `d676a5ff`; add MM classification and private model selection under `objects/object_box/cor_3ds_chests_mm/`; add meaningful regression coverage. Own chest actor/helper/test files only. Return UI additions for integration.
- [x] OoT Midna task: port companion/audio from `822f46b15`, adding a default-off per-game gate. Own SoH fairy actor, HUD sound routing, audio player/resources and their tests. Preserve ComboShip resource ownership. Return menu/build changes for integration.
- [x] MM Midna task: adapt the same private model, pose, eye and sound resources to Tatl using MM actor fields and APIs. Own MM fairy actor, companion audio and tests. Use an MM-prefixed C API to avoid cross-engine linkage ambiguity. Return menu/build changes for integration.
- [x] Coordinator: package fixed SoH chest archive and private MM counterpart, retrieve the current POC3 Midna pack, integrate UI/build hooks and per-game installation instructions, and check resource preservation.
- [x] Verify all changed production paths using focused harnesses and available build gates. Review opt-out, missing resources, game transitions, boss/minigame handling and two-engine symbol isolation. Publish a reviewable candidate without promoting the accepted baseline.
