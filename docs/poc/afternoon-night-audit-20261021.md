# October 21 afternoon source integration

Combine the afternoon and evening source fixes into `marsh6487/ComboShip:develop`.
The rollback baseline is `66bde021ccd651a3728db2338251961b1a44c9f7`.
Executable builds remain held at cor's request. This source merge does not
declare configured gameplay or a runtime master accepted.

## Included sources

| Source | Exact input |
| --- | --- |
| Cumulative roundup, Black Market collision and Saria receipt fixes | PR53, `724e0476b1dd2555f40faf7cc01b8d1cf0329ecd` |
| OoT Mirror arrival crash and green recall ritual | PR54, `f65774c8886e467e6ee1fdff38ce45b8150a3e11` |
| Native forest presentation and stump-cap return | PR55, `fdc2c7038c85db8b73adf95078417a95bcfc213d` |
| Cross-game title cards | `628458857325fc8af1b91873111cd3c7cc53218e` |
| Native Great Fairy access without a scene mod | Restored source `899116bfaa604f6adcfa413b7227dbeace995187` |
| Moon-tree depth repair tools | The exact two standalone Python scripts and their source notes |

## Behavior addressed

| Area | Source change |
| --- | --- |
| Saria and song giver | Retain the dialogue-arrow correction; terminate the song receipt with MM's native END marker to prevent decoder overrun after the reward. Preserve ordinary, teaching, repeat and cutscene-actor dialogue. |
| Black Market | Retain spawn normalization and increase the private shop collision pool to 40,244 nodes for the measured 34,490-node requirement. |
| Mirror arrival | Initialize the Combo cutscene manager before entering a private MM scene without a cutscene list, while retaining ordinary scene overrides. |
| Mirror ritual | Preserve Farore's cast motion and timing; replace the overhead Mirror GI with the existing seal, wisps and textured energy, all green. |
| Mirror collection and return | Keep pale-blue/white collectible twinkles, physical stump-top collection, and recall to the actual stump cap with guarded ground fallbacks. Preserve input, confirmation, save/seed/slot and Farore/grotto ownership. |
| Great Fairy | Use room 7's native north tunnel without a scene mod and return that visit to the same grounded approach. Preserve ordinary/shuffled visits, moved authored entrances, save/reload and the original private-forest portal. |
| Woodfall | Use room 7's right tunnel to enter Woodfall's normal exterior start, with its dry return opening and progression guards. |
| Title cards | Show OoT location title cards on cross-game returns, including both forest bridges and shuffled arrivals, while preserving ordinary intro and Time Saver behavior. |
| Forest presentation | Match the private portal and full-face twinkles to purple `#581880`, remove the detached bark bloom, swap the hanging signs, and preserve the avatar poster. |
| Ramp and actors | Preserve ramp geometry/collision, stabilize its material pipeline, move Skull Kid to the selected ledge, and retain the marked Saria/Epona placement and dialogue guards. |
| Light sheets | Provide the opt-in Lost Woods XLU `0A` scroll binding; authored sheets select `0x0A000001`. Existing native bindings, textures and UVs remain protected. |
| Moon tree tooling | Preserve the bounded depth-bit repair and checker as source tools. A game source merge does not apply an archive repair. |

## Source-only publication boundary

At cor's explicit direction, this integration adds no recovery ZIPs and no
optional blouse, foliage, model or scene-archive inputs. The supplemental
`tools/recovery/20261021/` packages and new `docs/checkpoints/` entries are
excluded. Existing baseline files remain intact. Recovery artifacts are
retained separately; the source merge does not install appearance packs.

The runtime, test, asset-preparation and standalone repair-tool bytes match
the previously reviewed combined implementation. Only publication metadata
and the excluded recovery packages differ from that saved candidate.

## Verification and limits

The saved combined-tree verification includes the complete entrance/title-card
runner, crash/assets preflight, all 29 world tasks, used-magic effect tests,
13 native production object compilations, 21 CI-policy tests and independent
source restoration/review. The native forest suite recorded 52 passes and two
optional original-donor skips, with the real authored donor exercised.

Publication checks compare every included source/test file against that exact
verified tree, preserve all baseline workflow blobs and all 203 original
regression task objects, and confirm the manifest's two additions. The final
GitHub source tree must match the local source-only candidate. These identity
checks reuse the recorded test evidence; they are not a fresh full build.

Full executable linking, hosted CI for this integration, configured gameplay
and visual acceptance remain untested/held. Archive-only Malon, canopy, water
and Moon-tree appearance candidates require their separate artifacts.

## Focused runtime follow-up

1. With zero mods, round-trip room 7's north tunnel through the Great Fairy;
   save/reload inside, then check Woodfall and the original forest portal.
2. Complete ordinary/reward/repeat Saria dialogue and a talkable cutscene
   actor; enter, browse, leave and re-enter the Black Market.
3. Recall immediately from OoT after launch, then from MM. Check single
   confirmation, Farore's motion, the green ritual and stump-cap arrival.
4. Return through both forest bridges and a shuffled entrance; check the OoT
   title card. Inspect signs, actor positions, portal and ramp with Alt on/off.
