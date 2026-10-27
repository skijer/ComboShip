# OoT Magic Mirror receipt: native newline softlock

| Field | Record |
|---|---|
| Baseline | ComboShip `bb6f927e` in the isolated October 24 repair worktree; user reports Solo OoT pickup softlock. |
| Candidate | This bounded receipt text change; no master promotion or runtime acceptance. |
| Scope | Correct OoT receipt line separators and exercise actual formatting and native message/item completion branches. |
| Preservation | Preserve Solo OoT's intentionally hosted MM forest/shop, Mirror ownership/grant/save timing, GI entry/model, choice prompt, MM receipt, travel and arrival. |
| Configuration | Reported Solo OoT, forest entrance `0x618`; configured runtime packs, load order, Alt Assets and player form were not independently reproduced. |
| Recovery | Revert only this candidate's OoT receipt separator edit and new focused regression/record. Baseline remains recoverable in git. |

## Evidence and cause

The supplied `Fleet of Harkinian(20261025-035718).log` records the OoT native path entering `0x618` at 22:43:29, displaying receipt `0x3feb` at 22:43:37, console reset at 22:43:44, and reloading `0x618` at 22:43:45. The user reports the receipt softlock and ownership after reset. Those logs identify the receipt stage; they do not alone prove which player/message fields were stuck.

The receipt body contained C++ `\n` characters. OoT's native `MESSAGE_PERSISTENT` is byte `0x0A`, which is also C++ `\n`; its native newline is `0x01`. `CustomMessage::AutoFormatString` translates `&` or recognizes native `0x01`, and does not convert C++ `\n`. Consequently native text drawing interprets the first apparent line break as persistent text, sets `TEXTBOX_ENDTYPE_PERSISTENT`, and stops. The native `MSGMODE_TEXT_DONE` branch explicitly excludes persistent text from the A-button close policy. The item receipt function waits for `TEXT_STATE_CLOSING`, so that item action cannot finish through ordinary receipt input.

The focused native-branch regression reproduced this before implementation: `FAIL Mirror receipt refuses A: native mode=2 endType=2 (persistent=2)`. This is source-level behavioral reproduction, not a launched game reproduction. After replacing the receipt separators with the formatter's `&`, the same path reaches ordinary native END and closes on A. The test then executes real `Message_GetState` and the real item receipt closing/finished gates: timer 2 keeps the item active; timer 1 clears the custom item entry/draw state, and the native item completion gate returns finished.

`GiveItemEntryWithoutActor` accepts the custom entry with `GI_MAX`; source inspection did not establish that ID as a softlock cause. The adapter already grants/saves ownership immediately after offering the receipt. That explains ownership and the saved forest location after reset; this patch intentionally preserves that behavior. No GI-ID swap or grant-timing change is included.

## Implementation and verification

- `soh/soh/Enhancements/Gameplay/MagicMirror.cpp`: use OoT formatter line separators in the receipt; preserve all text and the existing page break.
- `tests/magic_mirror/run_receipt_tests.py`: compile/run the actual adapter text loader, exact formatter/width logic, native page/end/persistent branches, native A-button close policy, and native receipt completion branches. Draw/audio and irrelevant ASCII substitution boundaries are explicit doubles. It also exercises the same loader's choice prompt and verifies its choice control remains.

| Check | Result |
|---|---|
| New focused receipt regression, before fix | RED: native persistent end type refuses A. |
| New focused receipt regression, after fix | PASS with C++20 warnings as errors and AddressSanitizer/UndefinedBehaviorSanitizer. |
| `python3 tests/magic_mirror/run_integration_tests.py` | PASS: 3 tests. |
| `python3 tests/magic_mirror/run_form_tests.py` | PASS: four transformed-form slot gates; Time Gate restrictions preserved. |
| `python3 -m unittest discover -s tests/magic_mirror -p 'test_*.py'` | PASS: 4 tests. |
| `python3 tests/magic_mirror/run_tests.py` | PASS for both hosts after supplying the existing JSON/spdlog dependency include roots; the initial missing-header prerequisite was resolved. |
| Native project compilation/full executable | Complete OoT Mirror and both shop source units compile against real headers. Full executable linking/boot not run. |
| Configured runtime/user acceptance | Untested. |

The next decisive runtime check is a fresh unowned Solo OoT file in the same forest configuration: collect the Mirror, advance both receipt pages, dismiss the final page, verify control/camera restoration and one owned Mirror after save/reload. An existing owned file cannot retest first acquisition without restoring an unowned save. Test the preserved MM/combined receipt path as an additional configuration check if required for release. This candidate is implemented and focused source-behavior verified, with no claim of configured runtime proof or master acceptance.
