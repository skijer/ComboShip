# Bridge surfaces and Summer lights implementation plan

> **For agentic workers:** Use executing-plans for the Lake task and dispatching-parallel-agents for the independent Snowhead/Summer tasks. Track each task with its verification evidence.

**Goal:** Implement the user's tunnel, Snowhead and Summer-night appearance corrections as one bounded source input.

**Architecture:** Leave collision and water builders unchanged. Add a visual-only Lake floor partition and overlapping backdrop, maintain separate stone/sand resource paths, reuse the Winter mist mesh, and tune both native Summer states equally.

**Tech Stack:** C++20, native GBI/display-list fixtures, Python archive/test tools.

**Spec:** docs/superpowers/specs/2026-10-26-bridge-surfaces-summer-lights-design.md

## Global constraints

- Preserve baseline eb3c7cadd6419afd4a68015358880b8d27d0601e and all accepted source/assets.
- No develop/master merge, remote publication, scene archive rewrite, global weather change or gameplay-acceptance claim.
- Retain existing stone override contract and add independently editable sand.

## Review focus

- Native and replacement stone/sand have different logical tile sizes; test actual queued sampler dimensions.
- Floor partitions must cover the original dry surface exactly and never intrude into the stream.
- Backdrop overlap is visual only; collision and portal pose remain unchanged.
- Snowhead must budget transient mist vertices and fade near the camera without inherited fog/depth writes.
- Both Summer ports and the preview must agree; pause and day/night fades remain effective.

### Task 1: Lake appearance (root)

- [x] Add and run failing production-geometry tests for sand area/coverage and backdrop overlap.
- [x] Add `combo/entrances/LakeHyliaSurface.h::BuildSurfaceGeometry()` returning stone/sand visual quads with shared boundary vertices.
- [x] Update the Lake renderer and material resolver/builder for clean native stone and independent sand; preserve water/collision.
- [x] Run sand tests, native Lake geometry/render suites and material factory/pack checks; compare protected source to baseline.

### Task 2: Snowhead Winter mist (snowhead_winter_mist)

- [x] Demonstrate native regression on the old dust-card mist.
- [x] Share Winter's 15-vertex feathered mesh/material with the local approach, whiten flakes and add smooth camera fade with correct arena budget.
- [x] Run native mist, Snowhead lifecycle/geometry and Winter renderer checks, recording limits.

### Task 3: Summer fireflies (summer_fireflies)

- [x] Demonstrate insufficient mature swarm/pulse/halo visibility in both states.
- [x] Increase both counts 56→96, alpha `.30+.64*pulse`→`.48+.52*pulse`, radius scale `.0030`→`.0036` and cap `4.8`→`5.8`; preserve all existing fades and motes.
- [x] Update preview parity and run both native Summer state/render suites, including paused transitions.

### Task 4: Recovery and handoff (root)

- [x] Review the combined diff, run relevant combined checks, create a bounded source commit.
- [x] Build a deterministic ZIP with patch, originals/changed source, refs, evidence and restoration instructions; restore-check independently.
- [x] Save the candidate and report source/native results separately from Windows/gameplay tests.

## Later authorized relocation and diagnosis

Snowhead departure/return moved to groundedX3600/3500 with recalculated full-valley shimmer and shared cold anchors. Actual ascent and valley geometry hashes are unchanged. Full regional ASan/UBSan gate and targeted native/shimmer/crossing controls passed. Independent review approved both the initial source range and this new delta. Optional broad Valley renderer/presentation checks remain blocked by external BS_thread_pool.hpp/zip.h, as recorded in the POC note. Windows and configured gameplay remain untested.

Termina tint was diagnosed only; no seasonal parser or archive change is included. Both parent Final and merged archive contain0xEF; actual production rejects that floor draw while Spring reaches its textures through image copies. See the separate diagnosis.
