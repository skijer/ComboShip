# Black Market magical materials with Alt Assets

| Field | Record |
| --- | --- |
| Baseline | Public cumulative candidate `0cd35e4d25f853dc68995dd88e8b1c177840e301`, tree `30c2a6d0ccce1587866df567560d48e9a3baf967` (PR66). Source recovery ZIP SHA256 `2e937c936bbf18e3c686c0661666d7584fb223c18837d0fc59e63881293893e1`. |
| Candidate | Local branch `poc/black-market-magic-alt-20261027`; exact commit/tree accompany the recovery archive. |
| Report | With Alt Assets enabled, Magical Room Materials leaves the shop's static textures visible; vanilla mode responds to the checkbox. |
| Cause | Preparation explicitly rejected an Alt source room without a separately paired magical graph. Attachment also rejected explicit `__OTR__alt/` source paths. |
| Scope | Material routing and source identity in the shared room helper and the two native attachment calls. |
| Preservation | Active source triangles, vertices/UVs, immutable loaded resources, host ownership, paired overrides on normal resource routes, palette/shelf isolation, Granny precedence, and exact draw restoration. Camera, collision, stock, actors, audio and all asset generators/payloads remain unchanged. |
| Status | Implemented and covered by scoped native/source checks; configured gameplay and user acceptance pending. No public push, full executable build, master promotion or merge. |
| Recovery | Apply this isolated commit to the cumulative source; retain PR66 and unrelated newer checkpoints. Do not transplant whole controller files over later work. |

For an unpaired Alt source, the owned graph copier now starts with the active
room and replaces only the 28 recognized material calls with the existing
private magical materials. Other draw commands and vertex pointers retain
their active source owners. Unknown/incompatible graphs still fail closed.
The ordinary vanilla and explicitly paired magical graph routes remain intact.

Both native hosts pass the actual opaque source identity to preparation.
An explicitly named Alt root is honored even when automatic Alt selection is
off; child references retain the resource manager's normal routing. Cache
identity includes the selected root path, resource, owner, host, Alt state and
source-copy mode. Source pointers are restored after each draw.

## Verification

The new unpaired-Alt resource assertion failed against the baseline. Explicit
Alt path matching also failed before correction. A subsequent production MM
attachment regression reproduced loss of a distinct Alt triangle with global
Alt off, and passed after the exact source identity was propagated.

Scoped commands (GCC C++20, ASan/UBSan; normal nlohmann JSON include required):

- `python3 -B tests/black_market/run_room_magic_tests.py`: actual owned copier,
  both native attachment bodies, live/reset/rainbow colors, shelf isolation,
  Granny/off behavior, source immutability, host ownership, explicit Alt roots
  with global Alt on/off, distinct Alt geometry/vertices, paired overrides,
  cache reuse, switching and teardown.
- `python3 -B tests/black_market/run_room_magic_loader_tests.py`: actual native
  XML DisplayList/Vertex factories; private/source modes in both hosts retain
  7,919 OoT or 7,577 MM triangles, 342 vertex loads and 30 magical scroll calls.
- `python3 -B tests/black_market/run_room_material_trace_tests.py`: production MM
  preparation/attachment, explicit source triangle, trace deduplication and
  owner/root/scene changes.
- `python3 -B tests/black_market/run_room_magic_draw_exit_tests.py`: native draw
  control flow and exact restoration, including pause/transition paths that
  bypass the ordinary end hook.

The unchanged six-test room asset suite also passed during baseline setup.
Initial native test setup lacked `nlohmann/json.hpp`; the final scoped runs use
the real v3.11.3 single-header dependency. This environment correction changes
no project files. Independent review's explicit-source finding was corrected
and re-reviewed. Full executable linking, Windows CI and GPU rendering were
not exercised here.

## Decisive runtime check

Use a matching rebuilt candidate with the existing accepted packs and load
order. In the Black Market, enable Alt Assets, turn Magical Room Materials on
and off, and confirm magical scrolling versus the original static appearance.
Check room color/reset, then repeat after an Alt off/on switch, pause/unpause
and exit/re-entry. Granny mode must still take precedence. Test the owning
host in combined/MM-only and OoT-only configurations before promoting this
source change; no existing runtime acceptance is broadened by these checks.
