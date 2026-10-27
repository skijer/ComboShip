# Solo OoT Black Market button glyph color candidate

Baseline: `bb6f927e` in the shared isolated candidate worktree. This is a source
candidate; no master promotion or configured game runtime acceptance is claimed.

## Confirmed cause

`BlackMarketText::Oot` emitted bare `0x9F` / `0xA0` glyphs on Welcome and Browse.
In `soh/src/code/z_message_PAL.c`, `Message_DrawText` initializes ordinary text
RGB to white, applies message cosmetics, and changes palette only when it sees
`MESSAGE_COLOR` (around lines 1400–1435). The generic character branch (around
1700) sends both buttons through `Message_DrawTextChar`. That routine loads an
I4 intensity texture and emits the current text RGB as primitive color (lines
276–290). OoT has no automatic per-button text tint. A restart cannot supply
missing color commands.

MM is different: `Message_DrawTextNES` explicitly saves current RGB, applies
`sColorsButtonsNES` for `MESSAGE_BTN_A` through `MESSAGE_CONTROL_PAD`, draws the
glyph, and restores RGB (`mm/src/code/z_message_nes.c:932–945`). MM text does not
need the OoT markup correction.

## Bounded correction

Only OoT Welcome and Browse gain `%b` immediately before A and `%g` immediately
before B, with `%w` immediately after each glyph. Native blue and adjustable
green message colors supply the tint through the existing formatter, palette
setter and message cosmetic preferences. HUD A/B cosmetics are distinct from
message palette cosmetics; the native ordinary OoT glyph renderer does not
consult HUD button preferences.

Item-name colors, yellow prices, tier/status colors, choice commands, quick-text
and EVENT endings retain their original construction. MM streams are unchanged.
No camera, layout, purchase, stock or receipt behavior is modified here.

## Verification

`run_oot_button_glyph_tests.py` executes the production pure OoT formatter and
page limiter. It tracks emitted color commands and sends the glyph color
through production `Message_SetTextColor`, `Cosmetics_MaybeSetTextColor` and
`Message_DrawTextChar`, asserting the resulting `G_SETPRIMCOLOR` packets. Font
texture content and preference storage are test boundaries; this is not the
complete native message dispatcher or an in-game capture.

Before the correction, it failed with:

```
FAIL OoT A glyph prim RGB=ffffff expected=505aff (custom=0)
```

After the correction, its ASan/UBSan checks pass for Welcome/Browse in EN/DE/FR,
native A blue (`505aff`), native adjustable B green (`46ff50` with fixture native
registers), distinct custom message palette values, default-color restoration,
item-name/price colors, EVENT endings and confirm-choice controls.

The existing `run_keeper_dialogue_oot_formatter_tests.py` also passes the real
formatter's width, pagination, choice-row and EVENT checks. The native MM text
codec passes catalogue metadata, prices/status/tier, font, choices, EVENT and
Start/Continue/Close/repeat-talk checks. With the existing dependencies on the
include path, the full OoT native text codec also passes its fixed localized
metadata, catalogue, prices/status/tier, font glyph, choice/EVENT and
Start/Continue/Close/repeat-talk checks under ASan/UBSan.

Run the focused regression with the normal dependency include path configured:

```sh
python tests/black_market/run_oot_button_glyph_tests.py
python tests/black_market/run_keeper_dialogue_oot_formatter_tests.py
python tests/black_market/run_text_codec_tests.py --game both
```

In the investigation workspace the focused tests require the existing
`/workspace/scratch/e483f65698b2/deps/json/include` on `CPATH`. The full OoT codec
also requires the existing spdlog, imgui, SDL and supplemental include directories
from that dependency root. An initial combined codec invocation passed MM but
stopped compiling OoT because `spdlog/spdlog.h` was absent from the include path;
this was an environment limitation rather than a runtime/test assertion.

## Remaining runtime gate

Boot the exact candidate in Solo OoT, enter/re-enter the Black Market, inspect A
and B on Welcome and Browse, and confirm surrounding labels/item/price colors
and the Buy/Don't buy cursor. Also inspect any user-selected message cosmetics.
Native packet/formatter checks do not establish configured visual correctness
or restart behavior. No game boot, full executable build, commit or publication
was performed for this workstream.
