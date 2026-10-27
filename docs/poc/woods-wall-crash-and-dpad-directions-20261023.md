# Woods wall crash and D-pad direction corrections

Status: both corrections are implemented and locally verified. Configured
Windows gameplay remains unproven. Public source publication and hosted CI
remain on hold pending authorization; develop and master are unchanged.

## Crash identity and trace

The supplied October24 04:08UTC log identifies build
`6bd13184298bf5541855693db07aaf805e81ad43`, compiled October24 00:22:50.
Its tree `32e1dcb6a49e936bdab838c820052e206400ba9f` is identical to candidate
baseline `196ac9b7f50721dbc5102d6bf0d6b1f9b3fc58eb`.
It contains none of the prepared native-camera/title corrections.

The access violation occurs during MM room rendering in Woods of Mystery,
room1, after an owl-save load on return from OoT. The matching Windows artifact
is from Build Artifacts run40040174118, artifact12540009707
(`ComboShip-windows`, SHA256
`df5844cd4bfbbd6d482790889d3325396c219df0f51b31f52b0d8acc1f39c8e1`).
Disassembly confirms `libultraship.dll` RVA`0x38c14` is the first byte read in
`Fast::Interpreter::ImportTextureRgba16+0x404`.
The saved registers identify a64x64 decode at row44,column8, byte offset5648.

The dump write failed: all15 stream-directory entries are zero. Its exception
stream at file offset`0x660` and referenced1232-byte AMD64 context at`0x6492`
survive and agree with the log. The dump does not provide the fault-time stack,
texture allocation or resource path. The resource attribution below follows
the matching shipped asset and native scene path, rather than a recovered heap
object.

## Root cause and introduced change

The native bridge replaces room01DL_000B90 with
`custom/cor_woods_bridge/wall`. In the exact shipped archive this wall selects
`scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORITex_00A1C0`, a32x32 RGBA16
resource containing2048 bytes, but emits a64x64 load requesting8192 bytes.
This matches the crashed format, dimensions and invalid read beyond the payload.

The baking tool's simplified mesh reader associates the last image with
sampler0's tile size. The original native wall has two samplers: sampler0 is a
64x64 intensity layer, while sampler1 is the32x32 RGBA16 image. This conflation
affected every retained/clipped wall face, and the annulus also explicitly
requested64x64. Correcting only the annulus would leave the fault intact.

The erroneous asset and generator were introduced by
`b3049a6e285b6e3a0536cc8ac3e33f71b653e07f` on October19. The asset is unchanged
between that introduction and the supplied build. An invalid read may appear
to succeed when the neighboring memory is readable; that is an explanation for
intermittency, not proof of what changed in the user's earlier runtime.

The narrow correction derives the baked wall's extent from its actual native
image. Only four command attribute groups change: load masks6→5,
load count4095→1023/DXT128→256, render line16→8/masks6→5, and tile edge252→124.
All339 XML elements retain their ordering. Wall/bridge/canopy vertices, UVs,
normals and all three baked texture resources remain byte-identical.
The original sampler1 uses zero S/T shifts, so no UV edit is needed.

## Wall verification

- The recipe reproduced all eight baseline resources and the original manifest
  exactly before modification, using the immutable native and acceptedPOC5 inputs.
- The new regression rejects the matching shipped wall before an invalid read:
  `wall requests 8192 bytes from a 2048-byte native texture`.
- The corrected material executes the production tile/load/RGBA16 decode
  functions under ASan/UBSan, using both deterministic fixture pixels and the
  original native image. The32x32 upload matches every decoded native pixel.
- Native asset preservation and deterministic regeneration pass. The native
  bridge ownership/restoration fixture passes across Alt, reload, room, scene,
  missing/custom-resource and failed-patch cases.
- All21 CI policy tests pass. The wall regression is registered in the world
  group. LeakSanitizer is disabled in the isolated native probe because this
  environment does not permit its process inspection; address/UB checks remain on.
- Independent review confirmed the other three baked material loads match their
  scaled payloads, and found no remaining production issue in this correction.

## Reversible wall test pack

`zzzzzz_ComboShip_Woods_Wall_Extent_Hotfix_POC1.o2r` contains exactly one
resource: the corrected `custom/cor_woods_bridge/wall`. SHA256:
`a6f3f418c0dbf801cea0f7a9e8735ca7890937e1392ab07566b6ff6cbf19fc43`.
Its XML SHA256 is
`95dc371107cc27052142c1af5cbd27f50edf80a7bdbd7f3a3aeb58402837c5e1`;
the faulty XML SHA256 was
`ba8ee52d4d272a634c666c5f2e6005c0390fadba3d530dd7044627c14f1dc88f`.

For the supplied build, place the pack in`mods/2ship`, with precedence over any
other replacement of that resource, then fully restart. No executable rebuild
is needed for this isolated asset trial. Remove the pack to restore the bundled
resource. Verify the same OoT→MM/owl-resume route into Woods of Mystery room1,
then Alt parity, room re-entry and the bridge appearance. The pack contains no
D-pad, camera or title-card changes. Configured runtime acceptance is pending.

## Unexpected Down D-pad stick

OoT stores D-pad buttons as Up,Down,Left,Right; MM stores Right,Left,Down,Up.
The previous departure/arrival logic copied indices positionally. Executing
the real transfer functions proves that a physical Left stick in either game
becomes a physical Down stick in the other, including its actual native item
and inventory slot. A four-direction pattern exposes all four misplaced
directions. This is an equip transfer error, rather than only a HUD icon error.

The correction uses tagged physical Up/Down/Left/Right order, named MM slot
constants, and decodes untagged in-flight payloads in the old peer's order.
It preserves C-button policy, custom16-bit items and native slot/shadow rules.
Order metadata is consumed with the arrival equips and excluded from FleetNet.
The22 ASan/UBSan scenarios pass: all directions in both games, Left-stick
preservation with Down empty, all-empty transfers, old untagged payloads,
malformed-order handling, all MM forms and custom-item/unsupported policies.

The mismatched MM loops are already present in October9 commit
`0914f957ed1260ad1c36102db16f2f67ce58191a`; later October19 edits retain them.
This establishes a pre-existing transfer defect, but does not establish when
the user's current saved assignment first appeared. Fresh-save initialization
and an all-empty transfer both preserve empty D-pad assignments. The supplied
log does not contain the faulting equip payload or D-pad save fields, so the
exact reported saved state still needs configured verification. The fix cannot
safely identify and erase an already persisted unwanted equip; the existing
empty-source policy retains destination-local assignments.

## Preserved work and remaining proof

All35 prepared native-camera/title source identities are unchanged by this
crash investigation. Their earlier native/asset/regression results remain
recorded in their separate POC documents. The combined recovery artifact
contains these changes and this additional wall/D-pad correction.

Remaining: full Windows/Linux builds and configured runtime checks for the wall
arrival crash, D-pad direction retention, native shop browsing and title-card
rendering. No runtime acceptance or master promotion is claimed.
