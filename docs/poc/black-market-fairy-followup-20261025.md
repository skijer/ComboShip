# Black Market fairy runtime follow-up, 2026-10-25

Baseline: public candidate 965e7c2f39d5c60d9be4438a784d3d4663b2c966, tree f86ab296602d9046a1f282c8c0aa68b9457b8d60 (local equivalent 134c875d1addad1f5ada6a15e1ae472d8e0f2a60). Prior full Windows/Linux CI and package remain available independently.

The user confirmed the framing, but reported attention cursors on Shadow Crystal, Bunny Hood and other stock and a visible companion during browsing. The supplied 30.296-second recording was sampled at 14 and 24 seconds: shop selection brackets and a separate white native attention arrow are distinct. Framing is accepted only as reported; this does not accept all other gameplay checks.

## Root cause and bounded correction

The private MM InitNativeStock bypasses native EnGirlA_InitItem, which explicitly clears ACTOR_FLAG_ATTENTION_ENABLED. Restoring that operation prevents all eight private merchandise actors from entering fairy attention/lock-on candidate selection, while retaining friendly/update flags and native shop selection. EnGirlA_Update2 and DoNothing do not restore the flag. OoT private merchandise uses its existing GI renderer rather than those native MM stock actors.

The prior quiet policy deliberately kept the companion model, animation and lights visible. Both hosts now return before native fairy or replacement Midna drawing when the existing private-shop companion predicate and native hidden-player bit 29 are active. Native shop browsing owns that bit; ordinary movement and return clear it. Receipt/get-item presentation may show the companion when native code restores Link. The light context draws its halo separately: both native light-update paths now use LIGHT_POINT_NOGLOW under the same browsing predicate, keeping radius/local illumination unchanged. Ordinary updates restore the native glow type automatically. No actor draw pointers, settings, saves, animation or other fairy types are altered.

Protected: accepted camera/FOV/framing, selected-item motion, shelf anchors and 1.5x scale; keeper A/talk, shop selection brackets and transaction behavior; ordinary companion/fairy behavior; every earlier cumulative fix and asset. No asset/archive changes.

## Verification

New real-draw cases failed before the draw guards in both hosts. A native shop-spawn fixture now starts with the actual targetable profile flags and failed before the stock correction. After correction:

- run_midna_navi_draw_test.py: real native/replacement draw matrix, native update clock and complete OoT fairy actor syntax pass.
- run_mm_midna_test.py: real native/replacement draw matrix, audio routing/mixer/resource checks and complete MM fairy actor/HUD syntax pass.
- run_shop_camera_mm_tests.py: native shop initialization, actions, camera ownership/returns/receipts and all five forms/eight selection paths pass, including geometry clearance.
- Both draw matrices cover combined/matching/foreign host modes, private/ordinary scenes, hidden/visible player and non-companion fairy controls. Existing graphic/state assertions remain. Native-light tests also verify halo suppression with Midna disabled, unchanged radius and other-fairy controls; removing only the halo guard fails both hosts at the expected light-type assertion.
- run_fairy_quiet_native_tests.py retains native update and keeper A/C-Up separation checks.

Native checks use existing fixture-sysroot nlohmann/ImGui includes; missing local includes were resolved through per-command CPATH without changing repository dependencies.

Independent source review found no blocking issue. Native/source evidence is not configured GPU/gameplay acceptance. Runtime still needs the target cursor absent on all stock; native and replacement companions hidden through browse/select/cancel/refused purchase; ordinary return/receipt and outside-shop behavior; keeper A/talk and blue selection brackets retained. The Time Gate materials and renderer-corruption causes remain unresolved.

Keep develop/master unmerged and accepted installations/archives unchanged. The previous public commit/package and this bounded source diff provide rollback.

The initial public follow-up ed9f7b6 started CI 40243433693. Additional source review caught the separate native light-context halo before delivery. Its bounded child includes that path and reruns complete CI; the initial run is superseded, not package acceptance evidence.
