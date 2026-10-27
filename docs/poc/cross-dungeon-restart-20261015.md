# Cross-game dungeon entrances: restart checkpoint

## Authorization and baseline

cor approved the full eligible cross-game dungeon pool POC, default-off opt-in checkbox,
and isolated implementation on October 15. After the original task froze, cor explicitly
requested a restart with periodic source checkpoints. No develop/master promotion is authorized.

- Baseline: `a82852a3a02c8cabc541cce7c124da71c5755efb` (accepted develop).
- Candidate: `poc/cross-dungeon-restart-20261015`.
- Original task: `poc/cross-dungeon-entrances-20261015`; its reported implementation is
  unavailable in this session. This restart does not claim those files were deleted.

## Required behavior

An eligible OoT dungeon entrance may lead to an MM dungeon and vice versa. Compose the
cross-game routing with the existing OoT entrance shuffle, including mixed grotto/boss
pools; do not replace or bypass those native settings. Return routes, death/save recovery,
boss warps and pending item delivery must agree with the generated graph. Native game
age, MM time and each game's progression remain owned by that game. A foreign dungeon
arrival grants access to that dungeon, not the destination game's whole overworld.

Disabled must preserve the old generation, save/load and runtime path. Store explicit
game-tagged routes with the seed and reject malformed/stale enabled data. Keep unrelated
weather, assets, rendering, audio and the separate Lost Woods POC outside this branch.

## Checkpoint policy

Commit and persist source at implementation boundaries and approximately every ten minutes
of longer work. Each checkpoint records exact completed code, commands/results and next
steps. A progress message alone is not a source checkpoint. Incomplete candidates remain
explicitly incomplete. Build verification and installed-game acceptance are separate.

## Checkpoint 0 — baseline established

Fresh clone verified at the exact baseline above; isolated candidate branch created.
No feature implementation or build is claimed at this checkpoint. Native source exploration
identified the existing launcher fill/oracle interface, OoT entrance shuffler and both
game-switch/title-setup adapters. Next: route schema/composition tests and implementation.

Environment: C++ compiler available; CMake and development dependencies need setup.

## Checkpoint 2 — seed and native integration candidate

Source now includes strict seed codec, per-host oracle state, joint reachability fixed
point, native graph adapters, transactional coordinator, launcher/headless wiring and
checkbox. Boss-completion escape routes compose with shuffled returns. Runtime helper
exists; native transition hookup is still in progress. Six focused executables pass:
coordinator, joint reachability, routing, runtime helper, seed codec and host state.
Native CMake configuration succeeds using task-local dependencies. Full native build
has started; no completed-build or installed-game claim. CMake's existing automatic
libultraship probe patch is a build-side effect and is excluded from feature commits.

Ruling: malformed enabled data attached through the existing void save-load callback
clears runtime routing and logs an explicit error rather than retaining another slot's
map or terminating the app. That callback cannot veto gameplay; hand-edited invalid
bound saves remain a limitation. New-seed and explicit reload preflight reject them.

Execution uses separate runtime and launcher implementers with frozen C ABI and disjoint
file ownership; parent owns native graph, integration/build verification and checkpoints.

## Checkpoint 3 — native boundaries and review findings

Runtime transitions, mixed-grotto hooks, both title arrivals and optional native save
checkpoint fields are implemented. Six focused executables pass normally and under
ASan/UBSan (detect_leaks=0). Exact generated native syntax checks passed both hosts,
OoT entrance logic, MM region logic, both title setups, OoT entrance/grotto dispatch,
MM transition setup and both save managers. Existing shared/plando C++17 regression
suite passes after exercising the actual new coordinator binding in its native seam.
Full Linux native build is ongoing; a completed binary/package is not yet available.

Independent review found six Important issues. Parent corrected recorded-native-map
precedence for replay and canonical blue-warp alias normalization, adding failing-then-
passing behavioral checks. Four runtime/launcher issues are being corrected before
completion: native-shuffle-off boss warps/direct Ganon continuation; pending animated
OoT boss reward persistence; foreign MM destination recovery after save/restart; and
native-state rollback after stale enabled seed rejection. These remain verification
work at this source checkpoint. No merge, runtime acceptance or final build claim.

Next: complete those four corrections, rerun affected real-header probes/core and
shared suites, finish native build, run real-engine generation/replay and restricted
arrival checks, regenerate port archives and package an isolated candidate.

## Checkpoint 4 — review corrections and pure replay preflight

All six Important review findings are corrected in source. Recorded-native returns and
blue-warp aliases share a pure catalogue builder. Saved override indices, types,
destinations, capacity and duplicates are validated before the native world changes.
A data-only native entrance table now supports that preflight without touching the
active settings, graph, RNG, goals or prices. Native-shuffle-off cross generation also
initializes graph edge metadata explicitly, without replacing connected regions.

The real launcher reload callback regression fails against checkpoint 3 and passes
against this candidate: explicit and silent stale seed rejection preserve the prior
native/pending state. Runtime fixes preserve pending check names until delivery,
restore after native queue clearing, retain destination dungeon checkpoints, route
blue warps with native shuffle off and compose optional Ganon continuation.

Eight focused executables pass normally; seven preceding tests passed ASan/UBSan
before the pure catalogue addition. Exact native syntax probes pass all changed
runtime units and both final catalogue/entrance units. Existing shared/plando suite
passes. Full native compilation continues; no completed-build or gameplay claim.
Next: sanitizer rerun including catalogue, real-engine generation/replay and restricted
arrival probes after linking, isolated build packaging.

## Checkpoint 5 — native graph lifecycle and verification preparation

Eight focused executables pass ASan/UBSan (detect_leaks=0); the existing shared,
launcher delivery/slot and plando checks pass. The production reload regression
passes. Existing asset collision gate passes with no new collisions. A baseline
headless generator from a82852a3 is compiled for the upcoming checkbox-off control.

OoT preflight endpoint validation now reads only immutable native metadata; search
refreshes its graph index after replay. Edge tags are initialized only for an untyped
vanilla graph, since native SetAllEntrancesData assumes original connected regions.
MM one-way Goht completion is accepted as a source despite being absent from native
reverse-arrival mapping, in both oracle and runtime validation. Native syntax checks
for these final lifecycle adjustments are recorded separately; full build ongoing.

The new real-module probe is prepared but NOT yet run. It will install every endpoint
in both oracle/runtime hosts, compare pure/replayed native catalogues, and check
restricted OoT/MM arrivals versus overworld roots. Generation matrix, disabled
baseline comparison, port-archive generation and package remain after linking.

## Checkpoint 6 — source frozen for linking

The extra pre-handoff save barrier and pending-reward capture are gated on the
feature being enabled, preserving the original checkbox-off portal save sequence.
Exact OTRGlobals native syntax passes after that guard. Both final native graph
adapter syntax checks pass, all eight focused sanitizer tests and existing shared/
plando/slot delivery suites pass, and no new asset collisions are present.

Feature source is frozen pending actual linked-engine tests. Linux Release native
build is still compiling; do not label this checkpoint a completed build. The
checkpoint zip now includes explicit restore instructions and full cumulative source.

## Checkpoint 7 — recovery verified and integer-boundary correction

Checkpoint 6 successfully restored exact candidate 995c96f3 into a separate
baseline-only Git repository. Restored source bytes match the implementation.
A failing-then-passing numeric boundary test now rejects UINT64_MAX in a native
override destination before signed conversion can turn it into the -1 sentinel.
The catalogue test passes normally and with ASan/UBSan after this correction.
Native build remains in progress, with no compile failure yet. No game/runtime
acceptance or completed native build is claimed.

## Checkpoint 8 — full initial Linux link complete

The Linux Release build completed with exit 0 for libultraship, soh, 2ship, comboui,
ComboShip and comborando. Actual SoH module smoke checks pass: 11-slot catalogue,
pure seed preflight, swapped recorded native layout, repeated catalogue queries
and enabled reachability search. Final incremental scan/stamp remains required
because the integer boundary fix followed initial OTRGlobals compilation.

The two-module native probe now covers first replay before lazy graph initialization
and overflow rejection as well as all catalogue endpoints, restricted roots and
recorded native layout replay. It is queued for the final linked candidate. Native
generation/replay matrix and checkbox-off baseline comparison remain unrun here.

## Checkpoint 9 — native generation exposed age-rule mismatch

Final incremental Linux compilation and both generated port archives completed.
The linked two-module probe passed catalogue validation, all oracle/runtime endpoints,
restricted dungeon roots, first-load preflight and native-layout replay.

The first cross-enabled/native-dungeon-shuffle-off generation failed all-locations
validation. The original baseline generator passes the same settings. A new actual
engine regression reproduces 56 lost OoT checks when adult temples are placed behind
child-only vanilla mouths. Oracle evaluation now scopes the native shuffled-dungeon
age/door rules to the active cross map, restoring the original setting afterward.
The regression is confirmed failing against checkpoint 8 binaries; the correction
passes native syntax but has not yet been rebuilt and verified. Runtime safety-read
parity and slot-load ordering are being corrected separately. No gameplay claim.

Next: freeze runtime correction, rebuild, prove the native age regression and original
failed seed pass, complete generation/replay/disabled-baseline matrix, package candidate.

## Checkpoint 10 — native age safety verified; layout retry remains

The corrected Linux modules link successfully. The actual-module age regression now
passes with Forest Temple behind Deku's mouth and Water Temple behind the well;
all previously reachable checks remain reachable. Stored native settings stay exact.
Both hosts still pass restricted roots, every native endpoint, catalogue and replay
probes. All eight focused executables pass; runtime projection passes ASan/UBSan and
the four changed native translation units pass exact syntax checks.

Runtime entrance safety uses effective reads only while the playing slot's map is
bound. Slot-load dispatch suppresses the previous slot's map and reapplies required
bluewarp/savewarp safety after binding. Closed forest, Mido and drained-well gates
and each game's actual age/time are preserved.

The original failing random map still places adult-only Fire Temple behind Jabu's
physically child-only mouth. Native ER itself retains that restriction. The current
cross permutation needs deterministic structural validation/retry before fill,
analogous to native shuffler validation; do not force an age change or weaken that
mouth. This remains active implementation work, with no complete seed/build claim.

## Checkpoint 11 — deterministic structural layout validation

New generation validates each cross map using both real oracle interfaces and every
pool/fixed item, preserving progressive multiplicity. It compares with the checks
actually reachable in the native baseline, then retries only the independent cross
layout stream (bounded at 128). Saved headless generation/playthrough maps are tested
once and rejected if structurally invalid; they are never silently replaced.

The focused behavior test passes impossible-first-map retry, determinism, inventory
multiplicity, native baseline exclusions, exhaustion/rollback, exact saved-map
rejection, runtime-map isolation and disabled zero-call behavior. GUI reload retains
its pure preflight: adding post-mutation rejection would violate its rollback
guarantee. Hand-edited maps that pass schema/catalogue validation still need explicit
headless structural validation; generated maps pass it before fill.

Native age regression, source syntax and focused tests passed before this addition.
Full nine-test suite, launcher syntax and final incremental build/matrix are next.
Changed production source also receives the repository's clang-format-14 formatting.

## Checkpoint 12 — full native build; fixed-item deadlock diagnosed

All 151 final Linux Release compile/link steps completed successfully. Nine focused
tests and launcher/headless syntax pass, including clearing temporary structural
arrival roots before item placement. A targeted independent review found no blockers
in the deterministic retry implementation itself.

Real generation still fails all-locations validation despite that initial probe.
Native diagnostic evidence identifies why: unconditional ownership of fixed items
lets a map pass even when its own songs, ocarinas and confined keys are inaccessible.
The original baseline's free-pool-plus-reachable-fixed closure reaches 360 OoT
checks; the accepted cross map stalls at 112. The actual failed fill likewise has
113 reachable OoT checks and 214 unreachable fixed placements, with no ocarina.

The probe is being strengthened to match that fixed-item collection process in both
native baseline and candidate searches. Diagnostics were compiled from a separate
source copy; no diagnostic instrumentation was inserted into this feature branch.
Generation/replay matrix and final package remain incomplete at this checkpoint.

## Bridge recovery note — surviving source and linked candidate recovered

Recovered checkpoint 11 and the surviving isolated checkout in a new session. Source
at `7fbfbaf0` additionally clears the structural probe's all-owned arrival roots before
item fill; its behavioral regression is included. No implementation was restarted.
The previous full-generation failure log predates verification of this cleanup.

The resumed Linux build reached all 151 steps and linked both hosts, the launcher,
UI and headless generator. The nine focused cross-dungeon executables were rerun in
this session and all passed. Native-module age/replay/restricted-root checks and the
complete generation/replay/native-shuffle/disabled-control matrix are now running.
Successful full seed generation and installed-game runtime acceptance are not yet
claimed. Baseline develop/master remains unchanged.

The time travel investigation concerns native progression and entrance age safety:
preserve actual Link age and MM time; use the existing native shuffled-mouth safety
rules during oracle/runtime evaluation; reject physically impossible layouts rather
than granting an age change or weakening Jabu's child-only mouth.

Recovery source: `/workspace/scratch/90fba16bed69/ComboShip-CrossDungeon-Restart`.
Current session evidence: `/workspace/scratch/cc27f418693a/`.
Next: inspect the linked native matrix; diagnose any full-fill failure from its actual
missing checks/time travel propagation; save another checkpoint before longer work.

### Bridge coordination

Superseded: the user confirmed that the original chat is frozen. Files and tool
output appearing in its checkout do not establish a usable ongoing conversation.
This chat now owns implementation, verification, packaging and checkpoints in the
separate `poc/cross-dungeon-bridge-20261015` worktree at
`/workspace/scratch/cc27f418693a/ComboShip-CrossDungeon-Bridge`.
The old checkout is retained as recovery/build evidence only. The captured
uncommitted patch and test files were imported into this isolated candidate;
later changes to the old checkout cannot silently change this candidate.

Bridge review: two Important MM boss-warp corrections are recorded in
`docs/poc/cross-dungeon-bridge-runtime-review-20261015.md`. The owned takeover
imports and verifies their fixes; the takeover checkpoint below records evidence.

## Checkpoint 13 — first complete native cross seed passes

The fixed-item closure correction passes all nine focused tests, C++17 checks,
ASan/UBSan (leak detection disabled for sandbox ptrace limits), and targeted review.
The actual rebuilt generator completes the previously failing seed with exit 0:
`cross-dungeon-native-off`, OoT start, no shared items, cross routing enabled.
It selects a valid map after seven isolated layout attempts, reaches all 236 free
advancement items, and reports zero unreachable OoT junk checks (three known
under-modelled MM checks remain tolerated by the unchanged native baseline policy).
Full hint analysis also completes; total native generation time was 233.79 seconds.

The final two-module probe passes again against the rebuilt modules, including age,
restricted-root, catalogue, numeric-boundary and recorded-native replay coverage.
The native matrix runner is now committed for reproducible generation/replay and
disabled-baseline checks. Its remaining cases are running.

Cross pare-down currently bypasses the existing native reachability memo, taking
210.7 seconds for 422 repeated counterfactual replays. A per-invocation cache of
both inventories' joint results is being added and tested before final packaging;
the disabled memo path stays unchanged. No installed-game acceptance is claimed.

## Owned takeover checkpoint 1 — source and runtime fixes verified here

The exact parent is `92d01a0156e8e0f21d2c9875fe1e4fd1e99caf9a`.
Recovered uncommitted fixes include the per-invocation joint requiredness cache,
MM final Giant escape composition and clearing source-specific cutscene staging
on actual remaps. The imported patch SHA-256 is
`4d34c7a3d3cad29fb3120aaeebfe88903f2200c9ae04d8e7e713360df9147b96`.

The production-function boss-warp regression was run against the unchanged parent
and failed because first-boss Giant bookkeeping/escape composition was absent.
The recovered correction then passed all four bosses' first/repeat warps, local
and foreign returns, native bookkeeping, identity routes and disabled controls.
The same behavioral probe passed ASan/UBSan with leak detection disabled.
All ten focused C++ executables plus the native behavior probe pass in this worktree.
MM reward/Oath eligibility is included in native JSON save serialization, preserving
the queued eligibility through a persisted handoff.

An independent native candidate is being built here by recompiling every changed
translation unit and relinking against the recovered completed build's unchanged
objects. The exact commands, reused library hashes and outputs are recorded in
`/workspace/scratch/cc27f418693a/takeover/owned-native-build.json` when complete.
No final native build, full matrix or installed-game acceptance is claimed here.
Next: complete native linking, linked module probes, the full generation/replay/
mixed/disabled-control matrix and isolated packaging. This chat owns those steps.

## Owned takeover checkpoint 2 — native verification and Linux candidate

The isolated native candidate is linked and packaged. All four native translation
units changed since the recovered complete build were compiled from this owned
source; unchanged native objects/libraries were reused. Exact commands and hashes
are preserved in the checkpoint's `evidence/owned-native-build.json`. This is an
incremental native build, not a second clean build of the entire repository.

All ten focused C++ executables and both production-function probes pass. The MM
boss-warp probe also passes ASan/UBSan. Shared-item/slot/plando checks and production
seed-reload/rollback checks pass. The linked native-module probe passes for both
the owned deployment and packaged AppDir with its bundled dependencies, including
native entrance replay, restricted foreign arrival roots, both-age native-off
rules and preserved native settings. An independent whole-branch review through
`da82ae67` reported no actionable Important/Critical findings in its bounded scope.

The forward affordability canary initially reported two false failures despite
beatable walks. The native oracle was correct: its built-in NEI wallet pair shares
OOT/MM wallet ownership even with the optional shared-items mask disabled. The
canary counted only that optional mask. It now uses the same resolved shared-item
groups as traversal, preserves foreign item ownership, and excludes pickups from
the current sphere. The source-extracted probe fails before the correction and
passes afterward, including unshared and same-sphere negative controls. The
headless executable was recompiled/relinked and all five forward checks rerun.

| Native case | Pool | Generation | Corrected forward traversal |
| --- | --- | --- | --- |
| Native entrance shuffles off, OOT start | 16 | PASS | PASS |
| Native entrance shuffles off, MM start | 16 | PASS | PASS |
| Native dungeon shuffle including Ganon | 17 | PASS | PASS |
| Coupled mixed dungeon/boss/grotto, Ganon excluded | 16 | PASS | PASS |
| Decoupled mixed dungeon/boss/grotto including Ganon | 17 | PASS | PASS |

Same-seed regeneration and recorded-map/native-layout replay are exactly equal to
the first enabled seed. The disabled candidate seed is exactly equal to the
original baseline generator's output and contains no cross-dungeon field. That
comparison intentionally loads the same candidate native modules in both
executables; it proves original-generator parity, not an independently rebuilt
baseline native runtime.

The original coupled mixed-plus-Ganon test seed remains rejected. The original
generator with cross routing disabled also rejects the same settings/seed under
the unchanged All Locations Reachable policy (105 OOT checks unreachable on its
second attempt). The positive coupled case above has an original-generator control
that passes as well. The first matrix's failed logs/result are retained as history;
the consolidated result separately records corrected canary runs and this inherited
native-layout rejection. No access rule was relaxed to force a green result.
The committed native matrix runner now uses these five verified profiles and
includes MM-start forward traversal. Its coupled fixture retains the exact seed
string of the passing control; coupled-plus-Ganon remains a documented limitation.

A self-contained Linux x86_64 AppImage was produced from the owned runtime. Its
native modules pass the same linked-module probe. The subsequent correction changes
only the headless validator, which the AppImage does not include. No user ROMs,
player saves, Windows build or installed-game acceptance are included. Actual
age/time, death/save/reset, boss/reward and mixed-route gameplay remain explicit
runtime acceptance work. The off-by-default POC remains isolated on
`poc/cross-dungeon-bridge-20261015`; develop/master is not promoted.
