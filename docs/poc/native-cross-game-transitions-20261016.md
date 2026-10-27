# Native cross-game entrance sequencing and forest arrival

| Field | Record |
| --- | --- |
| Source baseline | `86582f5a7c9b81fe68f0174b43fed32fa1a62997`, the combined draft candidate containing entrance/reset, Goron/Somaria and Four Sword/Byrna corrections. Its platform build is separate from this POC. The user's installed package revision is not established here. |
| Candidate | `fix/cross-game-transition-20261016`, isolated from that source. |
| Report | An opposite-game doorway freezes before its native entrance sequence, then snaps to the next scene. The Woods of Mystery arrival stays black until moving forward. |
| Intended result | Play the normal source entrance movement, camera and transition effect before loading the other engine; retain the completed effect during the handoff, then use the destination's native entry. Forest arrival should be visible without input. |
| Preservation | Routing/permutations, source checkpoints, native same-game transitions, source/destination animation data, forest geometry/trigger, MM cycle/time, Farore state and all baseline fixes. Broad Interiors remains available; a narrower Shops pool is a separate follow-up. |
| Acceptance | Implementation and named automated checks are separate from configured game/GPU acceptance. No accepted-base promotion. |

## Causes and bounded changes

Regular foreign routes were consumed by the next clean-frame hook immediately.
MM also explicitly canceled its native transition in `TRANS_MODE_SETUP`. This
began engine/save initialization before the normal outgoing sequence finished.

Pending routes now wait for native transition completion. Native Play marks a
route ready at its existing outgoing completion branch instead of replacing
the source scene. The clean-frame hook then queues the peer, persists the
source checkpoint and exits once. Instance transitions, fill, instant and
sandstorm completion retain their own native type and timing. Destination
spawn/animation/transition selection remains native. No artificial delay,
global replacement fade or loading-speed improvement is claimed. Cold/death
boss recovery retains its existing behavior because that path may have no
outgoing entrance transition. Play destruction, Reset and explicit Farore
priority discard an uncommitted ordinary departure.

MM commits after `Play_Main` has built the completed transition's display list;
committing at main-start would remove the effect before the last source draw.
Both forest sends also render a full-alpha frame before committing. OoT's
blue-warp redirect is a distinct native-init path: its source sequence has
already completed before `Play_Init`, so the existing override callback
explicitly acknowledges completion instead of waiting for another exit.

The forest blackout has a separate geometric cause. At the old RETURN spawn
`(90,0,-90)`, native human camera initialization puts the eye approximately at
`(215.267,75.070,-215.461)`. The sight ray crosses the double-sided black
passage mouth and the native wall. The native screen-fill state is reset and
the native arrival transition advances without stick input. Moving the RETURN
spawn to the clearing center `(0,0,0)` moves that initial eye to approximately
`(125.267,75.070,-125.461)`, clear of the mouth and wall. Room 1, outward yaw
`-8192`, start mode E and the passage geometry remain unchanged.

## Evidence and runtime boundary

The transition fixture first fails both baseline directions because production
`PrepareSwitch` accepts the handoff before the native effect completes. It
exercises production route/queue/checkpoint bodies, native transition cases,
native fade arithmetic and registered handoff hooks. Event dispatch, disk I/O,
DLL callbacks and GPU output are explicit boundaries.

The complete cross-dungeon suite passes locally with ASan/UBSan, including
native sequencing, blue-warp, adapter and boss recovery checks. The complete
forest suite and the sanitized camera fixture also pass. Both modified native
`z_play.c` translation units pass syntax compilation against real engine
headers. The blue-warp hook compiles and passes in standalone and Combo modes;
its conditional compilation stays outside the registration macro arguments.
The cold blue-warp regression executes the actual registered native-init hook
and route owner across eight first/repeat aliases, with foreign, local and
native controls. It passes normally and with ASan/UBSan; pairing the original
hook with the new gated runtime fails on a pending first Deku blue-warp route.
CI runs native translation-unit syntax and transition/camera fixtures just
after dependency setup, before the long regression groups and platform builds.

The camera fixture first fails at the old spawn and passes at the clearing.
It compiles native camera initialization, RETURN respawn assignment, passage
geometry/transforms, sine and triangle math. Stationary NORMAL0 settling is
checked for native heights 36, 44, 68, 80 and 124. Full camera collision/AI,
custom camera settings and rendered appearance remain runtime checks.

The next decisive test uses the matching rebuilt package: open a shuffled
doorway in each game and watch its normal sequence finish before loading;
repeat with a grotto and a native same-game entrance. Enter Woods of Mystery
and leave the controls untouched until the arrival completes, then return
through the passage. Verify Reset during a departure and an explicit Farore
recall still keep their own destinations. No diagnostic-only build is needed.
