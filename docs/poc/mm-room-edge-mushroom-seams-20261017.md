# MM Lost Woods: marked trunk and mushroom seams (POC5)

The 17:55 review screenshot resolves the entrance target: the large trunk at
the room edge behind the small cut stump. POC4a used the wrong stump in the
clearing. This follow-up supersedes that placement and preserves the stump-top
UV repair the user accepted. It addresses the remaining mushroom cap/rim joins
and underside sector lines without rebuilding the scene.

Source parent: `d890614f451561a853c23c805a7f336eb3bdb919`.
Cumulative source base: `43131342882972eeb4a257599ec8b061c7dc6d43`.
Asset parent: POC4a, SHA256
`8acc5368601ab6efdecdc419acf966d66d7fa62dcf2c5a0c9e53bd5b73de9240`.
Final POC5 asset SHA256:
`e6c5defb683088100e23301fc9d44fa5a3ab17640905ff3b375a51724270005b`.

## Changes and preservation

Move the five existing doorway vertex resources by X -270, Z -470, retaining
their Y, UVs, colours and triangle order. The new anchor is (-660, 0, -350),
yaw 16384. The mouth bounds are X -685..-684, Z -418..-282. Clipped native
trunk/root faces in the opening rectangle end behind the mouth. Four room-zero
setups select the new `opa_combo_return_edge_v4` wrapper. Active opaque and
translucent traversal reaches every doorway component exactly once, so the
old clearing location has no retained duplicate arch.

The mushroom atlas has discontinuous paint across UV charts that share physical
edges. Match only six-pixel border bands around the cap/rim joins and both
underside triangular charts, accounting for the small chart variants. The repair
changes RGB in 60,952 of 1,048,576 pixels (5.81%). Texture header and alpha are
unchanged. All mushroom vertices, UVs, colours, triangle data, the accepted
Geometry4 stump-top resources, all other textures, grass, weather and room-one
bindings remain byte-identical to POC4a.

The archive changes exactly ten existing resources (five doorway vertex
resources, four setup bindings and one texture) and adds one wrapper. Source
recognizes v4 and uses the same anchor for the trigger, camera and native
purple trim/shimmer. v1/v2/v3 retain their respective visible positions when an
older archive wins load order; recognized archives suppress the temporary
source-drawn arch. No save, story, onward-route or weather logic changes.

## Verification and remaining proof

`edge_seam_pack_test.py` checks archive integrity, exact resource deltas,
rigid placement, setup identity, active display-list multiplicity, native-trunk
clearance, and byte preservation. It samples 346 cap/rim joins and 406 underside
joins across both rooms, with no RGB discontinuity above 1/255 at the tested
points. The unchanged baseline failed these colour/location checks first. The
source return fixture first failed to recognize v4, then passed after the change.

The return, camera and portal-finish fixtures pass with ASan/UBSan. Coverage
includes v4 and native/v1/v2/v3, both forest routes, all days/forms, trigger
exclusions, projection, camera-service collision responses, release, rollback,
lifetime and save preservation. The production renderer compiles and requests
the native C interpolation symbols. Packed previews show both scene rooms and
the native finish geometry under neutral lighting.

These are asset renders and focused source checks. They do not establish
configured in-game traversal or collision acceptance. The broader entrance
suite was previously blocked by missing `SDL2/SDL.h` in the unchanged source
baseline; no full linked build was performed for this follow-up. Include this
source checkpoint in the next aggregate build. No master promotion is implied.

For Prelude, load the POC5 archive in place of the earlier Lost Woods mod: the
relocation and texture changes are in the archive itself. The native animated
trim/shimmer, trigger and camera require the matching updated source. Keep
the established seasonal/weather configuration and mod priority. The remaining
runtime check is to approach, enter and return through the marked edge trunk,
inspect the cap/underside joins and shimmer, and confirm the clearing stays empty
after re-entry and pause/unpause.
