# MM grotto return arrival POC

Baseline: `integration/big-merge-20261016` at `43131342882972eeb4a257599ec8b061c7dc6d43`.
Candidate: `fix/mm-grotto-return-arrival-20261017`.

The supplied 21.077-second clip repeatedly returns from an OoT grave interior to
an MM grotto opening, then enters the hole again. Returns are visible near 2, 8,
14 and 20 seconds. The clip does not identify the executable commit or settings.
Clip SHA-256: `785edc77399de0272bc04e57f4c69f0523a5208b48691aa1d2ba4d3bb092289c`.

The MM departure adapter persists `pauseSaveEntrance`. On an explicit cross-game
return, the grotto adapter restores respawn mode 4 and player start mode GROTTO.
Player initialization then sees the retained pause-save marker and replaces that
start mode with stationary mode D. This is a source-confirmed conflict consistent
with the clip; the installed build and controller input remain unverified.

Scope: let consumed cross-game arrivals and recovery checkpoints own their native
spawn behavior. Clear stale pause-save/OWL-arrival markers only after a composed
arrival or recovery was actually consumed. Ordinary native save resumes retain
their stationary-start behavior.

Preserve seed permutations, entrance identities, position/yaw/room descriptors,
seasonal fallback floors, native respawn flags, day/time, progression, fades,
solo modes, and all OoT routing. No asset or archive changes.

Verification: extend the production MM adapter fixture through the real native
player-start override, including all MM hole returns, ordinary resume controls,
and no-arrival cases. Run the cross-dungeon suite and focused sanitizer checks.
Runtime acceptance remains pending: repeat the shown grave/grotto round trip and
verify Link launches clear of the hole, can stand still outside, and can re-enter.

## Evidence

- The baseline MM adapter fixture passed before adding the startup regression.
- The extended fixture failed on the expected assertion: the composed grotto
  return must retain GROTTO start mode instead of stationary mode D.
- After the correction, all 24 MM hole-return descriptors and the ordinary
  native-resume/no-arrival controls passed.
- The same fixture passed with address and undefined-behavior sanitizers
  (`ASAN_OPTIONS=detect_leaks=0`).
- An independent source review found no blocking issues. Additional repeated-trip
  and explicit boss-recovery marker assertions were optional coverage suggestions.
- All 45 cross-dungeon commands completed: 18 C++ fixtures and 27 Python runners,
  including transition fades, boss recovery, shop returns and ordinary-resume
  controls. The initial aggregate runner stopped on missing headers/fixture files
  in the fresh sparse checkout. Those dependencies were restored and remaining
  commands resumed on the unchanged production source; no behavioral failure
  remained. Logs were inspected for each completed segment.

Commands: `python tests/cross_dungeon/run_tests.py`, then its remaining Python
runners starting at `transition_fade_behavior_test.py` and
`boss_recovery_behavior_test.py` after the missing checkout files were restored.
Focused sanitizer command:
`ASAN_OPTIONS=detect_leaks=0 python tests/cross_dungeon/mm_adapter_behavior_test.py --sanitize`.
The temporary include path supplied nlohmann/json 3.11.3; no dependency files
were added to the repository.

No full application build or candidate gameplay was run. This remains an isolated
source candidate; it is not a runtime-accepted or promoted baseline.
