# Flatwater Pitch Bend null tests

The scripts that measure the reference, fit and train Flatwater Pitch Bend's models and null the two against each other. They
need Windows, Python 3.12 or newer with the root `requirements.txt` (`requirements-train.txt` for the learned blocks),
the reference installed, the tools built (`build/`, the native helpers by `python scripts/build_native.py`) and
`local.yaml` at the repository root; the suite-wide modules they import (the reference host, the saved state, the
corpora, the report) and the `local.yaml` keys are in [scripts/nulltest/README.md](../../../scripts/nulltest/README.md).
Run a script from this folder with `python -I <script>.py`; its docstring gives its arguments. Renders, captures and
result files stay git-ignored. The report is [NULLTESTS.md](../NULLTESTS.md), written from
`measurements/FwPitchBend/nulltests.json` by `nulltest.py --report` and never edited by hand.

## Entry points

| File | What it does |
|---|---|
| `nulltest.py` | Null tests of Flatwater Pitch Bend: the null scripts in order, the numbers NULLTESTS.md publish |
| `first_null.py` | The first null test of Flatwater Pitch Bend: proves the hosts and the build before any DSP is written (the house workflow's Phase 0 gate) |
| `pb_ref.py` | Renders for Flatwater Pitch Bend's null tests and measurements: the reference (the reference, the reference sub-plugin, through tools/RefRender |
| `nulltest_lib.py` | Generic null-test and measurement helpers for the house null-test harnesses |

## Measuring

| File | What it does |
|---|---|
| `measure_control.py` | What the reference's shifter keys on the controls' own values (the Pitch and Formant knobs) and what on the modulated totals |
| `measure_level_flatten.py` | The shifter's voiced level where the level battery (measure_shifter.py level) did not reach: under Flatten (the output forced onto one note |
| `measure_level_grid.py` | The reference's voiced level per material at fixed settings, and our shifter against it on the factory presets' two takes |
| `measure_modpitch.py` | A modulated Pitch, the reference against ours (MEASUREMENTS.md, Phase 3) |
| `measure_mods.py` | The reference's modulators measured through its Mix (MEASUREMENTS.md, Flatwater Pitch Bend) |
| `measure_shifter.py` | The reference's pitch / formant shifter measured as behaviour, not as a null |

## Null tests

| File | What it does |
|---|---|
| `null_mods.py` | Our modulators against the reference's, through Mix (MEASUREMENTS.md, Flatwater Pitch Bend) |
| `null_presets.py` | Flatwater Pitch Bend's 40 factory presets against the reference's own: the reference with its preset loaded into its state (the stored values, hidden ones included |

## Checks and comparisons

| File | What it does |
|---|---|
| `check_shifter.py` | The shifter's scorecard (measurements/FwPitchBend/shifter/SUMMARY.md, M1  |

## Reading

| File | What it does |
|---|---|
| `read_ref_presets.py` | The reference's factory presets read from their files (the installed preset folder), in our terms: the controls, the modulators' settings, the modulation slots |

## Helper modules

| File | What it does |
|---|---|
| `editor_state_lib.py` | The reference's settings that only its editor sets (the modulation slots, Fine, cents values, the modulators' own settings), written into its saved state |

## The plug-in's scripts

In [plugins/FwPitchBend/scripts/](../scripts/): the scripts that write under `Source/` or `resources/`
(`make_`, `export_`, `pack_`) and the editor measurements (`measure_ui.py`, `compare_ui.py`, `fit_<part>.py`).

| File | What it does |
|---|---|
| `../scripts/make_presets.py` | Writes plugins/FwPitchBend/Source/Presets.h: Flatwater Pitch Bend's factory presets, the reference's 40 presets' values |
