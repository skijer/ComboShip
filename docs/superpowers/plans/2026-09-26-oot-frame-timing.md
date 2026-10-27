# OoT Frame Timing Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans to implement this plan inline.

**Goal:** Identify which part of OoT's frame consumes time after archive reduction and lower resolution produced no reported improvement.

**Architecture:** An allocation-free, main-thread timing recorder collects scoped durations and emits one aggregate log per second. C-compatible entry points cover game code; C++ scopes cover resource and rendering boundaries. This isolated diagnostic candidate enables collection by default.

**Tech Stack:** C/C++, steady_clock, existing spdlog and JSON dependencies.

**Spec:** The bounded design below.

## Design and preservation

- Parent: `020a50354ae531a2d8fcee51aeedd0ec22b2941d`, matching the source behind the reported recovery build. This is a reproduction baseline, not an accepted performance baseline.
- Candidate: `poc/oot-frame-timing-20260926`; original integration and all archives remain unchanged.
- Preserve gameplay, assets, resource ownership/cache policy, weather, audio production, randomizer and rendering order.
- Record tick construction, Play update/draw, frame hooks, cosmetics, MM asset lookup, audio mutex acquisition, window events, interpolation, and the combined graphics/presentation call.
- Expose optional elapsed stages from Fast3dWindow (ready/setup/commands/GUI/present) to distinguish command processing from presentation waits. Preserve its existing call order and count a presented frame only when its existing return value is true.
- Report actual scope semantics: nested measurements overlap; graphics/presentation includes pacing and GPU waits and is not a GPU timer. Logic ticks are distinct from rendered frames.
- Reset aggregation across scene/room/age/Alt/pause/target changes. Disabled collection must discard partial data, avoid clock reads and ignore stale scopes.
- Log at most one aggregate per second. Log formatting is outside the measured tick. Scene transitions discard incomplete windows instead of mixing configurations.
- Clone the application's existing logger and asynchronous writer with an independent Info level, so saved Warn/Off levels cannot suppress diagnostics. Preserve normal game logging settings. Emit an explicit startup record and enable-state changes; samples include schema and event fields.
- `gDeveloperTools.FrameTimingProbe` defaults to 1 only in this diagnostic candidate; setting it to 0 disables collection.
- Runtime request: reproduce the slowdown for about 20 seconds in Kakariko or Graveyard, then provide the log. No performance improvement is claimed.

## Global constraints

- No edits to saves, user settings, archives, scene geometry, or the integration branch.
- No claim of Windows compilation or in-game performance without corresponding evidence.

## Review focus

- Disabled and menu-only frames must not leak old timing data.
- Context changes and stale scopes must not mix measurements.
- Interpolated render calls must not be mistaken for game logic ticks.
- Nested spans must not be added together as exclusive CPU time.
- No locks or allocations on the collector's per-span path.

## Task 1: Timing recorder and call-site integration

**Files:** `soh/soh/Enhancements/debugger/FrameTimingProbe.{h,cpp}`, `soh/tests/frame_timing_probe_test.cpp`, `scripts/diagnostics/run_frame_timing_probe_tests.py`, and narrow scopes in existing OoT frame/cosmetics/MM loader call sites.

**Interfaces:** C `FrameTiming_BeginFrame`, `FrameTiming_BeginSpan`, `FrameTiming_EndSpan`, `FrameTiming_EndFrame`; C++ `FrameTiming::Recorder` accepts explicit nanosecond timestamps for deterministic tests, and `FrameTiming::Scope` supplies runtime timestamps.

- [x] Write tests for aggregate timing/counts, disabled reset, context reset, stale scopes, and nested timing semantics.
- [x] Run tests before implementation; expect missing recorder interface.
- [x] Implement recorder and logging adapter; rerun focused tests, expect all pass.
- [x] Add scopes at the existing boundaries without reordering work; compile C header and run touched-component tests.
- [ ] Review the complete diff, commit the isolated candidate, and obtain build evidence before describing a build as ready.

## Evidence ledger

- User reports no improvement from removing the large extra archive stack or reducing internal resolution. Exact FPS remains unmeasured.
- Existing PreludeLoadProbe is Lost Woods-specific and suppresses most ordinary slow frames; it does not resolve this question.
- Local baseline is clean; remote recovery branch still points to the parent above.
- Ruling: use a sibling worktree and implement inline to preserve the existing checkout and avoid unnecessary permission rounds for authorized local diagnosis.
- Recorder tests were observed failing before implementation, then passing. Renderer-duration accounting was also added through a failing test.
- ASan/UBSan run requires `ASAN_OPTIONS=detect_leaks=0` here because LeakSanitizer cannot inspect `/proc` in this runtime; the address/undefined-behavior checks themselves run normally.
- Independent review found one important issue: a context or enable change within the tick that closes a report window could emit mislabeled data. Fixed by checking all tags and enablement again at tick end. The regression test first failed on the mixed-window assertion, then passed with the fix. Six behavioral tests and C bridge compilation pass.
- Generated Python bytecode was removed from the index after review; it is not part of the candidate.
- The production logging adapter passes a C++20 syntax compile against real nlohmann JSON and spdlog headers. The existing custom-cosmetics regression suite passes. Full Windows build and in-game behavior remain unverified until CI and the user capture.
- Review limitations: rendering call order was inspected, not run against a GPU; no runtime overhead or root-cause claim is made. This candidate must not be promoted as a performance fix.
- User requires useful output without discovering missing logging parameters after a long build. An integration test using the production adapter and real asynchronous rotating-file sink reproduced the original missing-output failure with normal logging Off. After isolating the diagnostic logger, the test passes with both Off and Warn, verifies startup and disabled-state records, parses both complete samples with all 15 phases, and confirms normal logging settings remain unchanged. The CI gate now runs this output test before either full application build.
- Follow-up review found that retaining the cloned sinks through static destruction could outlive game DLL formatter code. A sink-lifetime assertion reproduced that retention. `FrameTiming_Shutdown` now releases the logger immediately before Context destruction, with both DLLs still mapped. The output test passes again and additionally proves sink release plus a fresh startup record on a new logger after reinitialization. This does not substitute for Windows DLL-unload runtime testing.

## Runtime capture and logger correction

- The first delivered candidate `ece0fe7` passed both platform builds and its packaged `soh.dll` contained the probe. Those checks did not prove the Windows logger boundary.
- The user subsequently measured Sacred Forest Meadow around 10–15 displayed FPS, versus roughly 20–35 in Lost Woods, Hyrule Field and Kakariko. These are the user's observations, not rates derived from the log.
- `Fleet of Harkinian(3).log` covers 12:05:25–12:12:49, includes Meadow actor records for scene 86, but contains no frame timing records. Its 1,225 texture-null errors occur earlier, with none during the recorded Meadow visits. Error count alone does not explain Meadow's slowdown.
- The Windows package links static spdlog copies into the game and engine DLLs. PE imports confirm that `soh.dll` imports the shared Context logger getters and `lusprintf`, but does not import spdlog's default logger/registry. Cloning the game DLL's default logger therefore does not select the engine's Fleet file sink.
- A revised output test binds a real async file logger to a Context owner while assigning a different default logger. The unchanged production adapter fails its `armed` assertion: no startup record reaches the file. The correction selects `Context::GetRawInstance()->GetLogger()` explicitly; normal game log severity and the existing sink/thread ownership remain unchanged.
- The local test substitutes only Context subsystem setup and retains the real Context header, production probe, async logger, rotating file sink and JSON parser. A separate Windows target runs the same test with the real `libultraship.dll` and a separately linked probe DLL, before compiling either game. This must pass before delivery. Test binaries are outside the package runtime directory.
- This repairs diagnostic delivery only. The performance bottleneck is still unproven, and no scene, asset, save, renderer workload or baseline branch is changed.
