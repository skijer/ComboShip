# Contents-based chests and opt-in Midna in ComboShip

This candidate adds the requested chest and companion functionality to both engines. It starts at cumulative ComboShip `39b63af006ed3157177a501ee2cab1e9af064159`, including the latest MM streamed-PCM fix. All parent features and accepted source branches are preserved. Runtime acceptance is separate from implementation and build verification.

## Sources and preservation

- SoH chest-size donor: `marsh6487/Shipwright:d676a5ffd2f61677e45190259165005d51e17df3`.
- Midna donor: current POC3 `marsh6487/Shipwright:822f46b15dcae98b0a0564ec469a061f5106d4c4`, including the face/eye correction, pose animation, visible-time blink clock and idle-yawn scheduler.
- Reviewed SoH chest archive SHA-256: `d07ab7d516d2c8da2df1eb45dfc0b1941ba3d0ea90b3a1dada5fa76a2b74a183`.
- Original six-chest source SHA-256: `cdbaf1edd3a764382f5187698a33675aaa1d80c60b148bdff6e626e91da71a94`.
- POC3 Midna archive SHA-256: `dbe4552da4110b845c00a78ea9b02dcc8239ec18214166120de46ddfc2f8c176`.

`scripts/mods/package_companion_chests.py` takes these three explicit inputs. It copies the SoH and Midna archives exactly, relocates the MM chest resources, reverses every XML path substitution to verify complete byte preservation, and writes a checksum report. User asset archives remain outside the Git repository.

## Controls and installation

All new options default off. ComboShip shares its CVar store, so the MM options deliberately have distinct names.

| Engine | Checkbox | CVar | Location |
| --- | --- | --- | --- |
| OoT | Midna Companion (Navi) | `gEnhancements.MidnaCompanion` | Enhancements > Graphics |
| MM | Midna Companion (Tatl) | `gEnhancements.MidnaCompanionMM` | Enhancements > Graphics |
| OoT | Chest Size Matches Contents | `gEnhancements.ChestSizeMatchesContents` | Enhancements > Quality of Life |
| MM | Chest Size Matches Contents | `gEnhancements.ChestSizeMatchesContentsMM` | Enhancements > Graphics |
| MM | Chest Style Matches Contents | `gEnhancements.ChestStyleMatchesContentsMM` | Enhancements > Graphics |

Put the SoH fixed chest pack and Midna POC3 pack in `mods/soh`; put the MM chest adaptation and another byte-identical Midna POC3 pack in `mods/2ship`. Disable older duplicates and restart after changing installed packs. OoT chest models also need Alternate Assets and the existing Containers Match Contents option. MM uses private resource paths gated by its style checkbox or the existing randomizer CSMC option.

## Chest behavior

The six models cover major items, lesser items, health upgrades, small keys, Skulltula Tokens and junk. No boss-key resources are supplied or replaced: the active compatible asset pack continues to provide them. Unsupported model categories use native/current-pack drawing.

The MM adaptation does not replace `gBoxChest*DL`. The existing randomizer copies and patches those display lists at fixed lengths and indices, which is incompatible with replacing them with short XML wrappers. Private names avoid both that assumption and collisions with cached native chest child resources. Body and lid must both resolve before either custom model is used. No loaded raw pointer is retained across frames.

Native actor type, collision, reward and collection flags remain intact. MM sizing changes only the draw matrix; its native opening-animation and interaction choices remain intact. This limits gameplay disturbance but requires runtime inspection of hand/lid alignment when the visual size changes. Treasure Chest Shop concealment is retained. Randomizer traps resolve progressive disguises to the displayed tier and latch their opening appearance. An explicit per-chest check identity prevents authored rotation metadata from being mistaken for an unrelated randomized check. Ordinary undisguised ice traps and unknown native items retain their authored appearance.

Custom chest materials are opaque. MM fading and Lens passes retain native rendering. This candidate does not claim transparent custom-material support.

## Companion behavior

Midna replaces only the player companion, Navi or Tatl. Existing movement, hints, dialogue, targeting, visibility, emergence and alpha gates remain native. Other fairies retain native rendering and sound. Missing model/pose/eye resources fall back to the available model; missing or unsupported individual clips use the original cue.

The MM port maps companion dash/emergence, recall, target, dialogue and HUD-call events to the existing Midna clips. Tatl's flagged anger loop remains native because the pack has no matching looping cue. Native call suppression and randomizer veto hooks are retained. Both engines use their own resource manager and volume settings. The private voices are cleared on checkbox changes, companion destruction, muted inactive-engine output and audio-thread shutdown, including game transitions. They mix before the existing final inactive-game mute. Custom audio remains dry and centered, as in the donor.

## Verification

The five focused diagnostic entrypoints are registered in Linux CTest and the existing build-artifacts gate:

```sh
python3 scripts/diagnostics/run_chest_size_tests.py
python3 scripts/diagnostics/run_mm_chest_contents_tests.py
python3 scripts/diagnostics/run_midna_navi_draw_test.py
python3 scripts/diagnostics/run_midna_audio_test.py
python3 scripts/diagnostics/run_mm_midna_test.py
```

They execute production selection/draw/audio functions and compile full affected C actor/HUD translation units against the game headers. Coverage includes default-off and independent controls, native categories, traps, partial pack failures, visibility and fade, companion-only routing, PCM validation and mixer/idle cancellation. The MM weather output test additionally checks that the new mix is silenced and its pending voice cleared by inactive-engine muting.

Local full CMake configuration stopped at the unavailable SDL2 development package. Focused checks and actual resource-adapter C++ syntax checks are available locally; a complete executable/link build must be taken from CI. Runtime rendering and sound have not been exercised here.

## Runtime acceptance

Use the same save and existing load order as the prior cumulative build. Check each companion enabled/disabled, missing-pack fallback, pose/blink and sound during movement, dialogue and idle. Switch OoT to MM and back, confirming independent settings and no lingering voices. Compare major, junk, key and trapped chests; retain a Djipi boss-chest comparison. Check opened chests, scene reentry, minigames, Lens visibility and altered-size opening alignment. Record executable commit, pack hashes, scene and Alt Assets state before acceptance.

Rollback is the parent executable and prior packs, or simply disabling these options. This candidate does not modify save formats or promote an accepted master.
