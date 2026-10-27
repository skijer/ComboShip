# Optional child-dungeon mask progression

| Field | Record |
| --- | --- |
| Baseline | `530d9077422edfb181d738ba04f0ad9f397d0168`, prior optional OoT aliases and shared Keaton tutorial candidate. |
| Candidate | `poc/child-dungeon-mask-gates-20261009`; previous candidate branch/archive preserved. |
| Scope | Default-off OoT progression for Deku, Goron, Zora and Keaton; deterministic mask grants and matching item/arrival gates. |
| Configuration | **Skijer's NEI > Masks > Child Dungeon Mask Rewards** (`gMods.TransformMasks.ChildDungeonGates`). Requires the MM mask inventory, transformation master option and `mm.o2r`; independent form options still apply. |
| Preservation | Hidden scene pickups, original item pools/seed placements, OoT trade interactions, native MM story progression, other forms and all Gerudo visuals. |
| Evidence | Actual-production routing/arrival and complete progression-module fixtures, eight dungeon-clear combinations, unsafe-seed guard, complete receipt suite, Gerudo regressions, real-header syntax checks and independent review. |
| Verdict | Implemented/static verified candidate; full game build and runtime verification remain open. |
| Recovery | Turn the setting off for normal item-based access; earned masks remain owned. Previous candidate remains available for source rollback. |

## Deterministic rewards

| Cleared OoT dungeon | Masks earned | OoT transformation entry |
| --- | --- | --- |
| Deku Tree | Original MM Deku and MM Keaton; OoT Keaton trade ownership | Deku and either Keaton copy |
| Dodongo's Cavern | Original MM Goron; OoT Goron trade ownership | MM Goron, and OoT Goron if its alias option is enabled |
| Jabu-Jabu's Belly | Original MM Zora; OoT Zora trade ownership | MM Zora, and OoT Zora if its alias option is enabled |

Use the boss blue warp to record dungeon completion. The gate reads completion
events rather than possession of Kokiri's Emerald, Goron's Ruby or Zora's Sapphire,
so shuffled stone rewards do not change the regional unlocks. It applies to both
normal and Master Quest layouts through the existing shared completion flags.

The checkbox defaults off. With it on, existing pickups remain in their original
places, but an early copy cannot enter its form in OoT until its dungeon is cleared.
Ordinary mask wear remains available. An already active form can still return to
Link if the mode is enabled mid-session. Other locked forms cannot be entered.
An MM-to-OoT arrival requesting a locked native form arrives as Link instead;
unlocked forms and Fierce Deity keep their existing arrival behavior.

The option is unavailable for loaded seeds with **Add All MM Masks to Rando**
or **Add Transformation Masks to Rando** enabled. Their solver can require an
early form to beat its own reward dungeon (for example, Goron strength to reach
a bomb bag in Dodongo's Cavern). For those seeds, the checkbox is disabled with
an explanation, and its grants/gates stay inactive even if the CVar was enabled
on a previous save. Vanilla and ordinary seeds without those logic options
support this mode. The guard reads saved seed options, not generator controls.

Fleet retains its existing active-host form sharing. When the OoT gate changes
an arriving native form to Link, that human form can be published back to the
frozen MM save. Return-to-MM form state, particularly the vanilla pre-Healing
Deku introduction, remains a targeted runtime check; MM host story/mask code
has not been changed or exercised here.

New grants use existing NEI mask inventory and child-trade ownership flags; the
currently held child trade item is preserved. Repeated player updates do not grant
duplicate items or repeat acquisition notifications. Existing completed saves catch
up automatically on the next player update after enabling the setting. Acquisitions
are announced by notifications; normal mask pickups retain the existing receipt
dialogues, including the shared Keaton controls tutorial.

No save fields, randomizer generation rules or item pools change. This is a player
option for deterministic OoT progression rather than a seed-generation policy.
Turning it off does not delete masks already earned. Cross-game sharing continues
to use its existing rules; the native MM host keeps its native transformation and
story requirements, avoiding an OoT dungeon dependency for MM-first play.

## Other forms

Gerudo, Garo, Kafei, Rito, Wolf, Pikachu and Fierce Deity retain their existing rules.
The recommended first scope is the child-dungeon trio plus Keaton. A future Gerudo
milestone would fit the fortress, and Fierce Deity would need a late-game condition;
those choices are deliberately outside this child-dungeon option.

## Verification

- `run_mask_progression_tests.py` compiles the complete production module with config, loaded-seed, event, save and notification boundaries replaced. It covers every cleared-dungeon combination, all three unsafe-seed option combinations, correct native mask IDs/trade ownership, early pickups, idempotent grants, existing-save catch-up, occupied trade slots, invalid contexts and disabling without inventory deletion.
- `run_mask_transformation_tests.py` compiles the actual lookup, form conversion, dispatcher and Fleet arrival function. It covers locked entry for both mask origins, all four matching active-form exits, blocked form switching, locked native MM arrivals, preserved default/alias/master/resource gates and unrelated forms.
- The complete receipt suite and Gerudo asset/dispatcher/face checks pass without dialogue or asset changes.
- Real-header syntax checks cover the new progression module, form router, C input wrapper and menu. Reconfigure CMake before building so its existing `mods/*.cpp` glob includes the new module.

Runtime checks: enable before clearing each child dungeon and verify the matching
mask(s), inventory page and notification after the blue warp; confirm unrelated
dungeons/stone pickups do not unlock them; test early hidden masks, live gate
activation, save/reload, MQ, shuffled dungeon rewards and turning the setting off.
Check native MM-first progression, locked/unlocked arrivals in OoT, unavailable
seed options, return-to-MM/pre-Healing form state and the unchanged Keaton/Gerudo behavior.

This candidate is not runtime accepted or promoted. No push, merge or CI build is
part of this local follow-up.
