# Combined tunic and Ikana shield recovery candidate

The user extended the tunic pass to correct the flat Alt Ikana shield GI and the OoT Mirror Shield artwork appearing in its dialogue and equipment icons. Preserve the upright authored shield fallback, tunic designs and shimmer, complete model replacement priority, and independent MM/OoT Alt settings.

Baseline: `13901c677d0084e0305eec4672c0557f4434aea7`. The completed tunic-only source candidate is `89650909267abbf940ba684bd75bf009c1258d93`, preserved on `poc/mm-tunic-gi-recovery-20261007`. This authorized extension is isolated on `poc/mm-tunic-ikana-gi-recovery-20261007`. The original tunic recovery ZIP is retained separately.

## Source findings and correction

- Native OoT and foreign OoT Ikana recipes apply a fixed 90-degree X tilt to MM's worn shield model. A replacement already upright is therefore laid flat. Native MM's selected GI replacement takes the native draw fallback without an orientation correction, leaving an XZ-oriented replacement flat.
- Probe the selected owner's real display-list graph, including resource matrices and vertex resources, with the existing bounded geometry reader. Tilt by 90 degrees only when its Z span exceeds twice its Y span. Preserve the previous route's pose if the graph cannot be read. Preserve original scale, split passes, materials, spin and the authored renderer. The foreign OoT recipe also respects the same OoT local model priority as the native OoT helper. MM's separate native-fallback export uses a bounded OPS recipe for the same correction when the shield is placed in OoT, and remains live across owner Alt changes.
- Ikana's MM Mirror Shield icon uses the correct native `icon_item_static_yar` resource name, but an unowned path and the equipment cold-cache donor fallback can select the OoT owner. Carry explicit `@mm` ownership in Combo builds through both equipment grids, native and foreign message staging, the OoT custom icon catalog and the MM icon export. Standalone builds retain the raw native path.

## Verification and limits

- [x] Controlled production-function regression reproduced eight old-source pose/ownership failures before the correction.
- [x] Execute real draw selectors, both equipment tables, the cold equipment selector and the engine texture route at graphics/loader seams.
- [x] Fresh review found that MM-owned shields placed in OoT bypass the native MM draw switch. The added actual MM export/OoT consumer regression failed before that correction and passes afterward, preserving split passes, scale, stack, authored bypass and unreadable defaults. Existing full export/resolver/cache/latch/OPS dungeon and sword tests also pass with sanitizers.
- [x] Exercise the actual owner query and resource scope with real DisplayList, Vertex, Matrix, Texture and host Array types in standalone/combo MM/OoT fixtures. Cover planar meshes, nested XML/hash resources, resource matrices, missing resources and all independent Alt combinations.
- [x] Receipt/icon and real MM catalog checks, equipment checks (63 MM + 12 OoT), combined tunic/GI sanitizer checks, and both existing sword bounds fixtures pass. The broader receipt suite stops later at an unrelated seed-settings translation unit because `BS_thread_pool.hpp` is unavailable; that suite is not marked passed.
- [x] Fresh read-only source review. The MM-to-OoT export omission was corrected and verified; no Critical, Important or Minor source findings remain. Package status and exact commit/tree are recorded in the accompanying recovery checkpoint metadata.

These checks are controlled source evidence. The user's installed ROM/mod stack is unavailable, so they do not establish actual pack or GPU behavior. The pose rule targets an axis-aligned horizontal shield; arbitrary rotations, runtime segment geometry and graphs that replace the caller's modelview remain unsupported and retain their previous pose. A full production build and runtime acceptance remain pending. No remote push, merge or baseline promotion is part of this pass.

Runtime acceptance: receive all three tunics with Alt off/on; receive the Ikana shield through native MM and foreign OoT rewards with independent Alt settings; inspect both dialogue and equipment artwork; toggle after warming caches; confirm the authored fallback stays upright when no replacement is selected.
