# MM Lost Woods: small leftward centering adjustment (POC7)

The user requested a slight move left on the same corner trunk, retaining POC6's
angle. Translate all five doorway resources by X -7, Z +17. This is about 18.4
units left along the doorway face, with less than 0.1 unit of depth change. The
anchor moves from (-677, 0, -357) to (-684, 0, -340); yaw stays 12288 (67.5 degrees).

Source parent: `a4cff96f27b8300232f9732bb3f595bc506c8bf2`.
Cumulative base: `43131342882972eeb4a257599ec8b061c7dc6d43`.
Input POC6 SHA256: `58b6b10dd1ac2cf532fab99b00dbed22ff5ca0a701f388ede2e8d920a75fa7f7`.
Output POC7 SHA256: `caa96a5941c24a07a32cf109c9a26e3fffa149ab2b9a5eab696e613c400a37ff`.

All 882 doorway vertices retain their Y/UV/RGBA and triangle topology. All
textures, mushroom/stump repairs and other scene resources are byte-identical.
Exactly nine resources change, plus one `opa_combo_return_centered_v6` wrapper;
four setups select it and the active graph contains one doorway. Rotated clipping
finds 3.33 units of static clearance behind the mouth. Before/after renders use
the same camera so the small leftward movement is directly comparable.

Source uses the new anchor for the trigger, camera and trim; v5 retains its
previous anchor. Earlier pack poses are also preserved. Sanitized return/camera/
finish fixtures and the production renderer object check pass. Archive/source
identity checks failed on the unchanged parent before the fix. Use the tool and
archive check's `--center-left` mode to reproduce this follow-up from POC6.

Use POC7 in place of the prior Lost Woods archive and add its matching source to
the next aggregate build. No full linked build, configured gameplay/collision
acceptance, runtime shimmer proof or master promotion is claimed.
