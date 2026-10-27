# Native elemental spin accents — October 24, 2026

Source candidate only. Executable gameplay observation and user acceptance remain
pending in OoT/MM, standalone/Combo, with both Alt settings.

## Cause

The implementation in the reported build constructed untextured triangle fans
in `SpinAttackAccentsDraw.h`. Fire and Ice had four spokes, making their actual
silhouette a diamond. Light had eight alternating spokes. Dark Energy reused
the Fire fan with a crimson tint. `G_CC_SHADE` supplied interpolated vertex color
and alpha; there were no UVs, native effect textures or camera billboards.

The original intended accents were Fire embers, Ice crystals and Light sparkles.
The former structural tests checked counts, colors and arena submission, so
they did not establish those visual identities. A new regression first failed
against that implementation because no native effect geometry was submitted.

## Correction

Only the accent draw/material/model path changes. The deterministic particle
builder, placement, sizes, lifetime-derived alpha, style policy, preserved base
spin resources, blade imbue and combat code are unchanged. Normal/great burst
budgets remain 12/24 including Dark Energy, and charge budgets remain 4/8.

| Accent | Native resource | Geometry/material | Orientation |
| --- | --- | --- | --- |
| Fire | `gEffFire2DL` | Two textured triangles; 32×64 flame and 32×128 scrolling mask | Upright camera-facing flame, centered on the existing particle origin |
| Ice | `gEffIceFragment1DL` | Actual 14-triangle shard; native texture, lighting and reflective texture generation | Deterministic 3D tumble in the parent burst/blade frame |
| Light | `gEffSparklesDL` | Native 16×16 intensity sparkle mapped to a two-triangle quad | Rotating full billboard |
| Dark Energy sparks | `gEffSparklesDL` | Same native sparkle, existing crimson tint and separate 4/8 allocation | Rotating full billboard; original deterministic spark motion |

Each host resolves its own existing `gameplay_keep` DL and retains that DL's
texture hashes, UV-bearing vertex resources, triangles, combiner and blend
commands. No native asset is copied into the custom resource pack or modified.
Native `EffectSsEnFire_Draw`, `EffectSsIcePiece_Draw` and
`EffectSsKirakira_Draw` provide the source patterns.

A small frame-owned DL clone replaces only embedded color commands and the
native segment-08 scroll call. Particle color/alpha is set before each draw,
and scroll calls point directly to a private native-format interpolated scroll
list. The parent ribbon's segment 08 is never overwritten. Native ice uses
per-particle look-at data from the native highlight calculation.
Texture animation advances once per gameplay tick, using the native flame/ice
scroll steps. It does not reuse the burst's rapidly advancing motion phase;
that would skip animation frames and cause flicker. Pause freezes this clock.

Particle origins are transformed by the complete current parent matrix, so
actor scale, Great Spin growth and the hand/blade charge frame are retained.
Flame and sparkle bases use the native camera billboard with the parent's axis
scales. Native shard geometry is normalized by its actual 16-unit bound, flame
by its 8000-unit half height, and sparkle by its 14-unit half width. The CPU
matrix is not mutated; the parent GPU matrix and native XLU setup are restored.
The charge/burst glue also explicitly restores the selected base primary and
environment colors after a successful accent pass, including edited cosmetic
colors and the original alpha. An omitted pass does not emit color resets.

The complete pass is validated and allocated before submitting any GBI. Missing
or malformed DLs, uninspected child/matrix commands and insufficient frame arena
space omit the accent pass without partial rendering or global resource edits.
No effect actors, effect-pool allocations or gameplay RNG are introduced.

## Evidence

| Check | Result |
| --- | --- |
| Regression against original shade fans | Expected failure: no native effect DL submitted |
| Policy, private resource closure and both native-header render fixtures | PASS, including ASan/UBSan |
| Native effect texture/vertex/triangle/combiner/blend preservation | PASS for both host fixtures |
| Alpha, 12/24 and 4/8 budgets, crimson spark identity, live charge selection, cosmetic base colors | PASS |
| Camera/3D transforms, nonuniform parent scale, exact charge origins and parent matrix restoration | PASS |
| Whole-call base primary/environment/alpha restoration, including edited colors | PASS after an independent RED→GREEN regression |
| Native interpolated scroll steps; no parent segment writes; resource/allocation failures | PASS |
| Actual native archive DL cloning under ASan/UBSan | PASS for both games: flame 2, ice 14 and sparkle 2 triangles |
| Complete modified spin actor C translation units | PASS in OoT/MM, standalone/Combo |
| OoT native state/combat fixtures | PASS in standalone/Combo, ASan/UBSan |
| Full existing native/combat runner | Blocked in unmodified MM `din_fire_sword.c`; see below |
| Executable build, actual in-game appearance/performance, user acceptance | Pending |

The native archive checks read unmodified local inputs. Their whole-archive
identities remain in the private recovery evidence; the inputs and their
fingerprints are excluded from publication. The checks inspect each host's
actual native effect display lists and referenced texture/vertex resources.

The particle builder remains SHA-256
`e77d9ef7fb37b289edbe4a4b7928ece6de4f8a2a91d77175df3b4455037eea90`.
An isolated before/after donor visualization was also reviewed; it is source
asset evidence, not a game capture. Ice preview lighting is illustrative.

The default native runner initially could not read unmaterialized CMake prefix
files in this checkout. A scratch harness read locally available copies whose
git blob IDs exactly match this worktree's index. Both OoT native variants and
their complete actor compilations passed. The MM combat fixture then stopped
on the unchanged Din sword module's cross-host resource header (`SoundFontSample`
unknown) and missing `CVAR_ENHANCEMENT` macro. Those failures were not hidden or
fixed as part of this rendering correction. Both complete MM spin actor TUs
were separately compiled with their native standalone/Combo flags and passed.

Runtime acceptance must inspect all elements during charge, regular/great
release and fade; custom base styles and edited cosmetic colors; Alt off/on;
camera movement and edge views; pause/resume; and frame time. Native source and
fixture checks cannot establish the resulting in-game appearance.
