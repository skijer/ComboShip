# Native OoT place-title glyph candidate / POC2

This optional pack uses actual English OoT place-title-card pixels from native source artwork, rather than a generic font or the previous generated italic candidate. Forty-four isolated native glyph crops retain their original proportions and 24-pixel baseline. The exact source face coverage is replicated four times across and down into 96x96 RGBA textures. Every 4x4 block downsamples exactly to its isolated native glyph. Thirty-seven explicitly audited detached pixels belonging to adjacent donor letters are excluded; no authentic glyph pixel is repainted. This is a faithful pixel upscale, not newly invented high-detail lettering. The title renderer supplies its existing one-pixel black keyline; original whole-card kerning and baked halo are not claimed to be identical.

The rebuilt `TwoShip-Native-OoT-Titles-POC2.o2r` contains 46 resources: 44 glyphs, a native-profile marker, and the previously accepted Black Market wordmark. The Black Market resource is byte-identical, SHA256 `a8d76bfa5e44fdd0d349a0f3ec007c46deac130e7468e6edbdb6f5419f446cbe`.

Install this optional `.o2r` through the normal mod-pack workflow with the native-profile code revision. Enable **OoT-style location title cards** in the MM graphics menu; it remains off by default and is available in solo 2Ship and ComboShip. Whole-title image overrides retain first priority. Native text, entrance eligibility and fade timing remain authoritative. Remove this pack to return to native MM glyphs or an independently installed older optional 16px title-font pack. Dialogue, boss cards and day announcements are unaffected.

Actual native MM message data and the pinned scene table establish 67 unique eligible English title IDs. This pack covers 61. Six retain the complete native MM glyph presentation because the available native OoT place-card source contains no E/N/O/& glyphs:

- East Clock Town
- North Clock Town
- Honey & Darling's Shop
- Post Office
- Astral Observatory
- Oceanside Spider House

Any other unsupported character or missing glyph resource also falls back for the entire card. The six gaps were not filled with dialogue letters or invented shapes. The native-profile marker prevents mixing with the previous generated font if both packs are mounted. No ROM, whole game archive, generic TTF or Djipi asset is included.

The separate visual delivery includes `native-glyph-comparison.png`, which compares actual OoT source artwork with actual packed glyph composition. Its `native-vs-packed-preview.png` shows representative MM names and the accepted Black Market card. These are offline pack previews, not GPU/gameplay evidence.

Rebuild and verify with Python 3 standard library only:

```sh
python build_pack.py --output TwoShip-Native-OoT-Titles-POC2.o2r
```

The compressed payload is one 27,919-byte binary part / 37,228-byte base64 source file. The builder validates bounded decompression, all hashes, exact resource inventory, native texture headers, HD stride/height metadata, alpha framing, source-face downsample parity and the accepted Black Market resource identity.

This candidate passed source fixtures and pack verification. Configured in-game rendering and user acceptance remain pending.
