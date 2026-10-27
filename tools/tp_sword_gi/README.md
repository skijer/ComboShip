# TP sword GI — POC 2

This candidate continues POC 1 on the PR #41 baseline, commit `c0181856968dd6f3869be0d1aa240743091fd6a8`. It is isolated on `poc/tp-sword-gi-20261009`. POC 1 and its binary patch are retained separately. Visual acceptance and configuration-specific gameplay proof are pending.

The user's rotation review found the authored swords too flat and the heavy pair too short beside Great Fairy. All ten authored sword variants receive 50% more blade depth and 20% more depth in planar fittings. Rounded grips retain their existing depth; the TP hilt/grip atlas shares one mesh and receives the modest fitting adjustment. Thickness is authored into the actual meshes and normals, with no camera enlargement. The top three retain their face-on shapes, palettes and texture atlases. Kokiri, MM Kokiri, Razor, Four Sword and Great Fairy retain their exact X/Y positions, topology, UVs, materials, native display lists, textures and scale matrices.

Biggoron's tip changes from author Y=122 to 166 and its grip grows from 35.5 to 44.5 units. Giant's Knife changes from Y=112 to 152, with a 39.5-unit grip instead of 28.5. At the existing common scale, overall heights are 145.2 and 132.6 versus Great Fairy's unchanged 129.9. Their distinct fittings and colors remain. Their production particles follow the longer blades, and Biggoron's held matrix is recalibrated while preserving native reach.

| Sword | Candidate treatment |
| --- | --- |
| Master Sword | Supplied TP blade and winged hilt, violet grip, original steel and hilt atlases enlarged 4× with Lanczos resampling and conservative sharpening. |
| True Master Sword | Same TP frame, white hilt/grip, blue inset jewel, tempered gold blade; source atlases enlarged 4×. |
| Gilded Sword | TP frame with the blade shortened 12%; three broad gold/silver diamond facets from the supplied reference, red grip, silver fittings. |
| Biggoron's Sword | Straight Ordon blade widened for a heavy sword; longer polished steel blade, sculpted bronze Goron guard/seal, blue wrapped grip. |
| Giant's Knife | Broader, shorter Ordon blade, riveted iron crossbar, brown-red grip, quieter sheen, separate authored GI identity. |

Ordon supplies the straight blade frame for the heavy pair. Scimitar of Twilight remains a source reference for the ornamental fittings; its curved silhouette is not substituted. All original supplied OBJ/MTL/PNG files are retained in `INPUTS`. Resampling preserves source detail; it does not invent high-resolution engraving.

The underlying blades remain opaque. A coplanar, normal-generated reflection decal uses the Four Sword authoring method after the base blade, preserving the source engraving. It replaces TP Master's zero-thickness environment card. Previous texture resources remain available unchanged for recovery; the active lists use only the candidate materials. The exact meshes and production particle samplers appear in the previews; their camera, lighting and reflections are offline approximations.

The five retained designs are depth candidates rather than new visual redesigns. Their original accepted resources are recoverable from the baseline commit and the handoff's recovery archive. Previous visual approvals remain historical records; none of the revised depth candidates is marked approved.

Giant's Knife and Biggoron originally share `GID_SWORD_BGS`. A concrete Knife callback and native `GI_SWORD_KNIFE` lookup now preserve their separate identities. Its effect ID is appended as 38; every prior ID is retained. Alt-enabled equipment/Din and legacy GI priorities remain available. Alt-off uses the shipped authored GI through the existing base-owner route in both hosts, including foreign Knife awards.

Four existing held-sword attachment matrices are recalibrated to the new grip/tip positions because they use the same GI resources. Native melee reach, animation, damage, collision and progression are unchanged. Giant's Knife's distinct mesh is scoped to its GI; its existing held family remains unchanged.

## Reproduce

From the repository root, with Python/numpy/Pillow and the project's C++ dependency headers available:

```sh
python3 -B tools/tp_sword_gi/build.py --output /absolute/candidate-output --install
python3 -B tools/nei_gi/generate_frame_bounds.py
python3 -B tools/nei_held/build_swords.py
python3 -B tools/nei_gi/verify_assets.py --report /absolute/asset-verification.json
python3 -B tools/tp_sword_gi/verify.py --report /absolute/preservation.json
python3 -B scripts/diagnostics/run_nei_gi_tests.py --combo --sanitize
python3 -B scripts/diagnostics/run_sword_asset_toggle_tests.py --sanitize
python3 -B scripts/diagnostics/run_sword_fallback_tests.py --sanitize
python3 -B scripts/diagnostics/run_sword_receipt_tests.py
python3 -B scripts/diagnostics/run_sword_mod_gi_tests.py --sanitize
python3 -B scripts/diagnostics/run_sword_resource_view_tests.py --sanitize
python3 -B tests/nei_held/run_held_sword_tests.py
python3 -B tests/nei_held/run_oot_sword_limb_tests.py
```

`render.py /absolute/preview-output --before-root /absolute/poc1-checkpoints` generates front views, common-scale before/after, edge-on comparisons, two rotation boards covering all ten variants, and the Great Fairy depth view using Mesa/EGL and ffmpeg. The before directory must contain the POC 1 sword checkpoint folders. `depth.py` reads the exact retained designs from the stated baseline git commit; that commit must be available for regeneration. `check_forged_proportions.py` remains the historical pre-TP approval gate for the preserved forged source; it is not the acceptance gate for these requested replacement and depth candidates.

## Handoff and runtime proof

The package contains the forward and reverse binary git patches, changed repository files, candidate GLBs/textures/resources, original inputs, previews, verification logs and a baseline recovery archive. Apply the forward patch to the stated baseline in an isolated checkout, build ComboShip normally and regenerate its shipped assets. The reverse patch restores this candidate's edits when applied to its exact matching state.

This is an integration package, not a standalone mod O2R. A mods-only overlay cannot replace the shipped `oot-gi-base` resources selected by Alt-off. No full game build, mounted-pack gameplay run, push, CI run or promotion is claimed.

Runtime review should cover both hosts with Alt on/off, optional item effects on/off, world/shop/acquisition displays, all MM receipt forms, separate Biggoron/Knife awards, and existing held attachments. Check the blade reflection decal for depth flicker, texture orientation and apparent brightness during rotation. Record executable/source commit, shipped asset hash, mounted archives and settings with any acceptance screenshots.
