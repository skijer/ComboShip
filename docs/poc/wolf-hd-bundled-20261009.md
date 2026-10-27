# Bundled HD Wolf Link candidate

Both games now ship the standard Wolf Link and the supplied Twilight Princess model in their normal generated port archives. **Skijer's NEI → Masks → Use HD Wolf Link** is enabled by default in both menus. It selects the TP mesh with softer blue irises and a six-link cuff chain. Clearing it selects the original standard model. A change applies on the next transformation, after the live character's skin buffers and animation references have been cleaned up.

The checkbox uses the shared `gMods.WolfLink.UseHDModel` CVar. It is independent of the general alternate-assets setting. Each game first uses its own resource manager. HD selects `objects/forms/wolf_link/gWolfLinkHDData`; standard selects the existing `objects/forms/wolf_link/gWolfLinkData`. Each choice retains its own Alt override priority. Missing HD data on an older port archive falls back to the same owner's standard model. A present malformed or wrong-type HD resource is rejected rather than silently replaced. The legacy copy ABI is retained; the new model-aware copy reports the exact selected resource key. Reloads reuse the existing character registration instead of consuming another registry slot.

Both resources are checked-in serialized LUS Blobs under each game's `assets/custom` tree. The existing `GenerateSohOtr` and `Generate2ShipOtr` targets therefore include them automatically. They also already include the Shadow Crystal model, name and icon. No separate Wolf appearance archive or loose `.bin` configuration is required. The explicit `NEI_WOLF_LINK_ASSET` deployment option remains available for older archives and development work.

| Item | Standard | HD candidate |
|---|---:|---:|
| Vertices | 8,112 | 8,640 (7,776 wolf + 864 chain) |
| Triangles | 2,704 | 2,880 |
| Texture atlas | 256 × 256 | 1,024 × 512 |
| Bones | 40 | 46 (40 original + 6 chain) |
| Animation clips | 144 | 144 |
| Original animation frames | 5,047 | 5,047, original 40 tracks byte-preserved |

The eye revision follows the user's supplied natural-blue reference. The previous broad neon ring is reduced to a muted, textured iris crescent with a larger dark pupil and the original dark outer iris. Only 261 texels in each eye use inverse emission alpha. The native material from blue-eyes commit `1cc3c72e16b1c09a77f5657fb88b864e938b9171` keeps those texels readable in shadow while retaining normal lighting and opacity elsewhere.

The uploaded chain ZIP is byte-identical to the previously recovered TP chain source. Six copies of its authored link mesh attach at the cuff on original bone 17. Link rotations alternate to interlock. Additional animation tracks bake drape and a small lag from cuff motion into all 144 clips. They are cuff children, with the first link head at a constant local offset. This keeps the attachment exact during native frame interpolation. The metal orientations use the closest supported TRS to the baked drape; some leg-scale deformation is inherited. The original body positions, normals, UVs, weights, 40-bone rig, animation float tracks, names, timing and PCM sounds remain intact. The chain adds no gameplay logic or extra draw pass.

The chain uses baked motion, not world collision physics. Slopes, walls, water, jump clearance, attachment interpolation and the inherited small-detail integer packing require game inspection. The HD mesh retains TP POC1's static open-eye bake and its existing fine-detail packing limits.

| Provenance | Record |
|---|---|
| Source baseline | October 8 audit bridge `0914f957ed1260ad1c36102db16f2f67ce58191a` |
| TP mesh baseline payload SHA256 | `d6447975f22bcb1e62a6fac6009f5aa7ab7f78f65f499f19c9b4b7ce3216416e` |
| Softer eye payload SHA256 | `3f9d052d6542ff46e0c1b4e6f60a96dcfc821aeb32e42d871016c9250f152f5a` |
| Final HD payload SHA256 | `140befb32ac339f13c4a48e07cb3e71ad5dd8a3b6d22197777f24a53b17740f6` |
| Serialized HD Blob SHA256 | `132cbb2b593a13c557772acd007785dd7c5924ac22e2c1ba00de22c689930c65` |
| Chain DAE SHA256 | `00cc00950f26ad410b73d934ccd4a4749ae9b3b930aa82c0f8d9f4892dd5d91e` |
| Status | Integrated into the October 9 remaining-items candidate; see `overnight-merge-bridge-20261009.md` and PR41 for the exact publication/build checkpoint. Game acceptance remains pending. |

Local verification covers both complete production loaders in sanitizer and fast-math modes, active-transform deferral, 20 repeated model switches without duplicate registration, actual typed Blob decoding, exact native/donor ownership, model-specific Alt selection, missing-HD fallback, present-resource rejection, animation/audio preservation, chain atlas UV bounds and protected fur texels. The existing MM native skin/draw/action/lifecycle fixture runs alternating standard and HD models. Offline production skin math checks integer and half-frame poses of all 144 clips. The running preview uses actual packed native vertices, with the original wolf vertices identical before and after adding the chain; it is a slowed software render, not recorded gameplay.

The real ZAPD exporter generated both normal port archives locally. Their Wolf resources are byte-identical to the checked-in Blobs, and both contain the existing Shadow Crystal dependencies. Asset collision checks pass with identical Wolf bytes in both owners.

Reproduce the asset conversion with the recovered TP mesh POC1 and supplied chain files:

```bash
python3 tools/wolf_blue_eyes/build.py --source /path/to/zz_Wolf_Link_TP_Mesh_POC1.o2r --output /path/to/soft
python3 tools/wolf_blue_eyes/add_chain.py --source '/path/to/Wolf Link Chain' --wolf /path/to/soft/zz_Wolf_Link_Blue_Eyes_POC2.o2r --output /path/to/hd
python3 tests/mm_wolf/run_bundled_asset_tests.py
python3 tests/mm_wolf/run_real_asset_tests.py soh/assets/custom/objects/forms/wolf_link/gWolfLinkData soh/assets/custom/objects/forms/wolf_link/gWolfLinkHDData
```

For runtime acceptance, rebuild the candidate executable and regenerate both port archives together. In each game, transform with HD checked and unchecked, change the checkbox while transformed and confirm it waits until the next transformation, and repeat with alternate assets on and off. Inspect the revised eyes at normal distance in bright and dark scenes; inspect the cuff while idle, walking, running, jumping and attacking. Check chain intersections on slopes and near walls, audio, collision and camera behavior. Clearing the checkbox restores the bundled standard appearance; removing the candidate build restores the previous source/asset baseline. Runtime acceptance and master promotion have not been inferred from local checks.
