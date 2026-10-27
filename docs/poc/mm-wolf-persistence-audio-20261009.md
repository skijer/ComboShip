# Wolf scene persistence and supplied audio

Normal MM scene initialization and Player replacement previously cleared Wolf
selection. The host now releases scene-owned runtime resources while preserving
the selection, then silently restores Wolf on the arriving Player. Explicit
detransform, disabling Wolf, native mask ownership, death, save/load, title and
file changes still reset it. MM's Wolf enable setting remains independent of
OoT's transformation-mask master setting.

Both hosts use twelve original PCM recordings supplied for this change:

| Action | Recording |
| --- | --- |
| Enter Wolf | TP_Transform_Wolf.wav |
| Return Human | TP_Transform_Human.wav |
| Bite | Lunge1 through Lunge5 |
| Aerial attack | JumpAttack1 and JumpAttack2 |
| Charged spin | Charge_Attack_A1 through Charge_Attack_A3 |

`combo/assets/wolf_link_sfx_sources.json` records source names, original sample
rates, durations and hashes. The generated PCM preserves the supplied samples;
the post-synth mixer resamples to the native 32 kHz output without trimming or
normalizing the recordings. Each host owns independent audio state. Game-thread
hooks publish pause/session/volume settings; the audio thread only consumes that
publication and immutable clips. Two voices allow transform and attack overlap,
and stereo additions saturate rather than overflow.

Wolf attacks replace Link's attack voices while retaining physical impact sounds.
Actual transformations trigger their supplied cue once; scene restoration is
silent. Human exit audio survives form cleanup and freezes during pause. Both
hosts respect native master and SFX settings, including ComboShip's untouched
40% master default and finite fallbacks.

No dedicated paw-chain recording was present in the supplied pack. Grounded
movement uses a quiet native Hookshot Reflect metal one-shot at 10% cue volume,
with distance and cooldown gates. It stops when idle, airborne, swimming,
blocked, damaged or cleaned up. `gMods.WolfLink.ChainVolume` controls its gain.

## Evidence and remaining proof

The persistence, native voice, missing chain and default-volume regressions were
observed before their fixes. Production Wolf core/host fixtures, both-host audio
fixtures, real standard/HD asset lifecycles and native syntax checks pass. Ten
audio cases cover voice selection, chain state, transformations, mixing and
volume defaults across the two hosts. Independent review also verified all
twelve PCM clips against the supplied WAV files byte-for-byte.

The main regression gate includes Wolf audio and incremental mod discovery.
OoT's CMake glob now detects the new audio hook in existing build directories.

These checks establish source/CPU behavior. Actual OoT transformation-state
callback execution, full application linkage and a configuration-specific game
session remain separate evidence: walk across MM scenes as Wolf; confirm the
new bite/transform balance and subtle chain cadence; pause a Human exit tail;
check standard/HD and Alt Assets enabled/disabled; confirm intentional session
and native-mask resets.
