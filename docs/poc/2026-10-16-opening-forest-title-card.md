# MM opening-forest OoT title card

| Field | Record |
|---|---|
| Baseline | `1bbb94e6`, isolated branch `fix/shop-forest-entrances-20261016`; existing opening-forest exploration predicate and native MM title-card renderer. |
| Candidate | `ComboOpeningForestTitleCard.cpp`; no title artwork or scene/archive resource is changed. |
| Scope | One centered OoT Lost Woods place card per ordinary MM `SCENE_LOST_WOODS` spawn-zero/layer-zero arrival. |
| Preservation | No save/history/story flags changed; title demos, Song of Time flashbacks, alternate spawns/layers, and native title suppression remain excluded. Existing title cards are not replaced. |
| Resources | Exact `textures/g_pn_36/gLostWoodsTitleCardENGTex`, drawn through aligned `__OTR__@oot:` by the existing shared renderer. OoT's own Alt setting selects installed Djipi art with native fallback; MM's Alt setting does not change OoT's independent choice. Binary resources and `.meta` aliases are recognized. Missing donor/resource safely skips the card. |
| Lifecycle | Waits for native incoming `TRANS_TRIGGER_END`, player entry control, pause, game-over and cutscene control to settle; cancels on outgoing `TRANS_TRIGGER_START` or play destruction. Native MM title context supplies update/fade/draw, with OoT place-card position `(160,120)`, size `144x24`, and 20-frame delay. |
| Evidence | Production-path title test failed first on missing arrival behavior, then passed with normal and ASan/UBSan builds. It executes the complete enhancement, native MM title init/update/draw, and real MM save types. Review identified native incoming-fade and pointer-alignment constraints; both received failing regression tests before their fixes. Production C++ object compiled against real headers. The existing cross-game texture-routing suite passed. |
| Integration | Entrance suite passed before concurrent return-anchor changes. The latest run stops at `leaving and re-entering the opening arch must return to the pink Fairy tunnel once`; its old coordinates need the separate return-anchor task's update. Title tests and production compilation remain passing. |
| Runtime | Untested in game. No bundled Djipi card was located in the provided environment archive; no replacement art was generated. |
| Recovery | Remove the one enhancement translation unit and its scoped tests to restore the title-card baseline. No master promotion or pack rewrite is part of this candidate. |

The decisive runtime check is a Fairy tunnel arrival into the playable opening forest with OoT Djipi/Alt enabled, then disabled. Check native/HD appearance, fade, re-entry, title-hide preference, and a Song of Time flashback. Static tests prove lifecycle and routing, not the visual contents of an installed pack.
