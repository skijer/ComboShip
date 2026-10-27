# Zora fin color, audio lifetime and cosmetic resource candidate

Baseline: ComboShip PR #41, `7511aedad2e21ce22e9c3fea1f01782d475e04f0`.
The user reported that the recently added swimming ribbons look good. Their
geometry, positioning, lifetime and alpha remain unchanged. Combat timing,
collision, barrier damage, magic cost, lighting and native MM audio are protected.

## Code changes

- Effects > Trails adds **Zora Fin Trail** on both hosts. OoT ribbons have a
  dedicated effect type and retain their existing white default; MM identifies
  native fin animations separately from punch/kick and preserves its own native
  palette/alpha on Reset. The existing optional generation sync includes this row.
- OoT blur cosmetic state and Reset palettes are stored per effect. Reset restores
  the exact authored gradient, including cyan thrown Zora fins and the axe trail.
- Shield and fast-swim hums use MM's refresh-driven sound IDs. Bank freshness and
  request dispatch advance once at native graph-frame `Audio_Update`, under the
  shared audio mutex. PCM/sequence synthesis still runs every audio callback.
  Reservoir prefill now holds that same mutex while producing a batch.
- Post-action cleanup stops swimming hums during release, idle, dolphin flight,
  interruption and land exit. Land exit clears native underwater filter/ambience
  and submerged flags. Reset, scene initialization, form exit and human Zora-swim
  exit stop the matching sound IDs.

## Eye and tattoo evidence — unresolved in runtime

The supplied files were inspected without modifying them:

| Source | SHA-256 |
| --- | --- |
| 1- 3DS Link - Custom Colors(2).o2r | `176e12da4e154b0e55e3107621011362bb9a46f4ce2a8ff3e297b9945ab80de2` |
| zzMikau Link update.o2r | `65fa422ae6df3d781bb28fb1b06673bf31aae04a7bcd9c16cc8dea9d4cfb2d43` |
| zzzz_Expressive_Zora_Goron_CustomColors_POC2.o2r | `6526b3aea15c4915ead151a25759d3946e4fa95e0be9c5abc93354dd09673250` |

The base matches the earlier supplied base byte-for-byte. Its arm UVs intersect
the tattoo mask. All eight eye RGB frames, closed-eye protection, scanner
bindings and actual encoded GBI/combiner behavior passed offline checks. Segment
0x08 eyes use normal Alt-first resource resolution; no forced-vanilla bypass was
found. These checks do not reproduce the user's socket/arm appearance in game.
The actual active resource owners and load order remain unverified.

No guessed replacement archive was made. Bounded `[ZoraCosmeticDraw]` logs record
textures actually used by triangles, archive ownership, Alt state, alpha coverage,
shader and prim/env colors. `[ZoraCosmeticBinding]` logs record the relevant
picker/material command bindings. Each probe emits at most 64 distinct samples
per scope; it does not modify assets or shading. The material evidence checkpoint
is `Zora_Material_Bindings_Evidence_20261014.zip` (SHA-256
`5173f2ce27b0f18af0e7f7c3e66fa198e2517ab152355ef6e91524c5242f20ed`).

## Verification and runtime handoff

`tests/oot_zora/run_presentation_tests.py` executes production blur/init, MM fin
selection, barrier/swim cleanup, native bank processing and actual mixer prefix
under ASan/UBSan. It uses three audio callbacks per game tick, verifies one fresh
loop start, one-shot completion, pause, alpha/Reset and fading underwater punches.
The original loop flags, callback-driven lifetime and authored-gradient failures
were observed before corrections. The worker fixture independently proves prefill
serialization and preserves graphics wake, sample cadence, mute and restart at
divisors 1/2/3; its new case fails the baseline and passes the candidate.

Existing Zora combat, barrier, custom cosmetic and MM cosmetic regressions are
part of validation. Compiler syntax checks and independent code review remain
separate from full CI/build and gameplay proof. Local sanitizer runs disable
LeakSanitizer because process/thread enumeration is unavailable in this runtime.

Runtime checks:

1. Change **Zora Fin Trail** to a conspicuous color, swim with both fins, attack,
   then Reset. Check vanilla and Alt assets; punch/kick colors stay independent.
2. Hold the shield on land/in water, fast-swim, release A, dolphin-exit, climb or
   walk onto land, then re-enter water. Verify no repeated ticks or land ambience.
3. With the actual packs/order used for the reported defect, change **Zora Eyes**
   and **Zora Tattoos** separately. Capture the appearance and matching Fleet log
   so drawn owners/alpha masks can be compared to the source evidence.

This is an implemented/source-verified candidate. Eye/tattoo rendering remains
unresolved; audible/visual acceptance and promotion to `develop` are not claimed.
