# Black Market asset reconstruction

The interrupted publication left 11 original PNGs and three packed texture files unavailable. The material PNGs were regenerated with the recorded prompts and the prior offline room preview as a color/style reference. They are replacement artwork with new hashes, not byte-identical recoveries.

The checked-in source PNGs use the existing 512 by 512 runtime resolution and lossless RGB PNG compression. The original generated outputs and their hashes are retained separately; source and generation identities are recorded in the two interior provenance files. The immutable market source archive remains SHA-256 `3ac1432d0e8f75e51f08892739a13e16762bb4d820a2bbc7a35469af1db872e2`.

Authoring now rebuilds both the architecture and prop bundles. Each compressed slice is at most 96 KiB, yielding a base64 source file at most 128 KiB. Standard-library build preparation checks part names/order, sizes, per-part and aggregate hashes, decoded bounds and pixel hashes. The existing material roles, UV atlas rectangles, geometry transformations, opaque routing, title and exit-door code are preserved.

From the repository root, rebake with Pillow installed:

```sh
python3 -c "import sys; sys.path.insert(0, 'tools/black_market'); import market_interior; market_interior.rebake()"
```

Validation: all 28 native-market asset tests pass, including the real CMake preparation target, source-archive conversion, both ports' 141-resource preparation, stdlib-only preparation, retained scene/material contracts, texture round trips and damaged/oversized/path-invalid part rejection. Repeating the authoring bake reproduces all 91 parts and both manifests exactly. Protected market functions and constants compare unchanged against the recovered source. CMake tracks both sets of numbered part files and no longer requires the superseded single architecture file.

Configured native/Alt gameplay, GPU appearance and full executable linking remain untested. Publication does not promote a runtime master. See the adjacent JSON record for exact upload identities and progress; the earlier publication-recovery record retains the historical pre-stall manifest.
