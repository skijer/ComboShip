# Hyrule Field resource selection regression

Source baseline: draft PR #41, `7511aedad2e21ce22e9c3fea1f01782d475e04f0`
on `bridge/audit-merge-20261008`. Accepted develop is unchanged.

Cor reports sporadic corrupted/native-looking Field grass and disappearing
Prelude geometry, including Granny's shop, Romani's house and the castle mesh
behind Market. Field → Kakariko → Field did not restore them. The supplied
screenshot was inspected; it does not identify an executable commit or prove
the underlying resource selection. The older October 14 log contains several
build sessions and is not treated as a log of this new incident.

The inspected, unchanged source archive is `!Hyrule Field - FINAL(1).o2r`,
SHA-256 `6a3c2662e03598a1b3aa4eaa6d421466dac9d0a295f121af0d5c7d864b5d49ad`.
It contains 4,334 entries, its Prelude edit metadata, and native/Alt room
headers with different custom mesh bindings. The archive's presence and
contents do not establish which header the installed game actually selected.

## Bounded correction

`GameState_Destroy` unloads `alt/*` while unprefixed cached resources can
remain. `ResourceManager::CheckCache` previously treated an uncached Alt
resource like an absent override and returned the retained native resource.
The renderer lookup shortcut, synchronous loader and asynchronous loader all
consult this decision. `LoadResourceProcess` instead probes Alt before cache.

An unresolved Alt cache entry now returns `NotCached`, allowing the loader to
probe its replacement before accepting the native hit. Warm Alt hits, known
Alt failures, dirty/null handling, exact requests, owner/archive-parent scope
and Alt Off retain their paths. A previously unprobed missing Alt incurs one
cold probe; after its missing result is cached, native warm hits stay warm.

This changes one cache-selection predicate. No additional cache, geometry,
vertex math, material/UV, collision, archive, save, actor or seasonal palette
change is included. Current weather, rod, lighting and NEI integrations stay
at the source baseline.

The same incorrect predicate was inspected in accepted performance-baseline
source `9c084d74a7f7d0ae8cf88b18c0ed74abde03a950`. The flaw therefore predates
the current weather merge; neither its original introduction nor its causal
role in this installed-game report is claimed proven.

## Verification boundary

The new test executes the production cache and loader decisions with actual
Texture/DisplayList resource types. It models travel by evicting only the Alt
cache entry over three visits. Archive import, `.meta` resolution and thread
scheduling are fixture boundaries, not an actual game replay.

- Baseline: 36 wrong texture/display-list selections per host, 72 combined,
  through renderer, synchronous and asynchronous entry points. The direct
  process path serves as a passing control.
- Corrected OoT/MM fixtures pass with UBSan and with ASan/UBSan, including
  native/Alt, direct/metadata-backed replacements, dirty reload, missing,
  empty/wrong types, exact requests, owner/manager and archive-parent checks.
- Warm synchronous-hit zero-allocation checks remain included.
- Seasonal pixel and binding tests pass normally and with ASan/UBSan. Source
  pixel/alpha/metadata preservation, protected atlas regions, season changes,
  external replacements and retirement remain their scope. All 55 archived
  Field target/namespace texture cases also pass with ASan/UBSan, across the
  four palettes with source, alpha, non-green pixels and metadata preserved.
- Production segmented display-list dispatch, raw/segmented texture-address
  and texture-failure command tests pass with ASan/UBSan. The custom texture
  scroll production runtime test passes.
- The complete ResourceManager translation unit passes C++20 syntax checking
  with engine headers; existing ignored-future warnings remain unrelated.
- Independent review found no blocking correctness issue. Additional
  scratch-only UBSan controls cover conflicting owners/archive parents,
  negative/null/dirty Alt entries and explicit Alt paths.

LeakSanitizer cannot inspect executor processes; ASan runs use
`ASAN_OPTIONS=detect_leaks=0`. Address/undefined behavior checking remains
enabled. Initial non-PIE flags on adjacent renderer fixtures put static test
pointers in the reserved segmented-address range; the unchanged fixtures pass
with their ordinary CI flags. Neither limitation warranted production changes.

Hosted complete build/gate results belong to the exact published candidate.
Installed graphics, actual RAM/VRAM/FPS and runtime acceptance remain pending.

## Native/Alt and optimization audit

Alt Off selects the unprefixed namespace using mod priority. It does not bypass
mods to retrieve original ROM assets. This archive supplies native room/DL
and texture replacements, so Djipi-like resources there can remain visible.
Shared `custom/prelude` resources may be referenced by both render modes
without being drawn or allocated twice merely because both modes exist.

The archive contains 93 native/Alt resource pairs. Eight are version-1 texture
pairs: their native pixel payloads total 999,424 bytes (0.95 MiB), versus
107,479,040 bytes (102.5 MiB) for Alt. For example:

| Field grass | Dimensions | Pixel payload |
| --- | --- | --- |
| Unprefixed `s00_yuka_01_78B00___Copie` | 256 × 256 | 256 KiB |
| Alt counterpart | 2048 × 2048 | 16 MiB |

The larger duplication is across recipe resources: 43 groups have identical
texture bodies, including format, dimensions, flags/scaling and pixel bytes.
Repeated copies account for 191,573,088 bytes (182.7 MiB) of stored pixel data
across the supplied archive. Two large groups each repeat a roughly 6 MiB
texture 12 times. Different paths can produce distinct loaded image allocations
and GPU cache keys. These byte totals are not measured resident memory or a
guaranteed saving: active references and cache residency still need inspection.

Sharing identical immutable payloads is a future bounded optimization candidate,
with resource metadata, archive priority, lifetime and seasonal atlas bindings
preserved. This correction does not delete native resources, remap recipes,
replace the sacred archive or claim any FPS improvement.

## Decisive runtime check and recovery

Use the same save/age/setup, full mod stack/order and Field archive as the
incident. Toggle Native → Alt, visit Field → Kakariko → Field twice, and check
the grass, Granny's shop, Romani's house and the castle mesh. Repeat once with
seasonal presentation Off, then restore the reported Spring setting, checking
pause/unpause and the material appearance. This separates resource selection
from seasonal presentation without adding a diagnostics-only build.

Status: isolated source correction and local verification; reported installed
Field regression is not yet runtime proven or accepted. Recovery is the
unchanged parent source and original archive. No develop/master promotion.
