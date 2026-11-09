# Flatwater Pitch Bend null tests

The scripts that measure Vocal Bender, fit and train Flatwater Pitch Bend's models and null the two against each other. They need
Windows, Python 3.12 or newer with the root `requirements.txt` (`requirements-train.txt` for the learned blocks), the
original installed, the tools built (`build/`) and `local.yaml` at the repository root; the suite-wide modules they
import (hosting, state, corpora, meters) and the `local.yaml` keys are in
[scripts/nulltest/README.md](../../../scripts/nulltest/README.md). Run a script from this folder with
`python -I <script>.py`; its docstring gives its arguments. Renders, captures and result files stay git-ignored; the
results are published in [NULLTESTS.md](../NULLTESTS.md).

## Entry points

| File | Role |
|---|---|
| `nulltest.py` | every case: `null_mods.py`, then `null_presets.py` |
| `first_null.py` | proves the hosts and the build before any DSP (PASS or FAIL per check) |
| `vb.py` | the harness: renders the reference (RefRender), the processor and its VST3 |

## Hosting

| Script | What it does |
|---|---|
| `editor_state.py` | writes the Vocal Bender settings only its editor sets (modulation slots, Fine, the modulators) into its state |

## Reference presets

| Script | What it does |
|---|---|
| `read_ref_presets.py` | Vocal Bender factory presets in Flatwater Pitch Bend's terms; drawn shapes are only named, never stored |

## Measuring

| Script | What it does |
|---|---|
| `measure_mods.py` | Vocal Bender's modulators measured through its Mix (an exact crossfade) |
| `measure_shifter.py` | Vocal Bender's pitch and formant shifter measured as behaviour, not as a null |
| `measure_control.py` | what the shifter keys on: the knobs' own values or the modulated totals |
| `measure_level_flatten.py` | the shifter's voiced level under Flatten |
| `measure_level_grid.py` | the voiced level per material at fixed settings, the reference against ours |
| `measure_modpitch.py` | a modulated Pitch, the reference against ours: when a stepped Pitch lands |

## Null tests

| Script | What it does |
|---|---|
| `null_mods.py` | Flatwater Pitch Bend's modulators against Vocal Bender's, through Mix |
| `null_presets.py` | Flatwater Pitch Bend's 40 factory presets against Vocal Bender's own |
| `score_shifter.py` | the shifter's scorecard (M1 to M8) on our shifter alone |
