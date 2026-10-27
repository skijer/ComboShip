# Saria event-arrow softlock — October 21, 2026

| Field | Record |
|---|---|
| Source baseline | PR #48 head `8ab33deeb5000c6a66663dc9659d9060f702881a`, tree `a6a6319e451515c26f27dc4a9a99f2c62be3629b` |
| Candidate / recovery | `fix/saria-dialogue-advance-20261021`, a separate child of the baseline |
| User report | Both MM Lost Woods Saria and the Termina Field song giver display text and an arrow, ignore normal advance, and respond to C buttons with error sounds |
| Scope | Handle the existing MM event-ended Saria dialogue in the shared actor update |
| Preservation | Text/IDs, decoder and font guards, models/poses/placement/particles, scene archives, song reward/receipt/save ownership, once-per-file and pending-event deduplication, other actors and OoT source |
| Verdict | Native regression reproduced the ignored advance; corrected fixture and complete actor compilation pass. Configured gameplay and user acceptance remain untested. |

## Evidence and cause

The supplied `Fleet of Harkinian(20261021-062241).log` has SHA-256
`403a797b1b1f656d19eb98aa0a9100cc1fdcf3f21d9f2b9bc8d0c7ae54e2534b`.
Its latest startup records executable merge `03c928e`; it contains accumulated
earlier sessions too. It has no MM Saria message-state or input trace, so it
cannot identify the precise moment of either reported softlock. The attachment
is unchanged and is not included in the source publication.

All three custom Saria messages (`0x8F28`, `0x8F29`, `0x8F2A`) end with MM
control `0x19`. After rendering, this produces `TEXT_STATE_EVENT`. The native
`Message_Update` deliberately returns for this actor-owned ending. The actor
must call `Message_ShouldAdvance` and `Message_CloseTextbox`, as ordinary native
MM NPCs do. `ComboSaria_Update` instead only waited for `TEXT_STATE_CLOSING` or
`TEXT_STATE_NONE`. Neither side could initiate closure. Both reported Sarias
use this shared update path; the active talk actor is exempt from actor freezing.

The preceding decoder correction remains necessary and is retained. It allows
these private high IDs to decode as normal MM text. Its regression checked the
event terminator and then explicitly called `Message_CloseTextbox` from the
test. That bypassed the missing player-to-actor advance handoff.

## Correction and verification

The shared talking branch now handles an acknowledged `TEXT_STATE_EVENT` by
calling the native close function. Its existing closing/none branch runs later,
so teaching still queues the song only when the text is closing. Final item
delivery, receipt, persistence and deduplication code are unchanged.

- Before the production change, the expanded native sanitizer fixture failed:
  `FAIL Saria text 0x8F28 ignores advance button 0x8000 at its event arrow`.
- Afterward, `python -B tests/custom_messages/run_decoder_tests.py` passes all
  nine combinations of the three actual Saria messages and A/B/C-Up. No press or
  C-Left leaves the event open; acknowledgement starts closing without an early
  reward; the teaching message queues one file-bound item on the closing tick;
  bridge dialogue and already-taught dialogue queue none. Existing decoder,
  credits, font-boundary and duplicate/claimed/non-gift checks also pass.
- The fixture links actual native message C units, uses the production custom
  hook/formatter and extracted Saria talking/queue bodies, and calls the real
  `Message_GetState`, `Message_ShouldAdvance` and `Message_CloseTextbox` under
  ASan/UBSan. Graphics/audio, unrelated hooks and final item delivery are
  boundaries. It establishes completed-event and final-closing states directly;
  it does not render a box or execute a game session.
- `scripts/diagnostics/compile_forest_actors.py --source
  mm/2s2h/Enhancements/Gameplay/ComboSaria.cpp --output OUTPUT` compiles the full
  modified actor with real headers in Combo mode. The existing macro-redefinition
  and comma-subscript warnings remain. This is object compilation, not a linked
  executable build. Object SHA-256:
  `5815827398d5583578465d932f263e8630670cc19cbf2d5fa34d67107e4b6568`.
- Clang-format 14.0.6 verifies the modified production file, and whitespace checks
  pass. Independent code review found no actionable issues.
- All nine commands in the surrounding October 20 preflight have passing
  results, including the retained Black Market arrival test. Its forest group
  initially stopped because this fresh environment had no CMake executable.
  After making CMake available, that group passed all 31 tests with two
  optional-input skips. No production change was needed for that setup issue.

## Recovery and next game check

Restore the baseline from `8ab33dee`, or fetch the named candidate branch for the
bounded correction. Apply only this candidate's source/test/document diff when
combining it with other pending bug fixes. No mod archive or new scene export is
required; an executable rebuild containing this source correction is required.
The active cumulative PR and accepted masters are unchanged by this isolated
candidate.

In the next matching build, talk to Lost Woods Saria and close her box with A.
Then talk to the Termina Field teacher: close the teaching box, receive Saria's
Song once, close its receipt, and confirm movement resumes. Talk again and
reload the save to check the already-taught dialogue and persistence. Record the
actual save, form, archive order and Alt Assets setting with that evidence.
