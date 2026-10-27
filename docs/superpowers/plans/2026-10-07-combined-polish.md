# ComboShip daily polish integration plan

> Execute inline with superpowers:executing-plans; retain all source checkpoints and review the combined branch before publication.

**Goal:** Integrate all implementations delivered on 2026-10-07 into the latest ComboShip branch and push the combined candidate, as authorized by cor.

**Architecture:** Apply isolated deltas to develop `13901c677d0084e0305eec4672c0557f4434aea7`. Preserve both sides of shared renderer, receipt and test changes; use the final key-dialogue bridge for its complete key identity/color policy and Chest Game dispatch. Keep supplied owner-specific mod archives with their matching source.

**Tech Stack:** C/C++20, Git binary patches/bundles, Python executable regression fixtures, CMake and GitHub Actions.

**Spec:** Supplied POC4 README and the dated POC records imported with each source patch; cor's authorization covers integration and pushing to marsh6487/ComboShip.

## Global constraints

- Preserve accepted sword geometry/framing, independent owner selection, grant counts, native/foreign dispatch and optional-effect behavior.
- Keep protected original meshes, textures, scene packs and other prior fixes intact.
- Use the newer tunic/Ikana and key-dialogue bridge checkpoints; older overlapping bundles are recovery inputs only.
- Include bottle and dungeon-key mod assets and equipment review seeds; do not install old scene test fixtures as live scene replacements.
- Distinguish source/renderer/build evidence from in-game visual acceptance.

## Review focus

- Overlapping effect transforms must retain sword fitting, gold effects, Shadow Crystal colors and elemental arrow shimmer.
- Key receipts must preserve complete OoT/MM identity coverage, localization, icons, Chest Game dispatch and grants.
- Cape choices must survive foreign receipt fallbacks and preserve saved visibility and normal message completion.
- Native and foreign tunic, bottle, shield and boss paths must preserve independent resource owners and Alt cache refresh.
- Added material recipes must retain archive ownership, required build assets and reversible seasonal treatments.

### Task 1: Integrate renderer and receipt patches

**Files:** Exact paths named by POC4, sword, tunic/Ikana, final key bridge, cape, Sand/Storm, Shadow Crystal and elemental arrow source patches.

- [ ] Verify incoming bundle hashes and record baseline and delta provenance.
- [ ] Apply each source delta with Git three-way merging, retaining both sides of overlapping changes.
- [ ] Resolve key policy overlap using the final bridge's identity/color lookup and the POC4 localized formatter; preserve Chest Game exception dispatch.
- [ ] Run `git diff --check`, inspect conflicts and checkpoint each integration boundary.

### Task 2: Preserve matching assets and test seeds

**Files:** `tools/polish_20261007/`, `docs/poc/combined-polish-20261007.md`.

- [ ] Include exact bottle archives/source input and repaired dungeon-key archive, models, certificates and rollback.
- [ ] Include equipment test plandos, checklist and generation/validation scripts.
- [ ] Document ownership/load paths, superseded overrides, source checkpoints and runtime review cases.
- [ ] Verify archived resource hashes and regenerate bottle recipes/NEI bounds when required.

### Task 3: Verify and compile the combined branch

**Tests:** `scripts/diagnostics/run_combo_nei_regressions.sh`, the additional key/Chest Game, autumn foliage, Ikana, cape, boss, bottle, elemental arrow and Shadow Crystal fixtures, normal build workflows.

- [ ] Restore external test prerequisites and fixed historical donor commits.
- [ ] Run combined regression commands, sanitizers where provided, and required syntax/asset/format gates.
- [ ] Diagnose any failures against their exact baseline and fix only integration defects with a failing regression first.
- [ ] Run a separate whole-branch review; address material findings and rerun affected checks.
- [ ] Build the actual application locally or through its normal Actions workflow and inspect the result.

### Task 4: Publish and preserve recovery

- [ ] Push the candidate/checkpoint branch without rewriting the existing baseline.
- [ ] Inspect build checks for the exact pushed commit and integrate into the requested latest branch after required gates pass.
- [ ] Retain original baseline and a combined patch/bundle with matching assets and verification results.
- [ ] Handoff exact branch/commit/build links and disclose outstanding game runtime checks.
