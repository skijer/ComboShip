# NEI GI upgrade candidate

The isolated [Ball & Chain metallic POC1](PREVIEWS/ballchain-metallic-poc1.md)
gives the accepted GI and held ball brushed silver reflections through the
existing forged sword exporter. Meshes and fitting remain unchanged. Rebuild
both with `python3 tools/nei_gi/SOURCE/build_ballchain_metal.py --install`;
the record covers the material comparison and runtime review still needed.

The current asset set contains **62 serialized GI models**: 61 bundled under
`soh/assets/custom/objects/nei_gi_redesign/`, plus optional Cojiro resources under
`tools/nei_gi/OPTIONAL_ASSETS/`. The latest pass adds
element colors for rods, spells and Slate variants, rings that turn with the
Slate, and sword-specific particle themes. It also includes distinct OoT/MM
Kokiri swords and the reviewed sword proportion corrections.

The approved Master, Gilded and True Master visual baseline is recorded in
[`SOURCE/sword_visual_approval.json`](SOURCE/sword_visual_approval.json), with
current previews in the [two-sword adjustment](PREVIEWS/sword-tuned-pass.md).
See [Slate model/color review](PREVIEWS/slate-element-preview.md) and
[attached Slate rings](PREVIEWS/slate-rings-attached.md) for the latest Slate work.
These are offline previews; visual approval does not establish in-game appearance
or a successful full game build.

The October 4 candidate adds an optional rounded blue Cojiro and a bundled
volumetric Mario Mask,
and closes the inward-wound side faces on all six Slate models. Existing Slate
positions, materials, front/back faces and attached rings are preserved. The
remaining accepted GI resources and all 39 held components are byte-identical
to PR34 `122dd5f`. New previews are in
[`PREVIEWS/20261004/`](PREVIEWS/20261004/); in-game appearance is untested.

Cojiro is delivered separately as an asset-only `.o2r`; it is absent from the
built-in game asset trees. The matching PR #34 renderer selects it when loaded
and retains the normal Cojiro fallback when absent. Existing legacy-mod priority
still applies. The archive contains three exact resources with identical base
and Alt copies. The 23 HD icons are a separate optional pack documented in
[`../nei_icons/README.md`](../nei_icons/README.md).

The [integration verification record](PUBLICATION.md) documents the final
19-suite regression run and the resolved historical MM fixture failure.

## Reproduce the current assets and previews

`CHECKPOINTS/` retains the exact exported GLBs, metadata and front/back views.
`SOURCE/` contains the authoring recipes, and `REFERENCES/` preserves earlier
reference inputs. Use Python 3.11 or newer with NumPy/Pillow; the recorded Python
dependencies are in the forged toolkit. From the repository root:

```sh
python3 -m pip install -r tools/nei_gi/SOURCE/forged_swords/requirements.txt
python3 tools/nei_gi/check_forged_proportions.py
python3 tools/nei_gi/check_slate_solidity.py
python3 tools/nei_gi/verify_assets.py
python3 scripts/diagnostics/run_nei_gi_tests.py --combo
python3 tools/nei_gi/package_archive.py /tmp/NEI_GI_Current.o2r
python3 tools/nei_gi/package_cojiro.py /tmp/ComboShip_Cojiro_GI_Optional.o2r
```

The focused diagnostics compile against the checkout's actual libultraship and
SoH headers and require a C++20 compiler and nlohmann-json headers. Packaging
uses the installed resources and creates identical base/Alt entries. The
general GI pack contains the 61 bundled models; Cojiro remains a separate pack.

For selected sword rebuilds, use the
[forged toolkit instructions](SOURCE/forged_swords/README.md). The legacy
`SOURCE/sword_revamp.py` entry point also dispatches imported sword slugs to that
toolkit. To rebuild selected Slate variants and install their resources:

```sh
python3 tools/nei_gi/SOURCE/quest_revamp.py sheikah_slate slate_bomb slate_master_cycle slate_stasis slate_cryonis slate_sensor --install
python3 tools/nei_gi/SOURCE/requested_revamp.py cojiro mario_mask --install
python3 tools/nei_gi/runtime_preview/render_requested.py tools/nei_gi/PREVIEWS/20261004 --frames 36
```

`requested_revamp.py --install` installs Mario Mask but always writes Cojiro to
`OPTIONAL_ASSETS/`, never to a game asset tree.

Generated intermediate `RESOURCES/` and Python caches are ignored; installed
custom assets and checkpoint files are the published outputs. Current sword and
Slate rotating previews use production C++ effects with Mesa EGL/OpenGL and a
common sword camera scale. Portable commands and system dependencies are in
[`runtime_preview/README.md`](runtime_preview/README.md).

## Historical original batch

The earlier presentation candidate extended the original batch to **21 GI models**:
the lantern, Spinner, Cane of Somaria, Minish Cap and Roc's Cape are included.
Both native SoH and NEI feather callbacks use the approved feather. That batch's
scope, references and evidence are recorded in
[`../nei_final_preview/VERIFICATION.md`](../nei_final_preview/VERIFICATION.md).
The original batch history below remains the record of its earlier review.

This batch replaces the get-item presentation for 16 NEI items: Fire, Ice and Light Rods; Roc's Feather; Time Gate; Whip A; Shovel; Gust Jar; Hylia's Grace; Zonai Permafrost; Demise Destruction; Ball and Chain; Deku Leaf; Mogma Mitts; Switch Hook; and Beetle.

The same bindings cover pickups, randomized shops and shuffled freestanding items that call `GetItemEntry_Draw`. Actor and held-item resource paths are unchanged. Source resources are bundled under `soh/assets/custom/objects/nei_gi_redesign/`. An older build without these bindings cannot use the standalone archive.

The **NEI item effects** checkbox in NEI Custom Items defaults off. Enabling it adds bounded deterministic presentation effects: native-color rod/spell motes, green leaf flecks and neutral shimmer on the remaining supported custom items. It does not change item pools, save data or gameplay effects. Missing replacement passes preserve the original item draw.

Demise's core is black (`#000000`) with a pale neutral translucent shell. Time Gate and Gust Jar are upright; the shovel remains tilted. Whip A is a new reconstruction based on the user-supplied screenshot of skeijer's unreleased model, not his original mesh. The Time Gate reference image was supplied with the existing item assets.

The user approved the offline designs on September 27, 2026, after reviewing all 16 items and revised front/back comparisons. The final Beetle uses a mechanical teal/brass casing and crescent pincers. Roc's Feather retains the approved open tuft clefts with clearer barb detail. Ball and Chain uses forged iron plates, pointed spikes, and heavy individual links. The supplied Beetle and Twilight Princess Ball and Chain images are preserved as visual references.

![Approved model preview](preview.png)

These are renders of the exported model geometry, without game particle effects. The spell cores are the actual assets; the pale crystal edges are modeled strips, not preview wireframes. Runtime effects surround the models without replacing their surfaces.

## Historical batch reproduction

`CHECKPOINTS/` retains each exact GLB and its metadata/front/back views. `SOURCE/` contains the Python mesh authoring and software preview renderer (numpy and Pillow required); `REFERENCES/` contains the preserved inputs.

```sh
python3 tools/nei_gi/verify_assets.py
python3 scripts/diagnostics/run_nei_gi_tests.py
python3 tools/nei_gi/package_archive.py /tmp/NEI_GI_Upgrade_Approved_POC3.o2r
```

The renderer tests compile against the checkout's actual libultraship and SoH headers; a C++20 compiler and nlohmann-json headers are required. The archive contains only these GI-specific resources with identical base/Alt copies. It is one combined pack.

To regenerate the original batch's authoring checkpoints, run `python3 tools/nei_gi/SOURCE/build_batch.py`; copy the resulting `tools/nei_gi/RESOURCES/objects/nei_gi_redesign/` into the corresponding custom-assets directory before validating and packaging. This entry point rebuilds the historical batch, not all 60 current models. Regeneration does not silently replace shipped assets.

## Acceptance still required

Compiled tests and offline previews do not prove game appearance. Check one shop and shuffled freestanding pickup with effects off/on, inspect rod-tip placement, rotate the three transparent spells, and repeat with Alt Assets toggled. Check the current swords and attached Slate rings in native and foreign host draws, including pickup scale, scene re-entry and pause/unpause. In-game verification of these configurations remains required.
