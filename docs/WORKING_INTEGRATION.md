# ComboShip working integration

Repository: `marsh6487/ComboShip`

Cumulative working ref: `integration/shared-items-soh-20260921`

Integration audit: 2026-09-22

Newest cumulative testing candidate: `poc/comboship-chests-midna-20260923`, based on
the complete MM streamed-audio candidate `39b63af0`. It adds optional contents-based
chests and independent default-off Midna companions in both engines. See
[the candidate record](poc/2026-09-23-combo-chests-midna.md) for sources, packs,
verification and runtime limits. This remains a testing candidate.

The September 22 cumulative candidate was `poc/mm-dungeon-bottles-20260922`, draft
[PR #8](https://github.com/marsh6487/ComboShip/pull/8). The September 22 correction
combines the complete working integration with the dungeon/bottle changes,
MM back-equipment and OoT-sky controls, and the separately published OoT
transformation-cosmetics and HD flame-sampling changes. This is a testing
candidate; the accepted baseline and its source branches remain unchanged.

The working integration carries compatible features forward together. Start new
integration work from the latest cumulative tree, preserve the accepted source
branches, and record any feature deliberately left out. A branch being pushed or
building successfully does not by itself establish in-game acceptance.

## Sources and retained features

The original cumulative base starts at published weather commit
`51ad3c1f3fe92e76f79ec0a47c68f412380fa319` and adds only GUI-font commit
`075e4e197faa865b5542be5e9d2f6ac96b85e4ae`, plus this record.

| Feature | Retained source |
| --- | --- |
| Shared Items, existing NEI behavior, and the SoH feature stack | `cf1568d4c9a5040c459fb9ed427d27b7216c0ae5`, including prior `cac526a8` stabilization |
| Per-game mod directories | `01e22573` ancestry and subsequent upstream integration |
| Independent opt-in MM scene cosmetic and audio randomization | `93117d11` through `cc1346ce` |
| MM streamed audio IDs beyond 255 | `93117d11`, `9e492676`, `cc1346ce` |
| Ogg/Opus decoding guards and decoder regression linkage | `5887d319`, `10712653` |
| MM cosmetic display-list bounds and resource-reload guards | `626d0e32` |
| Outdoor rain, independent weather audio, pause timing, spin/story overrides, and overcast sky fixes | `835489f0`, `51ad3c1f` |
| Live custom heart and magic-pot colors, including placed/boss hearts and Double Defense | `835489f0`; its seven item source/test blobs match `e64aff98` |
| Optional mod GUI fonts loaded after MM mod archives | `075e4e19` |

The original weather source `4984ef21` was bundled into `835489f0`: weather
implementation and test files match exactly at that bundling boundary. The item
and font source branches are preserved through the second parent of this
integration; they were not merge ancestors of the original weather branch.

The original cumulative base deliberately excluded the separately published
ComboShip transformation/Torch candidate `9702b20e`. The corrected PR #8 now
includes it following the user's explicit request to include all our pushed
ComboShip features. These are OoT-side changes; they do not replace MM's texture
factories. Djipi fonts, sky replacements, rain-splash textures and the evening
O2R ports remain separate asset inputs. No archive bytes are added to the source
integration.

## Verification and remaining coverage

The published `51ad3c1f` baseline passed the gate and Windows/Linux builds in
[Build Artifacts run 35670373874](https://github.com/marsh6487/ComboShip/actions/runs/35670373874).
That run predates the added GUI-font delta.

Local checks on the combined candidate passed:

- GUI-font selection: four configurations, zero failures; archive-before-font
  and font-before-GUI startup ordering. The same test on `51ad3c1f` reproduced
  three missing-font failures before applying the delta.
- MM scene randomization: opt-in defaults, independent toggles, repeated loads,
  and destination audio ordering.
- MM graphics patch bridge: all 13 existing bounds and reload cases.
- clang-format 14.0.6 for the two changed C++ files, and Git whitespace checks.

Fresh full Windows/Linux builds for the combined tree remain pending. In-game
font appearance, pack removal/restart, OoT/MM transitions, and the combined
Alt-assets/weather/audio configuration were not exercised here. Existing
pack-specific runtime limitations remain as recorded in the stabilization notes;
this integration does not claim general pause-crash or archive-mutex resolution.

## Dungeon items and bottles candidate

`poc/mm-dungeon-bottles-20260922` starts from the complete cumulative integration
at `639b526150898cee98ffdd09ccfa6ccd6a40f8ba`. Every retained feature above remains
in its base. Its original item delta adds scoped dungeon-item colours, optional bottle shimmer,
the corresponding Cosmetic Editor controls, a model-pack builder and regression
coverage. The generated Djipi hearts, HD fire, TP torches and TP dungeon/bottle
O2R packs remain separate mod inputs.

The dungeon/bottle candidate is tracked in draft PR #8. Production draw-command
tests and independent review passed locally; full builds and configuration-specific
runtime acceptance must be recorded separately. Its two new options start off
under Cosmetic Editor > Link & Items.

## September 22 cumulative-candidate correction

The original item build's displayed CI merge commit `dae84c8` was the correct
build for PR #8 at head `538039acbd42e2fac3228d6c166606efd20c126d`, but that head
omitted the separately published equipment/sky and transformation/sampling
branches. Its successful build did not establish complete feature coverage.
The corrected candidate includes these three published heads together:

| Published source | Features carried into corrected PR #8 |
| --- | --- |
| `538039acbd42e2fac3228d6c166606efd20c126d` | Full cumulative base, dungeon colours, bottle shimmer and selected TP model porter |
| `36c40fc8b0cdbe09ef66709b5eefb632d4f90da1` | MM Hide Back Equipment and Scabbard; Use OoT Sky Textures |
| `9702b20e71adb55b06435dcdf7b1f3a04fcad9af` | OoT form cosmetic groups/randomization, Zora barrier visuals and HD flame sampling |

GitHub ancestry comparisons confirmed that the cumulative base `639b5261`
already contains every commit from each earlier feature head below (zero
commits behind each head):

| Source branch | Published head |
| --- | --- |
| `develop` | `0d6aba2ae49bc61637804a56a50b4ca2ebe89cdc` |
| `fix/split-mods-paths` | `01e22573239c996cffbdbb6662f42bcc2ced9d0b` |
| `integration/soh-stabilized-20260912` | `cac526a8b9dc08c83f4c2ad7fece0113d284513d` |
| `nei` | `2d260f4f675afebaa35843ebf65474c1de11ff4b` |
| `poc/2ship-scene-randomization-stream-capacity` | `cc1346ceec9060e574cae4c57b89635a4bc2b3d9` |
| `poc/mm-custom-item-colors-20260921` | `e64aff98e4ad20b19267419e497aa488ca8ff3aa` |
| `poc/mm-mod-gui-fonts-20260921` | `075e4e197faa865b5542be5e9d2f6ac96b85e4ae` |

`poc/2ship-weather-item-compat-20260921` points to the cumulative base itself.
Design-only notes, diagnostic probes and unrelated upstream experimental branches
are not additional playable features. Their presence as refs is not an instruction
to enable those experiments in this candidate.

Both MM controls are under **2Ship > Enhancements > Graphics > Mods** and start
off. Dungeon item colours and Bottle shimmer are under **Cosmetic Editor >
Link & Items** and also start off. OoT transformation controls remain on the
OoT side, as described in `docs/poc/2026-09-21-oot-transformation-cosmetics.md`.
The existing evening mod bundle remains compatible with this corrected source.

The two added branches previously passed their full Windows/Linux builds in
runs `35687618130` (equipment/sky) and `35668061992` (transformation/sampling).
The original item head passed run `35704277243`. These establish source-branch
build evidence, not a full build or in-game acceptance of the corrected combined
tree. The corrected PR run and configuration-specific runtime checks are separate.

Local verification of the corrected combined tree passed: equipment/scabbard,
OoT sky (18 production draws), weather and weather/audio bridge, dungeon/bottle
visuals (16 owners, 32 tint scopes, seven bottles), existing custom item colours
(62 cases), OoT custom cosmetics, and Zora barrier cosmetics. The weather runner
used the available nlohmann JSON include path; its first invocation lacked that
local dependency. All 38 feature files from the three source heads are byte-for-
byte preserved, excluding the intentionally combined workflow and integration
notes. Independent integration review and Git whitespace checks found no blockers.
