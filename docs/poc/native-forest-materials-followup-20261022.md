# Native forest material and placement follow-up — 2026-10-22

| Field | Record |
|---|---|
| Baseline | `develop` source `c3b0c76b81766ce51378e573dcf9816ab3c59473`; native bundle ZIP `b0a33016b32d7e5415090038cff4cb8f4328cc8fd8a4b3f09d55918318be9fb5`, 49,704,908 bytes, unchanged |
| Candidate | `poc/market-forest-followup-20261022`; bounded source candidate, no visual-master promotion |
| Scope | Ramp material draw ownership/scroll, companion flanks/facing, Mirror cap arrival heading |
| Preservation | Native three-room framing, stock appearance, all scene/collision/room/texture bytes, every ramp vertex/UV/triangle, original six scroll tracks, shop/posters/portal/Saria shimmer and cross-game bridge behavior |
| Configuration | Native combined/MM solo/OoT solo forest assets; unchanged external `mm.o2r` texture owners and optional Alt lookup; configured game rendering remains untested |
| Evidence | Supplied `033447`, `033549`, `033609`, `033631`, `034214`, `034312` screenshots inspected directly; source bytes and native fixtures described below |
| Recovery | Original six source parts and ZIP identity remain unchanged; preparation validates the original before applying hash-pinned `presentation_polish` version 2 |

The alternating white/black bark is a draw-state leak. The ramp's material
selects one-cycle rendering and clears fog. Its former root restored lighting
and back-face culling only. Native cullable entries are depth-sorted, so the
ramp sometimes preceded the main tree, the moved stump or the connecting
tunnel. Their unchanged stock combiner `FC272C04 1F0C93FF` blends two textures
in cycle zero, then applies vertex shading in cycle one; they inherit the
room's two-cycle setup. Running that combiner after the ramp's one-cycle
material loses its intended shading path.

The real stock archive `mm.o2r`, SHA256
`f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59`,
was inspected. Relevant stock lists explicitly use `E3001001 00000000`
(`G_TT_NONE`) and the native I8/I4 texture hashes. The private moved-stump list
retains the same material and original external texture owners. No palette,
texture pointer, native DL, texture payload or Alt namespace was rewritten.
The repair restores the native two-cycle/fog/tile-zero state after the final
ramp triangle, independent of camera depth order.

The static ramp lacked a dynamic material call. It now loads the existing
authored 32×64 I8 wall mask into render tile 1 and calls segment 8, whose
second layer already scrolls at `(0,-1)` in both host adapters. Tile 0 of that
track is stationary and 32×32, so simply attaching the segment to the old
tile-zero mask would have selected the wrong layer/dimensions. The exact
TexAnim and OoT material Blob remain unchanged.

Companions use the existing authored portal transform, anchor `(-695,0,-313)`
and yaw 12288. Their local lateral offsets move from ±230 to ±120; depth stays
120. The resulting native positions round to Saria `(-630,0,-156)` and Epona
`(-538,0,-378)`. Native collision supports their sampled footprints without
nearby walls. Both face the portal's forward path at yaw 12288, matching the
marked screenshot's arrows. These are authoring-derived coordinates, not
purported pixel-exact coordinates recovered from the screenshot.

Mirror cap arrival now aims from the located cap toward the active return
portal, matching the final `034312` view toward the market/portal. The retained
default cap `(-377,80,416)` gives about 204 degrees in MM's packed actor yaw.
The old fixed 90-degree cap heading aimed toward the opposite tree. Position,
floor/wall safety checks, ground fallbacks, cached resources, respawns, pickup
logic and destination guards remain intact.

Validation includes red/green regressions for state leakage, absent ramp scroll,
old companion offsets and the old Mirror heading; exact prepared-resource
membership/hash checks; the native XML and binary factories under ASan/UBSan;
real MM companion spawn bodies and duplicate guards; real MM/OoT Mirror travel,
arrival and pickup adapters; native readiness/owner/dependency/material/scene
load fixtures; both solo routes; native spawn rotation; additive stump access
and hook ordering. The authored donor used for full forest/appearance checks
is SHA256 `65901fcc27453a915be9414c7203e8295fce36ce69b7658653bbcdf978af8d0e`.

The full native forest suite passed 57 tests with two skips for the unavailable
original full-FINAL geometry donor pair; all authored and optional-appearance
tests ran and passed. Actual preparation restored 85 resources, including the
same twelve textures, in each port. Full forest actor diagnostics passed,
including custom draw scale 1.10 applied once, mixed V1/V2 providers, Alt
rebinding, blink, talk, save and resource lifetime. Existing controller macro,
const/pointer and deprecated subscript warnings remain in that fixture build;
they did not prevent compilation or checks.

Runtime proof is still required in the user's actual pack/load-order settings:
turn the camera across the portal tree, connecting tunnel and stump; confirm
the ramp scrolls while walking; inspect both companions; use the Mirror to
check the initial player/camera heading. Repeat pause/unpause and re-entry,
and verify Alt on/off. Source/native checks do not establish visual acceptance.
