# Black Market vessel palette / glass candidate

Optional, unaccepted visual comparison for the existing Black Market. Four cauldrons have separate muted palette hex values; three capped right-side canisters have separate vertex hues. Ten connected bottles on the right shelf use distinct padded UV windows across smooth sea-green, cobalt, amber and plum glass. Their original UV axes were swapped during the bounded remap so vertical glass streaks stay upright.

`Black-Market-Shop-Appearance-v1-Candidate.o2r` contains exactly seven resources: vertex owners2/14/24 and textures `baseline_timber`, `baseline_counter_atlas`, `baseline_fittings_atlas`, `baseline_ceramic`, all beneath `custom/skiejer_market/`. No triangles, room roots, material commands, collision, placements, gameplay, title, arcane shelf texture or ordinary floor texture is included. Existing geometry at commit4fff75754ef1578e586312fbf8c68e3adb99b4df remains required.

This single recommended shop appearance archive includes the exact task6 timber candidate plus all six task14 resources, with their per-resource provenance in `overlay.json`. The counter/fittings already combine that timber underlay with the new prop pixels. No second timber or props archive is needed for this comparison. Place this single archive in the selected host's existing mod directory (`mods/oot` or `mods/2ship`); use a comparison setup without the earlier separate timber/props archives. Restart after mounting/removing to clear cached resources. Remove the archive and restart to revert. Actual loading and priority against other user overrides remain untested. No installation was performed.

The separate source patch prepares the same prop changes with the baseline wood, and does not require either optional archive. The coordinator owns source integration. Applying optional older atlas overrides over the new source output can hide the prop changes, so control override priority during comparison.

Rebuild this exact optional archive using Python standard library only:

```
python3 -S build_overlay.py --output Black-Market-Shop-Appearance-v1-Candidate.o2r
```

The builder validates seven fixed paths, bounded ordered compressed parts, base64/packed/resource/archive hashes and exact decompression. ZIP_STORED members have fixed1980 timestamps and Unix0644 regular-file metadata. `overlay.json` records hashes. No original91texture parts are needed or repeated.

The four `*-comparison.png` files use actual source geometry and prepared UVs/vertex colors, with the same task6 timber on both sides. They are offline CPU previews with approximate lighting/filtering. Dynamic shelves/backdrop (including restored arcane art), keeper, stock and effects are omitted. They are not gameplay or GPU captures.

Glass uses the existing fully opaque native path and stylized baked surface sheen; it is not physically transparent glass. Existing canister ornamental bands and all unrelated atlas regions remain protected. Cauldron liquids retain their original color. Color hex values are material palette/tint parameters; lit pixels vary with shading. GPU sampling, install priority, both hosts and user preference remain unverified. This candidate is not a promoted default.

The original generated four-cell glass atlas, exact prompt,320x122 upload strip, source palette recipe, validation receipt and source-mesh previews are included for review. Built-in imagegen generated the glass material; quadrant cropping/LANCZOS upload conversion was followed by lossless packing.
