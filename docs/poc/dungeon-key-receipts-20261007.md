# Dungeon key receipts in both games

This candidate adds a narrow receipt follow-up to the morning map, junk, song,
and key surface checkpoint. It does not promote or publish the candidate.

Original integration base: `13901c677d0084e0305eec4672c0557f4434aea7`.
Restored morning source tree: `61bda4caa1542122d1806e735a58f3c36359257f`.
Local morning checkpoint: `aef730951f001e28752ce4fe221c2dcb850c7886`.

## Behavior

Known dungeon keys use SoH randomizer wording in either game:

- `You found a Forest Temple Small Key!`
- `You found the Forest Temple Boss Key!`
- `You found the Forest Temple Key Ring!`

Only the item title receives its owning dungeon's color, then the receipt resets
to white. The catalog's exact dungeon name is retained. Key counts are not
added. The shared policy recognizes the 10 OoT small keys, 6 OoT boss keys,
10 existing OoT key rings, and 8 MM small/boss keys. Skeleton keys, building
keys, and invented dungeon/kind combinations keep their existing behavior.

| OoT dungeon or location | Receipt color |
| --- | --- |
| Forest Temple, Chest Game | Green |
| Fire Temple, Ganon's Castle | Red |
| Water Temple | Blue |
| Spirit Temple, Training Ground, Gerudo Fortress | Yellow |
| Shadow Temple, Bottom of the Well | Pink/purple text control |

| MM dungeon | Receipt color |
| --- | --- |
| Woodfall | Pink |
| Snowhead | Green |
| Great Bay | Blue |
| Stone Tower | Yellow |

OoT colors match its current item catalog, including the Chest Game's default
green. MM uses the standard text controls corresponding to its native dungeon
key palette. Text colors do not replace cosmetic RGB values on key models.

The policy is used by MM's native and foreign receipt composition, OoT's native
custom item receipts, its MM foreign sentinel, and its read-only donor export.
The bridge also registers the Chest Game's native `0xF3` receipt. Shuffled
single-key pickups retain their vanilla `GI_DOOR_KEY` grant entry, so that hook
uses the explicit Chest Game catalog identity and original small-key icon.
It requires an active randomizer single-key seed and a valid matching current
get-item entry. Off, key-pack, non-randomizer, stale-entry and unrelated textbox
paths keep their existing behavior. Grants and models are unchanged.
OoT retains catalog translations and articles; imported MM rows without an
article receive the small/boss key article fallback. Existing native icon
selection and foreign icon safety paths remain in place. MM composes key text
locally even when the OoT donor is unavailable. Receipt formatting does not
read key counts or change grants, inventory, rendering, or geometry.

## Verification

The regression fixture checks all 34 exact key identities and color control
bytes on MM native/foreign paths with the donor available and unavailable,
map information enabled and disabled, and first/repeat inventory counts. It
checks source attribution and unchanged save/inventory bytes. The OoT fixture
executes the production native custom item builder, read-only export, and all
8 MM foreign key receipts with the actual native formatter and text codec.
Existing receipt, queue/grant/trap, junk, story-song, map/compass, seed/settings,
and altar regressions are included in the same suite. Skeleton key exclusions
are explicit. Empty trailing native formatting lines are ignored when comparing
the title; color bytes and title text must still match exactly.

Focused commands, with the normal repository dependencies available:

```sh
python3 -B scripts/diagnostics/run_mm_item_receipt_tests.py
python3 -B scripts/diagnostics/run_chest_game_key_receipt_tests.py
python3 -B scripts/diagnostics/run_receipt_syntax_tests.py \
  mm/2s2h/Rando/ItemReceiptText.cpp \
  soh/soh/Enhancements/randomizer/Messages/ItemMessages.cpp
```

The delivery includes the failing pre-change Forest key regression log, final
focused verification, source copies, a combined patch against the original
base, and a receipt-only follow-up patch against the restored morning tree.
Patch application is checked in temporary indexes against the candidate tree.
Morning assets, rollback archives, and geometry certificates are preserved.

The bridge's Chest Game regression executes the production hook registration
and callback with the real catalog's vanilla entry identity. It fails before
the missing hook is added, then checks the named green receipt and native icon
on repeat invocations, together with the guarded default paths. The complete
receipt suite also runs this regression. Native localized title/custom-icon
runtime coverage still requires the targeted replay below; direct builder
fixtures alone do not demonstrate every engine dispatch or localized icon.

Full application build and in-game acceptance remain pending. Before promotion,
collect OoT and MM small/boss keys in both games, including repeat pickups, and
verify their titles/colors, source pages, original icons/models, and one grant
per reward. Existing unshuffled vanilla hook policy is not broadened here.
