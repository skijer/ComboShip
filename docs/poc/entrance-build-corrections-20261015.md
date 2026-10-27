# Entrance candidate: Windows compile and Linux module-load corrections

Baseline: PR #43, `092ece59657b78a7c2f623e9f462c1ff5ed680bb`, source tree
`9a69bb9732a495b0d5065eb25e1836da2e0837da`. The local recovery checkpoint
`0ec101587a4142a57f894a76be54322f4c228f1e` has that identical tree.

## Failures traced

Build Artifacts run `38815432503` passed the complete regression gate.
Its Windows job `116711485872` failed building SoH: MSVC C3878 in
`SkipBlueWarp.cpp` at the `REGISTER_VB_SHOULD` invocation. Conditional
preprocessor directives were embedded in a macro argument. The existing GCC
build accepted this nonportable construct.

The Linux job `116711485829` compiled and linked both games and the launchers,
and generated both port archives, then failed the `ci-linux-smoke` seed check
because the game modules could not load. A local native relink reproduced
`lib2ship.so: undefined symbol: _ZN12_GLOBAL__N_135FrameInterpolation_RecordCloseChildEv`.
The MM passage object's display macros declared interpolation functions in
an anonymous namespace, requesting private C++ names instead of the native C
symbols. The open-child function had the same defect.

## Bounded corrections and preservation

- Move the unchanged blue-warp routing query into a file-local helper, outside
  the hook macro argument. Preserve standalone behavior, native dungeon/boss
  shuffles, ComboShip routing, skip settings and boss-rush/credits exclusions.
- Include MM's canonical interpolation header and put the file-local passage
  draw function at global namespace scope. Its state, geometry, materials,
  transforms, scene guards and hook registration retain their existing behavior.
- Add the production-hook portability/behavior regression to the cross-dungeon
  runner and the actual passage-object ABI regression to the forest runner.

The forest handoff, cross-game entrance/boss/interior/grotto implementation and
Hyrule Field cold-Alt texture/display-list repair remain included. No scene
archive, texture, mesh, collision or accepted develop/master baseline changes.

## Verification

- Before correction, the real hook macro fails strict preprocessing with
  "embedding a directive within macro arguments is not portable". Afterward,
  standalone and ComboShip each pass 34 production-hook behavior cases, both
  normally and under ASan/UBSan.
- Before correction, the actual passage-object test finds mangled anonymous
  interpolation symbols. Afterward it requests both exact native C names.
- The complete cross-dungeon suite passes with ASan/UBSan; the complete forest
  boot, arrival, handoff/lifetime and renderer-linkage suite passes.
- The corrected SoH blue-warp and MM passage translation units compile against
  real engine/dependency headers. Local native relinks load both modules with
  `RTLD_NOW | RTLD_GLOBAL`, and `comborando --seed ci-linux-smoke --count 1`
  exits successfully. The relink reuses preserved native objects for untouched
  sources; it is separate from a fresh hosted platform build.
- Changed-source clang-format 14, Python syntax and whitespace checks pass.

Hosted Windows/Linux package results belong to the follow-up run. In-game
forest appearance/accessibility and the installed Native/Alt Field pack-stack
checks remain pending. This is a build-verified candidate, not runtime acceptance
or develop/master promotion.

## Fail early in later runs

The artifact gate runs the production-hook portability/behavior and MM
passage-object ABI checks immediately after checkout, before installing
dependencies or running the long regression suites. Both checks run with the
runner's compiler, Python and repository headers; no configured game build is
required. The Windows pipeline also compiles and executes the same standalone
and ComboShip hook cases with MSVC before CMake configuration or dependency
builds, including release calls of the reusable workflow. Existing regression,
full-build and seed-generation checks remain enabled.
