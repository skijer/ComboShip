# Four Sword GI and held visibility candidate

| Field | Record |
| --- | --- |
| Baseline | `35ce5f22f11c845a4ef0b28228880547e14ee92d`, PR #39 recovery source; reported running build `f069ea64` contains the same source. |
| Candidate | `fix/mm-pause-inventory-20261008`; bounded sword changes in the shared isolated checkout. |
| Expected result | Four Sword retains the authored pickup GI and uses that same mesh in OoT child/adult, MM human and MM Time Gate adult hands, with Alt Assets on or off. Generic Din/native sword choices remain authoritative for their own sword families. |
| Protected | All authored sword vertices, materials, textures and grip matrices; native geometry/collision/animation; upright pickup fix; shop shelf transforms; Four Sword behavior and its own legacy override pair. |
| Status | Implemented and focused production/static checks passed. Executable CI build and installed-stack game/GPU visibility remain untested here. No runtime acceptance or master promotion. |
| Recovery | Baseline source/assets are unchanged outside the candidate diff; remove the sword diff to restore the baseline. Other concurrent pause/ownership fixes are separate. |

The supplied logs record MM granting Four Sword (`fc 106`) and the installed archive stack. They contain no decisive Four Sword mesh submission or GPU evidence. User reports establish the visible regression; they do not establish the candidate's visual result.

## Confirmed production paths

The authored resource is `objects/nei_gi_redesign/four_sword/gi_dl` in OoT's shipped `soh.o2r`. Its held wrappers are `objects/nei_held_swords/four_sword/{oot_adult,oot_child,mm_human}/held_dl`; each uses the existing rigid grip matrix and calls the unchanged authored GI. Vanilla draws use `@oot-gi-base`; MM Alt draws use `@oot`. Every matrix, vertex and texture dependency is checked in that owner before submission.

Three independent gates violated the item's ownership:

1. MM `HasMmLegacyGiMod` treated Four Sword as the native MM Kokiri guard/blade pair. A generic Kokiri/Din GI override therefore declined the authored Four Sword and reached the old tinted Kokiri fallback. Four Sword is removed from that unrelated legacy family; its actual OoT legacy pair remains governed by the owner descriptor.
2. Both held selectors could reject a selected Link body or an underlying Kokiri/Master/Din equipment choice before the actual Four Sword getter ran. MM's native and adult limb callers repeated the body gate. The Four Sword item now reaches its own getter on those standard rigs; ordinary swords retain their previous selected-body/equipment priority and transformation/weapon guards.
3. OoT's CustomEquipment and final PAK hand hooks follow the early authored injection. Clones can be repainted by borrowed sword actions, and the PAK's unknown Four Sword item falls to generic Kokiri equipment or an empty slot. The existing final protected sword stage now reapplies Four Sword after those hooks, with a separate deferred compound for each draw. Its existing Din branch is unchanged for ordinary swords.

The real Four Sword getters and the held resource tables did not require an asset or namespace change. Their existing complete-graph validation, Four-specific legacy pair priority, and held frames are retained.

## Verification

`python -B tests/nei_held/run_four_sword_visibility_tests.py` initially failed in the production GI gate, generic held equipment/body gate, MM native/adult caller gates, and the late OoT hook stage. It now passes. The fixture compiles the real Four Sword getters and held selectors with each game's real structures/GBI, uses the real MM resource bridge, and executes the actual native/adult/late sword stages. It covers both Alt states, child/adult frames, custom/native sword priority, selected standard skin bodies, complete blade/hilt dependencies, MM-local collisions against missing OoT donor resources, Four-specific pair mods, final empty/generic PAK hooks, and unchanged player state. The production OoT GI descriptor and sword selector checks retain canonical Four Sword pickup ownership with Din on/off and preserve ordinary Din variants.

Additional checks passed:

- `tests/nei_held/run_held_sword_tests.py`: all nine held graphs, both host selectors, balanced matrices/grips/native reach, immutable authored source hashes.
- `tests/nei_held/run_oot_sword_limb_tests.py`: native fist/type, independent deferred player/clone compounds, hide/transformation guards.
- `scripts/diagnostics/run_nei_identity_tests.py`: production descriptor and MM fallback identities; its old generic-MM-mod expectation for Four Sword is corrected.
- `scripts/diagnostics/run_nei_gi_tests.py --held --combo`: complete combined renderer and real-header checks passed. Its legacy fixture now treats MM Kokiri guard/blade overrides as unrelated to Four Sword, while testing Four's actual OoT blade/hilt priority and MM-local collisions with independent host/owner Alt states. Native-family override assertions remain intact.
- `scripts/diagnostics/run_din_fire_sword_tests.py`: existing adult/child/Master/BGS/broken/progressive Din behavior, colors, eligibility, fallbacks and unchanged player state.
- `scripts/diagnostics/run_sword_asset_toggle_tests.py`: seven sword identities, independent live OoT/MM Alt states and legacy/non-sword priority.
- `scripts/diagnostics/run_mm_back_equipment_tests.py`: actual native MM player translation unit and adult limb callbacks; fixture supplies the new Four Sword query with Four Sword inactive.
- `scripts/diagnostics/run_mm_nei_tests.py`: native MM resource/render/held/action suites passed. The general-hand fixture supplies the Four Sword query as inactive so its existing native/adult fist, body/form and Din priority assertions remain focused; active Four Sword is exercised by the dedicated visibility runner.

The numeric `run_sword_pose_tests.py` upright/Four legacy pose check passed. Its subsequent real-header checks initially lacked `SDL2/SDL.h`; the coordinating task supplied actual SDL headers and reran the checks successfully for both hosts, including the native MM draw/presentation translation units. Final executable CI and integration checks remain with the coordinating task. None of these controlled production fixtures proves a linked executable or in-game visibility.

## Decisive installed-stack check

Receive Four Sword with the reported Din/skin packs installed; confirm the authored curved gold guard, ruby, ivory grip and blue/gold pommel in the pickup. Hold/swing it as OoT child/adult, MM human and MM Time Gate adult, and summon clones. Toggle Alt Assets both ways after draws warm. Confirm the sword remains visible and matches the GI, generic Din swords still retain their variants, and shelf previews/purchases keep the existing transforms. Master acceptance remains pending that runtime result.
