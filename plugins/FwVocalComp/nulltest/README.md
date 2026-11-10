# Flatwater Vocal Comp null tests

The scripts that measure the reference, fit and train Flatwater Vocal Comp's models and null the two against each other. They
need Windows, Python 3.12 or newer with the root `requirements.txt` (`requirements-train.txt` for the learned blocks),
the reference installed, the tools built (`build/`, the native helpers by `python scripts/build_native.py`) and
`local.yaml` at the repository root; the suite-wide modules they import (the reference host, the saved state, the
corpora, the report) and the `local.yaml` keys are in [scripts/nulltest/README.md](../../../scripts/nulltest/README.md).
Run a script from this folder with `python -I <script>.py`; its docstring gives its arguments. Renders, captures and
result files stay git-ignored. The report is [NULLTESTS.md](../NULLTESTS.md), written from
`measurements/FwVocalComp/nulltests.json` by `nulltest.py --report` and never edited by hand.

## Entry points

| File | What it does |
|---|---|
| `nulltest.py` | Null tests of Flatwater Vocal Comp: the null scripts in order, the numbers NULLTESTS.md publish |
| `first_null.py` | The first null test of Flatwater Vocal Comp: proves the harness, the hosts and the build before any measurement |
| `vc_ref.py` | Flatwater Vocal Comp (offline renderer of the plug-in DSP) against the reference's render, residual in dB |
| `nulltest_lib.py` | Generic null-test and measurement helpers for the house null-test harnesses |

## Probes

| File | What it does |
|---|---|
| `probe_gate.py` | Gate gain trajectory, sample-accurate on a DC carrier (private) |
| `probe_kernel.py` | Measure the reference's fast peak reading for isolated short patterns (private) |
| `probe_ramp.py` | Sample-accurate gain around events, on a DC carrier (private) |
| `probe_slow_dc.py` | Compressor slow path on a DC carrier: level steps of various lengths (private) |

## Fitting

| File | What it does |
|---|---|
| `fit_gain_hold.py` | Fits the gain hold on a DC carrier: the instant gain held as a minimum over H samples, then two box smoothers, in the linear or the dB domain |
| `fit_meter_energy.py` | Fits the reference Energy meter (input level) to a tools/MeterCapture recording: frame peak of the input, then one or two one-pole stages on linear amplitude |
| `fit_meter_output.py` | Tries rules for the reference output meter against a tools/MeterCapture recording (frames + output audio) |
| `fit_smoothing.py` | Tune the gain-smoothing constants of Flatwater Vocal Comp against the reference (private |

## Training

| File | What it does |
|---|---|
| `train_net.py` | Learned slow path for Flatwater Vocal Comp (gray-box): the identified detector, fast path (look-ahead) and static law stay exact |

## Renders

| File | What it does |
|---|---|
| `render_trainset.py` | Training / validation renders for the learned Flatwater Vocal Comp slow path (plugins/FwVocalComp/nulltest/train_net.py) |

## Checks and comparisons

| File | What it does |
|---|---|
| `check_net.py` | Scores a trained Flatwater Vocal Comp slow-path network (plugins/FwVocalComp/nulltest/train_net.py) on the standard null-test signals |

## The plug-in's scripts

In [plugins/FwVocalComp/scripts/](../scripts/): the scripts that write under `Source/` or `resources/`
(`make_`, `export_`, `pack_`) and the editor measurements (`measure_ui.py`, `compare_ui.py`, `fit_<part>.py`).

| File | What it does |
|---|---|
| `../scripts/compare_ui.py` | Compares a FwVocalCompSnapshot PNG with the reference skin composed from the local the reference install (extracted into reference/, which is git-ignored) |
| `../scripts/export_net.py` | Writes the trained Flatwater Vocal Comp slow-path network (build/fwvocalcompnet/<name>.pt |
