# Woods of Mystery shimmer placement, 2026-10-24

| Field | Record |
|---|---|
| Baseline | Source `196ac9b7f50721dbc5102d6bf0d6b1f9b3fc58eb`, with the separately prepared wall texture-extent correction preserved. The supplied Windows `2ship.o2r` has exactly the same bridge display list and vertices as this tree. |
| Candidate | Narrow source correction on `fix/black-market-mm-native-shop-20261023`; no staging, commit, publication or promotion by this workstream. |
| Scope | Attach only the Woods of Mystery shimmer to the accepted diagonal bridge's black portal cap and contain its emitter centers within that cap. |
| Preservation | Portal/bridge/wall/canopy meshes, collision, texture resources, recent wall extent fix, route and trigger anchors, native/pack ownership, pause/cutscene guards, 20 actual NEI twinkles, scale, palette, alpha, pulse and motion. Other location effects are unchanged. |
| Evidence | Actual baked and accepted POC5 black triangles; matching shipped resource bytes; actual production draw-body submissions; geometric red/green regression; particle, renderer and ownership fixtures under ASan/UBSan. |
| Verdict | Implemented and source/native-fixture verified. Configured game/GPU appearance and user acceptance remain untested. An executable rebuild is needed; this is not an asset-only hotfix. |
| Recovery | Restore only this POC's owned source/test paths. Accepted archives and every native bridge asset remain unchanged. |

The shimmer used the front-mouth anchor `[110, 0, -110]`. The actual 28 black cap triangles are approximately 149.2–149.9 units farther into the diagonal sleeve, around `[216, 0, -216]`. The effect then put its centers another ten units toward the clearing. This explains the reported suspended particles without moving or replacing any portal geometry. The accepted POC5 cap occupies the same position as the native baked cap; POC5's other side-building portal is excluded from this comparison.

The actual draw body now uses a separate black-cap anchor `[216, 0, -216]`, retaining yaw `-8192`. `kMouthX`, `kMouthZ` and the gameplay/route anchors are untouched. Only the WoM `Mystery` emitter field changes from radius 60 / crown 126 to radius 55 / crown 115, matching the actual cap. This also corrects two upper outer emitter centers that previously projected beyond its arch. The existing ten-unit front offset avoids placing the particle plane directly on the black polygons. Every complete NEI twinkle retains its original art, `.8` scale, color, alpha, pulse and motion.

The shared particle header is byte-identical outside `ForestGiParticles::Mystery`. `Emit`, `Entrance`, `Portal`, `Saria` and `Mirror` are unchanged.

| Actual source geometry check | Before | Candidate |
|---|---:|---:|
| Distance from emitter centers to native black triangles | 159.218–159.907 | 9.364–10.000 |
| Distance to accepted POC5 black triangles | 159.218–159.907 | 9.364–10.000 |
| Centers outside either projected arch, per configuration | 2 of 20 | 0 of 20 |

The regression executes `DrawWoodsMysteryHexes` from production source and captures its submitted mesh and matrix transform. It compares all 20 world-space centers in three configurations: native ownership with Alt off, native ownership with Alt on, and the accepted pack-owned path. Before the fix, the new test rejected both source caps for excessive distance and coverage leakage. After the fix, all 60 submitted centers pass both caps. This models resource ownership and placement; it does not claim configured GPU parity.

Independent review accepted the implementation and identified a missing signed-side assertion. The checker now orients each actual cap triangle's normal toward the clearing and requires positive depth: the candidate measures `+9.364..+10.000`. A C++ fixture control mirrors the sampled local front depth, equivalent to changing `+10` to `-10`, while retaining its footprint. All 60 control centers still pass the unsigned-distance and footprint limits, but measure `-10.634..-9.861` and are rejected by the same placement checker for being behind the cap. This negative control first failed against the old unsigned checker and passes after the signed assertion. No production placement or geometry changed in this review follow-up.

Verification:

```sh
python -B tests/woods_mystery/run_shimmer_placement_test.py \
  --accepted <immutable-POC5.prelude.o2r> --shipped <matching-2ship.o2r>
python -B tests/woods_mystery/run_hex_test.py
python -B tests/woods_mystery/run_active_pack_test.py
python -B tests/woods_mystery/run_native_bridge_test.py
python -B scripts/diagnostics/run_forest_gi_particles_tests.py
```

All pass. The default placement test uses only repository assets and the standard library; it is included in the existing `run_hex_test.py` CI entry. The full particle suite checks actual native render submissions, canonical NEI vertices/palettes/alpha, both hosts' other portals and Saria poses, pause/cutscene/solo/combined guards, balanced matrices and exhausted-arena handling. Its broader existing fixtures emit controller-macro/CVAR macro redefinition warnings; there are no test or sanitizer failures.

The native bridge ownership/restoration fixture also passes Alt changes, reloads, missing/custom/failed resources, pack ownership, mode, room and scene changes. All eight baked asset records, the native bridge helper, baking script and manifest compare byte-for-byte to the pre-change snapshot. In particular, the recent wall extent correction retains SHA256 `95dc371107cc27052142c1af5cbd27f50edf80a7bdbd7f3a3aeb58402837c5e1`.

Unchanged portal source identities:

| Resource | SHA256 |
|---|---|
| `custom/cor_woods_bridge/bridge` | `670e0342d8ac99b816ef598f8a1c0fa324c87fcd028f980b0f9667a73445a649` |
| `custom/cor_woods_bridge/bridge_vtx` | `0746321360d7578d09f6f5000dcc72eee894af2ee4c09127999919d4bdf7ffc7` |
| Accepted POC5 archive | `cc1963d9dae25cd684cb0fc79948a398a611f1f49fad82ccd45f455f6f4c9257` |

`Woods-Mystery-Shimmer-Placement.png` is a scientific source-geometry comparison, not a gameplay screenshot. Its before and candidate centers come from executing the corresponding saved/current production draw bodies and particle headers. `geometry-evidence.json` retains those centers and the actual 28 cap triangles.

Remaining proof: rebuild the executable, enter Woods of Mystery room 1 with the user's native configuration and Alt setting, inspect the shimmer against the black cap from normal approach angles, then check pause/unpause and room re-entry. If the makeover POC5 is used, repeat with that exact pack and load order. Runtime appearance, acceptance and promotion remain pending.
