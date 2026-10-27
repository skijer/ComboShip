# MM Lost Woods: tiny rightward correction (POC9)

The user judged POC8 too far left and requested only a small rightward nudge.
Translate the five doorway resources by X +3, Z -7: about 7.6 units right along
the face, roughly one fifth of the preceding leftward move. Anchor (-695, 0,
-313); yaw remains 12288. The depth change is less than 0.1 unit.

Source parent: `8bfd4fe1d98c8bbbeec1ad44450de629bfbefbc4`.
Cumulative base: `43131342882972eeb4a257599ec8b061c7dc6d43`.
Input POC8 SHA256: `9e62cc55104968710b795d3765f01ba607184438fc8e910f37da41fdece94ded`.
Output POC9 SHA256: `72d750b00583379f3619dac52dce59e2957a351fdfabd3a3a8da11f9bc19daf0`.

All 882 doorway vertices retain Y/UV/RGBA and draw topology. Every texture,
mushroom/stump repair and other scene resource is byte-identical to POC8.
Exactly nine resources change plus a centered_v8 wrapper. Four setups select
it; one doorway remains active. Static native-root clearance is 3.50 units.

The source trigger, camera and trim use the same new anchor; v7 and all earlier
poses remain supported. The unchanged parent fails the new archive/source
identity fixtures before the change. Final archive and sanitized return/camera/
finish checks pass, as does production renderer object compilation. The actual
packed before/after previews use identical cameras. Reproduce with `--tiny-right`.

Use POC9 in place of POC8 and include the matching source in the next aggregate
build. Final visual acceptance remains with cor. No full linked build, configured
gameplay/collision acceptance, runtime shimmer proof or master promotion is claimed.
