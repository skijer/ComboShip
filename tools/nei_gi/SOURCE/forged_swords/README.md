# Forged sword authoring toolkit

Imported from the user-supplied `NEI_Sword_GI_Latest_20261003` bundle. The seven
current GLBs live in `../../CHECKPOINTS/<slug>/<slug>.glb`; their exported
resources are installed under `soh/assets/custom/objects/nei_gi_redesign`.
The user requested subsequent proportion corrections. Master, True Master and
Gilded follow the supplied reference images with later feedback shortening
Gilded's blade and widening Master's blade root. The accepted True Master is
preserved; the two Master variants have independent blade, grip and guard
geometry. Great Fairy retains the prior two-handed proportion correction.
Current previews are linked from
[`../../PREVIEWS/sword-tuned-pass.md`](../../PREVIEWS/sword-tuned-pass.md).
The accepted Master, Gilded and True Master GLB/resource hashes are recorded in
[`../sword_visual_approval.json`](../sword_visual_approval.json). Visual approval
does not establish in-game appearance or a successful full game build.
Razor Sword, Biggoron Sword and Iron Knuckle Axe use the separate legacy toolkit;
Razor and Biggoron also receive the requested silhouette/proportion correction.

`meshkit.py`, `preview.py` and `build_swords.py` retain the imported toolkit.
`MODEL_MANIFEST.json`, `PROVENANCE.json` and `SHA256SUMS.txt` remain evidence from
the original bundle; the checksum file also refers to bundle files outside this
contained toolkit. `native_bounds.json` records the original bundle's authored
blade dimensions with the exported fixed-point matrix applied. It is neither
Nintendo geometry nor the current corrected bounds. Negative blade Y minima
include tangs inside the crossguards and upper grips. Rebuilt checkpoint metadata
records the current author bounds, matrix and GLB/resource hashes; an
`imported_approved_model` value of false identifies a changed bundle model.
The current six corrected mesh bounds are measured in
`../sword_proportion_bounds.json`; its native coordinates apply the exported
fixed-point matrix and precede the host actor's transform.

Use Python 3.11 or newer with NumPy and Pillow. From the repository root, install
the recorded dependencies in your Python environment, then rebuild selected
models and their current front/back checkpoints:

```sh
python3 -m pip install -r tools/nei_gi/SOURCE/forged_swords/requirements.txt
python3 tools/nei_gi/SOURCE/forged_swords/rebuild.py master_sword four_sword --install
python3 tools/nei_gi/check_forged_proportions.py
python3 tools/nei_gi/verify_assets.py
```

Omit item names to rebuild all seven. Omit `--install` to keep the generated game
resources under `tools/nei_gi/RESOURCES`; checkpoints are still updated. For a
scratch rebuild, use `--output /tmp/forged-swords-rebuild` without `--install`.
Existing entry points should dispatch to this helper in a subprocess so its
`meshkit` and `preview` imports stay isolated from the older toolkit.

A bundle-format build of the current recipes is also available:

```sh
python3 tools/nei_gi/SOURCE/forged_swords/build_swords.py --output /tmp/forged-swords-bundle
```

It writes `GAME_ASSETS/RESOURCES`, `PREVIEW_GLBS` and a model manifest. It uses the
current corrected recipes and does not recreate the original imported bundle.
The original manifests and provenance above remain unchanged historical evidence.
Front/back checkpoint images use the actual authored triangles and material
settings; these are offline previews. The import and rebuild metadata retain
`runtime_tested: false`. See the
[`runtime preview instructions`](../../runtime_preview/README.md) for rotating
sword previews with production particle geometry; those require Mesa EGL/OpenGL
libraries and a C++20 compiler in addition to the Python dependencies.
