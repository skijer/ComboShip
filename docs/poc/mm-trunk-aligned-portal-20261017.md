# MM Lost Woods: doorway parallel to the trunk (POC6)

The user requested a closer match between the entrance face and the bark surface
at the same marked corner tree. POC5's doorway was square to the room axes.
Keep the doorway upright and rotate it 22.5 degrees in plan, then inset it
slightly toward the tree. Preserve every accepted texture and scene repair.

Source parent: `2d98c5b24bedb76bc2f9ec40a039049c81c19fef`.
Cumulative source base: `43131342882972eeb4a257599ec8b061c7dc6d43`.
Asset parent SHA256:
`e6c5defb683088100e23301fc9d44fa5a3ab17640905ff3b375a51724270005b`.
POC6 asset SHA256:
`58b6b10dd1ac2cf532fab99b00dbed22ff5ca0a701f388ede2e8d920a75fa7f7`.

The nearby trunk cross sections at Y 60/90/120 yield a horizontal face-normal
angle of 67.32 degrees. The new doorway yaw is 12288 (67.5 degrees), compared
with POC5's 16384 (90 degrees). Its anchor changes from (-660, 0, -350) to
(-677, 0, -357), an inset of about 18.4 units along the new facing direction.
The five doorway resources move together with rounded world X/Z coordinates.
All 882 vertices retain their Y/UV/RGBA, and all draw topology is unchanged.

Four room-zero setups now select `opa_combo_return_aligned_v5`. Active traversal
draws every doorway component once. Clipping native trunk/root faces against
the rotated opening leaves 2.48 units of clearance behind the mouth. All
textures, the mushroom seam repair, accepted stump tops, grass and other scene
geometry are byte-identical to POC5. Exactly nine existing resources change
(five doorway vertex payloads and four setup bindings), plus one new wrapper.

The source selects the same new anchor and yaw for the trigger, arrival camera
and native trim/shimmer. v1/v2/v3/v4 retain their previous respective poses;
recognized packs still suppress the temporary source-drawn arch. No new shader,
effect geometry, collision mesh, save/progression or weather changes are needed.

The unchanged archive failed the v5 identity check first, and the unchanged
source failed the new v5 return fixture first. The final archive passes exact
delta/preservation, transform, fitted bark alignment, setup/draw multiplicity and
root-clearance checks. Sanitized return, camera and finish fixtures pass with
native/v1/v2/v3/v4/v5 coverage and every day/form. The production renderer object
compiles and retains its native interpolation ABI. Before/after previews use the
packed assets and the matching native effect transform under neutral lighting.

Use the POC6 archive in place of POC5 and include the matching source in the next
aggregate build. It remains an isolated source checkpoint: no full linked build,
configured gameplay/collision acceptance, runtime shimmer proof or master
promotion is claimed. The source patches and previews are supplied for review.
