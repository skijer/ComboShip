# Bundled MM dungeon keys and the winter follow-up gate

| Field | Record |
| --- | --- |
| Parent | PR #41, `bridge/audit-merge-20261008`, `e7165c78beb7e44fdddb543a2b38cc0caec55f16` |
| Failure | Build Artifacts run `38184738752`: the dungeon-key fixture rejected the newly added fairy-glass resource lookup; platform jobs were skipped |
| Scope | Bundle all eight existing MM key models; retain the repaired Woodfall flowers; select canonical MM resources with Alt on or off; repair the affected test fixtures |
| Preservation | Original key bodies, teeth, normals, materials, bounds, quarter scale, independent metal/emblem colors, MM ownership, receipts/grants, and the entire winter/Cape/fairy parent |
| Status | Source and fixture verification; full platform packaging and configuration-specific in-game acceptance remain separate |

## Asset provenance and ownership

The supplied `zzzz_MM_Dungeon_Keys_POC3.o2r` exactly matches the preserved source:
SHA-256 `2b4948ff7d784dbce5943f264ed8049b1e7e2a57b8744071fcf30b39c06b83c5`.
The existing surface-repair script reproduces the preserved repaired archive
byte for byte: SHA-256
`965b797c97d06b2717b46f92a5f8e2b81790753d294dbd470484690a6159b7a5`.

Its 53 resources are now tracked in
`mm/assets/custom/objects/combo_dungeon_keys/`, which the existing
`Generate2ShipOtr` target packages into `2ship.o2r`. XML dependency prefixes
change from the old Alt-only namespace to this canonical namespace. All other
resource bytes match the preserved repaired candidate, including the binary
16-by-16 RGBA16 Hilite texture. There are no duplicate copies in SoH's asset
tree. The existing foreign-item exporter routes these MM-owned resources to
OoT as `__OTR__@mm:objects/combo_dungeon_keys/...`.

Woodfall's small and boss flower meshes originally contain 44 and 56
coplanar overlaps. The repair separates the affected closed petal components
by two stored units, preserving their shared vertices and normals. Both now
have zero coplanar overlaps, zero open/nonmanifold edges, and unchanged bounds.
The earlier Great Bay and Stone Tower surface corrections remain intact; all
eight original donor bodies and teeth remain unchanged.

## Selection and gate correction

Key selection uses canonical paths and no longer requires an external POC3
archive or an enabled Alt setting. Each metal/emblem root can come from the
bundle or an enabled canonical Alt replacement. Missing roots preserve the
native palette/gem fallback. Native unloadable roots also fall back. The
pre-existing foreign exporter/interpreter behavior for existing but unloadable
roots is unchanged. Independent body/emblem color controls remain live after
the foreign pickup cache is latched. Maps and compasses retain their recipes.

The gate failure was a stale fixture assertion, reproduced on the exact parent:
`ResourceMgr_FileExists` accepted only `combo_bottle_gi/Empty` lookups. The
parent's real fairy selector also queries `FairyXlu` and selected bundled
shell/material paths. The fixture now models these exact supported resources
and asserts readable glass, missing-glass fallback, and selected casing/material
preservation through the MM export and OoT cache/latch. Unexpected bottle
lookups still fail. The generic native item-visual fixture explicitly models
missing bundled roots so its vanilla fallback remains independently exercised.

## Verification and runtime boundary

The asset regression first failed on the missing bundled resources, then
passed for all eight roots and complete dependency graphs, exact preserved
resource bytes, original bodies/normals, Woodfall clearance/topology and bounds.
The Alt-off model regression first failed on the parent's selection guard;
the corrected native suite covers all 256 owner/kind/native/Alt availability
combinations. Normal and ASan/UBSan key tests pass through the actual native
helpers, MM exporter, OoT resolver/cache/latch, and OPS replay. The production
native draw and MM item dispatcher syntax checks also pass. The key-receipt
suite preserves colors/icons, counters, queued grant timing and re-collection.
The full fairy suite also passes, including both real C++ host headers and
selected casing/material recipes. MM graphics-patch checks, Clang-format 14,
the staged asset-collision gate, conflict-marker scan and diff checks pass.

Independent review found no blocking asset, packaging, ownership, selection,
palette or fixture issue. These checks do not establish an in-game appearance
or a completed Windows/Linux build.

Use the updated executable and matching port archives. A separate key O2R is
not required. Inspect Woodfall small/boss pickups in native MM and on an OoT
check with MM ownership, with Alt off and on, and confirm stable petal surfaces
and live independent colors. The original upload and historical source/repair
archives remain unchanged for recovery; no master promotion is implied.
