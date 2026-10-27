# MM back equipment and OoT sky compatibility candidate

Baseline: ComboShip cumulative integration `639b526150898cee98ffdd09ccfa6ccd6a40f8ba`
on `integration/shared-items-soh-20260921`.
Candidate: `poc/mm-back-equipment-reloaded-sky-20260922`.

The existing hide-equipment option was present only in the OoT side of
ComboShip. The supplied OoT Reloaded Sky pack likewise contains no MM sky
resource names. This candidate implements both missing MM-side paths.

## User settings

Both options are off by default and live in **2Ship → Enhancements → Graphics → Mods**:

- **Hide Back Equipment and Scabbard** hides the sheath limb after native and
  adult/custom-form equipment overrides, and suppresses separately drawn NEI
  back shields. In-hand items remain visible. The pause doll uses the same
  native override/post-limb callbacks.
- **Use OoT Sky Textures** selects the original OoT pack's clear and overcast
  dawn, day, sunset, and night faces. Copy `zzReloaded Sky.o2r` into
  `mods/2ship`, enable **Use Alternate Assets**, enable this option, and enter
  another scene. Restart after adding/removing archives. The source pack is
  unchanged and does not need conversion.

Inspected source pack SHA-256:
`e349be49b332f8d85a49ae78477154468c1aa6ab0cd469ba3701f57ebdd6d8fd`.
It contains 55 files, including the 40 outdoor sky faces used by this change.
No pack or game archive bytes are committed to the repository.

## Boundaries and preservation

The sky path requires all 40 Alt faces; a partial/missing pack uses the original
MM renderer. Disabling either the option or Alternate Assets also returns to
MM's existing native/Alt lookup. Enabling after scene entry takes effect on the
next scene load. Only normal outdoor and regional-gloom skies are admitted;
special/cutscene sky rendering remains native.

The colored OoT art bypasses MM's grayscale sky-palette tint. Its time blend
follows MM's daily schedule. A second pass is used only during partial cloud
cover so time transitions and the existing weather fade can coexist. Native
weather/cloud selection and optional weather dimming remain active. Display
lists are rebuilt only when the pair of time variants changes; resource
availability is checked at scene initialization. Native geometry, UVs, native
fallback lists, scene rotations, stars, filters, and MM's moon code are retained.
The original pack's shared sun texture replacements can still resolve normally.

All existing shared-item, audio, rain, crash-guard, font, cosmetic, and SoH-side
features remain in the parent history. No gameplay, collision, inventory,
progression, actor placements, or source texture bytes are changed.

## Evidence and acceptance

- Equipment and sky regressions execute production rendering functions with
  real engine types and GBI commands. Settings, asset lookup, allocation,
  matrices, and unrelated engine services are fixture boundaries.
- The sky regression reproduced the missing renderer on the baseline, then
  passed clear/cloudy time variants, simultaneous blends, Alt/option fallback,
  missing/partial packs, scene reentry, and native geometry/list preservation.
  With `--pack`, every emitted sky resource is checked against the original
  archive's file list. Address/undefined-behavior sanitizer checks also pass.
- Existing MM weather regressions pass, including spin lighting, story weather,
  sky bindings, palettes, gloom, filters, stars, and weather/music coexistence.
- Both new suites are in the CI gate before Windows/Linux builds.

Full Windows/Linux build status is tracked in the candidate PR. In-game visual
acceptance remains pending; headless draw verification is not a screenshot or
gameplay test. Check native and custom Link with sword stowed/drawn, held shield,
pause/unpause, adult/custom forms, NEI shield, and option off/on. Check Clock Town
and Termina Field in clear/rain weather at dawn/day/sunset/night, Alt off/on,
scene reentry, and an OoT↔MM round trip. Verify sky seams, tint, smooth blending,
the MM moon, and rain/music continuity with the user's complete mod stack.

This is an implementation candidate, not a newly accepted master. The parent
commit remains the recovery point.
