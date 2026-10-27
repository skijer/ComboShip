# Final forest scrolling, native solo copies and avatar poster

This bounded candidate continues local checkpoint
`0c873316e1b4c2de0d393d68731a5795581272a6` / published checkpoint
`0fc74846629f45951025fa99ae71614f3aabfc1a`, common tree
`cae914230498822fc1b81545f344c8cf03eecf04`.
It brings the final forest to both private solo scenes, binds the six authored
scroll tracks in OoT and replaces the wanted poster's portrait at cor's explicit
request. It does not promote an accepted master or establish configured runtime.

## Exact assets

| Role | File | SHA-256 |
| --- | --- | --- |
| Unchanged user final input | `1 - lost woods 2ship, wood bench added. final for merge.prelude.o2r` | `89bdb0ce3ce2e22ca7e4aed438251a66a167ee8b1d68a7b1b019a05c35ac34f9` |
| Immutable native address repair input | `zzzz_MM3D_Lost_Woods_FINAL_native_segment_flags_POC18.prelude.o2r` | `4bef56b0b4e3fbe89308c85e29eefbfce19b4f91d21a7ed79fc5c3c519c4a7ef` |
| Immutable earlier solo header/title donor | `zzzz_Skiejer_Native_Solo_Forest_POC2.o2r` | `3d45015fa43997e2f3a96440e018a6668a2b37d654bc3e32843c0f9100093cfb` |
| Complete combined forest candidate | `zzzz_MM3D_Lost_Woods_FINAL_avatar_POC19.prelude.o2r` | `9097583c896a11c204e4950bd9a80aa8e69fb2a42307f55f50e75d4195f30e42` |
| Shared native solo addon candidate | `zzzz_Skiejer_Native_Solo_Forest_FINAL_avatar_POC4.o2r` | `65901fcc27453a915be9414c7203e8295fce36ce69b7658653bbcdf978af8d0e` |

Input file identities and recipes are also retained in
`tools/magic_mirror/scene/manifest.json`. The full combined candidate contains
123 resources and changes only one poster texture against the native address
repair. All 122 other resource payloads remain byte-identical, including the
fixed scrolling calls, scene/actor data and collision.

The solo addon has 120 dependency-closed resources. It derives its art,
materials, UVs, vertices, collision and scroll parameters from FINAL. It adapts
XML paths and 90 binary resource hash operands into the existing closed private
namespace. Every binary display-list byte outside hash operands remains
unchanged. The requested wanted-poster replacement is the sole physical art
exception. Native camera settings are normal camera 1; collision topology,
surfaces, vertices, water and stump identity remain intact.

POC2 supplies only stable native spawns, local exits, music/special-file framing
and the title texture. The final scene supplies lights and both room meshes.
Existing intro/cutscene actors are excluded from these exploration-only copies;
MM ambient actors retain the established solo policy, and OoT uses its local
grass/rock controller IDs and direct binary-angle encodings. The authored shop
door is transition index 1: MM actor 5 with packed 106-degree yaw, OoT actor 9
with direct binang 19296. Position `(-680, 0, 72)` and native plain-door params 0
are preserved. Native doorway creation adds the actual transition index.

## Native bindings and door

OoT invokes `ComboSoloForestOot_DrawMaterials` after its ordinary scene draw
configuration and before current/previous room rendering. The function is
restricted to an eligible private OoT solo forest visit and the new addon marker.
It supplies segments 8–13 to both opaque and translucent lists, using native
interpolated texture-scroll generators and current-frame graph allocations.
No cached resource holds a pointer to graph allocation memory.

The generated `SoloForestScroll.h` derives all six tracks from actual FINAL
TexAnim records, including MM's negative serialized-Y convention. Logical tile
sizes and all rates match native MM behavior, including frame wrap. OoT's four
normal scene setups are supported by the existing child/adult visit policy.
MM's private scene now selects its native animated-material draw configuration
and loads the unchanged TexAnim resource through command 26. Neither engine
borrows arbitrary segment contents from another scene.

OoT's native Open action recognizes the uniquely placed same-room plain door,
enters the existing local market and records a placement-relative return point.
It checks the live actor's native transition index, original position/yaw,
player ownership/action, scene/session/pack, health and transition state. An
unavailable or ambiguous marker cancels safely to native idle and clears native
animation movement. Any authored marker suppresses the retired shell/coordinate
trigger. Ordinary doors and unmarked legacy forests keep their native routes.

Shop return follows the loaded door 226 units outward, approximately
`(-462.75, 0, 9.72)`, with existing grounding and a native camera reset. Slot/seed
travel identity and synchronous save-worker ordering remain intact. The Mirror
still resolves the loaded 12-triangle stump cap for pickup and recall; no item,
logic, animation or Farore ownership behavior changes in this pass.

## Wanted poster

The supplied GitHub avatar exactly matches `https://github.com/skijer` at
`https://avatars.githubusercontent.com/u/124228042?s=460&v=4`.
Its decoded RGBA pixel SHA-256 is
`19a43f0f7580906ba94848954fbaade5db0a28ff40f28a3a49ebae274efcfe2c`.
No original artist or explicit reuse license was established. Cor then explicitly
selected that avatar for the wanted poster. This record makes no public-domain
or copyright-clearance claim.

The imagegen portrait swap removes the gold-haired character and retains the
wanted-poster wording, parchment and ornamental layout visually. The texture was
re-rendered, so surrounding pixels and avatar proportions are not an exact paste.
The source avatar, chosen PNG, prompt and provenance are retained under
`tools/black_market/poster/`. The chosen poster PNG SHA-256 is
`e3a3592412bb585fc8d68f87d3bfd6e9540366e96b739cabef1799253228d993`.
Native encoding performs PNG-to-RGBA conversion only, without resampling. The
new 1536×1024 texture scales 96/32 preserve the original logical 16×32 mapping,
so material dimensions and UVs do not change. Both complete scene candidates
include this same replacement.

## Reproduction and configuration

```sh
python3 -B tools/lost_woods_entrance/final_scene_segments.py ORIGINAL_FINAL --output SEGMENT_FIXED
python3 -B tools/black_market/poster_texture.py SEGMENT_FIXED COMBINED_CANDIDATE
python3 -B tools/black_market/final_forest.py SEGMENT_FIXED PREVIOUS_SOLO SOLO_CANDIDATE --contract combo/entrances/SoloForestResources.h --scroll-contract combo/entrances/SoloForestScroll.h
```

| Mode | Active forest pack | Other requirements |
| --- | --- | --- |
| Combined | Complete FINAL-avatar POC19 in MM mods | Matching source; AltAssets on; existing market/interior pack |
| OoT solo | FINAL-avatar solo POC4 in OoT mods | Matching source and existing native market pack |
| MM solo | FINAL-avatar solo POC4 in MM mods | Matching source and existing native market pack |

Select one version of each whole forest. POC4 supersedes earlier private solo
addons and uses base private paths, with ordinary native Alt fallback. It does
not replace the combined game's scene headers or native original Lost Woods.
The required-resource contract includes the new bindings marker and all art.
Missing complete dependencies disable the private approach through its existing
native recovery policy.

Focused source/native evidence includes real scene registration and actual
native scroll generators/macros, sanitized door/return/cleanup fixtures,
complete native C++/C objects, Mirror integration, both host forest/browsing and
market regressions, and independent archive/reference/preservation checks.
Two complete solo rebuilds produce the same archive. Exact commands/results and
any baseline fixture limitations are retained in the recovery artifact.

Configured game/Prelude first-frame load, rendering, selected archive priority,
child/adult/form traversal and a full linked executable remain unverified. The
reported original Prelude freeze is not declared solved. The decisive runtime
check is native approach -> both forest rooms -> shop -> forest return ->
Mirror pickup/recall in each mode, checking the building/bench overlays, purple
portal and stump shimmer, then the existing local/cross-game exits. Compare
base/Alt paths and a combined-game control. Publication is a recovery checkpoint.
