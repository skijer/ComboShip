# Boss soul aura and Great Spin GI — candidate checkpoint

Baseline: PR #31 head `9d9b75d195d34924969df10d196710f0ff65e2ef`, tree
`9e3a2a6006c8d77f6e733da1987dd44681566ba7`. The local mirror at `48b30843`
has that exact tree. This retains the corrected foreign skeleton dispatcher,
key body/emblem colors, standard bank attribution, item repairs, mask/remains
bridge and opt-in streamed positional audio work. Cor reports the TP Volvagia
model now renders correctly; preserve that reported runtime breakthrough.

Candidate branch: `poc/soul-ability-gi-20261002` in
`/workspace/scratch/c89f295fe199/comboship-soul-gi`.

## Bounded changes

- The existing animated soul flame now pins neutral opaque prim/env colors,
  matching the static foreign-soul path. This removes dependence on a prior
  shelf material's transparent colors. It is a source-level candidate for the
  missing aura; the exact installed boss pack has not been runtime inspected.
- Routed animation paths are cached per owner as well as per literal address.
  Shared effect literals no longer retain the first game's namespace.
- Great Spin Attack uses MM's actual disk and cylinder at GI scale with its
  native animated segment-8 scroll. Its existing Great Spin Burst cosmetic is
  queried live. Native MM, MM-in-OoT and OoT's imported Great Spin alias share
  the renderer and the MM resource owner. Foreign recipes remain live after
  acquisition latching. Dormant donor lookups retry rather than freeze a missing
  presentation. The standalone OoT sword fallback remains available.

No model archive, skeleton dispatcher, jaw matrices, model transforms, scene,
save, grant, audio or weather behavior is edited. New geometry is not generated
in code. Native/Alt resource selection remains with the MM owner for spin and
OoT owner for imported boss souls.

## Checkpoint and verification

The initial implementation checkpoint is local commit `b6995da5`. The completed
candidate is separately committed on the branch above. The preserved baseline
remains recoverable by its exact tree; this POC is stacked on PR #31.

- Existing native MM item-visuals suite and full `z_draw.c` syntax pass.
- Extended real native limb-drawer fixtures pass for both engines, including
  the replacement flex jaw, rigid vanilla, repeated Alt changes, flame prim/env
  initialization, scroll, tint, RM scope and cleanup.
- The original baseline fails the new transparent inherited flame-state
  assertion (`--baseline-aura`), isolating that omitted initialization.
- Real-header spin GI checks pass for both engines: actual display lists, live
  burst hex, animated scroll, full alpha, XLU submission, grayscale/LUT state,
  matrix/segment/RM cleanup. The actual OoT alias callback passes, including
  dormant/invalid donor responses and fallback.
- Actual MM export -> OoT cache/latch and OoT imported-alias recipe -> MM
  cache/latch/dispatch checks pass with color edits after acquisition. Existing
  progressive tiers, all 16 dungeon palettes, 24 masks and four remains pass.
- The soul, spin, cache/latch and item presentation checks pass normally and
  under ASan/UBSan. Local leak detection is disabled because this container
  prevents LeakSanitizer's `/proc` inspection; no leak-proof claim is made.
- Asset collision and clang-format 14 checks pass. CI includes the new spin
  checks with both normal and sanitizer runs.
- Full application build and visual runtime verification are pending. This
  checkpoint is implemented work, not accepted or promoted gameplay.

## Existing MM spin packs

Inspected the actual archives below without changing them. All replace these
Alt resources used by the GI:

- `alt/objects/gameplay_keep/gGreatSpinAttackDiskDL`
- `alt/objects/gameplay_keep/gGreatSpinAttackCylinderDL`
- `alt/objects/gameplay_keep/gSpinAttack1Tex`
- `alt/objects/gameplay_keep/gSpinAttack2Tex`

| Pack | SHA-256 |
| --- | --- |
| `zzzz_MM_Spin_Fire_Reborn_HD_POC2.o2r` | `96543e561f2d59d38503c9225e2b28d001d1f44cdc022cbbe15df6d7a7ba83f1` |
| `zzzz_MM_Spin_Lightning_HD_POC1.o2r` | `e87da3e71eafee2998dfb421c1abec47b636c33dfea0d81761a1299b7386d091` |
| `zzzz_MM_Spin_Arcane_Level2_Circle_POC4.o2r` | `794741cfcbfa39d4307be93da5d00681d079c05cfb534e4bdc37d758cdbc99e1` |

Their replacement geometry and textures can apply with MM Alt enabled, even
when the MM GI is rendered in OoT. OoT-only spin packs use different resource
names; use the MM variants here. Asset-path compatibility is verified, while
the final visual appearance and shelf/acquisition scale remain unobserved.
Their disk/cylinder roots do not set a fixed primitive color, so the GI's live
Great Spin Burst color remains available to their materials.

## Decisive runtime check

Use the same Trading Post Volvagia soul and TP boss pack/load order. Confirm the
flame is visible while the jaw remains attached, during browsing and re-entry.
Compare Alt off/on and adjacent shelf items. Observe Great Spin in shop and
acquisition GIs, edit Great Spin Burst, then switch the MM spin mod/Alt state.
Repeat a foreign MM Great Spin GI in OoT using MM's owning pack and palette.

Recovery: retain the baseline tree above; the candidate diff is independently
revertible. No merge or master promotion is part of this checkpoint.
