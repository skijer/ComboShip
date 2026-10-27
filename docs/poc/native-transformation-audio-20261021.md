# Native transformation audio candidate — October 21, 2026

| Field | Record |
|---|---|
| Source baseline | `8ab33deeb5000c6a66663dc9659d9060f702881a`, tree `a6a6319e451515c26f27dc4a9a99f2c62be3629b`; current cumulative PR #48, including the market arrival correction |
| Candidate / recovery branch | Local `fix/native-transformation-audio-20261021`, direct child of that baseline; publication must be verified separately |
| User evidence | `MedalTVScreenRecording20261021011422566.mp4`; user accepts the transformation camera and reports SFX sounding as though samples are skipped |
| Scope | Restore the isolated OoT/solo SoH MM SFX renderer's missing high-rate sample decoding; retain the native cue IDs, schedules and camera/controller |
| Preservation | `mm_native_transform.inc.h` remains byte-identical; MM's own audio renderer, all archives, geometry, animations, lighting, particles and unrelated cumulative fixes remain unchanged |
| Verdict | Specific sample-rate defect reproduced and corrected in real-DSP tests; native units compile. Attribution of the entire reported audible symptom and in-game acceptance remain unproven |

Recording SHA-256:
`4d0d96797b91fc282ebfa71c3296b20467c017ed9b266a4f9bcfb0246210b904`.
The recording does not identify its executable commit or all mod/load settings.
Camera acceptance is the user's report for this recorded configuration; it does
not establish every form/age/host configuration.

## Diagnosis

Both AAC tracks contain the same game mix at approximately 2:1 gain. Neither
has full-mix silence gaps of at least 20 ms at the measured threshold. This
does not exclude individual SFX dropouts. Direct audio audition was unavailable;
the investigation used extracted frames, waveform measurements and source/
synthesis tests rather than claiming to hear the recording.

Native MM mask animation cues are still scheduled at the donor frames. A
controlled real-DSP probe ruled out ordinary 528/544/560-frame output-buffer
partitioning as the cause of large gaps in a steady sample; small envelope
rounding differences remained. No speculative buffer-size fix was applied.

The actual native flash sequence `0x484f` selects `FlyingFairy_META` with
`frequencyFixedPoint=0xC000` and `hasTwoParts=true`. MM's note layer halves the
fixed-point frequency for ratios of at least 2, then its sample renderer decodes
and decimates two source spans. The isolated renderer instead forced one part.
Consequently, this layer consumed 264 rather than 528 source samples per 176
output frames: half the requested advance and an octave below the requested
pitch. That is a confirmed rendering defect; it is not proof that every sound
the user described as skipping has this cause.

## Change and verification

Use the donor's two-part loop, odd-span rounding, `aInterlImpl` decimation,
packed input offsets and early-completion padding. The existing single-part
decode/resample/envelope path is retained. There is no new setting or asset
dependency. The new regression is included in the existing audio CI step.

- `python3 -B tests/mm_sfx_render/run_tests.py`: real production renderer and
  host DSP, controlled S8/ADPCM/SMALL_ADPCM waveforms, AddressSanitizer. Before the
  correction, the normal case passed and the high-rate case failed with
  `consumed 264 source samples, expected 528`. Afterward, normal/high-rate,
  odd spans, first/second-part one-shot completion and loop playback pass (eight
  cases). Assertions cover silence padding after early completion and
  audible waveform frequency as well as source cursor advance and lifetime.
- `python3 -B scripts/diagnostics/run_oot_native_transformation_tests.py`:
  116 controller scenarios across solo SoH and ComboShip, including native
  donor camera comparisons, plus the two host completion lines; all passed.
- `python3 -B tests/goron_audio/run_tests.py`: all 28 sequence/direct dispatcher
  and lifetime runs passed, including the eight ritual cues and cancellation.
- The complete modified backend compiles as a native C++20 object with and
  without `COMBO_BUILD`. Formatting and whitespace checks passed.
- Independent read-only review found no blocking issue, compared the split
  directly with the MM donor and reran all eight AddressSanitizer cases.

A separate throwaway harness ran the production sequence, playback, effects,
data, glue and backend with the real host DSP. A resource adapter supplied
resident Sequence_0, fonts 0/1 and samples from the available native archive,
SHA-256 `2f44bafe1a00f44ddaf7eb5e5c99e73a3d23771038529d6d1748819ad575dc80`.
This adapter bypasses the production archive loader and does not prove installed
resource resolution, output-device behavior or game-frame contention.

Five-second isolated PCM renders for the seven other actual ritual cues
(`0x0877`, `0x1856`, `0x0874`, `0x1858`, `0x09aa`, `0x09a4`, `0x2912`) were
byte-identical before/after. The flash's high-rate layer now advances 1,584
rather than 792 source samples over three synthesis updates. Its PCM changes
as expected. No native archive or decoded sample is published with source.

## Runtime check and recovery

This requires a rebuilt OoT/SoH module. Preserve the accepted packs, order and
camera configuration; leave Instant Transform off. Repeat mask-on and mask-off,
listening through the struggle, flash and return to control, then skip a repeat.
Check whether the reported skipping is gone, especially around the flash.
If it remains during the gasp/scream or face-change cues, retain that separate
failure: those PCM paths were intentionally preserved by this correction.
Solo SoH and other age/form configurations require their own runtime checks.
No accepted master is promoted by source publication or passing fixtures.

The recovery branch retains the full source tree and this record. The accompanying
checkpoint package identifies the exact local commit and publication status.
After publication, restore with
`git clone --branch fix/native-transformation-audio-20261021
https://github.com/marsh6487/ComboShip.git`, or check out the exact candidate
commit recorded by the publication. Revert that single commit to return to the
baseline without undoing the camera or other cumulative bug fixes.
