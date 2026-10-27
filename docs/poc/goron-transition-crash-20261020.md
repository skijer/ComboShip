# Lost Woods exit: shield-particle light lifetime

Status: implemented and source/native-object verified candidate. No linked
executable or in-game transition has been tested here; no master promotion.

The user reports a crash leaving the Lost Woods bridge for Kokiri Forest while
in Goron form, plus separate Saria rendering defects. The supplied crash is in
OoT scene `SCENE_LOST_WOODS`, room 5, before destination scene initialization.
Its game build is `450b293646475d613c12151aaed802f82db73508`.

## Evidence

- Log SHA-256: `be26a9057f5b17c8d69fb4f23615520f34f0402c71fd24cc8135365141b443bf`.
- Dump SHA-256: `d85e431fb50d17b89f3de1e978957e5541a49f196fb2f92ab8cb71db93476376`.
- Matching `soh.dll` SHA-256: `12918af93eae166535242e9648ecace6205d8da232b9f9591caadef101b0f141`.
- Dump and DLL PE timestamp: `1792475570`. The 256 instruction bytes preserved
  around the exception match this DLL exactly.
- Fault: RVA `0xC6156D`, `mov rax, [rax + 0x10]`, with `RAX=0`.
- The matched leaf function is `EffectShieldParticle_Destroy`. Its entry tests
  `lightDecay == 1`, loads `this->lightNode`, compares it with the play-state
  light-list head, then reads `lightNode->next`. The null value and taken branch
  establish that both pointers were null. These field values follow from the
  executed instructions; the pointed-to data itself is absent from the dump.
- Caller RVA `0xCA934A` is the final shield-particle destructor invocation in
  `Play_Destroy`'s inlined `Effect_DeleteAll`, preceding soft-sprite cleanup.

The destructor treats two null pointers as a valid light. A failed light
allocation is permitted by `LightContext_InsertLight`; cleanup must tolerate
it. Separately, `Effect_DeleteAll` destroys inactive slots, and the previous
destructor retained released light pointers. Repeated cleanup could free a
node twice or remove a light that reused the slot in a later scene. The dump
does not identify why allocation was unavailable in this particular session.
Goron form is the reported configuration, not an established prerequisite.

## Bounded correction

Baseline source: `4af924940`, preserving the earlier native transformation
camera correction atop `613802447`. Only the shield-particle destructors in
OoT and MM change: require a non-null node, use the existing list-unlinking
function, and clear the pointer after release. No allocator, form logic, scene
data, actor placement, geometry, textures, or progression changes are involved.
The accepted inputs and existing source checkpoints remain unchanged.

## Verification

Run `python scripts/diagnostics/run_shield_particle_lifetime_tests.py`.
The test compiles actual production declarations and initialization/destructor
and light allocator/list functions with UBSan; only random generation and the
play-state accessor are fixture dependencies. All ten cases failed on the
unmodified source and pass after the correction: null node/empty list,
exhaustion followed by teardown, head/middle/tail unlinking, repeated release,
and next-scene slot reuse, in both engines. This is a source-level reproduction,
not a rendering or full game test.

Both full affected translation units compile in Combo and solo configurations.
Existing controller macro redefinitions and graphics pointer-conversion
warnings remain outside the changed destructor. The native transformation
sequence/camera tests pass, including both ages, all supported forms, donor
camera on/off, skip, facing, and the Windows `near` macro case.

Next runtime proof: on the rebuilt candidate, repeat the same Goron bridge
exit and return, then repeat a scene exit after shield sparks have expired.
Keep the installed pack order and Alt setting consistent with the reported
configuration. Human-form comparison is useful if another crash remains.
Saria's archive-reference and native face investigations are separate.

## Recovery

The candidate is an isolated source branch. The two-destructor change and its
test are a bounded commit, suitable for cherry-picking onto later integrations.
Retain the original POC model archives and prior source checkpoints unchanged.
