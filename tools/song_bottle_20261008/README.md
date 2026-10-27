# MM song and bottle GI POC — 2026-10-08

Base: `8276c87041a74f10af7d094fe64433b253cf7424` (cumulative recovery PR #39).
Branch: `poc/mm-song-gi-polish-poc2-20261008`.
This is an isolated presentation candidate. No master/develop promotion or
in-game visual acceptance is implied by its compile checks or previews.

## Songs

The existing native colored clef remains. These MM identities always submit their
matching shimmer and the selected particle profile, independent of the generic item-effects
toggle and the dormant donor's clock:

| Song | Particles |
| --- | --- |
| Song of Healing | Pink hearts and rising spirit trails |
| Sonata of Awakening | Green leaves, veins and seeds |
| Goron Lullaby Intro | Three red notation particles: quarter, eighth and paired eighth notes |
| Goron Lullaby | Six red notation particles, with the same three variants |
| New Wave Bossa Nova | Original seven blue bubbles and two lower ripples; one shorter staggered ripple above them |
| Elegy of Emptiness | Slow falling amber dust; #FF6200 shimmer |
| Oath to Order | Four widely spaced violet glints that twinkle; #620062 shimmer |
| Inverted Song of Time | Counterclockwise partial clock dial, ticks and trailing motes |
| Song of Double Time | Paired cyan hourglasses, falling sand and forward streaks |

Oath is the interpretation of the requested “other colored song.” Inverted's
clock motif is deliberate. Bossa Nova and Oath do not encircle the note with
large decorative rings. Accepted Zelda/Saria geometry is protected by recorded
sample hashes; the previous Prelude and Serenade decisions are preserved.

### POC2 visual direction

Cor requested that the vanilla native clef remain unchanged, both Lullaby
profiles use varied musical notation, and Bossa get one additional wave above
the existing waves. Elegy/Oath's figure-like silhouettes are replaced with the
falling amber dust and sparse violet glints that cor selected to finish today.
Their existing song-color shimmer remains. These motif choices do not imply
in-game visual verification or acceptance.

## Bottles and fairy

- MM native mushroom, Princess and gold dust receipts use appended draw IDs.
- MM randomizer mushroom and OoT-imported mushroom route to the filled bottle;
  gold dust's bottle no longer uses the empty seahorse alias. Refills are unchanged.
- OoT native mushroom/gold-dust callbacks and both foreign consumers share the
  same content composer. Mushroom uses the MM GI mesh; Princess uses her MM
  idle skeleton and blinking eyes; gold dust uses a mound with moving flecks.
- The casing draws last at the original bottle pose. Graphics admission reserves
  complete content budgets and leaves the glass/restore tail available.
- Princess has no randomizer item identity in this base. Her existing native
  receipt and the generic exported native recipe are covered; this does not add
  a cross-game randomizer Princess grant or pool entry.
- Both built port archives include the TP casing under a private non-Alt path;
  fairy bottles no longer require an optional mod pack to get that default shell.
  Fairy-specific, Blue Fire and generic selected-mod precedence is preserved.
- Animated fairy limbs explicitly select the active host's GPU resource manager
  while inside a foreign bottle draw. The bundled shell uses a visible bounded
  bounce centered at Y=-10 rather than the vanilla billboard anchor/scale.
- Ordinary bottle geometry, source UV/texture bytes and item-grant semantics are
  preserved. New draw kind 40 and MM GIDs 0x76..0x78 are append-only.

## Reproduce

```sh
python3 -B scripts/diagnostics/run_song_gi_tests.py
python3 -B scripts/diagnostics/run_bottle_contents_tests.py
python3 -B scripts/diagnostics/run_bottle_gi_tests.py
python3 -B scripts/diagnostics/run_fairy_bottle_tests.py
python3 -B scripts/diagnostics/run_nei_gi_tests.py --combo
python3 -B scripts/mods/build_bottle_contents_gi.py \
  --source-o2r tools/polish_20261007/source/TP_Bottle_Potions_POC3_Liquid_Sheen_OoT.o2r
python3 -B tools/song_bottle_20261008/render_preview.py --out /tmp/mm-song-previews
```

Real-header compilation requires the repository's SDK dependencies (SDL, JSON,
ImGui and thread pool headers). The recovery ZIP records the exact local
verification results and commands separately from runtime evidence.

## Preview and runtime proof

The preview exports the production C++ particle/shimmer triangles at the same
1/16-unit quantization used by the mesh renderer. It uses ordinary alpha blending,
a fixed offline Mesa camera and a **schematic gray note guide**. The native music
note resource, its lighting, the game GI camera and the live framebuffer are not
captured. GIFs show an eight-second clip; the loop boundary is a preview edit.

Still required before acceptance: native MM song receipts, shops/placed items and
both foreign directions in the actual configuration; bottled contents and fairy
wings/glow/bounce with no mod, selected TP pack and fairy-specific pack. No ROM
archive or running game was available here, so none of those runtime views is
claimed as passing.
