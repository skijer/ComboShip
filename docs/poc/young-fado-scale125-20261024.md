# Young Fado archive-owned 1.25 draw scale

The user requested a narrow uniform increase for young Fado matching the
absolute 1.25 scale already accepted in game for Anime Saria and young Malon.
The candidate uses the same optional archive scalar and existing flex traversal.
One source allowlist addition recognizes Fado's private skeleton; the archive
writer also accepts `--model young-fado`. The value stays in the archive rather
than in actor code. Existing builds ignore Fado's new resource, so this candidate
requires an executable rebuilt with the loader addition.

Base: `39ad19c9f4c737e388a09f14b5d7fe313e1ebad2` (continuous monkey calls,
inheriting the reviewed OoT inventory-page fix and recovered regional source).
Candidate branch: `poc/young-fado-scale125-20261024`. No public branch, develop,
master, accepted model pack or full review ZIP is changed.

## Immutable input and narrow output

Input collection: `ComboShip-NPC-Complete-Review-Set-Scale125-20261022.zip`,
Library identity `libfile_18557f816a408191817e140610a31e4a`, SHA256
`89b5c60c88cd3286d9bdb003c5ea1e0f35e96ef886a81ec544883fd5604da702`.
Use its latest `Common/Young_Fado_Shoulder_Lining_POC3.o2r`, SHA256
`f9d76f4fdd9be781377ee502e0a38a973653f7509c309645d7af92b638b0c6f9`.

Output: `Young_Fado_Shoulder_Lining_Scale125_20261024.o2r`, SHA256
`1f465322b395d02c2596ba876ec05288fe2d75acf05328340006b05d661101e3`.
It adds only `alt/objects/object_fa/YoungFadoNpcPOC1/DrawScale` (96 bytes).
All 174 original resource payloads remain byte-identical, including the shoulder
lining, pose fits, native timing, rig, geometry, face and texture resources.
The ZIP container is regenerated with deterministic metadata; raw ZIP records
are not claimed byte-identical. All other model archives remain untouched.

The scalar is absolute 1.25, 25% above Fado's former unscaled draw. It is
byte-identical to the accepted Saria/Malon 1.25 resource. Scale is applied once
at the existing incoming draw origin to root/limbs/props; it does not alter actor
collision, interaction distances, animation data or gameplay state.

## Recreate and verify

Extract the exact input member above, then use Python 3 from the candidate:

```sh
python3 scripts/npc_models/set_native_npc_draw_scale.py \
  --input /absolute/path/Young_Fado_Shoulder_Lining_POC3.o2r \
  --output /absolute/path/Young_Fado_Shoulder_Lining_Scale125_20261024.o2r \
  --model young-fado --scale 1.25
python3 -m unittest discover -s scripts/npc_models -p test_native_npc_draw_scale.py
python3 scripts/diagnostics/run_native_npc_model_tests.py
```

Archive tests first rejected `young-fado`; the production bridge first failed
its new 1.25 assertion against the old loader. Both pass after the narrow
addition. The native diagnostic suite passes model routing, the production
pose/resource bridge, animation sampling, four flex draw routes, tracking,
credits and compilation of eight native actors plus static Kokiri. The real
production resource bridge also passes ASan/UBSan with `detect_leaks=0` because
LeakSanitizer cannot inspect process tasks in this container. Clang-format 14
passes for the changed C++ header/test. Actual input/output CRCs, all 174 old
payloads and deterministic reconstruction pass; the per-resource hashes are in
the recoverable checkpoint's `evidence/fado-scale-verification.json`.

## Runtime status and installation

Source checks and native tests are complete. A fresh full application build and
configured runtime acceptance are pending. Replace the older young Fado pack
with this candidate in `mods/soh/`, with OoT Alternate Assets enabled and the
matching rebuilt executable. Do not stack both Young Fado packs. Check size,
feet/contact, seated and standing poses, talk/head tracking and live Alt toggle
in the intended scenes/load order before accepting it. Roll back by restoring
the original shoulder-lining pack. Adult Fado retains her separate pack/scale.
