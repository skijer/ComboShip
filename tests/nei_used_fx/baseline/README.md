# Fixed historical comparison sources

These are exact source files from `marsh6487/Shipwright` at the revisions already
required by the used-effects tests. `sources.json` records each original commit,
repository path and Git blob SHA. The test loader checks every file against
that original blob identity before compiling or comparing it.

They were recovered through the repository connection during the October 16
merge audit because shallow/recovery checkouts did not contain the historical
commits. Tests preserve their original baseline semantics and need no network
or historic Git objects. Do not regenerate these files from current output or
format them. They retain the source repository's licensing.
