# Black Market collision-index overflow

Baseline: PR #53 head `525b3160660225a06700470fb39d9d4948834414`, tree
`1ec44483e21dd0820ea7df066ba6ffc71b51249d`. cor's supplied Windows crash is
the PR merge build `8303491aa4ffebdebb59984a3b9cb949374131e0`.
Candidate: `fix/black-market-collision-pool-20261021`.

## Evidence and cause

The supplied log records a successful native Black Market door claim with all
entry checks passing, then entrance `0x1200`. The new failure happens during
the destination's collision initialization, before that initialization returns.
It is separate from the earlier player cutscene-ID and resource-readiness fixes.

The exact Windows artifact is `12384744496` from run `39677652804`; archive
SHA-256 `66bfd2bd2a2026526bc0ed7a6d8608f55e3dbaeed289b5604db6bb8f38014de6`.
Its `2ship.dll` PE timestamp `0x6ad936c4` matches the minidump. Disassembly of
RVA `0x5eb6af` identifies `StaticLookup_AddPolyToSSList`: it follows an SSNode
to a corrupted polygon index and reads outside the collision polygon array.

In the dump the node pool starts at `0x20c6327b160`, has capacity 26,932
four-byte nodes, and ends at `0x20c63295630`, exactly the adjacent lookup table.
Its count has already reached 29,649. The native MM node allocator does not
bound this insertion; excess nodes overwrite the lookup heads, corrupting later
traversal. The source fixture catches the first out-of-bounds write rather than
waiting for the later invalid read shown in the dump.

The bundled collision and the collision extracted directly from that build's
`2ship.o2r` are byte-identical: 152,722 bytes, 4,573 vertices, 7,823 polygons,
SHA-256 `382b641d48d0f84f1a31b5a8660808cec2401344a520d0179c0758bc715a6f48`.
The real subdivision builder needs 34,490 nodes for those polygons.

Supplied evidence hashes:

- Log: `c1fe7ef6b158192544997095be099a351d63bf075ae4d0693644b52e3175c603`.
- Dump: `681889eea0966da33496b10ddefb597da4843380958aaed67279f4aa7339f860`.

## Bounded change and preservation

Add a `COMBO_BUILD`-only memory-policy entry for the private MM market scene
`0x02`: `0x30000` bytes instead of the native fallback `0x23000`.
This reserves 40,244 static nodes, 5,754 beyond the actual requirement, while
retaining native `16 x 4 x 16` subdivisions and dynamic collision capacities.
The additional budget is 53,248 bytes for this scene only.

All archives, collision polygons, vertices, bounds, surface flags, cameras,
geometry, UVs, materials, water, keeper behavior, stock and purchases remain
unchanged. Preserve the existing door checks, cutscene normalization, local
return and re-entry behavior. Original non-Combo MM policy remains unchanged.
The OoT implementation is not modified.

## Verification

`python3 -B tests/black_market/run_collision_pool_tests.py --source-ref
525b3160660225a06700470fb39d9d4948834414` reproduces the baseline's ASan
heap-buffer-overflow in `SSNode_SetValue`, at the first write beyond its
107,728-byte node allocation. The normal-build control passes first.

The same runner without `--source-ref` passes three builds:

- Original MM policy with `COMBO_BUILD` absent.
- Complete production `z_bgcheck.c`, `sys_math3d.c` and `z_lib.c` with separate
  allocation blocks to expose pool boundaries to ASan.
- The same production code with the real `TwoHeadArena.c` allocator and arena.

Both Combo modes complete four fresh initializations. Every lookup chain is
acyclic and in bounds, every generated node and source polygon is reachable,
and 98 floor queries per initialization match a linear native triangle oracle.
The query fixture walks lower Y cells as the production caller does. It invokes
native static list queries; it does not simulate a complete running game.

ASan and UBSan are enabled, except the existing native cube-mask signed shifts
into bit 31 (`-fno-sanitize=shift`). The arena, fault-reporting and binary-input
adapters are explicit test boundaries; no application resource factory or
render loop is claimed. The new regression is wired into the existing shop,
decoder and particle CI entrypoint.

Independent review found one floor-oracle traversal defect, corrected and
independently rerun successfully. No remaining actionable review findings.

Configured Windows gameplay is pending: enter the market, wait/move and browse,
leave, and re-enter with the new executable. Keep the current mod configuration.
No replacement scene archive is needed for this source fix. Other custom
collision replacements with a different node requirement are untested.
This is a source-verified candidate, not accepted runtime/master promotion.

## Recovery

The separate source checkpoint contains the exact parent revision, patch,
complete changed source/test copies, evidence logs and restoration instructions.
Its receipt records an independent restore and implementation tree. Apply the
patch to the recorded parent to retain all prior morning integrations and this
bounded repair. Reverting the single production memory-policy entry restores
the former behavior; no input or accepted scene archive was overwritten.
