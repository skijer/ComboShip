# Black Market entry diagnostics — 2026-10-20

Candidate base: `11329f79a5de22b10ae3c8f5d5f5d0d11673224b`.
This change owns only Black Market access, its legacy resource lookup, and
diagnostics. It changes no scene numbers, meshes, textures, archives, save stock,
combined-game portal rules, or accepted baselines.

## Entry contract

Both native door adapters retain the existing exploration-scene/room/marker
recognition and player safety checks. A normal walk up to the door should already
satisfy these checks; they are not progression unlock requirements.

- The native Open action must own the recognized door and player.
- Play must be running in a playable save, with health remaining, no pause,
  game-over, scripted/pending cutscene or active scene transition. MM additionally
  requires the current game state and no soaring/time-song cutscene.
- The destination scene, room-zero header, and collision resource must exist.

Stock/session readiness and full presentation/keeper readiness no longer veto
physical entry or eject the player after entry. Purchases still require the
correct bound, validated session. Empty stock offers a normal keeper explanation
without starting browsing, creating a purchase, or regenerating save data.
Shelf fixtures can remain visible. Optional/custom art and native keeper
preflight retain their rendering guards.

An old OoT `sEntering` value is recorded but is not a second transition lock;
the actual transition trigger/mode prevents duplicate scene changes. Save reload,
slot/seed mismatch, void/death, and missing-destination recovery still return
outside. The existing original/combined-game route guards remain intact.

## Manifest defect

`OTRExtScanner` indexes the raw `custom/skiejer_market/manifest.json` member
under `custom/skiejer_market/manifest`. `ResourceMgr_FileExists` does not strip
the extension. Both legacy readiness adapters now accept the exact raw name or
that logical cache key. A truly missing manifest still fails legacy readiness.
This is a confirmed defect, but its repair alone does not prove game entry.

## Logs

`[BlackMarketDoor]` records one JSON object per native Open attempt in the expected
forest scene, including rejected/unrecognized attempts. It does not run per
frame or log unrelated-scene doors.

- `result`, `reason`, `checks`, and `failed_checks` identify each failed actual
  recognition, safety, or destination check, even after a prior check failed.
- `location`, `recognition`, and `state` include mode, scene, room, entrance,
  layer, cutscene index, marker/actor identity, all safety state, action/door
  ownership, health, and MM form or OoT cached keeper readiness.
- `market` and `travel` show session binding/seed/stock count/purchased bits,
  entry/return flags, travel slot/seed, and respawn flag as observations.
- `resources` shows missing structural paths and both manifest lookup results.
  Missing destinations additionally include full native and legacy missing-path
  inventories. Logging uses membership queries, never keeper/model imports.

`[BlackMarketReturn]` records entry normalization that sends the player outside.
It reads save/travel/resource state only, because actor initialization may not yet
have happened.

## Verification and limits

The regression runners execute the actual controller bodies with native engine
types under address/undefined-behavior sanitizers. They cover every safety input,
session/art independence, structural resource failures, exact failed-check
reporting, empty-stock talk/stay/exit recovery, native door integration,
transactions, shelves, keeper selection, and browsing/camera cleanup. Separate
cache tests exercise the raw-manifest extension-cache convention.

Native compilation uses `scripts/diagnostics/compile_solo_forest_core.py` with
`BlackMarket.cpp` and `ComboBlackMarketNative.cpp`, both real header environments
and no test shims. Exact commands and results accompany the recovery checkpoint.
Neither unit compilation nor fixtures establish a linked executable or configured
game entry, sound, drawing, traversal, or user acceptance.

The integrated native-forest candidate now includes the verified geometry-only
source in `tools/native_forest/resources.zip`: 39 resources, SHA-256
`7b21d19512a48b2363247b9276426ff6ba7afa4f10130f98513c22a90d0c6730`.
Native-market source is also bundled and hash checked. Both preparation paths
are available to port archive generation; the ordinary extracted game archives
still supply their declared stock dependencies. Optional packs must retain
their established priority over native fallback resources. See
`tools/native_forest/README.md` and the integration report for current packaging
and runtime boundaries.
