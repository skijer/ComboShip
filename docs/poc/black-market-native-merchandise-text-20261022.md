# Native Black Market merchandise text — 2026-10-22

Both market controllers now present the fixed merchandise item's native name,
native purchase choices and host font encoding. Saved stock remains the source
of the offer's price, tier, clear requirement and purchase status. No check fill,
progressive conversion, delivery, purchase or save policy changes are involved.

## Source evidence and correction

- MM `mm/2s2h/Rando/ActorBehavior/EnGirlA.cpp` builds native merchandise as red
  `itemName: price Rupees`, with green `I'll buy it` / `No thanks` choices. Its
  11-byte native header carries `firstItemCost`. The market now uses that form
  with `StaticData::Items.find(ComboBlackMarketItems::Id(item))->second.name`.
  It skips `RI_UNKNOWN` and falls back to the catalogue label for unresolved
  custom metadata. It deliberately does not run randomized-check `ConvertItem`.
- OoT `soh/soh/Enhancements/randomizer/Messages/MerchantMessages.cpp` uses native
  item color, yellow price and `Buy` / `Don't buy` choices, with French and German
  equivalents. The market now reads `RetrieveItem(ComboBlackMarketOotItems::Id(item))`
  names for the selected language and native color, then uses native `AutoFormat`.
- Shared `combo/BlackMarketText.h` keeps tier visible alongside sealed, owned,
  sold-out, insufficient-rupee or prerequisite status. Clear counts and prices
  come from saved stock. Result decisions remain in `Purchase`; only a matching
  successful `Yours: <catalogue label>.` substitutes the same native item name.
  The only refill is Chateau Romani, so the existing empty-bottle message remains
  distinct; this catalogue has no ammo-stack offers needing a new full-stack state.
- Merchandise uses the native no-item-icon header, rather than a cross-host
  get-item receipt icon. Native 2Ship randomizer shops directly grant merchandise;
  this correction does not add a reward cutscene or receipt delivery path.

MM metadata is converted from UTF-8/shared markup to MM font bytes before raw
choice control `0xC2` is appended. For example, `é` is MM `0x9D`, while OoT uses
`0x96`; the native A/B textures use MM `0xB0/0xB1` and OoT `0x9F/0xA0`. MM pages
are wrapped against real native font widths and limited to three rows. A wrapped
confirmation title gets a separate choice page with a blank first row, preserving
MM's native choice-arrow alignment on rows two and three.

The native OoT formatter left five rows for `Rune: Sheikah Sensor`, price `999`,
sealed at two clears, when the short trailing footer escaped its wrapping loop.
A market-only postpass parses native control arguments and inserts a native box
break before a fifth row, restores active color and preserves choice rows three
and four. `0x15` background operands are handled atomically, including operands
whose bytes equal newline, page-break or choice controls. The native global
formatter is unchanged.

## Verification and limits

Run `python3 -B tests/black_market/run_text_codec_tests.py`; the command is also
wired into `scripts/diagnostics/run_oct20_bugfix_regressions.sh`.

The ASan/UBSan runner extracts both actual `MarketText` functions and fixed-ID
policies, uses native metadata rows, the real MM message loader and full native
MM decoders, and the real OoT formatter/Text/Item implementation plus NES decoder.
Both use real native glyph loaders. Exact native renderer TWO_CHOICE/EVENT/END
branches establish terminal states; graphics, audio, unused item-icon services
and save services remain boundary doubles.

Coverage includes all 79 catalogue offers across six purchase states, saved
price `173` and maximum price `999`, native cost-header decoding, tier/clear/status
text, successful-name substitution, unavailable metadata fallback, supported
accents, button texture paths, choice placement, page row limits and unchanged
serialized market state. OoT also exercises the complete catalogue in English,
French and German. Existing native MM/OoT transaction, entry-access and stock
policy tests are separate regression checks.

The release contract runs the real native Start/Continue/Close wrappers and
native closing timer/common-reset code, then repeats welcome, browse,
confirmation, decline, purchase-result, back and leave payload sequences three
times without clearing the message context. It checks stale choice-index reset
and `TEXT_STATE_CLOSING` to `TEXT_STATE_NONE`. MM uses its full native OpenText;
OoT's OpenText boundary loads actual MarketText and executes the native common
message reset. Inactive song, notebook, heart-container and HUD side services
are excluded from the closing fixture. The separate native browsing fixture
consumes these verified terminal-state contracts to check Buy/Decline/Back/Leave,
camera/player-hide restoration, interruption cleanup and re-talk. This split
is not a configured gameplay run or a single end-to-end engine simulation.

Configured gameplay remains required for final visual acceptance: browse and
confirm short/long names, advance a wrapped title page, align both choice arrows,
check sealed/owned/sold-out/refill/rupee outcomes, and verify default/Alt font and
text rendering. MM's existing message opening path forces English; this change
does not claim new MM localization or configured game execution.
