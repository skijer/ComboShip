# Native forest authored presentation POC — 2026-10-21

Goal: bake the authored shop, poster, bench and return tunnel into the native
three-room forest while keeping the optional MM3D forest appearance separate.

Baseline: source `e62668e077d699dd51e91af891a5d3567f012332`; original
`tools/native_forest/resources.zip` SHA256
`7b21d19512a48b2363247b9276426ff6ba7afa4f10130f98513c22a90d0c6730`.
The accepted input remains unchanged. Candidate branch:
`poc/native-forest-shimmer-mirror-20261021`.

Authored donor: `zzzz_Skiejer_Native_Solo_Forest_FINAL_avatar_POC4.o2r`,
SHA256 `65901fcc27453a915be9414c7203e8295fce36ce69b7658653bbcdf978af8d0e`.

Architecture: retain native scene/collision/actors/room records and existing
shop geometry owners. Import only the authored additions' reachable resources
into a private namespace. Preserve exact pixels, UVs, vertices, triangles and
six scroll tracks; change only the portal mouth's primitive RGB to green.
Keep one deterministic source ZIP and restore ignored host outputs before
packing. Runtime particles and Mirror rendering are owned by the parent task.

Implementation plan and evidence ledger:

- [x] Add real-donor tests that fail when authored materials, poster, bench,
  tunnel, green mouth, six scroll tracks or native preservation are absent.
- [x] Implement the bounded donor bake and narrow texture validation; retain
  the original geometry-only transform as an explicit reproducible control.
- [x] Generate the one source ZIP/manifest and native readiness contract;
  verify exact bytes, draw closure, culling and two-tree preparation.
- [x] Run the native forest suite and production solo route fixtures, inspect
  owned-path diff, commit the bounded source checkpoint.

Protected invariants: all native scene commands; collision/waterboxes; room
commands other than room-zero mesh command 10; third room; actors, objects,
spawns, exits, transition actors, shop controller and solo entry/return/save
behavior. Existing shop vertex arrays and triangle lists retain their owner.

Review focus: donor identity mismatch; missing/recursive material dependency;
shared-mask global tint; hidden optional-pack dependency; geometry or collision
changes. Tests use the real donor and readiness gate rather than stubs.

Status: authored presentation implemented and source/native-fixture verified.
Configured runtime and user acceptance
remain unverified. cor has explicitly authorized publication of these bugfixes,
their source checkpoints, and source integration. Configured gameplay acceptance
and accepted visual-master promotion remain separate from that authorization.

Milestone 1: six presentation tests failed first for the missing bake. They
then passed against the real donor, including deterministic ZIP serialization
and both-tree preparation. The full native forest suite passed: 37 tests,
2 skipped because the original stock-MM/full-FINAL pair is not materialized in
this session. Those inputs' old geometry-only transform remains covered by its
controlled native graph. No native runtime was exercised.

Milestone 2: source bundle version 2 contains 82 resources, twelve exact donor
textures, and 45 authored source dependencies (three shop arrays are reused).
One ZIP is 49,702,619 bytes, SHA256
`0cc27b0fd623eb0f7915aa7d79a2b43c2381dae9e13e920a28d378cf55af0502`.
The donor archive and native control remain unchanged. No full MM3D terrain
display lists were included. Native collision/waterboxes and every protected
gameplay resource pass the pinned control SHA checks.

The actual bundled-readiness test failed against the old empty-scroll source,
then passed against version 2 for combined, MM solo and OoT solo. The full
native forest suite passed again: 37 tests, the same two donor-pair skips.
`tests/black_market/run_solo_forest_tests.py --native` passed both production
entry/return/save bodies under ASan/UBSan with the real bundle dependency
payload, including missing-pack and reload/void recovery. Actual preparation
restored 82 resources into each ignored host tree. Two ZIP serializations were
byte-identical. `git diff --check` passed.

Portal contract: `authored_portal_v2`; radius 68, center Y 66; stable native
`portal_root` draw/route marker; world anchor (-695,0,-313), yaw 12288. Runtime
particles are changed separately by the parent worker. The authored-only bake
does not change collision. A separately authorized fallen-log access step is
an independent candidate and must retain this milestone as its control.

Smallest configured game check: with no forest mod installed, enter the private
2Ship Lost Woods and inspect shop textures, separate poster, bench, green
scrolling portal and violet/black shop/stump effects. Enter/exit the shop,
return through the portal, pause/unpause and re-enter. Repeat the entry/shop/
return test in MM solo and OoT solo; check Alt on/off when using texture packs.
These checks remain untested here. No master is promoted.

Milestone 3: parent separately authorized the physical fallen-log access recipe
implemented and tested in `ec4bc59b`. The generator composes it only through
`--mirror-access`, after the authored bake. The current candidate includes its
three visible resources and additive 16/28/1 collision records, for 85 total
resources. It reuses the exact authored bark. Every native scene and non-mesh
room command remains unchanged. Removing exactly those appended records
restores each original collision SHA, including camera/waterbox bytes.

The integration test first failed for the absent composition function, then
passed for a fresh append, one root per room, a repeated call and an authored
rebake preserving the ramp. Final native forest suite: 38 tests, 2 original
donor-pair skips; production MM/OoT solo fixtures passed again under ASan/UBSan.
Actual ignored host-tree preparation passed. Current single source ZIP:
49,704,908 bytes, SHA256
`b0a33016b32d7e5415090038cff4cb8f4328cc8fd8a4b3f09d55918318be9fb5`.
The authored-only control remains independently recoverable from `05313bb2`.

Additional configured check, still untested: walk the visible log onto the
stump and collect the Mirror on top; approach below the stump and confirm
pickup does not occur. Verify the south recall landing and the shop/portal
routes remain clear. This source checkpoint is a candidate, not acceptance.
