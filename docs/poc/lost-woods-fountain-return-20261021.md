# Conditional Lost Woods Great Fairy return

Update: the source-native room-7 north tunnel is now covered without a scene
archive. See `native-lost-woods-fairy-20261021.md` for the bounded follow-up,
exact baseline, regression evidence and recovery checkpoint. The authored-exit
contract below remains supported for additional or relocated custom entrances.

Status: composed into the October 21 morning source candidate; configured
gameplay acceptance pending. cor's later morning-roundup instruction authorizes
the cumulative source publication and build, superseding the earlier hold while
other bugfixes were collected. See `morning-roundup-20261021.md` for the composed
baseline, Woodfall integration and fresh regression evidence.

- Baseline: `fix/bugfix-integration-20261020` at
  `8ab33deeb5000c6a66663dc9659d9060f702881a` (tree
  `a6a6319e451515c26f27dc4a9a99f2c62be3629b`).
- Candidate: `fix/lost-woods-fountain-return-20261021`.
- Scope: an authored collision exit from OoT Lost Woods to a native Great Fairy
  Fountain remembers its local approach. The fountain's ordinary exit returns
  to that room and grounded position only for the resulting visit.
- Protected: native/randomized fountain entries and exits, existing forest
  portals and solo forest routing, inventory/progression, and Farore/void
  respawn points. No archive, geometry, collision, texture or actor edit.

## Entrance authoring contract

The new collision should retain its intended native Great Fairy entrance index
in the Lost Woods scene's exit list. Both magic and spell fountains and their
native spawn variants are supported. The exit-list slot, tunnel coordinates
and Lost Woods room do not need to match the old pink passage.

The controller samples grounded, non-exit floor points in the current Lost
Woods room. When an actual collision exit selects a Great Fairy Fountain, it
captures a nearby approach point, preferring at least 48 units of separation.
Return uses that point with an idle start facing outward. Entry/return for
this authored connection bypass entrance shuffle; normal entrances keep their
existing native or shuffled behavior.

This uses actual collision exits, not a proximity entrance. The source-native
room-7 north floor is intercepted before entrance shuffle and has a known safe
approach for debug warps. Other custom connections use their authored Fairy
collision exit and sampled grounded approach. The existing coordinate-based
private-forest portal is preserved.

## Visit and save lifecycle

The marker is consumed by the fountain's native dynamic exit. Traveling to
another scene clears it. File initialization/rebinding and seed changes cannot
inherit another live visit. A versioned `lostWoodsFountain` native save section
preserves an ordinary save/reload made inside the local fountain visit.
Malformed saved origins are rejected. The saved marker is ignored during an
explicit paired-game arrival (`gComboReturnFileNum >= 0`), so a portal,
dungeon/boss, or Farore return retains its destination and respawn payload,
including an explicit arrival into the same fountain.

RETURN is borrowed only to spawn back in Lost Woods and then restored. TOP
and DOWN are untouched. Save-worker synchronization occurs outside saveMtx.

## Verification and remaining test

- Regression reproduced before implementation: custom Lost Woods entry was
  still passed through the ordinary entrance shuffler.
- Review regression reproduced before correction: saved fountain metadata
  overrode a later explicit paired-game/Farore arrival.
- `run_fountain_return_tests.py --sanitize`: passes against native OoT types
  and the production collision-exit resolution. Covers moved rooms, six native
  fountains, combined/OoT-only modes, native/randomized exits, save round trip,
  explicit-arrival priority, malformed data, seed and file cleanup, and respawn
  preservation. Included in the entrance runner and pre-build CI gate.
- Existing native solo forest route tests pass for both engines.
- The complete `tests/lost_woods_entrance/run_tests.py` suite passes, including
  native portal boot/fade/handoff, renderer linkage, five-form camera arrivals,
  opening-forest progression, native portal saves, scene/title/mesh selection,
  stump return, camera/particle policy and authoring-tool preservation checks.
- Real-header syntax checks pass for the new C++ controller, `z_play.c` and
  `z_player.c`; the compiler reports 1, 0 and 334 warnings respectively.
- Independent review has no remaining actionable findings.

No full application build or in-game traversal is claimed. With the new source
built, enter through room 7's north tunnel with no scene mods and exit back to
it. Then enter the same fountain through its original entrance and verify the
original return. Repeat after saving/reloading inside the local visit. A custom
entrance export is optional and should retain its own authored destination.

## Recovery

Apply the saved source checkpoint's `CHANGES.patch` to the exact baseline above
to restore its recorded source and tests, or fetch the named candidate branch
in `marsh6487/ComboShip` for the published source and current handoff notes.
The baseline remains unchanged. The source-native candidate adds no asset
dependency; a separately authored Lost Woods entrance export is optional.
Do not promote this candidate to an accepted scene/runtime master based on
static checks alone.
