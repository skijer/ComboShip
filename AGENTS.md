# Project recovery requirements

cor explicitly requires recoverable source checkpoints for large integration
work. Preserve source before starting a broad change, after each independently
verified milestone, and before a long build, external publication or context
handoff. Do not wait for every workstream or the full executable to finish.

- Commit bounded changes on isolated candidate branches. An unfinished recovery
  commit is allowed when clearly labeled WIP with its known failures recorded.
- Save a durable recovery artifact containing source, exact refs/trees, required
  asset identities and restoration instructions. A commit in transient scratch
  alone is insufficient. Verify that the saved artifact restores independently.
- Preserve accepted input archives unchanged. Retain their hashes and the
  recipes for generated candidates; use deterministic archive metadata for new
  outputs. Save recovered irreplaceable inputs promptly.
- Delegate with explicit file ownership. Stage and commit only the owned paths;
  another worker may have uncommitted changes in the same worktree.
- Keep original/combined game guards and accepted masters intact. Distinguish
  source checks, native compilation, configured runtime proof and user acceptance.
- Record remaining issues and dependency/load-order requirements with each
  handoff so another chat can continue without relying on agent memory.
