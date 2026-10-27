# Elemental spin styles candidate — October 23, 2026

Both games have an always-visible **Spin Style** dropdown alongside the sword
graphics controls, and a separate **Elemental Spin Effects** checkbox (on by
default). A sword imbue is not required to select a style.

| Value | Style | Appearance |
| --- | --- | --- |
| 0 | Native (default) | Current native/mod spin; full Fire/Ice/Light material treatment when imbued |
| 1 | Fire | Preserved Fire Reborn design |
| 2 | Ice | Native geometry with icy materials |
| 3 | Light | Native geometry with celestial materials |
| 4 | Lightning | Preserved Lightning design |
| 5 | Arcane | Preserved Arcane Level 2 Circle design |
| 6 | Astral | Each game's preserved Astral Rupture design |
| 7 | Dark Energy | Violet wisps, crimson inner glow and sparse red sparks |

Custom styles keep their base appearance and can receive Fire embers, Ice
crystals or Light sparkles from the sword. Turning Elemental Spin Effects off
removes those elemental layers. It does not disable an imbue's combat effect.
Selecting Fire or another style never grants an elemental hit by itself.

Every style follows the existing cosmetic editor entries during charge and
burst. OoT retains its Level 1/2 Primary and Secondary controls; MM retains
Spin Slash/Great Spin Charge and Burst entries. Private material RGB blends
those tint channels through texture alpha while preserving the original
alpha/fade equations. Dark Energy reverses that color ramp for a crimson core
inside violet wisps. Editor changes continue to apply live to the active effect.

Preferences are captured when a spin actor starts, so a dropdown change applies
to the next spin. Charging previews the current sword imbue on gameplay updates;
pause freezes that preview. Successful powered release captures the actual
imbue for appearance and combat through the fade. Native magic checks, costs,
charge thresholds, damage, radius, timers and Great Spin progression remain in
their original branches. Ordinary swings keep their existing free imbues.

Eligible owners are the local player's regular swords with magic acquired.
Sword beams, Gerudo cone spins, transformed/custom weapon owners, Four Sword,
Trident, Cane and foreign players keep their existing behavior. Original attack
flags are recovered only for the exact live spin cylinder in the same context
and scene; element bits alone never establish ownership.

Private resources coexist under `objects/combo_spin/`. Astral uses separate
`astral_soh` and `astral_mm` names to prevent cross-game collisions. They are base
resources, so both Alt settings can resolve them. No global native resource is
patched. Native elemental materials clone the currently selected native DL into
frame-owned memory, retaining its geometry and UVs. Missing dependencies,
invalid root DLs or unsupported native custom branches retain the native draw.
Accents use the native frame arena and XLU pass with a combined cap of 12 regular
or 24 great particles, including Dark Energy's sparks, and consume no gameplay RNG.
Charging uses 4/8 smaller elemental accents in the blade's native charge frame;
that pass stops at release so the burst retains the full 12/24 budget.

The October 24 correction replaces the original untextured diamond accents
with native scrolling flame, 3D ice-fragment and billboarded sparkle resources.
Dark Energy's crimson sparks also use the native sparkle. Native textures,
geometry and blend modes are retained without modifying shared resources or
the parent ribbon's scroll segment. See
[native accent correction and verification](elemental-spin-native-accents-20261024.md).

The five unchanged input archives, every entry hash, generated file hashes and
the bounded OoT Astral charge-vertex repair are recorded in
`tools/spin_styles/manifest.json`. Reproduce the resources with:

```sh
python3 -B scripts/assets/build_spin_style_resources.py --inputs /path/to/preserved-inputs --check
python3 -B scripts/diagnostics/run_spin_style_tests.py
python3 -B scripts/diagnostics/run_spin_style_tests.py --sanitize
```

The runner defaults to all five gates. Individual `--policy`, `--resources`,
`--render`, `--native` and `--combat` gates remain available. Fast source checks
run resource verification and the complete normal/sanitized gate; existing
regression groups are retained.

Source parent: `21d278219bf95dfbf5f0b5e9b21c324b3a7cc6d8`.
Implementation milestone: `699ab0daf39c9a6dd31e15a8f0d0ee3efc68943f`, tree
`18b3955215d8903ab3555cf4ce9b436301fdaf18`. Delivery commits may add verification
and review fixes; the recovery ZIP's `source-ref.json` identifies its exact
final source head/tree and per-file hashes. The bundle has been restored in an
independent clone without hardlinks or alternates, verifying the complete tree
and all changed bytes. Other chats' scene/title/passage work is excluded.

This is a source candidate. Native fixture execution and compilation do not
establish an executable build, configured gameplay proof or user acceptance.
Keep accepted masters and develop unmerged until those separate checks are met.

## Gameplay acceptance matrix (pending, both games)

| Check | Expected result |
| --- | --- |
| No magic / empty meter / denied consumption | Native unpowered/failed behavior; no new element |
| Regular and great charge/release | Original size, damage, timing and cost; MM unlock retained |
| Native + Fire, Ice, Light | Full elemental polish matching sword hits |
| Arcane + Ice; all eight styles without imbues | Selected style remains independent; Ice accents optional |
| Astral and Dark Energy at both levels | Intended artwork, readable fade and sparse particles |
| Elemental Spin Effects off | Base style retained; sword combat still elemental |
| Change dropdown during charge | Active spin stays consistent; next spin uses new selection |
| Pause, equip another imbue, resume, then change during fade | Preview resumes with new imbue; released effect stays fixed |
| Alt off/on, missing pack, custom native DL | Correct resources or clean native fallback; following draws intact |
| Scene exit/reload, beams, Gerudo/custom models | No stale ownership; excluded attacks unchanged |
| Frozen/stunned repeats, representative enemies, Baba/grass | Native damage/classification plus audited elemental reactions |

Observe frame time and visibility alongside dark player models during this pass.
