# Skiejer's Black Market

## Accepted brief

cor approved implementation on 2026-10-17 after reviewing the shop design. This is an isolated source/asset candidate for a later cumulative merge; no master promotion or configured runtime acceptance is implied.

- A hidden, fixed shop in native MM Lost Woods rewards exploration. Its entrance must not replace the accepted corner-stump cross-game return or the onward passage. Its own exit returns to MM Lost Woods, outside the shop trigger, facing the clearing, without touching the cross-game portal route.
- Eight seed-fixed offers: six NEI entries and two wildcard entries. All previously proposed inventory, equipment, individual powers, and wildcards are eligible, not merely the eight illustrative offers. No duplicate identity or alias in one inventory.
- Two offers in tier 1, four in tier 2, two in tier 3. Tiered prices are fixed per item: tier 1 30–90, tier 2 100–150, tier 3 160–200 rupees. Most NEI entries belong in tier 2; premium is reserved for a small influential subset. No Giant's Wallet requirement under tiered pricing.
- Seed settings: Tiered / Follow SoH Pricing, plus an independent dungeon unlock checkbox. With unlocks enabled, tiers open at 0/1/2 unique dungeon clears across enabled games. All eight goods remain visible with their requirements. Rematches and MM resets cannot remove or farm unique clears.
- Merchandise is supplemental. The solver, item pool, sphere/hint generation, and ordinary progression placements must never depend on market goods. A progression-capable market item is a bonus copy, not a relocated required copy.
- Stock and rolled prices use an independent deterministic seed stream and travel with the saved seed. Runtime CVars, save/load, entry, or time reset never reroll them. Old seeds without the feature remain supported.
- Equipment purchases persist across saves, MM resets, and paired-game handoffs. Chateau is a refill, requiring an empty bottle, not an unsolicited new bottle. Already-owned, unpayable, locked, or unusable purchases cannot charge or downgrade the player. Exact skill offers must grant the named power rather than an unpredictable next progressive step. Host tools may follow existing canonical grant semantics; other prerequisites and form restrictions remain explicit.
- Latest interior direction supersedes the initial Bazaar suggestion: Djipi's Zora Shop and Granny's Potion Shop are approved natural architecture alternatives. The retained Zora reference lacks complete geometry; this candidate uses Granny's complete Djipi mesh. Add restrained environmental storytelling where space permits.
- Chamber-of-Sages level replacement throughout the interior: floor, walls, ceiling, counter, shelves, trim and fixtures all use magical scrolling materials. Reduce visual busyness through coherent slow motion and gentle contrast, not ordinary exposed surfaces. Interior must feel otherworldly.
- Shopkeeper is the established goth Treasure Chest Shop Gal, retaining a functional blink/idle and scoped visual resources. No changes to the ordinary Treasure Chest Shop or other NPCs.
- Exterior uses an appropriate Djipi building/entrance recipe with its own black-market treatment, tucked into a side clearing. Preserve native floor/collision, accepted scenery, weather, water, textures, headers and actors except explicit additions.

## Complete eligible merchandise

NEI inventory/equipment: Shovel; Lantern; Whip; Beetle; Gust Jar; Ball and Chain; Fire Rod; Ice Rod; Light Rod; Bomb Arrows; Spinner; Goddess Shield; Kite Shield; Pegasus Boots; Climb Boots; Sage's Tunic; Magic Tunic; Champion's Tunic; Magic Cape; Four Sword; Trident; Cane of Byrna; Mogma Mitts; Deku Leaf; Roc's Feather; Roc's Cape; Roc's Boots; Switch Hook; Minish Cap; Dominion Rod; Hylia's Grace; Phantom Hourglass; Time Gate; Zonai Timer; Shadow Crystal; Demise Destruction.

Explicit powers: Spring/Summer/Autumn/Winter; Slate Remote Bomb/Master Cycle/Stasis/Cryonis/Sheikah Sensor; Somaria Statues/Blocks/Trirod; Pacci Flip/Lift/Ultrahand; Sand/Tornado/Water/Meteor/Storm Rods and Shadow Scepter. Bare host aliases must not duplicate or replace meaningful powered offers. Full-tool bundle behavior must respect the seed's existing mode.

Wildcards: Deku/Goron/Zora/Stone/Great Fairy/Truth/Blast/Gibdo/Garo/Fierce Deity masks, Bunny Hood, Captain's Hat, Great Spin Attack, Chateau Romani refill, magic capacity upgrade, Double Defense, Great Fairy's Sword, Mario Mask and Poke Ball. Crossover prerequisites and feature settings remain respected; unavailable configurations are documented rather than silently deleting an accepted item from the catalog.

## Source and assets

- Integration base: `43131342882972eeb4a257599ec8b061c7dc6d43` (`integration/big-merge-20261016`). The default develop branch is older and is not the implementation baseline.
- Accepted placement dependency: `634757d6d9fe0def4aab4200220121d49d7fb653`, recovered by its source bundle and fast-forwarded locally. Tree `b9cee7b5f05d23933230b3bc15909343b1a596a3`.
- Lost Woods donor: `ComboShip-MM-LostWoods-Right-Nudge-POC9.zip`, game archive SHA-256 `72d750b00583379f3619dac52dce59e2957a351fdfabd3a3a8da11f9bc19daf0`. cor accepted the portal placement and requested mushroom/stump seam repairs be retained.
- Scene-authoring reference: `1- Scene Edit Final - Grottos Shops Houses.prelude.o2r`. Its Zora room currently references donor resources; it is not a self-contained 3D donor.
- Djipi geometry/textures: `Djipi's 3DE - 20 Scenes Map(3).otr` and `Djipi's 3DE - 21 Scenes Map 3DS Textures(3).otr`.
- Shopkeeper donor: `MMShopGal_GOTH_EyeContrast_POC1.o2r`; use a private namespace so the market does not override the ordinary shopkeeper globally.

## Architecture and verification contract

Use a shared, engine-independent catalog/generator/state policy with explicit versioned seed JSON, MM-native shelf/purchase integration, and a dedicated MM scene/entrance registration in unused table slots. Present eight visible item models through a dedicated keeper with native Buy / Next offer / Leave messages, keeping market checks outside the solver. Scene resources and shopkeeper assets use private paths with normal/Alt support. The travel policy owns a separate destination/return transaction, preserves age/form/day/time and unrelated respawns, and cancels stale requests on destruction, file/seed changes, or resets.

Verify real production generator/state code over many seeds; all catalog entries reachable; six/two and two/four/two quotas; pricing and independent RNG; malformed/old seed behavior; purchase atomicity and permanence; cross-engine dungeon-clear reporting; exact named grants; native menu/callback routes; return position/heading, void/re-entry and save/reload safety. Validate donor closure, material coverage and animation, collision and namespace pairing; render centralized previews and inspect them. Full linked builds and configuration-specific gameplay remain separate evidence and must be reported honestly.
