# Native MM shop audio candidate

| Field | Record |
| --- | --- |
| Baseline | `392b4797a775d2d08f9e26238584b1e08df4d0a2`, the current dungeon-reward candidate on the progressive-fire / cap / Time Gate stack. These are retained candidates, not newly claimed runtime acceptance. |
| Candidate | `poc/mm-native-shop-audio-20260930` |
| Report | Cor hears severe crackling in 2Ship shops both while idle and while browsing/buying. Hold the separate SoH Shadow Scepter branch until this audio work is implemented and checked, then publish both. |
| Scope | Native MM audio production during rendering stalls; SSE2 native mixing in both ComboShip engines. |
| Preservation | Original sequences, instruments, samples, volumes, 32 kHz output, streamed PCM repair, private weather/Midna/SM64 cues, cap/Time Gate cleanup, game ownership and other cumulative features. No archive edits or platform-wide audio backend changes. |
| Verdict | Implemented and locally regression verified. Full builds and audible shop playback require separate evidence; no master promotion. |

MM's worker previously waited indefinitely for a graphics-frame notification.
A slow frame could drain the device queue, interrupting all native music and
sound effects. OoT already has an independent refill timeout and larger
reservoir. MM now waits at most 5 ms after its first ready frame and refills
within the backend's desired queue size. Its requested reservoir matches
OoT's 4096 stereo frames (128 ms at 32 kHz; previously 1680 / 52.5 ms).
This can add buffering latency; it does not change pitch, volume or sample rate.

The worker retains its own mutex for all production and request state. The
graphics thread publishes a bounded update divisor under that mutex. Refills
are bounded, remain unprimed until the first ready frame, and release the
graphics waiter even when the queue is full or shutdown occurs. A 528/544
sample cadence averages exactly 32000/60 samples per native audio update, so
independent refilling does not accelerate sequence tempo. The same final mix
and inactive-game mute ordering are preserved, including private voices.

Separately, both SSE2 mixers reused an already-unpacked low product vector
when constructing the high lanes. The candidate preserves the original words
until both halves have been unpacked. This is a low-bit arithmetic defect;
it is not asserted to explain the user's severe crackling by itself.

## Evidence

- The complete baseline MM worker fails the controlled device test after
  graphics wakes stop. The candidate passes repeated 75 ms device drains with
  no graphics wake at update divisors 1, 2 and 3; full-queue backpressure,
  fixed tempo, complete-output mute, mix-in ordering, pre-init shutdown and
  unprimed restart also pass. This exercises the actual production worker;
  the audio device and synthesis callbacks are controlled test boundaries.
- Both complete production mixers pass 132,529 cases each with zero sample
  differences against independent wide Q15 arithmetic and the scalar path.
  Coverage includes all 65,536 gains, zero/odd/native block sizes, saturation,
  unaligned addresses, in-place decay and repeated stereo reverb feedback.
  Before repair each engine produced 734,704 mismatched samples.
- The worker and both mixers pass ASan/UBSan. Local LeakSanitizer is disabled
  because this container cannot inspect its process threads; CI retains its
  normal sanitizer configuration.
- Existing streamed PCM parity passes for 32/44.1/48 kHz, including partial
  chunks; native audio translation units and runtime boundaries pass.
- Existing MM weather and Midna suites pass, including native headers and
  final mute/mix-in ordering. The weather fixture follows the relocated
  production submission block rather than replacing that logic.
- The uploaded native `mm.o2r` has SHA256
  `f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59`.
  Inspection confirms Shop_44 uses font 12 and KotakePotionShop_43 uses font 14.
  A separate arithmetic probe of 7,408,560 native ADPCM samples found no
  difference between the existing decoder and widened accumulation. No native
  sample data is changed or committed. This was not a full song/game playback.
- Baseline CI run `36693964242` failed in the dungeon-reward fixture because
  explicit `-isystem /usr/include` broke libstdc++'s `#include_next <stdlib.h>`.
  This candidate omits the redundant system-root argument, retaining custom
  include support. All 32 native pool configurations pass, as do the shared
  fill, delivery and plando fixtures. Dungeon-reward gameplay code is unchanged.

## Runtime check and recovery

### October 1 audible follow-up: unresolved

Cor reports no perceptible shop improvement with the gain-buffer checkbox on.
The runtime log records both gain modes executing: 384 requested bytes process
384 bytes on and 768 off. This rules out a checkbox that never reaches the mixer.
For identical input, both modes perform the same arithmetic on the requested
span; the correction only prevents processing the additional adjacent span.
Its audible effect depends on whether subsequent synthesis uses that memory.
This buffer defect must not be presented as the shop-crunch diagnosis.

The recorded gain is 127 (UQ4.4 = 7.9375). Production synthesis applies this to
the resampled mono note before the filter and envelope. The gain mixer saturates
to signed 16-bit, so input magnitudes above approximately 4128 can clip before
later volume attenuation. The log does not record note amplitudes or clipping;
high gain alone does not prove distortion, since a quiet sample can require it.

The Google explanation does not establish the affected samples' actual rate or
codec. Sample rate and encoding bitrate describe different limits. The engine
supports both 4-bit and 2-bit ADPCM, but that does not identify which instruments
are audible in this shop configuration. Original sample texture, premature
clipping, decoder/filter behavior and device underruns remain distinguishable
hypotheses. This log contains no PCM capture or underrun measurements. The
previous archive arithmetic probe did not render the complete shop sequence.

Review verification: complete production MM/OoT mixer suite passes 137,137 /
132,529 cases with zero mismatches. This verifies tested arithmetic and buffer
spans, not shop sound quality. Next evidence should correlate the active shop
sequence/font/sample codec and tuning with pre/post-gain saturation, final PCM,
and device queue minima; compare the same instruments with reference playback.
Any corrective audio behavior remains opt-in under Fixes. Cor subsequently
authorized publishing the narrow item repairs alongside the PCM diagnostics
below. No additional corrective audio behavior is claimed.

### October 1 PCM diagnostic follow-up

Recovered baseline: PR #31 at `19b5b4c6b1381b187caf3b09854330c7ad9cd738`.
The interrupted local work was copied to an isolated checkout before review;
the published baseline and original checkout were preserved. Build #72 on that
baseline completed successfully. This follow-up retains its renderer,
transformation, audio-worker and gain-toggle candidates.

The new **MM Enhancements -> Fixes -> Trace MM Shop Audio** checkbox is off by
default. Enable it inside the affected shop. `[MMAudioPCM]` logs identify the
actual sequence and sample resource paths, whether they are streamed, player
ownership (main BGM/fanfare/SFX/sub BGM/ambience), codec, gain, resampling pitch,
tuning, filters and comb-filter gain. WAV/MP3/FLAC/Vorbis decoder metadata also
reports the PCM rate, source channels and frame count when available. Sequence
heap copies are aliased to their factory resource identity. Ownership is
captured for each prepared synthesis sub-update, before later updates can reuse
the same note for a different player. Released notes with no owner remain
explicitly unattributed.

PCM summaries cover decoded/loaded samples, resampling, gain, filtering,
processed mono notes, native stereo output and final stereo submitted to the
backend. They include sample counts, peak/RMS, rail and zero counts, maximum
adjacent-sample steps and hashes. Gain tracing additionally counts values that
would saturate within the audible requested span, excluding padding and the
legacy spill. Pitch/tuning changes close the old interval. These measurements
are diagnostic signals, not by themselves proof of audible distortion; decoder
windows can contain lookahead or overlapping samples. No sample, gain, filter,
volume, tempo, reverb or audio routing is changed by tracing.

Queue summaries report minimum buffered frames, empty-queue observations after
initial submission and maximum worker wake gap. Empty observations are not a
backend underrun counter. Final PCM is captured before backend/device processing;
a clean capture cannot rule out distortion introduced by the device afterward.
Both output channels are measured separately. Detail logging is limited to 128
records per summary window; four output summaries and coverage records remain
available even if note/event activity exhausts that budget.

The first 20 seconds of traced final output are retained in memory. Turning the
checkbox off/on retains unsaved PCM, concatenating the enabled intervals within
that limit. Quit normally or switch games to stop/join the MM worker and write
`audio-diagnostics/mm-audio-pcm-*.wav` below the MM app directory. The log prints
the exact path. WAV output is signed 16-bit, stereo, 32000 Hz. There are no disk
writes on the audio worker. Resource registration is synchronized separately
from worker-owned statistics/capture; maps and capture are bounded. A process
crash before normal worker shutdown cannot save the in-memory capture.

Focused observer checks use the actual host, gain and DMEM-probe functions under
ASan/UBSan: tracing on/off preserves PCM and adjacent workspace in both gain
modes, BGM/SFX note reuse and sub-update ownership remain separate, pitch changes
close the right interval, invalid DMEM ranges are rejected, metadata and queue
observations are recorded, output survives detail-budget exhaustion, A/B toggles
retain the bounded capture and WAV header/payload are correct. The production
worker passes with tracing off/on, including stalled rendering, tempo,
backpressure, mute/mix-ins, restart and shutdown. Complete native mixers pass
137137 MM / 132529 OoT cases with zero mismatches, and 32/44.1/48 kHz PCM parity
and full MM audio C translation-unit syntax checks pass. Decoder/platform CI and
actual shop diagnostic output require their own evidence. This is an implemented,
focused-regression-verified diagnostic candidate, not an audible shop repair or
master promotion.

Use the same build configuration, packs, Alt setting and shop that exhibited
the problem. Listen while idle, browse and buy an item, hear the fanfare, exit
and re-enter, pause/unpause, and switch OoT -> MM -> OoT -> MM. Check steady
music tempo, ordinary SFX and weather/Midna audio outside shops. The latest
supplied log does not establish the current shop's frame/audio timing or exact
pack state; audible resolution in that configuration remains unverified.

Keep the parent candidate/build as rollback. Only this branch receives the
audio changes. The accepted integration branch remains unchanged.
