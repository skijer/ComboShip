# Final rendering/interpolation diagnostic implementation plan

Goal: one final diagnostic PR captures the evidence needed to implement and compare performance fixes, including stalls that occur outside periodic renderer sampling.

Baseline: 909b41f (runtime merge 8cac597), preserving Din POC2 and Alt lookup optimization. Isolated branch poc/oot-render-flight-recorder-20260927. No changes to assets, actor behavior, weather behavior, interpolation results, scheduling decisions, cache policy or master branches.

Design authorized by the user's explicit request to include all rendering/interpolation criteria now, with no further diagnostic-only PRs. Extend existing recorder and shared-engine boundaries; independent backend, resource and draw instrumentation are implemented in parallel with disjoint file ownership.

- Continuous per-attempt timing and identity: tick/frame IDs, interpolation fraction/index, matrices produced and coverage, ready/skipped reason, CPU wall/thread time, renderer/setup/GUI/Present duration, exact CPU presentation intervals; bounded one-second batches and transition/shutdown flush.
- Continuous per-tick phase arrays and named actor/scene/weather/hook timings, resource cold/slow events with original request/tick IDs and worker queue/read/import/wait stages. Explicit overflow and late records.
- DX11 asynchronous GPU timestamp/disjoint queries, frame correlation, supported/pending/error status; driver scheduling refresh/queue/present information, no synchronous readback.
- Detailed opcode/render-cache/upload/shader counters can cover every rendered frame in the same build; default comprehensive capture retains every slow draw, with periodic ordinary detailed reports and continuous compact frame rows. Mode 1 gives lower-overhead comparison, mode 0 disables opcode profiling.
- Probe overhead and unsupported metrics are identified. Windows build validates real engine/game DLL linkage; no fabricated zero GPU results or claims that CPU submission equals physical display timing.
- Tests exercise skipped frames, first-frame/unsampled stalls, transitions, disable/reset, bounds, async resource completions, output with normal logging off, GPU pending/error paths where portable. Analysis tool rejects missing coverage rather than declaring success.

Acceptance for this PR: tests and Windows build, coverage checklist tied to emitted fields, one documented capture setup. The next performance changes must implement a measured transformation and report matched before/after results; diagnostics alone are not a performance improvement.
