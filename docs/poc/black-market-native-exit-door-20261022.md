# Native interior exit door — October 22, 2026

The existing shop exit already worked by walking past z400. Both custom
scene headers omitted a transition-actor list, so the doorway had no visible
native door. This change adds the engine's wooden handle door and keeps the
existing return destination, travel/session binding, entrance tables, stock,
progression and collision.

## Scene and actor contract

`tools/black_market/exit_door.py` reads the shared constants in
`combo/BlackMarketExitDoor.h`. It inserts command `0x0E` immediately before
the existing player-start command `0x00`, after special-file command `0x07`.
The MM order is `15,04,03,06,07,0E,00,11,13,0F,1B,14`; OoT uses the same
order without `1B`. Other command payloads and the 64-byte resource header
remain byte-identical. The immutable geometry bundle is unchanged. The
prepared resource count is unchanged by the door adapter.

| Field | MM | OoT |
| --- | --- | --- |
| Transition actor count | 1 | 1 |
| Native actor | EnDoor, `0x0005` | EnDoor, `0x0009` |
| Position | `(-10, 0, 420)` | `(-10, 0, 420)` |
| Front room / camera | `0 / -1` | `0 / -1` |
| Back room / camera | `-1 / -1` | `-1 / -1` |
| Serialized yaw | `127`: zero degrees, CS_ID_NONE | `0`: binary yaw |
| Parameters | `7`: WHOLE_DAY, every half-day | `384`: DOOR_SCENEEXIT |
| Actor profile object | gameplay_keep, `0x0001` | gameplay_keep, `0x0001` |

The native initializer selects gameplay_field_keep (`0x0002`) when available,
and native gameplay_keep fallback otherwise. The scene already requests
special-file object 2. Native skeletons, animations and wooden door display
lists supply the appearance; no texture, custom model or new object bank is
required. A real transition record is essential because native door drawing
and destruction read it. The door is spawned by each host's existing
`Actor_SpawnTransitionActors`, rather than an unregistered manual spawn.

The preserved arrival is `(-11, 0, 381)`, facing the counter at yaw -32768.
It stays 39 units inside the door and below the old exit threshold. Native
door offering, A/Open, player animations and form/age selection remain owned
by the engines.

## Return and escape behavior

The first interior-only branch in each existing `TryDoorEntrance` hook
recognizes the exact native actor/record. After native Open selects its
normal action, the branch runs the previous `RememberTravel`, return-pending
and return `Transition` sequence. It returns before vanilla room-loading
camera logic. Unsafe, dead or talking states use the existing native blocked
Open cleanup.

A matching live, drawable door suppresses the old z400 trigger while the
player approaches. Ordinary native EnDoor leaves do not have solid collision.
A walk-through safety exit therefore remains at z440, 20 units behind the
leaf. The unchanged collision supports that point and ends at z458; z470
has no supporting floor. A missing, killed, not-yet-drawable or mismatched
door restores the original z400 escape. Live actor-list membership is checked
on demand; no actor pointer is cached across teardown or scene reload.

## Verification

With `test-dependencies/include_paths.env` loaded, these focused commands pass:

```sh
python -B tests/black_market/check_exit_door_resources.py --prepared
python -B tests/black_market/run_native_tests.py --case door-exit
python -B tests/black_market/run_oot_native_tests.py --case door-exit
python -B tests/black_market/run_exit_door_actor_tests.py
```

The resource fixture proves the two exact host records, preserved arrival,
unchanged resource inventory and all other bytes, idempotence, malformed
record rejection and floor support across the approach/escape corridor.
The controller cases execute the production bodies and actual existing
player Open hooks under ASan/UBSan. They verify the unchanged return entrance,
no vanilla door camera, no early walk bypass, far-side escape, missing-door
fallback and blocked-Open cleanup without grants, writes or stock changes.

The actor fixture executes actual native scene spawning, Idle, limb-display
selection and Destroy bodies under ASan/UBSan. It covers every MM form on
all six playable half-days, both OoT ages, both door sides, animation
dispatch, a concrete native wooden door display list, duplicate prevention
and three destruction/reload cycles. Actor allocation and animation dispatch
are fixture boundaries; native initialization/object selection is also
source-audited, not represented as a full game render.

Changed production paths are the new shared header and scene adapter, the
two controller includes/exit guards/interior Open branches, and the shared
preparation/CMake hook coordinated with the default title. Focused fixtures
are `check_exit_door_resources.py`, `interior_exit_cases.inc`,
`exit_door_actor_test.cpp` and `run_exit_door_actor_tests.py`, plus their
existing MM/OoT native runner/harness integration.

These are resource/native-fixture checks. Fresh game/GPU appearance,
configured controller interaction and user acceptance remain runtime checks;
no master is promoted by this POC.
