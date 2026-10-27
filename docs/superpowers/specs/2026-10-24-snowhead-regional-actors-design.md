# Snowhead shared valley, regional actors and portal materials

## Approved scope

The user explicitly requested parallel implementation on October 24 after the design discussion. Build on the recovered overnight integration, not the older public baseline alone.

- Expose the vanilla Zora's Fountain valley by removing only its tree backdrop. The donor valley collision has 43 selected triangles/34 positions matching existing base XML render geometry. Add the missing fitted floor/wall collision; preserve native waterboxes, exits, actors, UVs and other terrain. Both vanilla and Djipi use the same open valley without an artificial Snowhead doorway.
- Build one separate natural rock/ice ascent on the RIGHT side shown in the references, accessible by ordinary child and adult swimming/walking. Account for every native adult iceberg's complete motion/tilt/orbit, not one sampled pose. Do not intentionally connect child Link to Ice Cavern; retain the native Ice Cavern trigger and sufficient separation.
- Ground the Snowhead transition and return in that valley. Ice-blue NEI shimmer spans its opening, with local falling snow and icy blue drift/chill. Effects must not take over scene-wide seasonal weather. No opaque artificial cap/doorway remains.
- Add placeable cutscene presentations for captain-area Stalchildren (seated laugh, toe tap, salute; branch swing only as an optional supported pose), Mikau (idle plus guitar presentation with explicit attachment), and Darmani's ghost (idle/float with proper translucent presentation). Extend the existing list without renumbering existing presets. Place a pair of Stalchildren beside the marked grave, Mikau beside Lulu at the Great Bay portal, and Darmani near Snowhead, with clear traversal/return lanes. Explicit Prelude placements suppress duplicate defaults. Match supported host/solo behavior of existing actors; preserve native actors' original quest logic.
- Make all added Lake Hylia cave and Snowhead ascent/valley materials replaceable after build through stable .o2r texture resource paths and usable UVs. Support replacement metadata/HD dimensions through normal engine texture loading. Document each material path and provide a deterministic sample replacement-pack recipe. Do not embed raw pixel pointers that bypass replacement metadata.
- Inspect the Woodfall-to-OoT return portal's apparently opaque magic/scroll layer. Identify the actual path and cause. Fix an unintended opaque animated layer while preserving the intended entrance background, scrolling and other portals.

## Preservation and evidence

Preserve recovered source tree fce96d959ecbeaf37b0e49b05a90684549084e9f (local checkpoint 9347ae570ee1921b55f1eb1952f901afb38370be, original candidate 729e3826db8d3adc04dd60ec668823b9ab230ede). Never merge develop/master. Preserve original archives unchanged. No original game archives or private evidence in public source. One shared doorway-less Snowhead valley in vanilla and Djipi. One separate right-hand ascent usable by child and adult, preserving adult moving icebergs and native Ice Cavern behavior. Child Ice Cavern access is NOT a goal. Ice-blue shimmer in the valley opening, local falling snow and icy blue drift/chill remain required. Post-build .o2r replacements must retexture all added Snowhead/Lake Hylia surfaces without code or recompilation. Preserve all overnight shop/title/spin/actor/weather fixes and original/combined/solo guards. Static/native checks are not configured gameplay proof.

Read-only private inputs are in /workspace/scratch/b408a3508381/implementation_evidence/{oot.o2r,mm.o2r} and /workspace/scratch/b408a3508381/scene_reference/Overworld_Without_Ranch_Kak_River_Meadow.prelude.o2r. Screenshots are /workspace/scratch/b408a3508381/upload/image(20261024-123613).png (Djipi valley) and image(20261024-125219).png (vanilla backdrop removed). Existing measured findings: /workspace/scratch/b408a3508381/ComboShip_Snowhead_Valley_Investigation_20261024.json. Never add those whole inputs to git.

## Approach

Use runtime source-contained geometry and existing resource-hook patterns, with no mandatory extra user scene pack. Keep vanilla/Djipi appearances distinct while sharing matching valley geometry and route semantics. The user chose the shared open-valley approach, superseding the earlier two-profile doorway proposal. Retexture resources may fall back to native art when no replacement pack is present.

## Acceptance

Meaningful red-to-green tests must cover presentation resources/attachments/translucency, existing parameter stability and duplicate suppression; native/Alt backdrop and valley fitting; ordinary child access and complete adult iceberg clearances; separate Ice Cavern collision; grounded trigger/return/re-entry; post-build replacement selection and UV continuity; and Woodfall portal blending/scrolling. Run relevant existing native/sanitizer suites and syntax checks, independently review each task and the combined diff, and save independently restorable source at milestones. Mark full executable or configured gameplay checks unrun if unavailable.
