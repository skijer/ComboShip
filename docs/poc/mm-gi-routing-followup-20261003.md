# MM foreign OoT GI follow-up candidate

Baseline: PR33 `69957d96eca1ddf1880c0f7df3c21acac2bc75e8`, tree
`07da4fa13dd39661e5d2d70bc69f26371f40bdd7`. The reconstructed local parent
`fb8a166` has that exact tree. This candidate is isolated from that baseline.

Cor confirmed blue potion GI/message icon, blue-fire bottle GI/message icon,
and MM flame with the custom Volvagia head. Those are runtime evidence for the
baseline. New candidate tests are command tests, not new game acceptance.

| Request | Candidate behavior |
| --- | --- |
| Masks and transformation colors | Same procedural NEI `SampleShimmer` geometry through shared renderer in both hosts; existing transformation/remains palette retained |
| Progressive swords | Resolve actual concrete award first, select standalone OoT owner Alt equipment/Din blade when available, preserve shimmer on native fallback |
| Custom equipment | Typed concrete MM draw callbacks for inline equipment; authored paths for static/split models, owner-aware routes and validated operation metadata |
| Seasons | Four season color/flame recipes plus Rod of Seasons |
| Slate and runes | Authored slate mesh and native rune flame colors |
| Scepters, hourglass, shadow crystal | Authored static/split recipes rather than unsupported-callback sentinel |
| Actions | Existing crawl/climb/open-chest/grab GI recipes retained; no standalone `RG_SWIM` exists, swimming uses scale ownership |
| Barinade | Native frozen skeletal pose, procedural tentacle rotations/scales, electric/ring/translucent passes and MM-owned soul flame, including selected compatible Alt models |
| Twinrova | Selected model and native flying clip; custom head geometry preserved with native ice-hair/scroll post-pass |
| Native MM imported OoT souls | All nine imports query the same donor model recipes by exact item name instead of always drawing a skull; unavailable owner/module retains retry/fallback |
| Morpha foreign OoT GI | Actual membrane/nucleus, native rotation/scroll/color, MM native flame; selected owner Alt paths |
| MM souls rendered in OoT | All four native boss GI models/clips/flames, including Twinmold head/skin and normal/flex matrix support |
| Bean pack icon | Explicit magic-bean texture binding, avoiding randomizer/native itemId collision |
| Keys | Live OoT key body/emblem hex and rainbow sampling from MM frames; Well/Shadow settings remain separate; native grayscale scopes and acquisition latch refresh |
| Heart pieces/containers | Native OoT editor material patches; custom body-only grayscale with untinted border, including both-XLU models; live colors after acquisition |
| RPG magic jar | OoT cosmetic color carried with custom selected-asset classification; MM submits native-equivalent grayscale scope, refreshes after acquisition latch |

See `boss-soul-model-routes-20261003.md` for all nine OoT/four MM boss routes
and animation clips. Native SoH already advances those skeletal clips. Morpha
uses procedural rotations/scrolling, not a skeletal clip or native bobbing.

## Verification

Production-body tests cover the draw descriptor, actual award resolution,
owner Alt selection, routing validation, custom submission, inline axe draw,
mask/remains dispatch, exact shimmer mesh, C/C++ linkage, Morpha/MM soul recipes,
native MM flame, icon staging, and magic/key/heart cosmetic and latch behavior. Boss checks cover native Barinade
callbacks, electric/ring/translucent effects, matching native headers, custom
Twinrova head plus ice hair, all nine imported routes and matrix restoration.
Resource reload tests reproduce and fix pointer-address aliasing in limb paths.
Key tests compare the actual native rainbow tick against the bridge sampler,
including phase wrap and synchronization; heart tests check donor patch IDs,
reset and both-XLU consumer border/body grayscale separation. Focused suites
pass normally and under ASan/UBSan with leak detection disabled because the
container cannot inspect `/proc` for LeakSanitizer.

Both-host native color/syntax tests, foreign skeleton/Alt/fallback, spin,
NEI GI/held renderer and MM NEI suites pass. Independent review found and resolved
magic latch freezing, an early flame declaration, sword shimmer overwrite,
a C return-type mismatch, and metadata-only Alt magic aliases. No grant/action gameplay code changed.

## Remaining evidence and source limits

- Full application build and in-game appearance remain unverified locally.
- TP boss archive retrieval returned HTTP 502 twice; exact installed custom
  model/skeleton mappings remain unverified. Owner canonical Alt routes are
  preserved rather than inventing resource names.
- Frozen sword award recipes retain the selected Alt appearance until cache
  reset. Live Alt toggles during a latched progressive award are not proven.
- The requested death-skull texture swap is not implemented. The supplied
  image is a cropped save-panel reference with surrounding background, while
  the native save-panel art is embedded in IA16 file-info textures. A clean
  source texture is needed for an exact standalone RGBA icon replacement.
- The full local combo regression script cannot read its historical
  `c77c185...` Git object from the recovered snapshot. Downstream suites were
  run separately; this is not a full-suite pass claim.

Runtime acceptance should compare Alt off/on with the same pack/load order:
Barinade/electric effects and Twinrova/ice hair, Morpha vs accepted Volvagia; each progressive sword family; Seasons/runes;
scepters/hourglass/crystal/equipment; mask transformation hex and shimmer;
bean icon; RPG magic color; Well/Shadow key hex/rainbow; heart piece/container
body and border colors; blue potion and blue fire regression checks.
Native Twinmold also uses the shared selected-asset route in MM, retaining its
native flame phase. The intended presentations match the native GIs (Twinmold
head, Morpha core, Volvagia head), with their special effects. Rebuild both
modules together for the appended skeletal ABI. No master promotion is part of
this candidate.

## Bridge review on October 3

Final review found and corrected two integration issues in the dormant-owner
asset classifier: its translation unit now includes ResourceManagerScope, and
exact root loads run inside the owner scope so skeleton factories load nested
limbs through OoT rather than the active MM resource manager. The focused static
GI regression failed on that ownership boundary before the fix and passes
normally and with ASan/UBSan afterward. Magic, sisters and heart checks pass.
Full application build remains pending; no runtime acceptance is inferred.
