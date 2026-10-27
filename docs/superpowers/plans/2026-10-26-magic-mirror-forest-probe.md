# Magic Mirror Forest Probe Implementation Plan

> **For agentic workers:** execute this user-approved probe inline; use a fresh
> whole-change reviewer after native checks.

**Goal:** Test an actual forest reflection, magical activation chime, native
Farore departure/arrival sounds, and a bounded
Time Gate camera in both engines while preserving recall behavior.

**Architecture:** Shared Mirror renderer/runtime, native per-game audio calls,
owned subcamera, and a private MM KEEPON4 camera setting. Keep native/HD resource
keys identical and generate icons from the textured mesh.

**Tech Stack:** C/C++, native SoH/MM APIs and GBI, Python/Pillow/NumPy, ImageGen.

**Spec:** `docs/poc/magic-mirror-forest-probe-20261026.md`

## Global Constraints

- Parent `fa81f7926038f2aa85c7f3e73c96fec043168b66` stays unchanged.
- Keep Farore animation, green effects, prompt/save/equip/arrival and game guards.
- Keep develop/master unmerged; deliver source/assets, not a runtime success claim.

## Review Focus

- Interrupted/holstered casts must not acquire or strand a camera.
- Reused camera slots and a different cutscene's camera must remain untouched.
- MM forms must select a valid private camera mode, never CAM_SET_NONE.
- Textures must respect native arena limits and preserve untextured frame/glints.
- Audio must use each native host: one magical prompt chime, then Farore only
  at committed departure/arrival. No wind effect or cancelled departure cue.

## Tasks

- [x] Sound: observe failing actual-player/runtime tests; split magical activation
  from committed Farore departure/arrival and verify cancellation, holstering,
  rejected handoff, repeated updates and one-shot arrival under ASan/UBSan.
- [x] Camera: add native structure/pool ownership tests and runtime timing
  assertions; implement shared owned-subcamera and private MM KEEPON4 setting;
  verify cancellation, allocation failure, reuse, teardown and both hosts. Resolve
  the review finding with MM's existing native manager ownership guard and run
  the actual manager's idle-snapshot/interrupted-return paths under ASan/UBSan.
- [x] Reflection: add UV/material boundaries and native guarded-draw assertions;
  generate texture, update native/HD icons from the mesh, inspect rendered front
  and oblique previews; run geometry/arena/resource checks.
- [x] Cast texture follow-up: add private green botanical seal and vine-wisp art,
  preserve motion/timing and other items, verify both production dispatches,
  green palette, missing-texture fallback, native assets and offline comparison.
- [x] Handoff: review the complete diff, resolve material issues, commit bounded
  changes and independently restore a deterministic source recovery capsule.

The explicit approved-probe scope bypasses another design/plan approval round.
