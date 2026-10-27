# Fast-source dependency timeout repair — 2026-10-22

Baseline: PR #57, `poc/market-forest-followup-20261022`, commit
`221635898bd65d19c5eeb7c0e6ce9a643f9b5e3d`, tree
`56d5325b4e00ceedceb0f4d416cc974e54aead88`.

Run `39823047985` was cancelled at the fast-source job's ten-minute limit while
APT downloaded `libllvm14t64` and `libclang-cpp14t64` from
`azure.archive.ubuntu.com`. The conflict-marker and asset-collision checks passed;
the formatter and the build/test matrix never ran. The same mirror stalled the
earlier formatting-fixed attempt. This cancellation does not invalidate the
previously verified codec or arrival-fixture corrections.

Preflight now creates an isolated Python environment and installs binary wheels
from `scripts/ci/preflight-requirements.txt`: clang-format 14.0.6, PyYAML 6.0.2,
and the existing pinned Pillow requirement. The formatter wheel is about 1.1 MB,
compared with APT's 36.2 MB formatter dependency chain. A `clang-format-14` alias
preserves the existing formatting command. The setup step has a three-minute
bound plus explicit pip timeouts/retries. The Ubuntu runner already supplies
Python's venv support, as confirmed by the inspected installation logs.

No source, assets, formatting version, generated-pixel baseline or required
matrix checks change. Regression/platform dependency setup is outside this fix.

Validation in a fresh isolated environment using the new requirements:

- All packages installed as wheels; the formatter reports version 14.0.6.
- All 21 CI runner/routing/workflow tests passed.
- The generated sword UI deterministic-output check passed.
- Asset collisions passed with no new collisions.
- The complete existing formatting script ran and left `soh`, `mm` and `combo`
  source byte-for-byte unchanged according to `git diff --exit-code`.
- `git diff --check` passed.

Full hosted CI and Windows packaging remain pending. No runtime acceptance or
master promotion is implied. The immutable parent preserves the prior setup;
this commit's diff contains the workflow and requirements repair.
