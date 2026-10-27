# File-select audio handshake implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Release the OoT graphics thread after every completed audio request.

**Architecture:** Preserve the 9.3 graphics wait and the fork's audio self-pump. Restore the worker's completion notification for synthesized and capacity-limited requests, and release a pending waiter when the worker stops.

**Tech Stack:** C++20 condition variables; Python diagnostic runner; GitHub Actions.

**Spec:** `docs/poc/file-select-audio-handshake-20261013.md`

## Global constraints

- Parent: PR #41 head `a20e623720f245ce11ebe16e0edeae2f8f1b7fe6`.
- Preserve saves, seeds, archives, seasonal rendering, PCM mixing, reservoir limits and self-pump cadence.
- Publish only within the already-authorized ComboShip candidate; no develop promotion.

## Review focus

- First-frame consumer is already asleep before audio completes.
- A full backend still completes the request without advancing synthesis.
- Shutdown releases a pending consumer even before the first synthesis.
- Self-pump refills during a graphics stall at update divisors 1/2/3.
- Mix-ins, inactive-game muting and worker restart retain their behavior.

### Task 1: Restore and protect audio completion

**Files:** Modify `soh/soh/OTRGlobals.cpp` and `.github/workflows/build-artifacts.yml`; create `tests/audio_mixer/soh_worker_test.cpp` and `scripts/diagnostics/run_soh_audio_worker_tests.py`.

**Interfaces:** Consume the complete production `OTRAudio_Thread` and graphics wait loop; retain all existing engine interfaces.

- [x] Write a native worker test with a real sleeping consumer and a watchdog that fails if it must wake the consumer.
- [x] Run synthesized, full-reservoir and stopped-worker cases on the parent; require deadlock failures.
- [x] Restore notifications after clearing `audio.processing`, including worker termination.
- [ ] Run normal and ASan/UBSan tests, preserved audio checks, formatting and whitespace checks.
- [x] Add the regression to the existing CI gate; record results and obtain a focused review.
- [ ] Publish with a branch-head lease and monitor hosted gate/platform builds.
