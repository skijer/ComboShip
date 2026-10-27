# OoT transformation cosmetics candidate

Baseline: ComboShip PR #5, `1071265310da03b33475a4fd0f8c32db06d59249`.
This candidate adds to the integrated OoT side. The baseline rain, audio,
item compatibility, shared-item logic, geometry and archives remain unchanged.

## Changes

- Cosmetic Editor > Mods groups loaded tagged materials by Child, Adult,
  Deku, Goron, Zora and Fierce Deity. Existing custom CVar keys and manifest
  category/entry names remain intact. Only forms with available entries appear.
  The user's 3DS Custom Colors manifest also uses `object_link_goy` for Goron
  materials; skeleton/category identification covers that custom directory.
- Mod colors participate in Randomize All and the existing automatic modes.
  On New Scene generates fresh colors on each scene initialization, including
  reentry. Seeded modes retain their repeatability, and locked colors stay put.
  Global reset/lock/rainbow controls include the same custom entries.
- Rescanning uses color-command position instead of original RGB equality,
  and refreshes CVar string pointers after moving/sorting entries. Alt changes
  rescan bindings. Cached barrier copies receive live tagged color updates.
- Zora barrier land/idle-water scale now matches MM's intensity-dependent
  scale, also used by fast swimming: `intensity * (10 / 51) * 0.01` in world
  space. At full intensity the land effect is twice its former linear size.
- Cosmetic Editor > Effects > Magic Effects adds Zora Magic Shield,
  Zora Shield Glow and Zora Shield Highlights. The native two-layer shader
  keeps its alpha, primitive LOD fields, texture scroll and geometry.
  These controls share the existing Zora shield mod's custom color keys, so
  its Mods controls and the native effect controls stay consistent.
  Unrecognized custom shader layouts retain their own tagged controls.

The shield change affects visuals only. Magic drain, damage collider, movement,
immunity, sound and environment lighting are not changed. It also applies to
the existing human Zora-tunic barrier, which shares the same draw function.

## Verification and acceptance

Focused production-code fixtures cover scanner bindings, grouping, rescan,
randomization/locks/seeded modes, cached-copy colors, and the barrier's matrix
scale and emitted color commands. Both runners are included in the CI gate:

```
python3 scripts/diagnostics/run_oot_custom_cosmetics_tests.py
python3 scripts/diagnostics/run_zora_barrier_cosmetics_tests.py
```

These fixtures replace resource/graphics services; they do not render the game.
Full Windows/Linux builds and runtime acceptance are separate evidence.

Runtime check: enable the relevant form packs in the OoT mod set; inspect each
loaded form's Mods group; change and reset one color; use On New Scene and leave
and reenter twice, also checking a locked color. Test Zora R+B on land and R
underwater, the three shield colors, reset, charge/fade, and an Alt-assets toggle.
Keep the prior build until the candidate is accepted.
