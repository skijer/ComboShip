# Farore's Wind cross-game POC — 2026-10-08

Status: implementation and focused native checks pass; gameplay proof and user acceptance pending.
Do not promote this candidate. Current push hold remains in effect.

## Baseline and isolation

- Repository: marsh6487/ComboShip.
- Sacred baseline: develop at `a82852a3a02c8cabc541cce7c124da71c5755efb` (PR #38).
- Candidate: `poc/farores-cross-game-20261008`, isolated worktree.
- No accepted baseline, parallel candidate, archive, texture, or scene asset changed.
- This checkpoint contains source and recovery material; it is not a playable release build.

## Trace

MM already implements Farore's Wind in `mm/mods/items/logic/item_oot_spells.c`, including its
cast, return/dispel/exit menu, pillar and arrival. Its early scene whitelist allowed only temples,
their bosses and Moon trials. Clock Town and Termina Field were rejected before the spell or menu
could activate. OOT's HUD separately disabled the item in ordinary overworld scenes.

The native warp points also lived in separate saves. MM uses the NEI sidecar; OOT uses its native
Farore/respawn fields. Neither native entrance number can identify a location in the other game.
The legacy FleetShip shared-memory warp route does not implement the resident-DLL Combo launcher.

## Candidate behavior

- One slot-scoped point in `combo.faroresWind` records its owning game and native entrance,
  room, position, facing and temporary puzzle flags. Native save layouts are unchanged.
- Casting in ordinary MM outdoor scenes is enabled in COMBO_BUILD. OOT's normal gameplay HUD
  permits the shared spell. Existing gameplay, magic, ownership and medallion routing remain.
- An existing point opens the return menu without requiring new casting magic, even if the point
  belongs to the other game. Return, dispel and exit use the native menus.
- Same-game recall continues the native transition. Foreign recall queues the launcher's existing
  clean-frame handoff, saves source progress, loads the destination slot and then applies its native
  entrance/respawn. Recall retains the destination save's age/form; it does not grant equipment.
- Source saving captures live OOT scene flags through Play_PerformSave. MM captures the current
  scene and promotes all five cycle-scene flag fields used by its native save writer.
- The arrival animation consumes the shared point, like the existing native return. Foreign points
  do not draw a local pillar. Explicit null metadata prevents stale native saves from resurrecting it.
- Older saves adopt the first existing native point encountered. A game without a native point
  leaves absent metadata available for the other game's migration. Whole-slot copy preserves it.
- Song of Time and moon-crash rollback clear MM-owned points and their obsolete puzzle state;
  an OOT-owned point survives MM's cycle reset.
- Malformed records fail type/range checks, native scene/entrance/layer checks and destination room
  checks. A bad room falls back to the ordinary entrance spawn and discards the point.
- New points are refused inside MM grottos and OOT grottos/fairy fountains because native DOWN
  respawn describes their parent scene. Existing points can still be recalled from those scenes.
- Standalone MM keeps its original scene whitelist. No spell assets or visuals were changed.

## Evidence

- Initial regression check reproduced the COMBO outdoor whitelist failure before implementation.
- `python -B tests/farores_wind/run_tests.py`: 8 groups pass. These compile and execute production
  policy, JSON, native bridge and launcher container bodies using real native save structures.
  Coverage includes disk reload, both handoff directions, copy/clear, legacy migration, missing
  target callbacks, sentinel slots, point consumption, all MM flag words, malformed entrance
  layers/scenes, invalid room fallback, age retention and MM departure flag capture.
- `python -B tests/nei_shared_slot/run_save_tests.py`: MM and OOT native save regression groups pass.
- Native compiler syntax checks pass for all 15 changed translation units using the available
  Linux build commands/dependencies; the final two changed units were rechecked after review fixes.
- `git diff --check` passes. Independent final code review reports no Critical/Important findings.
- The MM fixture emits existing controller macro redefinition warnings. They are not failures.
- No full candidate link, Windows package/CI build or game execution was performed. Fixture
  handoffs exercise the production host callbacks with destination entry points supplied by tests;
  they do not establish linked DLL switching or animation/camera correctness.

## Required gameplay proof before promotion

Use a copied save and record the candidate build/configuration, archive set and vanilla/alternate
asset mode. Test with Farore owned and a magic meter acquired in both games.

| Case | Required result |
| --- | --- |
| MM Clock Town / Termina Field, C button and D-pad | New point casts; native menu opens afterward |
| OOT point, switch to MM, select Return | OOT resumes at its recorded native room/respawn |
| MM point, switch to OOT, select Return | MM resumes at its recorded native room/respawn |
| Same-game Return / Dispel / Exit | Native transition/consumption, dispel and idle cleanup work |
| Existing point with low/empty magic | Return menu opens without charging a fresh cast |
| Change chest/switch, recall immediately, return to source | Current and earlier scene progress survives |
| Quit/reload and copy slot | Point persists and stays isolated to the correct slot |
| Song of Time / moon-crash rollback | MM point is cleared; OOT point survives |
| Grotto/fountain with and without a point | Existing recall works; new point creation is refused |
| Normal Mask Shop / Clock Tower portals, reset, owl quit | Existing destination/title behavior remains |
| Cast with one OOT age, recall with the other saved age | Saved age retained; no equipment grant |
| Vanilla / alternate spell assets | Pillar, cast and arrival display correctly; no false foreign pillar |

Acceptance requires both directions to pass in gameplay. Stop on a save or placement regression;
recover to the sacred baseline rather than combining unrelated changes into this POC.
