# Bridge runtime review — original findings and takeover corrections

Status: both findings below are corrected in the owned takeover candidate. The
production-function regression fails on parent `92d01a01`, then passes with the
recovered corrections, including all four bosses, first/repeat, local/foreign,
identity and disabled controls. ASan/UBSan also passes. Native build and in-game
acceptance remain separate; see `cross-dungeon-restart-20261015.md` for current state.

Historical read-only review of committed runtime source through `50740049`, against baseline
`a82852a3`. No gameplay reproduction is claimed. These are concrete source-path
bugs found by the bridge's independent runtime reviewer, while the original
implementation session corrects fixed-item structural validation. The original
session was the source/build/matrix/packaging owner at the time of that review.
The owned bridge takeover supersedes that ownership; the findings below preserve
the original diagnosis and requested corrections.

## 1. First MM boss warp can bypass the cross-dungeon escape route

For Goht, Gyorg and Twinmold's first warp, native `z_door_warp1.c` sets the target
to `ENTRANCE(GIANTS_CHAMBER, 0)`. That target is not itself an escape alias. The
new hook in `mm/src/code/z_play.c` composes only `nextEntrance` during
`Play_UpdateTransition`.

Later, the forced rando Giants' Chamber skip in
`mm/2s2h/Enhancements/Cutscenes/StoryCutscenes/SkipGiantsChamber.cpp` directly
rewrites `gSaveContext.save.entrance` from its queued `GIEventTransition`, after
the transition interception. It replaces the target with Mountain Village
spring spawn 7, Zora Cape spawn 9, or Ikana Canyon spawn 15, then calls
`handleGiantsCheck`. These are native MM overworld escapes even when the MM
dungeon's cross return should lead to its source OoT mouth. The outside scene
also clears the destination checkpoint.

Compose the final forced-skip target at a boundary that preserves Giant/Oath
and boss-reward bookkeeping. Test first and repeat warps separately, including
OoT -> Snowhead -> first Goht completion -> original OoT return.

## 2. Same-MM remap retains a source-specific cutscene layer

`combo/rando/CrossDungeonMmRuntime.inc.h` rewrites only `*index` for a local
route. Gyorg's repeat bluewarp sets Zora Cape spawn 9 with
`nextCutsceneIndex = 0xFFF0`; Twinmold uses `0xFFF2`.

Concrete accepted permutation: Stone Tower normal mouth -> Great Bay Temple.
Its Gyorg completion remaps Zora Cape spawn 9 to Stone Tower spawn 2, but leaves
the Zora Cape cutscene index staged. `Play_UpdateTransition` adds cutscene layer
1 when reading the remapped entrance flags, then Play initialization selects the
same layer. `sStoneTowerEntrance2` in `mm/src/code/z_scene_table.c` has one entry,
so indexing it at layer 1 reads outside the entrance array and may produce a
wrong spawn or crash. Twinmold's layer 3 has the analogous failure.

Clear source-specific cutscene staging when an actual remap changes the
destination, preserving required boss/Giant/Oath bookkeeping. Cover local
first/repeat warps and a cross-game return; disabled routing must keep native
cutscene behavior.

The bridge now has an executable RED regression for this finding. It extracts
the actual production `Combo_MM_CrossDungeonTransition` function and uses the
real `RuntimeState`/seed codec, with only native save fields supplied as fixtures.
Disabled behavior passes its control, and an actual local remap fails with:
`FAIL: remapped MM bluewarp retains source cutscene layer fff0` (exit 1).

```sh
python3 /workspace/scratch/cc27f418693a/run_mm_warp_layer_regression.py \
  /workspace/scratch/90fba16bed69/ComboShip-CrossDungeon-Restart \
  --json-include /workspace/scratch/90fba16bed69/build-deps/usr/include
```

At review time the test was kept in the bridge workspace to avoid mutating the
original owner's implementation. Native layer routing, not a test-only copy of
the adapter, is under test. The owned takeover subsequently imported and verified
both corrections in `tests/cross_dungeon/mm_boss_warp_behavior_test.py`.

## Review limits

The generation correction was outside this initial stable review.
Do not count this report as runtime acceptance or claim the two findings fixed
without behavioral regression evidence. No other confirmed blockers were reported
at this point in the review.
