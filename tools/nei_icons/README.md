# Optional GI HD icons

This optional asset-only archive replaces the requested 23 inventory icons
with 512×512 RGBA art while retaining their 32×32 logical UI size. The game
archives retain their original icon assets; removing this pack restores those
assets. The accepted baseline commit
is `122dd5f68cb37fcf515c3726f8a77043db5dcdf4`; no game runtime is claimed here.

Six Slate variants, Elemental Wand and its six rods, Demise Destruction, Mario
Mask and Climbing Boots render the actual `tools/nei_gi/CHECKPOINTS` geometry and materials. The
six non-Magic RPG icons parse their native XML display lists, including nested
calls, vertex loads, triangle indices, signed normals and primitive/environment
colors. The Power model lies in the YZ plane, so its camera faces that plane.
No original 32px icon contributes pixels to these images.

Magic's native ROM-only jar geometry was unavailable. Its separate transparent
image was authored with the built-in image generator; the full prompt and
1280px source are retained in `SOURCE/`. It has a private `gStatMagicTex` path,
so other Magic Jar icons and their 24px UI layouts are unaffected. Without the
pack, the upgrade uses the native Magic Jar icon and its 24px layout. Both item
receipts and GUI texture registration resolve that fallback.

Lighting is an offline approximation. Missing ROM-only reflection/decal maps
on three RPG models use neutral illumination with the source colors; these
limitations and every source hash are recorded in `manifest.json`. Model
silhouettes and native mesh topology remain unchanged. The renderer uses
2× supersampling and premultiplied alpha filtering for clean transparent edges.
The source Demise core is exactly black; its native pale translucent shell
still overlays that core.

Rebuild from the checked-in sources with Python, NumPy and Pillow:

```sh
python tools/nei_icons/build.py --render --manifest --pack /tmp/nei-icons-hd.o2r
python tools/nei_icons/build.py --verify
python tools/nei_icons/verify_routes.py
python tests/optional_assets/run_tests.py
```

To package the checked-in artwork without rerendering, use only `--pack`.
Neither command writes the game asset trees. Authoring PNGs remain under
`PNGS/` and the review sheet is `contact_sheet.png`. The unchanged October 4
render-driver snapshot in `SOURCE/render_driver_20261004.py` preserves the
recorded provenance of the current artwork. New renders record the current
driver instead.

The serialized format matches `libultraship/src/fast/resource/factory/TextureFactory.cpp`:
64-byte resource header, OTEX version 1, RGBA32 type 1, raw flag 1, dimensions
512×512, HByteScale and VPixelScale 16.0, then 1,048,576 RGBA bytes. These are
pack resources only. The deterministic `.o2r` contains 23 resource paths with
identical base and `alt/` entries, so it works with Alt Assets off or on through
the normal external-mod loader. Other packs targeting the same paths follow
normal load order. Production `soh.o2r` and `2ship.o2r` do not contain this HD
art. Load the pack with the matching PR #34 build, which supplies the item
routing and private Magic-icon fallback.

The Wand's already-defined per-rod paths now appear in both hosts' receipt
mapping and `Wand_ModeIcon`. The in-game wheel selects these paths with 32px
logical slots; mode ownership, order, grants and confirmation stay unchanged.
MM's wheel confirmation also reloads its cached HUD pointers, as pause selection
already does; OOT resolves its active icon during each HUD draw.
The standalone Elemental Wand receipt retains its own base icon. The route
probe requires a C99 compiler and executes the actual selectors/wheel builders
with all 256 selector values plus full, sparse and empty owned-mode lists.

Verification checks all 23 source hashes, dimensions, alpha, framing, OTEX
version/raw flag/scales and exact decoded pixel equality; packaging also
verifies every serialized archive entry and base/Alt parity. The optional
asset regression checks absence from the built-in game trees and the Magic
fallback with every base/Alt selection combination. In-game
receipt, inventory, tracker and foreign-host presentation with Alt Assets off
and on remain the decisive runtime checks.
