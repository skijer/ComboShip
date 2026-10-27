# Regional runtime repair candidate

Baseline public source is `5f8966fae4b0810ce86021b3f88814ffd77e0224`,
tree `545292a667083fb835ccd86b078e3aac7a484cd1`. Local recovery parent
`86387591cb6fba0adb2528c258c8dadd624136c1` has the same tree. The supplied
Windows log identifies CI merge checkout `da79ceb3faf5dba987283b537e7000fc2744bdd4`,
also the same tree. This is a repair of the tested cumulative build. The user
accepted Ikana traversal. Develop/master remain unmerged. Native checks and
binary/dump reconstruction are separate from configured gameplay acceptance.

## Lake Hylia: exact second-crash reconstruction

Both current-session crashes occur in Lake Hylia after Mikau resources load.
Both log nine unhandled `0xA9` commands and invalid shader registration. The
first faults in vertex dispatch; the supplied second dump faults in shader
dispatch. The matching Windows DLLs and second dump allow an exact trace:

1. The interpreter's segment C points at OoT's two-command `gCullFrontDList`.
2. Twenty-three calls in the original Mikau display lists use `G_DL_INDEX` with
   segment C, index 2. The ordinary MM actor establishes a four-entry render-mode
   table before drawing. Its OoT catalogue adapter omitted that binding.
3. Index 2 of the inherited two-command OoT table starts after its terminator,
   so the interpreter walks adjacent DLL globals. The relocated pointer pairs
   produce the nine `0xA9` errors. An intervening hash-movemem command consumes
   one pair as data. Two nearby float records decode as shader commands.
4. The second bogus shader operand is `0x00000fcffeffffff`. The actual DLL's
   `SegAddr` code treats its low bit as a segmented-address tag and calculates
   an out-of-range segment-table read. Its computed address
   `0x000001e71f6ab950` exactly equals the dump's exception address.

`scripts/diagnostics/trace_lake_render_dump.py` independently checks DLL
identities, derives the native segment-table offset, reads the dump's segment C,
reconstructs relocated DLL words, verifies all nine errors and matches the
exception address. It reads the original inputs without modifying them. No game
archive, DLL, log or dump is included in public source. The exact first-crash
instruction sequence cannot be reconstructed without its own dump.

The correction explicitly binds the retained four-entry MM opaque table before
both Mikau poses in OoT. The native manually placed MM path also establishes
its normal opaque table. It does not suppress an opcode, disable the cave or
remove rendering. A regression fails on the old missing binding and passes
with the actual production draw callback. The added actual CaveDraw/native GBI
fixture also checks vertex lifetimes and command validity for native/replacement
materials and full/drained/refilled water states. That test alone would not
have detected this actor fault.

## Snowhead visibility and Graveyard titles

The native hash-vertex interpreter replaces offsets with cached vertex pointers
in the display list. The valley guard previously accepted offsets only, so a
backdrop already rendered by the time the player update ran could disable the
entire authored endpoint. The guard now accepts fresh offsets or exact pointers
into the same vertex resource. The hash, opcode, triangle signature and original
vertex coordinates remain validated. Foreign resources remain rejected and
restoration still edits only the eight intended low backdrop triangles. The
regression executes the production renderer's in-place cache mutation before
checking the endpoint.

An OoT regional return uses a covered setup load to resolve the authored
endpoint, then a native positioned return. The setup load displayed a title over
black; the final return displayed it again. Player initialization now suppresses
the covered load's card while consuming that request. The final native return
still requests its destination title once. The fixture executes native title
selection/lifecycle and the real endpoint staging state, including all three
regional scenes and standalone OoT parity.

Road to Ikana has native title-text ID zero. No new card is invented. The expanded
MM title fixture composes the actual regional arrival and preserves that
untitled destination while Great Bay and Mountain Village retain their native
cards. An untitled request is still consumed once.

## Manual placement and posed focus

The user now places cutscene catalogue actors in Prelude. The automatic OoT
regional actor hooks and spawning logic are removed. The compatibility update
entry point has no effect; authored actors and the catalogue remain available.

All nine new regional poses use their actual posed bone matrix for focus:
Stalchild head limb11, Mikau limb9 with its native head offset, and Darmani upper
body limb4. Initial focus is only a fallback until the first draw. Updates no
longer overwrite the resulting point with the fixed standing height. This is
implemented in both the OoT catalogue and manual native MM presentation paths.
The MM placements also retain attention eligibility.

The original donor's seated Stalchild head is at Y15.43–17.08 across both seated
clips; the previous focus was Y55. The tests execute the actual callback and
native matrix multiply over all244 animation frames in the nine clips, plus
both opaque/translucent callback signatures, prop attachment, update retention,
and manual-actor preservation. Arbitrary replacement rigs and configured camera/
Navi appearance still require gameplay verification.

## Verification status

The source evidence includes RED/GREEN binding and focus regressions, native
OoT/MM title tests with sanitizers, native valley resource lifecycle/cache tests,
actual Lake draw/GBI/collision fixtures, all nine manual actor/resource paths and
all244 donor focus frames. Full candidate CI and configured gameplay are tracked
separately at handoff. The earlier failed shop experiment is excluded.
