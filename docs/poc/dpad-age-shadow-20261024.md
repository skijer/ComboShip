# D-pad custom item shadow loss across OoT ages

Status: implemented and native-tested candidate. The supplied save confirms
the reported OoT Down icon is an unresolved custom marker with zero shadow.
Configured Windows gameplay and runtime acceptance remain pending.

## Baseline and preservation

The inspected source baseline is local `86387591`, whose tree matches public
`5f8966fa` (`545292a667083fb835ccd86b078e3aac7a484cd1`). This bounded correction
adds to the regional working candidate; it does not promote an accepted master.
The earlier physical U/D/L/R transfer correction and invalid-zero departure
validation remain intact. No item ownership, native slot, or intentional stick
assignment is erased. MM's native form storage and transfer policies are unchanged.

## Confirmed defect

OoT's native and randomizer `Inventory_SwapAgeEquipment` routines copy the eight
raw button bytes and seven inventory-slot bytes into `childEquips`/`adultEquips`.
The nonprogression age handoff in `SwitchAge.cpp` copies the same native structure.
All three paths omitted `ship.extButtons.items`, the single parallel array that
contains the full custom item IDs for `ITEM_EXT_BUTTON` markers.

The native reproduction equips custom item `0x0223` on child Down, swaps to
adult, equips a native bow (clearing the live shadow), then restores child.
Before correction the actual functions produce Down raw `0xd1`, slot `0xff`,
shadow `0x0000`, effective item `0x0000`. The old getter interpreted that zero
as a Deku stick. Replacing the adult button with a different custom item instead
restored the adult's full ID behind the child's marker. This affects C buttons
and all four D-pad directions, independently of the prior wire-order problem.

## Correction

- Append independent child/adult shadow arrays at the end of
  `ShipSaveContextData`; earlier offsets and the native `ItemEquips` layout are
  unchanged. `SaveContext.ship` is its final member. Legacy binary conversion
  continues to use the separate `SaveContext_v0` type.
- Store the outgoing age's custom shadows before each native/rando/nonprogression
  handoff, and load the matching incoming shadows when its raw buttons restore.
  An uninitialized nonprogression target keeps the current buttons and shadows.
- Persist the arrays as optional `extButtonItems` alongside the existing child
  and adult equip blocks, using the existing base version 4. Missing keys default
  to zero; normal new-save/load initialization clears all three arrays. Existing
  active `equips.extButtonItems` remains compatible.
- Read an unresolved custom marker as `ITEM_NONE`, without mutating the save.
  The real C/D HUD gates skip both icon and ammo rendering for that state.
  Raw native sticks, including valid `SLOT_STICK` bindings, keep their behavior.
- Apply the same passive empty rule to a raw native zero with `SLOT_NONE` on
  C/D. Native inventory maps `ITEM_STICK` to `SLOT_STICK`, and Kaleido's native
  equip completion writes the item together with that slot. Both the effective
  getter/HUD and native `C_BTN_ITEM`/`DPAD_ITEM` input macros now reject the
  invalid pair. B's separate zero behavior, valid `SLOT_STICK`, native bottles,
  raw custom markers and the existing disabled-status offsets remain intact.

An old inactive marker whose full ID was already lost cannot be reconstructed
safely. It remains stored, reads as empty, and can be equipped again explicitly.
There is no speculative migration of native sticks or unrelated equipped items.

## Verification

`tests/combo_button_transfer/run_age_tests.py --source-ref 86387591` fails all
eight pre-fix scenario groups for the expected lost-ID/phantom-stick assertions.
The same runner passes with the candidate under ASan/UBSan in standalone and
combined builds. It compiles complete native save types and executes the actual
native, randomizer/Combo and nonprogression handoffs, accessor/reset functions,
SaveManager traversal/templates and button save/load blocks. The tests cover
each C/D button, different child/adult custom IDs, intervening native equips,
legitimate sticks, B-sword policy, uninitialized nonprogression target retention,
active/inactive JSON round trips, missing-key defaults, passive getters and the
production HUD guards. Graphics commands are inert in the HUD fixture; this is
not a full renderer or gameplay test. The actual accessor translation unit also
passes standalone/combined syntax compilation.

The raw-native-zero follow-up was separately RED before its getter guard in
both builds (`raw C/D zero with SLOT_NONE must not read as stick`), then RED
again with the getter fixed but native input unchanged (`invalid native zero
press must be empty`). The corrected eight groups execute the actual
`Player_GetItemOnButton` and native Kaleido equip-completion block for every C/D
button, checking invalid zero is empty on press, a valid native stick still
works, B raw zero is preserved, full custom IDs remain behind the native inert
marker, bottles remain native, and each button's disabled status still applies.
The getter/HUD/input checks also compare the complete save before and after to
prove validation is passive. All current macro consumers use constant/local
indices and rvalue reads; none requires an lvalue or supplies side-effect arguments.

The expanded 26 native transfer scenarios pass with ASan/UBSan: directions,
empty transfers, native/custom items, unsupported local policy, old untagged
payloads, malformed metadata, MM forms and invalid-zero departure validation.
Both actual arrival functions explicitly retain legitimate local native C/Down
sticks and full custom IDs when the peer payload is all empty/keep-local.
Both hosts' existing travel-item save/reset regressions pass.

The pre-existing pedestal fixture also needed its actual shadow helper bodies
and modeled fields added. Its baseline runner/fixture at `86387591` independently
failed on missing cumulative renderer boundaries (`DinFireSword_CopyHandModel`,
`DinFireSword_DrawHand`, `MmForm_IsNativeTransformationPlaying`, open-hand tables).
Only fixture declarations/boundaries were adapted, preserving its fire-draw
observer and ordinary-form cases. Its flattened metadata fixture now also
supplies raw JSON to the existing sword-element validator. No production
renderer changes are part of this correction.
All 13 existing pedestal scenario groups pass after these fixture corrections.

## Current user save diagnosis

The supplied screenshot directly shows a physical Down stick in OoT. The current
log starts at line 36109 and identifies build `da79ceb` (October 24 23:26:10).
Its item-grant audit captures ownership, but never raw equipped buttons, slots
or custom shadows, so the log alone could not identify the persisted state.

The subsequently supplied save was inspected privately with its original
preserved unchanged. Its active OoT Down is an unresolved `ITEM_EXT_BUTTON`
marker with `SLOT_NONE` and zero full-ID shadow; the child cache repeats the
unresolved marker, while adult Down is already empty. This confirms that the
reported stick appearance is the invalid custom-shadow representation described
above, rather than a deliberately equipped native stick with `SLOT_STICK`.
The save does not identify the lost custom ID or when that state was created.

Both MM owl/new-cycle records also contain an assigned Down Ocarina. That is a
valid MM native item/slot pair, not a stick or an unresolved zero. Cor explicitly
states Down was never intentionally assigned, so a separate corrected copy clears
only active/child OoT Down and MM Down in both resume records, preventing a later
cache restore or paired transfer from reintroducing an unwanted Down assignment.
All other directions, buttons, fields and progression are preserved; no private
save, save hash, seed, notes or progress dump belongs in this repository.
This user-directed save repair is separate from the general source validation,
which continues to preserve legitimate native sticks and local-only assignments.

The full native young-Epona save fixture also needs the actual
`ExtButton_ResetSaveData` body and its declaration when it extracts SaveManager
initialization. Those dependencies are now included. Its existing UBSan save
and disk-roundtrip checks pass, with added stale-shadow checks confirming all
24 active/child/adult shadow cells clear in all five new-file modes. No save
types or production initialization behavior were replaced for this fixture.

The native shared-slot UI fixture had encoded native Grace `0xA1` through the
extended-item setter, creating a marker with an unsupported low-ID shadow. No
supported native equip, sync, travel or wheel writer creates that pair. The
fixture now executes the real native equip completion's final write and keeps
the invalid-marker rejection check separately. It verifies C1 and physical
D-pad Up/Down in both wheel directions, including raw item, slot, shadow and
icon refreshes, plus valid native sticks and invalid zero/empty-slot input.
Shared-slot core/UI/sync/save checks and the eight age groups pass; getter and
input validation are unchanged.

Configured runtime testing should equip different custom items on the same
direction in both ages, travel/swap, save/reload, and verify each remains correct.
