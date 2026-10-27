# OoT-host Zora combat and water-exit repair

The port played Zora punches 50% faster than MM and morphed into each attack for
eight frames. Its third hit used a Goron butt-punch approximation, and the form
renderer bypassed OoT's ordinary sword collision callback. The jump kick had no
live form collision submission. Dolphin exit replaced an unfinished water roll
and advanced its animation twice; fast-swim fin ribbons were absent.

The candidate uses MM's adjusted attack speed (2/3 in OoT's animation clock),
zero attack/recovery morph, and the native three swept forearm/shin tip/base
pairs. Grounded punches latch the hit window before advancing the animation;
the jump kick checks after advancing, as the respective MM handlers do. Quads
remain Player-owned and use the existing OoT damage mappings. Third-hit wall
recoil faces forward. Fresh B presses still chain attacks; the obsolete extra
four-frame pause is removed.

Dolphin launch keeps the current clip/frame until it finishes, then starts
fishswim. Animation tick ownership survives an action switch on landing. Two
independent fin ribbons use native MM tip/base coordinates in the existing fin
matrices. Landing, water entry, action interruption, reset and scene initialization
release or discard their handles appropriately. Failed native effect allocations
retry without treating the failure sentinel as a valid handle.

## Evidence

`python3 -B tests/oot_zora/run_tests.py` executes production action and collision
bodies on actual OoT types, including the native swept-quad helper, under
ASan/UBSan. Ten cases cover animation rates/morphs and other-form preservation,
dolphin pose retention, jump activation, paired shin sweeps, interrupt cleanup,
independent fin ribbons, tick ownership, landing trail ownership, native allocation
failure/retry, grounded pre-tick windows and dolphin landing's single tick.

The initial timing failure and four additional review regressions were observed
before their fixes. Independent code review reran all ten cases successfully.
The complete `mm_player_form.cpp` translation unit also compiled for syntax using
the real project/dependency headers; its existing extern-initialization warning
remains unchanged.

These are CPU/source checks. Gameplay hit feel, enemy damage/knockback, wall
recoil, Z+A kick, water re-entry, boomerang hold/throw/catch, barrier behavior and
visible ribbons require a matching application build and configuration-specific
playthrough. Existing swim-fin scaling/display lists differ from MM, so complete
visual parity is not claimed.
