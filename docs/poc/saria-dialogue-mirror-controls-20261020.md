# Saria custom dialogue and Magic Mirror prompt controls

Status: implemented candidate with native source regression checks. Configured
gameplay, visual acceptance and accepted-master promotion remain unproven.

| Field | Record |
|---|---|
| Evidence baseline | User's `450b293646475d613c12151aaed802f82db73508` Windows build; preserved diagnosis in `FINDINGS.txt`, `QUEST-GIVER-SARIA-FINDINGS.txt`, and `MIRROR-AND-NEW-LOG-FINDINGS.txt` from the prior investigation. |
| Candidate | `fix/bugfix-integration-20261020`, initially inspected at `a2b774ec0`; integration subsequently includes elemental revision `6b88d8116`. This record describes the bounded dialogue/control change. |
| Hypotheses | Custom MM messages must select the MM decoder regardless of their private ID. A live Mirror prompt must own gameplay movement independently of a native action's temporary freeze bit. |
| Scope | MM message classification and glyph-write boundary; shared Mirror prompt state, native local input and Wolf input policies in both engines; source regression fixtures. |
| Preservation | Real credits, ordinary messages, Saria teaching-close queue policy, raw choice input, B/No/Yes, cast motion/item/fade, cancellation, arrival, magic/respawn/save behavior, original-game build guards. |
| Excluded | Saria rendering/placement/shimmer, market readiness/entry policy, elemental effects, asset/scene/seed changes and publishing workflow. |
| Runtime configuration | Not executed here. Required packs/order, Alt setting, save and scene state must be recorded with the next actual game check. No seed reset is required for this source correction. |
| Recovery | Revert only this bounded source/test change from the integration candidate; preserve the supplied archives and prior accepted baseline. Root integration owns durable source checkpoints and publication. |

## Dialogue correction

The custom hook correctly loads normal MM-format text and declines the native
table. `Message_OpenText` nevertheless marked IDs above `0x4E20` as credits.
That sent Saria's `0x8F28`–`0x8F2A` messages, and the `0x8F10`–`0x8F13` market
range, to a decoder with different end/event controls. The preserved investigation
reproduced an illegal write beyond the 200-byte decoded buffer; the user's dump
also contained font-resource path bytes in actor links.

Credits classification now requires a native table load. The MM text hook,
formatting, event controls and Saria grant implementation are unchanged.
`Font_LoadCharNES` additionally rejects an invalid bank, an offset that cannot
hold a complete glyph slot, and a resource path larger than one slot. This is a
bounded guard at the observed corrupting writer, not a claim that every malformed
message/control expansion is now bounded.

`python -B tests/custom_messages/run_decoder_tests.py` compiles and links the
actual `z_message.c`, `z_message_nes.c`, `z_message_staff.c` and `z_kanfont.c` with
native MM layouts under AddressSanitizer and UndefinedBehaviorSanitizer. It uses
the actual Saria hook lambda, custom formatter/loader helpers, closing branch and
queue function from current source. Graphics/audio, unrelated hooks and final
item delivery are boundary doubles.

Observed red checks before correction:

- `FAIL custom MM text 0x8F28 selected the credits decoder`.
- The separate invalid-font-offset test reached UBSan's `index -1 out of bounds`
  in the production writer before its bank/slot guard was added.

Green coverage includes all three actual Saria messages terminating on MM event
`0x19`; normal custom routing for every market ID with a representative formatted
market payload; boundary/high custom IDs; both real native decoders around the
credits threshold; native table-backed high IDs still using credits; valid first
and last glyph slots in both banks; invalid banks/offsets/glyphs and oversized
paths; and the actual teaching-close queue policy (no early, duplicate, claimed
or non-gift queue). The queue fixture does not execute the final randomized song
grant, receipt, persistence or a configured actor encounter.

## Mirror correction

The prompt formerly installed its freeze bit only when opening. Native player
action changes clear that bit while the prompt may still be live. The prompt now
reacquires its owned bit each update and stops horizontal motion. A separate
predicate binds ownership to the live play/player, file/seed/scene/mode, private
message and existing safety checks.

Both native player paths clear their local gameplay pad while this predicate is
true. The raw message pad is preserved. Both Wolf readers independently report
blocked movement while the prompt owns the player, including when an action
clears the flag after the early update; this also reaches Wolf's existing blocked
procedure handling rather than allowing an existing dash to continue.
The B/No/Yes input tick remains owned after the choice box is released, using a
play/player-bound token cleared by the next early input update. This prevents
the closing press or stick from becoming a gameplay action in the same frame.

`python -B tests/magic_mirror/run_tests.py` uses both native engine layouts,
the actual local-pad selection branches, both actual Wolf readers, shared prompt/
cast/arrival code and native animation advancement under ASan/UBSan. Rendering,
unrelated feature gates and native idle/action setup have explicit fixture
boundaries. The test first failed at each relevant ownership boundary: lost
prompt flag, restored local pad and unblocked Wolf reader. Each correction made
the next check reachable; the finished fixture passes in both engines.
Review then identified that the original fixture allowed B to reach gameplay on
the cancellation frame. The corrected expectation failed before the closing-tick
token was added, and passes for B, No and Yes with raw input still unchanged.

Coverage includes unchanged raw Yes/No/A/B input; movement/actions blocked after
native action cleanup; closing-frame suppression and human/Wolf control restored
on the update after B/No; existing
No/Yes confirmation, cast/put-away/interrupt/timeout, fade hold and arrival;
preserved pre-existing flags and cleanup of reacquired flags; message/file/seed/
mode/safety invalidation; live player replacement and post-arena teardown. The
existing inventory/form fixtures cover selectors, ownership, saves and Time Gate
restrictions. No screenshot, player visibility or in-game travel claim follows
from these fixtures.

## Source verification

| Check | Result |
|---|---|
| `python -B tests/custom_messages/run_decoder_tests.py` | Pass, real native C message units plus the extracted production Saria/custom-message paths, ASan/UBSan. |
| `python -B tests/magic_mirror/run_tests.py` | Pass, both engine layouts, local-pad/Wolf readers and complete existing cast/arrival fixture, ASan/UBSan. |
| `python -B tests/magic_mirror/run_integration_tests.py` | Pass, three native hook/decoder-boundary checks. |
| `python -B tests/magic_mirror/run_form_tests.py` | Pass, Mirror access for the four transformed slots with Time Gate restrictions retained. |
| `python -B tests/magic_mirror_inventory/run_tests.py` | Pass, both hosts. |
| `python -B tests/magic_mirror_inventory/run_action_tests.py` | Pass, both hosts. |
| `python -B tests/magic_mirror_inventory/run_sync_tests.py` | Pass, both hosts. |
| `python -B tests/magic_mirror_inventory/run_save_tests.py` | Pass, both hosts. |
| Native object compilation | Pass in original and `COMBO_BUILD` modes: MM `z_message.c`/`z_kanfont.c`, and both hosts' player overlay, Wolf form and Mirror adapter (16 configurations). The final closing-tick change is also exercised by the native layout fixture. |

The local C++ checks used restored real nlohmann/json, spdlog, ImGui and SDL
headers through include paths. No full game executable was linked. Root
integration owns the wider project gate and configured Windows build. There is
no single repository-wide test command; the commands above are the affected
feature suites, with the wider integration gate reported separately.

## Next decisive game checks

1. With the same save and accepted scene pack, talk to forest guidance Saria;
   close/reopen the dialogue and re-enter the scene with a stable HUD/actor list.
2. In Termina Field, complete teaching, receive the song once, save/reload and
   check already-taught dialogue. Also check real credits and market dialogue.
3. Open the Mirror while human and Wolf, hold/move the stick and use Yes/No/B;
   confirm stable position/model, completed cast, correct arrival and restored
   movement. Repeat a cancel, an interrupted cast and a scene re-entry.
4. Record exact candidate build, game mode, age/form, packs/order and Alt state.
   These checks remain runtime-unproven until observed in that configuration.
