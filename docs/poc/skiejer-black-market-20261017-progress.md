# Inline implementation ledger — Skiejer Black Market

Plan: `docs/superpowers/plans/2026-10-17-skiejer-black-market.md`.
Branch: `poc/skiejer-black-market-20261017`.
Initial integration base: `43131342882972eeb4a257599ec8b061c7dc6d43`.
Accepted visual dependency/base for feature diff: `634757d6d9fe0def4aab4200220121d49d7fb653`.

## Progress

- Read accepted conversation, current integration source, accepted POC9 source/assets, donor scene-authoring archive, gothic keeper archive and relevant native shop/scene APIs.
- Recovered POC9 through its authentic bundle and fast-forwarded the isolated local branch; no shared branch changed.
- Native scene `SCENE_UNSET_02` and entrance `ENTR_SCENE_UNSET_09` have no existing owner; Fleet Limbo owns scene 01 / entrance 08 and must remain intact.
- Baseline entrance suite currently cannot compile because the container has no installed spdlog headers. This is an environment prerequisite, not a passing baseline. Use vendored headers/dependencies and rerun before interpreting test results.
- User steering: prefer Djipi Zora Shop or granny's potion shop architecture over Bazaar. Zora and Granny are approved donor alternatives, subject to complete retained geometry. Every interior surface remains magical scrolling; full agreed catalog remains eligible.
- Implemented the shared production catalog, independent deterministic RNG, 6 NEI / 2 wildcard and 2/4/2 tier quotas, fixed tier prices, all six SoH pricing modes, strict seed validation and purchase policy.
- RED: stock test could not compile before the production header existed. GREEN: 4,096 complete-catalog seeds cover every catalog item, quotas, pricing independence, validation, unlocks and purchase persistence.
- RED: parity fixture found an incorrect cheap-price CDF boundary. Corrected the CDF and free-wallet RNG consumption. GREEN: 2.4 million generated prices match the actual SoH pricing bodies across modes, wallet weights and affordability settings.
- Donor audit: retained authoring archive has only a Zora Shop room reference; retained Djipi map pack includes full Granny and Chamber of Sages geometry. Do not claim a full Zora mesh until dependency closure is verified.
- User steering: a concealed building threshold opening into a magical natural alcove is acceptable; add a visible magical cue at the doorway. Architectural continuity is secondary to a deliberate magical transition.

## Decisions

- Title-card steering: keep the shop native MM if both destinations can show centered OoT-style cards. Reproduced a direct-load suppression: MM file select normally initializes `showTitleCard`, but Combo loads bypass it and the opening-forest prologue guard returns before its reset. The old fixture explicitly initialized the flag true, hiding this case. A new direct-load regression failed on the original enhancement and passes (including ASan/UBSan) after using an eligible default for the existing hide-title-card hook. No native save/story/title flag is rewritten. This is source/lifecycle proof, not a claim that the user's latest binary has been tested in game.

- User explicitly approved implementation; planning/review files document that accepted scope without asking for repeat approval.
- A fresh clone/feature branch provides source isolation; do not create a nested checkout or alter the integration branch.
- Keep all exact donor files untouched under `asset-baselines/`; outputs are separate candidates.

## Implemented candidate

- New shared menu controls and GUI/headless seed writers; options and offers are baked into the consolidated seed. Reload rejects malformed offers; older seeds remain opt-out. Display hashes include the market options, and headless settings reproduction retains them.
- Independent actual boss-defeat callbacks for both games, restricted to the matching native dungeon arena. Twelve unique source-dungeon bits persist in the combined container; shuffled rewards and repeated MM-cycle kills do not increase the count.
- Dedicated MM scene 02 / entrance 09, native goth keeper idle/blink, eight visible fixed item models, magical seals, native message choices and fixed-identity purchases. The first candidate browses offers through Buy / Next offer / Leave; it does not replace or modify the Trading Post's controller.
- Verified 79 catalog identities exist in MM's actual item registry. Exact Statues avoids the progressive base grant; bundles use native rune/season/rod grant APIs without developer toggles or free medallions. Refill requires an empty bottle. Equipment ownership and purchased bits prevent repeat charges.
- Actual rupee balance, item and shelf bit enter the same MM snapshot. Added seed/slot-scoped local return, stale-request protection, void/reload recovery and a saved-inside resume marker applied before explicit dungeon/Farore arrivals.
- Asset donor is Granny's complete Djipi geometry, one of cor's accepted alternatives. Zora's retained room is not a closed mesh donor. Private game resources include 28 fully replaced scrolling materials, the goth keeper's complete model/idle/blink dependency closure, native collision and centered IA8 shop title. Exterior is a cropped Djipi entrance shell drawn independently of the POC9 forest mesh.
- The texture uses the built-in image-generation tool, then standard engine encoding/resampling; its original PNG and prompt are retained with the source. No ordinary room textures remain in this candidate.

## Verification so far

- Stock/purchase policy: 4,096 seeds; all 79 catalog entries represented; exact quotas and independent pricing; state isolation and malformed data rejection.
- Price parity: 2.4 million outputs compared with the production SoH pricing functions.
- Native transaction/recovery fixture: ASan/UBSan pass for charge/grant/persistence, failed grants, pending rupee deductions, locks, stale slot/seed, TOP preservation, missing assets, void and resume recovery.
- Complete affected Lost Woods suite passes. Its old handoff fixture targeted the obsolete POC2 stump; it now asks the production anchor policy for the current destination. New market calls are explicit boundaries in the older boot/save fixtures.
- Whole native MM market, MM item adapter, OoT clear/settings adapter, headless generator and launcher compiled to objects with native headers. Re-run after final edits. The complete GUI translation unit needs additional zip headers; compile the changed GUI section against native ImGui separately.
- Private pack contract and real geometry preview run; final packing/preview and fresh code review remain. No full executable link, Windows build, installed configuration gameplay or user visual acceptance has been performed. Do not merge/promote on these source checks alone.

## Final review and checkpoint limits

- Fresh source review caught a C++ hex-escape bug in the choice message. Fixed the literal boundary and added the actual production `MarketText` body to the native fixture, checking its three-choice control and labels.
- Exact named cane skills now bypass the progressive FC alias. Both native FleetSync implementations OR-merge exact `caneSkills` and grant only new bits. The legacy progressive cane FC counter is acknowledged without replay, including out-of-order counter/mask deltas, preventing an ordinary pickup from granting a second unwanted skill. Regression executes actual grant, both native merge blocks, and both full FC materialization functions; unrelated FC items still grant once.
- Final asset pack is `zzzz_Skiejer_Black_Market_POC4.o2r`: 186 private resources; 28 scrolling materials. Scene spawn is `(0,0,30)` with native params `0x0EFF`, clear of the front door exit region. Asset test compares packed scene bytes with production scene resources. Earlier internal POC1–3 packs are superseded and must not be installed with this one.
- Central asset preview inspected. It is a geometry/material preview with a title overlay, not a gameplay capture; NPC, shelves and collision behavior require native runtime validation.
- User explicitly approved publication and source checkpoint to marsh6487/ComboShip on 2026-10-17, while reserving other changes before integration. Preserve this branch; no merge or master promotion.
- Outstanding: full executable link/Windows build; configured playthrough testing of interaction, all special grants, whole-room scrolling, keeper idle/blink, both place cards on direct load/entry, doorway placement, exit/re-entry, void and saved-inside resume. Source fixtures do not establish those runtime results.

- Final targeted object compilation passed: native MM shop, MM grant adapter, both FleetSync translation units, OoT clear/settings adapter, headless generator and launcher. Changed market GUI fragment compiled against the actual ImGui/CVar/style headers; the full GUI TU was not compiled. All current stock, pricing parity, native ASan/UBSan, exact-cane/FC and title ASan/UBSan checks pass; the complete affected Lost Woods suite passes. Fresh focused review reports no remaining Important findings in its fixes. No complete application link is claimed.
