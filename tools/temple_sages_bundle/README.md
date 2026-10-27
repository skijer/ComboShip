# Built-in Sages Chamber base

The normal package provides the complete chamber geometry, collision, native
OoT texture bindings, age/night setup headers, destination markers and travel
support. It requires no external chamber archive. Both asset modes use the
same geometry and native artwork.

The separate visual archive remains optional. It replaces the same material,
texture and display-list identities to supply the custom textures, scrolling
surfaces and visual effects. Installing it does not supply missing gameplay
geometry or unlock a setting that the built-in base cannot use.

## Native artwork

Ordinary material bindings reference exact native texture-resource aliases.
Sampler format, dimensions, masks and shifts follow each native resource while
preserving the accepted geometry and UV data. All authored texture-scroll
commands are removed from the default materials. Portals retain visible static
native-texture surfaces and the existing caller-owned destination tint.

The two medallion atlases rearrange the original native half-disc texels into
the unchanged UV layout. No artwork is painted, recolored or interpolated.
RGBA16 channels are expanded with the engine's exact five-to-eight-bit rule.
The source manifest records every original region and its destination, plus
independently checked pixel hashes. The optional archive replaces these same
atlas paths with its existing visual resources.

Every walkable collision polygon uses native stone footsteps and maximum
six-bit collision echo. Only polygon surface-type references and the sound /
echo fields change; geometry, normals, other surface flags, camera and water
payloads are preserved. The optional visual archive carries the same acoustic
correction so it does not restore the prior floor audio.

## Proven bounded source storage

The source uses the established morning upload format: unique exact resource
bytes are pooled, compressed once with lossless zlib, and split into ordered
base64 text parts. Each part holds at most 96 KiB compressed / 128 KiB encoded.
Per-part, compressed-pool, decoded-pool and per-resource SHA-256 checks protect
restoration. Decoding has fixed size and path limits and validates everything
before changing any generated output.

scripts/assets/prepare_temple_sages.py restores both normal port trees using
only the standard library. CMake runs preparation before asset consumers and
collision scans. No source O2R/ZIP/image archive is uploaded for normal builds.
The preparer replaces matching owned identities and removes obsolete files
only inside its private chamber namespaces or exact recorded retired paths;
unrelated scene files are preserved.

Authoring input identities and native pixel bindings are retained in the
source manifest for reproducibility. To intentionally rebake, run the builder
with the preserved --addon, --base and --maps inputs, then run:

    python3 tests/temple_sages/test_packaged_chamber.py
    python3 tests/temple_sages/test_native_chamber_layers.py

Verified source checks are not gameplay acceptance. Native/Alt appearance,
optional visual layering, footsteps/reverb, collision feel, cameras and travel
still require the matching rebuilt executable and configured runtime checks.
