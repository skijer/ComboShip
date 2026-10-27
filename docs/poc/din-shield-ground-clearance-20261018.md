# Din Fire Shield ground clearance source candidate

## Baseline and scope

- Repository: `marsh6487/ComboShip`.
- Parent: `43131342882972eeb4a257599ec8b061c7dc6d43`, the inspected head of
  `integration/big-merge-20261016` when this work began.
- Candidate: `fix/din-shield-ground-clearance-20261018`.
- User evidence: cor likes the current shield appearance but reports that about
  half clips below ground when Link braces. The game was not run here.

The held effect previously used an unconditional hand-local offset and scale.
When guarding lowers/rotates the hand, the same full-size flame meshes follow
it below ground. Both engines now measure every vertex of the loaded surface
and rim after the existing final transform. While Link is grounded, the draw's
world-Y translation is raised only enough to put its lowest vertex one game
unit above `actor.floorHeight`. Already-clear and airborne draws retain their
existing attachment. Matrix push/pop and recorded Matrix_Put preserve the
caller's hand matrix and interpolation path.

Only the two shield renderer C files change production behavior. Shield size,
geometry, textures, bracer/hand placement, colors, animation/fade clock, SFX,
get-item model, options, eligibility, ownership and native shield colliders
remain unchanged. Existing accepted scene/equipment archives are untouched.

## Configuration and asset dependencies

Keep the existing mod load order and matching Din equipment packs. Enable
Alternate Assets and Din Fire Shield. This source correction requires a new
executable; replacing an O2R alone cannot apply it. No new O2R is needed.

The existing private resources remain:

```
objects/din_fire_shield/poc1/SurfaceDL
objects/din_fire_shield/poc1/RimDL
objects/din_fire_shield/poc1/SurfaceVertices
objects/din_fire_shield/poc1/RimVertices
objects/din_fire_shield/poc1/FlameTex
objects/din_fire_shield/poc1/FlowTex
```

The current user-installed archives were not supplied for this task, so their
hashes and load order were not independently inspected. Existing code guards
and resource resolution are retained. OoT child Deku/adult Hylian and MM
human/ported adult Hero Shield paths use their existing bracer requirements.

## Verification

Both original shield suites passed before changes. New compiled regression
cases failed on below-floor vertices in both original renderers before the fix,
then passed afterward. Cases cover both ages, low braced hand frames, rotated
shield aiming, curved surface and rim bounds, elevated floors, already-clear
and airborne draws, full-size geometry, unchanged player state and restoration
of caller matrices. These are synthetic engine-boundary frames, not footage
of the in-game animation.

The following production-boundary suites passed:

```
python3 scripts/diagnostics/run_din_fire_shield_tests.py
python3 scripts/diagnostics/run_mm_din_fire_tests.py
python3 scripts/diagnostics/run_mm_din_fire_damage_tests.py
python3 scripts/diagnostics/run_din_fire_sword_tests.py
python3 scripts/diagnostics/run_din_fire_sword_damage_tests.py
python3 scripts/diagnostics/run_din_progressive_hand_tests.py
```

The OoT damage suite needed the already-available nlohmann JSON include path
on CPATH; its first attempt was blocked by that missing environment dependency
and the rerun passed. MM compiler output retains the baseline controller-macro
redefinition warnings. Changed C/fixture files were formatted with
clang-format 14.0.6; `git diff --check` passed. Independent read-only review
found no Critical/Important issues and reran both shield suites successfully.

No full game executable was built and no runtime appearance is claimed.

## Runtime verdict and next check

Implemented and source/compiled-fixture verified. Runtime untested, not
accepted or promoted to the cumulative build. This is a horizontal-floor
clearance correction: uphill slopes, steps and walls under the unchanged
shield footprint can still intersect. It does not sample terrain around the
shield or modify collision.

At the next cumulative build, compare guarding/bracing on a level floor with
the same packs and options, for child/adult in both games. Rotate shield aim,
release/re-raise it, pause/resume, and check that it retains the desired visual
relationship to the bracer. Inspect a slope and step separately. User visual
acceptance is still required before promotion.

## Recovery

The accompanying recovery ZIP contains an incremental git bundle, a patch,
all seven changed files, exact candidate/base commit and tree IDs, and the
verification logs. Clone `https://github.com/marsh6487/ComboShip.git` into a
fresh checkout and fetch the bundle's candidate branch after obtaining the
parent commit above. The bundle preserves the exact local commit. Alternatively,
apply the patch at that parent; the result must match the recorded candidate
tree. No other transient worktree is required.

The candidate remains local plus the recovery package. Automatic approval
review rejected creation of a GitHub tree because this request did not
explicitly authorize disclosure/publication to the proposed repository
destination. No GitHub tree, commit or branch was created. Publication needs
explicit authorization; the code fix and private recovery handoff proceed.

To integrate later, cherry-pick the candidate commit onto the chosen cumulative
line and rerun the two shield suites. Discarding the candidate leaves the
parent build and existing archives unchanged.
