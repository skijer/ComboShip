# OoT NEI save-editor item refresh

- Candidate: `poc/oot-nei-save-editor-20261016` (local, unpublished).
- Source baseline: `marsh6487/ComboShip`, `bridge/audit-merge-20261008`,
  `1bbb94e692629c0d910f97b57dd7cd02240d3775`.
- Upstream OoT pin: `ecd889c20`, Ship of Harkinian 9.3.0.

## Scope and preservation

Update the item entries inside the existing OoT save editor. Keep its 9.3
tabs, cards, ordinary inventory editor, equipment editor and MM editor.
No game inventory layout, save format, asset, scene, randomizer policy or
single-game mode implementation is changed.

The editor projects the current u16 Page 2 cell owners over the legacy byte
table; the underlying gameplay table remains unchanged.

| Cell | Current entry |
| --- | --- |
| 27 | Elemental Wand |
| 39 | Sheikah Slate |
| 41 | Hylia's Grace / Phantom Hourglass |
| 44 | Shadow Crystal |
| 46 | Shovel / Dominion Rod |
| 47 | Rod of Seasons |

The picker includes the shared-cell alternatives. Labels use the current OoT
item table; icons use the native inventory resolver, with a text button if the
resource is unavailable. The Poké Ball and Mario Mask have their own ownership
controls instead of occupying Page 2 cells. Bomb Arrows also uses its ownership
flag. Slate runes and seasons can be granted individually.

“Give All Custom Items (Max)” grants the current cells, both Grace/Hourglass
identities, both Shovel/Dominion identities, Bomb Arrows, all cane skills, wand
modes, Slate runes and seasons, plus the two crossover flags. In the medallion
wand policy, it grants the medallions needed to use the modes. In the gated
season policy, explicit editor grants latch the selected season's saved gate.
The seed policies themselves remain unchanged. Bulk grants preserve the current
rune, season (including Off), Hourglass/Grace selection and Shovel/Dominion
selection. Clearing item slots retains the independently owned powers, matching
the existing editor's treatment of those fields; granting again repairs the cells.

## Verification

- `python3 -B tests/oot_editor/run_tests.py`: production inventory stores and
  power handlers pass all nine wand/season policy combinations, repeat grants,
  cleared-cell recovery, full-width entries, selection preservation, and
  ordinary inventory/bottle/ammo/mask/currency/equipment preservation.
- `python3 -B tests/oot_editor/run_tests.py --control`: the baseline editor
  fails the missing-Slate cell assertion as expected.
- `python3 -B tests/mm_editor/run_tests.py`: existing MM editor grant and
  item-binding checks pass, including real-header MM editor syntax checks.
- `python3 -B tests/nei_shared_slot/run_tests.py`: both hosts' shared-slot
  ownership and healing checks pass.
- `python3 -B tests/seasons/run_oot_mode_tests.py`: native OoT gate checks pass.
- `scripts/diagnostics/run_receipt_syntax_tests.py` checks the complete modified
  OoT editor translation unit against actual game headers and the pinned ImGui
  1.91.9b docking headers. It passes with one compiler warning.
- `git diff --check`: passes.

The focused test is registered in `run_combo_nei_regressions.sh`. The entire
unrelated regression suite and a full linked game build were not run for this
source-only change. In-game rendering, clicking the editor and save/reload remain
untested. The next runtime check is the OoT Inventory tab: view the new entries,
grant the Slate/Hourglass/Crystal/Seasons, check their unlocks, then save/reload.

## Recovery

The source archive contains the full tracked candidate tree and a focused patch
against the baseline, with exact baseline/candidate commit identifiers in its
`RECOVERY` directory. Apply the patch to the matching integration source, or
compare its two editor files when merging later work. The baseline branch was
left unchanged; this candidate is not merged, published or runtime accepted.
