# Bundled player emotes

User direction: include all eight supplied animations in one implementation, bundle their assets and credit their creators. Multiplayer visibility is part of the implementation groundwork; separate one-animation POC builds are not useful at current build costs.

Baseline: `bridge/audit-merge-20261008` at `1bbb94e692629c0d910f97b57dd7cd02240d3775`. Work is isolated on `feature/bundled-emotes-20261016`. Runtime acceptance is pending.

## Behavior

- Four persistent slots select any of the eight built-in emotes. Each slot has a configurable controller-button chord; physical keyboard/controller mapping uses the existing input mapper. Defaults use M1 and the four D-pad directions. The shared Settings menu has one Emotes panel and in-menu Play/Stop controls.
- Support OoT child/adult and MM human Link, including compatible custom player models. Different transformation skeletons are excluded until authored animations exist for them.
- Start while idle and grounded. Movement, other actions, damage, pause, dialogs, cutscenes, transitions, game swaps and scene destruction cancel playback. Emotes do not change collision, grant invulnerability or perform gameplay attacks/jumps.
- Decode the packs' 134-byte big-endian frames as 22 joint vectors and appearance information. Play at their original 20 frames per second independent of renderer FPS. Preserve valid native face/equipment behavior.
- Bundle all eight animation streams in each game's normal custom asset archive. Bundle the two supplied sounds as 32 kHz mono PCM, with local SFX/master volume and cancellation. No external `.pak` or script runtime is required. Remote custom audio is outside this visual-sync task.
- Retain manifest author/license information. Display Malon Rose/MalonRose, Unesag and Neirn credits; leave Stylish Sheathing's unspecified author explicitly unspecified.

## Integration

Keep catalog, frame decoding, binding selection, playback clock and PCM mixing in reusable engine-independent code. Each game owns a small adapter for eligibility, input consumption, resource loading and native pose application after the animation queue resolves. Restore overridden native pose state before the next player update.

Anchor and Harpoon serialize the same selected emote pose through their existing joint arrays, including MM Anchor's aligned raw buffers. No emote identifier, pack transfer or server protocol extension is needed for visible movement. Preserve native pose serialization for players who are not emoting. Root movement flags and upper-body offsets must agree with the cosmetic emote pose.

## Evidence and acceptance

Automated checks cover all eight complete streams, bounds/endian handling, 20/30/60 Hz clocks, one-shot completion, retrigger/cancellation, chord edges and consumption, native-buffer alignment, and audio saturation/stop. Compile changed game/UI translation units against actual headers. Attempt a complete build only if dependencies are available; do not label static verification as a working game build.

One runtime candidate contains the complete set. Check all eight in OoT child/adult and MM human with vanilla/custom assets; then check visible movement and cancellation using two clients on Anchor and Harpoon. Runtime availability does not gate implementing the multiplayer paths.
