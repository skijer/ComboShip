# Termina placed Moon tree seasonal foliage

Baseline: `e184851bf29491beec13d2170ff4592b1061b6d8` (published cumulative CI correction).
Candidate branch: `feat/termina-moon-tree-seasons-20261020`.

Cor requested seasonal targeting for the Moon tree already placed in Termina
Field, despite its unresolved bark rendering. This candidate extends the
existing MM scene-material seasonal draw treatment. It does not change the
source scene export or attempt to repair the bark or see-through faces.

## Exact inputs

| Input | SHA-256 |
| --- | --- |
| `1 - Termina Field Saria Song Giver.prelude.o2r` | `0c63a6f396cf1648f1707f143672bcea961ca58109450fcfba7a0b0c1d9fab7f` |
| `Moon-Tree-Prelude-Recipes-SeamFix-20261019.zip` | `6878696cb43ad86b90b83ca5d6c7471e86c9179e79935b58fc46586df756d065` |

Both inputs are preserved unchanged. The first is the current scene containing
the placed tree and Saria reference, rather than an older Moon scene pack.

## Selection and behavior

Read `prelude/project/edits.json` codec 2 from the archive owning each generated
`custom/prelude/Z2_00KEIKOKU/recipe<digits>` display list. Pair each shape's
`stored.textures[].label` with its `stored.texPaths[]`. Select only
`sou_ha_01`, `sou_ha_02`, `sou_ha_03` and `sou_sida_01`; accept only texture
paths belonging to that recipe. Generated placement IDs can change on import.
An unrelated export shadowing the global project path cannot supply the labels.

The supplied scene has 130 leaf batches (769 triangles), two bark batches and
11 other placed batches. The separate base-foliage recipe contains `sou_ha_02`
and `sou_sida_01`; it is not placed in this scene export, but those names are
covered by the focused fixture. Ground grass is not a Moon-tree target.

Use the existing canopy palettes: autumn `F4B783FF`, winter `CCDCEEFF`, spring
`F6C1D9E0`. Only triangles sampling the selected textures receive the existing
seasonal renderer commands. Mixed bark draws stay untinted. Source commands,
geometry, UVs, image pixels, alpha and culling stay intact. Summer, Off and scene
exit restore the original commands. Missing bark images are never loaded by
selection, which reads only project metadata. Discovery is cached between frames.

The mounted export must retain its codec-2 project metadata. Missing, malformed
or unrecognized metadata leaves the draw unchanged. Roots with a real `.meta`
alias are left unselected rather than guessing the resolved target's owner.
The supplied export contains only direct recipes. Existing native scene alias
treatment is unchanged. Reload the scene/export after editing pack contents.

## Verification and remaining proof

The new targeting test first failed on the baseline's unchanged leaf draw, then
passed through production update/variant code. Focused tests cover generated
IDs, all four labels, missing bark, mixed leaf/bark lists, owning-pack metadata,
native/Alt resources, unrelated material protection, malformed data, season/Alt
transitions and exact restoration. The optional real-export test checks every
placed display list against its original commands in all three palettes and
Summer/Off. Run it with:

```sh
python3 scripts/diagnostics/run_mm_autumn_scene_foliage_tests.py --moon-tree-pack '/path/to/1 - Termina Field Saria Song Giver.prelude.o2r'
```

The scene-material suite passed with AddressSanitizer, undefined-behavior and
bounds checks (`ASAN_OPTIONS=detect_leaks=0`,
`MM_FOLIAGE_TEST_CXXFLAGS='-fsanitize=address,undefined,bounds -fno-sanitize-recover=all'`).
Native tree/leaf actor controls and full production-header syntax checks for
combined and standalone MM also passed. Existing controller macro redefinition
warnings remain in the real-header compile.

These are source/command-level checks, not a configured GPU/runtime proof. No
full executable was packaged and this candidate has no hosted cumulative gate
result. User acceptance still needs the actual Termina mod load order: inspect
all sides of the tree in spring/autumn/winter, switch Summer/Off, leave and
re-enter the field, and check the bark/transparent faces separately. The earlier
cumulative CI corrections are the parent of this delta. Cor authorized adding
this candidate to PR #45; publishing it restarts that PR's gate and platform
builds. No master promotion is implied by this checkpoint.

## Recovery

The recovery archive contains the baseline-relative patch, full changed files,
exact candidate commit/tree, input hashes, test logs and restoration instructions.
Start from the exact baseline above and apply `changes.patch` with `git apply
--index`; verify the resulting tree against `checkpoint.json`. Keep the original
scene export installed in MM's normal mod archive location; this is a code
checkout change, not a replacement scene pack.
