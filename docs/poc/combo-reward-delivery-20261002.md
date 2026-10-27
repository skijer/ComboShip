# Combo reward delivery and failed item/rendering candidate recovery

Candidate: PR #33, `poc/mm-native-oot-soul-flame-20261002`.
Source baseline: `fd9bb36b44094dd8838fb59e71fbf2c6432c0488`, tree
`18798a1d57c4165a0afc9ef645972d2a12adae05`.
The isolated local checkout had the same tree at `fd648f64`; its local commit
identity differs from the published commit. The previous working PR #31 branch
is preserved. cor's runtime report concerns an older working build, not the
failed latest candidate; the supplied log does not establish its exact commit.

Evidence: `Fleet of Harkinian(20261002-205232).log`, SHA-256
`f342df50f6a1cd05757363a2f92932ecc5b1a8e22a749a76cc86147c5b19fbb0`.
The reported run uses seed `4231192002`; the file also contains an earlier seed.
The supplied screenshot shows the Ice Arrows icon beside Bottle with Blue Potion
in an MM get-item message.

## Traces and corrections

| Report | Source/evidence | Correction |
| --- | --- | --- |
| Filled blue-potion bottle shows Ice Arrows | OoT's randomizer filled-bottle entries carry a RandomizerGet value in `itemId`. The cross-game icon export falls back to native `gItemIcons[itemId]` without an explicit custom icon. | Bind all nine randomized filled bottles to their actual native bottle-content icon resources. The MM message consumer retains its existing owner routing and pre-grant staging. |
| Market wonder-item magic is unavailable in OoT until switching | At 15:45:53, the MM dormant native grant converts Progressive Magic to Single Magic and writes its native magic floor and acquisition flag. The MM relay returns early for RPG magic, skipping the corresponding OoT update. | Relay the resolved native tier as an absolute capacity floor immediately in both directions, update the resident save/HUD fields, and persist. Repeated/lower tiers do not refill, downgrade, or generate another progressive pickup. |
| Native magic can be misinterpreted in RPG mode | Fractional RPG magic can set both HUD ownership flags before either native magic pickup. | Shared-tier queries and dormant OoT progressive-magic resolution use saved native pickup levels in RPG mode, retaining flag-based vanilla behavior otherwise. |
| Chateau Romani refuses use in OoT | The bottle behavior table says CANT_USE even though the extended player action already aliases Chateau to the blue-potion drink and bottle consumption already activates the Chateau magic buff. | Enable native use, allowing the existing drink, bottle-emptying and infinite-magic consume hook to run. |
| Wonder-item notifications appear only after entering MM | MM's native give choke queues its pickup toast even when `gComboDormantGive` is true. The inactive MM notification queue advances only in MM. | Keep the dormant receiver silent. Both foreign finders emit the normal “You found” toast with the resolved item name in the collecting game. Preserve bank attribution. |

The log proves those MM-side rewards were written before the switch, including
Snowhead Stray Fairy, Single Magic and Moon's Tear. It does not prove they were
lost until the switch. It contains no OoT-side item audit or foreign-delivery
logs, so the active OoT corner display still needs direct runtime verification.

Hover Boots remains unconfirmed: cor describes a separate run and is unsure
whether it was a starting item. The supplied trace does not identify a Hover
Boots acquisition. No inventory or starting-item rule is changed on that basis.

## Failed candidate build recovery

Original PR #33 run:
<https://github.com/marsh6487/ComboShip/actions/runs/37060452996>.
Its gate failed before either application build started: the MM item-color
syntax check could not find `ComboMaskShimmer.h`. The new renderer legitimately
uses that shared header, and both application CMake targets already include
`combo/menu`. Add the same include directory to both MM and OoT item-color
diagnostic compilers; the OoT counterpart was also missing it.

Preserve the cumulative boss-soul model/flex-skeleton/jaw work, native MM flame
conversion, mask/remains shimmer, Great Spin GI, keys/emblems, bank attribution,
RPG rules, item provenance and audio changes. No model/texture archive or pack
order is changed. The accepted working baseline is not promoted or overwritten.

## Verification

Local production-body fixtures pass:

- Reward delivery normally and under ASan/UBSan: immediate native magic floors
  in both directions, RPG fractional capacity, native-tier queries, repeated and
  lower-tier suppression, invalid/title saves, receive guards, Chateau drink
  eligibility/consumption and live versus dormant notifications. Native pickup
  notification behavior is checked with and without COMBO_BUILD.
- GI/icon/cane presentation normally and under ASan/UBSan: all nine filled-bottle
  bindings against a colliding native icon number, MM blue-potion message owner
  routing, resolved-tier freezing, pointer lifetime, native icons, dimensions and
  formats, and all 64 cane ownership masks.
- Existing cross-grant and shared-item integration checks; complete provenance
  capture/tracker, C ABI/stock-build and preserved native-grant checks. The MM
  shared-tier audit fixture compiles the production RPG native-tier helper and
  distinguishes fractional HUD flags from native pickups. RPG math, wire merge,
  MM save migration, OoT pool/pickup and idempotent return checks.
- Both complete native item-color draw translation-unit syntax checks and their
  draw-command fixtures; native MM item visuals.
- Both engines' soul/flex-skeleton, mask/remains shimmer and Great Spin fixtures
  under ASan/UBSan; these retain the prior rendering candidate's behavior.
- Asset collisions, clang-format 14 for changed game code, and whitespace.

Local LeakSanitizer is disabled because this container prevents its process/thread
inspection; the sanitizer claims cover address/undefined behavior only. Full
Windows/Linux application builds are tracked by PR #33 CI. These fixtures do not
boot a ROM or prove scene pixels, HUD timing, or user acceptance.

## Decisive runtime checks

1. On the reported seed/settings, collect the Market checks above the Bombchu
   shop while OoT remains active. After the magic GI/text closes, verify the meter
   is visible and usable before switching; verify one corner notification per
   check. Switch to MM and back and confirm ownership persists without replaying
   those notifications or adding/refilling a native magic tier.
2. Drink Chateau in OoT after acquiring magic. Confirm the usual drinking action,
   empty bottle, restored magic and existing infinite-magic behavior; check both
   vanilla and RPG magic configurations.
3. Receive a blue-potion filled bottle in MM from an OoT-owned placement. Confirm
   the blue-potion icon, inventory content, dialogue and held-up model. Spot-check
   the other filled-bottle contents.
4. Retest the same Trading Post Volvagia shelf and TP pack order, attached jaw,
   nearby items, re-entry, independent owner Alt selections and acquisition.
   Check mask/remains shimmer and Great Spin presentation in both engines.
5. If Hover Boots recurs, retain that run's seed/spoiler, initial inventory and
   both engine item-audit logs so its actual grant origin can be distinguished
   from an intended starting item.

Runtime acceptance remains pending. No merge or master promotion is authorized
by static checks or a successful application build alone.
