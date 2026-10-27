# OoT Time Gate audio regression

Run `python3 -B tests/oot_timegate/run_audio_tests.py` from the repository root.
The fixture compiles the complete OoT item handler with the native OoT SFX
request/bank engine and sound parameters under AddressSanitizer and
UndefinedBehaviorSanitizer. Input, animation resources, camera, scene transition,
and audio-device output are boundary fixtures.

The production baseline at `7fc75feb` (which includes MM's `a6ae6e1b` fix) failed
with `OoT Time Gate sound survives Yes`. OoT lacked cleanup for its two warp
sounds when its item state ended. The candidate stops only those IDs at the
original player's position on existing terminal paths.

Coverage includes Yes, completed No cancellation, damage, unequip, invalid
state, repeated casts, and cancellation while both sounds are still queued.
The same IDs from a second actor and a different sound at the player position
must survive. Choice-only magic consumption, age-switch dispatch, transition
sound, visibility reset, and player control release are also checked.

OoT delegates A/B to the textbox and has no MM-style independent hover timeout.
The B and long-hover controls therefore verify the existing prompt remains
active, then choose No to finish. This cleanup change does not add new controls.

These are native state/audio integration checks, not an in-game playback test.
The reported loop's exact sample remains unconfirmed. Audible verification in
the user's Link/Mario configurations and resource packs remains separate.
