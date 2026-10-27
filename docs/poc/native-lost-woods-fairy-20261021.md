# Native Lost Woods Great Fairy tunnel

Final integration parent: PR #53 source head
`724e0476b1dd2555f40faf7cc01b8d1cf0329ecd`, tree
`f4d2092d09a511d030de1b5007188a9e7849b883`, retaining the separately verified
Black Market collision and Saria receipt/cutscene-dialogue corrections.
The initial diagnosis and full entrance run used `590376a6a53b612aa84c6b1f6203571e064e7d23`,
tree `a6181b130466f21d72af59ed9e707bf59375b863`; the intervening Saria change
does not modify this controller, its tests, or any entrance implementation.
Candidate branch: `fix/native-fairy-tunnel-20261021`.

cor reported that the fresh ComboShip build with no mounted mods passed the
other tunnel smoke tests, but room 7's straight/north tunnel still returned to
Kokiri Forest. The previous controller only recognized a Great Fairy entrance
already selected by an authored collision exit. Its conditional return was in
source; native access still depended on a scene export.

## Bounded change

Handle the existing native collision exit before entrance shuffle. Only Lost
Woods room 7's north exit floor, selecting `ENTR_KOKIRI_FOREST_UPPER_EXIT`,
is redirected to `ENTR_GREAT_FAIRYS_FOUNTAIN_MAGIC_DMT` (`0x315`). The normal
fountain exit then uses the existing visit-scoped return to the same room and
grounded approach. A debug warp directly onto the exit floor can use the known
dry native approach `(1600, 0, -2760)`, facing south into room 7, without prior
sampled frames. Saving/reloading inside retains the local visit.

No scene archive, optional texture pack, new collision, geometry or proximity
trigger is required. Authored exits with an explicit Great Fairy destination
retain their selected native fountain and sampled local return. Other authored
destinations retain their own route.

Protected: room 7's right/east Woodfall route, left room-8 connection, room 1's
former pink private-forest route, original fountain entries/exits and entrance
shuffle outside this local passage, both ages, OoT-only/combined modes, original
non-Combo compilation, MM mode exclusion, Farore/paired-arrival priority,
inventory/progression, and respawn preservation. Other bugfix sources and all
asset/workflow files remain inherited from the exact baseline.

## Native input evidence

The read-only native OoT archive has SHA-256
`ea80d61b223ee38075fe5383b652998f3b08164e903134922f63f205d995717b`.
Its resource `scenes/shared/spot10_scene/spot10_sceneCollisionHeader_00AC98`
has SHA-256
`1db0b64ed8c566d8bb4e810fe2c2a6a29e065833afe04236ae68e60a920acc1d`.

- North approach polygons 1722/1723: X1540..1660, Z-2600..-2800, Y0,
  scene exit 0 (ordinary floor).
- North exit polygon 1730: `(1540,0,-2800)`, `(1660,0,-2800)`,
  `(1600,0,-3200)`, scene exit 1, selecting exit-list slot 0.
- Native exit-list slot 0: `0x286`, Kokiri Forest upper exit.
- Surface words were read in the factory's native order: serialized data[1],
  then data[0]; scene-exit bits are `(data[0] >> 8) & 31`.

The source fallback is inside the ordinary approach polygons and outside the
exit floor. No native or authored archive was edited.

## Verification and status

The new source regression first failed on the exact unmodified Kokiri exit
input, then passed after the bounded source change. The fixture invokes the
production collision-exit resolution and controller with native save/player
types; it does not simulate a complete running game or renderer.

- `tests/lost_woods_entrance/run_tests.py`: complete runner passes, including
  native/legacy portal handoffs, private-forest routes, Woodfall, vertical
  scrolling, native interpolation objects, conditional fountains, saves,
  cameras and arrival ownership.
- `tests/lost_woods_entrance/run_fountain_return_tests.py --sanitize`: native
  room-7 entry/return for both ages and OoT modes, with/without entrance shuffle,
  unsampled entry and save/reload; six authored fountain destinations, moved
  rooms, ordinary exits, malformed/stale saves and Farore/paired priority.
- `tests/black_market/run_solo_forest_tests.py`: both engines' native route,
  grounded origin, Clock/stump return, reload/void/missing-pack and isolation
  fixtures pass.
- Complete production controller object compiles against real headers with
  `COMBO_BUILD`; the original non-Combo object compiles too.
- clang-format 14.0.6 and `git diff --check` pass.
- Independent read-only review approves with no actionable findings. It
  reviewed the production caller and return/save lifecycle; it did not claim
  configured runtime testing.

The recovery receipt records the final focused rerun after composition with
the Saria parent and an independent restore of the exact implementation tree.

Configured Windows gameplay remains untested. The decisive smoke test is a
no-mod round trip through room 7's north tunnel, then save/reload in its fountain
and return, followed by an original fountain entry/exit and the adjacent
Woodfall tunnel. This is a source candidate; no runtime master is promoted and
no executable build/publication is started while the other bugfixes finish.

## Recovery

The checkpoint receipt records the exact parent, implementation commit/tree,
changed-source hashes, patch, source copies, verification evidence and an
independent restore. Apply its patch to the baseline above to restore this
bounded change with all previous morning and Black Market sources preserved.
