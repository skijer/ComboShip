# Adult MM flex matrix slot repair — 2026-10-24

| Field | Record |
|---|---|
| Baseline | Accepted cumulative tree `545292a6` (public `5f8966fa`, local `86387591`); source investigation started at candidate `1376ea0cbfed2f529dc07d7e9ef4e1066706e91d` on `poc/regional-runtime-repair-20261024`. Earlier core/spin/D-pad changes remain separate. |
| Candidate | `adult-mm-flex-slots-20261025`; one adult base-model setup entry plus an obsolete comment. |
| Report | User reports adult Link vertices stretching in MM Lost Woods and the black market. No adult-specific clip or active Hide Back Equipment configuration was supplied. Earlier spin clips show OoT child Link and are not evidence for this report. |
| Hypothesis | Clearing the source sheath DL drops its reserved flex matrix when the final equipment policy hides the sheath, leaving the torso matrix slot unreadied. |
| Preservation | Native equipment selection and visibility; adult hands/sword/shield; full Alt model precedence; MM child and custom forms; original and combined game guards; accepted scene assets, placements, collision, title/viewer work. No asset archive, save, render budget, skeleton walker, or shared native override is modified. |
| Evidence | Actual native producer/consumer RED/GREEN detailed below. No configured game runtime or user acceptance is claimed. |
| Recovery | Revert the one adult setup entry to `NULL` to reproduce the old defect. Original inputs remain unchanged. Parent owns source checkpoints and cumulative candidate packaging. |

## Confirmed configuration-specific cause

`AdultLink_Setup` deep-patches the base OoT adult skeleton but had cleared source limb 19 (sheath) to `NULL`. The final adult limb override chooses an equipment mesh there. `gEnhancements.HideBackEquipment=1` subsequently clears that final mesh.

MM's real `SkelAnime_DrawFlexLimbLod` deliberately emits a matrix when either the final DL or the original source DL is non-null. This preserves canonical matrix indices for hidden geometry. With both null, the sheath slot disappears: only 17 of the skeleton's 18 matrices are written, and the torso moves from slot 17 to slot 16.

The supplied `oot.o2r` head and both shoulder display lists load segmented address `0x0D000441`. The interpreter treats the low bit as the segmented-address tag, so this is offset `0x440`, matrix slot 17 at 64 bytes per matrix. Those meshes then read an unwritten matrix. No render-budget inference is involved.

The fix loads the empty adult sheath DL as the source limb. The same final override still chooses or hides its rendered equipment. If hidden, the native walker now retains the sheath matrix and leaves the torso at slot 17.

The source object has no path overlap with MM's `object_link_boy` resources. The inspected native post-limb/mask path binds segments `0x0A` and `0x0B`, not `0x0D`; the cape uses body-part positions and does not rebind `0x0D`. The supplied log does not establish the user's active visibility CVar. This repair therefore proves a real mechanism that can produce the reported symptom, while the precise reported scene configuration remains unverified.

## Native verification

Run:

```sh
python -B tests/adult_mm_flex/run_tests.py --oot-archive /path/to/oot.o2r
```

Without `--oot-archive`, the same test uses checked metadata containing only the real skeleton topology, matrix addresses, and resource hashes. The archive check explicitly verifies the selected empty sheath, adult equipment variants, and both source LODs as display-list resources. Fixture loaders reject unknown or missing DL paths and incorrect skeleton paths. No vertices, textures, save contents, or user inventory are embedded.

The fixture compiles the actual MM `z_skelanime.c`, `sys_matrix.c`, and `gu_pc.c`. It executes the unchanged production adult setup, loader, complete final adult override, native visibility policy, and graphics wrappers. It reads the emitted segment-`0x0D` command, tracks actual `Matrix_ToMtx` destinations, and checks them against the archive's 24 matrix reads. Resource loading, pose/IK, optional equipment effects, and interpolation recording are controlled boundaries; this is not a full game draw or interpolation proof.

Before the fix, the test exits 1. Representative failure:

```text
FAIL adult flex alt=0 mod=0 hideBack=1 mirror=0 lod=0 sword=0 hand=0 shield=0: wrote 17/18 matrices; canonical torso slot 17 writer=0 (expected limb 21)
FAIL objects/object_link_boy/gLinkAdultHeadNearDL reads unwritten segment-0D offset 0x440
FAIL objects/object_link_boy/gLinkAdultLeftShoulderNearDL reads unwritten segment-0D offset 0x440
FAIL objects/object_link_boy/gLinkAdultRightShoulderNearDL reads unwritten segment-0D offset 0x440
```

Visible-back controls and a complete alternate-skeleton control pass before the fix. After the fix, all 432 frames pass across base/Alt fallback/full alternate skeleton, both LOD inputs, positive/negative actor-scale flags, hidden/visible back equipment, three left-hand types, sword equipped/absent, and all native shield choices. Actual alternate model geometry is outside this metadata fixture.

Also verified:

- Full native compilation of `mm/mods/items/logic/adult_link_render.cpp` succeeds.
- `scripts/diagnostics/run_mm_back_equipment_tests.py` passes.
- `tests/mm_nei/run_hand_selector_tests.py` passes; it emits existing controller-button macro redefinition warnings.
- `tests/nei_held/run_four_sword_visibility_tests.py` passes.
- `tests/mm_cape/run_visibility_tests.py` passes all OoT/MM draw/resume cases.

Input identities, read-only:

| Input | SHA-256 |
|---|---|
| `oot.o2r` | `ea80d61b223ee38075fe5383b652998f3b08164e903134922f63f205d995717b` |
| `mm.o2r` | `f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59` |

## Remaining runtime check

With the cumulative candidate and the user's actual packs/load order, enter MM Lost Woods and the MM black market as adult human Link. Record Alt and Hide Back Equipment settings. Compare stationary/moving/sword drawn/shield raised with back visibility on and off, then re-enter. This establishes whether the same mechanism explains the user's current stretch report. Runtime result, 60-fps interpolation, unrelated alternate models, and acceptance remain untested.
