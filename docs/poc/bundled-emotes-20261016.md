# Emotes implementation checkpoint

Plan: `docs/superpowers/plans/2026-10-16-bundled-emotes.md`
Baseline: `1bbb94e692629c0d910f97b57dd7cd02240d3775`

- Scope accepted by user on 2026-10-16: all eight bundled animations, attribution, implementation of both multiplayer pose paths; one complete runtime candidate.
- Isolation: `feature/bundled-emotes-20261016`.
- Pre-flight: both adapters consume the same engine-independent decoder/clock; UI consumes the catalog and the same CVar keys. MM Anchor consumes aligned byte buffers, whereas Harpoon and OoT Anchor consume joint vectors.
- No game code changes yet. Runtime proof and user acceptance remain pending.

## Recovery checkpoint — 2026-10-16 04:30 America/Chicago

- All eight animations and both supplied sounds are bundled under both games' custom asset trees, with creator credits and source hashes.
- Shared Settings > Emotes has four configurable slots/chords, Play/Stop and local sound volume. Defaults use M1 + D-pad.
- Both native adapters, post-animation application, reset/game-swap/audio seams, and all Anchor/Harpoon sender paths are implemented.
- Passing: shared behavior/assets suite; actual-header syntax for both adapters, both game loops, both audio hosts, all four network sender translation units, shared menu and Emotes panel. Some pre-existing compiler warnings remain.
- A full build has not run: CMake/Ninja are unavailable in this environment. Existing Windows/Linux CI regenerates both custom asset archives.
- Remaining work: independent review and any resulting corrections, broader native lifecycle tests, final packaging. All visual behavior, custom-model parity and two-client runtime proof remain untested. This is an in-progress recovery checkpoint, not a promoted or runtime-accepted build.

Resume in `docs/superpowers/plans/2026-10-16-bundled-emotes.md`; do not repeat the completed asset import or shared playback implementation.

## Reviewed source candidate

Implementation is complete as an isolated source candidate; runtime acceptance remains pending.

- Independent review covered baseline `1bbb94e6` through checkpoint `b13d3192`. No critical issues were reported. All three important findings are corrected: secondary torso yaw is saved/neutralized/restored; the actual OoT/MM `PrepareForTransition` handoffs reset playback before audio shutdown; unsupported appearance values reject the whole clip and stop decoding/audio.
- A native OoT harness first reproduced the secondary-yaw defect, then passed after the fix. Shared validation tests failed before `ValidateClip` was added and now reject malformed full-length clips. Both actual native adapter harnesses pass lifecycle, damage, pause/queued playback, chord consumption/hold, pose parity and reset/audio cleanup checks.
- Integration inspection also found that MM Harpoon's cached native templates retained the viewer's look offsets. Those offsets are now neutralized before drawing the peer; the shared helper is exercised on real native Player copies in the harness.
- Passing: `python3 tests/emotes/run_tests.py` (every frame in all eight clips, both asset trees, bounds/appearance, 20/30/60 Hz clocks, chords, audio); `python3 tests/emotes/run_native_tests.py` (both game adapters with actual game structs; dependencies supplied through CPATH); native-header syntax checks for all 13 changed game/UI translation units; `scripts/check-asset-collisions.py`; `tests/nei_asset_priority/run_abi_tests.py`; `git diff --check`. Compiler warnings in existing native headers/hosts remain.
- Windows/Linux CI now runs the portable shared emote/assets suite before the expensive build. Existing CI targets regenerate both custom archives; all eight raw animation streams are explicitly tracked in both trees.
- Blocked: full configure/build and `scripts/diagnostics/run_windows_launcher_resource_tests.py` because CMake is not installed; Ninja is also unavailable. No Windows build, in-game playback, custom-model visual parity, real peer interpolation or two-client Anchor/Harpoon runtime proof is claimed.
- Local bundled sounds are implemented. Custom emote audio is not transmitted to peers. Non-human transformation skeletons are excluded.

### First runtime pass

Build this complete candidate with both `GenerateSohOtr` and `Generate2ShipOtr`, or use the existing complete CI workflow. Under Settings > Emotes, test every built-in entry as OoT child/adult and MM human, then verify movement/damage/pause/scene and game-swap cancellation. Check vanilla/custom player models and emote visibility on both multiplayer transports. The source is preserved independently of those runtime checks.

### Solo integration repair

The native OoT and MM Settings menus now expose Emotes in solo sessions. They
reuse the same renderer, slot/chord CVars, Play/Stop, volume and creator credits
as the shared Combo menu; a solo player can select all eight clips without
initializing the other game. The MM adapter reads its native floating-point
master/SFX volume settings, including zero, while OoT retains its integer keys.

`python3 -B tests/emotes/run_menu_tests.py` executes both native registration
callbacks and the production panel with UI-input/CVar fixtures; its
`--source-rev 4a6c2dfbe` control detects the missing native route. The native
adapter harness also checks independent per-game gains and master/SFX mute.
This is source/harness evidence; native menu appearance, saved configuration
reload, audible in-game behavior and two-client playback remain runtime-pending.
