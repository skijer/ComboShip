# Final polish integration verification

Published baseline: `1c29c83f5d676d02ececa7302c138f7aa46b952a`, draft PR #34. Recovered original root source: `9994843f`. Existing completed GI worker `282b0090` was integrated as `7df56cb8`; the implementations were preserved rather than recreated. Existing Wolf, editor/wallet, receipt/icon and preservation reports remain applicable to their stated source/fixture scope.

## Independent final review

One independent final integration review inspected `7df56cb8` against the published baseline. It reported no Critical findings, two Important findings and no Minor findings. The implementer completed one focused correction pass and added production-boundary regressions; this is not a claim of a second reviewer rerun.

| Finding | Reproduction before correction | Correction and verification |
|---|---|---|
| Wolf skin capacity rejection did not reach Player_Draw; native continuation could open exhausted arenas | Real native OpenDisps advanced all three full heads after host returned submitted=1 | Skin supports opt-in caller reserve; host returns selected/skipped=2; exact native Wolf draw branch returns before continuation. Frozen scroll/matrix and native marker capacity are reserved before skin/pose mutation. Host and sanitized skin runners pass. |
| Intrinsic weapon flames were absent from model reservation | Actual Somaria flame's twelve-Gfx scroll plus matrix exhausted OPA tail before later model matrix | OoT reservation includes actual flame and later model; MM bridge preflights native flame and all subsequent model/restore matrices. New ASan/UBSan fixture runs real OoT/MM flame, scroll and marker bodies, checks short-capacity no-op and normal submission for Somaria and selected True Master Sword. |

The combined gate also includes the new flame fixture and the restored production dependencies of the song/icon bridge fixture. No missing dependency was bypassed by replacing the production decision logic.

## Current verification

The complete 43-command `bash scripts/diagnostics/run_combo_nei_regressions.sh` passed on final implementation source `8f771524`, with the pinned historical source fixtures and pinned dependency headers; the same complete NEI stage then passed in CI on published candidate `4612bd1a`. Exact resource/GLB parity passed for 62 GI models and 39 held components; HD icons/routes and optional-pack fallbacks also passed. Pinned clang-format 14, asset-collision, conflict-marker and whitespace gates pass. Exact-candidate Windows/Linux builds and packages follow publication.

## Runtime limits

Native matrices, deferred resource submission, GPU interpretation, real Wolf binary/assets and external model appearance require game verification. The synthetic Wolf rig and engine boundary fixtures prove source/lifecycle/allocator behavior, not runtime acceptance. Arbitrary inherited native get-item, active trail, equipment and third-party mod callbacks retain their existing allocator contract; these fixes do not prove whole-frame headroom. Actual interactions, enemy damage tables, camera/focus, freeze visuals, selected-mod appearance and Wolf passive equipment/anklet attachment remain runtime checks. Full seed-placement correctness also requires its separate existing acceptance workflow.

Keep PR #34 draft. Both game modules must build together. No merge or master promotion is authorized.

## Final harness publication review

An independent read-only publication review of `65145de1` found no Critical or Minor findings and one Important issue: the quest-held fixture was the remaining transitive consumer without native graph helpers or complete graphics arena initialization. The full gate reproduced that exact compile failure. Its runner now shares the same production helper extraction and temporary include path as the main/held fixtures; all three native arenas are initialized with explicit bounds. The quest-held runner passed after correction, covering five MM scenarios and seven OoT scenarios plus loaders/geometry/dispatch checks. The final complete 43-command gate passed after this correction; exact-candidate platform builds follow publication. No production behavior was changed for either harness repair.

## Fresh-runner logging dependency

The first published-head CI run (`37255727563`, candidate `8cf71afc`) reached the editor suite and exposed a standalone pool-binding link failure against Ubuntu spdlog/external fmt 9. The local bundled spdlog headers automatically enabled fmt definitions; the external configuration did not. The same missing-link dependency was reproduced locally with external fmt enabled. The standalone pool fixture now defines `FMT_HEADER_ONLY` explicitly; the complete editor suite passes with both bundled and external configurations, including production pools/GI bindings and both real translation units. This correction changes only test compilation, with no game-code change. The new exact-head CI run supplies the final complete gate/platform result.

## Final Young Epona harness dependency

CI run `37256522886` on `4612bd1a` passed every preceding gate stage, including the complete NEI suite, and stopped in the final Young Epona stage. Its full `z_message_PAL.c` syntax check lacked the `combo/menu` include directory required by the shared receipt-color header. The same missing-header failure was reproduced locally. The runner now supplies `-Icombo/menu`, matching the production SoH CMake include list. Its runtime fixture and all five production translation units pass; the remaining seven commands in that workflow stage also pass, including player/actor behavior, real JSON save round trips, asset checks and independent Epona cosmetics in both games. This correction changes only the standalone harness compiler arguments. A new exact-head CI run must complete the entire gate and both platform builds/packages.
