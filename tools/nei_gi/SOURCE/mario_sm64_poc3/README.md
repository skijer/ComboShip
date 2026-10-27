# Mario SM64 POC3 retained source

`serialized_source.o2r` is the unchanged saved
`ComboShip_Mario_SM64_POC3.o2r` archive. Its SHA-256 is
`9980d41450bcaa741d2f86cee3658606eec76f558b4d73c5d26d7eccc6f7e52f`.
`source.json` records the four resource hashes. The archive contains each path
once in the base namespace and once in `alt/`, with identical bytes.

The late October 9 / early October 10 procedural authoring source and original
GLB were unavailable during recovery. The earlier rounded-mask recipe is not a
source for this candidate. This archive is an explicit serialized source
snapshot, preserving the saved appearance without inventing a replacement.

Rebuild and install the saved GI plus the bundled native icon:

```sh
python3 -B tools/nei_gi/SOURCE/requested_revamp.py mario_mask --install
python3 -B tools/nei_gi/generate_frame_bounds.py
python3 -B tools/nei_gi/SOURCE/mario_sm64_poc3.py --verify
python3 -B tools/nei_icons/build.py --verify
```

The restoration copies all three GI resources byte for byte. It reconstructs
the inspection GLB from the display-list vertex cache and material-color runs:
13,112 triangles, 14,436 vertex records, and 464 vertex loads. Positions and
winding are exact. Normals retain the signed runtime bytes as values divided by
127; UVs retain the serialized coordinates divided by 1024; each primitive
color retains its serialized RGBA values divided by 255. The native fixed-point
matrix is preserved. Original authoring part names and hierarchy are not
recovered. PBR roughness and preview lighting remain offline approximations.

The optional 512px icon comes directly from the retained OTEX1 pixel payload.
The PNG reproduces every RGBA pixel; optional packaging reconstructs the exact
saved OTEX1 bytes, including raw flags and 16× scales. The `MarioMask` branch in
the icon renderer restores these pixels instead of assigning an unknown camera
to a new render.

The bundled SoH and MM icons use the existing 32×32 RGBA PNG source slots. They
are deterministic Pillow LANCZOS downsampling of the preserved 512×512 image,
and are separately verified against that derivation. These native icons are
not claimed to be byte-identical to the 512px OTEX resource. No extensionless
raw icon is installed beside either native PNG source.

Static verification checks retained hashes, base/Alt parity, exact installed GI
bytes, independent checkpoint geometry/winding/cache/matrix validation,
deterministic GLB attributes/materials, exact optional OTEX bytes, and matching
native host icons. The recovered combined build has not been tested in game.
This remains a candidate for draft PR41; it is not promotion or acceptance.
