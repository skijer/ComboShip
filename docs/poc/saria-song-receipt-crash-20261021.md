# Saria's Song receipt overrun — October 21, 2026

| Field | Record |
|---|---|
| Baseline | Published morning source `525b3160660225a06700470fb39d9d4948834414`, tree `1ec44483e21dd0820ea7df066ba6ffc71b51249d`; tested Windows merge `8303491aa4ffebdebb59984a3b9cb949374131e0` |
| Candidate | `fix/saria-song-receipt-crash-20261021` |
| Report | Saria's teaching box now closes and the item appears, followed by a crash |
| Configuration | MM Termina Field, room 0, native save slot 1, seed `1466159340`; active Alt setting is not recorded in the supplied log |
| Scope | Finish the fixed Saria reward receipt with the same helper as other direct song gifts |
| Preservation | Dialogue advance, item identity/grant, claim/save ownership, duplicate guards, models, poses, shimmer, placements, archives and other entrances |
| Verdict | Decoder failure reproduced; corrected production receipt/decoder, actor regressions and actor object compilation pass. Configured game runtime remains untested. No accepted master promotion. |

## Inspected crash evidence

The unchanged attachments are identified by SHA-256:

- `Fleet of Harkinian(20261021-225810).log`:
  `a70309d4f17ccf874a1b5d09eb33b19a5abf3e07888ec449f2a915eb44e70945`.
- `Fleet of Harkinian-crash(7).dmp`:
  `2cf0e195bac106fadf3266e0d4f1de1bd81f7fedf4d8a4067b5212d2eb6106ee`.

The log contains an older Black Market crash as well as the latest Saria run.
At its recorded `17:57:03.815`, MM grants `RI_SONG_SARIA` (319), projects the
shared song to OoT, and sets the MM quest bit. Save-write completes at
`17:57:03.855`. At `17:57:04.694` the latest run crashes reading address zero.
These are the log's own timestamp labels, not a timezone conversion.

The corresponding packaged `2ship.dll` has SHA-256
`eb942e5b8ac18b1cb7e57b95c984c7ec0c966b75683fc5c2f8be031dd2820a08`.
Its CodeView RSDS identity `df4a0eb6-6306-4d22-9e66-bb10ecc03061`, age 3,
matches the module recorded in the minidump. Disassembly maps fault RVA
`0x6779A5` to the dereference of
`interfaceCtx->doActionSegment[DO_ACTION_SEG_A].mainTex` in `Interface_Draw`,
before rendering the A-button HUD segment. The dump captures the Saria
receipt's text immediately before the interface context, followed by zeros
including its do-action and item-icon pointers. This establishes a text/state
overrun rather than an invalid song-model display-list reference.

## Cause and bounded correction

`ApplyItemReceiptText` produces an already-encoded receipt body and disables
automatic formatting. The body deliberately has no message terminator:
`AppendReceiptSource` owns the ordinary MM END (`0xBF`), even when its source
suffix is empty. Normal queued items and all five direct story-song gifts use
both calls. `GiveSariaSong` omitted the second call. The custom-message loader
therefore did not append END, and the native decoder continued past the
receipt into neighboring state.

The sole production edit is `Rando::AppendReceiptSource(entry, "")` after the
existing receipt builder. Inventory, claim persistence and all drawing code
are unchanged. This source requires a rebuilt executable; no replacement mod
archive is needed.

## Verification

- The original custom-message suite passes before expanding its coverage.
- The new fixture links the complete production `ItemReceiptText.cpp`, real
  Saria catalog row, icon selector and grant body, production custom-message
  loader and four native message/font C units. With the original Saria grant it reports a
  72-byte English body ending in `0x2E`, then UBSan fails at
  `z_message_nes.c:1082`: index 200 outside `char[200]`.
- After the correction, the same fixture passes all 12 combinations of
  English/French/German, cutscene/overhead delivery, and Combo/solo-MM mode.
  Every decoded receipt uses the native staged `0xF5` song icon and 240-pixel
  wrapping, ends in `0xBF`, preserves the complete sentinel-filled
  interface context, reaches a closable message state, saves one claim, rejects
  a different file slot, and prevents a duplicate grant. Existing Saria
  dialogue-arrow, decoder/credits routing and font-boundary checks pass too.
  Wrapped pages are advanced with the native acknowledged-page buffer step;
  each page and the final END are decoded and checked.
- Inventory grant, file-write implementation, hook registration and texture
  loading are explicit service boundaries in this fixture. Graphics/audio
  and the frame-driven receipt close/return to movement are not executed.
- `scripts/diagnostics/run_forest_actor_tests.py` passes with its real receipt
  finalizer added to the existing lifecycle fixture. Retains pose, resource,
  blink, spawn, permanent-claim and JSON-save tests.
- The complete affected `ComboSaria.cpp` compiles as a Combo native object
  against real headers. Existing controller-macro and comma-subscript warnings
  remain; this is not a full executable link.
- `scripts/diagnostics/run_mm_item_receipt_tests.py --catalog-only` passes the
  native/foreign receipt, icon, codec/layout, source-attribution and live
  catalog checks, including all five direct story-song gifts in the three
  supported Latin locales with cutscene delivery on and off.

Reproduce the decoder fixture with
`python -B tests/custom_messages/run_decoder_tests.py`. It is already part of
the October 20 bugfix preflight. External nlohmann-json/spdlog headers must be
available, as in the other native receipt fixtures.

## Cutscene-actor dialogue coverage

cor additionally requested coverage for cutscene-actor dialogue before
publication. MM Saria's placed cutscene actor and Termina song giver both
dispatch through `ComboSaria_Update`; the native decoder regression retains
all three actual Saria messages (`0x8F28`–`0x8F2A`) and A/B/C-Up closure,
non-advance input, deferred reward and no-reward ordinary dialogue controls.

The broader native cutscene-actor check now exercises all 23 talkable catalog
entries through the real `EnViewerStatic_OfferTalk` implementation in both solo
SoH and Combo configurations: offer, accept, wait without advance, acknowledge
an EVENT, close, and preserve the authored animation/pose, XYZ and yaw. The
existing model-only/no-talk, normal closing and pedestal-priority controller
checks are retained. The private dialogue catalog checks all eight entries,
including the monkey, across four language settings and loader failure guards.

`tests/custom_messages/run_cutscene_actor_tests.py` runs catalog/controller
checks under ASan/UBSan and the complete viewer translation unit with its
existing CMake section-GC linkage boundary. Message callbacks and cross-boss
progression are controlled services in this viewer fixture; it does not render
or run the full OoT decoder. The MM decoder/receipt regression does run native
message C units. Both runners are in the October bugfix preflight.

## Runtime handoff and recovery

In a build containing this candidate, use an unclaimed file to talk to the
Termina Field teacher, close her teaching box, receive the song, close its
receipt and resume movement. Talk again and reload to confirm the permanent
claim. The supplied run shows the song was saved before its crash, so that
same saved file may already show the taught dialogue; retain its backup rather
than treating that as a missed grant.

The changed source, tests, exact baseline, patch and verification evidence are
preserved in `docs/checkpoints/saria-song-receipt-and-cutscene-source-20261021.zip`.
The original receipt-only checkpoint is retained separately.
Apply the source diff to the named morning baseline or fetch the candidate
branch; no game assets or user crash files are redistributed. The separate
Black Market collision correction must be integrated on its own evidence.
