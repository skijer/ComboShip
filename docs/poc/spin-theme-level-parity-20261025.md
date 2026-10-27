# Spin themes at both charge levels — October 25, 2026

Baseline: local cumulative `cc11cfcdd2d157017cb7be37e61301fd8f63a8a3`,
tree `54ca0f4860ea91affc0498d8f36c8d5b03872097`, identical to public
`83eed9fff57d7b5dfa1f27ecf5fbe6b65e411832` in `marsh6487/ComboShip`.
Candidate: `fix/spin-theme-level-parity-20261025`; use its Git commit/tree
identities for recovery. This is a separate merge input, not a runtime master.

The user reports correct level 1 themes but flame artwork in some level 2
variants. Inspection of both hosts' actual private resource graphs confirms
Ice and Light great disk/cylinder lists bind their Fire donor's flame mask
`spin_lightning_mm_poc1/gSpinLightningLevel2Tex`. Their regular lists already
bind each style's own `burst1` sigil. The misleading legacy texture name does
not identify its artwork. Fire, Lightning, Arcane, Astral and Dark Energy have
their intended regular/great bindings; host-specific Astral flow is preserved.

Only Ice/Light great bursts now copy their **authored** disk/cylinder command
streams into frame memory and rebind the existing themed texture pair. They
retain their level-specific geometry, vertices, UVs, tiles, scroll calls,
combiner, palette, alpha and elemental accents. Their regular bursts and the
other styles keep direct authored submissions. Native continues to follow its
host/custom shell. Combat, magic gates, preference snapshots and charge previews
are unchanged. All texture pixels, generated resources, dependency manifests
and bounded compressed source parts are unchanged; no asset uploads are needed.

The shared material clone preflights aligned frame-arena space, including one
`Gfx` of headroom for `OPEN_DISPS`. OoT's real open writes a temporary OPA
marker even for XLU-only work; close removes its pointer without restoring the
word. Without headroom, an exactly fitting cylinder copy was overwritten.
Allocation/resource failures return to the existing native fallback before
submitting an incomplete burst.

Verification:

- The shipped-resource renderer test fails on the original Ice great binding.
- A tight-arena test fails before the allocation preflight; an exact-fit replay
  using the actual host open/close functions reproduces OoT marker corruption
  before the final headroom correction.
- The complete default spin runner passes with ASan/UBSan: policy, exact resource
  closure, rendering for both hosts, state, native/combat behavior and complete
  actor translation-unit compilation for OoT/MM standalone and Combo builds.
- Actual submitted commands are checked for seven styles × two levels × two
  hosts, with per-host Astral expectations, original-resource immutability,
  nontexture command preservation, missing/malformed resources, allocation
  failure, tight capacity and successful one-command-headroom rendering.
- Independent review's headroom finding is resolved; follow-up review reports
  no remaining findings in scope. Existing native button/pointer compiler
  warnings are retained. No full Windows/Linux executable build is claimed.

Reproduce with the project's dependency headers available to the compiler:

```sh
python3 -B scripts/diagnostics/run_spin_style_tests.py
python3 -B scripts/diagnostics/run_spin_style_tests.py --sanitize
```

Recovery: fetch the candidate branch from `marsh6487/ComboShip` and check out
its recorded commit on a separate branch. Its full tree retains the cumulative
baseline and the unchanged compressed spin pools. The bounded fix can also be
cherry-picked onto that baseline; preserve all unrelated cumulative work.

Runtime status: untested in the installed game. No active mod load order or
Alternate Assets configuration was established in this pass. The decisive
smoke check is Ice and Light at regular/great charge in both games, then a quick
regular/great comparison of the remaining dropdown themes, with default and
edited colors and optional sword-element accents. Source verification does not
establish GPU appearance, installed-pack parity or user acceptance. No accepted
master or cumulative branch is promoted by this checkpoint.
