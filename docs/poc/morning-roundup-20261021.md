# ComboShip morning roundup — October 21, 2026

## Authorization, baseline and scope

cor requested all morning source implementations and bugfixes committed and pushed to `marsh6487/ComboShip`, and an audit for omitted work. This explicitly supersedes the earlier conditional-Fairy instruction to hold publication while collecting fixes.

Candidate: `integration/morning-roundup-20261021`, isolated from current remote `develop` at `66bde021ccd651a3728db2338251961b1a44c9f7`, tree `5ba3645e9d78d97345529b37728f614c51c73caa`. The remote baseline preserves PR48/49/50 gameplay fixes and PR51/52 parallel CI/fail-fast behavior. Source integration and publication do not promote a runtime-accepted scene master.

## Requested checklist

| Requested implementation | Baseline/checkpoint and verification status |
| --- | --- |
| 2Ship Lost Woods Saria and Termina Field song-giver dialogue freeze | PR48 includes `6d171889`; nine real arrow/button cases and the complete native actor object pass. |
| Black Market entrance/access/crash | PR48 retains `8ab33dee` and native door/scene-loader work. Five-form startup, both-host door/transaction safeguards, 18 bundled cycles and 18 more cycles with actual native base/keeper resources pass. |
| Native 2Ship private Lost Woods framing/textures, authored tree tunnel and scene scrolling | PR48 native forest presentation preserves stock ownership and supplies the authored additions. 48 resource checks: 46 pass, two optional original-input skips. Fresh tracked-source restore reconstructs all 85 resources/12 authored textures for both ports. |
| OoT mask transformation skipping SFX | PR48 retains input `58e5c4bd`; eight real-DSP, 116 controller and 28 lifetime cases pass. |
| Lost Woods room 7 Woodfall shortcut | Published checkpoint `68aec126` is tree-identical to local `3352869b`; merged in `5797ff8b`. Five production sanitizer fixture groups pass with the native 648-polygon scene. |
| Lost Woods Great Fairy conditional return | `34d6f1f3` merged in `56f32ec8`; moved-room/six-fountain/solo/Combo/native/shuffled/reload/explicit-arrival checks pass. Requires cor's separately authored collision exit/export. |
| Original solo OoT Fairy / Combo private Lost Woods portal regression | Both real-bundle solo routes and the complete composed paired-entrance runner pass. The old portal and new conditional Fairy route retain separate ownership. |
| 2Ship Mirror input, one-press confirmation, use and stump-relative return | PR49 merged through PR48, checkpoint `a7b5244e`; both native layouts, forms, both-host inventory/action/sync/save and native stump arrival pass. |
| Mirror shimmer and acquisition on stump top | PR48 `ec4bc59b`/`039a8f41`/`a1f6c40e`; PR50 pale-blue/white follow-up. Physical log/stump and native draw/pickup checks pass. |
| Saria authored shimmer | PR48 actual NEI twinkle; production draw bodies/renderers pass under sanitizers. |
| Woods of Mystery and private Lost Woods black-space shimmer | PR48 actual NEI field covers both black faces including centers; exact geometry/pulse/palette, face coverage and actual render bodies pass. |
| Narrow segment binding for otherwise unbound vertical scrolling | Newly implemented local `5eab94e4`, published source `35a0beca8ce33dbdea67d1b0f8f012d3de80d98a`, merged in `a3210d43`. Real-draw RED/GREEN, normal/Combo native objects and existing forest scrolling pass. Sheets explicitly opt into XLU `0A`. |

## Additional morning work retained

| Work omitted from the initial checklist | Existing source retained in the baseline |
| --- | --- |
| Seasons no longer activate OoT Song of Storms puzzle flags | `641451ca`, published through `0257e242`. |
| NPC shading/blue eyes/pink ocarina/shoulder/prop repairs and protected repair tooling | `ee83e261`, `26accab6` and `0257e242`; replacement model archives remain separate installs. |
| BossSoul detached-flame alpha repair source | `c04d0e1c`, published through `352c45a0`; POC5 inputs/recipes/shader evidence retained, replacement archives separate. |
| Native transformation camera and Goron transition light cleanup | `0efbc133` and cumulative parents. |
| Termina Moon-tree seasonal targeting | `61380244` and cumulative parents. |
| Mirror pale-blue/white palette | PR50 `e5cf1527`; Saria/entrance green remains independent. |
| Parallel CI and first-failure cancellation/publication guards | PR51/52, baseline `66bde021`; current workflow files remain byte-identical. |

## Preservation and recovery

Accepted input archives remain unchanged. The native canonical forest source uses its committed manifest and verified numbered parts; it must reconstruct independently without a leftover local ZIP. Original/solo/combined routing, inventory/progression/forms, native/shuffled entrances, explicit return payloads and all earlier source integrations are protected.

The candidate retains individual source histories and checkpoint records. Before publication, restore a fresh tracked-source archive, compare its tree/content and reconstruct the required native resource source. Do not substitute a successful compile or sanitizer fixture for configured game traversal or visual acceptance.

## Execution ledger

- Plan and baseline recorded before integration. Parallel audits own core bugfix verification, forest/scroll verification and morning gap/provenance review; root alone owns candidate integration and publication.
- Existing user authorization permits the isolated worktree, source push and cumulative publication without another permission round.
- Baseline native solo-forest fixtures passed for both hosts. Initial missing JSON/SDL include roots were environment setup only; supplying existing native dependency headers resolved them without source edits.
- Woodfall's published checkpoint `68aec1266580a1e56aaa401806b00ddadfc7c430` has exactly local `3352869b`'s tree. Merge `5797ff8b` retains that published history and all PR51/52 CI behavior. Its sanitizer run passed all five production fixture groups with the actual native MM archive (`f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59`, 459 vertices/648 polygons).
- Fairy checkpoint composed cleanly in player/scene code. The workflow conflict was its obsolete serial-gate insertion; retained the current workflow unchanged and placed the Fairy sanitizer command in `scripts/ci/regressions.json` instead. All 21 CI policy/runner tests passed.
- The complete composed entrance runner passed, including six production renderer objects, original portal/paired boot, all forms, save/Farore priority, stump return, camera and conditional Fairy handling. The dedicated Fairy sanitizer also passed all six native fountains in OoT-only/Combo modes, moved room, native/randomized ordinary visits and reload/stale-state cleanup.
- Both real-bundle solo routes passed against committed resource dependencies. Fairy/ Woodfall controllers and OoT Play passed real-header syntax checks; the player overlay passed after its normal mods include root was supplied (334 existing compiler warnings).
- Core read-only audit: 17/17 targeted commands passed. Includes Saria's nine arrow/button cases and full actor object, both native Mirror layouts, both-host inventory/action/sync/save, eight real-DSP audio cases/116 transformation controller scenarios/28 audio lifetime cases, five-form market startup and 18 bundled entry/return/re-entry cycles. Full native-base/keeper coverage is being checked separately.
- Forest read-only audit: 48 resource tests (46 pass, two optional original-input skips), actual particle draw bodies/renderers under sanitizers, both native Mirror adapters, four physical stump-access tests, six-track legacy/two-track native scroll fixtures, MM material registration and both native-bundle solo routes passed.
- Gap audit retained seasonal Song of Storms flags, NPC repair recipes/alias protection, BossSoul POC5 repair tooling/pinned inputs, native transformation camera/Goron light cleanup and prior Moon-tree seasonal targeting. NPC/flame/optional appearance output packs remain separate installations.
- Ruling: the requested extra vertical binding was only a proposal after the earlier UV workaround, not a missing committed checkout. The current explicit source-roundup request authorizes a bounded COMBO-only Lost Woods XLU segment `0x0A`, with two 32x16 layers moving +1 vertically and wrapping at 64. Sheets opt in through native target `0x0A000001`; no authored archive/texture/UV is changed. A separate source branch owns its real-draw regression and implementation.
- Stronger Black Market verification passed using the available native `mm.o2r`: 235 actual base resources, including 146 keeper resources, both hosts and six route/stock combinations across 18 additional round trips. The adapter reuses the existing production extraction/cycle/failure assertions and explicitly keeps historical-overlay full-input mode false. Application mounting and gameplay remain outside this fixture.
- Vertical binding verified in the composed tree. Its new test is wired into the complete entrance runner and the parallel render CI group. Source-only native resource reconstruction passed with no local ZIP cache (85 resources, 170 output files).
- All 13 changed production C/C++ files pass clang-format 14.0.6. Formatting corrections were confined to the new Fairy source and two changed transition/arrival expressions.
- Durable pre-vertical composition: GitHub `checkpoint/morning-entrances-20261021` at `d3a0fa3eb464c825e66f6dae807078823801eb86`, tree-identical to local `56f32ec8`. The dedicated vertical source branch records its own exact tree, authoring contract and regression recipe.
- Fresh final composed entrance runner at `38c9e666` passed, now including the dedicated vertical binding. Independent whole-branch review of `66bde021..38c9e666` found no actionable Critical/Important/Minor blocker. No unresolved source-test failure remains; two optional original donor-pair resource tests remain explicitly skipped.

## Final recovery and remaining proof

`docs/checkpoints/morning-roundup-source-20261021.zip` preserves the reviewed
implementation through `38c9e66648ec3704d4c21633421301052af5f27c`, a binary-safe
patch, Git bundle, exact baseline/tree/source hashes, named input identities,
restore instructions and focused verification/review evidence. The companion
JSON pins the archive hash. This recovery commit adds documentation and the
checkpoint only; no additional gameplay code is introduced after review.

The bundle was verified and restored into a fresh depth-three GitHub clone,
independent of the original workspace. The restored tree is exactly
`9e748ecac5cc10c255f08ec20066233db2210b3a`. Its tracked source parts reconstruct
the canonical forest and restore all 85 resources/12 authored textures into
both port trees without any local ZIP cache.

The normal hosted gate still needs to establish full platform build/link and
all regression-group results for the published cumulative head. Configured
gameplay/mod order, new Fairy collision export, vertical-sheet opt-in/continuity,
Woodfall round trip, both Saria interactions, Mirror use/pickup/return, all
authored particles and transformation listening remain runtime checks. Existing
separately authored assets and accepted masters have not been overwritten.

## Windows link correction

Published source `023488082d93257c7b3d063dd913b8caf389375b` failed Windows
run `39668376368` while linking `soh.dll`: `ComboLostWoodsFountain.obj`
requested the C++ symbol `?gComboReturnFileNum@@3HA`, while `OTRGlobals.cpp`
defines the return slot with C linkage. Linux and all six regression groups
passed that run. The correction gives the consumer declaration matching C
linkage; routing, saves, assets and the CI workflows remain unchanged.

The fountain fixture previously omitted the consumer declaration and supplied
its own C++ variable, hiding the ABI mismatch. It now compiles the actual
consumer declaration with the actual provider definition. Before the source
fix, GCC rejected their conflicting language linkage. After the fix, the
fountain scenarios pass under address/undefined-behavior sanitizers, and the
complete production unit compiles with real headers in Combo and solo modes
without compiler diagnostics. The complete `tests/lost_woods_entrance/run_tests.py`
suite also passes, including Woodfall, conditional/native Fairy returns,
paired arrivals, Mirror/stump routing, scrolling, camera and particle checks.
An independent review found no issues and repeated the sanitized fountain run
successfully. Clang-format 14.0.6 and `git diff --check` pass. Windows hosted
link results for this follow-up remain pending; runtime acceptance is unchanged.
