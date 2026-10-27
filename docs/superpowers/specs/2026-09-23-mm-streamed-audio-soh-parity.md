# MM streamed PCM parity with SoH

Cor reports grain/static in higher-rate Prelude music in ComboShip/MM, while
the same streamed audio is clean in SoH. The requested result is matching
SoH's playback behavior with existing packs. Converting packs to WAV is only
a diagnostic, not the solution.

Both mixers use the same 32 kHz output and four-tap resampler. MM's generic
DMEM load rounds down to 16 bytes; SoH copies the requested length. MM also
uses that rounded load for host-decoded CODEC_S16 samples, then advances by
the full requested count. Non-aligned chunks can therefore lose up to seven
16-bit samples. Reproduce this difference before changing production code.

Copy every requested byte for decoded PCM only. Preserve native MM load
alignment, Opus handling, pending-decoder silence, pack metadata, timing,
volume, resampler coefficients and all existing integrated features.

Baseline: bb0b275fafe61d6f440e20ba879e2b659edd74cb, the cumulative owl-blue-flash
candidate. Work on a new candidate branch. No master promotion or replacement
of original music packs. Automated parity and memory checks are implementation
evidence; cor's runtime test with the original pack remains the acceptance gate.
