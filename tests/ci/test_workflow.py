"""Pin independent completion and required coverage in the actual Actions graph."""

import json
from pathlib import Path
import unittest

import yaml


ROOT = Path(__file__).resolve().parents[2]


class WorkflowTests(unittest.TestCase):
    def test_cumulative_native_ikana_and_silver_repairs_are_required(self):
        tasks = json.loads((ROOT / "scripts/ci/regressions.json").read_text())
        required = (
            ("python3 -B tests/ikana_gi/run_native_priority_tests.py", "items"),
            ("python3 -B tests/ikana_gi/run_native_availability_tests.py", "items"),
            ("python3 -B tests/oot_equipment_pause/run_ikana_icon_tests.py", "items"),
            ("ASAN_OPTIONS=detect_leaks=0 python3 -B tests/silver_rupee/run_tests.py --sanitize", "world"),
        )
        for command, suite in required:
            with self.subTest(command=command):
                owners = [task for task in tasks if command in task["run"].splitlines()]
                self.assertEqual(len(owners), 1, f"native regression must remain required exactly once: {command}")
                self.assertEqual(owners[0]["suite"], suite)

    def test_shared_geometry_runs_with_the_windows_sdk_before_configuration(self):
        command = "python -B tests/regional_passages/run_header_portability_tests.py --msvc --compiler cl"
        for path, job in (("build.yml", "build"), ("probe-custom-tex-scroll.yml", "compile-libultraship")):
            with self.subTest(workflow=path):
                workflow = yaml.safe_load((ROOT / ".github/workflows" / path).read_text())
                steps = workflow["jobs"][job]["steps"]
                owners = [i for i, step in enumerate(steps) if command in step.get("run", "").splitlines()]
                self.assertEqual(len(owners), 1)
                configure = next(i for i, step in enumerate(steps) if step.get("name", "").startswith("Configure"))
                self.assertLess(owners[0], configure)

    def test_native_market_lifetime_repairs_are_required_checks(self):
        tasks = json.loads((ROOT / "scripts/ci/regressions.json").read_text())
        required = (
            "tests/black_market/test_room_magic_assets.py",
            "tests/black_market/run_room_magic_tests.py",
            "tests/black_market/run_room_magic_loader_tests.py",
            "tests/black_market/run_room_magic_draw_exit_tests.py",
        )
        for runner in required:
            with self.subTest(runner=runner):
                owners = [task for task in tasks if f"python3 -B {runner}" in task["run"].splitlines()]
                self.assertEqual(len(owners), 1, f"required native lifetime regression is not registered once: {runner}")
                self.assertEqual(owners[0]["suite"], "world")
        steps = yaml.safe_load((ROOT / ".github/workflows/regressions.yml").read_text())["jobs"]["regression"]["steps"]
        installs = "\n".join(step.get("run", "") for step in steps)
        self.assertIn("python3-pycparser", installs)

    def test_reset_effect_lifetime_regression_remains_required_through_its_native_entry_suite(self):
        tasks = json.loads((ROOT / "scripts/ci/regressions.json").read_text())
        command = "bash scripts/diagnostics/run_oct20_bugfix_regressions.sh"
        owners = [task for task in tasks if command in task["run"].splitlines()]
        self.assertEqual(len(owners), 1)
        self.assertEqual(owners[0]["suite"], "world")
        entry_suite = (ROOT / "scripts/diagnostics/run_oct20_bugfix_regressions.sh").read_text()
        self.assertIn("python3 -B tests/black_market/run_interior_effects_native_tests.py", entry_suite.splitlines())

    def test_platforms_and_every_test_group_finish_independently(self):
        workflow = yaml.safe_load((ROOT / ".github/workflows/build-artifacts.yml").read_text())
        checks = workflow["jobs"]["checks"]
        self.assertIs(checks["strategy"]["fail-fast"], False)
        self.assertNotIn("continue-on-error", checks)
        entries = checks["strategy"]["matrix"]["include"]
        kinds = [entry["check"] for entry in entries]
        groups = {task["suite"] for task in json.loads((ROOT / "scripts/ci/regressions.json").read_text())}
        self.assertCountEqual(kinds, ["windows", "linux", *groups])
        self.assertTrue(all(entry.get("label") for entry in entries))
        self.assertEqual(checks["uses"], "./.github/workflows/ci-check.yml")
        self.assertEqual(checks["with"]["check"], "${{ matrix.check }}")
        self.assertCountEqual(workflow["jobs"]["gate"]["needs"], ["preflight", "checks"])

    def test_component_dispatch_preserves_builds_and_single_regression_group(self):
        dispatch = yaml.safe_load((ROOT / ".github/workflows/ci-check.yml").read_text())["jobs"]
        self.assertEqual(dispatch["windows"]["uses"], "./.github/workflows/build.yml")
        self.assertEqual(dispatch["linux"]["uses"], "./.github/workflows/build-linux.yml")
        self.assertEqual(dispatch["regression"]["uses"], "./.github/workflows/regressions.yml")
        self.assertEqual(dispatch["regression"]["with"]["suite"], "${{ inputs.check }}")
        regression = yaml.safe_load((ROOT / ".github/workflows/regressions.yml").read_text())["jobs"]["regression"]
        self.assertNotIn("strategy", regression)


if __name__ == "__main__":
    unittest.main()
