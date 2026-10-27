# Black Market browsing and flame preview POC

Parent: `825a8cab131edfd90417ccdda0d605f465fefa0b`, preserved on
`poc/black-market-external-granny-20261018`. Candidate:
`poc/black-market-native-browsing-20261018`. Parent already has a durable,
independently restorable source checkpoint. This candidate is not a master
promotion or native runtime acceptance.

The prior implementation had three-way Buy / Next offer / Leave dialogue and
no browsing camera. The offline image used the raw decorative flame mask in
place of the native two-texture effect. These were real omissions; neither
an offline overview nor successful compilation proved native shop behavior.

The shared browsing controller now starts facing the Goth keeper. Left/right
selects the corresponding four-item bank; vertical input selects a row and
horizontal input selects the inner/outer column or returns to the keeper.
Analog input has initial and held-repeat delays; Dpad presses work directly.
A opens Buy/Cancel; the existing transaction runs only after a fresh A confirms
Buy. B returns to browsing, the keeper, then leaves. FastText/SkipText held B
cannot select or buy. Locked/owned/sold-out and prerequisite rules are unchanged.

Both engines allocate a native manual FREE0 subcamera, hide Link using the
native shop bit, ease the view toward each bank, and draw the native selection
cursor. The flat displays use room-specific camera poses; vanilla shop scene
background-camera tables and actors are not required. The production camera
export also drives the offline clip. Views reserve the bottom native textbox
at Y142 and fit the existing full authored FrameFit envelope, including
selected scale, bob and seal bounds, at 4:3. Arbitrary native or Alt replacement
models are outside that conditional geometry proof.

Camera cleanup checks the native UID generation as well as pointer/index,
preserves preexisting player-hide state and foreign dialogue, and repairs
native cutscene return links before releasing its camera. The manual view
stays outside OoT's one-point cutscene queue. An always-running frame hook
monitors interruption even when NPC updates are halted. Play destruction only
resets ownership; MM's destroy hook occurs after actor arena cleanup.

The regular room already uses exactly two original EnLight actors, at
`(130,76,-8)` and `(-139,76,-8)`, OoT params `0x7F5`, mapped to MM `0xBF5`.
Native flame type 5 supplies scale `.0075`, the original colors, billboard,
scrolling textures, flickering light and normal actor draw. This candidate
does not alter those actors. The preview now evaluates gEffFire1DL's two
textures and native `FC272C60/350CE37F` combiner instead of painting its raw
mask. Offline room smoke, filtering and lighting remain approximations.

Preserve the accepted POC14 O2R byte-for-byte, the optional external Djipi
dependency, all original room data except the prior RAM-only counter raise,
keeper crossed-arm lean, flat eight-item layout, stock, purchases, entrances,
solo/combined routing and both appearance checkboxes. No Djipi room or texture
bytes are bundled into source, a build, a new O2R or the recovery checkpoint.

Verification uses extracted production interaction/camera functions with real
MM/OoT types under ASan/UBSan, complete controller object compilation, native
transaction/recovery, keeper, seal, stock and Granny adapter/metadata checks.
Regression cases include all eight slots, both camera banks, pause, cancel,
purchase, no camera slot, held B, foreign text, cutscene return handoff and
same-address camera UID reuse. Review found and corrected the held-B and
camera-lifecycle cases. Native message control codes and manual camera update
ordering were also checked against each engine's source.

The remaining decisive test is an executable run in OoT-only, MM-only and
combined mode: enter each appearance, browse all eight offers with stick and
Dpad, buy/cancel/leave, pause and re-enter. Inspect the original flame/smoke
effects with the installed Djipi pack and Alt Assets off/on. This work does
not claim that those runtime configurations have passed or start a full build.
