# Chateau Romani MM3D GI and two-color shimmer — POC1

The candidate adds cor's supplied MM3D Chateau model to both normal port asset
archives. Alternate Assets selects the converted model; disabling it retains the
native MM bottle. A single intrinsic shimmer mesh contains gold `#FFD45A` and
the label's dominant burgundy-purple `#4A0C41` in every frame. It does not tint
the bottle or consume the effect pool or gameplay RNG.

| Field | Record |
| --- | --- |
| Parent | Weekend cumulative candidate `520d7b6dc64ab5742226b9b2f998a34bcd6b4a9e`; PR44 prerequisite `43131342882972eeb4a257599ec8b061c7dc6d43` |
| Candidate branch | `poc/chateau-romani-3ds-20261019` |
| Source | User-supplied ZIP, SHA256 `73c375486b5f5323137b99679e9a13ae6e3b2dea25a12a03e89a89ef63e4df34`; preserved at `tools/chateau_romani_3ds/source.zip` |
| Credit | Nintendo / Grezzo; ripped by Guy for The Models Resource; original readme retained in source ZIP |
| Scope | Chateau GI geometry/materials and its native/foreign shimmer only |
| Preservation | Bottle grants, cow condition, localized text, refill and drinking logic, infinite-magic lifecycle, item icons, other bottles, masks, weather, actors and source integrations remain unchanged |
| Configuration | Combo, OoT-only and MM-only host paths; Alternate Assets on/off; ordinary and shuffled Chateau bottle/refill receipts and shop display |
| Evidence | Resource graph and source-pixel comparison; deterministic rebuild; actual OoT callback fixture; both-host real-GBI shimmer fixture with sanitizers; receipt formatting fixture; native translation-unit compilation |
| Verdict | Implemented and statically/object verified; complete executable linkage, configured gameplay and user acceptance remain pending |
| Recovery | Original ZIP and parent remain unchanged; bounded commit/patch can be omitted to roll back |

The converter retains all 284 source triangles, the complete 64×64 diffuse
atlas and the RGB of both authored reflection maps. It fits the native bottle's
60-unit height and quantizes positions to the engine's integer vertices. There
are 19 resources, 96 opaque triangles and 376 translucent-pass triangles
including reflection overlays. Glass and its reflection draw before the label
so the label stays readable. Generated resources are byte-identical in the two
custom asset trees, with no new differing-content namespace collisions.

Native MM continues using its existing two draw-table roots, with Chateau-only
alternate wrappers. OoT's existing strict MM donor loader ignores replacements,
so its Chateau callback selects the unique bundled resource graph while Alt is
on and both roots exist. Missing custom roots fall back together to the native
donor. The fallback remains hidden if no donor exists. The new graph itself has
no native-game archive dependencies. No pointer is latched across Alt toggles.

The new shimmer identity appends after the existing 0–49 identities. Existing
equipment identities retain their classifications and colors. MM exports the
new identity through the existing draw-info field; foreign draws submit one
two-color mesh. Native C and C++ draws share the same grayscale, matrix and
owner-state restoration boundary. The shimmer also accompanies the native
fallback bottle with Alt off, as requested for Chateau GI generally.

Rebuild the editable resources with Python, NumPy and Pillow:

```sh
python3 scripts/assets/build_chateau_romani_3ds.py --write-custom --output Chateau-Romani-MM3D-POC1.o2r
python3 tools/chateau_romani_3ds/verify.py --archive Chateau-Romani-MM3D-POC1.o2r
python3 scripts/diagnostics/run_chateau_3ds_draw_tests.py
ASAN_OPTIONS=detect_leaks=0 python3 tests/item_receipts/run_chateau_receipt_tests.py --sanitize
ASAN_OPTIONS=detect_leaks=0 python3 scripts/diagnostics/run_mask_shimmer_tests.py --sanitize
```

`python3 tools/chateau_romani_3ds/run_tests.py` runs the focused resource,
palette, routing and native GBI checks together and is included in the canonical
regression driver. The separate localized receipt check above remains available.

Chateau's dialogue/tutorial is explicitly preserved. The receipt catalog and
encoding/render helpers, localized native callback and registration IDs, cow
receipt entry and MM delivery queue are byte-identical to the weekend parent.
An additional production-header check covers Chateau bottle/refill aliases in
English, German and French: the assign/use-button instruction, page breaks and
the hearts/unlimited-magic lesson must all remain. This guard runs in the focused
gate; model and shimmer selection cannot replace or suppress the receipt hook.

The binary resources are checked in, so normal `GenerateSohOtr` and
`Generate2ShipOtr` packaging needs no converter or separate mod download.
Regenerate **both** port archives with the candidate executable. The supplied
standalone O2R is a convenient copy of those same 19 resources, not a substitute
for the OoT source correction. Later elemental-sword, spring-grass and other
independent checkpoints must still be reconciled by the cumulative integrator;
this branch extends the exact weekend parent and does not replace those deltas.

Two broader pre-existing fixture gaps were reproduced on the unchanged parent:
`run_bottle_gi_tests.py` and `run_mm_static_gi_bridge_tests.py` do not declare the
recent `NeiGi_ReportBlackMarketModel` diagnostic in their extracted fixtures.
Both fail to compile identically with this candidate. They are recorded as
baseline failures, not converted into passing evidence. The full canonical
regression gate and platform build have not been claimed by this asset task.

The decisive gameplay review is a full GI spin and shop display in native MM
and OoT, plus an MM-owned foreign receipt in OoT: confirm label orientation,
glass/reflection appearance, simultaneous gold/burgundy motes, Alt ON→OFF→ON,
pause/re-entry and one unchanged purchase/drink. Observe Combo and both solo
launch modes under the actual packs/load order. No configured runtime evidence
or master promotion is implied by the offline preview or object compilation.
