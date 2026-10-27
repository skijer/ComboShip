# Magic Mirror: native closing and input ownership

Status: source/native-object verified candidate. No configured game was run.

| Field | Evidence |
|---|---|
| Parent | `8ab33deeb5000c6a66663dc9659d9060f702881a`, current `fix/bugfix-integration-20261020` when inspected. |
| Candidate | Isolated branch `fix/magic-mirror-choice-travel-20261021`. |
| User report | OoT holds Link still, Yes requires two A presses, No closes immediately; MM still permits movement during the choice; neither Yes returns to Lost Woods. Reported runtime evidence, not a local game reproduction. |
| Scope | Shared prompt lifecycle/input ownership, both native adapters' phase logs, focused regression coverage. |
| Preserved | Early item-press claim, native Farore cast, stump-relative arrival, solo/combined destinations, magic/respawns, inventory/forms/saves and first-press No/B. |
| Excluded | Scene packs/retargeting, Saria, audio, other active worktrees and accepted-master promotion. |

## Confirmed source defect

The old Yes branch called `Message_CloseTextbox`, immediately forced
`msgMode = MSGMODE_NONE`, and started the cast. Both native close functions leave
`msgLength` nonzero while their two-tick closing phase performs cleanup. Both
native drawing dispatchers use that length to decide whether text remains live;
their default mode branch resets it to `MSGMODE_TEXT_DISPLAYING`. The next early
mirror update then cancels the cast because a textbox is open. This source
sequence explains the extra A press and missing travel without changing routes.

Yes now records acceptance, retains movement ownership and lets native closing
clear text length/ID and perform HUD/ocarina cleanup. Casting starts afterward.
Repeated A/B/C presses during closing cannot accept twice or cancel/reopen the
committed choice. Normal No/B behavior retains its existing release path.

## Input ownership gap

The late movement predicate called `Safe`, which tests mutable collision and
interaction state. The fixture reproduces a live prompt losing ownership when
the ground bit changes and a native action clears its temporary freeze bit.
Native and Wolf readers then regain the choice box's input.

Eligibility is now evaluated by the owning early update, which closes and
releases an invalid prompt. Until cancellation, ownership depends on live
play/player, file/seed/scene/mode, item ownership and private message identity.
A physical-state change cannot itself give a live prompt's input to movement.
Replacement messages, players and contexts still invalidate ownership.

This is a reproduced source bypass. It has **not** been established as the sole
cause of the user's MM runtime movement report. Both adapters now log phase
changes (open, Yes/No/cancel, invalidation, cast start/cancel/rejection and travel
request/rejection) with scene, text ID/mode/length, player flags, ground and cast
state, so the next actual run can resolve any remaining mismatch.

Review additionally identified a concrete MM Mario bypass: its cutscene shim
deliberately ignores the generic freeze bit, and `Sm64Mario_Update` reads the
raw controller and writes movement back after the native local-pad filter. Its
existing mirror hold covered only the cast action, leaving both prompt and
accepted closing uncovered. Both hosts' existing Mario hold now checks prompt
ownership too. The extracted production entry gate failed before this change
and passes across prompt, accepted closing, cast and release. This proves the
driver's gate, not a live libsm64 render/physics session. The user's current form
was not specified; this is not an assertion that they were using Mario.

## Verification

The fixture extracts actual native `Message_GetState`, `Message_ShouldAdvance`
and `Message_CloseTextbox`, plus the existing native animation, local-pad and
Wolf input code. Audio, settings and MM's advance-policy hook are explicit
doubles. The final closing tail is a fixture boundary, not a full native
`Message_Update`/draw/game execution.

Observed red checks before their respective corrections:

- Yes failed the assertion that live native text remains in
  `MSGMODE_TEXT_CLOSING`, rather than being forced to NONE.
- Live prompt input ownership failed after a native action/ground-state change.
- The actual Mario entry gate still allowed its raw-pad/writeback path during
  the prompt before its ownership check was added.

Verified green:

- `python -B tests/magic_mirror/run_tests.py`: both native layouts under
  ASan/UBSan, native Yes/closing/cast in solo and combined modes, repeated presses,
  eligibility changes, existing cancellation/teardown, travel/arrival and state
  preservation; both actual Mario driver entry gates during prompt/closing/cast
  and after release.
- `python -B tests/magic_mirror/run_integration_tests.py`: three hook/arrival and
  MM decoder-boundary checks.
- `python -B tests/magic_mirror/run_form_tests.py`: four transformed-slot gates,
  preserving Time Gate restrictions.
- Four runners under `tests/magic_mirror_inventory`: `run_tests.py`,
  `run_action_tests.py`, `run_sync_tests.py`, `run_save_tests.py`; both hosts.
- `tests/magic_mirror/check_scene.py`, `tools/magic_mirror/test_visuals.py`,
  `tools/magic_mirror/run_draw_tests.py`: scene anchor, geometry and both native
  drawing arena boundaries.
- `tests/mm_wolf/run_host_tests.py`: real MM Wolf host/core/accessor lifecycle,
  including native damage, input and equipment behavior.
- Both complete native `MagicMirror.cpp` adapters compile as `COMBO_BUILD`
  objects with real engine/dependency headers. Local setup initially lacked
  nlohmann/SDL include paths and ImGui's internal UI type; these were resolved
  with available real headers. No dependency shim or source workaround was added.
- Both full native `z_player.c` overlays compile as `COMBO_BUILD` C objects,
  including their actual Mario drivers and the shared mirror cast implementation.
- `python -B tests/sm64_caps/run_audio_tests.py`: both hosts' cap cleanup and
  unrelated-SFX preservation pass; the existing MM controller macro-redefinition
  warnings are emitted by that fixture.

No executable was linked. The full integration gate and configured Windows
gameplay remain untested for this candidate.

## Runtime handoff and recovery

Use the same save and scene packs; record build commit, mode, form, scene and
Alt Assets setting. In both hosts, move the stick during the choice: Link should
stay still. No/B should cancel; a single A on Yes should close, cast once and
return beside the placed Lost Woods stump. Repeat in Wolf form and after scene
re-entry; also check Mario's raw-pad lock when that form is used. Combined OoT
hands off to MM Lost Woods; combined MM travels there directly. Solo modes retain
their private forest routes.

The parent remains unchanged. The published candidate branch
`fix/magic-mirror-choice-travel-20261021` provides the bounded source, tests and
verification record. Fetch that branch to restore the candidate. The earlier
recovery patch also restores the same implementation; its publication-status
note records the earlier blocked attempt. No scene archive, seed regeneration
or model/texture replacement is required.

Fresh review of the first source milestone found one Important issue (the Mario
bypass above); its bounded fix passed RED-to-GREEN and both full player overlay
compilations. No other Critical/Important findings were reported. Final source
and test inspection followed the correction. Runtime acceptance remains open.
