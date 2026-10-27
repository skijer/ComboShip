# Shared Fountain valley candidate — 2026-10-24

Implemented and native/static verified; configured gameplay and user acceptance remain untested.

Baseline: restored overnight source tree `fce96d959ecbeaf37b0e49b05a90684549084e9f`, recovery checkpoint `9347ae570ee1921b55f1eb1952f901afb38370be`, original candidate `729e3826db8d3adc04dd60ec668823b9ab230ede`. No develop/master integration. This change requires the replaceable material API in `combo/render/RegionalBridgeMaterials.h`.

## Source distinction and narrow changes

The original OoT archive and supplied editable export are different render graphs. All six Fountain room setups resolve the same canonical room display-list roots, including `scenes/shared/spot08_scene/spot08_room_0DL_002538`.

* Original native: the low tree screen consists of precisely eight triangles in four commands at Gfx slots27,28,42,43 of the45-instruction list. Both command/vertex-load signatures and the12 referenced positions are verified before patching. Only those commands become no-ops. The resource is retained while patched, restored when its identity changes or play ends, and checked again before scene drawing.
* Export native/Alt: that root calls the high canopy leaf `spot08_room_0DL_002538_tri_0`; its referenced vertices have minimumY1843. It has no low screen to remove. Runtime recognition checks the active parent call marker and a separately named high vertex resource; it does not traverse the leaf or prove its linkage to those vertices. This classification relies on the independently traversed, complete supported native/export/Alt layouts. These checks do not guarantee rejection of arbitrary partial or restructured replacement packs.
* The export already renders the43 selected valley triangles. Its native collision lacks them, while its Alt collision contains them. Plain native OoT has a coarse lower floor, requiring the fitted visible snow floor and side surfaces as well as missing collision.

The process-owned actor adds only missing triangles by matching unordered exact vertex positions against the active scene collision. Native/export-native add43; export-Alt adds0. The27 floor triangles and16 side triangles are derived from matching existing valley geometry, not a wholesale donor collision header. Donor types0/37 are deliberately remapped to a local ordinary surface0 with a process-owned `CAM_SET_NONE` record; no donor camera, exit, conveyor, water or actor metadata is copied. Existing native collision data is never written.

Snow uses the exact fitted floor positions. The detailed presentation draws it as a coplanar opaque decal; native uses ordinary opaque surfaces over its lower coarse floor. Detailed native/Djipi walls and their UVs remain untouched. Native newly visible side surfaces use the replaceable Snowhead stone material. Added floor/side S coordinates share the safe world-X origin 2048 before signed16 packing, preserving the 8-unit density and 32-pixel repeat phase; floor T projects Z and side T projects Y. All new surfaces retain named resource signatures for post-build `.o2r` replacements:

* `textures/comboship/regional/snowhead/valley_snow`
* `textures/comboship/regional/snowhead/stone`

The API provides exact terrain `FloorAt(x,z,&y)` independently of placement clearance. `BodyClearAt(x,z,radius=18)` reserves player radius inside three continuously verified80-high physical bands: x1580..2000,z±60; x1700..2000,z±120; x1600..4080,z±40. Mouth1840/z0, return1740/z0 and the ascent handoff1600/z±24 fit. The mouth/return art width±100 fits the wider band with an adult radius18 plus2-unit lateral reserve. These are fitted ground coordinates, not the old y400 entrance.

The full set of floor-like side slopes is **not** a certified walking/placement area. Continuous full-footprint checks intentionally exposed natural wall/side-hill intersections outside the usable bands, including the native lip near1579/157. Keeping `FloorAt` separate from `BodyClearAt` avoids disguising those boundaries. The certified bands have no blocking native collision or protruding remaining render geometry.

## Checks and limits

`python tests/regional_passages/run_valley_tests.py` compiles the portable production header and the host translation unit; executes real native registration/deletion functions with the production actor/resource bodies under ASan/UBSan; checks all 129 actual `Build` draw vertices for signed16 UV bounds, edge gradients, shared-vertex continuity within each projection and whole-tile phase (including terminal side 17/floor 41); checks repeat spawn, destroy/re-entry, no duplicate collision, partial collision coverage, allocation failure, ordinary surface/camera ownership, unchanged scene water/exit metadata, foreign layouts, and resource restore/reload. Renderer compilation is included; it is not a renderer execution test.

`valley_archive_test.py` walks each actual room's roots and all recursive display-list calls with Alt/fallback resolution. It reproduces the old native backdrop obstruction, verifies43 exact export render matches, and clips all intersecting native/render triangles to every complete floor/body prism. An exact rational polygon-union subtraction proves full floor support for each production-exported clearance band before obstacle clipping; unsupported rectangle area cannot be silently discarded. The terminal physical bound4080 limits an adult center to4062. Default CI uses the20KB reduced derived fixture; optional `--native-archive` and `--scene-archive` read the supplied archives. Original archive bytes are not included.

Existing `tests/regional_passages/run_tests.py --sanitize` and21 `tests/ci` unittests passed in the isolated candidate. The existing regional suite still tests the overnight ascent on this branch; the ascent worker owns the replacement route and final integration checks.

Input archive SHA256:

* Original OoT: `ea80d61b223ee38075fe5383b652998f3b08164e903134922f63f205d995717b`
* Editable export: `82794582e4e6cebfbaf367ff46a588d4e4421c188bf347abae8f3d64b2dc8d49`

Still required on the integrated build: both ages' ordinary ascent/traversal/return, native adult moving icebergs and Ice Cavern behavior, vanilla/Djipi snow/decal appearance, Alt toggles/resource reload, repeated scene loading, and a post-build material replacement pack. Local snowfall/chill/shimmer and endpoint triggering are owned by the separate ascent/passage task. No executable was built, no game session was run, and no master was promoted by this task.
