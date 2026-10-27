# What's New color-only implementation plan

> **For agentic workers:** Use superpowers:executing-plans for inline execution.

**Goal:** Complete release highlight coverage without altering native layouts.
**Architecture:** Keep the exact-key inventory and RAII scope; remove marker
items. Apply that same scope to direct MM randomizer widgets. Insert existing
OoT Seasons widgets after Shuffle Masks during registration.
**Tech stack:** C++, native menu registrations, Dear ImGui, existing Python probes.
**Spec:** `docs/poc/whats-new-color-only-20261024.md` and the user's explicit request.

## Global constraints

Preserve native labels, IDs, CVars, defaults, callbacks and disabled behavior.
No gameplay or asset changes. Keep develop/master and other candidates intact.

## Review focus

Disabled controls keep input gates; nested scopes restore colors; raw MM controls
receive the preference; OoT Solo/Combo retain their existing CVar ownership;
registration insertion preserves callback objects and pre-export ordering.

### Task 1: Color-only scope and missing coverage

- [x] Update existing real ImGui fixtures to require identical on/off rectangles
  and zero marker geometry. Observe failure against the baseline helper.
- [x] Remove marker submissions and marker-specific help; add exact missing
  feature/shuffle keys and scope the direct MM item-pool/hint controls.
- [x] Run `IMGUI_DIR=<pinned imgui> python3 -B tests/whats_new/run_tests.py`.

### Task 2: OoT ordering and recovery

- [x] Insert the existing Seasons registrations immediately after Shuffle Masks
  with Starting Rod following the mode; preserve their options and callbacks.
- [x] Exercise native insertion with earlier/later/missing anchors and exact
  row order. Inspect the native registration source and both CVar branches.
- [x] Review the whole diff, run relevant checks, commit only owned paths,
  and save/restore-check the source candidate and its integration patch.
