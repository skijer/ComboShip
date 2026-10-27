# Native MM Leaf/Shovel softlock correction

Baseline: PR #24 head `cd9c95043dd477cab5908c4afa2c3e233517c20d`.
The user's `Fleet of Harkinian(4).log` records build `cb69b63` at 18:11:44,
then in-game resets at 18:16:13 and 18:16:24. That build's tree is exactly
`250b98ec5cb6499616c3e500145c4c6a7d7ebdf6`, matching the published head.
The user reports the same freeze for both tools and a Leaf remaining active
after reset. This candidate is a correction to that failed input-lock change;
it is not a master promotion or runtime acceptance.

## Cause and bounded correction

`CustomItems_Update` runs before `Player_Update` selects native input. Starting
either tool immediately set the global active flag, which the new movement
gate used to zero the very press needed by native `Player_UpdateItems` to
equip it. The tool then waited for an upper action that had never been
installed. Process-global tool state also survived Player replacement.

The movement gate now requires the matching held action and upper callback,
plus an animation tick from that callback. Pending activation keeps its input.
The authored Leaf/Shovel animation starts on the first upper callback, after
native equipment changes finish; otherwise the native equip animation could
overwrite the pending tool animation and consume its first use.

Tool cancellation runs before custom-item early returns and on native put-away.
Player destruction and initialization both reset these two transient tools and
invalidate the Leaf collider owner. Initialization also covers Combo resume's
heap replacement. Cleanup preserves native camera/cutscene flags and excludes
other Player instances. The previous rod, Mitts, texture-owner and arrow-pack
changes are retained; no art, save/progression, or Din's Fire changes are made.

## Evidence and limits

- The production input-selection plus native item-button/use pipeline failed
  against the published gate: the activation press never reached native equip.
  The correction passes for both Leaf/Shovel on C-left and D-down.
- The production Player destructor failed the stale-state regression before
  cleanup was added. Tests now cover destruction, initialization without prior
  destruction, and reuse of the same Player address.
- A finished native equip animation replacing `skelAnimeUpper` reproduced a
  missing first gust/dig. Deferring authored animation setup fixes that probe.
  Actual `PlayerAnimation_Once` advances the tool to completion; each produces
  exactly one effect and then releases movement.
- Interruption probes cover cutscene state, lost upper-action ownership, slot
  removal, damage, and preservation of a real native camera-suppression flag.
- The fixtures use native MM structures and production function bodies.
  Graphics/pose decompression, sound, collision, and the native equipment
  transition recorder are test boundaries; they are not in-game proof.

Local verification: the complete native MM player unity object compiled, the cumulative
`run_combo_nei_regressions.sh` suite passed, clang-format 14 was applied to the
changed C sources, and `git diff --check` passed. Independent review found no
remaining critical or important blocker.

The decisive runtime check remains: use Leaf and Shovel from another held item,
repeat each after completion, then reset/re-enter Clock Town during an action.
Link must regain movement, the authored gust/dig must occur, no stale tool may
remain after reset, and ordinary tool use must not add letterboxing. Also check
Leaf glide/landing and one interruption by damage. Master stays unchanged.
