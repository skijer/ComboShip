# Elemental swords and optional MM ice-arrow behavior

Candidate branch: `poc/elemental-swords-20261019`.
Development parent: `520d7b6dc64ab5742226b9b2f998a34bcd6b4a9e`, the preserved
weekend source candidate. Published prerequisite: PR44 source
`43131342882972eeb4a257599ec8b061c7dc6d43`.

This is a separate source candidate. Installation, a full application link,
configured gameplay proof, user acceptance and master promotion are separate.
The source recovery bundle includes the weekend changes and this work without
overwriting the accepted scene/model packs or the spring-grass candidate.

## Sword controls and progression

The Fire Damage option retains its existing setting identity. Ice Damage and
Light Damage are independent switches. All three appear without requiring the
Din visual option. Each element requires its corresponding actual elemental
arrow in the current game's inventory; medallions do not unlock this pass.

With an enabled and unlocked element, highlight an owned, usable regular sword
on the equipment page and press A. Choose None, Fire, Ice or Light. Locked
entries remain gray. A confirms with the native elemental equip sound,
sparkle/fusion texture and timing into the highlighted sword. The confirmation
equips or retains that sword. B cancels without changing equipment or the
saved element, including while fusion is pending. Page departure and pause
closure discard an unfinished selection. Ordinary equipment behavior remains
available when the chooser is unavailable or the highlighted cell is not an
eligible sword.

Each sword retains one element. Kokiri/Razor/Gilded, Master/True Master and
Biggoron/Great Fairy upgrades inherit their predecessor's choice until the
player explicitly chooses a new element for that upgrade. None is an explicit
choice and suppresses the old Din fire fallback. Existing saves with no new
fields retain safe zero defaults. Each host serializes its own per-file choices
through its existing NEI save path; this does not add a FleetSync protocol.

A native Fire/Ice/Light arrow badge overlays the B sword icon. It follows HUD
alpha, size and placement, including the MM HUD editor. Temporary bow/fishing
and action labels, transformed/custom weapon owners and hidden HUD modes do
not acquire a sword badge. A live legacy Din-fire effect uses the actual combat
gates for its badge, including arrow/resource availability.

On the first actual receipt of an elemental arrow while its corresponding
option is enabled, an added native dialogue page follows the original item
receipt. It explains the equipment-screen A selector and zero-magic sword
strikes. Fire, Ice and Light use their native red, light-blue and yellow/gold
text controls and the native A-button glyph. Already-owned arrows, save
loading, editor/sync grants, non-arrow receipts and disabled options do not
introduce retroactive tutorials. Native/custom receipt ordering and grants
remain intact. Tutorial copy is English; the OoT Japanese byte-decoder path is
left unchanged. MM uses three lines per added page, verified through its
production header and NES message decoder, including overhead receipt fades.

Swings and elemental damage consume **no magic**. The normal arrow item keeps
its existing behavior and cost. No new meter, ammunition or per-hit cost is
introduced for swords.

## Damage preservation contract

The original player sword flags and native sword damage remain authoritative.
One element flag is added only to an actual local regular-sword melee hit.
The original numeric sword row and native normal/jump/spin multipliers are
retained for sword-admitted receivers. Actor-local elemental effect codes are
used only on inspected compatible native response paths. Immune/zero rows do
not erase an otherwise damaging sword row. Existing accepted fire-only target
exceptions remain bounded to their original paths.

The added status responses are deliberately bounded:

| Host | Ice | Light |
| --- | --- | --- |
| OoT | Crow/Keese frost fall and the first Lizalfos ice stun | Wallmaster/Floormaster light stun |
| MM | Wolfos, Wallmaster, Floormaster, Guay, Deku Scrub, Peahat, Keese and Dinolfos native ice responses | The same targets plus ReDead and Freezard native light responses |

Other targets retain their native sword reaction. This includes Baba cutting,
Armos and blue Chuchu; an added effect must not introduce a zero-damage,
instant-kill or lost-cutting regression.

Grass cutting, Deku Baba cutting/drop routes, ordinary knockback and native
enemy-soul collision gates remain intact. Repeated hits while frozen or
stunned are checked against native sword behavior. In MM, frozen admission
guards use the original flags only for exact owned enchanted sword colliders;
genuine elemental arrows and unrelated/foreign colliders retain their native
flags. Already-stunned OoT Lizalfos use the sword response so a second hit
does not lose its ordinary damage.

Turning an option off, changing element, choosing None, scene/age changes and
stale contexts restore only this feature's own collider writes. No element
stacking or contamination of the vanilla crouch-stab prior-strike value is
permitted. Charged spin-wave/projectile paths and custom weapon owners retain
their existing rules. The earlier Cane, Trident and Four Sword exclusions
remain in place.

Selected Fire, Ice and Light enchantments have no dependency on Din assets or
Alternate Assets. The older automatic Din appearance retains its original
assets/settings when no explicit selection owns the sword.

## Drawn-blade auras

Each selected element also appears on the drawn sword. Fire reuses the existing
Din core/flame material, scrolling pattern and verified artwork. Its small
64 by 32 I8 fallback textures are compiled in; the original protected artwork
and accepted archive remain unchanged. Ice has a fitted cyan frost envelope.
Light has a white-gold shimmer envelope. The Ice/Light layers are translucent
so the underlying sword remains visible.

The generated aura meshes cover the shipped ordinary/NEI swords, native
fallback swords and recognized Din equipment, including progressive upgrades
and the broken knife. Native blade records are retained with their archive
and resource hashes for repeatable offline generation; auras do not need the
original archives at runtime. They add no replacement sword or new world
particle actor. An explicit menu selection suppresses the older Din hand
substitution, preserving the sword selected by the normal renderer.

Known renderer branches register the actual resolved blade and display-list
pointer. The registration follows native copies and is used only when that
pointer still wins at the final hand draw. This covers standalone MM's native
fallbacks and equipment selecting a broken blade after an upgrade. The accepted
Din aliases are identified by their actual resource command hashes, rather
than by the mere presence of a pack. Arbitrary external sword/body replacement
meshes are not fitted dynamically and require visual inspection.

Aura submission uses the current local player's actual drawn-hand matrix,
with frame, scene, rig and selected-element checks before rendering. Stowing,
None, a disabled/locked element, hidden hands, transformed/custom weapon
owners and transition contexts do not retain a captured glow. Animation
advances once per gameplay frame and freezes while paused. These rendering
changes do not alter sword collision, reach, damage or magic.

## Optional OoT MM ice arrows

MM Ice Arrow Behavior is an independent OoT checkbox, off by default. The
native MM host is unchanged. It operates in Combo's OoT engine and solo SoH.

Water platforms follow MM's 20-frame growth, 300-frame float, 50-frame melt,
three newest tracked floes, approximately three-unit bob and cutscene pause.
A fourth floe starts the oldest melting. Disappearing water, destruction and
scene teardown release the actor's dynamic collision and resource ownership.
The native first-water-entry projectile ordering is retained, including its
limitation when one long arrow step reaches submerged geometry before the
next water check. This does not promise a platform on every possible surface.

For solo-safe resources, the floe uses unchanged OoT Zora's Fountain iceberg
model/collision data, normalized to a roughly 72 by 83 unit footprint and
aligned to water plus ten units. It reproduces the behavior with native OoT
ice appearance and requires no extracted MM asset pack.

An actual native Ice Arrow can freeze an OoT Octorok into a temporary climbable
ice block instead of the normal freeze/death sequence. Sword ice flags and
other projectiles do not create this block. The native OoT cube is normalized
to 60 units per side. Parent/child destruction, failed spawns, fire thaw and the
450-frame hold preserve their native fallback/cleanup paths. No actor or save
structure layout changes are required.

OoT's existing blade-trap freezing and compatible enemy ice-arrow responses
remain intact. MM-only Great Bay waterfall, blue Chuchu and other absent scene
or actor state machines are not invented as generic OoT texture interactions.

## Evidence and runtime boundary

Focused tests execute production selection/combat code, actual actor damage
tables and reactions, save JSON parsers, native menu continuation, GBI badge
and fusion commands, HUD-editor transforms, arrow dispatch and ice actor/resource
lifecycles. Aura fixtures also execute the production native hand-selection and
compositor branches with substituted resource services, then inspect the real
submitted GBI and retained geometry. These are bounded branch fixtures, not a
whole-engine gameplay session. Standalone and Combo compilation are recorded
separately from the installed game's behavior. Address/undefined-behavior sanitizer checks use
`ASAN_OPTIONS=detect_leaks=0` in this container.

The focused gate is `bash scripts/diagnostics/run_elemental_sword_regressions.sh`.
Native receipt objects can also be checked with
`python3 -B tests/item_receipts/run_sword_element_receipt_tests.py --sanitize --native`.
The formatted source checkpoint `00b843da591c53e0433778141a29ae44900f4e38`
passed all 13 focused-gate commands with unchanged input hashes. The eight
complete native aura translation units passed in standalone and Combo modes
(16 objects); all 1,622 recorded inputs still match the final candidate.
All 73 changed source files selected by the repository formatting rules pass
clang-format 14.0.6. Exact evidence identities and the inherited gate failures
are indexed in `elemental-swords-verification-20261019.json`.

The recovery kit's `verification/` directory records command results, native
object/dependency identities and inherited whole-project gate blockers. The
Black Market fixture failures reproduce on the unchanged development baseline;
the broad development continuation is not presented as a clean final-head pass.

The delivery checkpoint includes exact commands, results and restoration
verification. A source/object test is not proof of final in-game visuals,
audio, standing/swimming collision or scene return. Required runtime checks:

- Receive each arrow with its option enabled; inspect the added native tutorial
  and elemental word colors without losing the original receipt/fanfare.
- Enchant and cancel on each sword family; inspect the native fusion, sound,
  B badge and HUD placement in both hosts, then save/reload and change games.
- Inspect the Fire/Ice/Light envelopes on ordinary and progressive swords in
  both hosts/ages, including draw/stow, pause, hidden hands, scene return and
  installed equipment/model packs. Check for clipping, misplaced glow and
  graphics-state effects on later actors.
- Compare ordinary sword health/cutting against None, Fire, Ice and Light,
  including grass, Deku Babas, frozen enemies and repeated stunned hits.
- Compare magic before/after swings. Test options off and vanilla/Alt assets.
- Shoot deep water, stand/swim near a growing/melting floe, exceed three floes,
  freeze/thaw an Octorok, then pause, leave and return to the scene.

Runtime acceptance and promotion remain pending until those checks are tied
to the installed candidate and configuration.
