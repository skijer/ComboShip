# Solo vanilla and randomizer games

## Candidate and baseline

This is an isolated source candidate on `feat/solo-games-20261016`, built on
merged PR43 `1bbb94e692629c0d910f97b57dd7cd02240d3775` and the complete MQ
candidate tree `78d7d3504b8d7c3a27d8e4ef978eb77a5fd76a42`.
The recovered MQ candidate is preserved locally as commit `3fd851dd`.
The donor checkout and accepted master are unchanged. Other separate polish
checkouts are not implicitly incorporated by this candidate.

## Player behavior

Ordinary launches display three game choices: Combo (OoT + MM), Ocarina of
Time, and Majora's Mask. The last choice is remembered in
`combo-launcher.json`. Closing this chooser starts nothing and preserves the
previous choice. Choose a different game by restarting the executable.

Both solo games offer their native vanilla and randomizer file-selection
flows, native generation/settings and native spoiler loading. OoT retains
vanilla, MQ and mixed randomizer dungeon layouts using the installed quest
archives. A solo session requests only its own game's ROM. Port modules and
bundled port/custom assets still ship together.

Vanilla means native progression and item placement; existing optional port
enhancements remain available. Randomized placements and win conditions use
the selected game's native generator. A solo game rejects consolidated Combo
spoilers and the legacy paired quest. Existing combined saves are not converted.

| Entry point | Result |
| --- | --- |
| Ordinary launch | Show the game chooser |
| `Fleet --game=oot` | Start OoT solo directly |
| `Fleet --game=mm` | Start MM solo directly |
| `Fleet --game=combo` | Start the existing combined game directly |
| `--game-select` | Show the chooser, including with a direct game argument |
| `--rom-setup` | Reopen setup for the selected game; OoT can add the complementary quest |

On Linux the executable is named `ComboShip`; the same flags apply.
Unrecognized game values and contradictory game arguments fail before boot.

## Isolation and preservation

Solo OoT uses `saves/oot/`, and solo MM uses `saves/mm/`. Slot/global saves,
backups, copy/erase and MM photo/raw-save sidecars follow that isolation.
Combined `.combosav` containers retain their existing routing and format.
Game mode is fixed before ROM-dependent initialization; persisted randomizer
settings cannot change the session's game.

Solo does not install combined seed, save, shared-item, goal or transition
callbacks. Runtime guards also disable cross-game entrances, foreign handoffs,
shared progression and combined victory rules. Native reset/quit, Ganon/MM
endings, Triforce goals and earned-equipment checks remain native. OoT retains
its native network transport; MM retains native Harpoon. The MM Combo Anchor
adapter remains inactive because it depends on the launcher's dual-game transport.

MM-only uses a ROM-free shared-window bootstrap without initializing OoT
gameplay. That resource owner registers the rendering factories needed for
bundled custom models. Solo native menus activate only after the corresponding
game is initialized. Closing setup releases the bootstrap before module unload.

Scene geometry, actors, rendering effects, weather and audio resources already
in the baseline are preserved; this work changes game/session boundaries.

## Verification

`tests/solo_games/run_tests.py` runs the C mode contract, CLI/persistence tests,
complete launcher with scripted module boundaries, the real chooser renderer
boundary, native save/file-select/seed/goal/transition functions, native settings
finalization, equipment ownership and resource-factory binding checks. The
launcher checks selected-ROM-only boots, argument forwarding, selection cancel,
teardown ordering and the combined path. Independent review found and fixed
the legacy paired-quest exposure and missing bootstrap rendering factories;
the tests include the actual production quest enum ordering.

The existing MQ/setup suite, cross-dungeon and boss/transition suite, shared
save/runtime suite and available NEI regressions are retained. Fixture changes
provide an explicit combined session for extracted production bodies; native
variants and their assertions are preserved. `clang-format` 14.0.6 was applied
to the changed production files and checked with `--dry-run --Werror`.

Reproduction commands, with the project's third-party headers supplied by the
normal build/dependency environment:

```sh
python3 -B tests/solo_games/run_tests.py
ASAN_OPTIONS=detect_leaks=0 python3 -B tests/rom_setup/run_tests.py --sanitize
ASAN_OPTIONS=detect_leaks=0 python3 -B tests/cross_dungeon/run_tests.py --sanitize
python3 -B tests/rom_setup/run_syntax_checks.py
python3 -B tests/solo_games/oot_syntax_test.py
python3 -B tests/solo_games/mm_syntax_test.py
bash scripts/diagnostics/run_combo_nei_regressions.sh
```

The syntax runners enumerate fixed production sources and do not depend on
unstaged git changes. MM's independent-unit runner uses the native project's
header/unity boundary and reports existing permissive-header warnings. These
are production source/header checks, not a complete CMake target build.

The source handoff contains exact per-command outcomes and logs. This
environment lacks CMake, `png.h`, and some historical pinned Git fixture inputs;
affected checks are reported as blocked, not passed. The `ColorPictograph.cpp`
header failure also occurs on the exact MQ baseline, while its changed path
function passes the compiled behavior test. Leak detection is disabled for the
container's process-inspection restriction; named address/UB checks remain on.

## Runtime acceptance still needed

Build and package the candidate, then create/play/save/reload a vanilla and a
randomizer file in each solo game. Test OoT with either quest alone and both
quests installed, including mixed randomized layouts. Check native endings,
MM reset/owl-save/cycle behavior, custom NEI equipment visibility, and switching
the launch choice between sessions while all three save sets remain intact.
Repeat a combined seed's cross-game forest/dungeon/boss traversal and reload.

No full application link, Windows package, real-ROM extraction or installed
gameplay was run here. Factory tests prove bindings, not decoded asset visuals.
This candidate is implemented and source-verified to the recorded limits;
runtime acceptance and promotion remain separate.

## Recovery and merge

The final export includes all tracked source, a solo-only patch relative to
the recovered MQ candidate, a cumulative patch relative to PR43, exact file
hashes, and verification logs. Use the patch matching the receiving branch;
do not apply the cumulative patch on top of an already-applied MQ patch.
When incorporating other polish checkouts, preserve their newer work and
resolve shared launch/menu/save changes deliberately. Reverse the solo-only
patch to return to the MQ source candidate. Existing saves need no migration.
