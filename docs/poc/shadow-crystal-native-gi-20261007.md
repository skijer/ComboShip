# Shadow Crystal native GI — POC1

| Field | Record |
|---|---|
| Baseline | `13901c67`, cumulative ComboShip merge; earlier violet interpreted GI. Baseline renderer harness passed before edits. |
| Candidate | `poc/shadow-crystal-native-gi-20261007`; original black/orange mesh repackaged for the authored GI renderer. |
| Scope | Shadow Crystal GI geometry/material source, opaque recipe, frame bounds, and its shared charcoal/orange effect policy. |
| Preservation | Original `objects/object_nei_shadow_crystal` files remain byte-identical. Item ownership, Wolf Link transformation, icons, Shadow Scepter and other GI palettes remain outside the change. |
| Configuration | Authored built-in OoT resources; MM routes the foreign GI through the OoT resource owner. Production fixtures cover base/Alt assets, pickups, shops and freestanding draws. |
| Evidence | Source geometry test, 62-model serialized asset verification, real-header OoT/MM renderer fixture with ASan/UBSan, native MM renderer suite, independent read-only review, and nine-second offline preview. |
| Verdict | Implemented and static/renderer verified. Gameplay appearance and user acceptance are untested; no promotion to the accepted master. |
| Recovery | Isolated commit and binary-capable Git patch based on `13901c67`; reference, editable GLB, preview and verification results packaged separately. |

The previous GI builder generated a violet six-sided crystal and transparent skin. This candidate imports the existing black/orange resource's three material groups, triangle winding, UVs and byte-normal values. It retains all 614 renderable source triangles. Two existing zero-area triangles are omitted because they cannot draw pixels. No new silhouette, smoothing, or surface textures are invented.

The GI's native bounds are X ±12, Y ±25 and Z ±8.5, retaining its established overhead height. Its former violet translucent pass and texture are removed, and the recipe now needs only the opaque display list. Rebuilding the Shadow Crystal also removes obsolete shell files so asset inventories and fit calculations cannot inherit them.

`Kind::DarkCrystal` has its own charcoal palette, thin copper-edged wisps, rising burnt-orange embers and warm glint centers. Shadow Scepter remains violet. The existing optional NEI shimmer toggle still controls the shared shimmer; intrinsic authored particles retain the established GI policy. Arbitrary third-party geometry overrides keep their existing priority and authored-energy suppression.

Validation:

```sh
python3 tests/nei_gi/shadow_crystal_source_test.py
python3 tools/nei_gi/verify_assets.py
python3 scripts/diagnostics/run_nei_gi_tests.py --combo --sanitize
python3 scripts/diagnostics/run_mm_nei_tests.py
git diff --check
```

All passed. Local JSON/spdlog include paths were supplied through `CPLUS_INCLUDE_PATH`; dependencies were not added to the repository. Existing engine header/const-qualifier warnings remain. The first candidate renderer run identified stale shell files; the cleanup-capable rebuild removed them, and the full sanitizer rerun passed.

Reproduction:

```sh
python3 tools/nei_gi/SOURCE/quest_revamp.py shadow_crystal --install
python3 tools/nei_gi/generate_frame_bounds.py
python3 tools/nei_gi/runtime_preview/render_shadow_crystal.py /absolute/output/path
```

The PNG/MP4 preview uses the exported GLB and production effect sampler with the renderer's 1/16-unit position packing. Lighting is an offline approximation. It is not gameplay evidence.

For integration, apply the source commit/patch and rebuild the built-in resources and executable with the other polish changes. A mesh-only archive cannot update the existing executable's effect policy.

The decisive runtime check is to receive Shadow Crystal at an MM randomized check with Alt Assets off and on, inspect a full spin for clipping and black/orange readability, then repeat an OoT pickup and a shop display. Confirm its Wolf Link transformation still behaves as before. Record the exact build and pack load order with the result.
