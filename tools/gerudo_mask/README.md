# Mask dialogue and veiled Gerudo candidate

Base: `bridge/audit-merge-20261008` at `c0181856968dd6f3869be0d1aa240743091fd6a8`.
Candidate: `poc/mask-dialogue-veiled-gerudo-20261009`.

Skull Mask, Spooky Mask, and Mask of Truth regain their OOT pickup descriptions, with permanent-grant wording. The Gerudo tutorial teaches transformation/reversion, drawing and chaining twin swords, forward+B, holding A to sprint, A in the air, and spending rage with R+B. Shared English/German/French text works in native and foreign randomizer receipt routes, including MM before OOT gameplay has started. Native OOT icons are installed after formatting so Spooky's `0x26` icon byte cannot be mistaken for the `&` newline marker.

The Gerudo GI contains the existing form's head, Henriko veil, and hair, centered into a 58-unit get-item object. The OOT draw-table recipe and MM's native OOT-mask dispatcher both use it. MM queries the registered OOT asset owner and submits an `@oot:` display-list route; old or unavailable archives retain the original mask fallback. Mask shimmer stays on its existing draw ID.

Henriko's matching Gerudo images were imported directly from the supplied [CHARACTERS.zip](https://drive.google.com/file/d/1rrE887pmtO-ClLRc_XoCubEkf8PUasz5/view). Their filenames and SHA-256 hashes are in `provenance.json`. The supplied `gerudo3dsplayer_a51e0.zip` matches the existing form's texture source. No generated facial details are used.

| Texture | Candidate resolution | Scope |
| --- | --- | --- |
| Body/armor/clothing atlas | 512 × 512 | Entire adult and child forms, GI veil |
| Face/hair atlas | 512 × 256 | Adult and child, GI |
| Open/half/closed eyes | 256 × 256 each | Adult and child; fixed open GI eyes |

The clothing and veil now use the same unaltered Henriko atlas as the rest of the body, including all alternate and first-person arm materials. Their materials use white primitive color rather than multiplying the source purple by Link's tunic color. Gerudo cloth therefore retains the atlas's fixed purple color; the earlier candidate's tintable grey cloth is no longer selected.

The ankle cuffs have a narrower, shorter flare. Five outer tips per leg are tucked 65% toward the existing inner ring in both ages, with their affected lighting normals recomputed. Duplicate vertices at texture seams stay coincident. The inner cuff boundary, shin, weighted knee, feet, vertex counts, topology, UVs, skeletons and animations are preserved.

Eye materials now bind segment 08, and canonical eye names allow the existing engine blink animation to select all three states. Other expression slots use the Gerudo open eye instead of leaking Link's face. OOT prefers the active Gerudo's eyes over an unrelated player pak and binds a stable OTR path so HD scale/format metadata survives. Other forms retain their previous precedence.

Scimitars use the repository's existing silver environment map with sphere-generated coordinates. Their metallic highlight varies with orientation. This is the engine's environment-mapped finish, not a reflection of nearby world geometry. Both age resources are updated; combat and the existing rig, UVs and animations are unchanged.

## Verification

Run from the repository with normal development headers installed:

```sh
python3 scripts/diagnostics/run_mm_item_receipt_tests.py
python3 scripts/diagnostics/run_gerudo_mask_tests.py
python3 scripts/diagnostics/run_mask_shimmer_tests.py
python3 scripts/diagnostics/run_receipt_syntax_tests.py mm/2s2h/Rando/ItemReceiptText.cpp mm/2s2h/Rando/DrawItem.cpp soh/soh/Enhancements/randomizer/randomizer.cpp soh/soh/Enhancements/randomizer/Messages/ItemMessages.cpp soh/src/code/z_draw.c soh/src/code/z_player_lib.c soh/mods/transformation_masks/custom_forms.cpp
python3 scripts/check-asset-collisions.py
```

The asset checks validate display-list dependencies, native vertex-cache limits, direct RGBA resources, full-color cloth bindings, the isolated cuff change, preserved form rig/UVs, HD texture metadata, three distinct blink frames, and the reflection load format. Compiled fixtures exercise the actual MM dispatcher/routing implementation and OOT face resolver, including cold/warm asset availability and fallback behavior.

Reproduce assets from the extracted Henriko directory:

```sh
python3 tools/gerudo_mask/build_assets.py /path/to/CHARACTERS
python3 tools/gerudo_mask/preview.py /path/to/previews
```

Preview images use the candidate's actual triangles and textures with offline lighting. The blink preview shows the form's three eye states on the GI mesh; the pickup GI itself intentionally keeps open eyes.

Full-body previews assemble all 21 actual limbs using the packed `gPlayerAnim_mhr_db_idle01_loop` player frame, including flex-matrix changes in the weighted torso. This archived dual-blade idle has the Link rig's base rotations; the older `gPlayerAnim_gerudo_stand` resources do not and are unsuitable as a complete standalone pose. The preview uses native candidate materials and neutral head/upper-body aiming. It is an offline pose, not evidence of the runtime's selected idle or mod stack.

```sh
python3 tools/gerudo_mask/preview.py /path/to/previews --full-body-only
python3 tools/gerudo_mask/preview.py /path/to/previews --ankles-only
```

The ankle comparison renders the previous and revised cuff vertices with the same current materials and pose. All ordinary previews now use the complete Henriko clothing and veil bindings from the game resources; no preview-only cloth substitution is needed.

## Runtime acceptance still required

No game executable or native runtime capture was available here. This candidate has source compilation checks and integration fixtures, not a full linked game build or runtime acceptance.

Check the three ordinary mask descriptions and Gerudo tutorial in OOT and MM, including MM-first pickups, native and foreign placements, repeated receipts and game transitions. With Gerudo transformation enabled, check the GI spin/veil/shimmer, natural idle blinking, both ages, the installed appearance-mod stack, and moving sword highlights. Confirm that normal Link and the other forms keep their previous appearance, and that Gerudo's sword, rage, sprint and transformation controls still work.

The optional visual archive in the handoff contains the complete Gerudo form namespace and the new GI. It is a test overlay for the candidate build; dialogue and dispatcher changes require the code patch. Mount it after the existing form archive in each game's mod stack so the candidate resources win. MM loads its own indexed form assets, so installing the overlay for OOT alone does not update the MM transformation. Keep the accepted archives as the rollback source. This overlay does not replace a game executable or establish runtime acceptance.
