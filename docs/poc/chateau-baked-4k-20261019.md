# Chateau Romani 4K stock-asset checkpoint — 2026-10-19

This checkpoint supersedes the dinner audit's statement that the upgraded Chateau Romani GI remains an external addon awaiting a stock-asset bake. It integrates the accepted 4K/glass candidate into both games' normal custom-asset inputs. Regenerating `soh.o2r` and `2ship.o2r` includes the upgraded model without installing a separate Chateau addon.

The existing Alt Assets selection still chooses the MM3D Chateau GI. The native fallback, geometry, UVs, shimmer colors, receipt text, and tutorial logic retain their prior behavior. Only the atlas, reflection texture, and three glass material display lists change in each game's 19-resource model.

## Source and build contract

- Preserved candidate ZIP SHA256: `18547dd53de7796cf53969a03e97f94eb6a1a8a7dd9f744ebcb62ccd59948556`.
- Native atlas: 4096 × 4096 RGBA, texture version 1, native coordinate scaling 64 × 64; SHA256 `8dd894e2100f91ac20067303f6c642a32358097b9e3cf3654a85568b31662a13`.
- Reflection: 512 × 512 RGBA, version 1, scaling 16 × 16; SHA256 `f800378df5aead3f49e657069c4d83785a336b1cd0979198851ae49cccecb5b6`.
- The large atlas is stored as hash-checked zlib data in bounded base64 source parts. Original 4K PNG bytes are also preserved in source parts for reproducible authoring. Generated full-size atlas files and PNG are ignored.
- `PrepareChateauRomaniAssets` validates and expands the exact native atlas into both custom-asset trees before `CheckAssetCollisions`; both existing stock archive generation targets depend on that gate. Native packers consume the resulting extensionless resource unchanged.
- Corrupt source data fails before replacing good outputs. Matching outputs are left untouched. The authoring builder regenerates the compressed atlas source when writing custom assets.

## Verification

Passed: clean temporary asset-tree preparation for both hosts; exact decoded resource hashes and texture header; repeat-run idempotence; corrupt-input rejection preserving existing outputs; original 4K PNG reconstruction; actual CMake preparation target; deterministic standalone archive generation and model graph verification; HD preservation checks; Chateau shimmer and tutorial probes; both native GBI draw boundaries; mask/shimmer sanitizer gate; localized Chateau receipt probes (normal and address/undefined sanitizers, leak detection disabled for this sandbox); native Domain thaw probes for both hosts; custom-asset collision gate with no new collisions.

The complete game executable and regenerated full stock archives have not been built here. Visual inspection of the baked assets in a running game remains pending; source and focused native checks do not establish runtime acceptance. The protected `develop` baseline is not promoted by this checkpoint.

## Domain checkbox

OoT menu: **Enhancements → Graphics → Mods → Always Thawed Zora's Domain**. Default: off. Source: `soh/soh/SohGui/SohMenuEnhancements.cpp`.
