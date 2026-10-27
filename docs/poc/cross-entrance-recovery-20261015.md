# Cross-entrance recovery — final source checkpoint, October 15, 2026

The approved implementation and available automated/native verification are
complete. Continue from the final checkpoint; do not restart the feature work.

Implementation commit: `affcc193ff9092f333f0481960ad4b2d9c96267f`.
Branch: `bridge/cross-entrance-recovery-20261015`.
Required base: PR41 `913bc3298bfcdd57226f87e326eaf4bf72ccd33a`.
The checkpoint's RESTORE.md identifies the exact final source/documentation
commit and restoration commands. Its bundle and patch are cumulative.

The recovery preserved the interrupted source, imported the continuing original
POC through its constrained-generation work, and added the reviewed native
decoupled boss death fix. Incoming native death ownership is separate from
reverse travel, including the actual spawn after foreign-host recovery.

All four pools, the expanded MM destinations, Farore recovery, MM time/clock
admission and Stone Tower normalization are implemented. New options default
off; legacy v1 map/codec/RNG behavior and physical reward ownership are preserved.
See the spec, completed implementation plan and
`docs/poc/cross-entrance-expansion-verification-20261015.md` for the exact scope.

The full cross-entrance and Farore suites, focused ASan/UBSan, launcher reload,
shared-item/slot/plando tests and independent reviews pass. Both engines,
launcher, headless generator and UI compile/link. The accepted native matrix
covers individual pools, simultaneous180 entries, both starts, complete JSON
determinism/exact replay, native mixed coupled and the documented open-time
mixed-decoupled preset. Every new-pool profile passes its no-trick playthrough.
Final header-complete stage03 repeats exact full replay, four combined
playthroughs and the linked recovery API probe. Source/artifact hashes match.

One advanced stress seed with native mixed decoupled entrances, closed Door of
Time and Ganon's Castle included exhausts both fill attempts and returns failure
without an accepted seed. It remains documented as a rejected control; the
passing decoupled preset opens the Door of Time and excludes Ganon's Castle.

Windows build and installed-gameplay/rendering acceptance remain unverified.
The verification report lists the concrete boss, death/reload, Farore, grotto,
time-reset and slot cases to play against installed assets. No ROMs or user
saves are included, and no develop/master promotion or push occurred.

The original checkout was `/workspace/scratch/de7bddddfa69/ComboShip`; it was not
used as the target for these recovery changes. The recovery checkout is
`/workspace/scratch/c2a29bb3cfc6/ComboShip-CrossEntrance-Recovery`.
Local build dependencies and old configured objects were reused without copying
the entire repository. Provision normal dependencies for a portable rebuild.
