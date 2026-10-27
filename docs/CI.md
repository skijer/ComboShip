# ComboShip build checks

`Build Artifacts` keeps its existing workflow name, filename and public artifact names. A short `preflight` checks conflict markers, asset collisions, the CI helpers, generated sword UI assets and formatting. It detects changed paths before any production compilation. Only Markdown under `docs/` and root `README.md`, `CONTRIBUTING.md` or `CHANGELOG.md` may skip the expensive PR jobs; unknown paths require full CI. Empty/missing change data fails preflight, and rename detection is disabled so moving a build input into docs still triggers builds.

For every change except the explicit Markdown documentation allowlist, preflight releases the existing Windows and Linux builds and six regression groups together: `world`, `entrances`, `items`, `equipment`, `systems` and `render`. All eight components share one native Actions matrix with `fail-fast: true`. The first required failure cancels the other queued or running components, including both platform builds. Each regression group retains its own checkout and the original fixed comparison commits and fixture headers. Cancellation can take a short time while a runner stops its current process.

The final job keeps the YAML ID `gate` and displays as **Final required-check status**. It runs even when dependencies fail and requires success from preflight and the complete eight-component matrix. Only a confirmed documentation-only PR may skip the expensive jobs. Missing routing output, cancellation and unexpected skips fail the gate. Its summary links to the original failed job and step, when Actions job metadata is available, and distinguishes that failure from the cancelled work. Download announcements require overall workflow success; a failed candidate cannot replace the previous verified links. `validate-seed.yml` continues to consume `comborando-windows` from a successful develop run.

## Reading a run

| Display name | Meaning |
|---|---|
| Fast source checks | Formatting, assets and CI configuration checks before compilation. |
| Build Windows / Build Linux | The actual platform builds, packaging and existing platform checks. |
| Test forest… / Test entrances… / other Test jobs | The six regression groups. A red test cancels the remaining required work. |
| Final required-check status | Reports the earlier results; it does not start another build. Open its failed-check link to see the cause. |

The first red build or test is the actionable failure. Grey cancelled jobs stopped because the run could no longer pass; they are not additional regressions. There is no need to watch each job finish after the first failure.

## Running and extending regressions

The command inventory lives in `scripts/ci/regressions.json`. Add a test there with `suite`, `name`, `origin` and `run`, plus an `env` mapping if needed. Keep related compile/run commands in the same task. Set `origin` to `nei` for the original umbrella's strict shell and pinned fixture-header scope, or `gate` for the other checks. Keep sanitizer and platform variants distinct.

```sh
python3 -B scripts/ci/run_regressions.py --suite world --list
python3 -B scripts/ci/run_regressions.py --suite world
bash scripts/diagnostics/run_combo_nei_regressions.sh
python3 -B -m unittest discover -s tests/ci -v
```

The legacy NEI entrypoint selects its original commands in their original order. The workflow runs the same inventory by group instead. Every task gets a log group and elapsed-time row in the job summary. Balance groups using those measurements; the first split is provisional until hosted timings are available. A failed task immediately emits an error annotation with the task name, command and exit status, then stops its group and returns that status.

## Caches and preserved build behavior

Windows retains MSVC and the Visual Studio 2022 generator. Its dependency cache is separate from the build tree; build-tree keys rotate per commit and restore from matching build inputs. This refreshes the former immutable whole-tree snapshot without changing generators. It is not a content-addressed MSVC compiler cache; further compiler-cache work requires separate measurement.

Linux retains Ninja, GCC, ccache and the four-worker memory limit. In-repository PRs may save their scoped compiler cache; fork PRs remain read-only. Successful develop runs seed the shared cache. A required failure takes priority over finishing cache work. Neither cache substitutes for executing the build and tests.

Packaging, generated game archives, Linux seed-generation smoke tests, Windows validator production and release workflow behavior remain unchanged. Successful compilation and CI do not establish configured gameplay acceptance.

## Baseline and recovery

The restructure starts from `6effafdeaa41d0e851395c19665f11f2b24f3145`. Before publication, the extracted inventory was compared against every original outer regression step and all 167 original NEI shell commands, including their order, flags and explicit environments. All original commands are retained, including existing overlaps, to isolate the scheduling change.

Revert the candidate CI commit to recover the previous scheduling. No production engine source or asset payload is changed.

## Publisher rollout

GitHub executes `workflow_run` from the default branch. Deploy the success-only condition in `pr-artifacts.yml` to `develop` before opening a PR that enables parallel artifact production. The candidate copy of that condition alone cannot protect its first PR run. This guard-only deployment does not alter builds or game code.
