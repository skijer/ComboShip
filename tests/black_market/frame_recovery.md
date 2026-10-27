Recovery checkpoint for native model-frame capture (2026-10-17).

Restored from the recorded implementation/review design against exact baseline
8ccbd47d: scoped capture; observational XYZ resource query; same-host native,
authored and retained legacy model reports. No controller, pack, gameplay or ABI
struct changes. `ModelBoundsReader::Read` retains its fit output.

At the first recovery checkpoint, the fresh ASan/UBSan C++ capture test passes:
asymmetry, all eight transformed corners, union, nested/disabled scopes, no offer
leak, invalid values and cross-translation-unit reporting. Production body and
resource-query regression fixtures and native object compilation are pending.
This is a recovery/WIP checkpoint, not a completion or runtime claim.

Second recovery checkpoint: fresh `run_frame_capture_tests.py` also passes the
actual OoT/MM generic draw bodies and all 13 retained legacy submission cases at
.25/.32 shelf scale (idle/wave cape, local/routed cloth, real/missing spell,
recolored boots, tridents and Four Sword). Resource paths at odd byte addresses
and inactive/disabled zero-query behavior pass. Fresh MM legacy-loader and cane
regressions pass. Passive test boundaries are restored in existing fixtures;
resource-query traversal, remaining regressions and native compiles are pending.

Third recovery checkpoint: fresh full GI renderer regression passes, including
actual authored native/shared frame fit, asymmetric Switch Hook center, both
shelf scales, procedural seasons and shimmer exclusion. Exact reader bounds
pass on both hosts. Fresh resource-query runtime fixtures pass owner/Alt changes,
atomic pair failure, private donor selection/fallback, narrow native segment8
material allowances and exact real Hylia/Zonai/Demise XML geometry. OoT legacy,
spin and mask regressions pass. The receipt-query and sword-pose scripts reach
their real Ship typed-header phase, then stop at missing `SDL2/SDL.h`; those two
commands are not claimed as overall passes. Native object checks remain pending.

Inactive captures perform no query work. Locked selected/native geometry walks
existing typed resource caches (normally 1–2 roots, max 16/query; 65,536 vertex
visits, 16,384 commands and 32 graph/matrix levels per reader). No new persistent
frame cache is introduced. Runtime frame time and full linking are unverified.

Final continuation: all nine native frame/resource/legacy object checks pass
with restored headers. The formerly blocked receipt-query and sword-pose typed
header phases pass, as do the full sanitized GI renderer, selected resource
graph, legacy loader/cane, OoT fallback and spin/mask suites. Both actual native
market X draw bodies and native keeper initializers pass. The shared forest is
merged into the primary candidate; see
`docs/poc/market-final-presentation-20261017.md` for the current source/asset
record. Full linking and configured gameplay remain unverified.
