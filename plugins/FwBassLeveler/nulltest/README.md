# Flatwater Bass Leveler null tests

The scripts that measure the reference, fit and train Flatwater Bass Leveler's models and null the two against each other. They
need Windows, Python 3.12 or newer with the root `requirements.txt` (`requirements-train.txt` for the learned blocks),
the reference installed, the tools built (`build/`, the native helpers by `python scripts/build_native.py`) and
`local.yaml` at the repository root; the suite-wide modules they import (the reference host, the saved state, the
corpora, the report) and the `local.yaml` keys are in [scripts/nulltest/README.md](../../../scripts/nulltest/README.md).
Run a script from this folder with `python -I <script>.py`; its docstring gives its arguments. Renders, captures and
result files stay git-ignored. The report is [NULLTESTS.md](../NULLTESTS.md), written from
`measurements/FwBassLeveler/nulltests.json` by `nulltest.py --report` and never edited by hand.

## Entry points

| File | What it does |
|---|---|
| `nulltest.py` | Flatwater Bass Leveler (its own processor, run offline by build/.../BlRender.exe) against the reference Bass Rider (rendered through pedalboard) at 48 kHz |
| `first_null.py` | The first null test of Flatwater Bass Leveler: proves the harness, the hosts and the build before any measurement |
| `bl_ref.py` | Harness of Flatwater Bass Leveler's null tests and measurements (nulltest/bl_ref.py): the reference through the suite's host |
| `nulltest_lib.py` | Generic null-test and measurement helpers for the house null-test harnesses |

## Measuring

| File | What it does |
|---|---|
| `measure_reference.py` | Phase 3 measurements of the reference (the shell VST3, the local install named by local.yaml's ref_vst3), from its audio |
| `measure_traces.py` | The reference's applied gain on musical material, for fitting Flatwater Bass Leveler's riding model alongside the probes: the bass line |

## Probes

| File | What it does |
|---|---|
| `probe_parameters.py` | Phase 1 probe of the reference (the shell VST3, the local install named by local.yaml's ref_vst3): the components the shell offers, each one's VST3 parameters (name |

## Fitting

| File | What it does |
|---|---|
| `fit_detector.py` | Fits the note detector of Flatwater Bass Leveler's riding model (RideModel.c) to the reference's probe set (render_probes.py) and its gain traces on musical material |
| `fit_hilbert.py` | Designs the quadrature allpass pair Flatwater Bass Leveler's envelopes use: two chains of second-order allpass sections in z^-2 (y = c (x + y[n-2]) - x[n-2], c = a^2) |
| `fit_lock.py` | The learned lock of Flatwater Bass Leveler's ride (RideModel.c, lock 1): when the gain stops following the law after a note's start and holds |
| `fit_polish.py` | Polishes fit_detector.py's fit one per-setting value at a time: each release depth (Response x Sensitivity), new-note threshold and attack margin |

## Renders

| File | What it does |
|---|---|
| `render_corpus.py` | The reference's riding on the bass corpus (scripts/nulltest/bass_corpus_lib.py): its applied gain on every part at a set of settings, through pedalboard |
| `render_probes.py` | The note-detector probe set for fitting Flatwater Bass Leveler's riding model: short synthetic signals (steady notes, legato dips, decays, re-attacks |

## Null tests

| File | What it does |
|---|---|
| `null_presets.py` | Flatwater Bass Leveler's factory presets against the reference's own (build/ref_presets_FwBassLeveler.json, plugins/FwBassLeveler/nulltest/read_ref_presets.py) |

## Checks and comparisons

| File | What it does |
|---|---|
| `check_port.py` | The C++ ride (Flatwater Bass Leveler's processor, run by BlRender) against the fitting model (RideModel.c with the fit in measurements/FwBassLeveler/detector_fit.json) |
| `compare_corpus.py` | Flatwater Bass Leveler's riding model (RideModel.c, the current fit) against the reference's riding on the bass corpus (render_corpus.py) |
| `compare_model.py` | Compares model (Flatwater Bass Leveler's riding model) with the reference, case by case: the reference's applied gain (its output over its input |

## Reading

| File | What it does |
|---|---|
| `read_film.py` | Reads a MeterCapture gl=1 film of the reference (or Flatwater Bass Leveler's screen grab) into per-frame readings: the audio time, the Energy meter's lit length |
| `read_ref_presets.py` | Reads the reference's presets (the factory preset file of the local install |

## Helper modules

| File | What it does |
|---|---|
| `model_lib.py` | Python side of RideModel.c (Flatwater Bass Leveler's riding model, Phase 4): the parameters, the host controls' mapping |

## Native helpers

Built by `python scripts/build_native.py FwBassLeveler` into `build/native/FwBassLeveler/`.

| File | What it does |
|---|---|
| `native/RideModel.c` | Flatwater Bass Leveler's riding model (Phase 4), for fitting against the reference: the measured stages of MEASUREMENTS.md "Flatwater Bass Leveler |

## The plug-in's scripts

In [plugins/FwBassLeveler/scripts/](../scripts/): the scripts that write under `Source/` or `resources/`
(`make_`, `export_`, `pack_`) and the editor measurements (`measure_ui.py`, `compare_ui.py`, `fit_<part>.py`).

| File | What it does |
|---|---|
| `../scripts/export_detector.py` | Writes the note detector's fitted constants (measurements/FwBassLeveler/detector_fit.json, from nulltest/fit_detector.py) into the plug-in's top DSP header |
| `../scripts/measure_ui.py` | Measures the reference's controls on its skin (extracted into the git-ignored reference/fwbassleveler/) and prints a report: per fader cap, switch, meter bar |
