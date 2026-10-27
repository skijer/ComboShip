# C-Up tutorial reconstruction — 2026-10-10

| Field | Record |
|---|---|
| Baseline | Recovered PR41 `3abeff897e38`; cumulative candidate parent at implementation time `142e82537c9838f56cb73458cda35d2b21098791` after the autumn, sword and tunic recovery patches. |
| Candidate | Combine-into-#41 working candidate. This is a new bounded reconstruction; former commit `5a0b6ba3` and its patch were unavailable and have not been reproduced byte-for-byte. |
| Scope | Existing C-Up shortcut, selected mask/trade identity, active rods/runes/cane entries, equipment cell identity, form-page routing and message lifecycle. |
| Preservation | A cycling/equipping outside descriptions, ownership, grants, randomizer checks, gameplay actions and other agents' asset work. No promotion to develop; the parent integration process publishes the cumulative PR41 candidate separately. |
| Evidence | Failing production lookup/display fixtures before implementation; failing actual MM equipment-cursor actions before the description guard; normal and ASan/UBSan tests; actual-header syntax checks. |
| Verdict | Implemented and static/compiler verified. Installed game/controller/archive configuration remains untested; no runtime acceptance or master promotion. |

## Confirmed causes

- Short pause-only tables did not reuse the newer shared tool, mask and magic tutorial builders. Both hosts now use those existing builders for Phantom Hourglass, Shadow Crystal, Gerudo/Keaton and the active rod/rune. The lookup snapshots OoT's tutorial before opening the textbox.
- MM's OoT mask and donor trade items expose placeholder IDs. Their selected wheel indices now resolve the actual mask/trade identity instead of describing the placeholder's borrowed vanilla ID.
- OoT's unified trade wheel includes 23 items, while its old pause lookup only covered the 11 adult-trade items. The lookup now uses the production trade catalogue, while preserving the Pendant's existing combat tutorial.
- Equipment icons alias multiple actual cells. C-Up now resolves MM's vanilla equipment by row/column, with progressive sword identity, and keeps Cape/Champion and Pendant/Climb contexts distinct in both hosts. All existing extended equipment and boss-remains descriptions retain their host routes.
- OoT's longsword cursor distinguishes the fragile Giant's Knife, broken knife, durable Biggoron Sword and Great Fairy upgrade. The tutorial now follows that selected identity. Its Longshot retains the ordinary tutorial until the Ultrashot ownership flag is set. MM's native tunic/boot tutorials describe its actual fireproof, fast-swim/electric-immunity and reduced-knockback behavior.
- MM's form selector returned before the grid's C-Up handler. It now opens the selected Link/Mario/Pikachu controls without equipping the form. OoT's existing C-Up handler receives those form tutorials through its normal lookup.
- Encoded MM bodies contain zero-valued color bytes. They now remain length-bearing `std::string` values through display, use the existing receipt reflow helper, and receive an explicit terminator. The template ID is validated against the current message table rather than cached across table changes.
- The MM equipment cursor continues to be called while its description is open. A narrow `itemDescriptionOn` guard gives the textbox ownership of A/L/C-Up and cursor input until it closes, then restores the existing equip/cycle behavior.

MM Byrna remains cosmetic in the current gameplay implementation; OoT teaches its existing Kinsect controls. MM's Tri Rod returns pending skill 6 and has no echo-cast implementation; its description says so. OoT retains its implemented echo scanner/grid controls. The Somaria platform and the Tri Rod are separate entries.

## Verification

The initial lookup/display regression fixture failed 10 MM and 8 OoT checks on the pre-edit sources. Further focused tests reproduced a missing encoded terminator, excess MM line offsets, and three actual cursor failures during an open description (A equipped a form, L switched the page, and C-Up reopened the textbox). Review fixtures then reproduced 11 missing trade routes, three knife/Ultrashot mismatches and three MM native-equipment text mismatches before their corrections.

Final commands, with `/workspace/scratch/6f91fd2cf849/diagnostic-env.sh` loaded:

```sh
python3 tests/pause_tutorials/run_tests.py
python3 tests/pause_tutorials/run_tests.py --sanitize
python3 scripts/diagnostics/run_receipt_syntax_tests.py \
  mm/2s2h/CustomMessage/PauseItemDescriptions.cpp \
  soh/soh/Enhancements/custom-message/PauseItemDescriptions.cpp \
  mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_item.c \
  mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_equipment.c
```

Both normal and ASan/UBSan runs passed **205 checks**: MM lookup/display 83, OoT lookup/hook 87, actual MM equipment cursor 35. Coverage includes all 23 selected trade identities, fragile/broken/full Biggoron and Great Fairy swords, and ordinary/upgraded Longshot flags. The runner invokes the cursor fixture automatically and inherits the diagnostic environment's header paths. Leak detection is disabled because of this host's ptrace limitation; address and undefined-behavior checks remain enabled.

All four actual-header translation-unit checks passed. They report existing MM header warnings (15/20/15); the OoT translation unit reports zero compiler warnings. Adjacent MM equipment ownership passed 373 checks, and the existing MM pause inventory suite passed after the description guard. The parent integration process runs the full canonical suite separately.

## Remaining proof

This does not claim the historical unavailable 159/477-check sweep. Shared receipt builders retain their existing translations; bespoke cane/equipment/form/trade text follows the existing English pause-description convention. Full linked application and installed-stack behavior must be verified by the parent build/runtime process. The focused in-game check is: open each affected C-Up tutorial, page through it, close it, then confirm A cycling/equipping and L navigation resume with the same selected item and ownership.
