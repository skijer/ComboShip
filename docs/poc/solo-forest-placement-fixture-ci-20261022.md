# Solo forest placement fixture CI repair — 2026-10-22

Baseline: PR #57, `poc/market-forest-followup-20261022`, commit
`03df0bf69f23f7ab07fbc1b568c9aa5745c83f3f`, tree
`f4bf44e65381a35a1413e015ee1985bd2ae7f261`.

Run `39825311276` passed Fast source checks, including the repaired dependency
setup. In the world group, the Granny header, October 20 regression batch,
native NPC models and forest actor checks passed. The next solo-forest runtime
fixture asserted Saria's obsolete world position `(-672.152, -54.586)`, derived
from the old -230 lateral offset. Production already uses the nearer -120/+120
flanks documented in `native-forest-materials-followup-20261022.md` and checked
against the retained native collision by `tests/native_forest/test_placements.py`.

The fixture now checks the documented Saria position `(-630.056, -156.212)`,
exact opposite local offsets -120/+120, unchanged forward depth 120, and
clearance from the authored radius-68 rim and stump trigger. The obsolete
distance from the old stock-tree center is replaced with portal-rim clearance.
Actor parameters, floor placement, authored override, readiness/retry, dialogue,
duplicate prevention and solo/combined mode assertions remain active.

No actor implementation, geometry, collision or placement constant changed.

Validation:

- Reproduced the original assertion before editing.
- Updated native solo-forest diagnostic passed with its existing UBSan checks.
- The real MM companion-spawn/native floor-and-wall footprint test passed under
  ASan/UBSan, independently confirming the shared placement against retained
  collision data.
- Ran the original world manifest commands from the failure point through the
  end: all 25 checks passed. This covers the remaining forest, Mirror, native
  shop/door/shelf/browsing, hex and scene-music checks. The first four unchanged
  world commands passed in the inspected hosted run, giving evidence for all 29.
  The 25 local manifest tasks completed in 104.82 seconds in total, as recorded
  by the existing timed runner; their command bodies and environment scopes were
  unchanged. Local replay used a manifest containing world entries 4 through 28.
- clang-format 14.0.6 dry-run and `git diff --check` passed.

Full hosted CI remains pending. Native fixtures do not establish configured
gameplay or visual acceptance. Restore the prior test from the immutable parent;
this commit's diff isolates the fixture correction without changing actors.
