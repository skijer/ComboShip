# Native solo forest candidate

This isolated source candidate continues the shop/title baseline `8ccbd47d`.
It adds native exploration copies of the accepted two-room MM3D Lost Woods so
the shop keeps the same physical approach in both solo games. The original
combined-game forest, intro and flashback remain separate.

The recovered item-frame/X and keeper presentation work stays on its separate
shop-visuals branch. This forest branch changes the access/routing integration;
its checkpoint must be merged cumulatively with that visual work later.

| Mode | Approach | Shop | Local forest exits |
| --- | --- | --- | --- |
| OoT only | Usual pink tunnel in native OoT Lost Woods | Same forest doorway, using the native OoT shop controller | Far stump and Clock Tower portal both return to the captured native OoT Lost Woods origin |
| MM only | Established Woods of Mystery room-1 passage | Same forest doorway, using the native MM shop controller | Stump returns to captured Woods of Mystery origin; onward exit remains native Clock Tower interior |
| Combined | Existing opening-Lost-Woods route | Existing MM shop doorway | Existing combined portal routes and prologue guards |

The private copies use MM scene `0x03` / entrance `0x1600`, and OoT scene
`0x6F` / entrance block `0x618`–`0x61B`. OoT's local Clock-exit block is
`0x61C`–`0x61F`. These routes bypass entrance randomization and cross-game
handoffs. The same shop location uses native player, actor, title, message and
save services in its active engine.

## Assets and load order

| Mode | Active engine mods | Required packs |
| --- | --- | --- |
| OoT only | OoT | Black Market POC11 and Native Solo Forest POC2 |
| MM only | MM | Black Market POC11 and Native Solo Forest POC2 |
| Combined | MM | Existing accepted Lost Woods POC9 and Black Market POC11 |

Use the matching source candidate. POC2 is an addon with private resources; it
does not replace the original combined forest pack. Keep only the selected
market/solo-forest candidate versions enabled. The original combined POC9 keeps
its accepted Alt Assets configuration; private solo resources support base and
Alt resolution. Required-resource checks disable the new solo approach when its
complete addon is absent.

| Artifact | SHA-256 |
| --- | --- |
| `zzzz_Skiejer_Native_Solo_Forest_POC2.o2r` | `3d45015fa43997e2f3a96440e018a6668a2b37d654bc3e32843c0f9100093cfb` |
| `zzzz_MM3D_Lost_Woods_Right_Nudge_POC9.o2r` | `72d750b00583379f3619dac52dce59e2957a351fdfabd3a3a8da11f9bc19daf0` |
| `zzzz_Skiejer_Black_Market_POC11.o2r` | `d439e88073d6f8d5230d4ccff1aa744824be6b60975ac8469e1ba9fde9c346c2` |

POC2 contains 88 dependency-closed resources. Geometry, texture bytes, UVs,
vertex colors, material values, and native collision shape/surfaces are retained
from the exact donors. The port remaps private paths, maps collision cameras to
native normal camera settings, removes intro/cutscene machinery and adapts
native scene/actor framing. OoT uses native Lost Woods music, Kokiri nature,
outdoor eligibility and no skybox; its numeric skybox 2 would be Bazaar.

The converter maps MM's packed transition yaw to OoT's direct binary angle,
and emits each engine's native player-spawn encoding. Native grass/rock
controllers use local actor IDs and the field keep; MM-only dust/Keaton actors
are omitted from the OoT copy. Existing shader/title/actor adapters and the
unmodified POC11 shop pack remain required.

Both solo copies join existing reversible weather/season presentation. Eight
private greenery identities retain their donor treatment while preserving
mushrooms, bark, non-green pixels and alpha. See
`solo-forest-weather-recovery-20261017.md` for exact checks and limitations.

## Recovery and validation

The exact accepted POC9 was recovered through the unchanged guarded POC1–POC9
patch chain. All intermediate archive hashes match. Two original ZIP wrapper
timestamps were recovered against the recorded hashes; source payloads were
never substituted. The saved exact-recovery kit contains the replay recipe,
unchanged tools, receipts and metadata proofs. New POC2 packaging uses fixed ZIP
metadata and reproduces the same complete archive under different clocks.

Source milestones are recorded in `solo-forest-wip-recovery-20261017.md`.
Recovery packages retain original milestone IDs/patches and separately provide
standalone snapshot history for an empty checkout. `AGENTS.md` records the
required checkpoint policy for subsequent work.

Fresh verification completed on the recovered source:

- Both native forest lifecycle/return fixtures, actual native actor-spawn
  rotation fixtures, market transaction/return fixtures, portal submission/state
  fixtures and the combined opening/prologue suite pass. The applicable native
  fixtures use ASan/UBSan; leak checking is disabled for the executor's process
  tracing limitation.
- All 10 core native translation units and the three changed weather/material
  units compile against real engine/dependency headers. Recorded source,
  shared-contract, dependency and object hashes match the checked files.
- The complete solo-mode gate passes its three compiled mode/options probes
  and all 18 production-boundary suites. The complete combined Lost Woods gate
  passes, including save, camera, title, original doorway, rendering-selection
  and prologue controls. Scene-name fixtures pass in ComboShip and standalone
  SoH configurations.
- MM weather/material and OoT foliage sanitizer suites pass, along with the
  complete OoT seasonal-weather controller suite. Independent review reran the
  focused weather, lifecycle, spawn, portal and scene-name fixtures.
- The actual POC2 archive passes ZIP integrity, strict donor/resource comparison,
  88-entry dependency closure and equality with the committed resource contract.
  Two builds with different clocks yield identical complete archives.

Reproducible native compilation helpers are
`scripts/diagnostics/compile_solo_forest_core.py` and
`scripts/diagnostics/compile_solo_forest_weather.py`. The recovery package holds
their exact commands, native header recovery notes and raw verification logs.
Broad checks are `python3 tests/solo_games/run_tests.py` and
`python3 tests/lost_woods_entrance/run_tests.py`; the sanitized prologue check is
`python3 tests/lost_woods_entrance/opening_forest_test.py --sanitize`.

This is an implemented source/asset candidate. A complete linked executable,
configured gameplay traversal, collision/camera acceptance and visual acceptance
remain pending. Existing donor collision-fitting limitations remain inherited;
preserving collision bytes is not proof that every custom mesh surface fits.

The decisive runtime check is a round trip in each solo game: native approach →
both forest rooms → shop → forest → stump return, followed by the OoT Clock
portal return and MM onward Clock Tower route. Check OoT child/adult, MM forms,
normal/Alt, weather/seasons, save/reload inside each destination, void/death and
missing-addon recovery. Compare a combined-game control for original entry,
intro/flashback behavior and return routes. Keep accepted masters unchanged
until that configured evidence and user acceptance are recorded.
