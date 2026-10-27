# MM streamed audio SoH parity implementation plan

> **For agentic workers:** Use superpowers:executing-plans to implement this plan.

**Goal:** Preserve all decoded PCM samples in MM, matching SoH's copy/resample behavior.

**Architecture:** Add an exact-length DMEM loader and use it only in the custom CODEC_S16 synthesis branch. Keep the generic native loader unchanged.

**Tech Stack:** C, Python regression runners, actual MM and SoH mixer functions, GitHub Actions.

**Spec:** docs/superpowers/specs/2026-09-23-mm-streamed-audio-soh-parity.md

## Global Constraints

- Isolated candidate based on bb0b275f; preserve the complete feature stack.
- Existing packs, decoder formats, sample rates, loop metadata and native audio behavior stay intact.
- Do not include user-supplied music in the repository.
- Record local regression evidence separately from full build and user runtime acceptance.

## Review Focus

- A fixture must execute the actual synthesis branch, not only a replacement helper.
- Non-aligned PCM requests and short final chunks must retain their exact bytes without overread.
- Native DMA alignment and Opus/pending-decoder behavior must remain unchanged.
- Consecutive fractional-rate resampling must match SoH with the same source and chunk schedule.
- Preserve all integrated features and keep runtime-quality claims within observed evidence.

### Task 1: Preserve custom PCM chunks and verify SoH parity

**Files:** mm/2s2h/mixer.{c,h}; mm/src/audio/lib/synthesis.c; mm/tests/audio_pcm_test.c; mm/tests/audio_decoder_test.cpp; scripts/diagnostics/run_mm_audio_pcm_tests.py; .github/workflows/build-artifacts.yml; docs/audio-streamed-pcm-parity.md.

**Interface:** `aLoadBufferExactImpl(const void*, uint16_t dest, uint16_t nbytes)` copies exactly the requested byte count. Only CODEC_S16 calls it.

- [ ] Add a production-function fixture for MM's streamed branch and both mixers. Check non-aligned lengths, source offsets, final tails, pending decoders, native alignment, and repeated 32/44.1/48 kHz resampling against SoH.
- [ ] Run the fixture before implementation; expect missing PCM bytes and SoH parity failures at non-aligned lengths.
- [ ] Add the exact loader, route CODEC_S16 to it, and update the existing decoder fixture and CI gate.
- [ ] Run the fixture normally and with address/undefined sanitizers; run existing MM runtime and decoder regressions. Compile changed C translation units with real headers.
- [ ] Compare an offline section of the supplied 44.1 kHz track with the same SoH/MM chunk schedule; retain only metrics in the report.
- [ ] Document scope and remaining runtime acceptance, commit the candidate, and request a fresh whole-branch review.
- [ ] Publish the candidate branch/build without merging into the accepted baseline; report exact build status and original-pack runtime test.
