# October 26 cumulative publication implementation plan

> **For agentic workers:** Execute the user-authorized integration in the isolated candidate checkout; independent asset tasks are delegated with explicit path ownership.

**Goal:** Publish a cumulative Windows candidate containing today's source deltas, a built-in Sages Chamber, and the corrected Lake Hylia sand drift.

**Architecture:** Preserve public baseline `fa81f7926038f2aa85c7f3e73c96fec043168b66`, descended from recorded parent `9f23b84859ac545a283c155b6642aea302aad414`. Merge exact independently verified source histories, then prepare source-owned scene bundles through normal CMake resource generation.

**Tech stack:** C/C++, Python resource preparation, CMake, GitHub Actions Windows packaging.

**Spec:** The user's October 26 publication authorization, including the chamber and half-platform sand correction, is the approved specification. No further approval gate is needed within that scope.

## Constraints and review focus

- Keep develop/master unmerged; publish a candidate PR only.
- Preserve original/combined game guards and baseline source behavior.
- Chamber must contain geometry, collision, all setup headers and material dependencies in the normal package, with both Alt settings usable without the external archive.
- Sand must occupy approximately half the platform irregularly along the tunnel bends; protect collision, water, platform shape and transitions.
- Preserve the latest appended Warp songs mode from the source checkpoint in addition to Off, Dungeon rewards (default), and Free.
- Do not confuse static/build proof with runtime acceptance; retain explicit test boundaries.
- Retain immutable inputs, hashes, reproducible source recovery and milestone checkpoints.

## Tasks

- [x] Merge latest chamber eae07d49 (already checked out), palettes 62431bf5, Mirror 176ae8ef and Tycoon 67dad00a. Resolve overlaps and run their existing focused regressions.
- [x] Package chamber POC4 through `tools/temple_sages_bundle`, `scripts/assets/prepare_temple_sages.py`, and `CMake/TempleSagesAssets.cmake`; validate resource closure and all age/setup headers.
- [x] Implement sand coverage through `tools/lake_tunnel_sand`, `scripts/assets/prepare_lake_tunnel_sand.py`, and `CMake/LakeTunnelSandAssets.cmake`; validate affected geometry/materials and protected resources.
- [x] Wire asset preparation into CMake and register meaningful integration checks. Record exact source/archive manifest and recoverable milestone.
- [x] Run relevant regression groups and resource preparation; resolve scoped failures. Obtain an independent whole-branch review.
- [ ] Commit and publish candidate branch/PR; monitor required CI, repair scoped failures, and obtain/verify downloadable cumulative Windows package.

## Progress

- Input inventories and all ten source ZIP CRCs verified. Baseline public CI passes. Input archive manifest records 457 verified file hashes.
- Baseline recovery is available from public PR65 and immutable source ZIPs; working candidate is isolated at `/workspace/scratch/6f515eaa8103/ComboShip`.

## Approved revision: native scene textures, optional visual archive

The user explicitly revised chamber packaging after pausing publication, then confirmed the split: builtin scene geometry/collision comes from the supplied scene; builtin ordinary surfaces use native OoT textures. The existing chamber O2R remains an optional visual addon providing custom/HD textures, scrolling materials and effects. Preserve the existing addon bytes and resource identities, and keep geometry/markers/collision/travel usable without it. The previous full-visual bundle is a superseded packaging experiment, not the publication payload.

- [x] Rebuild builtin geometry/material bindings against native OoT textures; remove embedded HD/custom texture payloads from the default chamber bundle.
- [x] Verify optional original POC4 resolves over the builtin geometry and restores authored visuals without changing layout/collision/progression.
- [x] Retain the built-in-aware portal readiness fix and its RED/GREEN evidence.
- [x] Recheck both asset modes and source dependencies, checkpoint revised source, then resume the already authorized candidate publication/CI.

## Asset publication and notes requirements

Use the existing morning source-pool workflow: lossless deduplicated resource pools, zlib, ordered base64 parts no larger than 98,304 decoded compressed bytes /131,072 text bytes, per-part/pool/resource hashes and bounded exact reconstruction before checks/packaging. Preserve previous accepted source parts byte-for-byte. New raw image/resource/archive binaries are generated from source pools and excluded from the publication tree. Public PR/release notes describe the native-texture base and optional visual archive without naming the donor pack.

## Final source evidence

The revised native-texture package, exact optional visual-layer closure and acoustic patch are verified as recorded in `docs/integration/20261026/verification.md`. The original optional input remains unchanged; the updated optional candidate adds its missing visual dependency closure and the same stone/echo metadata. Independent review identified a redirected private-namespace cleanup bug; its regression failed against the old behavior, and the complete eleven-case package suite passes with redirections rejected before writes. Full platform CI and configured runtime acceptance remain separate.
