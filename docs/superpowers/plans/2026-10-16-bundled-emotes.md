# Bundled Emotes Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Bundle all eight supplied emotes with configurable slots, native playback and existing multiplayer pose synchronization.

**Architecture:** A shared catalog, frame decoder and clock feed small OoT/MM adapters. The shared menu stores slot/chord CVars; both network implementations serialize the selected frame using their existing schemas. Audio is mixed locally through each game's existing final audio buffer.

**Tech Stack:** C++20, C game seams, ImGui, Python asset import, existing O2R packaging.

**Spec:** `docs/superpowers/specs/2026-10-16-bundled-emotes-design.md`

## Global Constraints

- Include all eight supplied animations in one candidate; no one-animation build gate.
- No external `.pak` or script runtime is required.
- Preserve normal player controls, collision and network packet schemas.
- Keep creator credits and distinguish static checks from runtime acceptance.

## Review Focus

- Chords held across frames must not repeatedly restart an emote or trigger D-pad equipment.
- MM joint buffers have alignment slack; serialize the aligned pose, not an assumed buffer start.
- Native queued animation copies must not overwrite the emote before rendering.
- Damage/scene/game changes must stop poses and sounds without leaving player flags altered.
- Malformed/missing assets and invalid persisted slot IDs must fail closed.

### Task 1: Assets and shared playback

**Files:** `combo/emotes/*`, `tools/emotes/import_paks.py`, both `assets/custom/emotes` directories, `tests/emotes/*`.

**Interfaces:** `Catalog`, `DecodeFrame`, `Playback::Start/Tick/Stop`, `DecodeFrame`, `SelectBinding`, `AudioVoice::Mix`.

- [x] Add failing standalone behavior tests for frame bounds, complete catalog, clocks, chords, cancellation and PCM mixing.
- [x] Run `python3 tests/emotes/run_tests.py`; expect missing production header.
- [x] Implement shared code and a data-only pack importer; import all eight packs and both sounds with credits.
- [x] Run the focused suite; expect all behavior and asset checks to pass. Commit.

### Task 2: Native game and network adapters

**Files:** both game `Enhancements` emote adapters; `z_play.c`; `OTRGlobals.cpp`/`BenPort.cpp`; Anchor and Harpoon pose senders.

**Interfaces:** C `ComboEmotes_BeginUpdate`, `ComboEmotes_ApplyPose`, `ComboEmotes_CopyPose`, `ComboEmotes_Reset`, `ComboEmotes_MixAudio` instantiated privately in each game module.

- [x] Extend the behavior tests to cover aligned raw-buffer writes and sent pose equivalence.
- [x] Implement eligibility, chord consumption, resource loading, cancellation and post-queue pose application.
- [x] Adapt all four pose sender paths; retain normal serialization when inactive.
- [x] Run focused behavior tests and native-header syntax checks for changed translation units. Commit.

### Task 3: Shared controls and complete candidate

**Files:** `combo/gui/ComboEmoteSettings.*`, `ComboMenu.cpp`, `combo/CMakeLists.txt`, checkpoint/verification record.

**Interfaces:** `ComboEmotes::DrawSettings`, shared `gCombo.Emotes.*` CVars.

- [x] Add shared Settings > Emotes with all eight choices, four chord selectors, play/stop, audio gain and credits.
- [x] Compile the UI and both native adapters, rerun focused tests and relevant existing checks, and review the complete baseline diff.
- [x] Record exactly what passed and what remains runtime-untested. Commit and preserve a source checkpoint without publishing to the shared branch.
