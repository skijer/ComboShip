# MM3D opening forest: authored return, deep-purple light and seasons

This is the continuation of `bb15350f` on isolated branch
`poc/mm3d-lost-woods-exit-seasons-20261016`. Publication was held at that
checkpoint; the user subsequently authorized integration with the narrower
entrance options. See `door-only-cross-entrances-20261016.md` for that continuation.
The earlier source checkpoint is preserved.

## Exact supplied baseline

- Library package: `ComboShip-MM3D-ScenePorts-POC1.zip`, SHA256
  `32034212f90477620ae133332c76210619c6c4f08995bce1cc8b89c98f9b8ed8`.
- Inner Lost Woods pack: `zzzz_MM3D_Lost_Woods_Henriko_POC1.o2r`, SHA256
  `3715273f31025f862567c7f5c771c2e803b6e779e801117cf310cd02bf904317`.
- The port's source checkpoints are `9be499ec` and `dbbf5966`; their visual
  archive is the input, not a replacement for the current gameplay source.
- Native reference `mm(1).o2r` matches the port receipt's SHA256
  `f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59`.

The authored tunnel is in room zero at native spawn zero: (-417, 0, -122),
yaw 11520. Gameplay rooms zero and one are overridden; title/flashback room
two remains native. The supplied POC's collision-fitting, camera, culling,
performance and runtime acceptance gaps remain inherited limitations. No
collision fitting or Palace work is claimed here.

## Archive and route contract

`tools/bind_mm3d_lost_woods.py` verifies the exact baseline hash and refuses
to overwrite an existing output. It changes four room-zero/setup headers'
opaque mesh references to a new `opa_combo_return_v1` XML wrapper. That wrapper
calls the original opaque root, including the authored tunnel. Every byte
before and after each serialized mesh command is preserved. All room-one
headers, actors, collision, cutscenes, textures, UVs and geometry remain intact.

The active native `RoomShape`, rather than mounted-file existence, identifies
the bound mesh. When it is active, the engine suppresses its temporary arch
and uses oriented bounds inside the supplied 68-unit inner opening: lateral
-58..58, vertical -40..130, local depth -62..12. The trigger remains disarmed
until the player leaves 120 units from the entry. A competing native/other
room mesh retains the fallback arch and original radius-based trigger. The
separate Woods of Mystery passage and its camera fix remain intact.

The requested light change sets RGB to (96, 32, 176), deep purple, on all 60
vertices in the existing glow resource. Positions, UVs, vertex order and every
alpha value are identical. The center/rim gradient, small entrance lights,
fog-free translucent material and culling remain authored. The historical
resource name `ReturnTunnelPinkLight_vtx` is retained to preserve its bindings.
No additional animation is claimed. This changes the supplied MM pack; the
separate OoT light sheet still requires its current scene archive.

The candidate adds one mesh wrapper and edits exactly five existing resources:
four mesh references and glow RGB. The rest of the supplied pack is checked
byte-for-byte. The Palace archive is not part of this change.

## Seasonal foliage

The port uses XML FILEPATH image commands under private `custom/` identities.
The old native texture-hash tint list does not reach those materials. Eight
exact private images now use the existing reversible green-only texture-copy
path, scoped to this scene with Alt Assets enabled:

| Material | Treatment |
| --- | --- |
| `lw_enkei_01_0` | Forest backdrop: autumn foliage, spring blush, winter frost |
| `lw_kusa_01_12600` | Leaf/grass atlas: autumn foliage, spring blush/lift, winter frost |
| `lw_wood_02_2CA00`, `lw_wood_03_31E00`, `lw_wood_04_37200` | Green foliage/moss islands; non-green wood retained |
| `lw_kabe_01_5400`, `lw_yuka_02_39C00`, `lw_yuka_03_44400` | Green terrain islands in autumn/winter; native spring ground |

Summer and Off restore the original images. Mushroom, cut-wood, plain-bark and
return-bark images are excluded. The purple vertex light has no seasonal tint
target. Mixed images retain every non-green pixel and exact alpha, dimensions,
format, raw flags and texture scales. Original pixels are never modified.
Scene/Alt/season changes use the existing ownership and restoration rules.

The native opening scene uses `SKYBOX_2`, which is excluded from the outdoor
weather selector. `MMWeather_FoliageSeasonForPlay` permits foliage during a safe
ComboShip exploration visit while preserving normal outdoor rain/sky policy.
It uses the same manual/Rod season selection and respects story lighting,
cutscenes, game-over, disabled sky and underwater guards. It writes no Rod,
inventory, cycle or story state. Other scenes retain the existing selector.

## Evidence and acceptance

- Test-first controls fail for the original missing private-image targets,
  the special skybox's missing foliage eligibility, and the old pink asset.
- The complete seasonal scene-material runner passes, including the new exact
  private targets, mixed pixels, spring ground, excluded images, repeated
  seasons, Alt off and scene restoration. ASan/UBSan is also exercised.
- The pack/native test verifies the actual archive delta and executes the
  production mesh selection, oriented bounds and placeholder/Mystery draw
  selection. With the supplied pack it executes the real copy function on all
  eight actual HD images under ASan/UBSan, checking source, alpha, non-green
  pixels and metadata preservation.
- The opening-forest suite covers the real `SKYBOX_2` season selection in both
  rooms, manual/Rod/Off controls and unchanged weather/story/save policy.
- The existing full forest and MM weather suites are retained. Source save
  round trips, all five forms, cutscene guards and the three-slot preservation
  coverage from `bb15350f` remain part of the candidate.
- The purple PNG is an asset render through the supplied Mesa preview tool,
  labeled not in-game proof. It is not a game screenshot or season-runtime proof.

The `.o2r` requires a build containing the matching source changes for the
return route and seasonal tinting. No full linked build or configured game
traversal is claimed. Replace the earlier Lost Woods POC1 mod with the candidate
in an isolated Alt-enabled setup, verify the effective room mesh, then check
entry/return, all forms, room one, all seasons, re-entry and native/Alt reloads.
Use an established save copy to compare progress before and after crossings.
The earlier save/cutscene guards remain mandatory. Promotion requires runtime
proof and user acceptance; no source or archive is promoted by this delivery.
