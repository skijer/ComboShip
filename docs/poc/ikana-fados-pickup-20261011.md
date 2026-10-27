# Fados Ikana pickup orientation — October 11, 2026

The supplied Fados replacement is authored for the worn shield. Its face's chin
points toward negative X, but its nearly round rim is only 3.45% wider than tall.
The pickup geometry classifier's 5% threshold consequently returns no roll.
The archive has no GI override or model matrix that rotates it; the authored
Ikana GI is a separate resource and already stands upright.

| Field | Record |
|---|---|
| Baseline | Draft PR #41, `bridge/audit-merge-20261008`, exact parent `aa0acc3f31c47bd396620d1bcc86b5b663a4c623`. Previous platform builds apply to that parent. |
| Candidate | Isolated `poc/ikana-fados-pickup-20261011` worktree; one pickup pose correction on that parent. |
| Source | User-supplied `Fados Ikana Mirror Shield.o2r`; SHA-256 `a5239d0186b22e53d6a5797c8a406fe4903f830a97c6004c191b432976a43051`. |
| Selected root | `alt/objects/object_link_child/gLinkHumanMirrorShieldDL`; 2,290 loaded vertices across 76 batches; XYZ spans 2,545 / 2,460 / 624. |
| Scope | Recognize the exact selected vertex stream in the existing GI geometry reader and return positive 90-degree Z roll. The transformed negative-X chin points down. |
| Preservation | Original archive, equipped/worn/held draws, authored GI, owner selection, matrices, general 5% classifier, other replacements and cumulative parent fixes. |
| Configuration | Install the original replacement unchanged. Match the parent test's packs, load order and Alt setting; test pickup in both hosts where used. Exact user load order and loader-level integration remain runtime checks. |
| Evidence | Archive traversal and regression failure before the code change; normal and ASan/UBSan Ikana suites pass afterward. Independent review confirms the fixture and reports no blocking findings. |
| Verdict | Implemented and focused-test verified. Platform build and configuration-specific visual acceptance for this candidate remain pending. No master promotion or runtime acceptance is implied. |
| Recovery | Revert this isolated follow-up to restore the exact parent behavior; the original mod archive remains unchanged. |

The match uses count 2,290 and FNV-1a64 `015b5334888c6583` over each original
signed 16-bit XYZ coordinate, low byte then high byte, in display-list load order.
Any resource model matrix, non-integral/out-of-range coordinate or edited stream
declines the special case. Filename reuse and equal bounding-box dimensions do
not trigger it. Texture-only edits retain the same correction.

The optional `--fados-archive` test input reads the original archive locally,
walks the selected display-list graph and validates all 2,290 coordinates and 76
load boundaries. The loaded stream's SHA-256 is
`ffd72b3eb8f13e9c70938e99dfa0d3130e11286e3b26ee221eaef3913b24f328`.
No archive or vertex stream is distributed in this follow-up. Automatic approval
review rejected publishing the original binary fixture because inspection/testing
authorization does not authorize publishing the mod's contents; the test was
revised to use the archive only as a local input. Independent review verified
the original local fixture byte-for-byte against the supplied archive.

## Verification

- Before the production change, the archive regression fails at
  `Fados pickup face must point down` against the parent's actual pose query.
- `python3 tests/ikana_gi/run_tests.py --fados-archive /path/to/mod.o2r` passes
  normally and with `--sanitize`.
  Both OoT/MM owners and XML/binary vertex resources are tested in all four
  existing host/build configurations. Controls cover an upright version, an
  unrelated stream with exactly the same bounds, live Alt switching and the
  original raw stream already rotated by a resource matrix. Existing authored
  geometry, native geometry, exports, draw consumers, owner and cache tests pass.
  The default invocation also passes without the private archive and explicitly
  reports that this optional mesh regression was skipped.
- The production reward-surface reader test passes normally and under
  ASan/UBSan, covering native/XML topology, hash resources, projected UVs,
  destination slots, material segments and malformed graph rejection.
- Clang-format 14's dry run and `git diff --check` pass. Independent review also
  compiles the reader with `GBI_FLOATS` and warnings treated as errors.
- The supplied archive's SHA-256 still matches the original after all checks.

Local native-resource fixture builds used `CPLUS_INCLUDE_PATH` pointing to a
temporary `spdlog/spdlog.h` because this checkout lacks that dependency. The
header requires `SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_OFF` and supplies only the
disabled trace macro used by the real resource destructor. Resource classes,
geometry traversal and pose query were compiled from the repository. This is
focused fixture verification, not a linked game build. Sanitizer runs disable
leak detection in this environment, as existing runners do.

## Decisive runtime check

Use this candidate's executable and matching `soh.o2r` / `2ship.o2r`, with the
original Fados mod installed. Acquire/display the Shield of Ikana pickup and
confirm the face stands upright with the chin down. Then check the equipped
shield, switch Alt selection and repeat the pickup. The authored GI should keep
its existing upright pose. A different edited mesh intentionally follows the
generic classifier and needs its own evidence if it remains sideways.
