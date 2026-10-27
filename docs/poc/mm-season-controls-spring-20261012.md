# MM seasonal controls, spring blossoms and shared bank tint

| Field | Record |
| --- | --- |
| Source baseline | PR #41 `bridge/audit-merge-20261008`, `d87316a76792698faf0c2633be9e22b5c6f64e53`; local `6651f97a21c36490f3f377e6f3f7341d778e9e7f` has the exact same source tree `9f5daf74b3b12514568803ea9084cbf417297424` |
| Runtime evidence | cor accepts the inspected October 12 winter Termina clearing screenshot. Its build and pack order are not embedded, so this is visual acceptance of that view rather than proof of every source/configuration combination. |
| Candidate | `poc/season-bank-20261012`, isolated from the preserved baseline |
| Scope | Exact shared rocky-bank texture; selective spring foliage and petals; MM Audio Editor season/effect controls; optional winter flurry motion |
| Preservation | Accepted autumn and winter recipes, native-green summer/ground, bark/rock/alpha, geometry/collision/UVs, native weather state, ownership/progression, renderer resource replacement guards and restoration |
| Configuration | MM outdoors with existing story/interior/underwater gates. Normal, `_scene` and explicit Alt resource aliases are covered where registered. Actual installed packs/load order remain a runtime boundary. |
| Status | Implemented and focused build/static verified; application build and new in-game presentation remain unproven; no master promotion |

## Selective spring and the bank resource

The supplied Prelude entry is
`scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_027050`.
Its cached texture receives grass-only autumn/winter copies in any eligible
scene that uses this identity. Normal and generated scene namespaces, explicit
Alt paths, native raw RGBA5551 and RGBA32 replacements are covered. Cold assets
remain owned by the renderer load path. Non-green rock/bark pixels, every alpha
byte/bit, source pixels and texture metadata remain exact. No archive is edited;
no additional bank textures with unrelated identities are inferred.

| Spring surface | Palette and blend |
| --- | --- |
| Actor tree canopies, upper canopy and distant tree wallpaper | `F6C1D9`, 224/255 blend, mean-preserving mode 2 |
| Middle tree wallpaper | `E8AACB`, 204/255 blend, mean-preserving mode 2 |
| Liftable grass/bush blades | `F6C1D9`, 32/255 blend, mode 2; native fade alpha retained |
| General ground, mixed bank/stump/arch grass, tree trunks and cut stumps | Native palette |

Spring has its own immutable display-list cache. All three seasonal caches
share validated renderer pointer observations; grass skipped by spring is
reconsidered when switching to autumn/winter. Summer and Native restore the
original resources. Accepted autumn and Termina winter palettes/lifts are
unchanged, including the softer winter treatment outside Termina.

## Audio Editor and weather composition

MM Audio Editor → Audio Options exposes **Native Weather**:
Follow Game / Rod (default), Native, Spring, Summer, Autumn and Winter.
Manual selections bypass Rod ownership checks without granting items,
unlocking seasons or changing the saved gameplay selection. Native removes
the seasonal overlay. Existing story, indoor and underwater restrictions apply.

The four expandable sections retain independent saved settings:

| Season | Controls |
| --- | --- |
| Spring | Rain mode; cherry blossoms |
| Summer | Atmosphere enable; drifting fluff; nighttime fireflies; sunbeams |
| Autumn | Rain mode; leaves |
| Winter | Snow; windblown flurries (off by default) |

Spring/Autumn rain offers Season Default, Off, Persistent and Intermittent.
Season Default preserves the legacy spring day schedule and autumn/native
composition. Explicit choices suppress native rain/lightning presentation and
ambience while retaining the live native source state, giving Off a dry view
and Intermittent actual dry periods. The pre-existing Outdoor Rain setting
remains an independent override and can still add rain; its priority is
explained in the editor. Disable that override when assessing seasonal Off.

## Petals and winter flurries

Spring reuses the accepted autumn distribution of 96 particles and the same
single supplemental-owner lifecycle. A distinct 13-vertex rounded/notched petal
mesh uses four pink/ivory materials, private cosmetic randomness and a gentle
extra flutter. Autumn artwork, density and motion remain unchanged. Spring
restores the production XLU setup once after a drawn layer so later textured
draws remain valid; disabled/all-culled layers leave incoming state intact.

Winter flurries reuse the existing 64 snow slots and add deterministic wind
offsets. The option adds no actors or RNG calls. Disabled flurries retain the
baseline winter motion/randomness; snow toggles preserve native actors,
precipitation targets and fog restoration.

## Verification and limits

Meaningful regressions first failed for the missing bank tint, selective spring
scene/actor tint, no-Rod manual selection, independent summer effects, spring
petals and the petal texture-state cleanup. Final focused checks cover:

- Scene materials: actual forest/middle/backwall command fixtures, all three
  seasonal transitions, both namespaces/native/Alt, renderer pointer writebacks,
  scene-specific frost, outside cache edits, native-green spring ground and
  exact rock/alpha preservation for RGBA16/32 bank copies.
- Real texture ownership: original buffers/metadata/alpha and malformed-input
  guards, with the real `Fast::Texture` implementation.
- Tree/grass actors: canopy-only and OPA/faded XLU scopes, native trunks/stumps,
  selected replacements, native impact/drop behavior and Off/Summer cleanup.
- Weather: native state/ambience/sky preservation, per-season rain, paused
  selector changes, manual seasons without progression, legacy gated Rod
  behavior, independent global overrides and scene/room/story/view exclusions.
- Native seasonal particles: 96 actual petal-mesh submissions across Termina
  and the five outdoor Clock Town districts; private spring RNG; ownership,
  toggles, pause/re-entry, native snow/fog restoration, and following textured
  XLU draws after full/culled/disabled petal layers.
- Summer production state/renderer: independently toggled fluff/fireflies/beams,
  existing emitted firefly opacity, depth/command/matrix budgets and reset.

These suites pass with AddressSanitizer/UndefinedBehaviorSanitizer; the native
particle runner also enables bounds checks. Leak detection is disabled because
this environment cannot inspect the process data it requires. Existing grass
controller-macro redefinition warnings remain. Clang-format 14, whitespace,
changed-file conflict and asset-collision checks are separate final gates.
Independent review found the petal texture-state issue; its correction and
regression leave no unresolved review findings.

Full Audio Editor translation-unit compilation is blocked locally by missing
`imgui.h`; the platform build must validate it. Historical summer preview parity
is also blocked by absent `tools/summer_weather/export_motion.cpp` and
`verify_preview.cjs`. The summer state/renderer segment was run separately;
that is not a claim that the historical preview comparison passed. No game or
GPU backend was launched here, and no new spring/flurry runtime acceptance is
implied.

## Focused runtime review

With the current packs and matching candidate build, select manual Spring in
MM Audio Editor without the Rod: inspect actor trees, middle wallpaper/canopy,
subtle liftable grass, green ground and pink/ivory petals. Test blossom/leaf/snow
checkboxes while paused and moving; switch Spring → Autumn → Winter → Summer
→ Native and re-enter the room. Compare the bank's grass edge in autumn/winter
while its rock stays intact. Compare winter flurries off/on, retaining the
accepted frost. Test seasonal rain modes with the independent Outdoor Rain
override disabled. Follow Game / Rod must resume the saved gameplay season.

Inspected evidence SHA256:

- Winter screenshot `image(20261012-151425).png`:
  `119747f640f2c1d9ccd0ff5ee1b3b636ad07eaeaa09ecd119e68499e1b5db78c`.
- Prelude bank entry `image(20261012-152324).png`:
  `17b0923ba5e0b7d0beeb9c83a1d6f61b040328d7c7a055087facaec03522848a`.
