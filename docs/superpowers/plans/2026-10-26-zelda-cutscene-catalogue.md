# Zelda Cutscene Catalogue Implementation Plan

**Goal:** Add child Zelda to the independent cutscene actor catalogue with custom model replacement support, and append a visibly distinct adult Zelda hands-clasped pose.

**Baseline:** 749f3035c74d132027e328178123615ccc0e88df; portal gates already native/static verified and durably saved separately. Keep all those changes.

**Design:** Append child actor ID 28 using free params 0x7D03 (standing) and 0x7D13 (hands clasped); object_zl4/gChildZeldaSkel and its native animation resources run through the same public ResourceMgr path used by the adult adapter. Bind both eye segments and the neutral mouth every draw, preserve blinking and native dress limb handling, and use the loaded skeleton's limb/matrix counts. This is an ambient viewer entry with no talk/proximity/story actions. Normal En_Zl4 is untouched. Append adult 0x7F24 using the distinct native 20-frame gZelda2Anime2Anim_003FF8 loop; preserve 0x7F04/0x7F14 standing aliases, object_zl2 and its face-matrix contract. Keep pose tracking off for the new clasped presentations. Existing catalogue collision conventions remain unchanged.

**Scope:** C++/C native policy, viewer dispatch, catalogue docs and focused fixtures. Native/Alt model resource resolution must remain dynamic. No authored model/archive is altered, no actors are automatically placed and no scene artwork/glow is changed. Child/adult scene placement can use the separate catalogue entries in normal age/setup headers. The selected native loops supply the pose; no new proprietary animation bytes are copied into source.

## Task 1: Append child entry and adult pose

- [x] Write and run a failing catalogue test against the parent: child param is unavailable and adult new pose aliases the old one.
- [x] Add append-only type/adapter/skeleton/animation descriptors, parameter decode and ambient talk exclusion; add viewer animation/skeleton/draw support. Update the catalogue and narrowly update previous invalid-param checks for the newly allocated ID.
- [x] Exercise production skeleton init through the actual native resource loader boundary using native and replacement skeleton dimensions; exercise face/blink/dress rendering and actor-state preservation. Decode actual native loops from the unchanged supplied oot.o2r and confirm distinct adult hand placement, valid joints and frames.
- [x] Run catalogue/controller, full viewer native/Combo compilation, existing rendering/lifecycle and affected registry checks. Run separate whole-change review; commit and save cumulative source plus additive patch and independent restoration proof. Leave full build/runtime explicitly untested and masters unmerged.

## Review focus

Existing enum/param values and both adult aliases must stay stable; child must use OoT resources despite appended ID after MM entries; both eyes/mouth must be rebound for replacement meshes; no native proximity/reward/cutscene callbacks; adult face matrices and portals stay intact. No blanket custom-pack runtime acceptance is implied by the fixture.
