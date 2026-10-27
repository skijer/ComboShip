# Black Market graphics-arena guard — October 22, 2026

The current shop audit found that both native seal renderers checked only an
allocation's returned pointer. MM's `GRAPH_ALLOC` and OoT's native
`Graph_Alloc`/`THA_AllocEnd` subtract from the OPA tail without rejecting
exhaustion. The prior fixture's optional null allocator did not exercise that
native behavior. A real short-tail fixture reproduced an AddressSanitizer
global-buffer-overflow before the guard.

`combo/BlackMarketGraphicsArena.h` checks the complete optional draw before
its first allocation. Its inputs are total aligned tail-allocation bytes and
OPA, XLU and overlay command counts. OPA tail alignment, allocation-size
overflow, reversed/null arenas and command/data overlap are rejected without
changing any arena pointer. The seal's budget includes its 64 vertices, one
matrix, 8 OPA packets, 64 XLU packets and 2 overlay packets. The OPA/overlay
allowance includes temporary native debug OPEN/CLOSE_DISPS nodes, including
the nested setup calls.

Only each host's `DrawSeal` allocation guard and required include changed in
the controllers. An arena with space retains the existing geometry, IA8 rune
texture, color, matrix and GBI state. An arena without space omits the
optional seal before writing vertices or commands. Stock, prices, tier gates,
item grants and persistence are unchanged by this fix. The shared helper is
also available to the independently owned interior effects.

Fresh verification:

- `python -B tests/black_market/run_lock_draw_tests.py` passes under ASan/UBSan
  for MM and OoT. It executes the production draw bodies, the actual host
  unbounded allocator macro/C functions and actual native debug open/close
  functions. The OoT allocator C bodies are compiled as C and linked into the
  C++ fixture without substitutions or permissive C++ conversions.
- Short vertex space, missing OPA command headroom, data-only exact fit,
  short XLU, short overlay and a reversed OPA arena all return without any
  pointer or buffer-byte changes. A tight valid budget renders successfully.
- Existing packet/vertex checks still cover blend/depth, LUT, texture and
  combiner state, fitted vertices, selected pose, locked-only capture,
  missing frames and arena-allocation failure, state restoration, visible
  shelves and independent interior effects.
- `python -B tests/black_market/check_lock_mesh.py` passes the focused mesh
  fit/volume, scrolling-UV and fast-math-safe invalid-bounds checks.

The audit inspected native SETUPDL_25 in both hosts: it establishes bilinear
filtering; each seal explicitly selects IA8, no TLUT, S wrapping/T clamping,
translucent blending and disabled depth writes. These are static/native
fixture checks. Fresh game/GPU appearance and runtime acceptance remain
separate and are not claimed here. No master is promoted by this guard.

The native cursor overlay now uses the same preflight with zero tail bytes
and budgets of 8 OPA, 8 XLU and 32 overlay packets. Debug open/close touches
all three arenas even for an overlay-only draw. The browsing fixture runs
the actual native debug functions and rejects exhausted and one-packet-short
OPA, XLU and overlay arenas without pointer or buffer changes; the complete
budget renders. All eight decoded cursor centers match each host's actual
native EnOssan projection in normal and mirrored-world modes. OoT also
matches its native mirrored analog and D-pad direction; MM retains its
native input direction.

The same focused ASan/UBSan browsing run covers NONE text, an inactive
scene/file/session predicate, transition trigger/mode, death and null-play
release, B from every phase during camera turns, result-to-browse release,
immediate re-talk, and destruction after actor/camera cleanup with poisoned
addresses. These cases introduced no lifecycle state-machine changes.
The codec fixture separately verifies native closing and choice reset.
