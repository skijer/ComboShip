# OoT file-select audio handshake — 2026-10-13

Baseline: PR #41 head `a20e623720f245ce11ebe16e0edeae2f8f1b7fe6`;
cor's installed PR merge build identifies itself as `ed7ecf6`, ComboShip 0.4.0.
The supplied log records three starts ending at the first OoT frame probe,
without subsequent save-load metadata. Renaming `Randomizer` did not resolve
the reported file-select freeze.

The SoH 9.3 integration restored `Graph_ProcessGfxCommands`' wait on
`audio.cv_from_thread`. The preserved `OTRAudio_Thread` clears
`audio.processing` after synthesis or a full-reservoir guard but never
notifies that condition variable. A sleeping graphics consumer therefore
has no completion signal. The same pending request must be released when
the worker stops. This is an audio/graphics synchronization correction;
save conversion, seed handling, assets and weather are outside its scope.

The correction sends a completion notification for both synthesis and the
full-reservoir guard. Worker termination clears a pending request under the
same mutex and wakes any remaining consumer. The frame wait, PCM generation,
audio self-pump, sample cadence and reservoir policy remain intact.

## Verification

- The uncorrected parent fails all three native handshake cases: synthesis,
  capacity-limited processing and shutdown before priming. A real sleeping
  graphics consumer remains blocked until the watchdog rescues the test.
- All three pass after the fix, together with priming, refill during a graphics
  stall, bounded queue, exact sample cadence, PCM mix-ins, inactive-game mute
  and worker restart at update divisors 1, 2 and 3.
- The same complete-worker checks pass under ASan/UBSan. Local LeakSanitizer
  cannot inspect this container's `/proc` tasks under ptrace, so only local
  runs use `ASAN_OPTIONS=detect_leaks=0`; hosted sanitizer coverage remains on.
- Existing MM worker tests with diagnostics off/on and passive PCM diagnostic
  attribution checks pass. The new OoT regression is part of the hosted gate
  in both normal and sanitizer modes.
- The preserved OoT streamed-audio runtime checks pass (full-width loading,
  bank lookup, readiness, release and cache identity). Actual native SSE2
  mixers pass 132,529 OoT cases and 134,833 MM cases with zero sample mismatches.
- A fresh independent review found no blocking or important defect and
  independently repeated the three parent failures and all corrected scenarios.
- `git diff --check` passes. Full clang-format and platform compilation are
  hosted checks; this local environment has no clang-format-14 executable.

The native test executes the complete worker and the unchanged graphics
wait loop with a controlled PCM/device boundary. It cannot establish native
Windows GPU or installed-pack acceptance.

Reproduce from the repository root:

```bash
python3 -B scripts/diagnostics/run_soh_audio_worker_tests.py --baseline-ref a20e6237 --case synthesis
python3 -B scripts/diagnostics/run_soh_audio_worker_tests.py
python3 -B scripts/diagnostics/run_soh_audio_worker_tests.py --sanitize
```

The first command must fail with a sleeping-consumer diagnostic. All cases
in the next two must pass. The watchdog is test-only and is not a production
timeout workaround.

## Candidate and runtime boundary

Hosted gate and platform build results remain pending. No develop/master
promotion or installed-game acceptance is implied. Use the resulting matching
Windows package, open file select repeatedly, create/load a fresh 0.4 save,
and check an OoT/MM round trip plus music continuity. Existing mods and
versioned save backups remain intact.
