#!/usr/bin/env python3
"""Run one independent regression group with strict shell errors and timings."""

import argparse
import json
import os
from pathlib import Path
import subprocess
import time


ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--suite", required=True)
    parser.add_argument("--manifest", type=Path, default=ROOT / "scripts/ci/regressions.json")
    parser.add_argument("--source", choices=("gate", "nei"), help="Select the legacy NEI-only entrypoint")
    parser.add_argument("--list", action="store_true", help="List commands without executing them")
    args = parser.parse_args()
    tasks = json.loads(args.manifest.read_text())
    selected = [task for task in tasks
                if (args.suite == "all" or task["suite"] == args.suite)
                and (not args.source or task.get("origin") == args.source)]
    if not selected:
        parser.error(f"Unknown or empty regression group: {args.suite}")
    if args.list:
        for task in selected:
            print(task["name"])
        return 0

    summary_path = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary_path:
        with open(summary_path, "a") as summary:
            summary.write(f"## Regression group: {args.suite}\n\n| Check | Result | Seconds |\n|---|---|---:|\n")
    for task in selected:
        print(f"::group::{task['name']}", flush=True)
        started = time.monotonic()
        env = {**os.environ, **task.get("env", {})}
        shell = ["bash", "--noprofile", "--norc", "-e", "-o", "pipefail"]
        if task.get("origin") == "nei":
            # The original umbrella supplied these headers only to its own
            # children and enabled nounset. Preserve both boundaries.
            shell.append("-u")
            if env.get("COMBO_NEI_FIXTURE_CPATH"):
                env["CPATH"] = env["COMBO_NEI_FIXTURE_CPATH"]
        result = subprocess.run(
            [*shell, "-c", task["run"]], cwd=ROOT, env=env,
        )
        elapsed = time.monotonic() - started
        code = result.returncode if result.returncode >= 0 else 128 - result.returncode
        status = "success" if code == 0 else f"failure (exit {code})"
        print(f"{status}: {elapsed:.2f}s\n::endgroup::", flush=True)
        if summary_path:
            name = task["name"].replace("|", "\\|").replace("\n", " ")
            with open(summary_path, "a") as summary:
                summary.write(f"| {name} | {status} | {elapsed:.2f} |\n")
        if code:
            # Surface the actual check in the workflow annotations immediately;
            # the final aggregate job may run much later than this failure.
            message = f"[{args.suite}] {task['name']} failed (exit {code}). Run: {task['run']}"
            message = message.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")
            print(f"::error title=Regression failed::{message}", flush=True)
            return code
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
