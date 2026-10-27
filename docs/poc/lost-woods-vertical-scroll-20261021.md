# Dedicated Lost Woods vertical light-sheet binding

Baseline: `66bde021ccd651a3728db2338251961b1a44c9f7`. Candidate branch:
`fix/lost-woods-vertical-scroll-20261021`.

cor's current source-roundup request authorizes this narrow binding. The earlier
texture/UV workaround remains a separate, unaccepted archive candidate. This
change leaves that POC5 archive, all texture pixels, geometry, UVs and materials
untouched.

## Draw ownership and preservation

OoT's `func_8009EE44` owns scene draw configuration `SDC_LOST_WOODS` (9), used by
the native Lost Woods scene `0x5B`. It already binds translucent segment `08` to
horizontal 32x16 light-sheet motion and `09` to the native 32x32 water motion.
The new `COMBO_BUILD` path binds the otherwise unused translucent segment `0A`
to this independent profile:

| Tile | Logical dimensions | S phase/step | T phase/step |
| --- | --- | --- | --- |
| 0 | 32x16 | 0 / 0 | `gameplayFrames % 64` / +1 |
| 1 | 32x16 | 0 / 0 | `gameplayFrames % 64` / +1 |

The phases use native quarter-texel units, so 64 units wrap a 16-texel tile.
`Gfx_TwoTexScrollEx` allocates the current frame's 12-command list and supplies
native tile-size interpolation. Original `08`/`09`, opaque commands, environment
colors and Cojiro logic are unchanged. The normal build emits only its original
two segment bindings. Other scene draw owners and the private forest's separate
six-track material bindings are unchanged.

A read-only audit of the retained `oot.o2r` found 114 Lost Woods display lists
with three raw segmented calls: two to `08` and one to `09`; none to `0A`.
This supports the slot choice for native resources, without asserting that
arbitrary external mods also leave it unused.

## Authoring contract

An explicitly authored translucent sheet calls native address `0x0A000001`
after its own texture/tile setup and before drawing its triangles. Address bit
zero is required by the native interpreter. An XML resource may use
`<CallDisplayList Path=">0x0A000000" />`; the existing XML factory adds that bit.
Preserve the sheet's texture identities, UVs, tile descriptors, masks and render
mode. Its two logical scroll tiles are 32x16 regardless of replacement image
resolution. A 16-texel repeating T window is required for seamless wrapping.

This binding is available only when this draw configuration runs in a
Combo-enabled build. It does not infer motion for arbitrary segmented lists,
bind shared segments globally, or make every original-scene effect portable.
Existing sheets continue using their existing calls until separately authored
to select `0A`. The existing provenance-based Prelude native-material adapter
is independent of this slot and continues preserving its original profiles.

## Verification

`tests/lost_woods_entrance/run_vertical_scroll_tests.py` compiles the actual
production scene draw body, GBI wrappers, native scroll generator, segmented
address resolver and raw display-list dispatcher. Engine allocation, inventory
flags and SFX are explicit fixture boundaries. Raw arena display-list targets
use the production dispatcher; resource-name resolution is not exercised.

- RED: `python3 -B tests/lost_woods_entrance/run_vertical_scroll_tests.py
  --source-ref 66bde021` passes the normal-build control, then fails because the
  Combo draw has no dedicated vertical segment.
- GREEN: the same command without `--source-ref` passes both normal and Combo
  builds under ASan/UBSan. Fifteen frame values cover 63/64, 127/128, 2047/2048
  and integer-overflow boundaries. Every native command is compared, including
  both interpolated tile endpoints. Flagged calls and branches dispatch to the
  bound list; unflagged and unrelated unbound calls remain rejected.
- The fixture verifies exact original `08`/`09` and opaque commands, plus native
  and shuffled Cojiro behavior.
- The complete modified `soh/src/code/z_scene_table.c` compiles to a native object
  with real project headers using `scripts/diagnostics/compile_solo_forest_core.py
  --unit soh/src/code/z_scene_table.c`. Repeating its compiler command without
  `COMBO_BUILD` also passes. This is compilation, not full linking.
- Existing private-forest legacy six-track and native two-track scroll fixtures
  both pass. Removing only the new six-line block restores the exact baseline
  file bytes. The changed C lines pass clang-format 14 and `git diff --check`.

The focused runner requires initialized project headers and a C++20 compiler;
provide the normal nlohmann-json/spdlog include directories through the compiler
environment when they are outside the checkout. Evidence logs are retained with
the cumulative roundup recovery record.

Configured gameplay remains untested. The next decisive check uses a separately
authored `0A` sheet in OoT Lost Woods: inspect vertical direction, seam continuity,
camera and transparency under the actual mod stack, then leave/re-enter and
pause/unpause. Confirm original horizontal sheets and water still scroll as
before. No archive change or accepted visual-master promotion is implied.
