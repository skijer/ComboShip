# Black Market and forest follow-up

Candidate branch: `poc/market-forest-followup-20261022`, based on
`develop` `c3b0c76b81766ce51378e573dcf9816ab3c59473`. This is an implemented,
native/static-verified candidate. It does not promote an accepted visual master.
The immutable native forest ZIP and retained market geometry bundle are unchanged.

## Black Market browsing

Both engines' `Actor_GetScreenPos` project `focus.pos`; the display-only item
actor previously initialized `world.pos` only. Every cursor therefore projected
the origin. The shared adapter now supplies the selected offer's focus position.
The supplied clip does show a shallow existing shelf pan; it does not prove
all camera writes were absent. The candidate uses a closer keeper view,
interpolated eye/target/FOV and adapted native 30-degree shelf views. Item
selection and cursor display wait for a completed pan; cancel remains available.

Native OoT EnOssan owns camera/navigation, while EnTana is the render anchor.
The existing shared shelf fixture, physical slots, collision, eight seed-saved
offers, purchase/progression code and optional keeper remain intact.

The MM EnOssan reference also separates keeper, look-to-shelf and shelf-browsing
states: it starts the scene camera before updating the selected item's focus,
cursor and description. The shared adapter follows those bank/row/column and
return-to-keeper semantics with host subcamera targets because this private
room has no native shop cutscene setup. Successful changes use each host's
native cursor sound. Purchased one-time merchandise disappears, while its
slot stays selectable for the sold-out text; renewable refills remain visible.

The saved stock still has eight offers, six NEI plus two wildcards, with the
existing two/four/two tier distribution and zero/one/two-clear unlocks.
Saved prices and fixed offer identities remain authoritative. The glyph bands
still use the fitted native captured item bounds and original IA8 strip; new
arena preflight checks protect actual OPA-tail and XLU allocations, including
debug display-list headroom. Native allocator shortage cases now test the
real allocator behavior instead of assuming it returns null. See
`black-market-graphics-arena-20261022.md`.

Merchandise descriptions and purchase choices now use each host's native
message format and fixed item's metadata name, with catalogue fallback.
MM uses the native header/price and "I'll buy it"/"No thanks" choices; OoT
uses its native formatter, locale and choices. Tier, seal, owned, sold-out,
prerequisite and currency status come from the existing market policy. Native
button glyphs are encoded for the owning font. This does not convert the
fixed offer through a randomized check or change what a purchase delivers.
Long titles keep native choice rows aligned; a market-local OoT page pass
handles the fifth-line edge without changing the global message formatter.
See `black-market-native-merchandise-text-20261022.md` for the actual
decoder/font, control-operand and layout checks.

The actual native message start/continue/close wrappers and terminal controls
pass three repeated conversations without clearing the message context between
visits. Both hosts reach native CLOSING and then NONE, reset stale choices, and
accept a new conversation. The companion browsing fixture verifies camera and
player release for leave, interruption, transition, death, absent play state
and post-cleanup destruction. These are native sanitizer proofs, not observed
gameplay. Mirrored cursor positions match all eight native placements; OoT's
horizontal mirror input follows its own EnOssan behavior, while MM keeps its
native input behavior.

## Original default interior

The old native fallback remapped all 28 room materials to one stock bark
texture. It also omitted `custom/skiejer_market/texture/arcane_stone`, despite
the shared shelves loading that path unconditionally. A failed load could reuse
the prior tile. The first timber/slate/plaster pass left decorative meshes
without appropriate materials. The final ordinary port archive preparation
maps all 28 retained material IDs using the actual room draw graph, including
the inactive retained material. It supplies original glazed ceramic, bronze,
wicker, celestial enamel, herbs, books/paper/bone/glass, potion liquid and cloth,
alongside the architectural surfaces and valid slate shelf image. Small atlases
and bounded prop UV corrections separate roles that shared the old material
slots. No Djipi texture is bundled or required for the default interior. The
optional Granny appearance remains an external override. Exact role mappings
and preservation checks are in `black-market-prop-materials-20261022.md`.

The immutable 125-resource source bundle is unchanged. The prepared graph has
141 resources: its retained materials/targeted vertices, 15 original surface
resources and one original title sprite, identical for both hosts. Room headers,
collision, spawn resources and topology are preserved. The two scene headers
gain the explicitly requested native exit-door record described below. The two
opaque torch stands move with their flames:
right delta (-55,0,213), left delta (29,0,213), yielding anchors (75,76,205) and
(-110,76,205). This brings them out from behind the shelf backs. The smaller
native decorative flames have radius 16 and height 64. The complete stands and
flame envelopes pass shelf/item bounds and retained-room triangle clearance,
including the nearby canisters, cabinets and cauldrons. Close keeper framing
can crop side decorations; the room and shelf views remain the visual review
targets. Prop UV corrections affect only the explicitly audited bone, paper,
leather and glass islands; other mesh coordinates and UVs remain intact.
The exact prepared-resource delta is 151 translated stand vertices, 861 UV-only
vertices and 28 materials. Independent clearance checks cover all 7,919 room
triangles, excluding each fixture's own stand.

The default room previously never submitted steam or flames; those draws only
existed in the optional Granny path. A separate default-only XLU pass now draws
continuous vapor from the four audited pot mouths and two pale violet flames.
It uses each host's native gameplay-keep effect resources, keeps the room and
shelves opaque, restores render state, and introduces no saved effects or light
actors. Granny's effect path remains separate.

Generated PNG originals, prompts and hashes are in
`tools/black_market/assets/interior/provenance.json` and `props-provenance.json`.
Verified compressed 512×512 RGBA bundles keep normal builds in the Python
standard library; Pillow is used only by the optional authoring rebake.
Architectural UV periods are 32×32; props and shelves use the retained 64×64
framing. Old private-path scene, material, room and vertex overrides can replace
these defaults, the title and door records, or torch positions; exclude those
overrides when testing the default candidate.

## Black Market scene title and exit door

The default geometry-only pack deliberately omitted the market title sprite,
so both presenters had no image to show. Normal preparation now includes an
original 192×24 IA8 wordmark, with white letter faces and a one-native-pixel
black keyline. The native intensity and alpha channels carry that edge through
the engine's normal fade. The immutable generated original, prompts, packed
sprite, PNG, checksums and optional authoring recipe are retained under
`tools/black_market/assets/title/` and `encode_title.py`; normal builds need
neither an image library nor an installed font.

Both hosts use OoT's native scene-card position (160,120), 20-frame delay and
80-frame duration. The actual native update functions match frame for frame
for 120 frames, including intensity/alpha fade-in and fade-out. Native Draw
fixtures execute the IA8 load and rectangle commands. Entry fades, pause,
game over, cutscenes, player entry and an occupied title context defer display;
the hide-card hook remains authoritative. Presentation is independent of saved
stock. OoT uses a synchronous transient hook-eligibility scope, preserving
every save byte and any existing cross-game title request. See
`black-market-default-title-20261022.md`.

The existing walk-through exit is retained behind a native wooden handle door
at (-10,0,420). Each host spawns it from a real transition-actor record before
player initialization. Its native Open action uses the existing return route.
A valid live door suppresses the old early z400 trigger; a supported far-side
escape at z440 and the original missing-door fallback prevent a trap. No new
door texture, collision bytes, saved state or progression rule is introduced.
Native actor/form/age/Open/destruction/reload and resource/floor fixtures pass;
see `black-market-native-exit-door-20261022.md`.

## Forest and title cards

The ramp now restores the native two-cycle/fog/tile state and binds the existing
scrolling segment's correct second layer. Portal companions move to the marked
nearer flanks and face outward; Mirror recall faces the market/portal. See
`native-forest-materials-followup-20261022.md` for the actual stock-resource
analysis, coordinates and preservation evidence.

Explicit fountain returns request the OoT destination card. MM boot requests
the final composed cross-game destination's native scene card, including
Woodfall, Woods of Mystery and entrance-shuffled destinations. Explicit
same-game private-area returns use the same one-use request. Hide-card settings
and cutscene ownership remain authoritative. The policy uses the final loaded
scene rather than a route allowlist: all 113 native MM scene-table rows pass
portal/shop/forest arrival checks, including 74 titled and 39 untitled scenes.
The Black Market's private card remains separate from those native MM cards. See
`native-custom-entrance-title-cards-20261022.md` for the transition fixtures.

## Model scales and supplied evidence

The previous `ComboShip-NPC-Complete-Review-Set-20261021.zip` (Library identity
`libfile_9a9ef7aec9608191b0f898bfb593ebdf`) already contains DrawScale 1.10 for
Anime Saria and Young Malon. Their unchanged model hashes are respectively
`2a9ce42b36ec2b1e9ef2d6f06def7a263a2716a76f1800a86c7df5f125890d06`
and `0619f9cd4b8752dfd438b67e62405c3667fc46fcc0be628cc247c382b01e6beb`.
The user confirmed these still look too small. The saved
`ComboShip-NPC-Complete-Review-Set-Scale125-20261022.zip` (Library identity
`libfile_18557f816a408191817e140610a31e4a`, SHA-256
`89b5c60c88cd3286d9bdb003c5ea1e0f35e96ef886a81ec544883fd5604da702`)
therefore changes only these two DrawScale resources from absolute
1.10 to 1.25 (about 13.6% larger than the previous packs), preserving model
proportions and every other repaired resource. Traditional Saria and the five
unaffected model archives remain unchanged. Native flex/provider tests cover
single application, including props, live Alt fallback and MM placed Saria;
the flex traversal also exercises the new 1.25 factor. No geometry or collider
resize is introduced. Visual size and ground contact still require game review.
The other 2,000 resources and five unaffected archives are byte-identical, CRCs
pass, and the included recovery recipe reproduces the complete ZIP exactly.
Use these replacements instead of stacking them with the previous versions.

The user reports successful Black Market access, Saria dialogue/static/song-giver
paths, MM Lost Woods→OoT and Woodfall→OoT cards. The clip directly shows eight
offer slots and several offer labels. No current-session runtime log was
attached, so a full saved-stock audit is not claimed.

## Verification and remaining runtime pass

Passed: both browsing adapters with real native camera setters, projection and
decoded cursor GBI under ASan/UBSan; eight item/seal framing envelopes; stock,
prices, shelves, keeper, locks and both transaction adapters; native market
source/default preparation and readiness checks; native NPC routing, pose,
draw-scale, MM providers and forest actor lifecycle; actual ramp XML/binary
factories; both Mirror adapters; title-card/fountain/entrance fixtures and
affected native translation units in Combo and solo modes. The forest suite
passes 57 checks across its complete run and the real-donor generator rerun.
The original pinned MM and FINAL inputs were recovered; the 19 generator
checks, including the two earlier skipped donor cases, now pass without skips.
Independent source review found no
critical or important issue. Existing compiler warnings are not new failures.

The existing early regression script now exercises the self-contained default
interior, required shelf resource, native door resources/actors/Open paths,
title presentation/draw/asset audit and generic MM destination policy. All
commands passed across the full prefix, corrected resource-driver reruns and
remaining tail. The complete entrance script also exits zero. Older derived
entry drivers now accept the case-enabled fixture entry point and forward the
resource-input argument correctly; boot/renderer fixtures retain the native
transient state and CMake CVar definitions.

Full real-resource entry/return/re-entry checks pass against the exact pinned
MM, FINAL geometry and POC14 inputs, including native and optional override
scenarios, both stock states and three cycles per route. These execute actual
controller/resource fixtures, not application archive mounting or a game loop.
The four final affected translation units compile in both Combo and standalone
modes; their compiled dependency hashes still match the source. Workflow files and the existing task
manifest are preserved. Asset-collision checks pass without changing baseline.

The final offline preview uses actual prepared geometry, authored keeper,
illustrative production-generated offers, glyphs and effect poses. Its lighting
and stock are illustrative and it is not gameplay proof. The saved preview
includes the original black-edged title at full brightness; its fade is verified
separately by the native fixtures. Full executable linking, actual GPU/Alt
appearance, camera occlusion and configured transition behavior remain untested
locally. Test browsing/purchase/cancel, turn around the tree/tunnel/stump, walk
the scrolling ramp, inspect companions/Mirror facing, and cross both directions
of the requested routes using the intended pack order. Preserve the accepted
baseline until this candidate is accepted in that configuration.
