# Final OoT rendering/interpolation diagnostic candidate

Parent: `909b41f8a217bef993af850b56ffd6c2c9bfd050` (tested CI merge `8cac597`). Preserves Din POC2, the Alt lookup optimization, all scene/actor/weather behavior, interpolation results, cache selection, and scheduling decisions. This is a diagnostic candidate, not a claimed performance fix or master promotion. Subsequent performance PRs must implement measured changes with before/after results; no further diagnostic-only PR is planned.

## Capture

Use the candidate's Windows build with the same save/mods/settings. Diagnostics are ON by default, independent of normal log level. Enter Hyrule Field, reproduce movement/stalls, then Meadow and Temple of Time, and exit normally. Supply the new `logs/OoT-RenderFlight-<number>.log`. This dedicated file preserves the session when the ordinary game log rotates. Startup announces its full path. No console setup is required.

Capture is bounded at 15 minutes of elapsed time after gameplay starts or approximately 256 MiB of serialized diagnostic payload. It emits a capture-limit event and stops instrumentation at the next tick; a byte-limit event explicitly reports the omitted final record. Restart the application for another capture. The shutdown drain does not wait for background work: tail records report outstanding scopes and explicitly mark that queued/late work may outlive capture.

The existing `gDeveloperTools.FrameTimingProbe=0` disables collection. `gDeveloperTools.RenderFlightDetail` is 2 by default (every rendered frame gets opcode/resource attribution), 1 for the original periodic opcode profiler with continuous frame/tick/resource evidence, or 0 to omit opcode profiling while retaining other metrics. No different build is needed. `gDeveloperTools.RendererCostProbe=0` also disables opcode collection. `gDeveloperTools.AltRenderLookup` retains the existing optimization on/off comparison. Mode 2 has real overhead; do not compare its FPS directly with an uninstrumented build and call the difference game cost.

## Coverage and criteria

| Question | Evidence |
|---|---|
| Which frame/tick stalled, including frame 1? | Every attempt and tick: monotonic IDs, timestamps, context ID, game tick, scene/room/age/Alt/pause/target tags. One-second batches; context/shutdown flush. |
| Is smoothness limited by scheduling? | Every attempted interpolation index/fraction, presented flag, ready duration and DirectX skip reason; original/effective desired time, queue depth, refresh statistics, scheduler resets, Present ID and waits. |
| Are matrix poses repeating or failing to interpolate? | Replacement count, within-tick matrix fingerprint, original-pose flag; branch/fallback/op/matched-op/matrix-write/camera-epoch counts. Matrix equality is not pixel equality. |
| Is a stall CPU work or waiting? | Tick wall and main-thread CPU time; game-update/draw/interpolation/render/setup/GUI/present phases, resource queue/wait, independent GPU duration. Wall minus thread CPU is NOT a pure GPU measure. |
| Is GPU work too expensive? | DX11 asynchronous timestamp/disjoint ring, source attempt ID, pending/available/disjoint/error/dropped statuses; no added GPU wait or Flush. Scope includes command execution and GUI, excludes pre-command setup. Other backends explicitly unsupported. |
| What inside Play_Draw or actor update stalls? | Reserved fixed-phase summaries covering room/scene, skybox, lighting, weather, actors, effects, overlays and hooks. Separate actor ID/params aggregation preserves draw/update counts and durations independently of resource churn. |
| Is weather or actor count the cause? | Actors visited/drawn/culled/lens queued, update/draw groups, actual rain/drop/splash/lightning loop counts and phase timings; RNG and geometry parity tested with diagnostics off/on. |
| Which resource blocks? | Cold and slow resource queue/read/import/wait events with origin tick, thread, path, manager and cache owner; cache outcome counts; duplicate/error outcomes; late completion explicitly tagged. Archive read includes decompression performed by the archive API; no fabricated separate decompression measurement. |
| Are uploads, shaders, draw calls or cache churn expensive? | Continuous compact renderer counters/scopes in detail mode; full opcode/resource/upload reports for every over-target-budget draw and every 31st ordinary draw. Filepath display lists receive names even without marker commands. |
| What changed between comparisons? | Build/commit/archive-order/settings metadata at capture start and approximately every 200 ticks; each attempt records detail/Alt state and, when profiled, effective render dimensions/MSAA. |
| Is tracing itself the cost? | Collector Record/Count time/calls, per-tick JSON finalization/build time, prior log-emission time, logger queue depth/overrun count, matrix fingerprint time, renderer non-command cost, explicit detail mode; same-build detail/disabled controls. Opcode clock cost cannot be completely subtracted from application work. |
| Did evidence overflow or become unavailable? | Separate attempt/tick/event/phase/actor/counter bounds and overflow counts; missing thread CPU/GPU values are null/unsupported, never fabricated zero; capture limits and incomplete shutdown tail flagged. |

CPU presentation/submission intervals do not prove physical scanout cadence, duplicated pixels, or display-driver behavior. DXGI statistics and GPU timings provide additional evidence; external display tracing may still be necessary for issues outside the application. No camera/weather/actor-state equivalence is inferred from scene names alone.

## Analysis

`python3 scripts/diagnostics/analyze_render_flight.py path/to/OoT-RenderFlight-....log --require-complete`

Produces coverage/error counts, per-scene CPU submission interval p50/p95/p99/max, skips/reasons, renderer/GPU costs, repeated matrix-pose pairs, worst ticks with full attribution and slowest resource events. It rejects missing core records, malformed captures and flagged overflows/limits. Use the dedicated capture alone; do not concatenate it with the same records from the ordinary game log.

## Validation

- Real async JSON/file logger with ordinary logging Off/Warn; skips, tick association, GPU pending, named counters, phases, shutdown/restart, C bridge and Windows export definitions.
- Production cache/Alt fixtures for both games plus bounded cross-thread trace/late-worker tests under UBSan; fixed phases and actor groups survive resource churn.
- Real scene/actor/weather C translation units and production rain-loop fixture: same random calls and geometry with tracing on/off.
- Production DX11 query methods with fake D3D provider: pending/disjoint/query errors, ring exhaustion, disable/re-enable, one-time draining.
- Production DXGI readiness decisions with fake provider: both skip reasons, clock reset, unavailable statistics, traced/untraced decision parity.
- Actual interpreter, Fast3dWindow and interpolation translation-unit syntax checks; timing accounting, every-frame stall and display-list ownership tests.
- Analyzer tests preserve large stalls across skipped ticks, reset at context changes, correlate delayed GPU results to their originating attempt and reject missing/truncated evidence.

Native Windows application build and in-game capture remain distinct acceptance gates. CI must pass; runtime performance and visual preservation cannot be proved by fixture tests.
