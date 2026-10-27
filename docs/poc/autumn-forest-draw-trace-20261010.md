# Termina forest draw-state trace

| Field | Record |
|---|---|
| Runtime baseline | Supplied build `d3328aafacaf2612e99dd42ce2f3e35bbba7b7a8`; PCM logging was disabled before the forest draw. |
| Candidate parent | PR41 head `8d7293ee83bb522b04d980b1744a84ad1a50ae11`. Its autumn source and renderer match the supplied build. |
| Reported failure | Upper canopy appears tinted; lower forest wallpaper stays green with Alt Assets both on and off. |
| Scope | Observe the two Termina forest textures at actual triangle/shader submission, including generated scene namespaces and private `.meta` payloads. |
| Preservation | Tint colours, seasonal wrappers, brush tint, geometry, UVs, textures, archives, audio and renderer submission stay unchanged. Only diagnostic metadata and capped logging are added. |
| Verdict | Diagnostic implemented and CPU-fixture verified. Visual failure is not reproduced or fixed here; runtime evidence remains required. |
| Recovery | Remove this diagnostic commit; retain the parent and existing accepted tint work. No develop/master promotion. |

The supplied log reports `[AutumnForest] bound` for native `room_00DL_0241B0`. That message precedes patch application and rendering. It does not prove the wrapper executed or the selected shader received a tint. The native list owns both wallpaper (`Tex_034098`, 14 triangles) and canopy (`Tex_037098`, eight triangles). Both receive the same `B96848FF` wrapper in the extracted production-handler fixture.

`[AutumnForestDraw]` records the selected texture's resolved resource path, current archive owning that path, format/dimensions, render tile and TMEM slot, RDP grayscale flag, vertex-packing grayscale flag, RGBA, selected shader IDs/grayscale option and backend. The archive field is named `path_owner_archive`: it is path ownership evidence, not a claim about an older cached resource's provenance. Resource-instance changes trigger a new sample. Requested texture identity is carried through texture loading so a `.meta` alias with a private PNG path remains observable. Raw N64 pixel pointers are never inspected as names.

Unchanged draws stay quiet. Reporting stops independently after 16 entries per layer for the process; no console toggle is required. An unused grayscale palette is excluded from deduplication while tint is off, so unrelated palette changes cannot consume the budget.

Verification:

- The new triangle test first failed because no draw-state observation existed. The alias case separately failed before alias tracking was added.
- `python3 -B scripts/diagnostics/run_gfx_triangle_run_tests.py` passes: actual prepared shader IDs/options and submitted copper vertex attributes, nonzero render tile/TMEM ownership, native/Alt aliases, archive lookup with absent `ResourceInitData::Parent`, excluded resources and independent output caps. Existing 16,384 renderer variants retain bitwise-identical vertex output and backend events.
- `python3 -B scripts/diagnostics/run_mm_autumn_scene_foliage_tests.py --mm <native_field_sources.zip>` passes all existing cases, including 160 native/private/mixed forest cases and real-handler Off/Autumn, scene-reset and pointer-writeback transitions.
- Modified C++ lines formatted with clang-format 14.0.6; `git diff --check` passes.

These are CPU tests with archive/backend seams. They do not run the application, compile a real GPU shader or establish visual acceptance.

Next decisive check: use the executable containing this commit, enter Termina Field with Autumn active and the existing packs, then inspect the wallpaper/canopy `[AutumnForestDraw]` entries. A missing layer indicates a different selected resource/path; differing flags or colour locate a state discrepancy. Matching copper state on both layers moves investigation to shader execution or overlapping visible geometry. Keep PCM logging off.
