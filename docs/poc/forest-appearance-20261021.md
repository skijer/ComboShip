# Optional native-v2 forest appearance POC — 2026-10-21

Goal: provide the exact retained MM3D terrain appearance as an optional pack,
while native v2 continues to own the shop, poster, bench, green portal,
scrolling materials and physical log access.

Native baseline: `tools/native_forest/resources.zip`, SHA256
`b0a33016b32d7e5415090038cff4cb8f4328cc8fd8a4b3f09d55918318be9fb5`,
85 resources; source checkpoint `039a8f4187aceced157362c03c074d7b193f0846`.
Donor: `zzzz_Skiejer_Native_Solo_Forest_FINAL_avatar_POC4.o2r`, SHA256
`65901fcc27453a915be9414c7203e8295fce36ce69b7658653bbcdf978af8d0e`.
Both accepted inputs remain unchanged.

Plan:

- [x] Write failing tests for six mesh-only overrides, exact terrain images/
  vertices, preserved native addition entries, forbidden resource/override
  rejection, missing native dependencies and actual draw/readiness closure.
- [x] Implement `tools/black_market/forest_appearance.py` with pinned inputs,
  terrain-only dependency collection, season-compatible combined aliases,
  preserved native culling entries and deterministic serialization.
- [x] Verify the native forest suite and generate the optional archive plus
  receipt under `evidence/forest-appearance/`; commit only this document,
  the new recipe and `tests/native_forest/test_appearance.py`.

Protected: every native scene/collision/spawn/actor/object/exit/setup record;
every room command except command 10 in rooms 0/1 of the three private modes;
room 2; all native material/scroll/geometry/texture resources; exact portal,
shop, stump-shimmer and log culling entries. No native private resource is
copied into the optional pack and no legacy shop/portal draw is imported.

Configuration: native v2 build/assets above, normal MM base owner, one optional
forest appearance pack in each host being used, Alt Assets enabled. The pack
does not replace the native v2 dependency and must fail validation without it.

Status: implemented and source/archive checks passed. Configured runtime
acceptance remains unverified. No runtime result or master promotion is claimed.

The six new tests failed first because the appearance-only recipe did not
exist, then all six passed against the pinned inputs. The retained terrain
closure is 55 source resources (22 draws, 22 vertex arrays, 11 textures).
Eight seasonal texture aliases and 22 combined draw aliases preserve existing
combined/MM-solo/OoT-solo seasonal identities. The aliases retain exact pixels;
all original vertex bytes are shared unchanged. Six private room meshes use
terrain roots plus the original four native addition entries in room zero.

The source-directory input uses
`tools/black_market/native_forest_bundle.py::load_bundle`; its six tracked
storage parts reconstruct the exact canonical ZIP above. CI does not need an
untracked `resources.zip`. A direct canonical ZIP remains an explicit CLI
input option. Storage representation does not change the pinned native bytes.

Reproduce:

```sh
python3 -B tools/black_market/forest_appearance.py \
  --donor /path/to/zzzz_Skiejer_Native_Solo_Forest_FINAL_avatar_POC4.o2r \
  --native tools/native_forest \
  --output /new/path/zzzz_Lost_Woods_MM3D_Appearance_Native_v2_POC1.o2r \
  --receipt /new/path/receipt.json
python3 -B -m unittest discover -s tests/native_forest -p 'test_*.py' -v
```

Compatibility is deliberately bounded to this native v2 source, including its
log. The pack supplies no missing native resource and does not enable this
scene on older builds. Disabling Alt Assets leaves the complete native
presentation available. Runtime appearance, seasonal rendering, pause/re-entry
and shop/portal/log use remain unverified in all modes.

Verified candidate:

- Archive: `zzzz_Lost_Woods_MM3D_Appearance_Native_v2_POC1.o2r`,
  147,452,340 bytes, SHA256
  `4132e029057b2d5ea1e5887ad134d598cfa094497445982dd3948b119238191d`.
- Generation destination: `evidence/forest-appearance/`, with `receipt.json`
  beside the archive. The parent worker owns durable artifact publication.
- 96 resources, exactly six Alt room-header overrides, zero native authored
  resource copies/overrides, twelve retained native addition culling entries.
- Exact donor terrain pixels and vertex bytes; all eight seasonal identities
  reachable in each mode; all six native scroll segments remain reachable.
- Two archive serializations are byte-identical. Saved archive CRC, SHA256,
  draw closure and retained input identity were checked after serialization.
- Full native forest suite through the shared source-directory reader:
  48 tests, 2 skips for the unavailable original stock-MM/full-FINAL donor pair.
  This includes the six new appearance tests and four split-storage tests.
- Real C++ `NativeForest::SceneAvailable` gates pass with the native baseline
  in all three modes and fail when the native poster or log vertex resource
  is missing. An optional appearance archive cannot satisfy those owners.

Smallest configured check: enable this one pack with Alt Assets on; enter
combined Lost Woods and inspect MM3D terrain plus the native textured
shop/poster/bench, green portal, violet/black effects and physical log. Walk
the log, enter/exit the shop, use the portal, pause/unpause and re-enter once.
Repeat the entry/shop/portal check in MM solo and OoT solo, and inspect one
seasonal selection if seasonal visuals are enabled. These remain untested.
The older full-scene and POC1 retarget packs must be disabled. No master is
promoted and no configured runtime acceptance is implied by these checks.
