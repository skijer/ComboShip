# Keaton tutorial and optional OoT mask transformations

| Field | Record |
| --- | --- |
| Baseline | `8f334101f4fe3431b8dfdf4b2bbbb7acc9d54035`, the prior full Henriko Gerudo/cuff candidate; in-game acceptance remains open. |
| Candidate | `poc/mask-keaton-optional-oot-20261009`, isolated from the previous candidate branch. |
| Scope | Shared Keaton pickup tutorial; a default-on checkbox for OoT Goron/Zora form triggers. |
| Preservation | Existing mask grants, pool rules, trade flags, sharing settings, MM forms and custom forms; all Gerudo assets remain byte-identical. |
| Configuration | OoT: **Skijer's NEI > Masks > OoT Goron/Zora Transformations** (`gMods.TransformMasks.OotGoronZora`, default `1`). Existing transformation master switch and `mm.o2r` requirement still apply. |
| Evidence | Actual-production routing fixture, complete receipt diagnostic suite, Gerudo asset/dispatcher/face fixtures, and real-header syntax checks for the changed engine/menu/receipt translation units pass. |
| Verdict | Implemented and statically verified; no full linked game build or in-game verification in this environment. |
| Recovery | Previous candidate branch and its supplied archive are unchanged; the follow-up patch applies to `8f334101`. |

## Mask behavior

| Mask | Checkbox on / missing setting | Checkbox off |
| --- | --- | --- |
| OoT Goron Mask | Existing Goron transformation | Ordinary OoT mask wear and NPC behavior |
| OoT Zora Mask | Existing Zora transformation | Ordinary OoT mask wear and NPC behavior |
| Original MM Goron/Zora/Deku/Fierce Deity | Existing transformation behavior | Same |
| OoT and MM Keaton Mask | Shared Keaton form, with its existing independent toggle | Same |
| Gerudo Mask | Existing independent Gerudo toggle | Same |
| Skull, Spooky, Truth, Bunny | Existing non-transformation behavior | Same |

The new setting only changes form lookup for the two OoT item IDs. It does not
rewrite pickups, saves, item pools or shared ownership. Existing hidden masks in
custom scenes therefore retain their current functionality by default. Switching
it off falls through to the player's existing OoT wear branch. If disabled while
already transformed, the same OoT mask can still return the active form to Link;
it cannot start or switch to a disabled form. After reverting, it wears normally.

Both Keaton copies now receive the same host-independent tutorial in English,
German and French, including MM-first pickups without a hydrated OoT donor:

- Use the assigned C-button or D-Pad button to transform and return.
- B chains three punches; keep B held after punching to charge, then release for a magic shot.
- A performs a long jump; B in the air performs a kick.
- Hold A at a wall to climb; ordinary walls and charged shots consume magic.
- Hold R to reflect projectiles.

Existing contextual A actions retain priority. The receipt converters retain
native icon ownership and encode each host's button glyphs, accents and colors;
the existing formatters handle line widths and page capacity. OoT's obsolete
Keaton "visual form / model not shipped" menu tooltip is updated to these moves.

## Verification and compile correction

`run_mask_transformation_tests.py` compiles the real form lookup and item-use
dispatcher with production item IDs/form enum. Coverage includes missing/on/off
settings, live opt-out with same-form exit, both original MM forms, Keaton's two copies and independent opt-out,
Gerudo preservation, cosmetic masks, and the existing master/resource gates.
The off-state test failed against the baseline before implementation.

`run_mm_item_receipt_tests.py` runs the complete existing receipt suite. Its
native/foreign mask fixtures now include both Keaton aliases, three locales,
cold/warm donors, correct icons/button glyphs, wrapping and save isolation. The
native MM Keaton test failed against the baseline's generic receipt before the
tutorial was added. `run_gerudo_mask_tests.py` also passes without asset edits.

Real-header syntax checks pass for `mm_player_form.cpp`, `SohMenuNEI.cpp`,
`randomizer.cpp` and MM `ItemReceiptText.cpp`. This check exposed an unrelated
pre-existing reference to the removed `func_80041DB8` collision helper in Zora
surface swimming. The same failure was reproduced from baseline `8f334101`.
The candidate uses the declared/defined `SurfaceType_GetWallFlags` helper,
retaining the existing `& 8` climbable-wall check. Existing compiler warnings
remain; these checks do not establish a full linked executable build.

## Focused runtime check

1. With the checkbox on, collect the hidden OoT Goron/Zora masks and verify the existing forms.
2. Turn it off while transformed: the same mask must still return you to Link. Then use both OoT masks: they should wear normally; test an NPC mask reaction.
3. With it off, verify original MM Goron/Zora masks still transform and Keaton/Gerudo remain available with their own toggles.
4. Collect both Keaton copies in each host, including an MM-first seed; read all tutorial pages and try the listed moves.
5. Verify the inherited Gerudo visuals/cuffs, blink and metallic swords in the same mod-stack configuration as the prior candidate.

The candidate is local and has not been pushed, merged or promoted. Keep the
accepted baseline until the relevant configuration has passed these in-game checks.
