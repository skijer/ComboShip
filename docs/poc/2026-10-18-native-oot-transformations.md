# Native MM transformation sequences in OoT

Candidate branch: `poc/oot-native-transformations-20261018`.
Parent: `1e650f7fe870a373f8ffad6bdd1a309362538bb3` (forest actors source).
Public develop reference: `a82852a3a02c8cabc541cce7c124da71c5755efb`.
The checkpoint manifest identifies the exact final commit/tree. This candidate
contains the cumulative forest actor baseline and a bounded transformation patch;
apply the patch to a newer cumulative branch rather than resetting its ancestry.

## Implemented behavior

Deku, Goron, Zora and Fierce Deity use native MM mask-on, struggle, flash,
recovery and mask-off clips in the shared SoH player controller. This covers
ComboShip's OoT game, its solo SoH mode and standalone SoH compilation. No MM
engine call, active MM save or MM DLL is required for the sequence.

- Human `cl_setmask` (66 frames), `cl_setmaskend` (3-frame loop), native flash,
  and `cl_maskoff` (10 frames) on the revealed form. Goron/Deku/Zora use their
  own 15-frame removal starts; Fierce Deity shares the native Zora removal clip.
- Native item, mask, freezing, mask-breaking, transformation voice, face-change,
  lightning and flash cues through the existing MM bank/dispatcher. Skipping
  cancels the outgoing sounds and permits the new flash cue in the same update.
- Native normal/stretched mask attachments, blue animated face veil, fog/ambient
  keys, temporary point light, alpha-100 framebuffer motion blur and white flash.
  A temporary front-facing OoT subcamera adapts the MM presentation.
- Held items stow synchronously before taking animation ownership. Drawn swords,
  custom props and an outgoing worn OoT mask cannot overlap the new mask. Deku
  bubble aiming exits its camera, slingshot pipeline, breath and charge state.
- Pose ownership prevents OoT idle copies from overwriting native removal and
  recovery. Completion, missing resources, skip, death and scene destruction
  release controls, camera, lighting and positional SFX ownership. A reused
  PlayState address cannot release a new scene's camera or light handles.
- The existing instant preference remains available; missing native animation or
  mask/veil geometry chooses a stable five-frame fallback. Other custom forms
  keep their existing transition routing.

The donor is `mm/src/overlays/actors/ovl_player_actor/z_player.c`:
`D_8085D160`, `Player_Action_86/87`, `func_80855218`, `D_8085D8F0/D904` and
its initialization/recovery branch. Attachments come from MM's `z_player_lib.c`;
blur ordering follows MM's `Play_DrawMotionBlur`. MM camera settings cannot be
passed to the OoT camera ABI. OoT allows skip after five updates; MM's first-use
week-event gating is not imported into OoT saves.

## Verified source candidate

- 80 production-controller scenarios passed with ASan/UBSan across standalone
  and COMBO_BUILD fixtures, including all four forms on/off, both ages, cue
  timing, skip, instant preference changes, missing clips/meshes, load failure,
  attachments/veil/blur, camera/lighting and scene/death lifecycle boundaries.
  The fixture uses actual item initialization, bubble cleanup and camera exit
  bodies for drawn-item and first-person regression cases.
- The actual MM SFX bank and dispatcher passed in sequence-synth and direct
  audio modes: all eight cues, pending/active cancellation with poisoned player
  position memory, and same-frame skip-to-flash. All existing Goron audio
  lifetime scenarios also passed (28 scenario/mode combinations total).
- All four affected translation units compiled to native objects in both modes:
  `mm_player_form.cpp`, `z_player_lib.c`, `z_play.c`, `mm_anims_data.c`.
  Existing warnings remain: respectively 1, 214, 56 and 0 per configuration.
- Existing mask routing, fast MM transformation/easy equip, mask progression,
  NEI leaf sound, MM SFX lifetime/parser and solo-games behavior suites passed.
  The solo gate covered three compiled mode probes and 18 production-boundary
  suites. Its initial missing SDL_net header was resolved using an existing
  dependency include path; no source change was needed.
- Read-only `audit_native_transform_assets.py` validated native animation
  strides/counts, nine display-list graphs and their referenced resources,
  plus the veil's segment/dual-scroll data against the unchanged MM archive.
- Code review findings were corrected and covered: camera sentinel, scene
  address reuse, missing-resource routing, visual effects, synchronous stow,
  first-person removal, outgoing mask overlap and positional sound teardown.
  `git diff --check` passed. The source recovery kit was independently restored
  from the retained baseline bundle and checked with git fsck and file hashes.

Animation/camera/resource/output services are fixture boundaries. ASan and
UBSan ran with `ASAN_OPTIONS=detect_leaks=0` because LeakSanitizer cannot inspect
this sandbox's process namespace. Native object compilation is not a full
executable link, Windows CI result or configuration-specific runtime proof.
No game was launched for visual/listening acceptance and no master was promoted.

## Dependencies and runtime acceptance

Keep `mm.o2r` available to SoH's existing MM asset loader, including in solo
SoH, and use the accepted form/Alt/HD packs and their existing load order.
The inspected unchanged archive SHA256 is
`f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59`.
Game archives and optional model packs are external inputs and are not modified
or redistributed by this source kit. Native mode uses the existing default
`gMods.TransformMasks.InstantTransform=0`.

Runtime acceptance is still required on the cumulative executable: all four
masks on/off as child/adult in ComboShip OoT and solo SoH; hands/face/struggle,
veil, flash and morph continuity; audible cues with BGM; skipped repeats;
controls/equipment after return; scene reload while transformed; Zora in water;
Deku bubble aiming and drawn-weapon transitions; one interrupted transition;
and instant mode. Check mask fit and camera composition with the exact accepted
optional packs, plus resolution/interpolation changes during the blur.

Reproduce the narrow automated checks from the repository root:

```sh
python3 -B scripts/diagnostics/run_oot_native_transformation_tests.py
python3 -B tests/goron_audio/run_tests.py
python3 -B scripts/diagnostics/audit_native_transform_assets.py /path/to/mm.o2r
python3 -B scripts/diagnostics/run_mask_transformation_tests.py
python3 -B scripts/diagnostics/run_mm_fast_transformation_tests.py
python3 -B scripts/diagnostics/run_mask_progression_tests.py
python3 -B tests/nei_leaf/run_tests.py
python3 -B tests/mm_sfx_lifetime/run_tests.py
python3 -B tests/solo_games/run_tests.py
```

Native compilation uses the normal project/dependency headers; the checkpoint
includes the object-compilation recipe and result records. The new native
sequence regression is part of the fast artifact CI and NEI regression runner.
