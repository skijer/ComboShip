# Snowhead approach: Winter mist visual correction

| Field | Record |
|---|---|
| Baseline | Public source `eb3c7cadd6419afd4a68015358880b8d27d0601e` |
| Candidate | `poc/bridge-tunnel-sand-winter-mist-20261026`; cumulative source candidate, no master promotion |
| Scope | Snowhead actor-local flakes and chill presentation; narrowly shared Winter mist visual helper |
| Preserved | Authored stations/mesh/collision, bridge material bindings, actors/lifecycle, precipitation/weather state, shimmer, arrivals, native asset ownership, deterministic local motion |
| Evidence | Production/native fixtures and native compilation described below; no game runtime executed |
| Verdict | Implemented and source/native verified; configured runtime and user acceptance remain untested |

The old local renderer used `gDust5Tex` intensity cards for both flakes and chill. `G_CC_MODULATEIA_PRIM` multiplied the texture's dark RGB into the flake color, while the local material inherited scene fog. The native I8 importer places the same intensity in RGB and alpha, confirming why the texture can serve as an opacity mask while darkening snow when multiplied into RGB. The chill cards were not Winter's mist visual. Player-distance fades alone did not account for a camera crossing an anchored particle or approaching a clip plane.

`SeasonWinterMist.h` extracts the actual Winter three-by-five mesh: fifteen vertices, eight triangle pairs, zero-alpha perimeter, 0.68-strength side core vertices, and RGB 192/222/245. Both `DrawWinter` and Snowhead now use that geometry and its one-cycle, untextured SHADE material, with alpha blending, depth testing, no depth writes and no scene fog. Seasonal state, placement, clipping decisions, aurora generation and Summer behavior are unchanged by this extraction. Native regression cases independently compare all shared vertex fields against the original Winter formula.

Snowhead retains its deterministic local drift and lifetime envelope. The local mist now uses Winter-sized horizontal wisps and Winter's 0.24 maximum alpha. Snow retains its native `gDust5Tex` resource and OoT owner push/pop, but RGB comes directly from an icy-white primitive color (225/242/255); the texture controls alpha only. Both layers fade with smooth camera-depth ramps before the near/far clip boundaries. The mist's zero-alpha mesh perimeter also permits continuous screen clipping without center-based screen culling. Geometry still occludes the effects through depth testing.

The local mist preflight accounts for each matrix plus the separately allocated frame vertices. Inspection of native `Graph_Alloc` confirmed that vertices consume the opaque arena tail; inspection of `NeiGi_ArenaHasRoom` confirmed that the existing matrix-count API can conservatively reserve those bytes. The native fixture verifies the reservation exceeds the matrix-plus-vertex demand and command count. No process-owned mutable vertex array was introduced into production.

Verification commands, from the candidate checkout:

```sh
COMBO_NEI_FIXTURE_CPATH=/workspace/scratch/371eba97e0b8/native-deps/root/usr/include python3 tests/regional_passages/run_chill_tests.py
COMBO_NEI_FIXTURE_CPATH=/workspace/scratch/371eba97e0b8/native-deps/root/usr/include python3 tests/regional_passages/run_snowhead_bridge_tests.py
ASAN_OPTIONS=detect_leaks=0 CPATH=/workspace/scratch/371eba97e0b8/native-deps/root/usr/include python3 scripts/diagnostics/run_soh_season_particle_tests.py --sanitize
git diff --check
```

All commands passed. The first focused regression was observed failing against the old production renderer with `Winter mist must submit 15-vertex soft-edge meshes, not dust cards` before implementation. Current native tests check the actual submitted packets and libultraship combiner generation, rejecting the old RGB-modulating combiner as a negative control. They cover texture-independent snow RGB, masked alpha, mesh edge alpha, exact Winter geometry parity, no fog/depth writes, native setup restoration, the actual emitted-alpha camera sweep, near/far fade limits, unavailable-texture independence for untextured mist, budget rejection/recovery, owner/mode/scene guards, unchanged object context, local drift/repeated-frame determinism, pause and cutscene freeze.

The complete Snowhead runner also passes the existing authored geometry/native Ice Cavern separation checks, real native dynamic-collision lifecycle/support/floor queries, ages/slots/re-entry/allocation-failure controls and production render-object interpolation C ABI check. The seasonal runner passes actual Winter mist/aurora, native weather coexistence, material/ownership checks, native renderer checks and real-header C++20 syntax. Its concurrent Summer checks reflect the separate Summer workstream.

Direct comparison against the baseline confirmed byte-identical `combo/entrances/FountainSnowheadBridge.h`, plus unchanged production `Build`, `Destroy`, `Init`, and `Update` bodies. Unchanged block SHA-256 values: `Build` through the start of `Destroy`: `69a395051c7326712759db439e762b389127bcceb8c579fcf172b60b822d2ca1`; `Destroy` through the local draw declaration: `bb0cafa4c6d8ac17a2727d60951a4f5b1175079714ea2e3d61b968febc001c7b`.

Dependencies were normal Ubuntu fmt, spdlog, nlohmann-json, tinyxml2 and glm development packages downloaded with apt and extracted into the scratch include root shown above; no substitute engine headers. Missing engine headers were restored through the root agent's sparse-checkout expansion. An initial seasonal sanitizer invocation without `ASAN_OPTIONS=detect_leaks=0` hit the environment's LeakSanitizer `/proc`/ptrace limitation; the corrected address/undefined sanitizer invocation passed.

Runtime follow-up: compare the approach from waterline through the valley mouth, pan/advance the camera through the mist, pause/unpause, and enter/exit a cutscene. Check both normal and Alt Assets configurations with the intended packs/load order, confirming bright flakes, Winter-like icy mist, smooth camera crossings and unchanged route/collision. No configured executable or pack configuration was tested here. Existing input archives and sacred master remain untouched; recovery is the bounded candidate source diff and the root agent's cumulative checkpoint/package.
