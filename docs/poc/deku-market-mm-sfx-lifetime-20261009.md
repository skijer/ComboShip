# Deku Market unpause crash: isolated MM SFX resource lifetime

Status: source candidate with sanitizer and compiler verification. Windows build,
configuration-specific runtime proof, and cor's acceptance remain pending.

## Report and baseline

- Report: daytime Market as Deku Link, pause, assign Ocarina of Time to D-pad Up,
  then crash immediately after unpausing.
- Crash log: `Fleet of Harkinian(20261009-234439).log`, exception at
  `18:43:37.389`, `0xc0000005`, scene `SCENE_MARKET_DAY`.
- Runtime: ComboShip `c480df33b050350d211713335d8b379986ad6b07` (`c480df3`).
- Candidate branch: `fix/mm-sfx-font-lifetime`, isolated from source parent
  `c0181856968dd6f3869be0d1aa240743091fd6a8`.
- The original synth-loader blob is identical in the crash commit and this
  source parent: `396436c09cc696f3809c6bb74445e7213d7501fd`.
- Pending afternoon merge work and accepted baselines are unchanged.

## Evidence and diagnosis

Disassembly of the matching Windows `soh.dll` places the fault at RVA
`0x1264EF3`, the instrument-envelope read in `mmsfx::AudioScript_GetInstrument`.
The instrument pointer in RCX is invalid (`0x009AA000988E0095`). This is a
source-function match by disassembly, not PDB symbolization; the nearby export
names printed by the crash logger do not identify the actual function.

The log records the isolated SFX engine loading fonts 0/1 and Sequence_0 at
`18:43:29.767`–`18:43:29.826`. MM BGM registration then registers those fonts at
`18:43:31.202` and `18:43:31.243`, before the crash at `18:43:37.389`.

The original loader obtained globally cached soundfonts and shallow-copied their
raw instrument/drum/SFX pointers. `RegisterMmFonts` later unloads the same font
paths. `AudioSoundFont` destruction frees those arrays and instruments, leaving
the isolated engine with dangling pointers. A sanitizer control reproduces the
use-after-free through the production instrument lookup and resource destructor.

The D-pad assignment/unpause is the reported trigger. The evidence does not
establish a separate D-pad or Deku instrument-state defect; no `Gakki enter`
record appears before this crash.

## Narrow correction

- Load fonts 0/1 through existing `MmSfx_LoadFont`, whose MM archive cache retains
  the resource owners and samples independently of global resource unloads.
- Load Sequence_0 through a small `MmSfx_LoadSequence` helper using that same
  retained MM archive cache. Music's global replacement/remapping cannot mutate
  or release the isolated engine's copy.
- Preserve initialization retry/idempotence. Asset loading stays on the game
  thread. Existing MM sample patching and pitch-range corrections remain in the
  established private font loader.
- Register the focused lifetime regression in the existing shared-item gate.

## Verification

| Check | Result and limit |
| --- | --- |
| Original loader control | ASan heap-use-after-free in `AudioPlayback_GetInstrumentInner`, reached through `AudioScript_GetInstrument` |
| `python3 -B tests/mm_sfx_lifetime/run_tests.py` | Six lifetime/service cases and seven native ResourceLoader parser cases pass under ASan/UBSan; global metadata cannot replace the MM owner, while MM-local aliases retain their contract |
| Lifetime fixture | Full production synth loader, actual font/sequence destructors, production private-cache helpers and instrument functions; archive parsing, the existing font/sample-patching helper, and engine startup are controlled service boundaries |
| Changed translation units | Native-header syntax succeeds for both `mm_asset_loader.cpp` and `mm_sfx_synth_loader.cpp`; this is not a full build/link |
| Nearby audio checks | Leaf cold/initialized sound-stop regression and MM worker checks pass; native MM and OoT mixer checks report zero mismatched samples |
| Format and patch checks | Changed production files formatted with clang-format 14; whitespace check clean |
| Windows gameplay | Not run; the supplied crash configuration remains the runtime gate |

Independent source review found no Critical or Important issues. Its optional
Minor follow-up is a sample-backed fixture through the production font-patching
helper; this suite directly proves font/envelope and sequence ownership, while
sample-owner retention is traced through the unchanged private loader.

## Runtime acceptance gate

Build the candidate with the same archives, save and active PAK configuration as
the crash. Start from a fresh process so font registration runs again.

1. Enter daytime Market and transform into Deku Link.
2. Pause, equip Ocarina of Time to D-pad Up, and unpause several times.
3. Walk for at least ten seconds after each unpause, then press D-pad Up, play
   notes, exit the instrument and repeat. Check Deku movement/voice sounds and
   instrument audio as well as crash absence.
4. Re-enter the scene and repeat after a fresh launch. Check normal Link,
   Goron and Zora audio because they share this isolated MM engine.
5. Repeat the relevant vanilla/active AltAssets configurations; record exact
   build, archives, PAKs and logs. No visual or audio acceptance is inferred from
   static checks.

Only the source candidate is ready for integration review. No accepted master
promotion or pending afternoon merge integration has been performed.
