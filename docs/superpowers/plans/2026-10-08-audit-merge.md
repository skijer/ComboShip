# ComboShip Audit Merge Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans to execute this recovered integration task by task. Checkpoint every completed source integration.

**Goal:** Recover the October 8 audit merge into one reviewable candidate with matching Windows/Linux builds and generated port archives.

**Architecture:** Start at current PR #39 `7de5b7fa`, preserving the post-audit starting-item fix. Apply only incremental follow-ups, merge shared source deliberately, then wire the accepted bottle policies/assets into both existing renderers.

**Tech Stack:** Native C/C++, Python production fixtures, CMake, GitHub Actions.

**Spec:** `docs/poc/audit-merge-handoff-20261008.txt`; accepted checkpoint READMEs and the recorded later overcast targets.

## Global Constraints

- Preserve develop `a82852a3` and keep this candidate on `bridge/audit-merge-20261008`.
- Retain current PR #39 diagnostic fixes and starting-save migration repair.
- Keep sword shop shelf transforms and existing custom Din layers; preserve both hosts' ownership routing.
- Apply one Rod POC6 route and one song/bottle route; omit overlapping full PR exports.
- Preserve the accepted bottle mesh and donor texture bytes; keep Princess/seahorse poses static as documented.
- Use Princess #80D846, fairy #FFA0EB plus four faint motes, seahorse #FFE66D, gold dust #FFD45A, mushroom #DE64F5.
- Build/static verification does not establish in-game appearance or runtime acceptance; leave the master unchanged.

## Review Focus

- Both games, Alt on/off and independent donor settings retain item ownership and resource selection.
- Sword shelf transforms survive equipment/Four Sword conflict resolution.
- Bottle translucency, glow, camera/matrix restoration and graphics-arena exhaustion are safe.
- Current saves and legitimate Pendant/trade ownership survive all new paths.
- Seasonal sky changes preserve indoor/story/cutscene and explicit global weather priorities.

### Task 1: Recover source sequence and repair the receipt gate

**Files:** PR40 incremental export; both Kaleido hosts; `mm/2s2h/Rando/ItemReceiptText.cpp`; the Lantern/tunic priority tables; both Rod cast wrappers; song/bottle composer; summer renderer; `tests/item_receipts/key_receipt_test.cpp`.

- [x] Apply PR40 follow-up, the two wand commits, Lantern, Rod POC6 original route, song/bottle POC2, summer visibility and late tunic in the audited order.
- [x] Reproduce the `CVarGetInteger` key-receipt fixture linkage failure with installed native headers; supply the fixture's normal configuration service, preserving actual receipt code and assertions.
- [x] Run `python3 -B tests/item_receipts/run_key_receipt_tests.py --sanitizers`, receipt, shared-slot UI, song, summer and tunic fixtures; record exact results.
- [x] Commit source recovery and gate repair independently.

### Task 2: Preserve equipment and held/shelf sword behavior

**Files:** equipment recovery patch; `soh/mods/items/logic/weapon_upgrades.c`; `mm/mods/items/logic/weapon_upgrades.c`; equipment pause/grant fixtures.

- [x] Apply the equipment recovery and inspect both conflicts against PR40 and recovered candidate sources.
- [x] Retain acquired-only equipment, u16 item input, authored Four Sword routing and later held/shelf logic together.
- [x] Run `tests/mm_equipment_pause/run_ownership_tests.py`, `tests/nei_held/run_held_sword_tests.py` and the production grant/UI fixtures named by the checkpoint.
- [x] Commit the integrated equipment candidate with conflict decisions recorded.

### Task 3: Integrate accepted bottle meshes/shimmer and later elemental sheen

**Files:** existing `ComboBottleContentsDraw.h`, both native GI renderers/presentation paths, accepted mesh conversion inputs/resources, bottle and elemental arrow fixtures.

- [x] Apply the later elemental sheen candidate incrementally; preserve original casing/core colors and native/Alt arrow selection.
- [x] Connect the prepared bottle policy profile IDs explicitly to the existing content draw IDs; integrate accepted meshes without revising their proportions or texture pixels.
- [x] Test both native and foreign renderers, resource/matrix restoration, policy colors and arena failure paths, using real production functions.
- [x] Run the five sheen checkpoint commands and the bottle/fairy production fixtures; commit the complete integration.

### Task 4: Reconcile approved audit-chat follow-ups

**Files:** Rod of Seasons pool/settings/ownership and relevant weather renderers; Hylia's Grace flight particle path.

- [x] Preserve Rod mode requirements: all four in one, individual season shuffle, and dungeon-gated unlocks applying only to gated mode; Spring uses the OR of Song of Storms/Jabu/Water Temple.
- [x] Carry the recorded seasonal sky targets: rain-consistent Spring, clear Summer, Autumn clouds following intermittent rain, overcast Winter; preserve global/story priority.
- [x] Correct Grace's flying sparks through its existing shared particle color path.
- [x] Record any missing historical design detail before choosing a compatible implementation; verify settings/pool/grant/save behavior and both host paths.

### Task 5: Verify, publish and build the exact candidate

**Files:** cumulative integration notes; production fixtures; existing CI workflow.

- [ ] Run source/resource integrity, formatting, asset collisions and the complete applicable cumulative regression gate; retain logs and a recovery bundle.
- [ ] Obtain an independent whole-branch review under the executing-plans skill; fix required findings with reproductions.
- [ ] Publish this branch and draft PR under the user's existing ComboShip publication authorization.
- [ ] Inspect the exact candidate's CI gate, Windows/Linux application builds, port-archive generation and package artifacts; resolve integration/build failures.
- [ ] Deliver candidate/build links and the short remaining in-game acceptance list.
