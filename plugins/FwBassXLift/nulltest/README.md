# Flatwater BassX Lift null tests

The scripts that measure the reference, fit and train Flatwater BassX Lift's models and null the two against each other. They
need Windows, Python 3.12 or newer with the root `requirements.txt` (`requirements-train.txt` for the learned blocks),
the reference installed, the tools built (`build/`, the native helpers by `python scripts/build_native.py`) and
`local.yaml` at the repository root; the suite-wide modules they import (the reference host, the saved state, the
corpora, the report) and the `local.yaml` keys are in [scripts/nulltest/README.md](../../../scripts/nulltest/README.md).
Run a script from this folder with `python -I <script>.py`; its docstring gives its arguments. Renders, captures and
result files stay git-ignored. The report is [NULLTESTS.md](../NULLTESTS.md), written from
`measurements/FwBassXLift/nulltests.json` by `nulltest.py --report` and never edited by hand.

## Entry points

| File | What it does |
|---|---|
| `nulltest.py` | Flatwater BassX Lift (offline renderer of the plug-in DSP) against the reference's render, residual in dB |
| `first_null.py` | The first null test of Flatwater BassX Lift: proves the harness, the hosts and the build before any measurement |
| `bx_ref.py` | Harness of Flatwater BassX Lift's null tests and measurements (nulltest/bx_ref.py): the reference through the suite's host |
| `nulltest_lib.py` | Generic null-test and measurement helpers for the house null-test harnesses |

## Measuring

| File | What it does |
|---|---|
| `measure_kernel.py` | The second-order kernel of G in the flat-gain region (Ratio 2, -50 dBFS per tone, gain at its cap), from two-tone tests: K2(f1, f2) at f1 + f2 and f1 - f2 |
| `measure_linear.py` | The small-signal transfer of the generated part G (the reference monitor) |

## Probes

| File | What it does |
|---|---|
| `probe_recovery.py` | How the level control recovers in silence and attacks at an onset |

## Fitting

| File | What it does |
|---|---|
| `fit_gain_trace.py` | The level-control gain g(t) by Gauss-Newton with the generator's exact Jacobian |

## Training

| File | What it does |
|---|---|
| `train_detector.py` | Flatwater BassX Lift level control, "det" model: an explicit detector with a learned gain law, trained end-to-end on the reference's generated part |
| `train_net.py` | Learned level control for Flatwater BassX Lift (gray-box): the identified structure of the generated part stays fixed and exact |

## Renders

| File | What it does |
|---|---|
| `render_trainset_extract.py` | Training set for the Flatwater BassX Lift learned gain: reference renders of the generated part G |

## Null tests

| File | What it does |
|---|---|
| `null_grid.py` | Null grid for Flatwater BassX Lift: random settings over every control (seeded), a few signals each, residual in dB against the reference |
| `null_oracle_grid.py` | Oracle check on grid cases: the generated part re-rendered through the identified structure with the reference's own gain (extracted, constrained) |

## Reading

| File | What it does |
|---|---|
| `read_ref_presets.py` | Reads the reference's presets (the factory preset file of the local install, named by local.yaml's FwBassXLift.ref_presets) and writes their settings |

## Helper modules

| File | What it does |
|---|---|
| `gain_trace_lib.py` | The level-control gain g(t) by forward least squares (no filter inversion) |
| `model_lib.py` | Flatwater BassX Lift model pieces identified on the reference (Python, float64), shared by the identification scripts |

## Native helpers

Built by `python scripts/build_native.py FwBassXLift` into `build/native/FwBassXLift/`.

| File | What it does |
|---|---|
| `native/Features.c` | Feature helpers for the Flatwater BassX Lift level-control training (loaded with ctypes by train_net.py |
| `native/Generator.c` | Per-sample helpers for the Flatwater BassX Lift identification scripts (loaded with ctypes by model_lib.py) |

## The plug-in's scripts

In [plugins/FwBassXLift/scripts/](../scripts/): the scripts that write under `Source/` or `resources/`
(`make_`, `export_`, `pack_`) and the editor measurements (`measure_ui.py`, `compare_ui.py`, `fit_<part>.py`).

| File | What it does |
|---|---|
| `../scripts/export_net.py` | Writes plugins/FwBassXLift/Source/dsp/Net.h: the trained level-control GRU |
