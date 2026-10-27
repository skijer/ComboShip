# MM streamed PCM: SoH parity candidate

## Problem and change

MM's native `aLoadBufferImpl` copies only complete 16-byte DMA units. The
custom `CODEC_S16` path reused it for decoded PCM, including Prelude MP3,
WAV and Vorbis samples. That path advances by the entire requested sample
count, so a non-aligned request discarded its last one to seven samples.
The cleared buffer then supplied zeros to the resampler at those positions.

SoH copies the entire requested PCM length. Both games already use the same
four-tap resampling coefficients and 32 kHz output. This candidate adds
`aLoadBufferExactImpl` and calls it only for custom decoded PCM in MM.
The native DMA loader, Opus decoder, sample tuning, pack metadata and loop
handling remain unchanged. Existing music packs do not need conversion.

Baseline: `bb0b275fafe61d6f440e20ba879e2b659edd74cb` on
`poc/mm-3ds-owl-blue-flash-20260922`. Candidate:
`poc/mm-streamed-audio-soh-parity-20260923`. This retains the cumulative
ComboShip integration and adds only this audio change, tests and documentation.

## Reproduction and verification

The regression runner compiles the actual MM streamed-sample switch branch
and the actual SoH/MM PCM loaders and resamplers. It feeds both mixers the
same source samples and chunk schedule, including consecutive fractional-rate
chunks. The test failed on the baseline before production code was changed.

| Input / chunk schedule | Output samples different from SoH before | After |
| --- | ---: | ---: |
| 32 kHz tones, fixed or varied | 0 / 64,000 per run | 0 |
| 44.1 kHz tones, fixed 160 samples | 2,161 / 64,000 | 0 |
| 44.1 kHz tones, varied | 1,679 / 64,000 | 0 |
| 48 kHz tones, fixed 160 samples | 0 / 64,000 | 0 |
| 48 kHz tones, varied | 337 / 64,000 | 0 |
| Supplied track, 44.1 kHz left-channel excerpt, varied | 8,399 / 319,952 | 0 |

The local track analysis used ten seconds starting at 128.7925 seconds,
decoded to mono signed 16-bit PCM without changing its sample rate. No user
music is included in the repository. This comparison covers the PCM load and
resample stages; it is not an end-to-end stereo game recording.

Additional checks passed:

- Exact loads and final tails from 0 to 576 bytes, source offsets, destination
  sentinels, and unchanged native DMA alignment.
- Pending PCM/Opus silence and routing to the existing Opus decoder.
- Address/undefined-behavior sanitizers, including precisely sized source
  allocations and the supplied-track excerpt. Local leak detection was disabled
  because LeakSanitizer cannot run under this environment's process tracing;
  the CI test uses normal sanitizer defaults.
- Existing MM streamed-audio runtime checks and all 21 decoder regressions.
- Full real-header syntax checks of MM's audio translation units, including
  synthesis and mixer, and clang-format 14 on changed C/C++ files.

Commands:

```sh
python3 scripts/diagnostics/run_mm_audio_pcm_tests.py
MM_AUDIO_PCM_CFLAGS='-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie' python3 scripts/diagnostics/run_mm_audio_pcm_tests.py
python3 scripts/diagnostics/run_mm_audio_runtime_test.py /tmp/mm-audio-runtime
python3 scripts/diagnostics/run_mm_audio_decoder_tests.py
```

The decoder runner requires the existing libogg/libvorbis development packages.
Both PCM runs are also part of the Build Artifacts regression gate.

## Runtime acceptance

Use the candidate executable with the original Prelude pack and the same mod
stack, audio backend, volume, scene and playback section as the noisy recording.
Check the higher-rate tracks, stereo playback and a loop boundary; compare
native music/effects as a control. Cor's listening test is still required before
claiming the reported grain is resolved or promoting this candidate.
