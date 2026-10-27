# NPC runtime rejection follow-up — 2026-10-20

The user explicitly rejected the reviewed NPC replacements. Earlier archives
and their historical notes do not establish acceptance. Missing-reference fixes
are retained, but shoulder deformation and the crowded Kokiri Forest workload
require separate correction. No candidate in this follow-up is a promoted master.

## Baselines and preservation

Source starts at local `a01321fe031e4c81aed826ec7ea816b261ba49c3`, corresponding
to public checkpoint `44a2063ed20384ea8d68fa79eaba2999ad6e7903` in draft PR #47.
The native read-only input is `oot.o2r`, SHA-256
`ea80d61b223ee38075fe5383b652998f3b08164e903134922f63f205d995717b`.

| Rejected input | SHA-256 |
| --- | --- |
| Young_Fado_SoH_NPC_POC2_Pose_Fit.o2r | 8f4d8994fc8c7f29b460dc0217281e389b24db7684acc7b840c0472149127649 |
| Kokiri_Twins_Render_Native_NPC_POC9_LiteralHash_POC1.o2r | 2723332dce6ee36c5049236d8dc7ddd8613eaf2f31bd97b975623b5c3b495164 |
| Young_Malon_Native_NPC_POC12_LiteralHash_POC1.o2r | e66a17e21327ff3351622068c78fda3eea47030932e62c6f032b60ad920bfa41 |
| Saria_Anime_Native_NPC_POC12_LiteralHash_POC1.o2r | bbcf3551ab4a9eb5184f28d7403795dbab2240d7a62f9d6f1ff459ac63e92562 |

Inputs stay unchanged. Preserve faces, facial artwork, blink/tracking, outfits,
props, corrected fingers, native fallback, animation identity and timing,
placements, interactions and progression. Only requested shoulder surfaces,
lossless display-list batching, and opt-in uniform draw scaling are in scope.
Traditional Saria, adult Malon/Fado and TP Epona are outside the asset edits.

## Reported configuration and direct evidence

Five supplied screenshots were inspected directly. They show shoulder bands on
the twins, an open-looking young Fado shoulder in another pose, and replacement
characters compared with native variants. The user reports poor performance
near Link's house with child Malon, TP young Epona, four twins, young Fado and
anime Saria. Exact frame timings, GPU/backend, runtime build and complete active
pack order are not established by those images.

Archive reconstruction uses the exact native input above. The legacy Fado cap
has inward-facing triangles at the chest/arm transition despite coincident seam
vertices. The twins have three specifically built shoulder poses and share an
idle-built cap with other clips; numerical fold and normal checks are required
across all six static entries, rather than treating one preview frame as proof.

## Lossless batching milestone

`rebatch_native_npc_archives.py` recompiles only the narrow single-material
F3DEX2 leaf contract emitted by the existing model exporter. It carries a
32-vertex batch across adjacent matrix-slot groups. The input/output proofs
compare ordered triangle corners consisting of the exact matrix address and all
16 original vertex bytes, plus identical material preambles and host restores.
Shared arrays, unsupported commands and changes without a measured benefit are
refused. All other resource bytes and names stay unchanged.

Baseline per-pass recursive workload, excluding actor setup and Epona:

| Actor selection | Triangles | Vertex loads | Matrix commands |
| --- | ---: | ---: | ---: |
| Child Malon | 47,333 | 50,471 | 3,186 |
| One twins instance | 34,619 | 42,230 | 4,237 |
| Young Fado | 23,606 | 19,675 | 126 |
| Anime Saria | 43,940 | 45,649 | 4,923 |
| Seven humanoids, with four twins | 253,355 | 284,715 | 25,183 |

For these same seven humanoids the native archive submits 3,041 triangles and
5,088 loaded vertices. These are interpreted/submitted asset counts, not FPS
measurements or counts of visible pixels. Models remain high-poly after batching.

On the rejected-parent geometry, lossless rebatching reduces one twins instance
to 1,837 matrix and 2,758 vertex commands (from 4,237 and 5,200). Child Malon
reduces matrix commands to 1,509; anime Saria to 2,362. Geometry is identical.
The shoulder candidates must be audited again after their independent changes.

## Uniform scale request

The user requested a modest increase for child Malon and the same increase for
anime Saria. The candidate target is 1.10 times the current visual size. Scale
belongs to each selected archive and is applied uniformly around the actor
origin during drawing; native animation channels and rig proportions remain
unchanged. Missing/invalid resources and Alternate Assets off retain scale 1.0.
OoT and both placed-Saria providers must agree on the resource contract. This
requires matching executable support; old builds may ignore the scale resource.

## Verification and disposition

Static equivalence and focused source compilation are distinct from configured
gameplay. Final candidate identities, measured shoulder checks and exact test
results are recorded in the transport bundle's manifest and evidence directory.
Required runtime review remains: Fado and both twins' shoulder silhouettes from
front/back through all six entries; blink, tracking, fingers and props; Malon
and anime Saria side-by-side with Link at 1.10 scale; Saria seated/ocarina poses
in OoT and MM; Alt on/off and re-entry. Compare the same camera in the crowded
nook before/after batching for actual performance evidence.

Recovery: restore the rejected inputs above for a control comparison, preserve
them outside the active mods directory, and apply the new source delta only
above the recorded source checkpoint. Do not stack old/new replacement packs.
Runtime remains unproven and user acceptance pending until that review occurs.
