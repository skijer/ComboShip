# Black Market shelf materials

Parent: `6d05a21251a29a829747357a24fcfbcf62ecc1ba`, preserved on
`poc/black-market-shelves-20261018` with an independently restorable checkpoint.
cor approved the shelf shape and requested a separate scrolling magical
material with a cosmetic entry, plus Granny shelves matching her room.
Candidate: `poc/black-market-shelf-materials-20261018`.

The approved rack geometry, collision, no-camera metadata, positions, keeper,
item anchors, camera and room controls remain unchanged. This change only
updates shelf materials, their UVs/colors and cosmetic controls. The prior
plain wood version remains recoverable from the parent commit/checkpoint.

The magical finish uses its own source-authored periodic 64x64 I8 flow texture,
independent of the room's stone texture. Native `gSPScrollTexture(0,-1,1)`
provides quarter-texel scrolling with the renderer's tick/interpolation path.
Both cosmetic editors expose **World > Black Market Shelves**, including
normal reset, rainbow and lock controls. Rendering reads each host's own
canonical color/Changed CVars every frame. The existing opt-in OoT-to-MM
cosmetic sync includes the new row. Its default is RGB `(126,178,226)`.
The second combiner cycle multiplies COMBINED by PRIMITIVE; tile 1 is unused.

Regular Granny shelves use only the installed Djipi room's original textures:

| Finish | Exact external texture | Region in the original image |
| --- | --- | --- |
| Aged timber | `alt/scenes/shared/mahouya_scene/sh_maho_ki_01_2CA00` | Whole image, logical 32x64 |
| Burgundy lining | `alt/scenes/shared/mahouya_scene/sh_maho_obj_01_43580` | X98..188, Y66..84 of 256x256 |
| Carved colored trim | Same original counter atlas | X0..96, Y0..88 of 256x256 |

The back panels and board tops use the burgundy region; board front edges use
the carved band; remaining framing uses original timber. All use neutral
vertex shading and white primitive color. Granny receives no magical tint or
scroll. Atlas regions are UV coordinates; no texture is cropped, exported or
bundled. The existing room adapter retains both original texture owners. Its
shelf paths pin the same maps owner via `__OTR__@oot:alt/...` or `@mm`, including
when the other game owns the installed pack. Appearance selection follows
the successfully attached room, so toggling the checkbox applies on next entry
as before. Missing packs still retain the magical room.

Each finish has its own immutable native vertex array. Neither per-frame color
changes nor scroll rewrite a vertex buffer that an asynchronous draw might
still reference. Collision ownership and lifetime are unchanged. The accepted
POC14 archive and both external donor archives remain byte-for-byte unchanged.
No Granny donor assets are added to source, a build, a new O2R or the checkpoint.

Verification covers the actual native GBI and Dyna/camera fixtures in both
engines under ASan/UBSan: scrolling magical material, current/reset tint,
untinted stationary Granny textures, missing texture rejection and unchanged
geometry. The initial regression failed on the prior material's absent scroll.
Complete native controllers and cosmetic editor translation units, plus the
sync translation unit, are compiled. The extra OoT UI compile required CMake's
actual ImGui `v1.91.9b-docking` pin and its project patch; the earlier verification
environment contained the older non-docking 1.90.9 headers. Recovery identities
and compile commands are recorded with the evidence.

Browsing, transaction/recovery, keeper, seals, Granny room/dependency/metadata,
4096 stock inventories, shelf clearance and native 4:3 framing remain checked.
Read-only review found no concrete issue with material state, routing, lifetime,
CVar identity or sync. These are source/compile/fixture checks, not configured
executable proof. Final evidence is under `shelf-materials-*` in the source
checkpoint. Offline stills and the motion clip use the same source mesh, flow,
UVs and external original textures, with source/input/output hashes.

Remaining runtime checks: both appearances in OoT-only, MM-only and combined
mode; live shelf tint/reset/rainbow; original pack and Alt off/on; browse,
buy/cancel/leave, pause and re-entry. Offline lighting and room effects remain
approximate. No full game build, remote push or master promotion is claimed.
