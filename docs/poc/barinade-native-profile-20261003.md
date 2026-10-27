# Barinade native model candidate

Parent checkpoint: `4542e9a`. Worktree: `gi-barinade`, branch
`poc/gi-barinade-native`. No remote mutation or master promotion.

The old OoT export excluded Barinade from the skeletal class and selected the
generic soul skull. This candidate exports `object_bv/gBarinadeBodySkel` and
`gBarinadeBodyAnim`, with the native 64-joint count, world Y=-25 and scale 0.03.
An appended POD profile carries the native procedural behavior and two canonical
XLU paths: `gBarinadeDL_008D70` (limb 25 ring) and `gBarinadeDL_008BB8`
(limbs 10–19 electric effects). The existing soul flame stays independent.

The consumer freezes the selected animation on its last frame and reproduces
the native limb 6/61 sinusoidal scaling, limb 7/10–19 rotations, limb 20 material
and quarter-turn, and XLU postdraw for limb 25, limbs 10–19 and limbs 29–55.
Initial and per-limb scrolls use `gameplayFrames`, as native Barinade does.
Segments 8/9 are restored on both streams after drawing. The normal/flex decision
still follows the loaded skeleton, preserving replacement flex matrices and the
accepted Volvagia jaw path. Already-routed rigid postdraw paths retain one owner
prefix instead of being prefixed twice.

OoT's registered resource manager owns the skeleton, clip and model resources,
including its Alt selection and vanilla fallback. Both native SoH and the bridge
apply the same native effects to compatible selected custom assets: frozen pose,
procedural rotations/scales, electric/ring/self XLU meshes, material scrolls and
independent flame. Surviving limbs retain their selected replacement display lists.
Normal and flex replacements follow the loaded-type route. A mismatched joint
count is rejected before emitting commands. Native limb semantics are required;
this does not invent a remapping for arbitrary rigs. Installed-pack runtime
acceptance is still required.

Verification:

- `run_boss_soul_model_tests.py` first failed the production Barinade skeleton
  gate; it now passes normally and with ASan/UBSan.
- `run_foreign_soul_skeleton_tests.py` passes both hosts normally and with
  ASan/UBSan. It executes the production recipe, all native Barinade callbacks
  versus foreign callbacks at every limb for frames 0/7/600, and native engine
  limb traversal for 63 display limbs under repeated normal/flex Alt selection.
  It checks frozen initialization, XLU ring/electric/self draws, native MM flame,
  owner paths, matrices, segment cleanup and invalid/incompatible-recipe fallback.
  Selected custom geometry retains native 53 OPA and 38 XLU submissions in
  both the bridge and native SoH callbacks, including electric/ring effects.
  Existing Volvagia-style head/jaw, flame and owner/Alt/cache/fallback checks pass.
- `--baseline-barinade` loads checkpoint `4542e9a`'s consumer and fails the
  native rotation/hiding comparison, demonstrating the missing behavior.
- `run_barinade_syntax_tests.py` compiles the production callbacks and frozen
  animation initialization against both actual engine headers. It uses MM's
  `Matrix_RotateXF` and SoH's `Matrix_RotateX` through the host adapter.
- Leak detection is disabled because this runtime does not permit LeakSanitizer's
  `/proc` inspection. No leak-proof claim is made.

Resource I/O, animation initialization and matrix primitives are fixture
boundaries. Full application build and installed TP-pack visual proof are
pending. The decisive runtime check is Barinade in native SoH and imported MM
shop/acquisition with the same TP pack/load order and Alt off/on, including
re-entry and adjacent Volvagia: actual selected body and XLU parts, independent
flame, frozen body pose with procedural motion, and retained Volvagia head/jaw.
