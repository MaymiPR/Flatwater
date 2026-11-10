# Flatwater Vocal Leveler null tests

The scripts that measure the reference, fit and train Flatwater Vocal Leveler's models and null the two against each other. They
need Windows, Python 3.12 or newer with the root `requirements.txt` (`requirements-train.txt` for the learned blocks),
the reference installed, the tools built (`build/`, the native helpers by `python scripts/build_native.py`) and
`local.yaml` at the repository root; the suite-wide modules they import (the reference host, the saved state, the
corpora, the report) and the `local.yaml` keys are in [scripts/nulltest/README.md](../../../scripts/nulltest/README.md).
Run a script from this folder with `python -I <script>.py`; its docstring gives its arguments. Renders, captures and
result files stay git-ignored. The report is [NULLTESTS.md](../NULLTESTS.md), written from
`measurements/FwVocalLeveler/nulltests.json` by `nulltest.py --report` and never edited by hand.

## Entry points

| File | What it does |
|---|---|
| `nulltest.py` | Flatwater Vocal Leveler (its own processor, run offline by build/.../VlRender.exe) against the reference Vocal Rider (rendered through pedalboard |
| `first_null.py` | The first null test of Flatwater Vocal Leveler: proves the harness, the hosts and the build before any measurement |
| `vl_ref.py` | Harness of Flatwater Vocal Leveler's null tests and measurements (nulltest/vl_ref.py): the reference through the suite's host |
| `nulltest_lib.py` | Generic null-test and measurement helpers for the house null-test harnesses |

## Measuring

| File | What it does |
|---|---|
| `measure_reference.py` | Phase 3 measurements of the reference (the shell VST3, the local install named by local.yaml's ref_vst3), from its audio: no latency |

## Probes

| File | What it does |
|---|---|
| `probe_parameters.py` | Phase 1 probe of the reference (the shell VST3, the local install named by local.yaml's ref_vst3): the components the shell offers, each one's VST3 parameters (name |

## Fitting

| File | What it does |
|---|---|
| `fit_model.py` | Checks Flatwater Vocal Leveler's riding model (RideModel.c) against the reference renders (render_reference.py) by a local fit: CMA-ES over the continuous constants |

## Renders

| File | What it does |
|---|---|
| `render_reference.py` | Reference renders for fitting Flatwater Vocal Leveler's riding (Phase 4): the vocal corpus (scripts/nulltest/vocal_corpus_lib.py) and a set of probes (steps, gaps |

## Null tests

| File | What it does |
|---|---|
| `null_presets.py` | Flatwater Vocal Leveler's factory presets against the reference's own (build/ref_presets_FwVocalLeveler.json |

## Checks and comparisons

| File | What it does |
|---|---|
| `check_port.py` | The C++ ride (Flatwater Vocal Leveler's processor, run by VlRender) against the fitting model (RideModel.c with the values in plugins/FwVocalLeveler/Source/dsp/Fit.h |
| `compare_model.py` | Compares the riding model (model) with the reference renders (vr_ref): per case the null (output difference against the reference's output, dB) and the gain error |

## Reading

| File | What it does |
|---|---|
| `read_ref_presets.py` | Reads the reference's presets (the factory preset file of the local install |

## Helper modules

| File | What it does |
|---|---|
| `model_lib.py` | Ctypes front end of RideModel.c (Flatwater Vocal Leveler's riding model): ride(x, settings, params) -> gain in dB per sample |

## Native helpers

Built by `python scripts/build_native.py FwVocalLeveler` into `build/native/FwVocalLeveler/`.

| File | What it does |
|---|---|
| `native/RideModel.c` | Flatwater Vocal Leveler's riding model (Phase 4), driven by model_lib.py |

## The plug-in's scripts

In [plugins/FwVocalLeveler/scripts/](../scripts/): the scripts that write under `Source/` or `resources/`
(`make_`, `export_`, `pack_`) and the editor measurements (`measure_ui.py`, `compare_ui.py`, `fit_<part>.py`).

| File | What it does |
|---|---|
| `../scripts/export_fit.py` | Writes plugins/FwVocalLeveler/Source/dsp/Fit.h: the riding model's constants, the reference's hidden values (plugins/FwVocalLeveler/nulltest/model_lib.py DEFAULTS) |
| `../scripts/measure_ui.py` | Measures the reference's editor on its skin (extracted into the git-ignored reference/fwvocalleveler/skin/) and prints a report: the panels' shading |
