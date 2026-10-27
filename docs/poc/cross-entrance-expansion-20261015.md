# Cross-entrance expansion ledger

Plan: `docs/superpowers/plans/2026-10-15-cross-entrance-expansion.md`.
Spec: `docs/superpowers/specs/2026-10-15-cross-entrance-expansion.md`.

## Checkpoint 0 — current candidate restored

Imported exact recovered routing `f6c4eecc` into isolated PR41 source `913bc329`; six merge conflicts were resolved with both branches' behavior retained. In particular, current generation cancellation/finalization, save RAII and Farore handoffs remain intact. Shared cross routing now coexists with the current 9.3 source.

Fresh baseline verification: all ten C++ cross-dungeon groups, production MM first/repeat warp tests and affordability tests pass. Existing Farore policy/JSON/container/native OoT/MM bridge groups pass with the task's nlohmann include path. Native MM header compilation emits existing controller macro redefinition warnings. No installed gameplay proof is claimed.

Independent source audits found separate boss-completion ownership requirements, MM grotto identity/materialization requirements and the Farore checkpoint mismatch. Worker ownership is disjoint: recovery/Farore/Stone Tower; new MM catalogue/grotto files; new boss adapters and native boss consumers. Main integration owns shared routing, launcher, UI and existing native adapter headers.

Source baseline is the current unaccepted PR41 candidate, preserving its documented unresolved Spring nighttime lighting and Zora eye/tattoo appearance. This task does not change those systems.

## Checkpoint 1 — typed routing core

Added independent Dungeon/Boss/Interior/Grotto permutations, opt-in v2 catalogue metadata and exact v1 compatibility. Boss completion routes compose through the owning dungeon overlay once, while reverse doors retain separate recovery targets. Added the physical-to-source boss map and joint search closure; destination MM safety is evaluated after both hosts have the current inventory. Gameplay route state and temporary search state remain separate.

Focused typed-pool tests pass across generated/replayed permutations, a 140-record catalogue, category rejection and overlapping dungeon/boss escapes. Joint-boss fixture passes physical-to-source completion, fresh-query isolation and current-inventory arrival gating. The cross suite passes against the working integration; native host and launcher wiring is still being completed and is not claimed built at this checkpoint.

## Checkpoint 2 — approved expansion integrated

Implemented the three recovery hardening areas plus independent opt-in boss, expanded dungeon, interior and grotto pools. MM adds seven expanded dungeon entrances, 42 eligible interiors and 26 logical grottos. The Clock Tower interior and Happy Mask Shop stay outside eligible pools to preserve the game portal; recorded native returns to the Mask Shop remain valid. Native OoT catalogue types supply its eligible interiors/grottos and eight boss arenas; MM supplies four bosses. New UI controls and five headless flags save selection with each v2 layout. Legacy dungeon-only v1 records and their RNG are retained.

Farore points capture optional owned recovery metadata; legacy adoption clears that metadata rather than attaching the current dungeon to an old point. Stone Tower exterior state is normalized before room initialization. Foreign MM oracle arrivals require a playable Song of Time and actual owned clocks when clock shuffle is enabled; runtime day/time is preserved. Both grotto adapters materialize logical identities exactly once, including source saves, returns and recovery.

Boss rewards remain physical. The joint oracle maps actual physical defeat to source completion, then recomputes reachability. Runtime source completion is stored per slot in `combo.crossBossClears`; actual first/repeat warps apply the source world's effects. Dormant ownership is explicit because a parked game can retain a non-null PlayState. MM cycle effects are not replayed merely by loading the permanent mask. Source progress never grants another physical boss's reward flags.

Independent review produced and closed regressions for: logical masks leaking into direct searches; foreign exit predicates using gameplay masks; OoT native ConditionsMet rejecting redirected foreign regions; native resolved warps entering another selected forward pool; malformed/narrowing boss identities; native boss reverse checkpoints (Gohma/Fire and unpooled dungeon targets); missing MM defeat gates; and Woodfall ordinary/prison exits being mistaken for boss completion. All have production-function regressions.

Verification: full normal cross suite exits0 after all fixes; native syntax matrix covers45 selected compilation entries (31 initial passes plus all15 retry entries after local dependency/include setup, with one repeated source compiled for two targets). Farore and existing shared-item/plando/slot suites pass. Focused new adapters pass ASan/UBSan; the full sanitizer suite completed its final affordability group without findings. LeakSanitizer is disabled because the container cannot enumerate `/proc` tasks. Full linked native build and native generation/replay matrix remain in progress; installed-game acceptance remains unclaimed.

## Late native boundary checks

Two additional production-path regressions are fixed after Checkpoint2: native-only OoT baseline searches now suppress the playing slot's completion aliases even when the candidate oracle map is empty, restoring prior search state on exit; and cross Boss routing enables SoH's existing Barinade/Morpha reverse-room/spawn repairs without changing native shuffle settings or RNG. The real scope/condition fixture and both actual spawn function bodies pass normally and under ASan/UBSan. Independent read-only review confirmed the empty-map isolation and nested/exception state behavior. Final normal cross suite exits0 in104.62s; existing shared tests and Farore native/JSON/container sanitizer groups pass. The linked native build and expanded native CLI matrix are still in progress.

The new `scripts/diagnostics/run_cross_entrance_expansion_native.py` records per-profile logs and exact native output, compares disabled output with a baseline launcher, covers every selected pool and both starts, checks deterministic/exact replay, and traverses native mixed coupled/decoupled profiles. It does not claim installed-game acceptance.

## Linked native boundary checks

The headless generator, both game modules, shared UI and launcher link successfully. Linked generation exposed a missing first-Gyorg-warp source alias (Zora Cape spawn8); the native region index now includes it, with a fixture built from the actual MM paired exits and one-way entrance declarations. All four MM boss records install against that native index. The new defeat requirement on three MM completion edges is limited to boss-shuffle oracle maps; native and legacy dungeon-only conditions remain unchanged.

Native interior generation also exposed synthetic-start time metadata on foreign physical roots. Arrival roots now inherit the native region's waiting capability and merge existing time states. Actual native initialization/expansion regressions cover ordinary waiting, closing restrictions, frozen regions, preserved earlier reach and owned clock bounds. Normal and ASan/UBSan checks pass.

Boss recovery previously pinned an inaccessible arena after death/reload. Runtime now derives the inverse boss ingress, returns same-host deaths to that source's safe parent and stages a validated foreign-source resume through the existing clean-frame handoff. Explicit automatic MM game-over and Continue callers distinguish death from unrelated negative warp/void respawns. Production tests cover all four MM arenas, both host directions, cold load/title handling, native mixed source returns, Farore precedence, malformed metadata, v1 parity and completion-route separation. Full cross suite, focused recovery sanitizer checks, Farore and six native syntax units pass.

Default-hint native generation has passed for boss-only, seven expanded dungeons, grotto-only and interior-only profiles. A recovered original v1 control reproduces its entrance map exactly. Combined all-pool structural generation is still being diagnosed; no acceptance or release claim is made. Native final relink and exact replay matrix remain pending.

## Combined-pool generation review

A linked identity-map probe with all pools enabled matches native physical boss masks (OoT0xff/MM0xf00) and passes generation. The combined randomized MM-start seed passes native generation and a no-trick playthrough. Independent review then confirmed OoT foreign exit predicates were missing the source `CurrentRegionKey`: native `AnyAgeTime` asserted in the production fixture. The adapter now supplies the actual parent and restores prior context on all returns/exceptions. Its native/sanitized fixture passes.

The remaining sparse-layout case motivated a bounded expanded-v2 fallback after the existing whole-map budget. It validates identity, then only accepts pool-isolated cycles/swaps that preserve the full structural oracle footprint; it rejects an identity-only repair and reinstalls/revalidates the final map. V1 generation and recorded replay do not take this branch. Strict-warning and ASan/UBSan tests pass. Independent fake-host integration verified deterministic output, final installation in both hosts, empty roots, no runtime setters and rollback after an injected oracle exception. The real default OoT-start all-pool case now passes structural validation through this fallback; item-fill and mixed-native proofs are running.

## Recovery integration — separate death ownership

The resumed recovery branch includes the continuing original session's cold-reload recovery, native source-condition context, and bounded v2 generation fallback. Independent review found a decoupled native edge that the original recovery fixture missed: Fire boss ingress can lead to Gohma while Gohma's reverse independently leads to Water. Death ownership is Fire; reverse travel is Water.

Boss records now carry `deathCheckpoint` separately from reverse/warp checkpoints. The OoT producer derives it from the incoming native override using immutable source/reverse pairs, matching native `Entrance_Init` ownership. A recognized boss-arena recovery is narrowly normalized to that arena's canonical temple entrance so repeated load/death cannot reinterpret it as another cross ingress. Other native endpoints, including synthetic grottos, retain their identity. Foreign recovery reloads source-host state and applies this same parent as the actual title spawn. Reverse-door/completion routing remains independent. V1 catalogue/RNG stays unchanged; stale provisional boss metadata fails exact preflight rather than being silently rerolled.

New production regressions failed on decoupled ownership and actual foreign title spawn before the corrections, then passed with ASan/UBSan. The full cross-entrance suite and Farore suite pass. A scoped independent review found no remaining Critical/Important defects in the recovery, native condition-context, or constrained-generation changes. The additional Coordinator/oracle fixture proves a real cross-game constrained shuffle, deterministic replay, empty temporary roots, runtime isolation and fail-closed rollback.

The first final-stage relink missed native objects inside linker response files; its matrix was superseded, not counted as final evidence. The corrected relink expands those files, asserts every required object replacement, and links both engines, launcher, headless generator and UI. A linked-module public-API probe confirms the distinct decoupled death/reverse catalogue, both runtime setters, both recovery request directions and transactional invalid-metadata rejection. Native internal title/death consumption is covered by production-body fixtures; installed gameplay remains separate. Corrected final-stage generation/replay is in progress at `../verification/final-native-matrix02`.

## Final recovery candidate

Recovery implementation affcc193 completes the additional incoming-source death metadata and actual foreign spawn correction. Required native acceptance now passes all individual pools, all180 entries from both starts, exact whole-JSON regeneration/replay, coupled and explicit open-time decoupled presets, with no-trick playthroughs. Header-complete final03 compiles12 affected units, links both engines/launcher/UI/headless, passes the linked recovery API and repeats exact whole-JSON replay plus four combined playthroughs. Launcher reload and shared-item/slot/plando checks pass with project C++20 diagnostics. Final source/artifact hashes are verified. The closed-door/Ganon decoupled stress seed returns1 after two failed fills and emits no accepted seed; this control is kept separate from the passing preset. Detailed evidence and remaining Windows/installed-game acceptance are in the verification report and final checkpoint. No promotion or push.
