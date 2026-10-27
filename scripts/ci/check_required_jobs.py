#!/usr/bin/env python3
"""Fail the stable gate unless every expected job has the expected result."""

import json
import os
import sys


def main():
    try:
        needs = json.loads(os.environ["NEEDS_JSON"])
        code = needs["preflight"]["outputs"]["code"]
        if code not in ("true", "false"):
            raise ValueError("Missing or invalid preflight code output")
        expected = {"preflight": "success", "checks": "success" if code == "true" else "skipped"}
        failed = False
        rows = []
        for name, required in expected.items():
            actual = needs[name]["result"]
            rows.append(f"| {name} | {actual} | {required} |")
            if actual != required:
                print(f"::error::{name}: expected {required}, got {actual or 'missing result'}")
                failed = True
        report = "## Final required-check status\n\n| Job | Result | Required |\n|---|---|---|\n" + "\n".join(rows) + "\n"
        if failed:
            report += "\nThis final job reports earlier check results; it does not compile the games.\n"
            try:
                jobs = json.loads(os.environ.get("WORKFLOW_JOBS_JSON") or "[]")
                earlier = [job for job in jobs if job.get("conclusion") == "failure"
                           and job.get("name") not in ("gate", "Final required-check status")]
                earlier.sort(key=lambda job: job.get("completed_at") or "")
                if earlier:
                    report += "\n### Earlier failed checks\n\n"
                    for job in earlier:
                        name = str(job["name"]).replace("[", "\\[").replace("]", "\\]")
                        url = job.get("html_url", "")
                        steps = ", ".join(step["name"] for step in (job.get("steps") or [])
                                          if step.get("conclusion") == "failure")
                        report += f"- [{name}]({url})" + (f": {steps}" if steps else "") + "\n"
                cancelled = sum(job.get("conclusion") == "cancelled" for job in jobs)
                if cancelled:
                    report += f"\n{cancelled} remaining check(s) were cancelled."
                    report += (" Open the failed check above for the original error.\n" if earlier
                               else " No original failed job was reported; the run or its checks were cancelled.\n")
            except (ValueError, TypeError, KeyError, AttributeError):
                report += "\nDetailed job metadata was unavailable; the required failure remains recorded above.\n"
        print(report)
        if os.environ.get("GITHUB_STEP_SUMMARY"):
            with open(os.environ["GITHUB_STEP_SUMMARY"], "a") as summary:
                summary.write(report)
        return 1 if failed else 0
    except (KeyError, ValueError, TypeError) as error:
        print(f"::error::Cannot verify required CI jobs: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
