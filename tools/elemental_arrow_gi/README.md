# Elemental Arrow GI POC1

The final-polish candidate, live cosmetic routing, real native-core preview and spell crystals are documented in [the elemental spell GI polish README](../elemental_spell_gi/README.md). The POC1 representative-core preview below is historical.

Fire, Ice and Light arrow receipt effects share the mod's medallion artwork and
NEI shimmer. Fire uses curling flame sheets, spiralling heat and rising embers.
Ice uses frost sheets, a pale spiral, a blue glow and tumbling faceted crystals.
Light uses white-gold rays, counter-rotating tapered arcs and a luminous head.

The native/selected arrow core keeps its existing display-list path. The three
GI-only aura resources live under `objects/nei_elemental_arrow_gi/` and are owned
by OoT in both hosts. The original medallion pack and its pixel payloads remain
unchanged. The only copied resource-header change declares the expanded payload
as RGBA32; the physical image dimensions and IMG flag are preserved.

Both native arrow callbacks and both foreign dispatchers use profiles 1 Fire,
2 Ice, 3 Light. Foreign draw kind 39 carries one opaque owner core and its
elemental profile. It appends an ABI enum value without changing struct layout.
The effect carries its own shared shimmer, including with optional Item Effects
off. The foreign wrapper therefore excludes its second overlay shimmer.

The complete effect is admitted before any arena mutation. Each child pass
retains the existing mesh validation/batching, and the caller restores resource
ownership, grayscale, setup, colors and modelview. Missing artwork keeps a
geometry fallback. The standard native/Alt resource system still selects the
core. Projectile actors, held arrows, combat, audio and original effect packs
are outside this receipt-only change.

## Reproduce the candidate

Baseline: `13901c677d0084e0305eec4672c0557f4434aea7` (develop, 2026-10-07).
Candidate checkout: `poc/elemental-arrow-gi-20261007`.

Apply the recovery bundle's binary patch in a separate checkout of that baseline:

```sh
git apply --check candidate.patch
git apply candidate.patch
```

Use the repository's normal Windows or Linux build instructions. The custom
OoT asset archive target embeds the three supplied files beneath
`soh/assets/custom/objects/nei_elemental_arrow_gi/`. The small supplied
`Elemental_Arrow_GI_POC1_Assets.o2r` is an alternative copy of those same three
private resources for `mods/soh/` in a matching candidate build.

The asset pack requires this source change. A recovery bundle contains source,
assets and previews; it contains no compiled game executable.

To regenerate private resources from the accepted original reference pack:

```sh
python3 -B tools/elemental_arrow_gi/build_textures.py PATH/zzz_Medallion_Magic_POC1_HD.o2r --output OUTPUT
```

The source archive hash and every unchanged pixel payload hash are pinned in
`material_manifest.json`. The builder verifies the archive's closed three-key
scope and copied resource bytes.

## Animation preview

```sh
c++ -std=c++20 -O2 -Isoh tools/elemental_arrow_gi/export_preview.cpp -o export-arrows
./export-arrows arrow-preview.bin
python3 -B tools/elemental_arrow_gi/render_preview.py arrow-preview.bin OUTPUT
```

The preview renders the production sampler at native packed position/UV
precision, the same original textures, standard alpha blending and opaque core
depth. It adds a neutral representative arrow as a size guide. The 18-second
loop uses 360 game ticks at 20 updates per second. The GIF shows every second
tick; the MP4 contains every tick. Original/native/Alt arrow geometry, actual
receipt pose, GPU interpolation, lighting and scene occlusion require a running
game check. This preview does not establish game runtime acceptance.

## Verification and acceptance

```sh
python3 -B tests/elemental_arrow_gi/run_tests.py
python3 -B scripts/diagnostics/run_nei_gi_tests.py --combo
```

These checks compile the pure sampler, actual OoT and MM shared renderer code,
and both production foreign dispatchers with real engine types/GBI. They cover
all profiles, bounded vertices/UVs, animated identities, native fast-math,
Alt/optional-toggle parity, absent-texture fallback, exactly one shimmer,
resource-owner and matrix restoration, and exhausted graphics arenas. The
MM three-pass peak is 9,968 OPA bytes / 304 XLU commands in the test fixture.
Existing real-header common/overhead/shop C translation-unit checks also pass.

The broad cumulative regression harness was attempted. It stopped in the
unrelated item-receipt seed-settings compile because this environment lacks
`spdlog/spdlog.h` and `imgui.h`. No full ComboShip application build or game
runtime proof was obtained here. The passing focused checks and the blocked
broad run are recorded separately in the recovery bundle.

The candidate stays isolated for review. Check all six native arrow awards and
both foreign directions in the intended ComboShip build, with Alt and optional
Item Effects on/off. Inspect overhead framing, arrow visibility, flame/frost/ray
material appearance, shimmer, actor/model rotation and nearby scene occlusion.
Then iterate on the actual result before promoting this candidate.
