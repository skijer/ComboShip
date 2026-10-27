# ComboShip evening publication

The user authorizes consolidating the October 24 evening source checkouts and
publishing required compressed asset parts to `marsh6487/ComboShip`, updating
candidate/cumulative branches and draft PRs, and producing a Windows candidate.
Keep `develop` and `master` unmerged.

Baseline: public `602e430613ce3045f782f1ec9b76bd5878879784`, tree
`292685496dd311b2b1b97c0847eea537988b6c5a`, identical to local `bb6f927e`.
Retain the earlier audited cumulative source and all repairs in this baseline.

| Input | Exact local head | Scope |
|---|---|---|
| Shop | `66fedceece85fbc02eb6bf216fa2f44380c1fa0d` | Close OoT camera, browsing/return, counter barrier and optional original room materials |
| Bridges | `48cb1f7d149bd2aaea129da7ce874c714af91920` | Owned texture sampling, vertical outward water spill, OoT Fountain resource ownership and readiness |
| Solo OoT | `32a20c26af19ff40c03fd1bef299c053b1e4a2c5` | Mirror receipt close, raw segment restore on market entry, native A/B glyph colors |
| What's New | `20a01120e616e2413f5262e2acd65737d76044b7` | Color-only highlights, missing settings, OoT Rod registration order; includes Solo OoT head |

Preserve accepted scene geometry, native collision/waterboxes, alternate setups,
child/adult access, standalone/combined guards, stock and purchase semantics,
manual Prelude actor ownership, original archives and independent input recovery
packages. The vertical spill supersedes earlier inward/shallow water revisions.

Use the proven lossless compressed pools: each decoded part is at most 96 KiB,
each encoded text part at most 128 KiB. Reuse existing blobs; upload changed asset
parts individually with SHA/size receipts. Verify the assembled remote tree
against the complete local tree before moving any candidate ref. Exclude private
saves, logs, dumps, original game archives and intermediate build products.

Implementation/native tests, full builds, configured gameplay and user acceptance
are separate evidence levels. This publication is a test candidate, not runtime
acceptance or master promotion. Required gameplay checks include shop camera and
counter interaction, Mirror pickup/reset/shop re-entry, button glyphs, color-only
menus and Rod placement, Lake flow/materials, and both-age Fountain access with
vanilla/Djipi packs. Record exact public commit/tree, CI checkout and artifact
digests in the final publication receipt.

## Integrated verification

All four exact source heads are retained in local merge ancestry. Every input
changed blob matches its input except deliberate CI composition and the later
formatting/fixture repairs. Every baseline/input CI command is retained.

Fifteen focused commands passed: actual MM shop/camera, native counter/talk,
room graph/factory/ownership/draw exits and asset identity, both-host Mirror,
Solo OoT receipt/glyph/raw-segment lifetime, real ImGui menu/shared-seed, and
regional sanitizer/presentation translation-unit checks. Direct native OoT Rod
registration and MM Item Pool probes passed. Market preparation reconstructs
200 OoT and 212 MM resources. Asset collision checking passes with no new
collisions. The complete project clang-format 14.0.6 pass changes only four
product files' formatting; tokens and concatenated strings are preserved.

Independent source review found no Critical issue, one Important missing-CI
coverage issue and no Minor issue. The new CI coverage guard first failed on
all four missing room regressions and the parser dependency, then passed after the
required world group gained the existing room-material tests. The raw-segment
regression remains required through the original native entry shell suite.
The full CI policy suite passes 24 tests; the registry has 225 entries.
`python3-pycparser` is installed for the actual native draw-exit flow runner.
The Generate menu fixture also reproduced its missing room-material declaration;
it now includes the actual production header and checks the new widget while
retaining previous color, identity, input and complete-rectangle assertions.
The complete real ImGui suite passes after that correction.

Publication uses an exact tree snapshot parented on public `602e4306`; the
independently restorable recovery bundle retains the original local ancestry.
Existing compressed texture/geometry pools are reused unchanged. No new encoded
texture part or original game archive is introduced. These source/native checks
do not establish full-build success or configured gameplay acceptance.

## Windows compiler follow-up

The first full Windows run compiled SoH and both port archives, then stopped
in 2Ship on `LakeHyliaBridge.h:121`: MSVC C2760 at the ternary expression.
The Windows SDK's empty `far` macro erased the new water lambda's parameter
name. Subsequent diagnostics were parser cascades. A separate read-only CI job
extracted the bounded compiler excerpt after the large log download failed.

The production correction renames only that local parameter to `farBank`.
All geometry, material and water-flow literals and function bodies are exact
apart from that identifier. A regression compiles the actual shared header
with exposed near/far macros, reproduces the original parse failure and passes
after the rename. Full regional sanitizer and presentation syntax suites pass.

Both Windows workflows now compile/run that regression against the actual
Windows SDK before CMake configuration. The existing required regional suite
also runs its portable macro control. CI policy pins that early ordering.
The 2Ship build step retains its exit status and now emits a bounded compiler
annotation, preserving both the first cause and the last diagnostics when
the complete log is too large. The real MSVC replay and full final CI remain
required before any Windows package is delivered. No failed-run package is a
deliverable, and develop/master remain unmerged.

The next hosted world replay exposed one test-environment difference: those
older native entry drivers did not consume `COMBO_NEI_FIXTURE_CPATH`, although
their newly compiled actual ResourceManager state requires the project's pinned
thread-pool header. A control with the valid pinned header supplied reproduces
the original missing-header failure. Both drivers now extend only their own
compile environment from that existing pin; the dispatcher retains its original
scope. Native dual-host entry assertions and all 24 CI policy tests pass with
the narrower CI-style include environment. Production and asset bytes are
unchanged by this fixture-environment correction.
