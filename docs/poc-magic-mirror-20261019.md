# Magic Mirror candidate — 2026-10-19

The Magic Mirror is a permanent, fixed Lost Woods reward outside the shuffled
item pool. It returns the player to the Mirror stump from safe outdoor
areas. The elevated stump displays the spinning mirror; approaching its base
starts the native item presentation, so obtaining it does not require a seed's
equipment or reaching the inaccessible top.

## Source and inputs

- Candidate branch: `poc/magic-mirror-20261019`.
- Parent: `31ea4e711f13f1e5b2b1b9ac9b36ea43bc13b18b`.
- Parent tree: `bbc04bc7603296c9e8a5b69d5c0a2f81127ba45e`.
- This is an isolated candidate on the published cumulative integration. No
  accepted baseline or integration branch is promoted by this checkpoint.
- Placement was checked against the unchanged input
  `1 - lost woods 2ship granny shop and stump moved.prelude.o2r`.
  SHA-256: `7fde769411d21792f6bc6cde62ee934fb17b8e24d7a01b8d74f30069cce49a83`.
- That input's unique stump cap has center `(-377, 80, 416)`, dimensions
  `110 × 112`, and 12 triangles. The runtime locates the compact horizontal cap
  in the loaded collision; it does not edit the scene or collision archive.

## Behavior and integration

- Full item ID `0x0224`; independent saved ownership. Time Gate and Mirror share
  page-two cell 31 through the existing item selector. Old Time Gate saves heal
  into independent ownership, and receiving either sibling keeps the selection.
- Both games save/load and share the ownership flags. Full-width C/D-pad items
  survive cross-game transfers, including an owned tool behind its sibling.
- Combined OoT recall uses the existing clean-frame save/fade/handoff into MM's
  opening forest. MM recall uses its native forest entrance. Solo OoT/MM use the
  existing resource-checked copied forest and their native transitions.
- Recall prompts Yes/No and cancels with B. It requires a living, grounded,
  unmounted player in ordinary outdoor play. Dungeons, interiors, cutscenes,
  transitions, timed activities, and blocked item states reject use. There is no
  magic, currency, age, form, or seed-equipment requirement.
- Farore's Wind and grotto respawn structures retain their contents. The Mirror
  clears only its destination cutscene/respawn override and uses normal black
  departure/arrival fades. Native quest progress, age/form, and clock remain
  under their existing systems.
- Pickup callbacks are bound to slot, seed, scene, and mode. Reset/scene teardown
  clears pending pickup events and releases only the Mirror's private prompt.
- The same generated blue/gold round mesh supplies the world reward and get-item
  presentation. The renderer reserves native arena capacity before writing.
  Inventory/name PNGs are identical in both games and use the existing custom
  asset collection. Rebuild recipes and native renderer checks are in
  `tools/magic_mirror/README.md`.

The approved ALTTP visual revision is isolated on
`poc/magic-mirror-alttp-20261019`, based on published checkpoint
`65c34dba7c73b43909946963486dc06d075ddb08`. It replaces the first gold-oval draft
with the reference's round blue casing, gold inner rim, short grip and pale
mirror glass.

The temporary casting motion is isolated on `poc/magic-mirror-cast-20261019`,
based on the ALTTP visual tree `be2d96f9449805696958c56ef3a044560ff3a658`.
Confirming Yes now commits to the native Farore's Wind main flourish, played
once at 1.25 speed after normal held-item put-away. As the hands rise, the blue
Mirror appears above the current head position, facing the camera. Its final
raised-hands pose and the Mirror remain visible through the existing fade.
The native clip ending, rather than a wall-clock delay, triggers recall.
No Farore spell actor/sparkle mesh, magic consumption, warp-point write, or new
scene asset is involved. The TP activation sigil remains design work.

The cast is bound to the same player and slot/seed/scene/mode context. Losing
ownership or outdoor safety, a replacement message/action, or a stalled native
put-away cancels it. Cleanup returns only the Mirror's own action to idle; an
interrupting native action retains ownership. Destroy discards player pointers
without dereferencing them, including MM's post-arena-cleanup destroy hook.
Mario's independent controller explicitly yields for this item action. His
current mesh stays parked while the overhead Mirror and recall finish; this
placeholder does not retarget Link's Farore clip onto the libsm64 rig. The
overhead anchor follows his actor position because Link's hidden head joints
can be stale. Configured Mario presentation remains a runtime acceptance check.

## Verification and limits

Focused runners are registered in `scripts/diagnostics/run_combo_nei_regressions.sh`:

```sh
python3 -B tests/magic_mirror/run_tests.py
python3 -B tests/magic_mirror/run_form_tests.py
python3 -B tests/magic_mirror/run_integration_tests.py
python3 -B tests/magic_mirror_inventory/run_tests.py
python3 -B tests/magic_mirror_inventory/run_action_tests.py
python3 -B tests/magic_mirror_inventory/run_sync_tests.py
python3 -B tests/magic_mirror_inventory/run_save_tests.py
python3 -B tools/magic_mirror/test_visuals.py
MIRROR_TEST_CXXFLAGS="-fsanitize=address,undefined -fno-sanitize-recover=all -no-pie" \
  ASAN_OPTIONS=detect_leaks=0 python3 -B tools/magic_mirror/run_draw_tests.py
```

The tests execute the actual runtime, shared native casting action, each engine's
verbatim once-animation advancement/dispatcher, save/ownership/selector functions, and
native graphics allocator/setup bodies with real engine types. External system
boundaries such as disk saves, launcher calls, and GPU rendering are fixtures.
The recovery archive records exact command results, syntax probes, asset gate,
source trees, and independent patch-restoration evidence. Casting fixtures cover
put-away, completion before travel, repeated-button suppression, overhead draw,
fade pose retention, safety/context interruption, a stalled cast, and destruction
after freeing the player. Animation asset I/O and native idle/put-away helpers
remain fixture boundaries; the actual adapters require native compilation too.

This checkpoint has no configured in-game proof or user acceptance. A complete
executable/archive build and link was not performed here. The real-header syntax
probes are not a substitute for that build; in particular the whole SoH save
translation unit requires the unavailable libzip development header. Its actual
travel save/load bodies are exercised by the focused save fixture instead.

Before promotion, build the candidate with both custom texture sets and the
usual forest resources, then test the actual selected launcher mode and archive
load order. Runtime acceptance must cover the moved stump's visible placement
and ground pickup, get-item scale and sound/text, C/D-pad prompt and cancellation,
save/reload, both cross-game directions, solo forest visits, and blocked-use
states. Include Time Gate ownership/selection and Farore recall in that session.
For the placeholder cast, check the visible morph and hand/head placement on the
selected age/form, the overhead Mirror at the last beat, and the fade/handoff.

## Stump-relative recall revision

Candidate `poc/magic-mirror-stump-recall-20261019` builds on local cast checkpoint
`4f48cf77911ee8519f190e6a9cef20c3a933ebc0`, whose tree is identical to published
`b4e035fbbc0db175574a68b04cf1e91a20bf4ac3`:
`3d44cf535e82a8b2faca85c81bdf7898152e1739`.

Recall now resolves the same loaded collision cap as the pickup. The cap's
12-triangle radial-fan shape identifies it independently of world position or
height; duplicate matching caps fail closed. Prelude translation/rotation carries
the pickup and destination together when visual and collision edits are exported
together. Resizing or duplicating that authored stump requires a locator update.

At the currently retained placement, the reward is above cap `(-377, 80, 416)`;
recall lands at `(-377, 0, 296)` facing the stump, within the ground-pickup radius.
The runtime tries eight cap-relative directions for static floor support, a
40-unit supported footprint, wall clearance and 100-unit headroom. Missing cap,
ambiguous cap or no clear base retains the scene's ordinary entrance spawn.

Native transitions queue a slot/seed/mode-bound one-shot arrival. Combined OoT
uses a distinct Mirror handoff route; MM queues the arrival after destination save
loading and does not queue the tunnel portal camera. Immediately before native
player initialization, recall substitutes a copied spawn entry. Native player
and Mario controller therefore initialize at the destination. Immediately after
actor initialization, the original scene spawn pointer is restored (preserving
legacy doorway anchors) and the arriving player leaves the native doorway walk
before camera setup. Cached scene bytes and Farore/grotto respawn data are not
written by the Mirror arrival.

The unchanged original editable Prelude export is now bundled under
`tools/magic_mirror/scene/`, with an exact-byte identity manifest. The previous
complete POC14 scene is separately retained and identified there, including its
durable file identity and SHA-256. It contains the same visual/collision stump
and newer Black Market work. The final complete candidate now supersedes POC14, as described in
`docs/poc/lost-woods-final-door-20261019.md`. Do not overlay older whole scene
or collision resources onto that candidate. Solo modes retain
their existing private forest addon and resolve its loaded stump in the same way.

Additional checks:

```sh
python3 -B tests/magic_mirror/check_scene.py
python3 -B tests/magic_mirror/check_scene.py --compiled /absolute/path/to/zzzz_MM3D_Lost_Woods_FINAL_avatar_POC19.prelude.o2r
python3 -B tests/lost_woods_entrance/run_tests.py
```

Both actual Mirror adapters compile to native C++ objects. Both full native player
and Play initialization translation units pass their engine's C syntax probe.
Sanitized fixtures exercise moved loaded collision, cached-address reuse, pending
arrival lifetime, context mismatch, physical-query rejection, exact-once use,
cached-spawn restoration and native handoff/fade paths. Scene checks validate all
13 cap vertices against the Prelude edits and the final candidate's rendered cap resource, execute
the production locator across all horizontal scene faces before/after translation,
and inspect static collision support/clearance at the current base landing.
Configured gameplay, archive priority, runtime camera/Mario presentation and a
complete linked executable remain unverified.

## Recovery

Use the published candidate ref or the recovery archive's binary-safe patch.
For the patch route, check out the exact parent above in a clean copy of
`marsh6487/ComboShip`, apply `source.patch` with `git apply --index --binary`, and
compare `git write-tree` with the manifest's candidate tree. Restore the input
archive only to its intended scene-mod position; the supplied copy is unchanged.
Generated previews are software renders of the native mesh, not game captures.
