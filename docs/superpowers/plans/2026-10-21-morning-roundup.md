# Morning Roundup Integration Plan

> **For agentic workers:** Use executing-plans for integration and independent read-only audits for the existing subsystems. Track completed steps here and in the roundup record.

**Goal:** Commit and publish every requested October 21 morning implementation as one recoverable ComboShip candidate.

**Architecture:** Start from the actual remote `develop` at `66bde021ccd651a3728db2338251961b1a44c9f7`, retaining merged PR48–52. Compose the separately verified Woodfall and conditional Great Fairy checkpoints, then verify their shared player-exit and arrival boundaries. Existing actors, native assets and particles are audited rather than reimplemented.

**Tech Stack:** C/C++, Python native fixtures, ASan/UBSan, Git and GitHub Actions.

**Spec:** cor's October 21 morning-roundup request, copied into `docs/poc/morning-roundup-20261021.md`.

## Global Constraints

- Preserve all earlier cumulative integrations, native/original/combined game guards, current CI fail-fast changes and accepted input archives.
- Keep original solo OoT Fairy routing and Combo private Lost Woods routing intact.
- Conditional Fairy return belongs only to a visit through the authored Lost Woods collision exit.
- Woodfall arrives at the normal exterior entrance and does not skip its traversal.
- Both black entrance faces use the actual authored NEI twinkle; Mirror retains pale blue/white.
- Preserve optional appearance/model/flame packs separately; source publication is not configured-gameplay acceptance.
- Generate durable, independently restorable source checkpoints before external publication.

## Review Focus

- Shared OoT player exit interception must not let Woodfall claim Fairy or native/shuffled exits.
- Explicit paired-game/Farore arrivals must supersede saved conditional Fairy and pending Woodfall state.
- Save/reload, file/seed changes and destruction must not leak return state.
- Original solo OoT and Combo private forest entries must retain their distinct destinations.
- Fresh source checkout must contain every required asset part and narrow scroll binding.

### Task 1: Verify baseline and recover pending checkpoints

**Files:** Existing `docs/poc/*20261021*`, native forest assets and source manifests; new roundup record.

- [x] Record the actual current baseline, checkpoint identities and item-by-item provenance.
- [x] Run the baseline native solo-forest route tests; retain source and asset hashes.
- [x] Audit core bugfixes, native forest/particles/scrolling and additional morning omissions independently.

### Task 2: Compose Woodfall and conditional Fairy return

**Files:** `combo/entrances/LostWoodsEntrance.h`; both engines' passage/arrival controllers; `soh/src/code/z_play.c`; `soh/src/overlays/actors/ovl_player_actor/z_player.c`; `tests/lost_woods_entrance/`; CI regression manifest.

- [x] Merge Woodfall checkpoint `3352869b5cec8560fcaba20ea1c32df53200e89d` and Fairy checkpoint `34d6f1f3266bafdee86a144749ba66faf52abb0e` into the isolated candidate.
- [x] Resolve only shared integration points, retaining current parallel CI architecture.
- [x] Run both sanitizer fixtures and the complete entrance runner; correct any reproduced integration failure with a failing regression first.
- [x] Verify actual original-game/Combo routes, native collision inputs where available and source-only reconstruction.
- [x] Commit the bounded verified integration and record every unresolved runtime check.

### Task 3: Review, preserve and publish

**Files:** This plan; `docs/poc/morning-roundup-20261021.md`; source recovery receipt; Git branches/PR.

- [x] Inspect all audit findings, complete targeted verification and obtain a fresh whole-branch review.
- [x] Verify an independent Git-archive restore and record exact source/tree/asset identities.
- [ ] Push the recoverable source candidate, publish the cumulative integration and start the normal hosted gate.
- [ ] Report the exact commit/build link, extra included work and untested gameplay honestly.
