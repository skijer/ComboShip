# Seasonal world-lighting correction

Baseline: draft PR #41 source `3ca3f24c0dc13819f2114d0c50bfccb9140f103a`.
This follow-up corrects the excessive seasonal world darkening, rather than
only documenting it. Accepted develop remains unchanged.

## Problem and correction

Autumn and Winter selected the native cloudy weather palette and additionally
subtracted seasonal RGB from ambient light and both directional lights.
This changed the scene and Link independently of seasonal surface colors.
Night protection still allowed half the available native light to be removed.

The native lighting bridge now returns a neutral offset. The extra seasonal
shade tables and night weighting are removed. The scene's selected clear or
cloudy weather palette supplies lighting without that second subtraction.
This applies at all clock phases, not only at night. Native rain/overcast can
still change the palette; the correction does not force sunny lighting during
cloudy weather.

The clear/cloudy sky requests and native fades, rain/audio, seasonal surface
colors, Spring grass strength, particles, Winter mist/aurora, native lightning,
bomb flashes, room-actor lighting adjustments and saved/manual season ownership
are preserved. No MM code, sky texture, scene archive or save payload changes.

## Source verification

A stricter production test first failed at displayed ambient-light equality
against the scene's actual native palette on the baseline. After correction,
the full weather suite passes normally and with ASan/UBSan. Its native palette
matrix covers 400 combinations: Native/Off and four seasons, manual/saved
selection, five clock phases, independent rain on/off and four seasonal rain
modes. It requires exact native ambient and both directional colors, then
checks native sky/light restoration, saved state, lightning and bomb writers.

Thirty-two additional production measurements cover dawn, noon, dusk and night
for Native, all four season defaults, Spring rain with clear/cloudy skies, and
Autumn with rain disabled. Every measurement retains the native ambient color
and has zero extra seasonal ambient/directional subtraction.

The same Hyrule Field native palette control at 20:45 records:

| Presentation | Baseline ambient | Corrected ambient | Baseline moonlight | Corrected moonlight |
| --- | --- | --- | --- | --- |
| Dry Spring / Summer | `40,40,70` | `40,40,70` | `50,50,100` | `50,50,100` |
| Rainy Spring, overcast | `70,70,90` | `70,70,90` | `30,30,80` | `30,30,80` |
| Autumn | `35,35,56` | `70,70,90` | `15,15,54` | `30,30,80` |
| Winter | `35,35,70` | `70,70,90` | `15,15,66` | `30,30,80` |

The production particle suite also passes with ASan/UBSan, including actual
rain-controller coexistence and native effect/environment ownership.
Independent code review found no critical or important issues.

## Acceptance boundary

These are production source measurements using the previously frozen Hyrule
Field palette, not values extracted from the scene in the supplied clip.
Full platform build results belong to the exact follow-up commit's hosted run.
Installed appearance requires the same location/time/packs comparison. The
separate Gerudo black-sky resource/configuration cause and deferred Kokiri
archive investigation are not claimed resolved. No visual previews were
rendered for this correction, and no develop/master promotion is implied.
