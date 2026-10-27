# Native forest source and authored additions

The source bundle is one deterministic ZIP for the combined, MM solo and OoT
solo native Lost Woods. Its exact bytes are tracked once as six consecutive
`parts/resources.zip.part001` through `part006` files, each at most 8 MiB, so
each file fits the source publication API. `manifest.json` pins their order,
individual SHA256/size, the whole ZIP identity and every member's SHA256/size.
The canonical ZIP remains 49,704,908 bytes, SHA256
`b0a33016b32d7e5415090038cff4cb8f4328cc8fd8a4b3f09d55918318be9fb5`.
There is no recompression or resource change in this storage split.

Build preparation verifies each part and the aggregate before opening the ZIP,
then validates all resources before restoring either ignored custom-asset tree.
The 4096×4096 authored tunnel bark is stored once across these source parts;
generated copies are not tracked. A local `resources.zip` is an ignored cache;
normal preparation reads the parts even when that cache exists. Missing,
corrupt or reordered parts cannot fall back to a cached ZIP.

`tools/black_market/native_forest_bundle.py::load_bundle(directory)` returns
the verified canonical ZIP bytes without requiring a cache. Source tooling can
open those bytes with `zipfile.ZipFile(io.BytesIO(...))`. To restore a local
canonical cache explicitly from the repository root:

```sh
python3 -B - <<'PY'
from pathlib import Path
import sys
sys.path.insert(0, 'tools/black_market')
from native_forest_bundle import load_bundle
directory = Path('tools/native_forest')
(directory / 'resources.zip').write_bytes(load_bundle(directory))
PY
```

The generator below still emits a canonical ZIP. Existing single-ZIP manifests
and an explicit `prepare_native_forest.py --bundle /path/to/resources.zip`
override remain supported, with the same whole-ZIP and per-resource checks.

The geometry control is the stock-MM forest generated from the immutable
MM archive SHA256
`2f44bafe1a00f44ddaf7eb5e5c99e73a3d23771038529d6d1748819ad575dc80`.
Its original 39-resource ZIP SHA256 is
`7b21d19512a48b2363247b9276426ff6ba7afa4f10130f98513c22a90d0c6730`.
The current bundle adds the exact authored shop materials, separate Skiejer
GitHub-avatar poster, three bench meshes, return tunnel, cosmetic overlays
and all six scrolling tracks from
`zzzz_Skiejer_Native_Solo_Forest_FINAL_avatar_POC4.o2r`, SHA256
`65901fcc27453a915be9414c7203e8295fce36ce69b7658653bbcdf978af8d0e`.

The donor closure contains 45 resources. The three shop vertex arrays are
byte-identical to the existing native arrays and retain those owners. Their
three material prefixes replace only the native material resources; the
native shop loads and triangle lists stay unchanged. The other additions
live under `custom/comboship/native_forest/authored/`. Twelve texture payloads
are allowlisted; every pixel, texture scale, UV and vertex remains exact.
The one intended color change is `ReturnTunnelMouth` primitive RGB
`(124,40,184)` → `(68,255,116)`, preserving alpha 96 and the shared soft mask.
Shop, bench and stump scrolling retains its original purple/black colors.
The unused historical `return_tunnel_purple_magic_mask` is not imported.

No full MM3D terrain display list, scene header, room header or collision is
imported. Four `Geometry*_vtx` arrays are retained only because the authored
mirror-stump shimmer references them. A later optional MM3D forest pack
should replace only the forest appearance and keep these native additions'
draw ownership. The older full-scene/retarget candidates remain unchanged;
they are not this native source bundle.

The authored bake retains native scene/collision/waterboxes/actors/spawns/
exits/transitions and all non-mesh room records byte-for-byte. Room-zero mesh
command 10 receives the authored opaque/translucent roots and recomputed
type-two culling spheres. All three rooms remain present. Untouched stock
geometry and textures are still provided by the normal `mm.o2r` owner;
OoT initializes its existing `MmAssets` base owner before readiness checks.

The current candidate also includes a separately reviewed fallen-log route to
the Mirror stump. `--mirror-access` appends sixteen collision vertices,
twenty-eight polygons and one exact copy of the original wood surface, with
matching visible bark geometry. Every original collision vertex, polygon,
surface, header/bounds, camera and waterbox byte remains intact. The log runs
from (-105,0,416) to (-345,80,416), with a 64-unit flat walking surface and
18.435-degree slope. It leaves the south recall footprint, shop and portal
clear. The original authored-only source checkpoint remains commit `05313bb2`.
Three `mirror_access_{root,material,vtx}` resources share the already-baked
tunnel bark; no additional image is imported. A repeat verifies the existing
collision and visual geometry before leaving them unchanged.

The stable `custom/comboship/native_forest/portal_root` entry still marks the
native return route and suppresses the procedural fallback arch. It now calls
the three authored opaque tunnel/mushroom roots, whose vertices are already
in world space. It must not apply the old generated arch matrix. The authored
rim has local radius 68 and center Y 66 at world anchor `(-695,0,-313)`, yaw
12288. `authored_portal_v2` and the generated C++ contract identify that rim;
the old generated `portal_{material,transform,vtx}` remain inert control data.
Runtime particle rendering is separate from these scene material resources.

MM reads `custom/comboship/native_forest/texanim` through scene command 26 and
its existing animated-material draw configuration. OoT reads equivalent
parameters from `custom/comboship/native_forest/stock_materials`, a version-zero
LUS Blob. Segments 8/9 animate wall+bench/roof materials, 10 the portal mouth,
11/12 black wall+bench/roof overlays, and 13 the violet stump shimmer. The
source TexAnim bytes are unchanged. OoT's Blob velocities already negate MM's
serialized Y velocity and bind both opaque and translucent draw contexts.
The runtime Blob factory's sixteen padding bytes are not serialized.

Build preparation's separately pinned `presentation_polish` version 2 retains
the immutable ZIP and applies the existing portal/sign polish plus the ramp
material repair. The ramp now calls segment 8 after loading its I8 mask onto
render tile 1, reusing the exact authored second layer (32×64, Y velocity -1)
in both ports. The original six tracks, every texture and the ramp's vertices,
UVs, triangles and collision stay unchanged. After the last ramp triangle,
`mirror_access_root` restores the room's two-cycle/fog/tile-zero pipeline.
Native bark display lists already set `G_TT_NONE` and retain their exact native
texture hashes; they depend on that room pipeline. Leaving the ramp's one-cycle
state active made tree/stump/tunnel bark change with the cullable draw order.
The derived material and root bytes are checked against the version-2 receipt
before either port is written. Source rebakes still consume the original ZIP.

Per-host `dependencies_{combined,mm,oot}_v1` Blobs contain every generated
shared resource plus that host's scene/collision/three rooms, and separately
all external stock dependencies. A missing addition or native stock owner
fails readiness; a mod archive is not required to make the additions visible.
`combo/entrances/NativeForestResources.h` is generated from the actual complete
bundle and exposes presentation version 2 plus authored portal dimensions.

Reproduce from an immutable native control (or rebake the current source):

```sh
python3 -B tools/black_market/native_forest.py \
  --base-manifest tools/native_forest/manifest.json \
  --authored /path/to/zzzz_Skiejer_Native_Solo_Forest_FINAL_avatar_POC4.o2r \
  --mirror-access \
  --output /new/path/resources.zip --manifest /new/path/manifest.json \
  --core-contract /new/path/NativeForestResources.h
```

To reproduce the original control directly, use `--native-mm MM.o2r --final
FINAL-avatar-POC19.prelude.o2r` instead of `--base-manifest`; include `--authored`
to produce the authored presentation. Omitting it explicitly generates the
old geometry-only control. The source edit defaults to the preserved
`tools/magic_mirror/scene/LostWoods-MirrorStump.prelude.o2r`. Every CLI output
must be a new path, and every donor identity is checked before output writing.

```sh
python3 -B -m unittest discover -s tests/native_forest -p 'test_*.py' -v
python3 -B tests/native_forest/run_material_loader_tests.py
python3 -B tests/black_market/run_solo_forest_tests.py --native
python3 -B scripts/assets/prepare_native_forest.py --output-root /temporary/source
```

Tests pin the native control's protected resource hashes, exact donor images
and vertices, poster/bench/tunnel reachability, all six material segments,
per-portal green tint with shared purple masks unchanged, culling, complete
host readiness, deterministic ZIP output, bounded part ordering/identities,
exact cache-free reconstruction, hash-checked dual-tree restoration
and the real CMake archive dependency. Access integration independently removes
the appended 16/28/1 records and verifies exact native collision hashes,
checks one visible/colliding log per room and idempotent rebaking. Native
control tests remain separate.
The material-loader fixture compiles the actual XML and binary factories and
checks ramp/native-stump draw orders under ASan/UBSan. Its optional
`--tinyxml2-root` points at a materialized tinyxml2 source checkout when the
normal `ZAPDTR/lib/tinyxml2` submodule is unavailable. The placement fixture runs
the real MM Saria/Epona spawn bodies, including duplicate guards, and checks
their portal-local ±120/depth-120 footprints against retained native collision.
These are source/resource/native-fixture checks. Configured gameplay rendering,
pause/re-entry, shop access, solo return routes and user acceptance remain
unverified; no master is promoted.
