# ComboShip feature carry-forward candidate

Candidate branch: `recovery/magic-din-both-games-20260925`. Recovered from `poc/magic-din-both-games-20260925` at `115984a3449d74dc0eaaa49ec65fa3bcd7627ff4` plus its 105 pending source/test/report files. The original checkout and branch remain preserved.

The candidate starts at ComboShip `5e6e063a147f4b4ee0da130ca638b9b124abd4be`, the latest cumulative integration tip fetched for this task. That commit descends from `integration/shared-items-soh-20260921` at `639b526150898cee98ffdd09ccfa6ccd6a40f8ba`. SoH donor: `integration/nei-weather-static-actors` at `628deea3cf851af19a18f915e0665d772294c383`.

This is an implementation and build candidate. Gameplay appearance, audio, installed archive priority, and hardware-specific behavior require a run of this exact candidate with the intended packs and save. Earlier user reports do not establish acceptance of this candidate.

## Coverage

| Feature | SoH / OoT | 2Ship / MM | Verification focus |
| --- | --- | --- | --- |
| Din fire sword and fire checkbox | Donor implementation, including custom equipment/pedestal paths | Native and alternate adult hand adapters; owned melee flags and MM damage-table multipliers | Grass cutting, normal sword damage, target reactions, drops, toggle/off/missing assets |
| Din fire shield, SFX, item/icon and colors | Donor bracer/shield implementation | Hero shield/Deku skin bracer adapter; native blocking retained | Guard/fade/pause, default-off SFX, ownership, icon/get-item fallback |
| Six medallion arrows | Donor private texture pairs and independent colors | Native MM renderer, paired companion/native geometry selection, actual MM RGBA picker API | Bow and seed charge, release, flight, impact, live colors, missing/partial packs |
| Native ice snowflake | Donor POC2 charge/flight/impact | Native MM POC2 draw adapter | Full animation continuity, native fallback, gameplay lifetime |
| Fire/Water/Forest cast appearance | Donor cast bindings and Forest particle isolation | MM sphere/curve/particle adapters | Native geometry and scroll, private texture selection, live colors, Alt freshness |
| Elemental sounds and Demise lightning | Latest donor implementation | Native MM counterparts | Original off behavior, event timing, effect ownership and pool budget |
| HD effect texture import | Donor bounded normalization | MM native-name/format adapters | Legacy and current texture metadata, native dimensions, named references |
| Epona cosmetics and child riding | Retained cumulative integration | Retained native MM Epona cosmetics/riding paths | Defaults/reset, Alt masks, scene rules, old/new saves |
| Scene accessibility and persistent drops | Retained cumulative integration | Retained MM counterparts | Scene restriction dispatch and configuration defaults |
| Young pedestal orientation, arrival/exit | Imported donor scene/ceremony changes | MM Time Gate/adult renderer retained; Lost Woods pedestal is an OoT scene | Child grip, custom sword selection, transition cleanup |
| RPG generation and pickups | Full donor settings/items/pool/logic, exposed in shared Randomizer → OOT Randomizer → Stat Upgrades | Canonical cross-world delivery; no duplicate MM item identities | Saved seed settings, pickup caps, foreign delivery once |
| RPG runtime and persistence | Native seven-stat implementation with native magic floor | Seven-stat save mirror and native movement/combat/magic/HUD adapters | Kokiri speed composition, Ruby climb, repeated handoff, reload and cycle save |
| Midna per-event audio | Donor assignments and archive clip list | MM-specific assignments and menu, retains model-presence guard | Original/silent fallback, unavailable clips, persistence |
| House feather | Donor Link’s House pickup and normal-save message eligibility | Shared item ownership/foreign delivery retained; no Link’s House scene in MM | Ownership gate and message dispatch |

MM has no native crawling action; the crawl stat and its seed rules remain saved/shared and apply on return to OoT. Ordinary MM movement is not repurposed as crawling.

The existing opt-in randomized-cosmetics sync includes all 24 new medallion/Din color entries. MM reads its own `gCosmetic.<id>.Color` RGBA fields; OoT retains `gCosmetics.<id>.Value`.

## Save and randomizer rules

The bridge carries counters, required counts, enabled flags, adjustable magic total and quarter-heart mode. It merges counts with caps, never treats handoff as another pickup, and reads rules from the loaded seed. Native magic pickups guarantee their original 48/96 capacity in both games even when fractional RPG magic is enabled; RPG-derived ownership flags cannot skip the first native magic tier. MM’s existing native-magic logic therefore keeps its capacity guarantee.

RPG state is part of MM `Save.shipSaveInfo.nei`, including owl/new-cycle JSON and whole-save copies. Cycle resets leave these permanent fields intact. Fresh saves clear the state; legacy speed-only saves migrate explicitly.

## Assets

The existing `zz_Henriko_Ice_Arrow_Snowflake_POC2.o2r` was recovered from `Henriko_Ice_Arrow_Snowflake_POC2.zip`. Its SHA-256 is `7b172fc2a3cef98fc85992964dbbcb41e51b71d0dc733775a199afbcb840c25a`; its single key is `alt/custom/henriko_effects/arrows/ice_snowflake_poc2`. This is the existing 3DS-guided POC2 effect and artwork, not a newly recovered original 3DS asset.

Both resource managers need the relevant packs installed in their game’s mod path. Din equipment, fire layers, ice art and private medallion textures are optional resource contracts with fallbacks. The source does not redistribute game archives or silently replace the user’s configuration.

## Verification record

The independent source audit compared 197 donor-changed SoH production/asset files since `6f03c439`. Shared Epona headers were verified at their Combo-owned paths. The missing house feather and Midna assignment changes were restored; the remaining differences are Combo-owned adaptations.

- GCC 13.3 / CMake 3.28.3 / Ninja Release configuration passed.
- Shared libultraship compiled and linked; `ldd -r` found no missing dependencies or undefined symbols.
- Initial gate compiled 87 changed/affected production translation units with the real generated build commands.
- The interrupted GCC 13.3 Release build resumed through the complete `ComboShip` target (both engines, shared renderer, combined UI and launcher), exit 0. The three recovery fixes were compiled with the generated production commands against the recovered source tree, then both engines, UI and launcher were linked again with those replacement objects, all exit 0. The original source checkout and original build objects were left intact.
- Eleven focused suites passed in recovery: MM Din rendering, MM Din damage/reactions, OoT Din damage/reactions, Din shield, both Midna audio paths, house feather, time pedestal, shared RPG, shared items, and stat upgrades. MM magic/fire bindings also passed after the format gate. Prior effect-suite logs cover ice snowflakes, six medallion arrows, Forest dust, effect import and Demise lightning.
- Asset-collision gate passed with no new collisions; changed files satisfy clang-format-14 and `git diff --check`.
- Windows packaging and configuration-specific gameplay remain unverified locally.

CMake applies its existing `patches/libultraship-custom-tex-scroll-e3cd6591.patch` during configure. Those generated vendor edits are build inputs from the established baseline, not an additional feature commit.

## Recovery corrections and review

The recovery test run found an unfinished native-mine regression: the extra fire bit on a sword hit selected projectile knockback and torque. Both native MM mine routes now consult the owned original sword flags; real arrows and unrelated/copy colliders retain their native projectile route. The existing regression failed before the fix and passed after it, including air/water geometry and overlap fallback.

One final independent source review found two additional RPG defects. Native OoT magic grant paths now record the permanent 48/96-unit capacity floor, including foreign/shared grants and duplicate first-tier grants. Meter-capacity animation now preserves the saved remaining fill target on load and game handoff instead of granting a free refill. Added tests failed against the recovered source and passed after these changes. The remaining focused review found no further actionable source defect; visual/audio/runtime acceptance was explicitly outside that source review.

Recovery decisions: reuse the interrupted build and recompile/relink only the three changed production units, with source comparisons for every other file, to preserve completed work; publish a separate candidate branch so the prior cumulative baseline and interrupted checkout remain available. This is a reviewable candidate, not an accepted gameplay master.

## Runtime acceptance still needed

Use the intended child/adult models and Alt state in each game. Check Din guarding/idle/swing, grass and several enemy reactions, native and medallion arrows through release/impact, spell color changes during animation, Epona scene transitions, young pedestal arrival/exit in OoT, and a seed with RPG items through pickup, save/reload, game swap, and MM cycle reset. Include native magic before and after RPG magic. Record the exact build, save, active packs and configuration with any clip.

## CI gate follow-up — 2026-09-26

PR #15's first gate at `3bb84121f48ee551e24b29a939f49e025c77ee6d` failed while compiling the extracted MM HUD draw functions: the fixture lacked the new `ComboRpg_HeartCount` declaration and implementation. The HUD runner now links the real MM RPG adapter against fixture save state. A draw-command regression also checks that saved quarter-heart mode displays fractional capacity.

Running the checks skipped after that failure exposed two more missing fixture dependencies: the item-visuals fixture lacked `DinFireShield_DrawItem`, and the adult back-equipment fixture lacked the Din hand adapters. Both fixtures now include the production headers and provide typed, disabled-feature boundaries. The separate production Din module suite still checks enabled rendering, hand composition, ownership and native item fallback. No production behavior or CI gate was changed.

The HUD suite, all 21 commands after it, and the independent Din rendering suite passed locally, including the repaired item/back-equipment fixtures. The two renderer sanitizer commands required `ASAN_OPTIONS=detect_leaks=0` locally because LeakSanitizer cannot run under this container's tracing; address and undefined-behavior sanitizers remained active. CI retains its original sanitizer settings. Changed C/C++ fixtures satisfy clang-format-14; `git diff --check` passes. Hosted Windows/Linux build and package results must be read from the follow-up CI run; this fixture repair does not establish gameplay acceptance.
