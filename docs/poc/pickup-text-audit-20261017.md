# Pickup text audit after the large merge

Parent: `43131342882972eeb4a257599ec8b061c7dc6d43`
(`integration/big-merge-20261016`). This candidate retains the separate
[foreign Master Sword receipt correction](master-sword-shop-receipt-freeze-20261017.md).
The parent and accepted game assets are unchanged.

## Confirmed findings and corrections

| Finding | Evidence | Candidate behavior |
|---|---|---|
| Self-matching localized replacements never finish | Exact foreign Master Sword lvalue call timed out before correction | Both overloads resume after inserted text; empty searches do nothing |
| MM map text can fit in pixels but exceed its font texture bank | Real normal/inverted Stone Tower source names produce more than 120 glyphs on one page; native `Font.charBuf` has 120 slots per bank | Reflow pages when the glyph budget is reached, independently of text scale; all source names and colors remain |
| Chateau Romani and the save continuation share `0x9214` | Actual hook registration/dispatch replaces the choice prompt; Better Save's state 4 waits for a choice | Chateau uses named `TEXT_CHATEAU_ROMANI = 0x9217` in the cow reward and callback; save IDs stay unchanged |
| Unsupported translated glyphs reach the native width table | Safe production-formatter checks reject Latin-1 bytes and the UTF-8 lead byte of `œ`; native drawing indexes a 144-entry width table | Eight catalog entries use supported `oe` spellings/UTF-8 accents; French Chateau also uses a supported UTF-8 accent |
| Extended item IDs enter an enum limited to 0–255 | Full production formatter/catalog UBSan run rejects value 544 as `ItemID` | Both registry declarations store `int32_t`; receipt callers narrow to the existing byte operand before constructing `ItemID` |
| Icon operands are interpreted as text markup or page breaks | All four season receipts lose their `#` icon operand; a native `0x04` operand receives an extra icon | Encode text with a safe leading icon, restore only that prefix, and insert page icons while skipping encoded command operands |

The translated catalog entries are Shield of Ikana, Spirit Breastplate,
Sheikah Sensor, Don Gero's Mask, Circus Leader's Mask, Couple's Mask,
MM Mask of Truth, and the Clock Town Owl Statue. The Sensor entry affected
both German and French. No pickup effects, ownership, placement, shop prices,
save layout, models, audio, or animation data are changed.

## Verification and boundaries

- `run_message_replace_tests.py`, normal and ASan/UBSan: exact production
  overload bodies, implicit localized conversion, self-containing/unchanged
  labels, fallback, adjacent deletions, and empty searches. Original timed out;
  candidate passes.
- `run_chateau_receipt_tests.py`, normal and ASan/UBSan: actual callback,
  formatting, cow/registration IDs, conditional registration, deferred removal,
  and hook dispatch; both registration orders and re-registration; all three
  locales; exactly one handler; own receipt body and retained save choice.
  Original safely failed; candidate passes. Font staging and hook metadata are
  explicit engine seams.
- `run_pickup_catalog_tests.py`: compiles the full production `CustomMessage`
  class against real headers, reads current custom/NEI catalogs and shared
  equipment/bottle/magic lessons, and checks 939 locale receipts. Checks include
  termination, glyph range, decoded/page capacities, three wand rules, and
  preservation of the emitted icon byte. Another 18 cases cover markup/control
  icon operands, page icons, and a distinct authored embedded icon. The glyph,
  enum, and icon checks failed before correction. Final combined runs are
  recorded with the candidate.
- `run_mm_item_receipt_tests.py`: native/foreign queues and grants, traps,
  cold/warm donor, three locales, bottle contents, masks, keys, magic,
  source attribution, map/compass information, solo/combined gates, real donor
  bridge, seed/settings round-trip, typography, and rendering boundaries.
  The layout fixture adds all 462 ordered distinct pairs of 22 real entrance
  source names and 120/121/241-glyph boundaries. Safe counters validate capacity;
  no test writes beyond a font allocation.
- Real-header syntax checks pass for both native message decoders/renderers,
  `CustomMessageManager.cpp`, `ItemMessages.cpp`, and the cow actor. The included
  NEI registry passes with its actual `global.h` include context; it is not a
  standalone CMake source. Existing native compiler warnings remain.
- Full `randomizer.cpp` syntax checking is blocked locally by unavailable
  `SDL2/SDL.h`; its catalog and field declarations are compiled by the catalog
  regression. A complete Windows/Linux game build and game runtime remain
  unverified. Local LeakSanitizer is disabled because process/thread inspection
  is unavailable; address and undefined-behavior checks remain enabled.

CI runs the fast replacement, Chateau, and sanitized layout checks before
dependency installation. The existing dependency-backed regression group also
runs the catalog sweep in normal and sanitizer modes. These are candidate gates,
not evidence of a completed platform build or user acceptance.

## Decisive game checks before release acceptance

1. In the reported seed, buy the MM-owned Master Sword from Goron City Shop
   Item 6; finish the receipt, regain control, and verify a single award.
2. In MM, collect a Stone Tower map with long, different normal/inverted source
   names; finish every page and verify both entrance names.
3. With Better Save enabled, save in an overworld and answer the continuation
   choice. Separately receive Chateau from Link's house cow while wearing the
   Romani mask, including French.
4. Sample the corrected translated pickups and a Slate rune in both hosts;
   finish the receipt, confirm the icon and award, and open the next textbox.

Status: implemented and regression-verified candidate; complete platform build,
configuration-specific game proof, acceptance, and master promotion remain open.
