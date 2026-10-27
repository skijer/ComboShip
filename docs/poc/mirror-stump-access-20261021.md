# Mirror stump access candidate — 2026-10-21

Baseline: `e62668e077d699dd51e91af891a5d3567f012332`, retained on the isolated
`poc/native-forest-shimmer-mirror-20261021` candidate. This bounded work changes
the pickup contact gate and adds an isolated fallen-log access recipe. It does
not promote an accepted master or establish configured gameplay acceptance.

The old pickup cylinder deliberately reached 220 units below the loaded stump
cap. The new contract requires grounded contact on the native cap, within a
45-unit radius and five vertical units. Recall continues to land safely beside
the base. Saved ownership, native item presentation, seed/file/mode binding,
once-only grants, and the accepted choice/cast fixes remain protected.

Ruling: use a small fallen-log ramp rather than a vine angle exception. The
existing 80-high stump's broad lower faces are sloped and fail the native vine
angle gate. A walkable physical route preserves native player movement and does
not make unrelated scene surfaces climbable.

| Field | Candidate record |
|---|---|
| Collision input | All three native collision resources, SHA-256 `d98991a03ff914e651f7f0e276fa86faa6060636716500e25e7f7627dbe807ad`; 710 vertices, 958 polygons, 33 surfaces. The binary test fixture retains this exact input. |
| Access | East-side fallen trunk, top from `(-105, 0, 416)` to `(-345, 80, 416)`, 240 run / 80 rise, slope `18.435°`, 64-unit walk surface, 80-unit total width, 28-unit faceted thickness. |
| Physical changes | Append 16 vertices, 28 polygons and one exact copy of original wood surface 0 in each collision. Original bounds/header, vertex/polygon/surface records and camera/water tail remain exact. |
| Visible changes | 48 vertices / 28 triangles, matching collision; independent bark material and root. Uses `custom/comboship/native_forest/authored/textures/portal_bark` with its original logical 32×32 RGBA16 tile and retained HD metadata. |
| Integration | `mirror_stump_access.augment(native_forest_module, resources)` returns the root, culling bounds `(-225, 26, 416, 137)`, collision deltas and texture identity. The generator owns room-0 mesh and dependency integration. |
| Safety | Missing/incompatible resources reject without partial mutation. Reapplying a complete matching recipe preserves all bytes and returns `already_present: true`; partial/different mesh or collision data reject. |
| Pickup | Static cap floor beneath the grounded player, radius 45, vertical tolerance 5. Floor height matches the cap within 1 and its polygon belongs to the loaded fan. A ramp or dynamic platform cannot grant. |
| Recall | Stump-relative base arrival, including the current south landing `(-377, 0, 296)`, preserved. Its former positive pickup assertion is now negative. |
| Shimmer | Only the uncollected world Mirror calls the native NEI shimmer callback. Owned, queued, invalid, and overhead casting paths suppress it. The native renderer belongs to the separate particle change. |
| Configuration | Native combined MM forest, MM solo and OoT solo. Configured vanilla/Alt rendering and gameplay remain untested. |

Verification:

- Baseline Mirror fixture passed both native layouts under ASan/UBSan with
  `CPLUS_INCLUDE_PATH=/workspace/scratch/632353d61c30/test-dependencies/nlohmann_json/include`.
- RED: a new anchor assertion rejects the ground beneath the cap; the unchanged
  broad cylinder failed at `anchor_test.cpp` as expected.
- RED: tightening the cylinder alone still granted from a platform; the native
  floor-contact fixture failed before its implementation.
- RED: the world reward did not request shimmer before its callback was added;
  the native fixture failed at that assertion.
- `tests/magic_mirror/run_tests.py`: both full native layouts passed under
  ASan/UBSan. Covers base/ramp/platform/non-cap/null-floor/airborne rejection,
  once-only pickup/save, choice/cast/arrival/input teardown and world-only shimmer.
- `python -B -m unittest discover -s tests/magic_mirror -p test_stump_access.py -v`:
  four tests passed. The missing recipe failed first. Tests preserve every input
  record and tail, verify native planes and matching mesh, sample five lanes over
  25 positions with body/ceiling checks, preserve recall, reject partial changes,
  and verify byte-identical rebaking.
- Mirror integration (3 tests), all four transformed-slot gates, all four
  inventory/action/sync/save runners, geometry/visual checks (3 tests), and both
  native graphics-arena draw fixtures passed. The draw fixture initially needed
  the same local nlohmann include path; rerunning with it passed.
- `git diff --check` passed.

Status: implemented and source/native-fixture verified. The helper is integrated
into the generated scene by the separate native presentation work. No configured
game was run here, and no running master is promoted by these checks.

The decisive gameplay check is a fresh unowned save approaching the stump base:
the Mirror stays visible and unowned; walk up the east-side log and onto the cap
to receive it exactly once. Repeat in MM combined, MM solo and OoT solo, then
save/reload and recall from outdoors. Confirm the native receipt and shimmer,
clear base arrival, unchanged shop/portal routes, and vanilla/Alt parity.

Recovery: restore these owned source/test/doc paths from the candidate's parent,
or omit access augmentation when regenerating the native source. Accepted
inputs and original collision records remain retained independently.
