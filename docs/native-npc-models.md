# Optional native NPC models and fitted poses

This POC adds five separate O2R candidates using the optional paired-rig pattern
from Young Fado. The source checkpoint targets ComboShip develop
`a82852a3a02c8cabc541cce7c124da71c5755efb`. It is not a game executable or a
runtime-accepted release. Keep the existing accepted build and packs intact.

| Pack | Native and placed actor coverage | Private namespace |
| --- | --- | --- |
| Saria Anime | En_Sa, Demo_Sa OPA/XLU, static Saria | objects/object_sa/NativeNpcV1 |
| Saria Render | Same Saria actors; alternative to Anime | objects/object_sa/NativeNpcV1 |
| Kokiri Twins Render | En_Ko girl types 1, 5, 6, 9, 10; static Kokiri girl | objects/object_kw1/NativeNpcV1 |
| Young Malon | En_Ma1; static child Malon | objects/object_ma1/NativeNpcV1 |
| Adult Malon Render | En_Ma2, En_Ma3; static adult Malon | objects/object_ma2/NativeNpcV1 |

The girl types cover the twins' house, forest, shop, and story-gated shop awning.
En_Ko type 12 and static Fado remain separate. Install only ONE Saria variant.
The girl, young Malon, adult Malon and selected Saria packs can coexist.

## Build and install

Apply the source checkpoint to its recorded base, or reconcile it into the
cumulative source branch before the next normal build. Place the desired O2Rs
in that build's usual mods directory and enable Alternate Assets. These packs
need the matching adapters; an older executable does not acquire new actor
support by loading the files. No full game executable was built for this POC.

Each pack has a native base skeleton and an Alt custom skeleton under a private
path. Both preserve native joint order, count, hierarchy and matrix slots.
POC2 fits Alt limb translations to the GLB using one body scale, instead of
stretching its torso to the unmodified native neck and shoulders. The base
retains the original native translations. SkeletonPatcher can switch between them. The
native animations, dialogue, placements, progression and actor identity remain
owned by the game. Missing optional packs use the original skeleton path.

The POC2 repair candidate includes the earlier Young Fado NPC selectors,
reconciled into the current native and static Kokiri actors. It retains the
legacy seated cloth wrappers and paired `Animations/Seated5` animation.
Generic private pose clips and limb overrides take precedence, including when
the actor still uses that legacy animation key. No donor actor file was copied
over the current ComboShip actor. The published POC1 checkpoint predates this
integration; use the repaired source candidate for the cumulative build.

## Archive-only correction contract

One matching build installs the resolver. Later fitting revisions use O2R
resources under the selected model's namespace:

| Revision | Resource contract |
| --- | --- |
| Hand basis, fingers, mesh seams, rigid vertex ownership | Existing Body display lists and vertex arrays |
| Limb proportions and bind offsets | Alt Rig/LimbNN; keep native hierarchy, joint count and matrix slots |
| Pose-specific rotations | alt/NS/PoseAnimations/FULL_NATIVE_ANIMATION_PATH |
| Pose-specific hand, arm, cloth or prop mesh | alt/NS/PoseAnimations/FULL_NATIVE_ANIMATION_PATH/Limbs/LimbNN |
| Adult Malon basket, including static actor | alt/NS/PoseAnimations/FULL_NATIVE_ANIMATION_PATH/Props/Basket |
| Eyes and mouth | Existing Faces/LxRyMz wrappers and FaceParts; neutral head remains available |
| Texture/material fitting | Private texture and display-list resources |

`NN` is the zero-based native limb index. Callback limb indices are NN+1.
`FULL_NATIVE_ANIMATION_PATH` includes its object directory; it is not only the
clip basename. Example:

```
alt/objects/object_kw1/NativeNpcV1/PoseAnimations/objects/object_os_anime/gKokiriSittingCrossedArmsLegsAnim
```

Private pose clips are ordinary normal-animation resources. The runtime rejects
wrong types, joint counts, frame counts, channel bounds, and root translations.
It preserves the native animation identity used by dialogue/cue/prop code and
uses the engine's normal frame interpolation and positive/negative morphs.
Alt OFF or a missing/invalid fitted clip selects the native animation. A fitted
static crossed-legs clip owns its legs/head too; the older fairy-leg overlay is
then skipped. Without a fitted clip that overlay behaves as before.

A pose-specific mesh is optional independently of a fitted clip. This permits a
hands-only or cloth-only correction. A full body-part display list must use the
existing native matrix-slot convention and restore its host matrix. Faces or
props included in a pose-specific head/hand wrapper must preserve their required
expression/prop routes. Renderer recursion and vertex-cache limits still apply.

## Build and inspect assets

Python dependencies: numpy, Pillow, scipy. The native archive and original GLBs
are read-only inputs and are not included in the source checkpoint.

```sh
python3 scripts/npc_models/native_npc_models.py /path/to/oot.o2r /path/to/glbs /path/to/output
python3 scripts/npc_models/verify_native_npc_models.py /path/to/oot.o2r /path/to/glbs /path/to/output --allow-fitted-rig
python3 scripts/npc_models/preview_native_npc_models.py /path/to/oot.o2r /path/to/output
```

The body writer preserves the exact original 4096x4096 diffuse pixels. It keeps
source triangles except for Young Malon's explicitly recorded broken clasp
replacement: the manifest lists removed source faces and the rebuilt surface.
Its Bowser artwork is projected from the supplied adult Malon GLB, which is
also required when compiling the child pack. The adult pack is not required
at runtime. Native facial wrappers use separate private textures for the
authored blink states, the requested forward gaze on Saria Render and the two
young Malon cheek specks. Neutral young Malon eye artwork is preserved; only
the tiny cheek masks were taken from the cleanup draft. Original GLBs are
read-only. The writer uses the recovered MIT Fado resource writers under
`scripts/npc_models/native_assets/`. The preview decodes exported display lists,
vertex caches, matrices and texture references, then poses them with real native
animation data and any fitted clip in the candidate. It is an offline review,
not an in-game capture. The preview reads the Alt rig offsets too. For a later
proportion-fit revision, pass `--allow-fitted-rig` to the verifier; the native
fallback, hierarchy and matrix layout must still be preserved.

For a narrow pose correction, start with a JSON recipe for the exact parent pack:

```json
{
  "schema_version": 1,
  "parent_sha256": "SHA256_OF_THE_SELECTED_O2R",
  "clips": [{
    "native_animation": "objects/object_os_anime/gKokiriStandingArmsBehindBackAnim",
    "rotations": [{"joint": 9, "mode": "add", "binang": [0, 0, 128]}]
  }]
}
```

The values above demonstrate a small rotation; they are not an approved sway
fit. Angles are integer native binary angles; 32768 is a half turn. Joint 0 is
root translation and cannot be edited. Omit `frames` for every frame, or supply
an explicit list of frame indices. `mode` may be `set` or `add`.

```sh
python3 scripts/npc_models/edit_native_npc_pose.py parent.o2r oot.o2r recipe.json fitted.o2r
```

Enable the full `fitted.o2r` in place of its parent. `--overlay` instead creates a
small patch containing only edited private clips plus provenance; it needs the
exact parent enabled and patch precedence in the archive load order. The editor
also understands `YoungFadoNpcInfo.json` from the recovered Fado NPC packs.

Review sway from front/back through the whole loop, and seated crossed arms
from front/side before accepting a correction. Check wrist axes, palm direction,
elbow volume, arm contact, cloth coverage, feet and tracking. Preserve separate
pose-4 and pose-5 behaviors. A change to an actor's story behavior, rig topology
or an unsupported rendering feature can still require code.

## Verification and remaining work

The checkpoint records focused native actor compilation/routing, the production
C++ pose resolver, actual native animation sampling/morphing, Kokiri donor-pose
controls, culling controls, Saria diagnostics and archive readback. The normal
fast regression gate now calls the new adapter tests. Run them without game
assets with:

```sh
python3 scripts/diagnostics/run_native_npc_model_tests.py
python3 scripts/npc_models/test_native_npc_models.py
```

The recovered native archive is additionally needed for Kokiri pose and export
checks. No platform link, full game build, live archive toggle, story-gate test
or in-game visual acceptance is claimed.

These are fitting candidates. Seated tunic/leg intersections, shoulder/neck
joins, Malon's collar/scarf, Saria's ocarina grip and facial expression art still
need refinement and runtime review. The GLBs have a single painted face and no
expression morphs. POC2 uses separately authored eyelid textures and a joined
skin surface for each blink, avoiding the open eye shells' shading rims.
The neutral face is retained except for the explicitly requested gaze/cheek
repairs. Mouth states still require separate artwork review. Do not treat the
expression routing check as visual acceptance. The new contract lets those
fitting edits ship as updated assets after the matching adapter build.

## POC3 pose and detail review

POC3 retains the accepted Saria head and torso fit at native vertex precision.
It uses Fado's relaxed-finger baker, a palm frame that constrains roll, and
Fado's fixed-armhole/free-cap fitting principle. A bounded source-to-chest/arm
blend avoids folding these models' thin segmented shoulder rings. The girl and child Malon have private
pose meshes for folded shoulders; child Malon also blends the elbow surface.
Child Malon's dress includes sleeve fragments; these follow the same shoulder
blend, and her idle hand targets clear the actual dress envelope.
Adult Malon's front scarf is anchored to the chest rather than auto-weighted
to her clavicles/head. These are asset compiler changes.

Private arm clips fit Saria's ocarina, seated hands and hands-behind-back wait;
the two Malons' singing poses; child Malon's idle/raising arms; and the girl's
folded arms. All frames retain native root, legs, feet, torso and head channels.
Native girl/adult idle arm rotations are retained. Reach limits clamp targets
without stretching bones; residuals are recorded per clip. Adult Malon's
basket retains its native materials and has a private handle fit to both palms.

The preview now closes Saria's eyes while playing and can show the static
talking state. Native En_Sa already owns this behavior. The static actor uses a
small interaction eye selector: playing closes the eyes, dialogue opens them,
and unrelated poses retain their eye state. Both the native texture segments
and replacement head use that selector.

Additional art is limited to Young Malon's mouth-corner and crown cleanup and
adult Malon's slightly stronger neutral smile. Approved eye/blink artwork is
preserved. Front, back and side views are exported from actual O2R display
lists; native animation controls are clearly labeled untextured clay renders.
Pose-specific surface fitting is baked for the pose family, not live smooth
skinning. Full-motion runtime acceptance and the merge/build gate remain open.
Young Malon's original collar/tie edges remain visibly rough around the rebuilt
clasp in close-up and are not visually accepted by the structural checks.

The repair checks include the real static Fado initialization, her 16-joint
seated fitted clip and hand-mesh resolution through native/legacy keys, and
Alt fallback. An offline O2R proof contains the earlier fitted seated rotations
under the generic private animation key, with the native root and two-frame
timing preserved. This is loader and asset-format proof, not live game proof.

## POC4 continuous joint blending and hand review

The parent source checkpoint is `1ef3752b` (POC3). POC4 is a separate fitting
candidate; the accepted build, original GLBs and POC3 archives are retained.
Both Saria packs and the twins pack were rebuilt for this visual pass. The
existing Malon packs have not acquired these changes merely because their
source checks pass.

The GLBs contain source skin weights. The earlier native export baked a bind
shape but retained only the strongest native bone at draw time; finger joints
were also collapsed to one hand. Its selected shoulder meshes were baked at a
clip midpoint. Those are separate limitations from a rig having no weights.

The exporter now authors two-bone bands at shoulders, elbows, wrists, hips,
knees and ankles. Each influence retains its source-to-bone map; it does not
blend an already-retargeted bind shape a second time. Coincident seam copies
share weights and their source position. Saria's head and central
torso fitting remain protected. Her relaxed fingers have independent spread
and curvature; the ocarina curl bends towards the palm instead of away from
it. Grip measurements are reset between clips so they cannot move later
relaxed hands.

`alt/NS/Skin/Palette` is an ordinary U16 Array resource containing version 1,
native matrix count, source-to-bone deformation bases and quantized two-bone blends.
`alt/NS/Skin/Limbs/LimbNN` holds optional weighted body lists. A pose may supply
`alt/NS/PoseAnimations/FULL_NATIVE_ANIMATION_PATH/Skin/Limbs/LimbNN`, for example
a curled hand with its original ocarina route. The palette appends matrices to
segment D without changing native skeleton hierarchy, joint count or original
matrix slots. It evaluates the actual matrices produced by the native flex
draw, including actor callbacks and tracking, after all native limbs are ready.
OPA, XLU/generic and flex LOD draws use the same calculation. The normal frame
interpolation recorder also receives the blended matrices.

This is linear skin blending with 1/16 weight steps, not a live cloth solver.
The palette is bounded to 256 total matrices. A missing, invalid or disabled
palette selects the previous rigid/pose lists; native assets remain available.
The weighted path requires the matching POC4 source build. After that build,
weights, bind matrices, hand shapes and pose-specific surfaces can be revised
in the O2R. Changes outside that resource contract may still require code.

For every future custom model, require a joint review before accepting the
archive: open/relaxed palms, prop contact, shoulders raised and folded, elbows,
seated knees, ankle/boot seams, and transitions between the tested poses.
Inspect both sides and intermediate frames. Source auto-weights and a good
single still image do not substitute for that review. Missing geometry and
poor segmentation require surface repairs in addition to weight tuning.

```sh
python3 -m unittest discover -s scripts/npc_models -p 'test_*.py'
python3 scripts/npc_models/verify_native_npc_skin.py /path/to/oot.o2r /path/to/glbs
python3 scripts/diagnostics/run_native_npc_model_tests.py
python3 scripts/npc_models/review_native_npc_joints.py /path/to/oot.o2r /path/to/poc4 /path/to/poc3 /path/to/review
```

The skin input check covers all five source models, welded weight consistency
and export-order independence. The production C++ check exercises a live bend,
inverse bind pivots, actor translation, weighted-list selection and invalid/Alt
fallback. These are focused implementation checks. The POC4 comparison sheets
decode the actual weighted O2Rs; full-game motion, runtime acceptance, merge
and the two-hour build remain held for review.

Shaded previews use the engine's directional-light calculation: normalize
the matrix-transposed light, then dot it with the local quantized normal.
Normalizing a transformed vertex normal produces different shading for
non-rigid skin blends. The review light is controlled; it does not reproduce
each scene's live lights.

The twins' idle and hands-behind-back sway resources are copied byte-for-byte
from the native archive, including all arm, torso and head channels. Their
crossed pose retains native non-arm channels while its hands curl around the
forearms. Source proportions remain fitted, so native rotations do not imply
identical world-space vertex positions. The forehead and headband's skin edge
share smooth skin normals without moving the headband geometry. Joined eye
surfaces replace overlapping shells, with their retained boundary vertices
fitted to the same skin curves. The neutral projection uses the original
painted face and excludes stray headband/hair surfaces. All expression states
use the repaired face route; approved blink texture bytes are unchanged.


## POC5 Saria asset repair pass

POC4's Saria grip, cloth joins and traditional eye surfaces were rejected in
visual review. Its twins neutral face was also rejected as flattened; that
candidate remains held and is not promoted by this Saria-only pass. POC4
archives and the original GLBs/native archive remain separate, read-only inputs.

POC5 changes archive authoring only. There are no new actor, sampler, renderer
or runtime adapter changes beyond the POC4 contract. A matching POC4 adapter
build is still required once; these POC5 refinements can then be revised through
O2R resources without rebuilding the executable.

- Explicit source-hash-gated tunic pieces use one coherent chest map. Their
  auto-rigged collar vertices are restored to the garment instead of moving
  with the head. Actual facial geometry, source scale and fitted limb lengths
  remain protected. Sleeve contact rims start on that same chest map and
  transition smoothly to the arm. Generic and per-asset authored weights share
  the existing version-1 palette contract.
- Anime sleeve outer rims share normals only across eligible nearby pieces;
  inward caps are excluded. A bounded posterior calf normal field reduces the
  diagnosed angular streak without changing source XYZ, UVs, boots or silhouette.
- Traditional front-tunic pieces share a smooth depth-derived normal field.
  This corrects the original source's jagged waist/yoke shading and chest patch
  without changing the garment shape or removing source triangles.
- Traditional eyes exclude projected hair. Blink patches retain visible skin
  contours at their boundary. Neutral gaze composition keeps the original
  lower lash and copies centered artwork only through the visible eye aperture.
- The original 4K Diffuse remains byte-identical. A separate corrected diffuse
  changes green chroma only in the audited hands/ears, using this model's clean
  cheek chroma at original linear luminance. Other UV footprints are protected;
  body and pose-specific limbs use the same corrected atlas.
- The traditional yoke contains a disconnected 44-triangle Tripo shard beside
  the upper arm. Only that audited component is omitted; the manifest records
  its exact original face indices and readback verifies the bounded removal.
- Per-finger grip profiles retain the native wrist orientation. Palm offsets
  also export forearm wrist-rim overrides, keeping both sides of the connection
  consistent. The profile is shared by all four native ocarina-family clips.

`native_npc_contact.verify_archive(native, candidate)` decodes the exported
integer vertices, inverse-bind palette and blend slots, then checks every frame
of all four ocarina clips. It measures signed triangle-surface clearance and
checks triangle edges in both directions for crossings. This is an offline
archive check: its float pose matrices do not emulate the game's final native
Mtx conversion, scene lights, callbacks or live actor configuration.

`review_native_npc_saria.py` renders exact POC4/POC5 versions into comparison
sheets, including waiting hands/feet, standing and raised shoulders, elbow
joins, rear calf shading, neutral/closed eyes and prop contact. Previews are
actual archive geometry, not generated likenesses. Full-game verification and
user acceptance remain separate from these implementation and asset checks.


POC5 checks (2026-10-18): 46 Python tests pass. The five-source skin loader,
weld and export-order regression passed; both Sarias were rechecked after the
final contact-rim adjustment. Original input and approved face-art bytes stay
unchanged. See the packaged Verification.json for exact final O2R identities,
protected-feature comparisons and all-frame contact results. Full C++ adapter
checks belong to the POC4 checkpoint; no runtime source files changed in POC5.

```sh
python3 scripts/npc_models/native_npc_contact.py /path/to/oot.o2r /path/to/Saria_Anime_Native_NPC_POC5.o2r --output /path/to/contact.json
python3 scripts/npc_models/review_native_npc_saria.py /path/to/oot.o2r /path/to/poc5 /path/to/poc4 /path/to/review
```
