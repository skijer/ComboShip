# MM seasonal ground coverage, 2026-10-26

| Field | Record |
|---|---|
| Baseline | `53f21d5c0d1f1bb12b6afd249c8ba60b8f2065eb` |
| Scope | MM seasonal parser compatibility, reviewed ground texture fallback cohort, reviewed outdoor grass/bush children |
| Protected | Existing tint formulas and appearance routes, tree/Moon foliage, mixed stone/bark/alpha, geometry/UVs/material commands, native resource ownership, queued buffers, unrelated bridge/Summer work |
| Candidate | Source implementation/native fixture verification; configured game runtime and user acceptance untested |
| Recovery | Bounded candidate diff on the existing POC branch; root owns checkpoint/package/integration |

The merged Termina archive's `alt/scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU_room_00DL_005EC8` has a valid native `G_RDPSETOTHERMODE` packet (`EF182C10 / C8112078`, archive offset `0x2A8`). The seasonal parser previously treated opcode `0xEF` as unknown and discarded the entire floor variant. Its grass texture hashes were already recognized. A read-only copied-command counterfactual established this cause before implementation. The source now accepts and preserves that one-word packet. A permanent exact-resource-derived fixture checks all 57 local grass triangles in Autumn/Winter and exact original commands after removing only inserted seasonal tint commands.

The surface query list now includes the same reviewed ground identities for Autumn/Winter that Spring already queried. Existing Autumn/Winter draw tint remains authoritative wherever a supported loaded draw applies it: broader selective pixel copies are a fallback, not a conversion of accepted draw materials. This preserves the existing treatment of non-green samples, texture modulation and brightness. The coverage classifier tracks the actual primary render tile through the production parser, normalizes native/Alt/generated scene aliases, and uses original command snapshots instead of an installed seasonal branch. Whole-list wrappers cache their safely parsed image coverage as well, so cold and warm frames make the same choice. When an accepted wrapper contains an unknown command, its explicit unclassified state conservatively blocks only newly broadened Autumn/Winter fallback copies; original surface treatments remain unchanged. Unknown command lengths are never guessed.

The existing eligible draw selection/preload code now runs once before choosing pixel fallbacks. The exact same resolved resources then supply coverage and actual variant installation. This prevents a cached native draw from classifying texture A while a real cold Alt replacement using texture B loads later in the same update and receives a draw tint over newly tinted pixels. No additional unrelated cold assets are loaded; tests retain the single expected internal Alt load.

Coverage is retained in existing immutable per-season/resource variants. Warm draws and already-ignored resources do not rescan the material parser or build temporary display-list vectors. A cold coverage-only pass emits no display-list words. Tests count these parser-only calls in the extracted fixture and assert no further walks or command-buffer replacements across 120 steady updates, including all four supplied large Termina grass roots in native/Alt and all three seasons.

Reviewed randomizer grass/bush foliage children now receive all three seasonal palettes throughout eligible outdoors, matching Spring's previous scene coverage. Reward flower/stem children remain excluded. Weather eligibility still comes from `MMWeather_FoliageSeasonForPlay`; no actor, weather, arrival or save-state policy changed.

The user's requirement to preserve formulas is enforced by byte comparison against the baseline. These complete function bodies remain byte-identical after formatting:

| Function | SHA-256 |
|---|---|
| `SeasonPalette` | `f6a95fd6540f90c38445f5a00b5e64972f23238026d8add28ea014cfa7f0f3b4` |
| `SeasonTintMode` | `b8e74f9f8682be86fbdfa97ee4c36d4d5adabdbf33f89836fe2f1d8f111af9ea` |
| `BuildSurfaceTexture` | `5c91d6220e961de019fe7dcf2380098ece1a7b8e89990aa7db675e4e84a587dd` |

No palette, blend/brightness constant or green-pixel mask changed. Existing accepted scene/material tests retain their original expected draw palettes, modes and exact texture bytes. New renderer tests assert one treatment per binding: selective pixels with no draw tint when fallback is needed; original pixels plus the existing draw palette when a supported binding appears. They cover both scene namespaces, native/Alt, cold canonical and `.meta` loads, reloads, Autumn/Winter/Spring/Off/Summer transitions, outside texture replacement, exact non-green/alpha/source bytes and immutable queued pixel copies. Multi-frame wrapper controls exercise unknown commands both before and after the ground load. Cold-Alt controls exercise unsupported native/supported Alt sharing one texture, and supported native/Alt using different reviewed textures with both images already cached, checking first/warm frames, toggles, Off restoration and unchanged load counts.

Verification, from the checkout:

```sh
PYTHONDONTWRITEBYTECODE=1 CPATH=/workspace/scratch/371eba97e0b8/native-deps/root/usr/include ASAN_OPTIONS=detect_leaks=0 MM_FOLIAGE_TEST_CXXFLAGS='-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie' python3 scripts/diagnostics/run_mm_autumn_scene_foliage_tests.py --materials /workspace/scratch/371eba97e0b8/inspection/termina_materials --moon-tree-pack /workspace/scratch/371eba97e0b8/termina_inputs/Termina_Field_Henriko_Prelude_MoonTree_Optimized_POC1.o2r
PYTHONDONTWRITEBYTECODE=1 CPATH=/workspace/scratch/371eba97e0b8/native-deps/root/usr/include ASAN_OPTIONS=detect_leaks=0 MM_FOLIAGE_TEST_CXXFLAGS='-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie' python3 scripts/diagnostics/run_mm_autumn_surface_texture_tests.py
git diff --check
```

The complete scene/material fixture suite and supplied-archive gate passed under address/undefined sanitizers. Archive-specific coverage includes four actual serialized grass draws, and five actual Moon leaf batches / 1271 triangles with twelve protected bark/building/grass batches. The real `Fast::Texture` surface suite also passed. The complete floor regression was observed RED on the old parser before its fix; the new broad-ground regression was separately observed RED against baseline source in a scratch adaptation (`shown != source`), with no production rollback or archive edits. Reviewer-identified cold-Alt and unknown-before-load wrapper failures were each observed RED in permanent fixture controls before their corrections.

Scratch evidence is in `inspection/mm_broad_grass_gate.log`, `inspection/mm_broad_baseline_red/result.log`, `inspection/mm_cold_alt_red.log`, `inspection/mm_wrapper_unknown_red.log`, `inspection/mm_coverage_order_green.log`, and the earlier `inspection/termina_material_probe/`. The merged archive remains unchanged, SHA-256 `977080dcd5e538f50b431a6d7b0e667a83207e37fd8bef04c981e982aa9038a2`.

A standalone full-translation-unit syntax attempt was blocked by missing external `BS_thread_pool.hpp` after obtaining normal SDL2 development headers. No substitute header was introduced. The production/native fixtures compile the actual implementation against native MM/GBI headers with the established archive/cache/GPU seams. A configured full game executable was not built or run for this change.

Selection limits are deliberate: only the existing reviewed Spring ground cohort and explicit foliage children are broadened. A shared image already owned by an accepted draw retains that route across its aliases; the fallback does not override that accepted appearance to recolor some other unsupported draw of the same image. Arbitrary private grass identities or unknown display-list extensions remain outside this additive scope. In-game normal/Alt appearance, traversal, transitions and FPS still require configuration-specific runtime verification and user acceptance.
