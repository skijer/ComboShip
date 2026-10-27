# MM Hylia's Grace and generated autumn leaves

Date: 2026-10-06. Isolated branch: `poc/mm-grace-autumn-20261006`. Exact base: `af54f452a99ec59800bb59ba770f58d70deefbff`.

This POC prepares offline source and assets for two reported failures. It has not been merged, deployed, or accepted in-game. Concurrent build and effects worktrees were left untouched.

## Findings and resulting behavior

The MM Grace controller called the donor `func_80077D10` compatibility stub. That stub never writes the stick magnitude or yaw outputs, so the fairy had no horizontal control. The controller now calls native `Lib_GetControlStickData`, retaining inversion handling and camera-relative motion. Stick moves, A ascends, B descends, L sprints, and the equipped Grace button toggles the transformation off. The existing 24-magic cost and casting effect remain.

The donor's camera-finish, room-request, room-finish, and floor-exit paths also needed MM equivalents. Native room loading now accepts live negative door actor IDs, decodes MM's packed door yaw, validates destination bounds, waits for room data, completes the owned request once, and moves across the correct side. Floor exits call MM's actual player transition helper, including respawn/return groups, fade behavior, and native scene-event exclusions. Player initialization/destruction clears transient fairy state without dereferencing prior-heap actors. Water, death, cutscenes, scene transitions, and transformation changes cancel the active spell. The player-only no-clip wall guard includes active fairy flight.

The earlier autumn implementation tinted only `En_Wood02` trees and occasionally spawned a few nearby native leaf actors. It did not connect autumn to the snow particle owner. Clock Town's `Obj_Tree` branching canopy was a different actor and had no autumn tint. The new seasonal layer reuses native snow motion and renders generated transparent leaves in crimson, orange, gold, and copper palettes, including clear weather. Its cap is 32 particles per native snow actor, subject to native screen culling and distance fade. Supplemental actors are reused between Autumn and Winter and removed for other seasons. Initialization and motion use private seasonal randomness in Autumn. Native precipitation counts and Snowhead's fog state are preserved.

Native Clock Town branching canopies now receive crimson/gold autumn tint. Only canopy drawing is tinted; the trunk and collision/sway behavior retain their native paths. Disabling Autumn restores normal materials immediately. Native texture-only replacements can receive the tint; replacement display lists with their own materials are skipped. Scene-baked foliage and unrelated bush/grass actors are outside this narrow change.

The supplied session log contains audio/import and item-grant output, but no Grace or autumn execution traces. The source failures were reproduced by native-boundary harnesses. Those results do not prove the user's deployed binary/archive contained this exact base or that these changes have rendered correctly on their machine.

## Seed policy

| Mode | New seed pool | Activation after ownership |
| --- | --- | --- |
| Off | Excludes Grace | Disabled in that seed |
| On | Includes Grace | Available with the normal magic cost |
| Gated | Includes Grace | Requires the selected number of collected dungeon rewards |

New seed menus default to **Off**. The gated slider defaults to **4**, ranges from **0 to 13**, and counts distinct OoT spiritual stones/medallions plus MM boss remains. Songs and soul flags do not count. Threshold zero opens the gate immediately. This POC chooses the reward-count gate; an all-spells requirement is not added.

Combo uses the MM setting as one authoritative choice for both source pools even when general settings sync is Off. Both Grace controls are disabled throughout threaded generation, retries, and pending finalization. Settings are stored with the seed; changing the menu afterward does not change that seed's casting policy. Fresh and repaired MM files apply the consolidated seed's settings before initialization, then restore the current UI settings, including on failure. Headless new generation also normalizes the policy; seed replay restores its saved values.

Appended option IDs preserve preceding IDs. Old short MM option arrays are bounded-copy/zero-filled; future extra entries are ignored. Missing Grace keys in old consolidated seeds and old native OoT spoilers retain legacy **On** behavior. Old native OoT save arrays likewise default missing Grace mode to zero/On. Native spoiler import clears an earlier seed's Off/Gated policy before parsing. Non-randomizer casting remains available.

MM logic already does not rely on fairy flight. OoT logic may rely on owned Grace in On mode, but treats Gated flight as optional so it cannot require Grace to reach the rewards that unlock it.

## Verification and limits

- Failing controls reproduce the baseline movement, native door handling, exit, water cancellation, pool inclusion, saved-policy application, short-array compatibility, omitted Clock Town canopy, and missing autumn snowfall paths. A separate negative control removes the new native spoiler defaults and fails after prior Off/Gated imports.
- Grace tests execute the production Start/Stop/fairy/door/exit/handler bodies, the actual native stick/camera/exit helpers, native saved structs, and the MM activation bridge. Engine graphics, input routing, magic, collision-query and room-loading services have explicit fixtures. Tests cover analog/A/B/sprint movement, camera yaw, cancellation, lifecycle reset, packed doors, room completion, respawn/floor exits, all modes and reward counts.
- Seed tests execute real normalization/restoration/save-application boundaries and the complete native OoT spoiler parser with native Context option storage. They cover changed UI values after generation, file repair/failure restoration, legacy seeds/saves/spoilers, and option-array compatibility. They do not run the asynchronous fill worker or real file I/O.
- Weather and foliage tests execute native snow rendering commands and whole tree actors; sanitizer checks cover initialization/motion RNG, generated texture submission, canopy tint scopes, native/alternative materials, story/interior/underwater eligibility, season changes, precipitation restoration and native impact-leaf limits.
- All generated leaf resources load through native `FastTextureFactoryV1`, `BinaryReader`, and `MemoryStream` with RGBA32 dimensions, alpha, scaling, and buffer lifetime checks. Asset collision validation reports no new shared collisions.
- The changed game and launcher translation units pass native-header compiler syntax checks, including the full MM and OoT player/custom-item unity units and both menu surfaces. This is **not** a full CMake build, link, archive build, GPU test, or gameplay run.

The broader 65-command Combo/NEI suite was resumed in segments after fixing the affected test fixtures and locating CMake 3.31.6. Its other 64 commands completed successfully. `tests/nei_used_fx/run_tests.py` reaches an interpolation comparison that requires historical commit `c77c18587a976f6d6cb5c8f91f27593286469218`, which is unavailable in this checkout, so the full suite is not claimed green. Leak detection was disabled after this execution environment's LeakSanitizer process-inspection failure; AddressSanitizer, undefined-behavior, and bounds checks remained enabled where the tests request them. The focused Grace/autumn checks passed.

## Integration and game acceptance

Apply the included commits in order to the exact base or review their patches against the active integration branch. `ComboShip.cpp`, seasonal snow code, and the Rod of Seasons file overlap likely integration work; reconcile intentionally instead of copying a whole worktree over another chat's changes.

Build the games/launcher and **Generate2ShipOtr**, then deploy the regenerated **2ship.o2r** together with the rebuilt DLLs. The generated textures live in a private native archive namespace, so the POC does not require an external texture pack.

| Runtime check | Expected evidence |
| --- | --- |
| MM human Grace with vanilla assets | Casting completes; stick/A/B/L respond; toggle restores Link and camera |
| MM room doors in both directions and native floor exits | Correct room side, no bounce or premature room unload, native exit/fade |
| Water, cutscene, death, mask change, reload, game swap | Spell ends cleanly; no stale fairy or actor state |
| New Combo seed, sync Off, each mode | Both source pools and saved snapshots agree; generation-time controls disabled |
| Gated seed below/at threshold and after file repair/reload | Below threshold rejects before magic cost; threshold permits casting; menu edits have no effect |
| Old consolidated seed/native spoiler/native save | Legacy ungated Grace remains usable |
| Autumn in eligible vanilla Clock Town/field scenes | Four generated leaf palettes fall with native snow motion; native branching/conical/oval canopies tint |
| Winter/Spring/Summer/Off and Snowhead transitions | Native weather/fog/counts restore; no generated leaf layer outside Autumn |
| Texture-only pack and replacement scene/display lists | Confirm expected native tint scope and visual compatibility without material leakage |

GPU color, leaf scale/alpha, camera feel, nonhuman cast animation, performance, and real room/grotto behavior still require configuration-specific in-game proof and user acceptance. No promotion to a sacred baseline is claimed.

## Combined candidate review

The offline commits were applied to the recovered O2R bridge candidate `33e2ad0a41cbec4f7abeb3b7d89b9f280c463d73`. Independent review found that Grace's generic door bypass could reload a room already preloaded by `En_Holl`, or choose the wrong room and move horizontally at a vertical loading hole. Both positive and live negative `En_Holl` entries now remain under the native handler's control; actual door bypass remains active. New controls reproduce both failures before the guard and pass afterward, alongside the existing flight, door, floor-exit and cancellation cases. Full combined build and regression results are recorded separately for the published candidate.
