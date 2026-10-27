# Dungeon reward pool regression

`Fleet of Harkinian(9).log` records `RAND_INF_ZR_WONDER_NEAR_CUCCO_3`
(0x714) immediately before a cross grant of Zora's Sapphire (fc 226),
followed by native OoT item 0x6e at 02:01:26 on build cb69b63.

ComboShip deliberately returns false from `FleetCombo_UnifiedPoolActive`:
it uses the launcher's fill, not NEI's older unified fill. That lets MM's
standalone `RO_SHUFFLE_OOT_QUEST` block run. Its nine OoT reward additions
therefore bypass OoT's confined placement and enter the free cross-game
pool. With quest items enabled, the unfixed native generator reproduces
this leak for all nine rewards, including plentiful duplicates.

The fix excludes those nine additions only in `COMBO_BUILD`. OoT continues
to supply and place its genuine rewards according to its configured policy.
Other quest items, standalone MM, NEI items and native MM remains settings
retain their existing behavior.

Run:

```sh
python3 tests/dungeon_rewards/run_tests.py --json-include /usr/include
```

The test compiles the complete production `GeneratePools.cpp` and the
unchanged production `FleetCombo_UnifiedPoolActive` function in both build
modes. Real game/rando types are used. Config/starting-item inputs, a small
native check graph, item classifications, RNG and inactive standalone peer
state are fixtures; GUI-only ShipUtils declarations are excluded.
Sixteen configurations per build cover quest off/on, normal/plentiful,
MM remains off/on and starting with Sapphire/Stone of Agony. The baseline
fails 68 ComboShip reward assertions; standalone passes. This is not a
full-game or complete-seed generation test.

Existing seed placements are serialized. This fix applies to newly
generated seeds and does not rewrite saves or remove acquired rewards.
The supplied log has no seed settings or spoiler: inspect that seed's
spoiler/save separately before attempting any repair. In-game acceptance
and promotion of this candidate remain pending.
