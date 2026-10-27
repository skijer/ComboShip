# Optional external Granny interior

Baseline: `9ad00e33e7fcacb2c22fb1edaae5a84ec8649800`, with POC14
`3ac1432d0e8f75e51f08892739a13e16762bb4d820a2bbc7a35469af1db872e2`.
cor accepted its visual POC; full native runtime parity is still unproven.

The optional regular interior requires the user's installed Djipi 3DE scene
maps and 3DS textures. It must not add the original room, textures, or a converted
copy to a build, generated O2R, or recovery archive. Source may contain paths and
the procedural counter adjustment. Read the external resources at runtime and
retain their ownership. Clone display lists and the counter vertices in RAM;
never patch the shared Djipi resources or original Granny scene.

Preserve original materials, UVs, vertex colors, props, opaque/translucent
routing, lighting and decorative flames. Raise the audited main counter from
Y30 to Y41 with its existing seam closure; preserve the left book station.
Retain the Goth keeper's crossed-arm lean, eight flat floating displays, stock,
purchases, solo/combined access and exits. Keep the magical variant as default.
The checkbox applies on the next entry. Missing/incompatible dependencies leave
the magical room usable and visibly explain the dependency in the menu.
An independent Animated Item Seals checkbox preserves the effect while allowing
it to be hidden immediately in either room; unlock and purchase rules remain intact.

Implementation uses a shared private resource graph and small native room-shape
adapters. Native OoT resources supply the portions Djipi does not replace; they
are read from the user's base archive. Cross-game rendering must use the owning
resource manager. No entrance or scene routing changes are permitted.

Decisive native test: enter with the checkbox off/on, Alt Assets off/on, in
OoT-only, MM-only and combined configurations; inspect the whole room, two
flames, keeper and offers; purchase, exit/re-enter, reload and pause. This record
does not infer runtime success from a preview or object compilation.

## Candidate setup and evidence

Candidate branch: `poc/black-market-external-granny-20261018`.
Keep the existing POC14 market pack installed. No new Granny O2R is generated.
Install the user's Djipi maps and 3DS textures in the active game's mod stack.
In MM-only mode, the existing optional extracted `oot.o2r` companion supplies
native Granny smoke and metadata; OoT gameplay does not need to boot.

The shared controls are in the combined Black Market section and both native
Graphics menus (SoH Models & Textures; MM Mods). Regular Granny Interior defaults
off and applies on entry. Animated Item Seals defaults on and applies immediately.

External input identities used for verification, never included in the candidate:

| Input | SHA256 |
| --- | --- |
| Djipi 3DE 20 Scenes Map | `0f79d188684b1459b722437999a44101df48a1aae44fbfc5674ba51f9e9ad2d4` |
| Djipi 3DE 21 Scenes Map 3DS Textures | `cc2d44fc66b4a46e22db75207d5b8a518f02514a4452f443a02f482668aac81d` |
| Native OoT archive | `ea80d61b223ee38075fe5383b652998f3b08164e903134922f63f205d995717b` |

The original graph transport check covers 55 reachable lists, 337 vertex loads,
27 texture references and 4513 unchanged non-reference commands. Those commands
are represented by distinct opaque tokens in that fixture; it proves transport
and immutability, not actual game rendering. The actual counter buffer retains
all original vertex attributes except 46 Y coordinates increased by 11; eight
procedural seam vertices/four triangles close the raised step.

Native attachment fixtures execute production functions with real engine room
and environment types, checking four copied light settings, exactly two mapped
flames, missing-pack fallback and the next-entry latch. Native transaction,
keeper, stock and seal draw fixtures pass. Complete controller, combined menu,
and both native menu translation units compile with project dependency versions.

Review fixed foreign Scene cache contamination, MM-only dependency resolution
and solo-menu reachability. Raw metadata parsing never loads a native Scene into
shared caches; translucent resources use a pinned archive view and private route,
with guarded teardown. All truncated prefixes of both native headers are rejected.

The offline preview reads original assets locally and changes only the counter
buffer; it does not create an asset archive. Its smoke/flame combiner is an
approximation. Native Alt parity, pause/re-entry, normal browsing and transitions
remain unproven in an executable. The accepted magical POC14 archive is byte
unchanged; this candidate has not been promoted to master.

The subsequent `docs/black-market-browsing-poc.md` candidate corrects the
missing native browsing camera/input flow and the offline decorative flame
combiner. Its code/fixture evidence is separate from executable runtime proof.
