# Termina Field autumn wallpaper and canopy candidate

| Field | Record |
| --- | --- |
| Source baseline | PR #41, `bridge/audit-merge-20261008`, exact source `c0181856968dd6f3869be0d1aa240743091fd6a8`. The user reports the private Djipi brush tint now works. |
| Candidate | Isolated `poc/autumn-private-forest-20261009`; scope the existing forest copper-red `#B96848` around the exact forest-only `room_00DL_0241B0` owner and its nine archived setup aliases. |
| Scope | Wallpaper and upper forest canopy ownership, scoped-command lifetime, and a regression in the existing NEI gate. |
| Preservation | Retain the brush material registrations, ochre terrain, geometry, textures, UVs, alpha, culling, collision, scene headers, actors, weather and audio. Original archives remain unchanged. |
| Evidence | Read the earlier material proof, current source, and the native forest commands and routing audit in `Termina_Field_3DS_POC4_Checkpoint.zip`. Native forest registrations already pass. |
| Verdict | Implemented and tested with production transformation/lifecycle and actual MM/GBI headers; GPU appearance and the active replacement pack remain untested. No master promotion. |
| Recovery | Revert the isolated candidate or keep using the unchanged baseline. |

The previous material proof confirmed `Z2_00KEIKOKUTex_034098` and
`Z2_00KEIKOKUTex_037098` against a native forest draw. The replacement brush
used actual replacement commands; the replacement forest did not. Both forest
texture identities were already registered, so repeating their registration
could not correct a private binding. There was no draw-owner fallback for the
forest, unlike the earlier brush roots. A partly recognized list would also
bypass an empty-variant fallback and leave its other private layer untouched.

The checkpoint identifies `Z2_00KEIKOKU_room_00DL_0241B0` as the exclusive
owner of the lower wallpaper (14 triangles) and upper canopy (8 triangles).
The candidate prefers a scoped wrapper for that exact family, so both layers
receive `#B96848` with native, private, or mixed texture bindings. It matches
normal and `_scene` namespaces, Alt resources, the base room and the nine
archived setup names. Unknown setups, other rooms and neighboring draws remain
outside this owner rule. Other scene lists retain their existing material
selection and brush fallback behavior.

The wrapper executes a separate command copy. The real interpreter's
`gfx_set_timg_otr_hash_handler_custom` writes resolved image pointers into the
executed command's `w1`. Executing the immutable comparison snapshot would
therefore make later `Variant::Apply` calls reject an otherwise unchanged
source. Keeping the snapshot separate preserves repeated updates, Off
restoration, queued command lifetime and later external-patch detection.

The committed 60-word fixture is copied from the checkpoint's
`work/native_field_sources.zip`; the original serialized entry SHA-256 is
`c7d90c85d5997d7f3b3fe1e2e24c506c3c90a3bb7670e6833b3bae0c79b155d3`.
The new regression fails on the source baseline because both private forest
layers receive no tint. It passes for 160 combinations: four native/private
binding combinations, two scene namespaces, two Alt states and ten setups.
It verifies the exact source commands, scoped color and reset, simulated real
interpreter pointer writeback, 100 steady updates without extra loads or
allocations, immediate Off restoration, reactivation and teardown. Private
fixture names are deliberately synthetic; they are not claimed as the names
of the user's active pack.

Verification commands from the repository root:

```sh
python3 -B scripts/diagnostics/run_mm_autumn_scene_foliage_tests.py
ASAN_OPTIONS=detect_leaks=0 MM_FOLIAGE_TEST_CXXFLAGS='-O1 -g -fsanitize=address,undefined,bounds -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie' python3 -B scripts/diagnostics/run_mm_autumn_scene_foliage_tests.py --mm /path/to/native_field_sources.zip
python3 -B scripts/diagnostics/run_mm_autumn_foliage_tests.py
python3 -B scripts/diagnostics/run_mm_autumn_grass_tests.py
python3 -B scripts/diagnostics/run_mm_weather_tests.py
ASAN_OPTIONS=detect_leaks=0 python3 -B tests/seasons/run_native_weather_tests.py --sanitize
python3 scripts/check-asset-collisions.py
bash -n scripts/diagnostics/run_combo_nei_regressions.sh
git diff --check
```

These focused checks and the changed-source clang-format-14 check pass.
The grass real-header check retains 42 existing
controller-button macro redefinition warnings. The scene-material test runs
in the existing NEI regression command after the tree/leaf check. No full
application build, GPU execution, remote CI run or whole-repository test run
is claimed.

The active private forest binding and executable were not supplied this turn,
so the runtime cause is not conclusively pinned to a particular private path.
This candidate corrects the demonstrated owner-coverage gap without guessing
one. Test the candidate executable with the same Termina Field setup, installed
packs, mount order, lighting and Alt state: both marked forest layers should
take the copper-red palette alongside the already-working brush. Select Off,
toggle Alt and re-enter to verify restoration and retention of the brush win.
