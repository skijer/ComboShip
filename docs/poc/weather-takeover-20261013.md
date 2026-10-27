# Weather takeover candidate — October 13, 2026

Parent: PR #41 `bridge/audit-merge-20261008` at
`4cf4866d2dd33551e8a1372a5cce8bd7ad40196e`. Cor reports that this build boots;
Termina's winter appearance remains the accepted visual baseline.

## Recovery and scope

Recovered and combined the unpublished weather/lightning commit `783262c8`,
the exact 34-file visibility manifest for local checkpoint `c015651c`, and
the five-file Woodfall foliage patch from its separate worktree. Original
checkpoints remain intact. The combined source retains the boot/audio fix,
51 additional Hyrule Field identities, stronger spring foliage, consistent
outdoor MM frost, both aurora camera-placement changes, clearer MM mist and
bounded OoT autumn/winter night shading.

The MM weather correction preserves native strike/bolt/RNG evolution under
seasonal presentation masks, native ambient-write ownership and transformation
snapshots. Its default Spring rain covers all three days; Autumn has its own
wet/dry cycle, Summer is clear and Winter is overcast. Manual/per-season rain
controls and the independent outdoor-rain override remain available.

## Woodfall material correction

Recovered targeting includes the loaded Woodfall scenery object, three shared
leaf identities, selective two-layer green wall/detail surfaces and the exact
paired Road-to-Swamp forest surface. Immutable originals preserve brown/stone
pixels, texture companions, alpha, metadata and Native/Off restoration.

Independent review identified a brown stem embedded in the native CI4
`gWoodfallSceneryBranchAndLeavesTex`. Whole-triangle grayscale would recolor
that stem. A new production fixture first fails on that unwanted wrapper.
The correction uses green-only copies of the native RGBA16 TLUT and RGBA
replacement image. Native CI4 indices remain untouched. The real native TLUT
loader verifies the emitted palette; spring leaf brightness is retained and
included in the copy-cache key, including shared-source wall/leaf aliases.

The combined fixtures retain the accepted full winter palette globally and
all 567 unique native scene/object cases. The Woodfall object's 24 mixed-image
triangles now receive no whole-triangle tint; their leaves use the selective
palette/image path instead. Separate bark and the embedded stem stay native.

## Evidence and remaining gates

Verification is recorded for the combined source, rather than inferred from
the earlier isolated checkpoints. Thirteen recorded focused suites pass:
MM weather (normal and ASan/UBSan), sky, selectors, winter atmosphere, native
foliage/grass, surface copies, focused scene materials and the actual-archive
scene renderer; OoT particles, actor draws and texture/cache/archived pixels.
The real OoT weather/native-light controller also passes its season, clock,
independent-rain, flash/release and restoration matrix. The 22 Hyrule Field
pixel cases include the four protected atlas bounds; all 567 actual MM cases
pass in autumn and winter, with native/Alt and both scene namespaces.

The final surface-copy fixture separately passes with the sanitizer flag
actually consumed by its runner. The new stem regression passes on the
actual native CI4/TLUT pair and on raw RGBA replacements; a shared-source
wall/leaf control catches brightness-cache aliasing. Production MM foliage
compiles against real engine/library headers. Clang-format 14 checks all 41
changed C/C++ files, Python syntax and whitespace checks pass, and there are
no unresolved merge entries. Local ASan disables leak detection; no leak
result is claimed. Independent review found one important stem issue, fixed
through the failing production regression and these passing checks; no other
critical, important or minor finding was reported.

Full Windows/Linux compilation and packaging
belong to the resulting hosted candidate run. No installed-pack/GPU acceptance,
leak result or develop/master promotion is implied.

The spring nighttime blackout remains unresolved. The existing Weather Audio
Diagnostics checkbox now captures bounded, read-only lighting snapshots. A
Spring/Native comparison at the same location and clock remains the decisive
runtime control. This is a feature candidate, not a diagnostics-only build.

Native MM control archive: SHA256
`17ca3a4be1726b3080c5e7ea923e0673cd55ec560439d77478b7663663087c6a`.
Supplied Woodfall pack: SHA256
`fde399add585ebaffa2524d8965b8b2351034c740cef98a0faa844a469e42e06`.
No scene/archive, geometry, collision, UV, scrolling or save bytes are changed.

## Hosted gate follow-up

Build run `38485534667` on `61d86c35` stopped in the accepted NEI/MM pickup
step before either platform build. Its complete native-season fixture still
expected Spring to use enhanced rain only on Days 1/3 and Autumn to borrow
native Day 2 rain. The first obsolete assertion at line 1193 was reproduced
locally. Those expectations predated the recovered controller's current
three-day Spring default and independent Autumn wet/dry presentation.

The correction changes only that test and this record. The same real native
actor/environment/audio paths now verify Spring density, rain gain and
overcast on all three days; raw Day 2 storm/precipitation targets continue;
Autumn's dry phase masks native rain/ambience/darkening; and Off restores the
native rain, ambience and lighting. The complete fixture passes normally and
with ASan/UBSan, including its particle projection, native snow/fog, paused
wheel, actor reuse, lifecycle and scene/story/underwater controls. Existing
assertions remain active; no production weather or visual code is changed.
