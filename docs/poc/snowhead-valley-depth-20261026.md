# Snowhead threshold farther down the Fountain valley, 2026-10-26

## Delta design

Baseline: pre-relocation checkpoint `a765cd6fd156f8c8f489a692f3ff3f5a5f77c05f`, preserved durably by the integration owner before these edits. This is a narrow delta on `poc/bridge-tunnel-sand-winter-mist-20261026`, not promotion or configured gameplay acceptance.

The user wants Link to walk farther along the existing Fountain valley before the Snowhead handoff, leaving time to view a future horizon mesh. The agreed approach moves the fixed authored threshold to X3600 and grounded return to X3500, fits the entire shimmer to that cross-section, and moves local flakes/mist to the final approach. Runtime endpoint search or moving the actual path would add unrelated behavior and is excluded. No future horizon mesh is authored here.

Valley/collision triangles, ascent stations/widths, NPC/manual archive placements, global weather, packs, foliage, MM Snowhead arrival/framing, and ordinary owl arrival logic are unchanged. The corrected bright flakes and Winter mist mesh/material/depth/camera-fade implementation are retained; only anchors/proximity targets change.

## Exact locations

All positions are Fountain (`spot08`) world coordinates. Y comes from the existing fitted floor.

| Item | Before | After |
|---|---|---|
| Authored mouth | (1840, 280.892731, 0) | (3600, 225.984467, 0) |
| Grounded return | (1740, 270.713776, 0) | (3500, 234.475128, 0) |
| Mouth/return yaw | -16384 | -16384 |
| Trigger slab, exclusive endpoints | X1806..1840 | X3566..3600 |
| Shimmer center | X1830 | X3590 |
| Complete billboard depth footprint | X1824..1836 | X3584..3596 |
| Conservative full-valley shimmer Z bounds | -293..360 | -161..334 |
| Flake anchors | Seven ascent stations, mouth, X2400/3200/3800 | Grounded X3000/3200/3400/3600, Z0 |
| Mist anchors/proximity target | Seven ascent stations and mouth | Same four deep anchors |

The unchanged ascent ends at X1600. The authored mouth is now 2000 units beyond it, with first trigger contact just after 1966 units; relocation adds 1760 units to the previous route. The return remains 100 units west of the mouth and 66 units outside the trigger's nearest edge, preserving the no-bounce margin.

Actual floor-triangle intersections over the entire X3584..3596 footprint give common supported Z **-161.812363..334.833649**. Inward-rounded limits -161..334 retain support through camera-facing depth and native 1/16 packing. Existing ten-column fitting spans both banks and preserves NEI art.

The deep body band has half-width 40 with adult radius 18 reserve, so legal foot-center lanes are Z±22. Existing `BodyClearAt` guards remain authoritative: Z±20 crossing samples pass; Z±23 and feet 21 units above ground are rejected. The visual shimmer does not certify a wider walking lane. Existing complete-area floor-union and 80-high body-prism checks cover the corridor through X4080.

The four weather anchors have floor Y values 277.254761, 260.257233, 242.965805, 225.984467. The 650-unit proximity fade and 350-unit full-opacity radius remain; centerline weather starts feathering after X2350, leaving the ascent and former mouth clear. Budgets remain 72 flakes / 18 mist meshes, with unchanged local time, culling, arena guards and render-state restoration.

## Red/green and verification

New tests first failed because X1820 still triggered and native weather still appeared on the ascent. Moving the mouth alone then made the existing packed shimmer test reject the old lateral bounds at its first vertex. All three checks passed after the final changes.

Commands ran from `/workspace/scratch/371eba97e0b8/bridge_candidate` with:

```bash
export CPATH=/workspace/scratch/371eba97e0b8/native-deps/root/usr/include
export COMBO_NEI_FIXTURE_CPATH="$CPATH"
export ASAN_OPTIONS=detect_leaks=0
```

| Command | Result |
|---|---|
| `python tests/regional_passages/run_tests.py --sanitize` | **PASS, exit 0:** full regional source/native gate, relocated getter/departure/grounded return, Snowhead collision/camera/render/chill, MM fixed framing and ordinary owl/pause-resume controls, handoff/boot/priority/cancellation guards. |
| `python tests/regional_passages/run_chill_tests.py` | **PASS, exit 0:** no cold effects on ascent/old mouth/X2200; deep bright flakes and Winter mesh, actual fades, frozen motion, owner/mode/scene cleanup and native object context. |
| `python tests/regional_passages/run_valley_tests.py` | Floor samples, complete projected floor-union support and 80-high body prisms **PASS** for all three bands across native/export/export_alt, six setups each. Later unchanged renderer compilation **BLOCKED** by missing external `BS_thread_pool.hpp`; exit 1. |
| `python tests/regional_passages/run_presentation_tests.py --syntax` | Art preservation, all 180 shimmer ticks/three cameras and actual native OoT endpoint presentation **PASS**. Later unrelated wave-audio fixture **BLOCKED** by missing external `zip.h`; exit 1 before optional complete syntax/MM presentation stages. |
| ASan/UBSan C++20 `ascent_test.cpp` and `snowfall_test.cpp` | **PASS, exit 0:** delayed crossing, grounded walk/body lane/jump/return guards and existing particle motion/alpha. Exact compiler commands in `narrow.log`. |
| `git diff --check` | **PASS.** |

Missing sparse resource metadata blocked an initial presentation attempt; the integration owner materialized it before the recorded run. No production workaround was added for host dependencies. No full-suite success is claimed for blocked commands. Leak detection is disabled for the host process namespace; ASan/UBSan remain active.

Records:

- `/workspace/scratch/371eba97e0b8/verification/snowhead-valley-depth/presentation.log`: actual captured stdout/stderr, including dependency failure.
- `/workspace/scratch/371eba97e0b8/verification/snowhead-valley-depth/narrow.log`: actual final narrow command/exit records and protected-geometry comparisons.
- `/workspace/scratch/371eba97e0b8/verification/snowhead-valley-depth/observed-runs.json`: exact commands, tool-session IDs and observed results for earlier runs whose stdout remained in the tool transcript; explicitly a structured status record, not a raw transcript.

Production changed ranges were formatted with **Ubuntu clang-format 14.0.6**, using `native-deps/root/usr/bin/clang-format-14` and its matching library directory.

## Protected geometry and handoff

Pre/post exported rounded ascent meshes are byte-identical, SHA256 `b00fa32c74c43eb441f2b66daf0e07ff0f61a7a5dd7aa7dcd4486791f5b2577f`. `FountainValley.h` is unchanged, SHA256 `0e9730e4ac831bb4147d58795247f0509deff35e404aef34b85366623d5b862e`. The actor delta changes only flake-anchor and mist-anchor/proximity selection; no material, vertex, fade, blend or collision construction edits.

Owned paths: `combo/entrances/FountainSnowheadBridge.h`, `soh/soh/Enhancements/Gameplay/ComboFountainSnowheadBridge.cpp`, `tests/regional_passages/ascent_test.cpp`, `tests/regional_passages/snowhead_bridge_test.cpp`, `tests/regional_passages/oot_native_test.cpp`, `tests/regional_passages/chill_native_test.cpp`, and this note. No staging, commit, remote action or durable save by this worker; the integration owner checkpoints separately.

No configured game was launched. Travel time, camera sightlines, actual floor/occlusion, weather appearance/density, installed-pack/load-order/Alt parity and user acceptance remain untested. Walk through the former mouth without handoff/cold effects, continue through the deep weather band and new grounded threshold, cross to MM, then return outside without bouncing. Compare both ages and ordinary MM owl arrival with the preserved configuration. Future horizon-mesh placement remains separate authoring work.
