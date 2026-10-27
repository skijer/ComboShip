# Selected sword blade auras

`scripts/assets/build_elemental_sword_aura.py` generates the private, built-in
GBI/vertices and four logical 64×32 I8 maps in
`combo/render/sword_element_aura_data.h`. The generated file disables formatting
inside its data region so clang-format 14 does not invalidate `--check`.

The Fire maps are LANCZOS reductions of the exact I8 red channel in the accepted
Din progressive archive. Its artwork is not regenerated. Fire keeps the existing
`build_din_fire_weapons.sword_meshes` geometry recipe and two-cycle material,
including its opaque core, translucent flame and 3/5 scroll. Ice and Light use
distinct procedural I8 maps and a tighter, translucent cross-section envelope;
the original sword metal remains visible. No new weapon model, magic cost,
particle actor or room effect is introduced.

The 2026-10-24 Ice/Light visibility pass raises their material opacity and
broadens the existing frost veins/light rays, with a higher I8 floor so the
casing survives distance filtering. Light retains its gentle opacity pulse.
All blade geometry, identities, scroll speeds, palette overrides and protected
Fire assets/material remain unchanged. The source preview uses phase-zero
opacity; configured in-game camera-distance acceptance is still required.

## Reproduction and provenance

Normal regeneration needs only this directory and the checked-in authored NEI
sword resources. It does not need an external equipment archive:

```sh
python3 -B scripts/assets/build_elemental_sword_aura.py
python3 -B scripts/assets/build_elemental_sword_aura.py --check
```

The retained `*.i8` maps, `din_layers.json.zlib`, accepted equipment alias
commands, and native clipped triangle records are pinned by SHA-256 in the JSON
source manifests. The manifests also identify every source DL/vertex dependency,
blade clipping plane and authored held matrix. Original input archives remain
read-only:

| Input | SHA-256 |
| --- | --- |
| Accepted Din progressive candidate | `5f7972b03e494aa3c309860dc2e91d85ab3bbe18c61244c06d063f8205e27306` |
| Native OoT archive | `ea80d61b223ee38075fe5383b652998f3b08164e903134922f63f205d995717b` |
| Native MM archive | `f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59` |
| Original adult Din equipment | `ce1b8bfbfe9e1b2cf4a51355ea62c16826f60e8a65f07945d33a628b724cc781` |

Optional `--import-fire`, `--import-oot` and `--import-mm` arguments recreate the
small retained inputs from those exact archives; each import rejects another
archive hash. Native extraction recursively follows actual hashed DL and vertex
commands, clips out the hand/grip, and rejects an unexpected matrix instead of
inventing a transform. Ordinary source regeneration does not read the archives.

## Final blade identity

The shipped NEI swords have independent adult OoT, child OoT and MM human fits,
including their progressive variants and a clipped broken Knife. Native OoT/MM
fallbacks retain their actual shorter blade geometry. The accepted Din pack's
four fits retain its exact protected core/flame vertex records.

The successful held-sword branch registers its model and exact DL pointer.
Native compositors transfer that identity when copying their staging list.
CustomEquipment identifies the accepted Din aliases from their actual hashed
blade target, including a retained broken blade after a Great Fairy upgrade.
The post-limb hook supplies the final DL, so a later equipment/PAK winner cannot
inherit an earlier fit merely because the pack is present. Explicit selections
suppress the old Din hand replacement and preserve the final weapon model.

The captured hand belongs to the local player and current scene, rig, frame and
selection. It is consumed once after the body. Hidden hands, reflections, foreign
players, excluded weapon owners and transformed forms do not receive an aura.
Pause freezes its existing phase. Selected auras depend on the real selector's
element checkbox and corresponding arrow ownership, with no Din/Alt-assets gate.
Unselected legacy Din visuals keep their existing resources and behavior.

Arbitrary third-party sword replacements keep the logical sword's fallback aura
fit; their blade geometry is not inferred. A configured in-game visual pass is
still required for final appearance, blending, animation and user acceptance.

## Original ModernDin Biggoron identity correction

The original adult Din equipment uses different `ModernDinPOC2_LONGSWORD_BLADE`
and `ModernDinPOC2_LONGSWORD_BROKEN` targets from the later progressive pack.
Previously these aliases were unrecognized; a combined native hand could also
be assigned its native profile without inspecting the actually resolved DL.
The original full blade reaches X=6295, while native BGS reaches X=5492. Its
socket and cross-section differ too: at X=4500 the blade's positive Z surface
is 67.26, but the native casing reaches only Z=11.5 and can sit behind the metal.
Thus uniformly stretching a native or logical fit does not establish coverage.

The two appended profiles retain the actual 172 blade triangles and authored
normals from each original blade leaf, without the hand, hilt or stowed model.
Their graph has no embedded matrices. `modern_din_source.json` records every
DL/vertex hash and actual alias command; `modern_din_blades.json.zlib` retains
the extracted records. Optional `--import-modern-din ORIGINAL_ADULT_ARCHIVE`
reproduces them and rejects other archive hashes. All previous generated
profiles, protected fire maps, Din layer records and native source inputs remain
byte-identical to baseline `613802447cf8a4fbc3e058ba084f9e3dd7b9f7ae`.

Both CustomEquipment aliases and native OoT/MM BGS imports now identify these
actual blade targets using only complete commands within the named resource's
reported byte size, after matching its current data pointer to the drawn DL.
The MM imported BGS hand resolves through the resource cache each draw, so an
Alt switch or reload cannot retain an old raw allocation and inspect it using
a new owner's byte count. Sanitizer regressions exercise a two-command unknown
branch-only replacement, freed/reloaded allocations and mismatched size owners.
Unknown replacements retain the documented logical fit.
This correction proves source coverage for the recovered original Din aliases.
It does not establish the cause of the reported partial casing on the built-in
authored Biggoron with Alternate Assets disabled.

## Built-in Biggoron coverage control

The Alt-off held selector routes the MM human blade through
`__OTR__@oot-gi-base:objects/nei_held_swords/biggoron_sword/mm_human/held_dl`.
Its held graph pushes `grip_mtx`, calls the GI graph (which pushes `scale_mtx`),
and balances both pops. The source blade follows the same row-vector
`vertex * scale * grip * hand` order as the resource matrix renderer. The 40
`ordon_steel_tex` triangles contain both long blade faces; the 40 sheen triangles
repeat those surfaces. Other material groups stop at the fittings near X=968.
The MM human/child blade reaches X=5500, as does its generated core.

`biggoron_builtin_render.c` exercises the actual successful held selector with
Alt disabled, its per-frame compound identity transfer, the renderer's captured
hand pose and the actual generated GBI for every element in both hosts. The
source control passes with a rotated/scaled/translated hand pose after the
current matrix has changed. It is a boundary test, not a live skeleton/game pass.

`biggoron_depth_coverage.py` additionally depth-tests the exact exported core and
tight triangles, UVs, vertex alpha and frost I8 against the actual source metal.
Across all three rigs, eight rolls and four length sections, including the tip,
more than 98% of sampled metal pixels have visible combined frost alpha above
0.03. This rules out a lower-third-only source envelope, a missing opposite long
face and a length-specific UV cutoff in this retained source. The orthographic
control does not model perspective, the full RDP, lighting or an installed asset
archive. The screenshot's configured partial casing remains unreproduced.

For a focused gameplay diagnosis, set `gDeveloperTools.SwordAuraDiagnostics` to
1. It defaults to 0. Each model/rig/source kind logs once, for example:

```text
[SwordAura:OoT] logical=6 fit=22 rig=1 element=2 matched=1 final=<composed>
```

Fit 22 is the recovered full ModernDin blade; fit 23 is its actual broken blade.
Fit 6 is the shipped logical NEI Biggoron, 11 native OoT BGS and 20 the later
progressive Din BGS. Rig 0 is adult OoT, 1 child OoT and 2 MM human. A composed
hand's archive name is not inferred from its pointer. Different active packs
still require their exact resolved resource and configured visual evidence.
The expected Alt-off authored MM human line is
`[SwordAura:MM] logical=6 fit=6 rig=2 element=2 matched=1 final=<composed>`.
A different fit identifies a different runtime route. A matching line confirms
the selected fit and rig; it does not hash the installed held/GI resource graph.

## Verification

```sh
ASAN_OPTIONS=detect_leaks=0 python3 -B scripts/diagnostics/run_elemental_sword_aura_tests.py --sanitize
python3 -B scripts/diagnostics/run_biggoron_aura_tests.py --sanitize
python3 -B tests/elemental_swords/biggoron_depth_coverage.py --output /tmp/biggoron-source-depth.png
python3 -B scripts/diagnostics/preview_elemental_sword_auras.py /tmp/aura-offline-comparison.png
```

The aura runner executes actual GBI and vertices, the real held selector,
CustomEquipment compositor, imported native hand branches and selection core.
It covers all three elements, native/progressive/Din fits, assets off/Alt off,
pack-on hand preservation, arrow/checkbox gates, explicit None, exact hand-list
transfer, later replacement, body colors, captured matrices, pause, scene/frame
changes, ownership and duplicate submission. It also compiles the complete
renderer TU for both engines in solo and Combo modes.

The optional preview projects the exact generated vertices and I8 maps over a
neutral blade reference. It is an offline diagnostic, not an in-game screenshot
or evidence of a configured runtime visual pass.
