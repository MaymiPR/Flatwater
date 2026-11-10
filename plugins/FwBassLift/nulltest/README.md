# Flatwater Bass Lift null tests

The scripts that measure the reference, fit and train Flatwater Bass Lift's models and null the two against each other. They
need Windows, Python 3.12 or newer with the root `requirements.txt` (`requirements-train.txt` for the learned blocks),
the reference installed, the tools built (`build/`, the native helpers by `python scripts/build_native.py`) and
`local.yaml` at the repository root; the suite-wide modules they import (the reference host, the saved state, the
corpora, the report) and the `local.yaml` keys are in [scripts/nulltest/README.md](../../../scripts/nulltest/README.md).
Run a script from this folder with `python -I <script>.py`; its docstring gives its arguments. Renders, captures and
result files stay git-ignored. The report is [NULLTESTS.md](../NULLTESTS.md), written from
`measurements/FwBassLift/nulltests.json` by `nulltest.py --report` and never edited by hand.

## Entry points

| File | What it does |
|---|---|
| `nulltest.py` | Null tests of Flatwater Bass Lift: the null scripts in order, the numbers NULLTESTS.md publish |
| `first_null.py` | The first null test of Flatwater Bass Lift: proves the harness, the hosts and the build before any measurement |
| `bf_ref.py` | Flatwater Bass Lift (offline renderer of the plug-in DSP) against the reference's render, residual in dB |
| `nulltest_lib.py` | Generic null-test and measurement helpers for the house null-test harnesses |

## Measuring

| File | What it does |
|---|---|
| `measure_generator.py` | The harmonic generator |
| `measure_kernel.py` | Second-order kernel of the generator from two-tone tests |
| `measure_sweep.py` | Harmonic responses vs fundamental frequency at fixed levels, to place the filters before or after the nonlinearity |

## Fitting

| File | What it does |
|---|---|
| `fit_combo.py` | Combined fit of the two-detector gain model: time-domain renders (4 Freq settings + transients |
| `fit_gain_trace.py` | Recovers the time-varying gain of the reference's generated path by least squares |
| `fit_lstage.py` | Fit of the l-driven gain stage alone on the pinned carrier set (pinned.json, wide detector pinned by a strong tone), the wide stage held at build/combo.json |
| `fit_twodet.py` | Fit of the two-detector gain model (model.generated7 with the l-driven stage) on steady harmonics: the detector-pinning set (pin.json) |

## Training

| File | What it does |
|---|---|
| `train_net.py` | Learned gain control for Flatwater Bass Lift (gray-box): the identified structure of the generated path stays fixed and exact, only the level-to-gain block is learned |

## Renders

| File | What it does |
|---|---|
| `render_fitdata.py` | Renders the reference's fitting set: the generated path alone, split off with two Intensity settings (Intensity is an exact gain on the generated path), for tones |
| `render_pin.py` | Low carrier tone plus a 'pinning' tone that the wide level detector sees but the l band does not (Decay 0) |
| `render_pinned.py` | Low carriers with the wide level detector pinned by a strong tone (-3 dBFS, an odd multiple of f0/2 near 200 Hz: no collisions with the carrier's harmonics), Decay 0 |
| `render_trainset.py` | Training / validation renders for the learned Flatwater Bass Lift gain block (plugins/FwBassLift/nulltest/train_net.py) |
| `render_transients.py` | Renders transient material (the null test's kick-like and bass-notes signals, plus a fast bass riff) through the reference at several Freq settings |

## Helper modules

| File | What it does |
|---|---|
| `model_lib.py` | Python model of the reference's generated path (Phase 4 identification), vectorised with scipy filters |

## Native helpers

Built by `python scripts/build_native.py FwBassLift` into `build/native/FwBassLift/`.

| File | What it does |
|---|---|
| `native/Follower.c` | Per-sample helpers for the Flatwater Bass Lift identification scripts (loaded with ctypes by model_lib.py) |

## The plug-in's scripts

In [plugins/FwBassLift/scripts/](../scripts/): the scripts that write under `Source/` or `resources/`
(`make_`, `export_`, `pack_`) and the editor measurements (`measure_ui.py`, `compare_ui.py`, `fit_<part>.py`).

| File | What it does |
|---|---|
| `../scripts/export_net.py` | Writes the trained Flatwater Bass Lift gain network (build/gainnet/gainnet_h<H>.pt |
