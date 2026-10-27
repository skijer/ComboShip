# Termina winter, Cape appearance and fairy glass follow-up

| Field | Record |
| --- | --- |
| Published parent | Draft PR #41, `bridge/audit-merge-20261008`, `87e953618f6851316d6036718009d336c685f5e0` |
| Candidate | `poc/termina-winter-frost-20261011`; preserves the published Fados Ikana correction and the earlier cumulative fixes |
| Winter checkpoint | `a4bce6387d725b45470bcbb7a268b0e317b63b7c`, originally tested on `aa0acc3f`; merged with the newer published parent before these corrections |
| User evidence | Termina winter remained green; Clock Town winter grass and Termina autumn were accepted. Hiding the Cape dimmed its equipment icon. The accepted fairy bottle casing was faint when viewed on a phone. |
| Scope | Termina winter foliage, owned Cape icon alpha in both hosts, bundled fairy bottle glass in both hosts |
| Preservation | Autumn and other scenes' winter treatment; Cape ownership, magic savings and Show/Hide handling; Pendant toggles; fairy motion, size, glow and motes; common bottle materials and geometry; selected replacement routing |
| Recovery | Original inputs and published parent remain intact. Push updates the draft cumulative candidate; no develop/master promotion or runtime acceptance is implied. |

## Resulting behavior

Termina uses a full cool-white winter palette with a 1.8 mean-RGB lift. Native
grass, bushes, grouped grass and tree canopies now receive winter treatment in
Termina. Trunks, cut stumps, alpha and reversible resource ownership retain
their original behavior. [The winter record](termina-winter-frost-20261011.md)
contains the exact material scope and regression evidence.

The Cape icon keeps the full pause-page alpha while owned, whether its cloth is
shown or hidden. The existing popup preselects Show/Hide from the saved appearance
flag. Hiding changes appearance only; magic savings already depend on ownership.
The Pendant still dims when its separate gameplay effect is inactive.

Bundled fairy glass uses white primitive alpha `80/255` (31.4%), increased from
`32/255` (12.5%). `FairyGlassMaterial` calls the existing reflective material,
then applies the new alpha before the existing glass triangles. `FairyXlu`
retains the original shell submission order and cleanup. Only two resources per
host are added; all 29 existing bottle resources remain byte-identical.

Only the native fairy-shell selectors choose the new glass. Empty/general bottle
recipes remain exact. Missing new assets fall back to the accepted empty-bottle
glass. Selected direct casing passes, shared shell roots and shared glass
materials retain their original recipes. Texture/vertex replacements continue
through the shared children. The existing generator reproduces both host trees,
and the established fairy test invocation includes the rebuild guard.

## Evidence and verification

- The Cape draw regression failed with 48 hidden-owned alpha cases per host on
  the original production code. The corrected real-command boundary passes
  2,016 MM and 2,464 SoH checks, including page fade, absent ownership/icons,
  saved-state immutability and Pendant controls.
- Existing Cape receipt/choice, equipment selector, MM ownership (480 checks)
  and OoT access (111 checks) suites pass normally and under ASan/UBSan.
- The combined winter tree passes all 538 frozen scene materials in both seasons,
  real POC4 atlas controls and 16,384 renderer-state cases. The original winter
  checkpoint also passed address/undefined sanitizer coverage.
- The new fairy material regression first failed at the submitted glass
  triangles' original alpha 32. The corrected native and foreign routes submit
  alpha 80 while preserving geometry, other material state and cleanup. Exact
  source hashes and deterministic asset rebuilds pass for both owners.
- Review separately reproduced a selected shared shell being bypassed and a
  selected glass material being recolored. The revised guard preserves the
  selected `24/160/210/144` RGBA across six native/foreign routes; texture and
  vertex-only replacements retain the stronger glass.
- The complete fairy invocation passes normally and under ASan/UBSan, including
  both engines' real C++ header checks. Both complete native C draw translation
  units also pass syntax checks. Local dependencies use real official spdlog
  `v1.12.0` and SDL2 `release-2.30.0` headers, plus the existing JSON headers.
- Bottle GI, polish and the complete contents/composer suite pass. The latter
  checks both C++ composers and native C draw/export translation units using
  the same real SDK chain plus the CI-pinned thread pool `v4.1.0` headers.
- Independent winter, Cape and final glass reviews report no remaining findings.
- Clang-format 14's full changed-source dry run, staged asset collisions, changed
  file conflict scan and diff checks pass. No new differing-content collisions
  are introduced.

These are source, syntax and fixture checks. The full linked Windows/Linux build
and configuration-specific in-game visual acceptance are separate. Local ASan
checks disable leak detection because this container blocks process inspection.

## Runtime check

Use this push's executable with its matching `soh.o2r` and `2ship.o2r`; earlier
packages do not contain the new fairy-glass resources. At the accepted Termina
view, inspect winter and cycle winter → autumn → Off, then re-enter. Confirm
Clock Town's accepted winter grass remains unchanged. For an owned Cape, choose
Hide and confirm the icon stays solid while the cloth disappears; Show restores
the cloth. Display the fairy bottle and judge the casing around its moving pink
fairy, including a phone-sized view. Keep the existing Fados mod installed.
