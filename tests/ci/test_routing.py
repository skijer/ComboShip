"""Unknown and renamed build inputs must never be treated as documentation."""

import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


SCRIPT = Path(__file__).resolve().parents[2] / "scripts/ci/route_changes.py"


class RoutingTests(unittest.TestCase):
    def route(self, paths=(), event="pull_request", rename=False, broken_history=False):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)

            def git(*args):
                return subprocess.run(["git", *args], cwd=directory, check=True, capture_output=True)

            git("init", "-q")
            git("config", "user.name", "CI test")
            git("config", "user.email", "ci-test@example.invalid")
            if rename:
                (directory / "source.cpp").write_text("preserved bytes\n")
                git("add", ".")
            git("commit", "-q", "--allow-empty", "-m", "baseline")
            for name in paths:
                path = directory / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text("preserved bytes\n")
            if rename:
                (directory / "source.cpp").unlink()
            if not broken_history:
                git("add", ".")
                git("commit", "-q", "--allow-empty", "-m", "candidate")
            output = directory / "output"
            result = subprocess.run(
                [sys.executable, str(SCRIPT)], cwd=directory,
                env=dict(os.environ, GITHUB_EVENT_NAME=event, GITHUB_OUTPUT=str(output)),
                text=True, capture_output=True,
            )
            return result, output.read_text() if output.exists() else ""

    def test_only_explicit_markdown_documentation_may_skip_builds(self):
        result, output = self.route(("README.md", "docs/CI.md", "docs/nested/notes.md"))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(output, "code=false\n")

    def test_unknown_root_inputs_and_new_directories_require_full_ci(self):
        for path in ("run-clang-format.ps1", ".gitattributes", "asset-collisions.json",
                     "new-component/input", "docs/poc/fixture.json", "tests/new/test.py"):
            with self.subTest(path=path):
                result, output = self.route(("docs/notes.md", path))
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(output, "code=true\n")

    def test_production_file_renamed_into_docs_still_requires_builds(self):
        result, output = self.route(("docs/moved.md",), rename=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(output, "code=true\n")

    def test_missing_history_and_empty_changes_fail_instead_of_skipping(self):
        for broken in (True, False):
            result, output = self.route(broken_history=broken)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(output, "")

    def test_pushes_and_manual_runs_require_full_ci(self):
        for event in ("push", "workflow_dispatch"):
            result, output = self.route(event=event)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(output, "code=true\n")


if __name__ == "__main__":
    unittest.main()
