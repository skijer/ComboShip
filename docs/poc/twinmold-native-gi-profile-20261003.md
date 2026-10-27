# Twinmold native GI parity

Parent candidate: Barinade `2d6b68e` plus native-effects correction `3c52758`.
No remote mutation or promotion.

The fourth native MM boss soul now exports `object_boss02/gTwinmoldHeadSkel`
and `gTwinmoldHeadFlyAnim`, scale 0.06, blue skin on OPA segment 8, and the
native yellow soul flame (168,180,20), size (3,3,3). Twinmold has zero world Y
translation; the other three native MM boss GI recipes retain Y=-20.

This matches native `DrawTwinmold`: the head and its animated appendages.
The separate actor body rig is not part of that native GI. No full worm body,
new body motion or remapped custom limbs are introduced.

An appended procedural profile identifies Twinmold's native segment-13 backing.
The canonical loaded head is a normal 12-limb skeleton. Its 23 backing matrices
are all initialized to the current model transform before submission, rather
than leaving native allocator bytes exposed. A selected flex replacement uses
the actual engine flex traversal and its populated matrix array. Both routes
restore segment 13; texture/flame segments are also restored. The unused,
unpublished Barinade custom-suppression flag was removed from the ABI.

MM owns skeleton, animation and blue skin selection, including Alt and fallback.
Native ComboShip MM `DrawTwinmold` calls a small shared consumer helper using
MM's real display-name lookup, so native and imported copies follow the same
selected skeleton kind. The helper restores its model matrix on success or
failure. Standalone MM keeps its existing native fallback body. MM-host/MM-owner copies call the actual native soul flame body, including its
shared once-per-frame phase; matrix, RM bracket, segment and color cleanup stay
inside that effect. The accepted MM-host/OoT-owner native effect branch is
unchanged. OoT-host/MM-owner flames retain the ABI scroll rates. Flame rendering
remains independent of selected model display lists.

Verification:

- The production recipe test first failed because Twinmold lacked an animated
  export. It passes with native paths, scale, texture, translation and flame.
- The complete production foreign consumer runs against both native engines'
  actual limb traversal. Repeated native/flex Alt selection checks 23 initialized
  rigid backing matrices or the generated flex array, all selected authored
  limbs, MM-owner skin binding during recording, yellow flame, cleanup,
  incompatible-count/invalid-recipe fallback, and owning-RM restoration.
- The production native helper and `DrawTwinmold` body execute in the same
  fixture, including successful dispatch and fallback. Old initialization and
  fallback-flame primitives are fixture boundaries; the new helper and shared
  consumer are actual production bodies.
- `run_barinade_syntax_tests.py` now also compiles Twinmold matrix initialization
  against both actual engine headers and its native helper against MM's actual
  StaticData declaration. Matrix_ToMtx host prototypes are checked.
- Normal and ASan/UBSan production suites pass. Leak detection is disabled for
  this environment's inaccessible `/proc` inspection.

Resource loading, animation sampling and matrix/GBI primitives are modeled test
boundaries. Full application build and installed-pack pixels remain unverified.
Runtime acceptance needs native/imported Twinmold GI with the user's actual
archives/load order, Alt off/on, appendage deformation, native blue skin and
soul flame, adjacent items and re-entry. No archive mapping beyond canonical
selected-owner paths is asserted.
