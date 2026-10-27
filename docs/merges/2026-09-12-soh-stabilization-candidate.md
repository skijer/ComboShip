# SoH stabilization candidate — 2026-09-12

Status: candidate only; no full Fleet compile or game-runtime approval yet.

## Inputs

- ComboShip base: 62d1555e37071dcb113c1192b23a87cb2e6ddd35, retaining ced161fd72cafd1fa154bb868d33a8222093ec7d scrolling probe.
- Tested SoH donor: fcc86528612ecabb192fe3642acaacf7c2766869.
- Integration delta base: c29262b76 (NEI before the weather/static-actor merge).
- User additionally approved the uncommitted strict-MM direct texture binding experiment. It bypasses global HD overrides for strict display-list textures; visual correctness is still unproven. This strict graph also serves the existing Clawshot call site, which needs a runtime check.

## Reconciliation

Ported the 94-file soh integration delta, not a wholesale source-tree replacement. Preserved ComboShip's CMake project/DLL ownership and added only the focused test block. Five paths needed three-way reconciliation:

- AudioEditor.cpp: retain ComboShip context/resource handling; add donor night and weather registration.
- OTRGlobals.cpp: weather mix before inactive-game mute; initialization/reset added to existing audio lifecycle. Existing transition exports, thread joins, and save handling retained.
- z_play_otr.cpp: actor lifecycle logging added without replacing scene error handling.
- audio_heap.c: retain existing persistent-cache guard and larger 512-entry permanent cache, adding donor identity/eviction logic.
- audio_load.c: donor full-width font IDs and guarded sequence slots retained with ComboShip registration headroom and real-ID readiness fallback. Removed a duplicate readiness check produced by merge alignment.

The donor test assumed 32 permanent slots. The candidate test fills the actual array and verifies refusal at its boundary; sample-bank dummy entries avoid masking later real font loads. Production capacity was not reduced to satisfy the donor test.

## Verification

`bash scripts/diagnostics/run_stabilization_tests.sh <mm.o2r>` passed:

- 12 focused C/C++ executables;
- 10 Python archive-audit tests;
- production-function streamed-audio harness;
- production night-combat handoff harness;
- 34 culling rewrites and 59 texture rewrites across 22 actual MM display lists.

The streamed-audio harness also passed with `-fsanitize=address,undefined -fno-omit-frame-pointer` and `ASAN_OPTIONS=detect_leaks=0` (fixtures intentionally retain allocations).

Asset collision check: 1733 shared paths, 181 existing differing-content collisions, no new collisions. `git diff --check` passes. mm/, combo/, libultraship/, root CMakeLists.txt, upstream-pins.json, SoH ResourceManagerHelpers.cpp, SaveManager.cpp and ShipInit.hpp are unchanged from the candidate base.

## Outstanding gates

- Full Windows configure/compile/link and port-archive generation. Local environment has gcc/g++ but no CMake or Windows compiler; focused tests do not cover full translation units or cross-DLL ABI compatibility.
- Confirm scene reloads, Skull Kid and Clawshot visuals, Impa/Ruto/Navi behavior, rain cycles, dawn/combat music handoffs, large streamed catalogs, and repeated OoT/MM round trips.
- Inspect audio-thread shutdown/resume and strict-resource lifetime under the actual combined executable.
- Do not replace nei or declare the candidate stable before these gates pass. No upstream pin or release/save-version change has been made.
