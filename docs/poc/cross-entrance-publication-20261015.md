# Cross-entrance publication and remaining donor transfers

The user authorized publication to `marsh6487/ComboShip` on October 15 and
also requested the latest Demise and Lost Woods–Woods of Mystery work in the
combined candidate. Publication does not establish installed-game acceptance.

## Recovered shuffle candidate

- Required base: PR41 `913bc3298bfcdd57226f87e326eaf4bf72ccd33a`.
- Recovered final source: `d15905cbf2dd73efb42666d813ad0f0922454756`.
- Implementation: `affcc193ff9092f333f0481960ad4b2d9c96267f`.
- Branch: `bridge/cross-entrance-recovery-20261015`.
- Scope: all four opt-in entrance pools, expanded MM dungeons, cross-game
  bosses and recovery, native entrance composition, Farore checkpoints,
  owned time/clock admission and exact legacy/recorded seed handling.

Publication preparation applies the repository's clang-format 14 output to
50 changed engine/shared source files. Changes are whitespace and inspected
comment formatting only; the recovered implementation is retained. All 78
changed files covered by the repository formatting gate match clang-format 14.
The existing native matrix and its exact limits remain documented in
`cross-entrance-expansion-verification-20261015.md`.

Fresh checks in the publication chat pass the complete cross-entrance and
Farore suites after formatting, production launcher reload/preflight and
shared-item/slot/plando suites, asset collision validation and whitespace
checks. All six native stage03 artifact hashes match the saved final manifest;
its linked recovery probe and the four combined OoT-start, MM-start, native
coupled and accepted native-decoupled playthroughs pass again. The native
binaries correspond to the recovered implementation before the formatting-only
commit. A fresh Windows/package build remains pending.

## Required donors for the requested combined merge

| Work | Exact latest revision | Transfer status |
| --- | --- | --- |
| Demise purple/black medallion effect, sustained voiceless dark-energy sound and quiet 28-frame dissipation | `cfb9508f847181113c674934f0ab36ed020dfa7b` | Source absent from this checkout and GitHub; saved final preview provenance identifies this commit. |
| Bidirectional OoT Lost Woods tunnel and MM Woods of Mystery tree passage | `ae5c3162` on `poc/lost-woods-2ship-entrance-20261015` | Source absent from this checkout and GitHub; prior handoff identifies this commit and its develop base. |

The public Demise branch currently ends at the earlier `c7987256` revision.
That earlier implementation is not the requested final donor. The final
Demise source and forest source have not yet been merged into this candidate.
Each original task must provide a cumulative source bundle or patch containing
its final revision and its exact base. Preserve the final Demise renderer,
damage behavior and voiceless audio, and preserve the forest route's native
portal, save/slot, maze-exit and reset behavior when resolving shared files.

The shuffle source can be preserved on its authorized remote candidate branch
while those transfers are outstanding. Update the candidate and its PR only
after both donor diffs are available, reviewed, merged and verified. Keep the
accepted develop/master baseline unchanged; Windows and installed-game
acceptance remain separate.
