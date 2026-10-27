# Black Market keeper lean POC12

| Field | Record |
|---|---|
| Source | `zzzz_Skiejer_Black_Market_POC11.o2r`, SHA256 `d439e88073d6f8d5230d4ccff1aa744824be6b60975ac8469e1ba9fde9c346c2` |
| Source checkpoint | Candidate branch `poc/black-market-cross-lock-20261017`, merged native forest base `3128176d` |
| Candidate | `zzzz_Skiejer_Black_Market_POC12_KeeperLean.o2r`, SHA256 `761fa811149f860b6a174901bdda1ad7fdf5610a0064f0e3efe5927853bb1352` |
| Hypothesis | The retained native Sway animation supplies the requested forward lean with crossed arms. Its feet can remain above the actual rear floor while its upper body extends over Granny's counter. |
| Scope | One shared keeper animation/placement contract, native asset decoder, isolated room carrier placement patch, reproducible recipe and geometry checks. Native controller consumption is companion work. |
| Preservation | Original POC11 untouched; 190 of 192 entry payloads byte-identical. Skeleton, both animations, every model/room vertex, textures, blink heads, collision, materials, cosmetics and OoT native scene/room headers are preserved. |
| Status | Implemented and offline geometry/static verified. Configured gameplay, Alt parity and user acceptance remain untested; no master promotion or full game build. |

## Native pose identification

Both packed animations are normal native 32-frame resources with 24 joint
entries for the same 23-limb flex skeleton and the same initial root translation
`(4, 3199, 0)`. The selected path is
`custom/skiejer_market/keeper/ShopGalMMDLevelSwayAnim`; its SHA256 is
`b53fe2fef2d0db1abfc19c2e6d83800fb1168750c4527818d7b17f6a7c7eb837`.
The preserved Idle resource SHA256 is
`a1cee344479c4b0956ea981e9a0ab6f51cbc6a92d9ca001206545eb8aafab13b`.

Identification uses the actual decoded limb transformations and mesh, rather
than the filenames. In all 32 Sway frames the wrists stay within six world
units of each other, at least twelve units forward of their shoulders. Idle
frame 0 instead spreads its wrists across about thirty world units. Front and
side inspection of the actual mesh confirms that Sway bends the torso forward
and crosses the forearms. The regression was first run against the previous
Idle default and failed at frame 0 before the decoder selection changed.

`combo/BlackMarketKeeper.h` defines the common native/preview contract:

| Constant | Value |
|---|---|
| `kAnimation` | Aligned `__OTR__custom/skiejer_market/keeper/ShopGalMMDLevelSwayAnim` |
| `kX, kY, kZ` | `0, 1, 130` |
| `kYaw` | `-32768` |
| `kScale` | `0.01` |

The original model scale and native animation root are retained. The one-unit
upward placement corrects a 0.613-unit sole penetration that Sway would have at
the old Y=0 spawn over the rear floor at Y=1. The crossed arms remain above the
counter top; they are not forced onto it by lowering the character.

## Geometry evidence

`NativeKeeper.position_triangles` follows native vertex-cache ownership: every
vertex uses the flex matrix active when its native vertex-load command ran.
Its positions are compared with the existing complete preview decoder for
frames 0, 15 and 31 and all three blink heads, with absolute tolerance `1e-5`.

The full loop checks all 16,304 triangles in each of 32 native integer frames
(521,728 triangle poses). Combined world bounds are
`[-7.74424794, 1.38706697, 100.88932545]` to
`[7.48753883, 60.82693905, 138.41741047]`.

- The actual rear floor is at Y=1. Minimum sole clearance is `0.38706697`.
- All 8,109 checked low-vertex XZ projections lie within actual rear-floor
  triangles, rather than merely within the floor's bounding box.
- The actual counter top consists of seven triangles at Y=30 spanning
  X `[-51, 30]`, Z `[93, 123]`. Every keeper triangle is either above the
  counter's full enclosing box or behind its rear Z=123 plane. The minimum
  clearance for triangles crossing the counter's vertical range is
  `4.14136228`. This also excludes triangle interiors from the counter volume.

These are CPU decoded geometry checks. They do not simulate fixed-point matrix
rounding or prove camera visibility, lighting, transparency, interaction, game
transitions, alternate-asset routing or runtime acceptance.

## Isolated pack delta and reproduction

Only these payloads change:

1. `scenes/nonmq/skiejer_market/skiejer_market_room_0`: the MM keeper carrier
   moves from `(0,0,155)` to `(0,1,130)`. Actor identity, rotations, parameters,
   object list and the rest of the room bytes are retained.
2. `custom/skiejer_market/manifest.json`: records the shared keeper contract.

The OoT room keeps its empty actor list. Its dedicated native keeper controller
must consume the same shared constants. Selecting Sway requires no animation
payload edits or additional resource paths.

Run from the repository root with a new output/report destination:

```sh
python tools/black_market/keeper_lean.py \
  --source ../restored/ComboShip-Black-Market-Polish-Source-8ccbd47d/assets/zzzz_Skiejer_Black_Market_POC11.o2r \
  --output ../market-keeper-candidate/zzzz_Skiejer_Black_Market_POC12_KeeperLean.o2r \
  --report ../market-keeper-candidate/keeper-lean-report.json
python tests/black_market/check_keeper_lean.py \
  --pack ../restored/ComboShip-Black-Market-Polish-Source-8ccbd47d/assets/zzzz_Skiejer_Black_Market_POC11.o2r
```

The patcher refuses an unexpected baseline, existing outputs and aliased paths.
The regression independently creates two candidates, compares their complete
ZIP bytes, compares every unaffected entry payload and rechecks the source
hash. The build recipe emits the same native room placement and manifest field.
Retained asset and room-polish checks pass against this candidate: 192 private
resources, 28 scrolling material bindings, all 7,819 drawn collision faces,
seven boundary repair faces and compatible native OoT scene headers. The shared
header also passes a standalone C++17 syntax/static-constant check.

The decoder's default animation, position, yaw and scale read the shared
header. Explicit actual-room placements remain supported for baseline/control
inspection. A regression caught the previous default position before it was
updated, and another caught an output/report path alias before the patcher
added explicit pre-write path guards.

Recovery is the unchanged POC11 source plus the bounded source commit containing
the header, patcher, decoder/build updates and regression. Remove the candidate
pack and revert those paths to discard this experiment. Keep the companion
native consumer wiring consistent with whichever contract is retained.
