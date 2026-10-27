# MM pause/equipment regression candidate — 2026-10-08

| Field | Record |
|---|---|
| Baseline | PR39 head `35ce5f22`; supplied Windows runtime logs identify test merge `f069ea6`. Baseline preserved at `origin/bridge/combined-recovery-20261008`. |
| Candidate | `fix/mm-pause-inventory-20261008`, parent `35ce5f22`. |
| Scope | MM pause arrow/grid and L access, shared custom item C/D assignments, unearned equipment display and Four Sword pickup/held ownership. |
| Preservation | Earned ownership and randomizer checks; all cumulative weather/Farore/scene work; other custom Din swords; upright receipts and original shelf presentation. |
| Configuration | Supplied three logs; seeds `3414832249` and `1316389493`; MM human/Mario skin seen; actual vanilla/Alt pack stack from archive logs. |
| Evidence | User reports pause lockout, missing scepter/Rod assignments, unearned vanilla/additional equipment, Din Four Sword GI and invisible held sword with Alt on/off. Source trace confirms L nested inside grid-only branch; empty first page rejects arrow entry. Transfer scans only native inventory and loses EXT IDs. Seed1316389493 additional ownership remains zero until manual grants at 11:04:24/31. |
| Verdict | Four scoped source fixes implemented with failing-baseline/passing-candidate production fixtures. Combined integration verification is finishing. Push is held at the user's request; runtime remains untested for candidate. |
| Recovery | Isolated diff from preserved baseline; package to contain patch, changed sources, test commands and runtime checklist. |

## Confirmed failures and changes

1. MM handled L only while the cursor was already in the item grid. Its arrow-entry scan sent an empty page to the opposite arrow. L now runs inside the existing item/idle guards before the grid-only branch, and an empty scan permits entry at the corresponding grid edge. Wheels, descriptions and equip animations retain input ownership. A page switch returns before a simultaneous equip press can act on the old item.
2. Button departure payloads truncated custom u16 items, and arrival ownership searches ignored NEI inventory. Both hosts now extract resolved button IDs, accept only shared full-ID mappings, find native or NEI-owned cells, and update marker/shadow/slot together. MM C/D slots use the native shared form-0 convention. Unsupported destination-local items and unowned incoming items are preserved. The shared spell/rod acquisition rules are unchanged.
3. MM drew and allowed hovering of unowned vanilla and NEI equipment cells. Display, cursor entry, titles and sync publication now use acquired ownership. Borrowed sword/shield model nibbles do not prove ownership. Real native starter equipment remains visible and its proven ownership is retained when actually replaced; receipt/applied floors advance only to the native sword tier already materialized, preserving pending higher grants. Incoming shields preserve genuine starters and keep imported skins/NEI bases coherent. Browsing does not mutate acquisition state.
4. MM incorrectly aliased Four Sword to generic Kokiri/Din GI roots. Custom-body/native-equipment gates and late OoT custom/PAK hooks could also suppress its held injection. Four Sword now retains its actual authored GI and reaches its existing complete-graph held getter on the standard child/adult/human rigs. The final OoT stage preserves an independent compound per deferred player/clone draw. Ordinary swords, transformation guards, authored meshes, grip matrices and pickup/shop framing retain their existing paths.

Equipment-specific provenance evidence is in `equipment-ownership-20261008.md`; Four Sword routes and controlled coverage are in `four-sword-visibility-20261008.md`.

## Verification boundaries

| Dimension | Evidence | Status |
| --- | --- | --- |
| MM pause input | Real cursor function and ExtInv mapping/switching; empty pages and both arrows, page cooldown, wheel/description/equip guards, simultaneous L/C | Baseline fails; candidate normal and ASan/UBSan pass |
| Shared button assignments | Actual both-host extraction/application and ExtButton helpers with native save structures; wand and Rod of Seasons, C/D, transformed MM, missing ownership, local items and malformed JSON | Baseline fails; candidate normal and ASan/UBSan pass |
| Equipment acquisition | Actual draw/cursor/action/slot/sync writers and JSON incoming shield fragment; fresh/earned loadouts, borrowed model values, native starters, pending grants and borrowed-base coherence | Baseline fails 153/373; final ASan/UBSan passes 373 checks |
| Four Sword routing | Actual getters/selectors, MM bridge and limb stages; both Alt states, owner dependencies, native/custom bodies, late hooks and clone compounds | Baseline fails; focused candidate checks pass |
| Actual headers | Both complete FleetSync translation units, MM equipment/adult/GI sources, actual MM player unity and real sword fit/GI helper include paths | Passed; final MM sync syntax rerun after review repairs |
| Repository integration | Formatting, asset collisions, shared-item/grant/dungeon/reward checks, accepted NEI gate and fresh read-only review | Complete: all 83 canonical commands passed, plus collision/shared integration checks and an independent review with no blocking findings |
| Linked executable | Full application object compilation/linking and Windows/Linux CI | Not run; push is held at the user's request |
| User configuration | Installed archive stack, controller and actual game/GPU draw | Not yet demonstrated; no runtime acceptance or master promotion |

Local ASan/UBSan runs disable leak detection because this host's LeakSanitizer rejects ptrace. Address and undefined-behavior checks remain enabled; executable CI runs in its own environment.

## Installed-stack acceptance checklist

- Start a fresh seed: show real starting gear, hide unearned vanilla and additional equipment, then collect one of each and confirm only those cells become available.
- In MM, enter an empty inventory from both arrows and cycle all pages with L. Exercise a populated page and an open item wheel/description. Equip the scepter and Rod of Seasons, switch OoT/MM both ways, and confirm C/D assignments and usable held items survive without creating ownership.
- Receive, hold and swing Four Sword with the reported Din/skin packs installed, Alt off/on after resources warm. Check OoT child/adult, MM human/Time Gate adult and clones. Confirm pickup/held identity, ordinary Din sword variants and existing shop framing.

The preserved parent is PR39 head `35ce5f22f11c845a4ef0b28228880547e14ee92d`. The recovery package contains a full-index patch for cumulative merging, changed files, verification record and reverse-apply instructions. No push or PR is authorized in this turn; the user explicitly requested the recovery ZIP and holding the push.
