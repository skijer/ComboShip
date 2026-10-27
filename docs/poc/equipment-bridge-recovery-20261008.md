# Equipment bridge recovery — 2026-10-08

Baseline: PR39 source `35ce5f22f11c845a4ef0b28228880547e14ee92d`.
Recovered original branch: `fix/mm-pause-inventory-20261008`.
Bridge branch: `bridge/equipment-recovery-20261008`.
The package manifest records the exact final bridge commit/tree.

The original interrupted task's production source was recovered without reimplementing
its four fixes. The final ownership correction was already present and was retained.
The baseline and original checkout are unchanged by this bridge. Push is held.

## Resulting behavior

- MM accepts cursor entry from either arrow onto an empty item page and cycles extended
  pages with L while preserving wheel, description, equip-animation and same-frame guards.
- Shared scepter and full-width Rod of Seasons assignments survive both-host C/D-button
  serialization and application through native or NEI ownership, with marker/shadow/slot
  coherence. Unsupported local and unowned incoming items retain the documented policy.
- Equipment icons, cursor eligibility and names follow acquisition state. Genuine native
  starters survive imported/NEI replacements, while borrowed model nibbles and ownership-only
  sync deltas do not manufacture receipts or replace the active shield alias/skin.
- Four Sword retains its authored pickup identity and complete held graph on the existing
  supported rigs/skin routes, with independent deferred compounds and the existing hide,
  form, action, grip, alternate-asset and shelf guards.

## Fresh verification

- All 83 commands from `scripts/diagnostics/run_combo_nei_regressions.sh` passed. The bridge
  driver ran independent temporary-directory fixtures with three workers and retained
  every command's full output. The canonical script was used unchanged.
- Three controls execute the preserved baseline without moving HEAD: MM pause access,
  full-ID C/D transfer and equipment provenance fail for the reported reasons. The final
  ownership control executes 373 checks / 153 expected failures; the candidate executes
  373 checks / zero failures under ASan/UBSan.
- Asset collision check and all six CI shared-item/grant/dungeon/reward commands passed.
- Both complete FleetSync units and six other changed production entrypoints pass actual
  native-header syntax (eight entrypoints total). Included `.c` modules are checked at
  their owning production fixture boundaries rather than as standalone translation units.
- Changed CI-eligible C/C++/headers pass clang-format 14; `git diff --check` passes.
- Fresh independent read-only source review found no Critical, Important or substantive
  Minor issues. No source/index/branch changes were made by the reviewer.
- The final late-limb stage introduced Four Sword references into three unrelated fixtures.
  The recovered lantern, Din progressive-hand and Time Pedestal adapters provide an inactive
  Four query and fail-fast sword/allocator boundaries. Their existing assertions remain
  intact, with the original missing-symbol failures and successful reruns retained.
  Active Four Sword continues to use the dedicated production-dispatch/compound tests.
- The complete nine-command Din/Time Pedestal/lantern/cap/timegate follow-up passed after
  recovering the original final adapters. Only test dependencies were adapted at this
  final recovery step; production source did not change after the canonical gate.

## Boundaries and integration decisions

The output is a source/static/regression-verified recovery candidate. A linked Windows
or Linux application build has not been produced for this bridge, since pushing is held.
The installed archive stack, independent Alt combinations, actual controller/GPU behavior,
world switches and save/reload require the existing runtime checklist in the main POC.
No runtime acceptance or master promotion is inferred. Unrelated prior behavior is kept
within the preserved cumulative baseline rather than changed during this narrow recovery.

Apply the package commit/patch once onto the current cumulative integration tree using
three-way application. Keep any intervening integrations when resolving overlaps in
FleetSync, pause/player rendering and the regression runner. The source copies support
recovery and inspection; the patch carries the isolated delta. The package contains no
replacement model, icon or weather resource bytes. Build the standard application/port
archives from the integrated tree, then perform the main POC's installed-stack checklist.
