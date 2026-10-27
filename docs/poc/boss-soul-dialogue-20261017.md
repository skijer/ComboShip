# Boss soul dialogue source checkpoint

Baseline: `43131342882972eeb4a257599ec8b061c7dc6d43`,
`integration/big-merge-20261016`. Candidate:
`feat/boss-soul-dialogue-colors-20261017`. The baseline remains unchanged.
This is a separate candidate for the upcoming cumulative merge, not a release.

## Scope and finding

The user reported a soul pickup with entirely white dialogue; its exact soul,
host and configuration were not supplied. Source tracing found that MM's
native/imported fallback could return no authored receipt and leave the queue's
plain white item name. The existing RGB hook only handles a name span that
already begins with a green control, so that hook alone cannot fix this path.
The new regression failed on a cold-donor, solo-MM soul receipt before the
implementation. This reproduces the uncovered source boundary, not the user's
unidentified runtime configuration.

Both engines now author explicit localized receipts for all 14 boss souls.
MM's native and foreign builders handle them before attempting donor lookup.
OoT's native builder and imported-MM builder use the same authored names and
colors. Only the item name is inside the color span; the article, body and
punctuation use the ordinary textbox color.

| Soul | Theme/color |
|---|---|
| Gohma | Deku Tree green |
| King Dodongo | Dodongo's Cavern red |
| Barinade | Jabu-Jabu blue |
| Phantom Ganon | Forest green |
| Volvagia | Fire red |
| Morpha | Water blue |
| Bongo Bongo | Shadow purple |
| Twinrova | Spirit yellow |
| Ganon | Ganon's Castle red |
| Odolwa | Existing Woodfall purple, `#911485` |
| Goht | Existing Snowhead green, `#0A8A2E` |
| Gyorg | Existing Great Bay blue, `#1363A5` |
| Twinmold | Existing Stone Tower yellow, `#A8B414` |
| Majora | Existing final-boss orange, `#E88015` |

The MM RGB values continue through the existing exact-name renderer hook.
No new control bytes, render-state latches, configuration settings or assets
are introduced. Soul models/flames, progression, item grants, save data, traps,
receipt-source attribution and native icon ownership remain outside the change.
The separate pickup-text audit and grotto-return checkpoints are not included
as dependencies; this patch is intended to merge alongside them.

## Verification

- `run_mm_item_receipt_tests.py`: the complete receipt suite, with new production
  caller checks for all 14 identities, three locales, solo/combined mode,
  cold/warm donor availability, native/foreign equality, preserved staged icons,
  white reset and no save/NEI writes. OoT checks cover native catalog icons,
  imported-MM icon omission, localized names and preserved color spans.
- `run_mm_item_receipt_tests.py --catalog-only --sanitizers`: address/undefined
  behavior checks through the full production MM receipt translation unit and
  its caller fixtures. Local LeakSanitizer is disabled because host process
  inspection is unavailable.
- `run_receipt_soul_color_tests.py`: both production renderer color arms retain
  the exact five MM RGB values, localized/wrapped name recognition, white reset,
  and rejection of unrelated names and intervening controls. Both renderer
  translation units also pass real-header syntax checks.
- OoT `Messages/ItemMessages.cpp` passes real-header syntax checking. The MM
  receipt translation unit is compiled and linked by the production caller
  tests. The separate MM PCH syntax command is blocked locally by missing
  `SDL2/SDL.h`; no replacement SDK stubs were added.
- Production files pass clang-format 14 and whitespace checks. Exact patch
  application is checked against the base and the separate pickup-text audit
  checkpoint before packaging.

Complete platform builds, configuration-specific runtime proof, user acceptance
and master promotion remain unverified. No CI build or publication is requested
by this checkpoint.

## Decisive game checks

Collect a native and a cross-game soul in each host, including a colored OoT
soul such as Volvagia/Morpha/Bongo Bongo and an MM soul such as Odolwa/Goht.
Check the name color, ordinary body/punctuation, native icon, completed textbox,
single award and the following ordinary textbox. Include solo-MM without an
active OoT save to exercise the previously white fallback.
