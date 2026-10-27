# Elemental spin styles verification

Source baseline: `21d278219bf95dfbf5f0b5e9b21c324b3a7cc6d8`.
Isolated candidate: `poc/elemental-spin-styles-20261023`.
Runtime and user acceptance: **pending**.

| Evidence | Result |
| --- | --- |
| Complete five-gate runner, explicit flags, normal and ASan/UBSan | PASS |
| Default runner (all five gates) | PASS |
| Real spin Init/Charge/Destroy and exact live ownership | PASS in OoT/MM, standalone/Combo |
| Complete modified spin actor translation units | Compile PASS in all four configurations |
| Real GBI material cloning, bounded accents and fallback | PASS for both hosts |
| Deterministic generation from all five original input archives | Byte-exact PASS |
| Complete private dependency graphs and texture metadata | PASS |
| Repository custom asset collision gate | PASS, no new differing shared paths |
| Existing spin GI tests, normal and sanitized | PASS |
| Both existing spin texture importer tests | PASS |
| CI failure/routing tests | 21 tests PASS |
| Existing elemental sword regression script | PASS, complete rerun after fixing baseline fixture stubs |
| Full executable build / in-game visuals and behavior | Not performed in this checkout |

RED→GREEN evidence included the absent policy, renderer, native integration and
independent unimbued-sword gate. The cross-game collision check rejected five
Astral paths before separate host names resolved them. Original archives were
not modified.

The initial existing sword regression run stopped in the OoT damage fixture:
its boundary stubs lacked `Audio_StopSfxByPosAndId` and
`SoundSource_PlaySfxAtFixedWorldPos`. An independent untouched baseline checkout
reproduced both linker errors. The MM damage fixture also lacked its host's
fixed-position sound stub. Only those fixture boundaries were added; both real
damage suites then passed, including native multipliers, drops, cutting and
audited reactions. Sanitizers run with leak detection disabled because this
execution environment is traced; address and undefined-behavior checks stay on.

The private recovery checkpoint includes exact refs/trees, all changed file
hashes, input hashes, a git bundle, a text review patch and restoration steps.
Restoration was tested from a separate baseline clone without alternates or
hardlinks. Final head/tree identities are in that checkpoint's `source-ref.json`.

One independent whole-branch review found no critical issues and two important
presentation gaps: imported shaders did not consume OoT secondary colors, and
custom charges did not draw their live elemental preview. Both were reproduced
by failing tests and fixed in one pass. The resource test now decodes every
shipped combiner and checks both RGB tint channels plus unchanged alpha muxes.
Native material clones receive the same two-channel treatment. The real-header
renderer checks edited charge/burst colors for all eight styles and both levels.
It also compiles the actual actors' charge-draw helpers and verifies Fire →
paused Ice → resumed Ice → released Ice vertex colors, ownership and particle
caps. Released charge draws add no preview particles. The complete five-gate
suite passed with ASan/UBSan after these fixes. The final normal run and resource
reproduction are recorded in the delivery checkpoint.

No minor findings were deferred. Appearance, Astral parity, frame time and the
cross-host/Alt gameplay matrix require configured runtime observation. Unrelated
scene/audio/shop/title/passage behavior remains outside this candidate's scope.
