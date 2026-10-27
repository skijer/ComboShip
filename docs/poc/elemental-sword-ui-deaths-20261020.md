# Elemental sword UI, independent cheats, and finish presentation

Baseline: published PR45 source `613802447cf8a4fbc3e058ba084f9e3dd7b9f7ae`.
Candidate branch: `poc/elemental-sword-ui-deaths-20261020`.

cor approved the design in chat on October 20, 2026, then clarified that
an element-immune enemy must still take the sword strike's normal damage.
The supplied selector screenshot also establishes that new layout and smooth
typography are required in addition to replacing the arrow icons. A second
screenshot reports incomplete Biggoron frost casing; its diagnosis is included.

## Preservation and scope

- Fire/Ice/Light arrow acquisition alone unlocks the corresponding normal
  A-button equipment choice. Each sword retains its saved element and explicit
  None, including progressive inheritance. The existing save layout is unchanged.
- Existing damage checkbox identities/locations become independent cheat
  overrides, usable without arrows, Din resources, or Alternate Assets. Enabling
  one clears the others. Disabling it restores the saved normal choice. Legacy
  preferences containing several enabled flags resolve deterministically to
  Light, then Ice, then Fire, without stacking. Any subsequent checkbox toggle
  clears the other override flags, so turning the active cheat off cannot reveal
  a hidden legacy cheat.
- Normal sword damage, multipliers, cutting, drops, soul gates and zero-magic
  swings remain authoritative, including when the elemental row is immune/inert.
- Selector typography, layout and dedicated fire/ice/light artwork are replaced.
  The same artwork marks the effective B-button sword enchantment.
- Native enemy finish effects are reused where available. Added presentation
  must never delay actor destruction, progression, or cleanup. Exact body
  encasement before an actor's immediate disappearance cannot be promised by a
  global health hook; copied effect anchors must not retain freed actor pointers.
- Concurrent native forest and transformation-camera candidates are preserved
  separately, not silently included in this candidate.

## Evidence so far

The access regression failed on the unchanged core at arrow ownership with
checkboxes off. The override-menu regression failed with multiple effects still
active. After implementation, all four real-header core/combat variants pass
(solo OoT/MM and Combo OoT/MM) with address/undefined-behavior sanitizer checks.
The actual checkbox helper passes last-enabled/off/unrelated-option controls,
including turning off preferences containing several historical enabled flags.
The first-arrow receipt regression failed with the switches off, then the real
native/custom pickup/queue/decoder fixtures pass after decoupling tutorials from
cheat switches. Both hosts' existing save roundtrip/malformed-data tests pass.

Environment: dependencies come from the earlier native-camera dependency cache
using CPLUS_INCLUDE_PATH; the first broad baseline command stopped because its
historical nlohmann include path no longer exists. No production failure was
inferred from that missing dependency. Leak sanitizer is disabled for this
container; address and undefined-behavior checks remain enabled.

No configured gameplay, audio, Windows/full application build, user acceptance
or master promotion is claimed. Source, native compilation and offline drawing
evidence below do not establish those acceptance stages.

## Selector/HUD milestone

The selector uses a rounded translucent panel, four visual choices, smooth
Montserrat font strips, active/selected/locked states and A/B control chips.
Fire/ice/light share dedicated generated art with the B-sword HUD. Preserved
source PNGs and the generation manifest are in the repository. Icons have
64x64 physical detail split into native-safe32x32 RGBA32 tiles; font/chrome
IA8 loads remain <=4096 bytes. No separate archive is needed for this UI.

Fresh selector/input/render/TU checks pass in both hosts and both solo/Combo
modes. The B-HUD and real MM HUD-editor anchor/scale tests pass under ASan/UBSan.
The generator --check passes deterministically. Source-menu native objects
compile in solo and Combo SoH. Both full HUD translation units have real-header
compile proof from the implementation pass. Offline previews are reconstructed
from the actual emitted GBI, not engine captures; the offline None sword icon
is omitted because its game-resource texture is not provided to that helper.
Configured appearance and full pause-overlay composition remain unproven.

The follow-up UI review reproduced backward texture sampling at one- and
two-pixel badge sizes: Fast3D interprets the step as signed S5.10. The helper
now clamps to INT16_MAX, and the sanitizer suite decodes and checks the actual
emitted signed steps. Generated font strips use the cross-platform asset
alignment macro; generator output stays deterministic under clang-format14.
The final HUD and four-mode selector checks pass after these corrections.

## Elemental finish implementation

Both hosts record accepted hits from the player's exact elemental melee
collider, then confirm destruction during that actor's update through native
health/fatal/kill paths. Another attack in the same update cannot inherit stale
sword credit. Confirmed presentations retain copied collider bounds and world
positions, never actor/collider pointers after deletion. Scene, room, transition,
pause, expiry and actor teardown are handled separately from native gameplay.

Audited native elemental deaths retain their own effect. Other confirmed sword
kills use native fire, ice-fragment envelopes or light-orb rendering. Light uses
MM's DEADLIGHT held loop with projected stable source positions and the existing
MM audio bridge in OoT. This adds no health mutation, delayed destruction, extra
magic charge, drop RNG calls, native death-state replacement or progression
change. The original damage-table fallback still supplies ordinary sword damage
against elemental immunity.

Narrow adapters cover flags-only and delayed deaths: OoT Bubble, King Dodongo,
Barinade's Bari and Flare Dancer fire; MM Bubble, Real Bombchu, Gyorg fry, Gomess
bats, Majora's Mask, Nejiron and the ranch alien variant. Adult/larva Peahat
deduplication and signed boss overkill are observed without changing their native
behavior. The source audit and exact adapter boundaries are retained in
tests/elemental_swords/finish_coverage_audit.json.

Copied envelopes are a fallback presentation, not an exact skeletal attachment
for every enemy. The pools hold2048 hits/effects; rendering is bounded to32
points per frame, checks native graphics-arena room, and rotates between effects
under load. These limits preserve native rendering work and do not prove that
every simultaneous kill has a complete envelope in every frame.

## Biggoron investigation and open runtime check

The user's clarification makes Alternate Assets OFF with the built-in authored
Biggoron model the primary reported case. It remains unreproduced in the current
source controls. The real authored sword selector, base-resource route,
per-frame compound copy, exact fit identity and captured transformed hand matrix
have been checked in both hosts and solo/Combo builds. An offline depth/alpha
projection of the exact exported core and tight shell covers the upper blade
as well as the base at eight blade-roll angles. This is not an engine capture
and does not prove that the user's installed archive has the same resources.

Two independent defects were established and corrected: original ModernDin
Biggoron aliases require their own exact blade profile; and native imported
hand inspection must pair the drawn resource pointer with its own bounded
command count, resolving MM's Biggoron hand each draw after asset switches.
Neither correction is claimed to resolve the clarified built-in screenshot.
The first22 published aura profiles and retained original input assets remain
unchanged; the two original ModernDin profiles are appended.

Set gDeveloperTools.SwordAuraDiagnostics=1 for opt-in fit records. The built-in
MM human authored Biggoron path should report logical=6, fit=6, rig=2,
matched=1; the old ModernDin full/broken fits are22/23. The configured runtime
still needs the actual selected fit, build identity and loaded asset identities
checked against this candidate before this bug can be marked fixed.

The base-resource owner is cached from the installed stock soh.o2r archive;
loose repository XML is not read by this owner. A code-only rebuild therefore
does not establish that the installed authored blade and grip match this source.
The exact authored asset source identities are retained in the aura manifests.

## Final source verification and recovery

The bounded review has no remaining source findings after explicit pause and
Play_Destroy teardown,32 sound requests per gameplay frame (including repeated
draws and pause/resume), native graphics budget guards and idle actor guards.
The Biggoron sanitizer/native/depth matrix passes in all four modes. The final
teardown-inclusive finish matrix passes in all four modes and compiles38 full
native objects. The complete16-command elemental gate passes. Its optional
native C++ ice-arrow resource bridge was then explicitly enabled with --include
and also passes in solo/Combo builds. Both native source menus compile in solo
and Combo modes. All44 changed files selected by CI formatting rules pass exact
clang-format14.0.6; deterministic generators, whitespace and asset-collision
checks pass.

The full elemental gate initially caught a missing identity-helper include in
its extracted MM hand fixture; the shared fixture now includes the real helper.
The broader project gate caught its crash-owner fixture's missing actor-observer
boundary. That harness now uses typed observer functions and verifies active
crash snapshots across nested, successful and failed updates; its sanitizer run
passes. Neither repair changes production gameplay.

The broader run passed37 commands before that crash fixture, then the repaired
fixture and five continuation commands. It stops at reward-GI asset packaging
because cmake is unavailable on PATH. Remaining broad commands are not claimed
to pass. An earlier ImGui discovery failure was resolved using the existing
pinned ImGui source via IMGUI_DIR. Exact commands, log hashes, passed scopes and
the dependency blocker are indexed in elemental-swords-verification-20261020.json.

The new finish core has full ASan/UBSan checks. Host fixtures exclude shift-base
only for the untouched native signed-bit damage-flag idiom; address and remaining
undefined-behavior checks stay enabled. Leak checking is disabled under the
container's tracing restriction. No full application or Windows executable was
built here.

Keep this as an isolated source candidate. The reported authored Biggoron
coverage, actual selector/full-overlay appearance and enemy visual/audio
responses still require a configured gameplay pass. In particular compare the
actual fit diagnostic and installed stock asset graph to the expected source
identities, then inspect full/broken/progressive blades, all elements, element
immunity, pause/resume and room/scene teardown. No develop merge or accepted
master promotion is part of this checkpoint.
