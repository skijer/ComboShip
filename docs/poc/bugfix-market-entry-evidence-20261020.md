# Black Market entry evidence — 2026-10-20

This candidate has executable production-controller evidence for the MM market
and OoT solo private market. **Configured in-game entry remains unproven.** No
accepted master is promoted by these checks.

## Candidate and preservation

The isolated branch is `fix/bugfix-integration-20261020`. It incorporates native
forest/market `9d5f550d9`, access/diagnostics `1d19e86cd`, camera/NPC `a01321fe0`,
and the integration's later fixes. The final test receipt records its exact
source HEAD and SHA256 for each exercised production unit. This checkpoint adds
tests and evidence only; it changes no production controller or resource bytes.

Keep scene IDs, native door recognition/safety, original/combined-game guards,
slot/seed binding, grants/prices/stock persistence, destination recovery,
RETURN/TOP respawn ownership, scene geometry/collision/UVs and optional-pack
priority intact. Input archives are read-only and remain at their original
identities.

| Input | SHA256 |
|---|---|
| Actual supplied stock `mm(1).o2r` | `2f44bafe1a00f44ddaf7eb5e5c99e73a3d23771038529d6d1748819ad575dc80` |
| Actual FINAL POC19 authored forest | `9097583c896a11c204e4950bd9a80aa8e69fb2a42307f55f50e75d4195f30e42` |
| Actual POC14 market | `3ac1432d0e8f75e51f08892739a13e16762bb4d820a2bbc7a35469af1db872e2` |
| Checked-in native forest bundle | `7b21d19512a48b2363247b9276426ff6ba7afa4f10130f98513c22a90d0c6730` |
| Checked-in native market decoded bundle | `1b75185882cacd214b18a74e9aecc8bad19df0eb0aae72aad483cd38c6fddba9` |

## Why application testing could not run

Inventory of `/workspace` found no Linux game executable. Available older
artifacts are Windows PE DLLs plus `Fleet.exe` inside
`ComboShip-windows-450b293.zip`; they predate this integration. Neither Wine nor
Xvfb is installed. A stock MM archive and older port archives do not provide a
runnable integration candidate. No full game build, installed game, or hosted
build was started by this test workstream.

The authorized fallback therefore executes the production entry path in two
ASan/UBSan executables. It does not substitute compilation, a declared inventory,
or a successful fixture for gameplay acceptance.

## Executed route

`tests/black_market/run_resource_entry_tests.py` verifies source bundles, runs
the normal preparation functions into isolated asset trees, and consumes those
actual files. Full mode also verifies the three immutable archive hashes and
decodes actual base/optional resources. It uses 164 generated native resources
and 235 actual base resources, including the native keeper's serialized graph.
No external resource is manufactured to satisfy a readiness query.

The executables use real engine types and exact current production bodies for:

- Native MM door offer/caller/Open hook and the OoT Open hook/animation cleanup;
  authored transition rows come from real scene command bytes, including native
  packed MM yaw and the production conversion to OoT binary yaw.
- `DestinationAvailable`, `PackAvailable`, `TryDoorEntrance`, entry normalization,
  scene initialization, player update, travel binding, exit/return and re-entry.
- Native forest dependency parsing and the complete private/base readiness gate.
- OoT `LoadNativeKeeper` and `NativeModel::Load`, including all 23 real limbs,
  animation metadata, 16 graph roots and the production display-list command
  patcher with actual donor vertex/texture/list dependencies.
- Native MM destination registration and the actual OoT entrance table.

The destination selected by the controller is checked against the decoded real
scene header, room-zero header, collision identity and spawn `(-11, 0, 381)`.
The authored market and bundled market destination framing are byte-identical.
MM uses entrance `0x1200`, local scene `0x02`; OoT uses entrance `0x614`, private
scene `0x6E`, with return to private forest `0x618`/`0x6F`.

Each successful case performs three full controller cycles: native Open,
transition destination, entry normalization, real spawn, stay inside, local exit,
grounded outside return and re-entry. RETURN is restored, TOP is unchanged,
stock is not regenerated, and a purchased bit persists across the cycles. An
unbound/empty market session stays inside, creates no purchase and still exits.
MM's unavailable text remains outside the credits decoder's ID range.

## Results

| Configuration | Executable result |
|---|---|
| MM combined native forest, no supplemental scene/visual archive, stock and no stock | Pass; three cycles each |
| MM solo private native forest, same variants | Pass; three cycles each |
| OoT solo private native forest/market, same variants | Pass; three cycles each |
| Actual optional POC14 visual resource view, both hosts, stock/no stock | Pass; native structural routes remain available |
| Actual POC14 authored market destination, both hosts | Pass; same destination framing and spawn |
| Actual authored POC19 MM transition/collision | Pass; three cycles with and without stock |
| Production conversion of actual POC19 transition rows to OoT | Pass; three cycles with and without stock |
| Genuine missing scene, room-zero header or collision | Pass; each missing path blocks Open, logs the exact path and cleans up the native action |
| Genuine missing native forest private/base ancestry or the entire base owner | Pass; full forest readiness fails closed |
| Genuine missing keeper, limb, display-list or material | Pass for the intended separation: presentation readiness fails; structural entry remains independent |
| Duplicate shop marker and paused player | Pass; no market transition |

Full mode covers 14 MM cases and eight OoT cases: **66 entry/exit/re-entry
cycles**, plus negative checks. The committed receipt/logs identify the final
tested source and binaries. Existing `run_entry_access_tests.py` remains the
separate exhaustive safety-state suite.

The CI `--bundled-only` mode runs the same structural production routes with the
actual checked-in destination/forest bytes and an **empty base-owner index**. It
performs 18 cycles across MM combined, MM solo and OoT solo, stock/no stock,
plus structural and safety failures. It explicitly requires full forest and
keeper readiness to fail without the genuine base owner. It provides structural
controller coverage only, and never silently replaces the full real-input run.

## Exact commands

On this machine the nlohmann header is a separate include root. An environment
with that header already installed may omit `--include`.

```bash
python tests/black_market/run_resource_entry_tests.py \
  --bundled-only \
  --include /workspace/scratch/593f6dd85762/analysis/dependencies/nlohmann_json/include \
  --output-dir /workspace/scratch/9dcd75c38e6f/market-entry-bundled-evidence

python tests/black_market/run_resource_entry_tests.py \
  --include /workspace/scratch/593f6dd85762/analysis/dependencies/nlohmann_json/include \
  --mm-base '/workspace/scratch/4589f3b809b8/upload/mm(1).o2r' \
  --authored-forest /workspace/scratch/4589f3b809b8/inputs/zzzz_MM3D_Lost_Woods_FINAL_avatar_POC19.prelude.o2r \
  --market-pack /workspace/scratch/4589f3b809b8/inputs/zzzz_Skiejer_Black_Market_POC14_EntranceOrientation.o2r \
  --output-dir /workspace/scratch/9dcd75c38e6f/market-entry-evidence
```

The runners preserve generated C++, binaries, logs and `entry-receipt.json` in
the named output directory. Full mode requires all three real inputs and fails
on missing files or a hash mismatch. Committed copies of the receipts and text
logs live under `tests/black_market/evidence/20261020/`.

## Explicit boundaries and next decisive test

Archive membership, binary-to-native data imports and floor traversal have
test adapters. Floor queries scan the **actual static collision triangles** with
production Math3D predicates, not the engine's live collision context/spatial
index or dynamic actors. Scene transitions are advanced explicitly; actual
scene loading, actor/object initialization and the game loop are absent.
Existing grant/save/camera/actor-spawn adapters retain their original fixture
roles. No configured renderer, sound, menu, collision traversal or animation
appearance is proven by these checks.

The `mods/2ship` visual case indexes decoded POC14 visual files laid out beneath
that test directory. It does not execute the application's mod-directory scan,
mount priority or Alt renderer. The authored OoT case uses the actual authored
transition bytes through the production converter; the previously distributed
solo FINAL archive itself was unavailable. These cases cannot establish that
the old combined FINAL art targets the new native graph; the compatibility
limitation in the native-forest design remains.

Physical entry's contract is the three structural destination resources.
Missing optional art never vetoes it. Missing native keeper/material resources
can fail presentation readiness while the structural destination remains
enterable. A genuinely absent scene, room-zero header or collision blocks entry;
the test does not reclassify every presentation dependency as an entry lock.

Next, launch the exact linked candidate and actual port archives in both game
checkouts: native door interaction to market, keeper/no-stock dialogue, purchase,
exit and re-entry, first without supplemental scene art and then with the
intended visual/Alt load order. Capture the attempt/return logs and a clip tied
to that build. Until that occurs, configured game entry and user acceptance stay
**untested**, and the candidate stays unpromoted.
