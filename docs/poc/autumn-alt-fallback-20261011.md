# Autumn forest native fallback correction

| Field | Record |
|---|---|
| Baseline | `298fdc6c38510f17d5e614b9bdad8d9907481350`; runtime merge `3c3461990a23b5b257fb3a0cab3b4810823abad9` has the same source tree. |
| Runtime evidence | Supplied `Fleet of Harkinian(20261011-042441).log` submits Alt wallpaper `_034098` and canopy `_037098` without grayscale at 23:23:59; no forest binding message. User confirms liftable grass and Djipi brush turn autumn colours. |
| Source defect | A failed Alt lookup inserts a null archive entry. `HasFile` reports that entry present. Seasonal code consequently selects a nonexistent exact Alt display list and skips its cached native fallback. |
| Candidate | `poc/autumn-alt-fallback-20261011`, isolated from Grace and the set-aside owner diagnostic. |
| Correction | Seasonal Alt and `.meta` selection requires nonnull archive ownership. The loaded native draw stays eligible when a missing Alt was previously queried. Genuine Alt replacements retain priority. |
| Preservation | No new logging or renderer changes. Grass/brush palettes, assets, geometry, UVs, GBI commands, general archive APIs, audio and Grace remain unchanged. |
| Verdict | Source defect reproduced; correction passes production CPU fixtures and sanitizers. Candidate application build and runtime remain untested. No publication or promotion. |
| Recovery | Parent retained. Revert this isolated correction if candidate runtime evidence rejects it. |

## Why the old selection fails

`ArchiveManager::LoadFile(uint64_t)` reads `mFileToArchive[hash]`. A missing Alt path therefore acquires a map entry whose archive is null. `HasFile(uint64_t)` checks only map membership. `GetArchiveFromFile` returns null for the same path.

The renderer attempts an Alt before the native fallback. On the next seasonal update, the old `HasFile(altPath)` condition mistakes that failed lookup for an installed replacement. The exact Alt cache lookup and cold load cannot produce a display list, so the cached native forest is skipped. This reproduces the missing binding message and untinted forest while real Alt foliage replacements continue to work. The submitted runtime log does not itself capture the null archive entry; configuration-specific confirmation remains pending.

The correction is scoped to seasonal selection: it checks both map presence and actual archive ownership before choosing an Alt path or alias. It leaves the general archive loader and renderer unchanged.

## Verification

- Extracted production `ArchiveManager` methods reproduced the sequence: native loads, missing Alt initially absent, missing Alt load returns null, then `HasFile(alt)` is true while its archive is null.
- With the new missing-Alt regression and unchanged seasonal production code, the native renderer fixture failed with **0/14 wallpaper and 0/8 canopy** tinted triangles.
- After the correction, missing Alt and null `.meta` cases render **14/14 wallpaper and 8/8 canopy** with the copper palette, without retry loads; Off and reactivation remain correct.
- A real meta-only Alt alias still wins over the cached native resource even when a previous missing-base lookup left a null Alt entry.
- `python3 -B scripts/diagnostics/run_mm_autumn_scene_foliage_tests.py --mm /workspace/scratch/bef80075ef24/tint-evidence/native_field_sources.zip` passes. Existing coverage includes 160 forest owner cases, genuine Alt cold-load/toggle, resource replacement, private brush/grass, exact Off restoration and production renderer pointer writebacks.
- The same suite passes AddressSanitizer and UndefinedBehaviorSanitizer with `MM_FOLIAGE_TEST_CXXFLAGS='-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie' ASAN_OPTIONS=detect_leaks=0`.
- Changed C++ ranges formatted with clang-format 14.0.6; whitespace check passes.
- Independent review reproduced the passing native suite and found no critical or important issue in the correction.

These are local CPU fixture compilations, not another diagnostic game build. No candidate GPU/application build, installation change or new user runtime result is claimed. Publication remains on hold.
