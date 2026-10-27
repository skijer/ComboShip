# MM Native Black Market Shop Implementation Plan

> **For agentic workers:** Use the existing native POC and implementation workflows. Keep explicit file ownership and validate each boundary before integration.

**Goal:** Use 2Ship's actual native shopkeeper behavior and camera setup for the Black Market while retaining Treasure Gal's model, sway, blink, custom stock and transactions.

**Architecture:** Allocate a real `EnOssan` controller and real `EnGirlA` stock actors. Execute the original native state functions instead of MM's shared custom `BrowseInput`/`State` loop. A private-market adapter supplies presentation, eligibility, text IDs, layout and receipt boundaries; ordinary shops and standalone builds retain their existing behavior.

**Tech Stack:** Existing C actor implementation, C++ Black Market adapters, native MM scene resources, native ASan/UBSan fixtures and deterministic Python asset preparation.

**Spec:** The user's explicit instruction in this conversation: keep Treasure Gal and emulate the exact native 2Ship shopkeeper/camera code, including the necessary Black Market adaptations.

## Global constraints

- Preserve source checkpoint tree `843fca161d8b6fa573d3cb5d1c991c174b5aedd4` as the previous candidate; it was independently restored and saved before this work.
- Preserve all OoT implementation, title-card, Woods crash and D-pad corrections. The new work supersedes only MM's separate browsing controller.
- Keep Treasure Gal's skeleton, animation, blink sequence, scale and resource resolution. Never cast an allocated `EnTakaraya` to the larger `EnOssan`; Gal needs 24 limb entries whereas the native controller has 18 inline entries.
- Use original native input accumulation, cursor navigation, look/browse/select/cancel/tween/continue states, including one-B exit and inward-stick return to keeper.
- Preserve seeded stock, eligibility, pricing, uniqueness, save/seed/slot ownership and the complete GI/tutorial/cape receipt chain; native purchasing must not independently charge or grant.
- Preserve camera ownership protection and check capacity immediately before native Start. Use native scene camera/cutscene records; no MM manual eye/at/FOV writes or custom settling gate.
- Publication and hosted builds remain paused. Runtime proof and user acceptance remain separate from source/native integration checks.

## Review focus

1. Actor initialization precedes OnSceneInit: scene camera metadata must already exist before native GetCutscenes.
2. Scene teardown or foreign camera takeover must release only this shop's ownership.
3. Receipt acknowledgement and complete follow-up-message completion are separate predicates.
4. All four forms may browse; Trading Post-specific Deku refusal and ANI/Zora animation switches do not apply to Treasure Gal.
5. Custom room coordinates and GI envelopes require separate framing/motion proof; do not claim byte identity with vanilla's flat shelf layout.

## Task 1: Native actor boundary and behavior regression

Files: `mm/include/black_market_shop_adapter.h`, `mm/src/overlays/actors/ovl_En_Ossan/z_en_ossan.c`, native shop test fixtures.

Interface: the typed private `ComboBlackMarketShopAdapter` is returned only for the private controller. Its initialization, talk eligibility, text mapping, cutscene-capacity/identity, item-position and receipt predicates supply the necessary adaptations.

- [x] Add a failing differential against native B exit, input timing, diagonal navigation, select/return tween, camera calls and receipt continuation.
- [x] Add guarded native adapter boundaries; share original UI initialization and actual action functions. Keep ordinary actor behavior unchanged when the adapter is absent.
- [x] Verify C/C++ native compilation, all forms, all slots, sparse/out-of-stock cases and unchanged non-market controls.

## Task 2: Treasure Gal, stock, scene and receipt integration

Files: MM `BlackMarket.cpp`, a focused private native-shop helper, native scene preparation recipe and its tests.

- [x] Replace only the private MM room's keeper entry with a correctly allocated native controller; retain Treasure Gal drawing and separate correctly sized animation arrays.
- [x] Supply registered native stock actors with Black Market callbacks and actual native item positions/focus; draw from their positions rather than resetting anchors each frame.
- [x] Prepare native Fixed1 records and keeper/left/right/return linked scene commands before actor initialization. Preserve native default FOV60, rates1.0/0.75, HUD4 and letterbox32; author room coordinates explicitly.
- [x] Remove MM's shared custom browsing loop/cursor and automatic previous-shelf receipt resume. Keep native continue-shopping flow and adapt acknowledgement/full-chain completion explicitly.
- [x] Verify the asset delta, Treasure Gal buffer bounds, unchanged transaction semantics and adapted framing/item motion.

## Task 3: Combined verification and recovery

- [x] Run the real native actor/camera/manager fixture, transaction/receipt, keeper, shelf and relevant asset checks; preserve independent ownership and pool-exhaustion regressions.
- [x] Review with native source as the behavioral reference; fix concrete failures before broadening checks.
- [x] Save an independently restorable source checkpoint and record exact source/asset identities, verified boundaries and outstanding configured runtime checks.
- [x] Leave public branches and accepted masters unchanged pending the user's publication authorization.

## Source completion ledger (2026-10-24)

Task1: native C adapter, full native actor differential and unchanged ordinary37-frame trace pass. Task2: actual controller/stock/Gal buffers, typed scene validation, native actions, all message/receipt continuation boundaries and ownership/destruction regressions pass. Task3: scoped independent reviews pass; source/native gates are documented in the integration POC. Recoverable WIP source commit20f56bff/tree20025c0f was saved and restored before final corrections; a final checkpoint follows. No public source/hosted build. Three bridge handoff, legacy scene override consolidation, full build and configuration-specific runtime/user acceptance remain open release gates.

Ruling: do not fetch missing optional dependency headers or modify unrelated Din sword code to make broader harnesses green. Record those environment/baseline limits; run changed native boundaries with real headers and retain the full application build gate.
