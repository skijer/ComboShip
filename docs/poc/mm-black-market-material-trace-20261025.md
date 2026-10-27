# MM Black Market material decision trace

Parent: `991c21f245399299976a00bf3fef4ea68de0c3a7`, with accepted original material behavior from `7cd3ac8d09dfb72009c2404ef10ac8da4a62c0c1` retained. The reported failure after Magic Mirror, Lost Woods and TimeGate remains **unresolved**. A fresh-launch pass is separate accepted runtime evidence.

This candidate adds observation only. Material CVar, guard order, owner selection, graph validation, source binding, palettes, scroll commands and lifecycle cleanup retain their existing decisions. No fallback, cache flush, age repair or automatic recovery is added.

While inside MM's Black Market, ordinary info logs contain `BlackMarketMaterials:` once on entry and when its primitive state changes. No new debug toggle is required. Logs include:

- `result`: `attached`, `granny`, `room`, `disabled`, `shape-null`, `shape-layout`, `source-mismatch` or `resource-unavailable`.
- `prepare`: `skipped`, `ready`, `no-owner`, `source-alt-unpaired`, `graph-absent`, `root-not-display-list`, `graph-invalid` or `exception`.
- Scene/room, material CVar, Granny preference/current active state, persistent NEI adult flag, forced-adult CVar, MM Alt CVar and named-MM owner Alt flag.
- Named-MM/active manager identities, canonical source shape/type/entry count/opaque identity and selected display-list resource identity.

The snapshot stores no owning resource references. Diagnostics issue no additional resource loads or clears. Successful rebuild and reuse both report `ready`, preventing a second routine frame log. Granny's per-frame wrapper pointer is excluded; scene initialization/destruction resets the snapshot. Ordinary repeated drawing and exterior scenes produce no repeated output. Color animation is deliberately excluded from the key.

Read the next observed failure's `result` and `prepare` first. If it reports `attached/ready`, CVar and attachment succeeded; investigate rendering rather than altering owner/selection guards. If it reports a refusal, the logged reason identifies the boundary and state tuple. Process restart alone does not prove age or cache causation.

Static trace found no direct TimeGate/AdultLink mutation of the material CVar, room mesh, Granny state, named owner or Alt setting. MM TimeGate performs a live visual-age toggle. Supported solo startup **does** register MM: `MM_RunSolo` sets the shared-context transition flag before constructing MM's manager. An earlier potential missing-registration suspicion was rejected after tracing this call.

Verification uses native game headers and unchanged production function extraction:

```bash
source /workspace/scratch/321e7afbde59/evidence/fixture-env.sh
python3 -B tests/black_market/run_room_material_trace_tests.py
python3 -B tests/black_market/run_room_magic_tests.py
python3 -B tests/black_market/run_room_magic_loader_tests.py
python3 -B tests/black_market/run_room_magic_draw_exit_tests.py
python3 -B tests/black_market/run_native_tests.py
python3 -B tests/black_market/run_oot_native_tests.py
python3 -B tests/black_market/run_granny_native_tests.py
python3 -B scripts/diagnostics/run_receipt_syntax_tests.py \
  mm/2s2h/Enhancements/Gameplay/BlackMarket.cpp \
  soh/soh/Enhancements/Gameplay/ComboBlackMarketNative.cpp
```

The trace regression is also registered in the existing CI world suite. It exercises actual MM attachment, preparation and snapshot functions with controlled RM/archive/log boundaries, covering all refusal categories, owner/Alt/root/age changes, clear/re-entry, 500 identical draws, 500 failed draws, and 500 changing Granny wrapper draws. It asserts that disabled/Granny diagnostics load no resources. Real XML loader checks preserve MM's 7,577 triangles and 30 original scroll commands. None of these tests runs the game or reproduces cor's configured session; that runtime proof is still missing.
