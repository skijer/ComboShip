# Audited Cumulative Publication Implementation Plan

> **For agentic workers:** Use executing-plans for integration/publication and
> dispatch independent source, asset and verification work in parallel as the
> user explicitly authorized. Preserve a durable source checkpoint per milestone.

**Goal:** Publish the complete audited pass, run final CI and deliver matching
executable/host archives while preserving develop and accepted inputs.

**Architecture:** Merge the exact source tips through their existing ancestry.
Publish an exact verified tree through resumable individual Git blobs and
guarded refs. Keep optional packs and runtime acceptance explicit.

**Tech Stack:** Git, Python 3, native C/C++, existing asset generators, GitHub CI.

**Spec:** `docs/poc/cumulative-publication-20261024.md` and the user's explicit
publication authorization reproduced by that scope record.

## Global Constraints

- Include all seven exact authorized commits and retain earlier integrations.
- Preserve public develop `196ac9b7f50721dbc5102d6bf0d6b1f9b3fc58eb`; master absent.
- Preserve accepted input archives and original pixels/geometry/resource paths.
- Reuse verified asset blobs; packed parts retain 96 KiB/128 KiB bounds.
- Save recoverable checkpoints and verify complete source-tree restoration.
- Preserve every required check; label unavailable configured runtime evidence.

## Review Focus

- Combined Knife profiles and stronger maps both survive regeneration.
- Pause, D-pad, monkey and Fado source/test coverage survives UI integration.
- Native shop scene/room/camera metadata matches the executable/host archives.
- Source blobs, tree modes, deletion handling and guarded remote ref are exact.
- Optional packs remain separate and do not overwrite newer prepared records.

### Task 1: Combine and preserve

- [x] Verify version-2 recovery ZIP SHA/CRC and all seven input trees.
- [x] Merge UI, Knife and casings onto Fado's inherited base/pause/monkey chain.
- [x] Check the combined aura generator/header deterministically.
- [ ] Save a durable integrated source bundle/patch with exact tree identities.

### Task 2: Verify and repair

- [ ] Run targeted native sword/GI, UI, pause/monkey/Fado and D-pad checks.
- [ ] Run generated asset, collision, location-title and CI preflight checks.
- [ ] Obtain independent integration review and resolve actionable regressions.
- [ ] Checkpoint any bounded source repairs with evidence.

### Task 3: Publish and finish CI

- [ ] Prepare exact changed-path size/hash manifest and verified reuse receipts.
- [ ] Upload only missing blobs; verify remote tree before guarded ref update.
- [ ] Publish candidate/cumulative branch and PR; monitor Windows/Linux plus
      all six regression groups on the exact published source.
- [ ] Repair in-scope failures, checkpoint and repeat affected CI as needed.
- [ ] Deliver build/archive links, source identities, ledger and runtime limits.
