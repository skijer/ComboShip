# Young Epona riding and cosmetics in both games

This candidate starts at ComboShip `56dcde31b355d71c7c58844c764c1ef04ce06b26`,
the latest cumulative MM rainbow-cache, persistent-drop, rain and unrestricted-item
candidate. Its parent Windows and Linux build workflow passed. It carries forward
the existing Midna POC3 and contents-based chest integration.

The riding donor is Shipwright's `integration/nei-weather-static-actors` at
`42ee604c2e30d7ff320f968e93ab57b33ad9fbbf`, verified against the live branch on
September 24. The original `f5a992078e89ea95ba55a55aaf20d537a272538e` port already
included its mounted-entry, mount-access and ranch-handoff fixes; the follow-up
adds child-object binding and the separate young-horse outdoor scene gate.
Only the SoH/OoT engine receives the riding port. MM retains its existing riding
behavior.

## Controls

| Engine | Location | Controls |
| --- | --- | --- |
| OoT | Enhancements > Quality of Life | Ride Young Epona as Child |
| OoT | Cosmetic Editor > World & NPCs > NPCs | Adult Epona and Young Epona: Coat, Eyes, White Hair |
| MM | Cosmetic Editor > Epona | Coat, Eyes, White Hair (Mane / Forelock / Hooves) |

Riding defaults off. Learn Epona's Song, enable the checkbox and reload the area.
An ocarina and the song's notes are required, including randomized-note checks.
Young Epona can enter, return to and answer the song in the donor's 29 outdoor
scene variants, including Kakariko, the Market and the Temple of Time exterior.
Adult Epona retains the original five-area gate. Native fences and movement
restrictions remain in effect. Young Epona's saved location is independent of
adult Epona's; the young riding actor does not spawn or draw in the adult
timeline. Existing saves initialize the new young-horse data safely. ComboShip's
merged-save callbacks remain intact.

The two editors use separate settings. Reset restores the original appearance.
Coat, eyes and white hair change independently; saddle and bridle controls are
outside this candidate. The ordinary young ranch actor uses the young cosmetic
settings too.

## Assets and compatibility

Put `Young_Epona_SoH_POC1_Assets.o2r` in `mods/soh`. It supplies eight privately
named riding/mount-animation resources and is unchanged from the SoH candidate:
SHA-256 `d1990518522c4b781a9b62f3b2df21e9aec5df0eae99542892093359ecab945d`.
MM's native Epona cosmetics require no additional archive.

The separate `TP_Epona_POC4_Cosmetics.o2r` is an optional replacement for the TP
adult Epona POC3 archive. Use it with this executable and Alternate Assets, and
disable the older TP Epona archive so only one version supplies the horse.
The original POC3 archive remains unchanged. POC4 retains its skeleton, skin
vertices, animations, source textures and twenty cached eye vertices. Its only
changes to existing resources are the body/head display-list path strings; new
private resources supply isolated color passes. The archive manifest records
exact checksums and preservation checks.

Native material support is restricted to verified display-list templates. HD
texture replacements using those UVs are supported; arbitrary replacement
geometry falls back to its original drawing. TP POC4 has an explicit separate
protocol. Color updates use small GPU commands with cached immutable masks and
display lists. They do not recolor HD texture pixels on the CPU or flush the
global texture/resource caches each frame.

## Other requested fixes

The latest donor's Zora shield feet anchor and animated swimming-root transform
were missing from this ComboShip parent and are included here. Its stronger
matrix fixture fails the old position and passes the corrected one. The Midna
movement, visible-time blinking, cosmetics and idle-yawn follow-ups were already
present, with ComboShip's independent opt-in gates retained. Existing chest and
Midna production diagnostics pass.

## Verification and acceptance

The build-artifacts gate includes the four riding production harnesses, young
asset bounds/dependency checks and cosmetic regressions. Local archive checks
also exercise the actual OoT and MM native horse resources. Affected full C/C++
translation units are checked against the real engine and dependency headers.
The candidate remains separate from `develop`.

Build/static checks and offline asset previews do not establish in-game
acceptance. Check the same save and mod order used by the prior cumulative
build: mount/dismount, song recall, fences and scene transitions; save/reload and
child/adult separation; each color and Reset in both editors; blinking, HD/Alt
Assets, and OoT to MM to OoT transitions. Confirm the TP POC4 eyes remain attached
through idle, gait, rear and jump animations. Check the Zora shield grounded and
swimming with the active model stack.

Rollback is the parent executable and prior TP archive, or disabling child
riding and resetting the cosmetic rows.

## September 24 riding donor follow-up

The follow-up is based on ComboShip `21a42b2de1dc783cea4cf475a23218e59543d585`
on `poc/young-epona-both-games-cosmetics`. Its three production-file changes are
the exact riding delta from Shipwright `f5a99207` to `42ee604c`; the horse actor
retains ComboShip's cosmetic include and BeginDraw/EndDraw hooks. Player code,
adult horse behavior, MM riding, animation assets and unrelated integrations
are unchanged by this port. The standalone Shipwright branch is unchanged.

Before porting, expanded production harnesses failed on Kakariko mounted entry
and the young actor's missing child-object bank binding. After porting, the
actor, 29-area entry/save/re-entry/song-recall, player and save-manager harnesses
pass. The actor fixture covers existing and absent child object banks and the
donor's negative allocation return guard. The save-manager harness uses real
JSON and disk round trips; the affected full C translation units compile, with
the existing Combo player macro/pointer warnings. Animation dependency/bounds
tests pass; the optional actual-archive test was not run for this code-only
follow-up.

The reported `6d4392c` Kakariko crash log contains no horse actor, consistent
with the old scene gate. That explains the missing mounted horse but does not
prove the separate invalid-display-list renderer crash is fixed. This remains
an implemented and statically verified candidate, with Kakariko mounted entry,
re-entry and song recall still requiring an in-game check under the same pack
order and Alternate Assets setting as the report. It has not been promoted or
accepted as a runtime fix.

## September 24 cosmetics and crash follow-up

The native recoloring overlay used `COMBINED, 0, 1, 0` for second-cycle alpha.
In the alpha multiplier slot, mux 6 selects primitive LOD fraction, not constant
one. The overlay sets that fraction to zero, making coat, eye and white-hair
overlays invisible in both engines. The corrected combiner passes first-cycle
alpha through the final addend. Adult intensity-texture sections used a separate
direct path, explaining the report that only parts of the adult legs changed.
TP POC4 already uses valid alpha pass-through and is unchanged.

A new regression runs the emitted commands through the production renderer's
combiner decoder and shader-input mapping. It failed before the fix (alpha 0.5
became 0.0) and passes afterward for opaque, masked and filtered pixels across
all seven nondefault color combinations. The native archive suites also pass:
21 OoT display lists / 168 variants and 11 MM lists / 88 variants, plus the MM
draw-wrapper checks for Alt resources, reset, blink masks and cached rainbow.

Both supplied `6d4392c` crash logs show ASCII bytes being executed as commands.
The first four invalid opcodes match bytes 3, 19, 35 and 51 of the child eye OTR
path, including `gLinkChildEyesRollRightTex` in the first crash. Inspection of
both active Young Din POC4 archives confirms that their head calls segments 8
and 9 as display lists; all eight eye and four mouth Alt resources are valid
display lists. Those resources and their animation calls remain unchanged.

The CPU resource loader can return a cached vanilla texture before checking an
uncached Alt replacement. This leaves the eye/mouth segment bound to an OTR
name while the separately loaded Alt head expects a display list. The shared
renderer now resolves OTR targets through the current resource manager before
raw or indexed display-list dispatch, requires a nonempty DisplayList resource,
validates offsets after resolving the segment base, and skips missing or
wrong-type resources. Both engines' CPU segment loaders preload the exact Alt
entry before the ordinary cached lookup, retaining the native and Alt resources.
A production-handler regression
reproduces the unsafe name dispatch before the fix and passes afterward,
covering call/branch behavior, asset type changes, raw pointers and offsets.
The cache regression compiles the real resource-manager cache/load decisions
and both games' segment wrappers; it covers a warm native texture with a cold
Alt display list, toggling, retained resources, HD texture paths and invalid
replacement types. Address/undefined-behavior sanitizers pass for the renderer
dispatch tests with leak scanning disabled because of this environment's ptrace
restriction; the CI gate runs the full sanitizer command.

These are regression results, not a full game runtime reproduction. Retest the
reported Alt toggle and child Kakariko entry with the same Din packs, then the
three Epona colors and reset in both games. Adult TP appearance and cosmetics
remain a separate in-game acceptance check.

The later report of a black scene after returning the Master Sword (adult to
child), with HUD or audio continuing, remains unresolved. Neither supplied
log's `6d4392c` sessions contains a Temple of Time entrance/return marker.
The native destination, age-switch helper and story-skip handlers match the
standalone donor; no cutscene or camera fix is included without a reproduction.
Capture a fresh log immediately after that sequence to investigate it separately.
