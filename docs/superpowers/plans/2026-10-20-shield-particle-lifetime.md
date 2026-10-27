# Shield Particle Lifetime Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans to implement this bounded correction. Steps use checkbox syntax for tracking.

**Goal:** Prevent the supplied Lost Woods exit crash and preserve light ownership across scene cleanup.

**Architecture:** Keep the engine's light allocator and effect pool unchanged. Make the existing shield-particle destructor tolerate a failed allocation and release its light exactly once, in both engines.

**Tech Stack:** C, Python standard library, native compiler with undefined-behavior sanitizer.

**Spec:** The user reports an OoT Lost Woods bridge exit crash while Goron. The supplied dump identifies `soh.dll` RVA `0xC6156D`: `EffectShieldParticle_Destroy` reads `lightNode->next` when both `lightNode` and the scene light-list head are null. The preceding equality check treats two null pointers as a valid light. `Effect_DeleteAll` visits inactive effects, so released pointers must also be cleared to prevent subsequent destruction of reused lights.

## Global Constraints

- Baseline: `4af924940`, retaining the native transformation camera correction; crashing executable: `450b293`.
- Preserve input log/dump, original source checkout, form behavior, scene assets, placements, progression, and renderer behavior.
- Do not infer game runtime proof from a compiled probe or native object.
- Saria's independently diagnosed archive references belong to a separate workstream.

## Review Focus

- A null light pointer with an empty scene light list must be safe.
- Exhausted light capacity must not stop other lights from being released correctly.
- Head, middle, and tail removal must retain list links and occupancy.
- Repeated effect destruction must not decrement occupancy again.
- A new scene's reused light slot must survive cleanup of an old inactive effect.

### Task 1: Release shield-particle lights once

**Files:** Modify `soh/src/code/z_eff_shield_particle.c` and `mm/src/code/z_eff_shield_particle.c`; create `scripts/diagnostics/run_shield_particle_lifetime_tests.py` and `tests/effect_lifetime/shield_particle_test.c`.

**Interfaces:** Consumes existing `EffectShieldParticle_Destroy(void*)`, `LightContext_InsertLight`, and `LightContext_RemoveLight`. No new production interface.

- [x] Compile production declarations, initialization, destructor, and real allocator/list functions in a small fixture. Assert the five review conditions above.
- [x] Run `python scripts/diagnostics/run_shield_particle_lifetime_tests.py`; verify the unmodified functions fail, including the dump's null dereference and later scene ownership corruption.
- [x] Require an owned node before removing it, rely on `LightContext_RemoveLight` for list unlinking, and clear the effect's pointer afterward.
- [x] Run the same command; expect all ten cases to pass across both engines.
- [x] Compile the two full native translation units in Combo and solo modes; run nearby transformation tests and inspect diffs.
- [ ] Commit only owned paths, publish a recoverable candidate checkpoint within existing authorization, and keep runtime/master status unproven.

## Execution ledger

Ruling: Execute the already authorized reversible bug correction without a separate plan-approval pause; higher-priority session instructions require completing concrete reviewable work first. Cost if wrong: isolated candidate can be discarded.

Pre-flight: No shared code interfaces with the Saria archive repair.

RED: All ten cases fail on the baseline functions; the two empty-list cases in each engine report null `LightNode` access under UBSan.

GREEN: All ten cases pass; four full native translation-unit compiles pass with pre-existing controller/graphics warnings. Native transformation sequence/camera suite passes. No full executable/runtime claim.
