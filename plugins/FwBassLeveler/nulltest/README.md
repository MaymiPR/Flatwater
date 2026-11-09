# Flatwater Bass Leveler null tests

The scripts that measure Bass Rider, fit and train Flatwater Bass Leveler's models and null the two against each other. They need
Windows, Python 3.12 or newer with the root `requirements.txt` (`requirements-train.txt` for the learned blocks), the
original installed, the tools built (`build/`) and `local.yaml` at the repository root; the suite-wide modules they
import (hosting, state, corpora, meters) and the `local.yaml` keys are in
[scripts/nulltest/README.md](../../../scripts/nulltest/README.md). Run a script from this folder with
`python -I <script>.py`; its docstring gives its arguments. Renders, captures and result files stay git-ignored; the
results are published in [NULLTESTS.md](../NULLTESTS.md).

## Entry points

| File | Role |
|---|---|
| `nulltest.py` | every case: the processor through `BlRender` against the reference |

## Reference presets

| Script | What it does |
|---|---|
| `read_ref_presets.py` | Bass Rider factory presets |

## Corpora and probe signals

| Script | What it does |
|---|---|
| `make_probes.py` | the Bass Rider note-detector probe set: steady notes, legato dips, decays, re-attacks, gaps |

## Measuring

| Script | What it does |
|---|---|
| `measure_reference.py` | Bass Rider: the applied gain as output over input, per section of MEASUREMENTS.md |
| `probe_parameters.py` | Bass Rider's components and VST3 parameters (names, defaults, value texts) |
| `measure_traces.py` | Bass Rider's applied gain on musical material, beside the probes |
| `render_corpus.py` | Bass Rider's riding on the bass corpus at a set of settings |

## Fitting

| Script | What it does |
|---|---|
| `fit_detector.py` | Bass Rider: the note detector of the riding model by CMA-ES on probes and traces |
| `fit_polish.py` | Bass Rider: per-setting values polished one at a time on a grid |
| `fit_lock.py` | Bass Rider: the learned lock of the ride after a note's start |
| `export_detector.py` | writes the fitted detector constants into `plugins/FwBassLeveler/Source/dsp/Ride.h` |
| `design_hilbert.py` | the quadrature allpass pair of Flatwater Bass Leveler's envelopes |

## Models

| Script | What it does |
|---|---|
| `model.py` | the Python side of `model.c`: parameters, control mapping, the ride in dB |
| `compare_model.py` | the riding model against Bass Rider, case by case |
| `compare_corpus.py` | the riding model against Bass Rider on the bass corpus |

## Null tests

| Script | What it does |
|---|---|
| `null_presets.py` | Flatwater Bass Leveler's factory presets against Bass Rider's own |

## Validation

| Script | What it does |
|---|---|
| `check_vst3.py` | Flatwater Bass Leveler: the VST3 against the renderer, and the release checks |
| `check_port.py` | the C++ ride against the fitted model `model.c` |

## Editor, meters and control feel

| Script | What it does |
|---|---|
| `read_film.py` | a film of Bass Rider or Flatwater Bass Leveler as per-frame readings |

## C models and their builds

Each `.c` file is a per-sample model loaded with ctypes; its `build_<name>.bat` compiles it into a git-ignored DLL
with the newest Visual Studio found by vswhere.

| File | What it does |
|---|---|
| `model.c`, `build_model.bat` | Flatwater Bass Leveler's riding model with every constant a parameter |
