# Kakariko texture corruption investigation and command-preservation candidate

## Baseline and evidence

- Supplied archive: `Overworld Scene Edit - FINAL.prelude.o2r`, 9,093 ZIP entries.
- SHA256: `5a729d2f819b05cdb9aab84aedb3ee6045dc56d3714a13937374dc249396870a`.
- Supplied clip: `MedalTVScreenRecording20260930180621677.mp4`, 30.033 seconds.
  Sampled frames show Kakariko with Alt assets, rain, and broad textured surfaces
  and triangular fragments above the village. The clip does not identify their
  submitting display lists or prove a particular engine cause.
- `Fleet of Harkinian(10).log` contains 21 accumulated sessions. The last session
  begins at 16:36:36.889 and reports `4403ac0`, PR #26's test merge, whose tree is
  `cd0a392ff70c3e7b724f33f385c266d4805e347c`. It matches PR head `456033dc`.
- Reproduction baseline: `456033dcae5b81927396eb8922d30b97a9fbd06f`.
- Integration parent: `87d5ea7662b4a3b06bc114fa47651510790d6d25`, PR #28.
- Branch: `poc/kakariko-texture-trace-20260930`; isolated renderer candidate
  stacked on the existing audio/dungeon-reward candidates, with no promotion.

The new FILEPATH diagnostic was present. At 17:41:31.646–647, in Market Day
(scene 32), it named `objects/object_ahg/VillagerEyelid_pal_rgba16` and
`objects/object_ahg/VillagerEyeball_pal_rgba16`, Alt=1. These are NPC palette
failures, not identified Kakariko terrain failures. The earlier session also
named `objects/object_haka_objects/m_Fkousi1_model_A80___Copie_pal_rgba16`.
The 20,162 anonymous FILEPATH errors belong to older sessions, not this capture.

The latest Kakariko entries are scene 82 at 17:51:27.601 and 18:05:24.450 after
guest-house re-entry. There are no newly reported texture errors in that span.
FILEPATH reports are deduplicated for the process, so silence alone cannot prove
that all texture submissions succeeded. Raw texture failures were silent.

The matching RenderFlight log has only its startup record, `enabled:false`.
It contains no frame/command/GPU timing samples. FPS causation cannot be measured
from this capture, and this candidate makes no FPS-improvement claim.

## Static archive findings

Kakariko has 627 entries under `spot01_scene/`. A binary hash dependency audit
and XML resource/vertex-range audit found no absent literal texture dependency,
out-of-range vertex load, or unresolved binary hash in this scene. Its XML
segment-8 texture binding is dynamic and cannot be resolved statically.
A triangle reconstruction across the included Kakariko lists and setups found
18,213 triangles with their referenced vertex slots loaded. This is a
structural check, not an in-game render or proof of correct runtime ownership.
Other loaded packs and resource lifetime remain outside this archive-only check.

| Requested scene | Archive named in the latest runtime mount log | In supplied archive |
|---|---|---|
| Kakariko | `Overworld Scene Edit - FINAL.prelude.o2r` | Yes |
| Hyrule Field | `!Hyrule Field - FINAL.o2r` | No `spot00_scene/` entries |
| Lost Woods | `Lost_Woods_Tunnel_POC10_Y65_Game.o2r` | No `spot10_scene/` entries |
| Market | `!Market Suite FinalMaster - R15.o2r` | No Market Day/Night/Alley entries |

## Proven code defect and candidate

`gfx_step` advances after a handler returns false. Three texture failure paths
also advanced too far inside their handler: a raw `G_SETTIMG` OTR lookup failure,
an unknown `G_SETTIMG_OTR_HASH` hash, and a known hash whose resource failed to
load. Each discarded the next command. If that command sets up vertices or a
material, subsequent drawing can use stale state. The uploaded log does not
establish that one of these paths occurred during the clip.

The candidate corrects only these failure-path increments. Successful lookup,
texture binding, active resource-manager selection, Alt policy and prior texture
state on failure retain their existing behavior. New `TextureTrace` errors cover
raw/segmented rejection, raw OTR lookup failure, unknown hashes, hash lookup
failure and null image data. They include reason, path when available, source
address/hash, resolved address, active manager and Alt state. Reporting is bounded
to 128 contexts plus one overflow notice and never bypasses lookup/recovery.
These are failure records, not a complete scene or display-list execution trace.

The scene archive is unchanged. Geometry, collision, headers, actors, textures,
UVs, material bindings, weather and audio are outside the candidate diff.

## Verification and next decisive test

The production-handler fixture reproduces all three command-skip failures on
`456033dc`. The candidate passes command preservation, successful raw/hash
binding, prior-state preservation, repeated failures, manager/Alt distinctions,
bounded logs and recovery after saturation under ASan/UBSan. The existing texture
address and cross-game texture-owner/FILEPATH suites also pass with sanitizers;
the low-address mapped-module fixture passes separately. Local LeakSanitizer was
disabled because this environment cannot inspect `/proc` tasks; no ASan/UBSan
errors were reported. The new fixture is wired into the existing Linux renderer
CI step. Full Windows/Linux builds and in-game validation have not run.

Affected production/fixture files are unchanged between the reproduction baseline
and PR #28. This branch retains its audio and dungeon-reward changes unchanged.
PR #28's Windows/Linux workflow `36761967272` passed; that proves the parent
builds, not that shop music is repaired in the user's configuration.

### MM shop-audio follow-up

Cor reports that MM shop music is still broken while authorizing this renderer
candidate. Treat audible resolution as failed/unresolved, not accepted. The
exact build used for that latest shop test is not established by a new log.
The supplied `4403ac0` session predates PR #28's audio worker/mixer changes, so it
cannot establish whether those changes were exercised. Preserve that distinction
without discounting the reported failure.

The audio dependency is included explicitly to deliver the authorized cumulative
build, not as a proven fix or a new audio experiment. This renderer diff does not
alter it. Record the new build's startup commit and repeat idle/browse/buy/fanfare,
shop re-entry and game switching. If crackling persists on that identified build,
use its corresponding log and a short audible capture to distinguish synthesis
or mixing distortion from queue/device interruptions before another audio patch.

First compare the same Kakariko route/camera and guest-house re-entry on a build
containing this candidate, with the same packs, load order and Alt setting. Inspect
the new failure records and clip together. If corruption remains without a
relevant failure, investigate live vertex ownership/draw state rather than
declaring the missing Market eye palettes to be its cause. Extend a demonstrated
finding to Hyrule Field, Lost Woods and Market using their actual scene archives
and equivalent timing captures. Candidate is implemented and locally tested;
Kakariko root-cause attribution, runtime acceptance and master promotion remain
unproven. Original archive/build are the recovery baseline.
