#!/usr/bin/env python3
"""Only a proven documentation-only PR may skip platform and regression jobs."""

import os
import subprocess
import sys


def main():
    try:
        if os.environ["GITHUB_EVENT_NAME"] == "pull_request":
            # checkout's PR merge commit has the current base as its first parent.
            # Disable rename detection so moving production code into docs still
            # includes the deleted build input in this classification.
            diff = subprocess.check_output(
                ["git", "diff", "--name-only", "--no-renames", "-z", "HEAD^1", "HEAD"],
                stderr=subprocess.PIPE,
            )
            paths = diff.decode("utf-8", errors="surrogateescape").rstrip("\0").split("\0")
            if not diff or not all(paths):
                raise ValueError("No changed paths; refusing to infer documentation-only")
            docs = {"README.md", "CONTRIBUTING.md", "CHANGELOG.md"}
            code = any(path not in docs and not (path.startswith("docs/") and path.endswith(".md"))
                       for path in paths)
        else:
            code = True
        with open(os.environ["GITHUB_OUTPUT"], "a") as output:
            output.write(f"code={'true' if code else 'false'}\n")
        return 0
    except (KeyError, OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"::error::Cannot classify changed paths: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
