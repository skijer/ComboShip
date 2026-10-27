# Always Thawed Zora's Domain — source candidate

## Baseline and scope

Parent: `520d7b6dc64ab5742226b9b2f998a34bcd6b4a9e`, the prepared
`integration/weekend-audit-20261018` source from
`ComboShip-Weekend-Merge-Ready-20261018.zip`. This is an isolated addition for
the cumulative merge. The weekend candidate's own build/runtime holds remain;
neither it nor this change is promoted to an accepted master.

cor requested an immersion checkbox that keeps adult Zora's Domain thawed,
using the child water and its installed texture replacements, without changing
story flags or randomizer logic.

## Behavior

- **Enhancements → Graphics → Mods → Always Thawed Zora's Domain**, off by default.
- Config key: `gEnhancements.AlwaysThawedZorasDomain`.
- The existing `Bg_Spot07_Taki` actor draws its native flowing pool/waterfall
  resources in both ages and removes only its corresponding ice collision.
  Its state updates collision and rendering together when the checkbox changes.
- Adult caustics use the child animation rather than the adult frozen phase.
- This choice outranks seasonal freezing/draining within the Domain. Waterboxes
  remain swimmable, and winter's automatic ice-walking pin is omitted here.
  Explicit Roc Boots/Garo water-walking abilities are unchanged.
- Leaving the Domain restores the normal seasonal water policy elsewhere.
- Turning the checkbox off restores the existing age/season/console policy.
  The old `gEnhancements.ZorasDomainIce` values 0/1/2 still mean native/always
  frozen/never frozen, with the pre-existing seasonal precedence.

No age, scene setup, King Zora/red-ice actor, quest reward, story flag, saved
progression, or randomizer solver rule is replaced. This is presentation plus
the water collision needed to match that presentation. Removing the ice permits
swimming; this does not add those routes to seed-generation logic.

## Assets and configuration

No new asset pack is required or produced. Native water display lists remain
`object_spot07_object_DL_000460` and `object_spot07_object_DL_000BE0`, with their
original texture identifiers, scrolling segments, UVs, and translucent rendering.
Existing water replacements resolve through the same resource paths. Installed
Alt/Djipi replacement appearance still needs a configured game run.

Child/adult scene and room headers, actor lists, waterbox definitions, base
geometry, and texture bytes are untouched. No archive load-order changes are
introduced. This belongs to the OoT host in Combo/solo-SoH; MM source is unchanged.

## Evidence

`python3 -B scripts/diagnostics/run_zoras_domain_thaw_tests.py --sanitize`
executes the complete production Domain actor, native season getters, native
caustic setup, seasonal water suppression/restoration, and winter water-walking
decision on real engine types. Graphics service calls, dynamic collision
registration, and resource-patching service boundaries are fixtures.

Both standalone and Combo variants pass:

- 144 age/season/legacy-override/checkbox/actor-parameter combinations per variant;
- both native flowing-water display-list identities and matching ice collision;
- repeated thaw/refreeze updates and balanced collision destruction;
- unchanged ordinary `SaveContext` and separate NEI save bytes;
- seasonal draining/freezing, leave/return restoration, and caustic phases.

The new case failed on the original source's ice draw before implementation.
Address/undefined-behavior sanitizer runs pass. The OoT saved-season gate and
native seasonal weather suite pass. Full native object compilation passes for
the Domain actor, scene table, seasonal scene controller, real `z_player.c`
unity owner, and enhancements menu, each in Combo and standalone mode.
clang-format 14.0.6 and `git diff --check` pass. The focused runner is added to
the canonical regression script; the complete cumulative gate is not rerun here.

Initial verification setup issues are retained in the handoff logs: the solo
gate needed the existing JSON include directory; `custom_items.c` must compile
through its actual `z_player.c` unity owner. Neither required a production fix.
Inherited native decomp/CVar macro warnings remain in the object-build logs.

## Next decisive runtime check

Build the cumulative candidate with its matching generated port assets. Enter
both Domain rooms as adult with the checkbox off, then on. Review the waterfall,
pool, scrolling caustics, swimming, shore collision, and adult NPC/King Zora state.
Repeat with the installed water-replacement pack and Alt Assets on, then re-enter,
save/reload, and compare child Link. With the Rod, also check winter/summer in the
Domain and after leaving it. Runtime appearance, real collision behavior, and
configured acceptance remain pending.
