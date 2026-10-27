# Black Market codec CI include repair — 2026-10-22

Baseline: PR #57, `poc/market-forest-followup-20261022`, commit
`86cf83f6732b34e3ac15e5a0cf048e297af39684`, tree
`62f15b5a90b4e57e6d3c044a402adc2e7ca17c7a`.

Build Artifacts run `39818409809` passed Linux packaging and the entrances,
items, equipment and systems groups. The world group stopped in
`tests/black_market/run_text_codec_tests.py`: compiling native OoT `item.cpp`
reached `tricks.h`, which includes `imgui.h`, without an ImGui include path.
The workflow had fetched the pinned dependency headers and exported their
paths as `COMBO_NEI_FIXTURE_CPATH`; the legacy runner intentionally puts that
variable into `CPATH` only for NEI-origin tasks. The codec is a gate-origin task.

The OoT codec compiler now explicitly includes those prepared dependency paths,
with the existing Ubuntu ImGui directory as its standalone fallback. This is
scoped to the codec fixture. Global header scope, native source, assets, runtime
behavior and every required CI check are preserved.

Validation:

- Reproduced the identical missing-`imgui.h` failure before the change.
- Both-host codec runner passed with the workflow's pinned ImGui and thread-pool
  revisions, exercising native text lifecycle, metadata, choices and font glyphs
  under ASan/UBSan.
- All 21 CI runner/routing/workflow tests passed, including NEI-only global
  header scope and fail-fast behavior.
- Every command after the codec in `run_oct20_bugfix_regressions.sh` passed
  locally. The final native-forest suite ran 57 tests with 15 donor-dependent
  skips. Its initial local missing-CMake error was resolved by supplying CMake
  3.28.3 and rerunning that suite; the workflow already installs CMake.
- `git diff --check` passed.

The full CI rerun and Windows executable remain pending. No configured gameplay
acceptance or runtime-master promotion is implied. Recover the prior source at
the immutable baseline above; this commit's diff isolates the correction.
