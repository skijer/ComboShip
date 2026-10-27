# Snowhead, regional actors and post-build materials — implementation candidate

The shared open Fountain valley, separate both-age ascent, local snow/chill, ice-blue opening shimmer, regional actor presentations and post-build surface replacements are implemented on `poc/snowhead-regional-actors-20261024`. Reviewed regional production snapshot: `a29fd34a` (final fix originated at `99ddc446`); reviewed native shop declaration correction: `a1cd772e` (origin `218463ec`). Develop/master remain unmerged and source is unpublished. Configured gameplay and user acceptance remain pending.

## Resulting behavior

- Vanilla removes only the eight verified low backdrop triangles. Vanilla and the supplied Djipi/export layouts share 43 fitted valley triangles, adding collision only where absent. Native exits, water, room/setup actors and camera metadata are retained.
- A separate southern/right-bank ridge starts 20 units below normal water, rises to the valley, and supports both ages using ordinary swimming/walking. Native adult ice actors and the original Ice Cavern route are unchanged. Intentional child Ice Cavern access is outside scope.
- Snowhead uses a grounded valley threshold, a separately grounded return outside retrigger bounds, the retained NEI opening art fitted to the natural opening, 72 local falling-snow billboards and 18 local drifting blue chill puffs. Local clocks pause/reset with actor ownership; no global weather change.
- Append-only actor catalog IDs 25–27 provide Stalchild's five poses, Mikau idle/guitar and translucent Darmani wait/fly. Defaults are OoT-side only: two seated Stalchildren at Ikana, Mikau with existing/fallback Lulu at Great Bay, and a visible Darmani ghost beside Snowhead. Authored placements suppress defaults. Ordinary native MM quest actors/parameters and existing presets remain intact; native MM also supports the nine reserved placed presentations.
- All six added surface materials resolve normal named `.o2r` resources before native fallbacks. See [the replacement-pack guide](../../../tools/regional_materials/README.md) for PNG names, stable paths, dimensions and the pack builder. No CVar, source change or recompilation is required to install a replacement pack. The recovery artifact includes an optional generated checker pack; it contains no original game artwork.

## Woodfall result

The inspected Woodfall return owner submits static opaque bark and an intentionally separate black backing. Native emitted commands are identical across six widely separated frame values and submit no XLU overlay. The supplied MM/archive evidence does not identify a magic or scrolling replacement layer. This pass adds a native characterization regression and preserves the intentional backing. It does **not** claim to reproduce or resolve the user's configured animated/opaque appearance.

## Verification

All commands below were run on the integrated source with the user-space native SDK environment activated. They establish source/native CPU/command behavior, not configured GPU or Player traversal acceptance.

| Check | Result |
|---|---|
| `tests/regional_passages/run_tests.py --sanitize` | PASS: native geometry, collision/lifecycle, both-host handoff, slot and boot-priority regressions |
| `tests/regional_passages/run_presentation_tests.py --syntax` | PASS: NEI preservation/fitting, local presentation and both hosts' full graphics translation units |
| `tests/regional_passages/run_material_tests.py` | PASS: six stable paths, deterministic packs, real TextureV1/SETTIMG metadata and ownership, ASan/UBSan |
| `tests/regional_actors/run_tests.py <original-mm.o2r>` | PASS: native params/callbacks, real spawn gate, placement/duplicates,232-resource closure, exact-byte loader and posed native geometry |
| Explicit regional-only `mm_ordinary_verify.py` native factory mode | PASS: all nine presentations and correct/wrong/absent guitar prop with actual resource factories |
| `tests/regional_passages/run_valley_tests.py` | PASS: complete supported bands, actual native resource/collision lifecycle, full render-object C linkage |
| Original OoT plus exported native/Alt geometry controls | PASS: complete ascent/body prisms and fitted valley support; continuous adult-ice clearance 229.129 after reserve; original Ice exit separation 2215.907 |
| `python3 -B -m unittest discover -s tests/ci -p 'test_*.py'` | PASS: 21 tests; new portable valley, actor and material commands registered |
| Original input identities | PASS: all three supplied archive SHA256 values unchanged |
| Final correction regressions | PASS: actual valley Build 129-vertex signed UV range/continuity/phase, full render-object ABI and Woodfall native command origin |
| Recovered MM shop native build correction | PASS: actual configured full TU RED/GREEN, C linkage, native adapter, scene factories/5 tests and shop/camera/receipt/teardown fixtures; two declaration/include lines only |
| Full configured native Linux `combo` build | PASS at source `9e2d6484`: SoH, MM, shared UI and launcher linked; corrected interpolation/shop symbols have no unresolved imports |

The normal full native resource helper retains a deterministic pre-existing failure in `TestAnjuNativeMetadata`'s synthetic metadata-poison assertion. The coordinator independently reproduced it on unchanged baseline `8a92bcb0`. The assertion is still enabled; the explicit regional-only mode proves the new actors separately. Do not describe the full resource suite as green. LeakSanitizer cannot enumerate processes in this environment; ASan/UBSan fixtures use their existing `detect_leaks=0` convention.

## Recovery and preserved overnight work

Restored checkpoint `9347ae570ee1921b55f1eb1952f901afb38370be` has exact original tree `fce96d959ecbeaf37b0e49b05a90684549084e9f` and remains an ancestor. All 128 recovered files remain: 114 byte-identical and 14 intentionally edited in the regional geometry/presentation/tests/CI scope or the two-line native shop build correction. Shop behavior is retained; its consumer gains only the missing native include/prototype. Title, spin and unrelated overnight fixes remain byte-identical. Source checkpoints include exact branch refs/trees, binary patches, reports, the original archive identities and independent restoration instructions. A fresh repository seeded only with public baseline `196ac9b7` restores every saved snapshot and matches its complete tree identity.

All task reviews and their bounded corrections are approved. The final whole-branch review found a signed16 valley UV overflow and two minor evidence/documentation gaps. One consolidated correction uses a common safe UV origin, qualifies graph recognition to the supported complete layouts and asserts both Woodfall tile-command words. Its scoped re-review approved all three corrections with no new breakage. The source review gate is closed; full application build and configured acceptance remain separate. The durable artifact preserves the unmerged candidate separately from accepted masters and from all original game archives.

The subsequent full native build linked the OoT module, then exposed two undeclared calls in the unchanged recovered MM shop consumer. A separately scoped two-line include/prototype correction reproduced both compiler failures before editing and compiled the complete configured TU afterward; existing native shop behavior/scene/camera suites passed. Independent review approved the exact native C signatures/linkage. This newly observed build-integration correction changes no shop state or behavior and is separate from the closed regional review wave. The resumed full Linux `cmake --build ... --target combo --parallel 4` finished with exit0 at source `9e2d6484d49c8171667efcba83026d244916d55c`; all four application targets linked, and final module symbol inspection found no unresolved corrected hooks. Later source edits record documentation/evidence only. No configured game session or GPU acceptance is inferred, and the recovery download contains source rather than a playable binary package.

## Remaining acceptance

Run the candidate with the intended vanilla and Djipi configurations: ordinary child swim-to-ground handoff, full child/adult ascent, adult moving icebergs, camera, Snowhead entry/return without immediate retrigger, scene/Alt reloads, actor appearance/prop/ghost translucency, local weather density and an enabled replacement pack. Test user-specific Woodfall active resources/load order to identify the reported animated surface. These observations remain unproved by the static/native fixtures.

## Rulings made during this pass

These are the ledger's explicit rulings, in order, including the cost if the interpretation is wrong:

1. Execute the already-discussed scope without another design-approval pause, keeping changes isolated and reviewable. A mistaken scope interpretation would require a candidate revision; no accepted master or original archive was replaced.
2. Honor the user's requested parallel workflow despite the workflow skill's serial default. The cost is integration risk between interfaces; separate ownership, bounded commits and independent combined review address it.
3. Keep four implementation seats with no worker-spawned agents; controller owns reviews. The cost is potentially slower dependency work, in exchange for bounded ownership and recoverability.
4. Use manually written briefs, diff packages and ledger because the skill's helper scripts were unavailable. The cost is possible recordkeeping omissions; exact git ranges, full diffs and independently restored checkpoints provide the audit trail.
5. Treat intentional child Ice Cavern access as outside scope, following the user's correction; build the separate right-side ascent. If that preference changes, native Ice Cavern access needs a separate deliberate change.
6. Initially retain the nonblocking Woodfall tile-origin assertion nit because complete packet comparison already proves frame invariance. Its temporary cost was incomplete fixed-origin coverage. The consolidated final correction now checks both command words, superseding that deferral.
