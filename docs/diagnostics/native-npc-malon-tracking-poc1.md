# Malon tracking pivot POC1

The replacement Malon rigs already receive the actors' head and torso tracking.
Their fitted head origins sit at the source models' neck joints, while the draw
callbacks still rotate around the original fixed `(1400, 0, 0)` pivot. This makes
the fitted neck anchor move during tracking, especially for young Malon.

| Field | Record |
| --- | --- |
| Source baseline | `d835ec0122cbc691abd52805bbab7ff363c7df69` |
| Candidate branch | `poc/malon-tracking-pivot-20261018` |
| Accepted visual packs | Young Malon POC7; adult Malon POC5 |
| Permitted change | Head tracking pivot in En_Ma1, En_Ma2, En_Ma3, and both Malon En_Viewer draw callbacks |
| Result | Active replacement models rotate around the current limb translation; other models retain the original pivot |
| Preserved | Model geometry, poses, textures, faces/blinking, torso tracking, native animation identity, singing tracking rules, player targeting, placement and interaction logic |
| Configuration | Existing NativeNpcV1 adapter; Alt Assets enabled; each accepted model pack installed once |
| Evidence | Real draw-callback regression, resource routing/pose/skinning/sampler tests, real actor compilation |
| Verdict | Source and focused checks verified; full game build and runtime playback untested |
| Promotion | Isolated source checkpoint only; no master or published build changed |

The accepted child's local head offset is `(645, 0, 0)` and the adult's is
`(1340, 0, 0)`. Before the correction, a 22.5-degree tracking turn displaced the
child's neck anchor by 294.586 model units (2.94586 world units at actor scale
0.01). The regression test executes the actual callbacks and checks the anchor
stays fixed for active replacement rigs. It also checks original-pivot behavior
for Alt off and every incomplete-pack state, offsets on all three axes, and the
static singing poses' tracking suppression. There are 1,442 passing callback
checks across the five affected callbacks.
Each tracking case also checks an off-origin head point, so leaving the head
still or turning it in the wrong direction cannot satisfy the anchor test.

Run the focused suite from the source root:

```sh
python3 scripts/diagnostics/run_native_npc_model_tests.py
```

The suite also passes existing model routing, private pose resources, live
joint blending, animation sampling and compilation of eight real actor units.
The baseline emits existing discarded-array-qualifier warnings from asset
pointer declarations; these are also present in the candidate run.

No model archive regeneration is required. Preserved pack SHA-256 values:

| Pack | SHA-256 |
| --- | --- |
| `Young_Malon_Native_NPC_POC7.o2r` | `3a773eaed193a4126cabefb8b68ae7fd24a29f05d7a8dfba52ef77f8e9702748` |
| `Adult_Malon_Render_Native_NPC_POC5.o2r` | `0fe195bc5eaa4f5170ba91aae1638febd577a6f91595d713b6c3b3a959cf49a2` |

The next runtime check is to approach each idle placed Malon from both sides and
from different heights, then repeat adult basket tracking and both singing
placements. Inspect head turning, the neck/scarf join, hair/shoulder clearance,
and the return to neutral. Repeat with Alt off for native behavior. Regular
Malon actors retain their existing conversation and singing tracking rules.
