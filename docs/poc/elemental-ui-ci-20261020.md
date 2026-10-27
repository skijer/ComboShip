# Elemental sword UI rasterizer CI correction

Baseline: PR #48 head `3a2c80f377fd0b65bf1046288265c97fb46b4a64`, tree
`c8742a992427bb0087f4869e4d16d3909e2b4ecf`. Failed Build Artifacts run:
`39523087839`, gate job `118971336662`.

The HUD fixture's asset check regenerated Montserrat masks with Ubuntu's
Pillow 10.2.0 / FreeType 2.13.2 and rejected the committed type header. The
same bundled font and generator reproduce every committed output exactly
with the Pillow 12.3.0 binary wheel / FreeType 2.14.3.

The gate now installs the pinned binary Pillow wheel in a system-Python
virtual environment. System packages remain visible for the apt-provided
NumPy dependency. Its interpreter is added to `GITHUB_PATH` before subsequent
steps, including nested shell/Python fixtures. The asset check also runs
immediately after dependency setup, before lengthy native regression groups.
The later HUD and elemental checks remain enabled.

Verification:

- Reproduced the original type-header assertion with the distro stack.
- A fresh system-Python environment with the pinned wheel passes the full
  generator `--check`: art header, type header and manifest are byte-identical.
- All 16 commands in `run_elemental_sword_regressions.sh` pass, including the
  native HUD/chooser, aura/combat/finish, ice-arrow resource ownership and
  receipt/decoder cases, with ASan/UBSan where configured. Local checks were
  resumed after supplying public C++ dependency headers; fixtures are unchanged.
- Existing icon/Mario asset verification and native seasonal leaf loading pass.
- Workflow YAML parsing, new shell-block syntax and `git diff --check` pass.
- Independent read-only review found no actionable issues.

To recreate the generation environment on Ubuntu 24.04, install
`python3-venv` and `python3-numpy`, create a venv with
`python3 -m venv --system-site-packages /tmp/combo-ui-python`, and run its
Python with `-m pip install --only-binary=:all: -r
scripts/assets/sword-element-ui-requirements.txt`. With that venv's `bin`
directory on `PATH`, run `python3 -B scripts/assets/build_sword_element_ui.py
--check` and `bash scripts/diagnostics/run_elemental_sword_regressions.sh`.

Recovery: retain the baseline commit above; revert this bounded CI correction
to restore its workflow and remove the new dependency pin. Game source,
generated assets, model archives and the complete cumulative integration are
preserved. Hosted platform builds and configured runtime acceptance retain
their separate pending status; this change does not promote a gameplay master.
