# Streamed positional music repair

| Field | Record |
| --- | --- |
| Baseline | PR #31, `d196518008067debf922372ab60bc9649d440c8b`; prior item-grant repair reported runtime-passed by cor |
| Candidate | Opt-in streamed positional music policy, with the verified gain byte-span correction made unconditional |
| Scope | Shared native point-source music processing across scenes and BGM players; no scene allowlist |
| Preservation | Native instruments, authored filters and non-native gain, decoded source/rate, pan, distance volume, reverb, comb effects, BGM/SFX ownership, item/save grants |
| Configuration | MM Enhancements -> Fixes -> Clean Streamed Positional Music; default off |
| Verdict | Production-function and sanitizer checks passed; audible resolution remains untested |
| Recovery | Baseline retained; opt-in restores native positional effects when disabled |

The supplied `Fleet of Harkinian(20261001-182854).log` identifies scene 52 as Trading Post and scene 104 as Bomb Shop. Streamed codec-8 music acquires gain 127 and a filter after initially playing without them. Trading Post's Horon Village PCM has 7,323/14,400 saturated samples after gain, with none before gain. Bomb Shop also receives this processing with the earlier gain-buffer fix enabled. These are direct PCM observations, not an assessment of an unavailable WAV.

`Audio_PlayObjSoundBgm` uses flags `0x3F` for ordinary point-source music, and `Audio_SetSequenceProperties` maps bits `0x08` and `0x10` to the shared band-pass buffer and UQ4.4 gain 127 (7.9375x). Other callers use the same processing, including positional sub-BGM and fanfares. The new policy follows the exact shared native filter-buffer identity rather than an ambiguous reverse Audio Editor lookup or a shop-only scene list. It bypasses that filter and its gain 127 only for PCM/streamed codecs (`CODEC_S16`, `CODEC_OPUS`) when opted in. Authored sequence filters and native instruments are preserved. No asset or sequence registration flags change.

The policy runs while initializing effective sample states and before copying a released note's filter into its private buffer. Release tails therefore keep the policy selected at release instead of reintroducing native crunch. A checkbox change applies to active notes on the next audio update; already released notes retain their frozen attributes during their remaining fade.

The previous **Fix MM Audio Gain Buffer** and **Trace MM Shop Audio** widgets are removed. Gain counts now always consume eight s16 samples as sixteen bytes; legacy saved `MMAudioGainBuffer` values are ignored. This independently verified workspace-overrun repair is distinct from removing the native music treatment. Existing trace instrumentation remains available through the hidden `gDeveloperTools.MMAudioPCMTrace` setting, retaining its saved value and bounded capture behavior. Context logs identify the new policy and report the unconditional byte-span correction.

Validation covers the actual production note initializer and release path, all codecs, on/off, native versus authored filters, custom gains, synthetic/null guards and byte-identical remaining spatial attributes. Complete native mixer tests preserve adjacent DMEM outside each gain command, compare every 8-bit gain and representative rounded spans against independent wide arithmetic, and retain MM/OoT SSE2 parity. Existing complete MM audio C translation units, PCM resampling, passive diagnostics and worker tests also pass.

Small decisive runtime check: compare off/on in Trading Post and Bomb Shop, then a different native point-source music scene (for example Milk Bar or a spatial sub-BGM source) using streamed replacements. Check music clarity and loudness, distance/pan behavior, pause, exit/re-entry and ordinary SFX. With hidden tracing enabled, affected active notes should show effective gain 0 and filter 0 while opted in. Native instruments and other authored filters should retain their original processing. This candidate is pushed for runtime validation, not accepted or promoted as a master.
