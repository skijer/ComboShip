# Saria grip profiles

Each JSON belongs to one exact source GLB SHA256 and the native ocarina at 0.8
scale. In `whole_arm_wrist_seat` mode, Left/Right `wrist_offset` values are in
native hand-local units. They move the complete arm through the two-link IK
solve; they must never translate the hand mesh away from its wrist. The prop
receives the inverse right-wrist offset, preserving the intended mouth/world
target. Measure the achieved pose too: IK reach limits and angle/vertex
quantization can prevent exact target agreement. This seated `Props/OcarinaGrip` is selected only by the four grip
clip overrides; the shared `Props/Ocarina` remains unchanged for other native
poses and scene placements. `curls` are independent bends for the original source finger joints.
Keep them in a natural range and review the actual shape before accepting a fit.
An optional `approach_wrist_offset` is an additional hand-local wrist target
offset for the lower/approach portion of an ocarina clip. `approach_start` holds
that clearance through the native wrist turn; `approach_end` sets where it has
fully faded out in the eased mouth blend (0 <= start < end <= 1). The
playing grip remains exact. Use it to clear the instrument during native wrist
turns; it moves the whole arm and never changes hand vertices.
Native global wrist orientation, clip timing, root and non-arm animation
channels remain unchanged; the upper-arm, forearm and local wrist rotations
adapt to reach the seated wrist with the original global hand orientation.

Optional right-hand `carried_offset` is a common body-space translation of
both wrists and the instrument in the lowered hold. `carried_support_end`
sets the mouth-blend fraction where that support has fully faded out. It can
keep the custom hands above bulkier knees while leaving playing contact
unchanged. Review raised/lowered poses for both prop and body intersections.

These files author private hand and forearm surfaces for the four ocarina
clips through the existing optional pose-limb resource contract. Re-exporting
an O2R is enough after the matching adapter build exists. Keep the source hash
and prop scale check in place when adding another model.

Recheck the full exported surfaces with native_npc_contact.verify_archive;
vertices alone miss a triangle that cuts across the instrument. Inspect all
angles and transitions after the archive check. Also verify wrist continuity
and visible finger placement over the prop front. Clearance alone can reward
an overcurled hand or a hand moved away from the instrument. Positive offline
clearance is not live-game acceptance.
