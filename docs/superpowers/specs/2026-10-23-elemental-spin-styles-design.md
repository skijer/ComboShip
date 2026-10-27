# Elemental spin polish and live spin styles

## Agreed behavior

cor accepted the two-layer approach on October 23, 2026: the dropdown selects
the base spin appearance; the equipped sword adds optional elemental polish.
This extends the existing sword imbue system to powered spin attacks in both
OoT and MM. An unenchanted sword can use every visual style.

| Control | Values | Default |
| --- | --- | --- |
| Spin Style | Native, Fire, Ice, Light, Lightning, Arcane, Astral, Dark Energy | Native |
| Elemental Spin Effects | Off, On | On |

The dropdown is visible independently of elemental-arrow ownership, sword
imbues, and the elemental damage override checkboxes. These are visual
preferences, not item grants or unlocks. The existing game still decides
whether Link can perform a powered spin.

| Selected style | Imbue | Elemental effects | Appearance |
| --- | --- | --- | --- |
| Native | None | On | Existing native/mod-selected spin |
| Native | Fire | On | Native spin geometry with flame materials and embers |
| Native | Ice | On | Native spin geometry with frost materials and ice crystals |
| Native | Light | On | Native spin geometry with white-gold materials and sparkles |
| Arcane | Ice | On | Arcane shape, textures, and motion with frost accents |
| Any custom style | None | On | Selected style by itself |
| Any style | Any | Off | Selected style by itself |

Fire uses flame/ember accents, Ice uses cyan frost/crystal accents, and Light
uses white-gold glow/sparkles. Custom styles keep their distinctive base
materials; the elemental layer adds a separate glow/particle pass. The Native
style receives the fuller elemental retexture originally requested. Charge,
normal release, great release, and fade must read as one consistent effect.

cor additionally requested the existing Astral variant in the shipped version
and a new ominous/evil energy style for dark custom models. Astral must preserve
the existing **Astral Rupture** design. The builder proposes **Dark Energy**:
dark violet wisps, a crimson inner glow, and sparse red sparks. Bright edges
must keep the effect legible against dark scenes and dark player models.
Both are independent cosmetic styles, with the same optional elemental layer.

The actual hit element follows the sword's existing imbue, including its
existing progression gates and explicit override controls. A Lightning visual
does not grant lightning damage. Turning elemental visuals off does not turn
elemental combat off.

## Approaches

1. **Two cooperating layers — selected by cor.** Preserve manual style choice
   and add elemental accents. Native can receive the full elemental material
   treatment; custom variants retain their recognizable designs.
2. **Automatic elemental replacement — considered, not selected.** Simpler
   resource selection, but an imbue would replace a manually selected style.
3. **Independent style only — considered, not selected.** Easy visual selection,
   but it would leave the original sword-system polish incomplete.

The selected approach gives each control one purpose while allowing both
features to operate together.

## Preservation contract

- Preserve native spin damage, charge thresholds, size/range, timing, magic
  requirements/costs, and Great Spin progression. Add no elemental surcharge.
- Ordinary sword swings remain free and retain their existing imbues, damage,
  cutting, HUD badge, equipment selector, and per-sword saved choices.
- Apply the existing audited elemental enemy responses to the powered spin's
  owned collider while preserving its original attack classification and
  numeric damage/multiplier. Unrelated colliders and genuine elemental arrows
  retain their own rules.
- Sword beams, Fierce Deity attacks, Gerudo cone spins, Four Sword, Trident,
  Cane, transformed forms, and foreign/network players retain their current
  custom behavior. This candidate targets the eligible local regular sword.
- Preserve both hosts' native/Alternate Assets paths and cosmetic controls.
  Native plus no elemental layer follows existing resource/mod selection.
- Preserve current Great Spin acquisition/shop GIs; gameplay style selection
  does not replace their existing resource ownership or rendering.
- Leave scene, weather, shop, title-card, audio, and passage work outside this
  candidate.

## Implementation decisions proposed by the builder

These details fill in the accepted behavior without adding gameplay systems:

- Use shared saved CVars `gEnhancements.Graphics.SpinStyle` (0 through 7 in the
  displayed order) and `gEnhancements.Graphics.ElementalSpinEffects` (default 1).
  An invalid style resolves safely to Native.
- Snapshot the style and visual toggle for each newly created spin actor.
  Changing preferences applies to the next spin; an active charge/release
  finishes consistently. During native gameplay updates, charge accents preview
  the currently active sword imbue. Pause freezes that preview with animation;
  changing equipment while paused is reflected on the first resumed update.
  Resolve and snapshot the actual imbue at successful powered release. Release
  visuals and combat then keep that element through fade, even if equipment
  changes afterward. A visual setting cannot change combat ownership.
- Store native/original and applied attack flags on the owning spin actor.
  Recover them only for that actor's exact live collider; never infer ownership
  from element bits alone. Scene teardown must leave no pointer cache.
- Keep custom assets in each host's private `objects/combo_spin/` namespace.
  All supported variants coexist; no global texture or native DL replacement
  is needed when selecting a style.
- Reuse the existing Fire, Lightning, Arcane, and Astral Rupture designs. Adapt their resource
  references per engine and retain both charge levels. Ice and Light use the
  native spin shape and compatible existing elemental material resources.
- Add at most 12 accent particles for a regular burst and 24 for a great burst,
  submitted in the existing effect draw with no new world actors. Derive motion
  from the effect lifetime rather than consuming gameplay RNG. Pause freezes
  their animation; fade uses the native effect lifetime.
- Missing or incompatible private resources fall back to the existing native
  spin without altering combat. Custom variants must work with Alt off and on.

## Source and input identities

Candidate branch: `poc/elemental-spin-styles-20261023`.
Source parent: `21d278219bf95dfbf5f0b5e9b21c324b3a7cc6d8`, fetched from
`marsh6487/ComboShip`'s published `poc/runtime-shop-audio-20261023` branch.
This is a source baseline, not a declaration of accepted gameplay. Parallel
uncommitted scene/title/passage edits were excluded by using a separate worktree.

Resolved existing inputs (metadata located; bytes must be inspected and hashed
before copying any resources):

| Design | Input | Persistent file identity |
| --- | --- | --- |
| Fire | `zzzz_MM_Spin_Fire_Reborn_HD_POC2.o2r` | `libfile_b903d021a49081919ff1a5eee24c7a39` |
| Lightning | `zzzz_MM_Spin_Lightning_HD_POC1(1).o2r` | `libfile_8bf27d64419c81919310b313e0b0f319` |
| Arcane | `zzzz_MM_Spin_Arcane_Level2_Circle_POC4.o2r` | `libfile_93bff2a18a3481918cb996380adac09f` |
| Astral (MM) | `zzzz_MM_Spin_Astral_Rupture_HD_POC1.o2r` | `libfile_aed78ecbdcac8191948cb1bf4cfa89d3` |
| Astral (OoT) | `zzzz_Spin_Astral_Rupture_HD_Alt_POC1.o2r` | `libfile_6ebf5120028c8191b108bc713bc03e04` |

The unchanged input archives remain recoverable. Published historical hashes
in `docs/poc/boss-soul-spin-gi-20261002.md` are comparison evidence, not a
substitute for verifying these exact input bytes.

## Verification and acceptance

Test the production style resolver, both native spin actor branches, scoped
damage ownership, actual resource graphs, native/Alt selection, magic failure,
charge/release/fade, live preference changes, and render-state cleanup. Run the
existing sword state/damage/aura/finish and spin GI/texture regressions. Compile
affected units for standalone and Combo configurations and run clang-format 14.

Gameplay proof remains separate: compare Native/None against this baseline,
then Native Fire/Ice/Light and a custom-style/imbue combination in both games,
including Astral and Dark Energy at both charge levels, with Alt off/on.
Include no magic, an empty meter,
pause/resume, a style change, scene teardown, repeated elemental enemy hits,
and grass/Baba cutting. Check effect visibility, visual clutter and performance.

Keep the candidate isolated until these checks are recorded. Source checks,
compilation, and publication do not imply runtime acceptance or promotion.
