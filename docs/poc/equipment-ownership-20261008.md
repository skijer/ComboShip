# MM equipment ownership POC — 2026-10-08

| Field | Record |
|---|---|
| Baseline | `35ce5f22`, PR #39 `bridge/combined-recovery-20261008`; reported running test-merge build `f069ea64` (`f069ea6` in the supplied startup log). Baseline has the reported equipment display defect; no runtime acceptance is inferred. |
| Candidate | Bounded equipment changes on `fix/mm-pause-inventory-20261008`, based on the preserved baseline. |
| Scope | MM vanilla and NEI equipment visibility, cursor/name eligibility, native ownership preservation, and the MM sync ownership/equipped-shield publishers when equipment borrows native slots. |
| Preservation | Existing item grants/checks, earned ownership, imported equipment titles/resource routing, Cape/Pendant identities, equipment behavior, unrelated GI/held models, inventory, audio and weather. No save reset or removal of ownership. |
| Configuration | Supplied Windows logs, save slot 0, randomized MM seed `1316389493`; previous live seed `3414832249`. Relevant pack order/Alt configuration is not established by the inspected ownership events. Resource selection is unchanged. |
| Evidence | Supplied item-grant audit events, compiled production draw/input/action and sync publisher/incoming branches, real JSON payloads, real MM-header helper and equipment-page syntax, title/resource regression tests. GPU/gameplay candidate execution is untested. |
| Verdict | Implemented and focused source/static verified. Full linked build/integration checks belong to the combined candidate; runtime acceptance and promotion are outstanding. |
| Recovery | Preserve baseline commit. Equipment changes are isolated in the paths listed below; no commits or pushes were made by this POC. |

## Evidence and cause

The pause screen's display and acquisition state differed. MM's equipment loop rendered
every populated cell on both sub-pages, using grayscale for missing ownership. The NEI
cursor was allowed to land on every cell, and both pages published item names without an
ownership check. A fresh save therefore showed an entire collection even though its A/C
action guards already refused unowned equipment. The OoT equipment draw actually draws
only owned equipment; an old MM comment incorrectly described grayscale ghost icons as
OoT parity.

The relevant events in `Fleet of Harkinian.1(5).log` distinguish old state from new seed
ownership:

| Time / log line | Observed event |
|---|---|
| `11:01:52.383`, line 1721 | MM save slot 0 deleted. |
| `11:01:55.228`, line 1722 | Oracle's old live seed `3414832249` snapshot contains `extEquipOwnedBits=0x0fdf0000` and `shieldOwned=0x18`. This is an existing-state snapshot, not a grant to the later seed. |
| `11:01:59.494`–`.528`, lines 1768–1775 | Save initialization resets NEI ownership, then initializes new seed `1316389493`. The native save template has equipment `0x11` (Kokiri Sword/Hero Shield); NEI equipment bits and shield mask are clear. |
| `11:04:24.363`, line 16711 | Explicit save-editor Cane grant changes NEI ownership from `0` to `0x00010000`. |
| `11:04:31.910` onward, lines 17418 onward | Explicit save-editor randomizer grants fill the NEI equipment bits. |

No NEI equipment ownership change is recorded for the new seed before the explicit
save-editor grants. The startup `f069ea6` identification appears at `10:56:34.735` in
`Fleet of Harkinian.2(5).log`. Audio trace spam is not ownership evidence.

Further production-branch tests exposed actual native ownership problems that became
visible when the grid stopped displaying unowned icons:

- A real starter Hero Shield was rejected because MM has no separate native shield
  ownership mask and the NEI mask had not recorded the template's native equipment.
- Master's borrowed Gilded equip value was mistaken for ownership of the Kokiri line.
- Equipping Deku added a Hylian ownership bit; swapping an OoT Mirror for an owned native
  shield could add the Ikana bit from the imported shield's borrowed native value.
- A true starter Kokiri/Hero could lose its only native ownership evidence when an
  imported or NEI piece replaced that slot.

Integration review then exposed a second ownership publisher. MM `ComputeShieldOwned`
treated every positive shield nibble as Hylian, and nibble `2` as Ikana, without checking
the active extended shield or imported skin. It wrote those inferred bits back to the
save. The real equip → sync → draw test reproduced a previously hidden Hero icon
reappearing after equipping only Divine/Kite/Ikana, Deku or OoT Mirror. A true native
Mirror by itself also acquired an unearned Hylian bit. MM `ExtractShared` likewise
exported Kokiri for every positive native sword nibble, including Master's borrowed
Gilded slot. The shield canonical publisher exported Deku as Hylian and OoT Mirror as
Ikana. These publishers require the same explicit ownership provenance as the grid.

The incoming projection had a separate preservation defect. An incoming owned Ikana
shield could overwrite a fresh Hero nibble before any ownership bit preserved the
starter. The canonical setter had the same problem, left old imported skins on native
Hero/Ikana, and ignored canonical Deku/OoT Mirror (`1`/`3`). Executing the actual setter
and exact `ApplyShared` shield block produced 17 failures across 28 additional checks
before the coordinated incoming fix.

## Candidate behavior

The MM grid now skips unowned cells before looking up resources or emitting their icon
commands. Cursor eligibility and published titles use the same production ownership
predicate. A stale cursor after changing a file or sub-page moves to a populated cell
before accepting input; reserved left-column placeholders retain their existing behavior.
Existing A/C ownership guards remain in place.

The native ownership fallback is deliberately specific:

- Kokiri ownership uses a real `FCI_KOKIRI_SWORD` receipt, real progressive Kokiri upgrade
  state, or no active extended sword plus an exact matching native Kokiri/Razor/Gilded
  equipment value and human B item. Master's borrowed Gilded slot and mismatched/stale
  B state do not prove ownership.
- Hero/Ikana ownership uses its existing mask bit, or the exact current native shield
  value with no extended shield and no imported shield skin. A Divine/Kite/Deku/OoT
  Mirror slot does not prove ownership of a different native shield.

Browsing and ownership queries do not write acquisition state. During an actual equipment
replacement, `ExtEquip_RecordNativeShieldOwnership` retains only the proven native
Hero/Ikana bit before a slot or skin borrows it. `ExtEquip_RecordNativeSwordOwnership`
retains the proven native Kokiri tier only when the native B item matches that tier and
there is no extended sword owner. Obtained and applied counters are each independently
raised to that already-materialized tier floor. Higher obtained counts and pending
deficits are preserved: obtained `3` / applied `1` / native tier `1` stays `3` / `1`.
Repeated replacement never increments an acquisition count.

The MM sync publisher now uses that native provenance for shields and Kokiri, retains
existing earned mask bits, and exports imported shield canonical IDs only when their
skin matches the borrowed native slot. Mismatched skin/slot pairs export no invented
shield. Its incoming projection preserves the genuine outgoing native shield before
overwriting its slot, clears a stale imported skin for native Hero/Ikana, and projects the
existing Deku/OoT Mirror aliases for canonical `1`/`3`.
Ownership-only deltas retain the currently equipped imported or NEI shield's borrowed
base and skin. This also protects Divine/Kite when an explicit canonical setter repeats
the already-active slot and returns early. The existing unborrowed native upgrade
projection remains; an explicit canonical setter handles equipment changes.

## Verification

`tests/mm_equipment_pause/run_ownership_tests.py` executes the production ownership
accessors, cell selectors, draw function, cursor update, A/C guards, slot writer and
native slot projection, plus MM's real sync ownership and shield export functions.
It also executes the actual `ExtractShared` Kokiri initializer, so adding a correct
predicate without wiring it into the JSON publisher cannot falsely pass.
Incoming tests execute the actual canonical setter and the exact `ApplyShared`
`shieldOwned`/`equippedShield` block using real nlohmann JSON. Native helper accessors
and direct sync equipment writes share the same `MM_EQ.equipment` storage in the fixture.
GPU/resources, audio, controller and player refresh/cleanup are
fixture boundaries; native item IDs and `NeiSaveData` layout are the real headers.

| Check | Result |
|---|---|
| Baseline `--source-ref 35ce5f22` | Red: 153 expected assertion failures across the final 373 checks. This executes baseline production branches without changing HEAD. |
| Equipment-page/native-helper candidate, before sync publisher fix | Original fixture green at 252 checks, normal and ASan/UBSan. Expanded fixture red at 327 checks / 32 publisher failures before any publisher source change. |
| Publisher candidate | Green: 327 checks, normal and ASan/UBSan. Covers native starters, individual borrowed shields, sync → page icon reappearance, imported matching/mismatched skins, exact shield canonical IDs, borrowed/stale sword state, real Kokiri receipts/upgrades and pending acquisition deficits. |
| Incoming projection, before its fix | Red: 355 checks / 17 incoming failures, after publisher checks were green. |
| Combined incoming candidate | Green: 355 checks, normal and ASan/UBSan. Includes real starter retention through direct/incoming native replacement, ownership-only projection, imported/native skin changes, incoming canonical `1`/`3`, RAM/save extended-slot release, round-trip export, idempotence and unrelated receipt preservation. |
| Borrowed-base coherence, before its fix | Red: 373 checks / 6 failures. Ownership-only Ikana deltas corrupted Deku and Divine/Kite bases, including packets that repeated the current NEI canonical equip. |
| Final combined candidate | Green: 373 checks under ASan/UBSan. Adds both imported skins and Divine/Kite across ownership-only/same-canonical deltas while preserving real new ownership. |
| Native ownership helper C syntax | Pass with actual MM headers; 14 controller-header warnings. |
| Equipment pause title/resource fixture, normal and ASan/UBSan | Pass: 63 MM and 12 OoT checks. Title/resource scenarios explicitly model acquired equipment; ownership itself is exercised by the separate production fixture. |
| Complete MM equipment-page C syntax | Pass; 15 existing header/const warnings. |
| Actual MM player unity C syntax | Passed the repository Wolf syntax gate against actual source/headers (1245 existing header/unity warnings); no standalone-file substitution. |
| MM Wolf equipment arbitration/Pendant policy | Pass. |
| `git diff --check` | Pass. |

The JSON headers are supplied by the system include path or `CPATH`; the local runs used
`/workspace/scratch/2253e076dc2a/deps-json/single_include`. No dependency file is copied
into the candidate. The final sanitizer run reported
`PASS: MM equipment ownership (373 checks, 0 failures)`.

A standalone syntax invocation of all of `extended_equipment.c` is not a valid native
build boundary: this file is included by the player unity translation unit. Such an
invocation fails on existing parent-defined animation/actor aliases and behavior globals.
The changed helpers were checked against actual MM save/equipment headers and executed
through the real slot/action paths. The coordinating task also passed syntax checking of
the actual player unity translation unit. Application object compilation/linking and
runtime acceptance remain outstanding.

## Exact changed paths

- `mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_equipment.c`
- `mm/mods/extended_equipment.c`
- `mm/mods/extended_equipment.h`
- `tests/mm_equipment_pause/run_ownership_tests.py`
- `tests/mm_equipment_pause/run_tests.py`
- `docs/poc/equipment-ownership-20261008.md`

The coordinating agent owns the accompanying publisher/incoming fixes in
`mm/2s2h/FleetShipCombo/FleetSync.cpp`; the ownership fixture executes those exact
production branches without this POC editing that reserved file.

## Remaining runtime proof

On the candidate, start a fresh randomized seed without debug grants, inspect both
equipment sub-pages, and verify only genuine starter/earned pieces appear and can be
selected. Receive one vanilla/imported piece and one NEI piece; confirm only their cells
appear, their existing names/icons load, A/C behavior works, and both remain owned after
swapping equipment, saving/reloading and switching worlds. Specifically retain a native
starter Kokiri/Hero through Master/Four/Trident and imported/NEI shield swaps. Confirm that
an incoming Ikana projection keeps a genuine starter Hero, imported skins round-trip as
Deku/OoT Mirror, and returning to a native Hero/Ikana clears the old skin. Confirm that
an existing obtained `3` / applied `1` pending sword state still receives its two missing
tiers. The standalone legacy “always available” fallback is unchanged and was not the
COMBO branch tested by this POC. No GPU appearance, runtime acceptance or master
promotion is claimed.

## Fresh bridge verification

The recovered final fixture executes 373 production checks. The preserved baseline
`35ce5f22` reproduces 153 expected ownership failures across those checks; the recovered
candidate passes all 373 with ASan/UBSan. The exact publisher/incoming paths also pass
complete MM FleetSync syntax and the canonical NEI/shared integration checks. This
adds fresh source evidence to the earlier incremental results above; it is not an
installed-game ownership or save/reload acceptance claim.
