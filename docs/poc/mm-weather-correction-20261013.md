# MM weather correction candidate — 2026-10-13

Based on current PR41 commit `4cf4866d2dd33551e8a1372a5cce8bd7ad40196e`.

## Source behavior

| Default season | Rain and sky presentation |
|---|---|
| Spring | Persistent rain and soft overcast on Days 1, 2 and 3 |
| Summer | Clear |
| Autumn | Intermittent gentle rain; cloud cover follows its own wet/dry cycle |
| Winter | Overcast; native rain and lightning presentation suppressed |

Explicit per-season rain modes, manual season selection and the independent
outdoor-rain override retain their controls. Native precipitation targets,
lightning state, bolt slots and gameplay RNG continue progressing underneath.

The actual OoT cloud getter normalizes ordinary dry MM CLOUD artwork and
composes the explicit overcast fraction, including its fade and dry phases.
Native rain, target-only Song of Storms, snow and story/view boundaries retain
their native cloud blend. The native sky renderer and binding loop are unchanged.

Native flash, thunder PORT0 and bolt geometry have separate presentation masks.
An ambient sidecar tracks native START additions and the conditional END fades.
Full ambient replacements clear it; additive bomb fades and fog-only/early-return
lighting retain it. Player transformation snapshots preserve and restore it.

## Local verification

Targeted production fixtures failed before their corresponding corrections:
Spring Day 2, native rain during Autumn's dry phase, Summer's frozen native END,
ordinary dry OoT CLOUD, and explicit overcast fade over ordinary CLOUD.

- `run_mm_weather_tests.py` passes normally and with ASan/UBSan. Fixtures execute
  the actual sky getter, environment draw/strike/bolt paths, original RNG,
  ambient composition, bomb light statements and player interpolation/snapshots.
- The runner checks ten native C translation units against the real headers,
  including every changed actor ambient writer and player integration.
- `run_mm_oot_sky_tests.py` passes normally and with ASan/UBSan: 18 renderer draws
  and 131 native-star ownership checks. Its renderer fixture supplies cloud policy;
  the weather fixture tests the real resolver.
- Native season-selector and winter atmosphere/controller checks, clang-format 14
  and whitespace checks pass.
- ASan runs use `detect_leaks=0`; these results do not include leak testing.

## Runtime acceptance pending

Use the user's actual archive order and selected OoT sky artwork to check Field
and Clock Town at noon/night, Spring across all three days, Autumn through a wet
and dry cycle during native Day 2 weather, Winter overcast, and Summer/Off during
an active native strike. Check bomb flashes, spin lighting, mask transformation,
Song of Storms onset and snow boundaries. Configuration-specific visual acceptance
and the complete game build remain separate from the local fixture evidence.
