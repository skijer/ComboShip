# Termina Field private brush autumn tint

| Field | Record |
| --- | --- |
| Baseline | Source checkpoint `56c83a878562052ca873b7d414f7d0e1175b6325`, isolated branch `work/overnight-autumn-20261009`. The supplied material trace reproduced the gap against this exact source; the screenshots do not establish an executable commit. |
| Candidate | Register the exact `foliage_poc3/leaves_rgba` and `foliage_poc3/stems_rgba` identities in `AutumnSceneFoliage.cpp` with existing forest copper-red `#B96848`. |
| Scope | Termina Field material ownership only. Existing normal, `_scene` and explicit Alt hash handling supplies the aliases. Keep the existing native forest `_034098` and `_037098` registrations and legacy root fallback. |
| Preservation | No archive, texture, vertex, UV, collision, scene/setup, actor, gameplay, weather or audio edits. Strip the inserted tint commands and every original display-list word remains equal, including alpha, combiner and render commands. Off restores the original loaded commands. |
| Configuration | Six actual Alt brush lists from the supplied POC4 archive; the native forest list is a separate control from `mm(2).o2r`. Archive/cache and season selection are harness boundaries; the production transform and lifecycle execute unchanged. |
| Verdict | Implemented and verified by production-list tests, sanitizers and real-header compilation. Final scene appearance, lighting/fog, pack precedence and visual acceptance remain untested in game. |
| Recovery | Retain the original archives and source baseline. Revert this isolated source/test/documentation change to remove the candidate. No master promotion is implied. |

The three brush top lists load ordinary terrain `Z2_00KEIKOKUTex_01C650`
before private `foliage_poc3/leaves_rgba`. The terrain already produces a
nonempty per-material variant, so the whole-list fallback never runs. The
unregistered private texture then clears the selected color. Explicit material
registration fixes that boundary without changing the transform or its cache.
Stems now use the same per-material path, retaining their existing copper-red
result without relying on whole-list tinting.

| Display-list suffix | Material | Before | After |
| --- | --- | ---: | ---: |
| `0153A8` | Private leaves | 0 / 3576 | 3576 / 3576 |
| `015DF0` | Private leaves | 0 / 997 | 997 / 997 |
| `016598` | Private leaves | 0 / 2296 | 2296 / 2296 |
| `0158E8` | Private stems | 244 / 244 (root fallback) | 244 / 244 (material) |
| `016180` | Private stems | 142 / 142 (root fallback) | 142 / 142 (material) |
| `016A38` | Private stems | 200 / 200 (root fallback) | 200 / 200 (material) |
| `0241B0` | Native lower forest `_034098` | 14 / 14 | 14 / 14 |
| `0241B0` | Native upper forest `_037098` | 8 / 8 | 8 / 8 |

The new archive-backed regression failed before production edits with
`copper-red tint covers 0/3576 triangles`. After the two registrations it passes
for all 6,869 leaf triangles, 586 stem triangles and both forest controls, using
the exact `0xB96848FF` grayscale color. It checks exact command preservation,
100 updates without additional loads or variant allocation, Off restoration,
reactivation using the retained variant, and safe teardown. The default test
also covers mixed terrain, leaves, stems and an unrelated material in both
scene namespaces and Alt hashes; terrain retains its existing ochre color and
the unrelated draw remains untinted.

Reproduce from the repository root with the original archives:

```sh
python3 scripts/diagnostics/run_mm_autumn_scene_foliage_tests.py \
  --poc4 /path/to/zz_Termina_Field_3DS_POC4.o2r \
  --mm '/path/to/mm(2).o2r'

ASAN_OPTIONS=detect_leaks=0 \
MM_FOLIAGE_TEST_CXXFLAGS='-O1 -g -fsanitize=address,undefined,bounds -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie' \
python3 scripts/diagnostics/run_mm_autumn_scene_foliage_tests.py \
  --poc4 /path/to/zz_Termina_Field_3DS_POC4.o2r \
  --mm '/path/to/mm(2).o2r'
```

Archive SHA-256 values used for this proof:

| Archive | SHA-256 |
| --- | --- |
| `zz_Termina_Field_3DS_POC4.o2r` | `e8ac8fc5c5a4f4d15f1c9ba489225376d05210ba78dd739fae91a1269b776ec8` |
| `mm(2).o2r` | `17ca3a4be1726b3080c5e7ea923e0673cd55ec560439d77478b7663663087c6a` |

The existing autumn tree/leaf, grass and MM weather suites also pass. The full
scene-foliage production module passes C++20 syntax compilation with real
engine headers. Native-header checks retain existing controller-button macro
redefinition warnings. Address, undefined-behavior and bounds instrumentation
pass; leak detection is disabled in this container. Formatting and whitespace
checks pass. No full platform build, generated game archive, GPU rendering or
configuration-specific runtime proof is claimed here.

The next decisive test is the candidate executable in Termina Field with the
same scene/setup, season, Alt setting and actual mounted-pack order as the
baseline. Compare the two forest layers and brush tops/edges under the same
lighting/fog, then select Off and re-enter to verify restoration. Preserve the
accepted installation and original archives for that comparison.
