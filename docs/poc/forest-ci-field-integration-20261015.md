# Forest passage, CI repair and retained Hyrule Field correction

## Source and preservation

Host: PR #43 `bridge/cross-entrance-recovery-20261015`, published `e00fb378`.
Forest donor: reconstructed `c2a4add83e1ffde095bdd2dbe5863895d399bb89`,
base `a82852a3`. All 15 donor files match the attached full-source ZIP and
the recovery manifest. The expired original `ae5c3162` is not claimed recovered.

The donor is applied as a delta. Newer cross-dungeon/boss recovery, shared
checks, save ownership, Farore recall and ordinary Market/Clock Tower portals
remain in the host. Demise is deferred. No scene archive, mesh, collision,
texture, UV, actor placement or accepted develop/master baseline is replaced.

## Hyrule Field

`libultraship/src/ship/resource/ResourceManager.cpp`, the production cache
fixture and `field-resource-regression-20261014.md` are byte-identical to repair
`e4813206`. This is the source correction for cold Alt resource lookup after
scene teardown: retained native textures or display lists cannot bypass an
unprobed Alt replacement. It covers the grass patches and the render paths for
the missing Prelude recipes (Granny's shop, Romani's house and castle mesh).
It does not modify or reconstruct those meshes or the sacred Field archive.

Fresh ASan/UBSan fixtures pass in both hosts over three simulated scene visits,
native/Alt changes, texture/display-list types and renderer/sync/async/process
entry points. Installed Field appearance remains unverified; this evidence
does not prove that the cache flaw caused every reported visual symptom.

## Integration corrections

- The hosted gate failed because the OoT seasonal fixture extracted new logical
  boss-clear calls without their API. It now uses the real boss completion
  state and checks physical-versus-source clears, native fallback and latching.
- The complete 122-line host NEI runner is retained, with forest, cross-entrance
  and Farore suites appended. No prior test is removed.
- Forest positioning is consumed but not installed over a composed OoT arrival.
  The regression fails with the real consumer contract before correction and
  passes afterward; the fixture no longer invents respawn clearing.
- MM captures the forest departure slot and cancels its fade on Play destruction,
  stopped gameplay or slot changes. Production MM and OoT handoff registrations
  exercise reset, successful crossing, shuffle priority and Farore ownership.
- Weather, renderer and horse-save fixtures are updated for newly introduced
  production header/API dependencies. Renderer submission and save serialization
  remain production code; unrelated logging/archive and runtime services are
  explicit fixture boundaries.

## Verification record

Focused forest arrival/trigger/handoff suites and the two-host Field sanitizer
suite pass. All six final affected native translation units compile against actual
engine headers: both title loaders, both host handoff implementations, the launcher
and the new MM passage renderer. Source hashes match the compiled candidate.

The remaining hosted CI groups pass locally, including weather/particles, renderer
triangle/vertex equivalence, audio, cosmetics and horse saves. The full NEI gate
is checked on the same source with verified continuations after correcting older
fixtures for the new checkpoint and runtime-binding services. No production
behavior is removed to accommodate those fixtures.

Runtime status: candidate, not installed-game acceptance. The decisive checks
remain a forest round trip in both directions, normal maze/swamp/Market routes,
shuffled dungeon and Farore returns, and Field → Kakariko → Field with native/Alt
and the reported Djipi/Prelude stack, checking grass and all three recipe meshes.
