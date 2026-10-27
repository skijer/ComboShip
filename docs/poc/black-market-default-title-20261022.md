# Default native Black Market title — 2026-10-22

The default market now supplies its own original location card in both hosts.
It uses the native OoT placement and timing: logical size 192×24, center
`(160, 120)`, delay 20 and duration 80. The native IA8 sprite includes white
lettering, a black keyline and a transparent perimeter; no additional border
draw or replacement title renderer is needed.

## Cause and resource contract

The immutable 125-resource native geometry bundle intentionally omitted
`custom/skiejer_market/title`. Preparation previously added only the interior
surfaces, while readiness tests explicitly accepted the missing optional art.
Both presenters probed that path and silently skipped a missing texture. The
OoT presentation was also behind the active-stock draw gate.

`market_title.apply` now adds the default title after the surface pass and
before the native exit-door pass. Both prepared host graphs contain 141
resources; the geometry bundle remains unchanged. The title is a native
texture-version-zero record with format IA8 and 4,608 pixel bytes. The generated
`BlackMarketNativeResources.h` includes its path in both core readiness sets.
Existing resource overrides keep their normal load-order behavior.

The original and native PNGs, packed sprite and provenance live in
`tools/black_market/assets/title/`. Preparation uses Python's standard library
to audit every listed size, SHA-256 and CRC32, PNG framing/chunk CRCs and native
dimensions, safe local provenance paths, raw size and alpha framing before
writing outputs. `encode_title.py` is an optional authoring tool; normal
preparation and CMake do not invoke an image library or font renderer.

| Frozen asset | SHA-256 | CRC32 |
|---|---|---|
| `black-market-title.ia8` | `5991dcef52bd5817b06ceb142e3274ff278f3e740b6a40558c03a3992a688fac` | `113380eb` |
| `black-market-wordmark.png` | `ad9d78ba0c1bdd65648b2594ecea1b2fdcc1be31daf446d2b336e8ce428e6cde` | `09dea5fc` |

## Native presentation

Each interior scene visit arms one presentation independently of stock or the
shopkeeper conversation. Missing resources or an explicit hide/veto consume
only that visit's request. Unsafe gameplay, native cutscenes and an occupied
title context defer it until the native context can accept the card. Scene
destruction clears it. Native/Alt resource availability is checked before use.

MM uses the existing `ComboPlaceTitleCard_Show` helper and native
`TitleCard_InitBossName`, update and draw functions. OoT initializes its native
title context with the same card geometry/timers and retains its native
`OnPresentTitleCard` notification and cosmetic fade colors. A synchronous,
restoring custom-presentation scope allows this custom card through OoT's
randomizer default-hide policy. Explicit Hide Title Cards and other hook vetoes
still win. This scope never reads or changes a travel request, entrance, save
flag or progression byte, and restores a prior nested scope exactly.

Other MM destinations retain their own native scene-title messages. The
generic final-destination policy was additionally checked against every one of
the 113 native scene-table rows for portal, market and solo-forest requests:
74 titled rows select their own title ID and 39 untitled rows remain untitled,
including unset entrance-card bits and positioned arrivals. This does not add
cards to ordinary room changes or overwrite native cutscene ownership.

## Verification

Run after sourcing the native test dependency environment:

```sh
python3 -B tests/native_market/run_title_tests.py
python3 -B tests/native_market/run_title_draw_tests.py
python3 -B -m unittest discover -s tests/native_market -p 'test_*.py'
python3 -B tests/native_market/run_runtime_readiness_tests.py
python3 -B tests/lost_woods_entrance/run_mm_title_card_tests.py --sanitize
python3 -B tests/lost_woods_entrance/run_oot_title_card_tests.py --sanitize
python3 -B tests/lost_woods_entrance/run_title_card_tests.py --sanitize
python3 -B tests/lost_woods_entrance/run_tests.py
```

The presenter fixture executes both actual presenter bodies, native MM helper,
native update functions and OoT's actual hide-hook branch. It compares all 120
frames of delay, duration, alpha and intensity against native OoT, and checks
defer/retry, missing/native/Alt resources, stock independence, one visit,
explicit hide/default/veto combinations and byte-for-byte save preservation
with both prior travel-request values and nested scope values.

The draw fixture executes the production presenters, native update/draw,
native graph pointer framing and actual GBI macros under ASan/UBSan. It checks
the exact resource pointer, IA8 tile format, 192×24 extents, native centered
rectangles, per-frame primitive color/alpha, no draw at zero alpha and all
three host language values. Wrong-centering and wrong-texture-format mutations
fail. GPU setup, resource loading, interpolation recording and default cosmetic
settings remain fixture boundaries.

Asset tests check the frozen border's black pixels with nonzero alpha, solid
white faces and transparent outer perimeter, reject corrupt provenance/PNG/raw
input before output, verify both prepared graphs and ensure preparation has no
PIL/font dependency. Native title, asset and generic MM policy commands also run
in the early regression script. Existing OoT market transactions and the
opening-forest title renderer remain green.

The complete entrance suite also passes after the shared older boot fixture was
given the transient custom-title state and a native-boot reset assertion. Its
six whole-source renderer objects compile with the actual per-host CMake CVar
prefixes, retaining the native interpolation ABI checks. The final run is
recorded in `market-title-full-entrance-final.log` beside the candidate worktree.

These source, asset and native-fixture results do not establish configured
gameplay acceptance. Remaining checks are entry and re-entry in both hosts,
title appearance with native and Alt packs, Hide Title Cards, a delayed entry
cutscene and an unrelated native MM destination after leaving the market.
