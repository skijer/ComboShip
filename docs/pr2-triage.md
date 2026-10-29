# ComboShip PR #2 triage

PR #2 (`skijer/ComboShip`, head `ffa312d4`, 360 of its 525 commits by marsh6487) mixes four kinds of work. Each feature below keeps the ID from marsh's audit bundle (`ComboShip_Integration_and_Bugfix_Audit.json`), which holds the full description, paths, symbols and acceptance checks for every entry.

Read any PR file with `git show ffa312d4:<path>` in a ComboShip clone that has fetched `refs/pull/2/head`.

| Bucket | Goes to | Who |
| --- | --- | --- |
| A. Fleet combo | this repo plus the games' `fleet-rando` RandoRequest services | Claude |
| B. Game fixes | Shipwright-unbound / 2ship-unbound-modapi | Claude, one by one after checking Unbound still has the bug |
| C. New mods | `mod-sdk/mods/` in Shipwright-unbound-mod-nei and 2ship-unbound-mod-nei | Codex, branch `codex/pr2-mods` in each repo |
| D. Already covered | nothing, or a bugfix delta into the existing NEI mod | Codex, same branch |

## A. Fleet combo

The engine under `combo/rando/` is already here, unchanged. What is left is the game side it calls, and the Fleet host.

| IDs | Feature | State |
| --- | --- | --- |
| — | Oracle, seed dump, Link's Pocket, settings dump and restore (WU148) | Done |
| — | Goal: bosses or combined Triforce Hunt; starting game OoT or MM | Pending |
| — | OoT entrance shuffle, dump and replay of its layout | Pending |
| WU057–WU058 | Vanilla/MQ dungeon layouts and exact replay | Pending |
| IT109–IT126 | Shared item families (bows, bags, magic, wallets, hookshot, arrows, lens, songs, masks) | Pending: generation, then runtime tier sync in the host |
| WU043–WU053 | Cross-dungeon, boss, interior, grotto and shop entrances across games | Pending: catalogue, exits and runtime warps |
| WU147 | Cross-game hints | Pending |
| IT127–IT130 | Cross-world grant correctness (dormant banking, wallet safety, MM no-effect grants, deterministic foreign junk) | Host runtime |
| WU054–WU056, CO016 | Combo/OoT/MM launcher modes, solo save namespaces, two-game save container | Host runtime |
| WU121–WU124 | Config import, settings sync, Time Splits and trackers | Host UI |
| CO014 | Headless seed smoke in CI | `fleet-seedgen` job |
| CO004, CO017 | Per-game mod roots and owning-game resource routing | Already in libultraship `GameRegistry` |
| CO001–CO003, CO006–CO013, CO015 | ComboShip's monorepo build, AppImage, Torch/cumulative assets, upstream pin automation | Not applicable: each game builds and extracts on its own |

## B. Game fixes

Candidates for the games themselves, not mods. Each needs a check that Unbound still has the bug before porting.

| IDs | Game | Feature |
| --- | --- | --- |
| RW050–RW056 | both | Audio: permanent-cache guards, malformed Ogg/Opus rejection, exact-length PCM, MM refill independent of rendering, OoT graphics/audio handshake, SSE2 mixer high-lane fix, gain-command overrun |
| RW049, RW057 | MM | Streamed sequence catalogs; clean streamed positional music |
| RW066–RW067 | MM | Cosmetic patch bounds; selective palette cache invalidation |
| RW068 | MM | Mod-provided fonts (a ModApi feature) |
| IT142 | both | Current tunic/boots markers on new files |
| IT143–IT147 | both | Receipt and tutorial text: bottles, swords, shields, dungeon keys, stray fairies, bank source, Tycoon wallet width |
| IT100–IT102 | both | Boss soul draw safety and receipt colors |
| RW070–RW072, RW076 | libultraship | Renderer performance. Submodule: needs its own exception before touching |

## C. New mods (Codex)

Each becomes a self-contained ModApi mod with its own assets. Credit marsh6487 in the mod manifest and README. Follow the existing mods in `mod-sdk/mods/` for layout. Keep MM-native behaviour on the MM side: no OoT reimplementations of things MM already has. A mod that needs something the ModApi lacks says so in its README instead of patching the game.

| Mod | IDs | Games | Notes |
| --- | --- | --- | --- |
| `sword_enchantments` | IT068–IT075 | both | Din fire shield and sword, fire/ice/light imbue, per-sword chooser, B badge, auras |
| `spin_attack_styles` | IT076–IT085 | both | Selectable spin themes, native accents, elemental spin ownership |
| `ice_arrow_floes` | IT086–IT087 | Ship | MM-style ice floes on water, freezable Octoroks |
| `black_market` | WU023–WU042, WU135 | both | marsh's shop scene, registered through the ModApi scene registry. Stock is deterministic per seed: needs a Fleet seed hook later, so ship it with a local seed first |
| `magic_mirror` | WU011–WU017, WU136–WU137, WU019 | both | Travel tool, seal ritual, recall landing, rando shuffle, Time Gate chest |
| `sages_chamber` | WU020–WU022, WU138–WU142 | Ship | Chamber scene and five-route travel |
| `story_actors` | WU059–WU083, WU128–WU134, WU084–WU090, WU149–WU153 | Ship (+MM poses) | Static NPC poses, native NPC rig adapters, pack compiler tooling, optional fitted packs |
| `midna_companion` | WU091, WU143–WU144 | both | Navi/Tatl replacement with per-event audio |
| `epona_cosmetics` | WU096–WU097 | both | Adult/young Epona, young Epona riding |
| `emotes` | WU098–WU107 | both | Eight emotes, chords, Anchor/Harpoon pose sync |
| `time_pedestal` | WU154–WU155 | Ship | Young pedestal ceremony |
| `location_title_cards` | WU156 | MM | OoT-style title cards |
| `bottle_presentation` | IT091–IT095 | both | Bottle casing and contents composition |
| `boss_soul_presentation` | IT096–IT099 | both | Animated boss soul pickups |
| `seasons_weather` | RW010–RW048, RW017, RW069 | both | Rain, thunder, seasonal foliage, particles, aurora, MM sky. Large: split if needed |
| `prelude_materials` | RW073–RW075 | Ship | Scroll bindings for Prelude surfaces (Prelude permission already granted) |
| `voice_pack` | WU146, AU001 | Ship | Nested PAK discovery and persistent PAK selection; may belong in the existing `pak_loader` |
| `hyrule_field_night_music` | RW063 | Ship | Replaceable night track |
| `stat_pickups` | IT131–IT138 | both | Port of upstream Shipwright PR 6760. Hold until upstream decides; then a mod only if upstream rejects it |

Combo-only world features wait until Fleet can move the player between games. They become Fleet mods then, not now:

| IDs | Feature |
| --- | --- |
| WU001–WU005 | Lake Hylia ↔ Great Bay, Graveyard ↔ Ikana, Fountain ↔ Snowhead passages |
| WU006–WU010, RW027, RW032 | Native Lost Woods scene and its forest art |
| WU053 | Farore recovery across shuffled routes |
| RW078 | Moon Tear foreign animated material |

## D. Already covered

Inherited NEI work that the Unbound NEI mods already carry. Only bugfix deltas move, into the matching existing mod.

| IDs | Existing mod |
| --- | --- |
| IT001–IT021, IT023–IT041 | The item mods of the same name (`elemental_rods`, `elemental_wand`, `sheikah_slate`, `somaria`, `cane_pacci`…) |
| IT022, IT149–IT160 | `useful_medallions`, `oot_items` (MM spells, medallion spells, elemental arrows) |
| IT042–IT044 | Hylia's Grace (Off/On/Gated seed policy is new) |
| IT045–IT048 | `spiritual_stones` |
| IT049–IT067, IT106, IT108 | The equipment mods and `easy_equipment` (ownership, C/D activation, borrowed slots) |
| IT088–IT090 | GI and held presentation inside each item mod |
| RW001–RW009, RW082 | `rod_of_seasons` |
| RW060–RW062, RW080 | The form mods (MM SFX and instruments in Ship) |
| WU018 | `time_gate` |
| WU092–WU095 | `wolf_link` |
| WU108–WU120 | Save editor controls: each mod registers its own editor entries |
| IT041, IT148 | Known boundaries, documentation only |

Not ported: IT139, RW048, RW058 and RW081 are diagnostic probes; RW064–RW065 extend ComboShip's cosmetics editor, which the Unbound games do not share.
