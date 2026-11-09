# Flatwater Vocal Leveler null tests

The scripts that measure Vocal Rider, fit and train Flatwater Vocal Leveler's models and null the two against each other. They need
Windows, Python 3.12 or newer with the root `requirements.txt` (`requirements-train.txt` for the learned blocks), the
original installed, the tools built (`build/`) and `local.yaml` at the repository root; the suite-wide modules they
import (hosting, state, corpora, meters) and the `local.yaml` keys are in
[scripts/nulltest/README.md](../../../scripts/nulltest/README.md). Run a script from this folder with
`python -I <script>.py`; its docstring gives its arguments. Renders, captures and result files stay git-ignored; the
results are published in [NULLTESTS.md](../NULLTESTS.md).

## Entry points

| File | Role |
|---|---|
| `nulltest.py` | every case: the processor through `VlRender` against the reference |

## Reference presets

| Script | What it does |
|---|---|
| `read_ref_presets.py` | Vocal Rider factory presets |

## Measuring

| Script | What it does |
|---|---|
| `measure_reference.py` | Vocal Rider: the applied gain sample by sample, per section of MEASUREMENTS.md |
| `probe_parameters.py` | Vocal Rider's components and VST3 parameters |
| `render_reference.py` | Vocal Rider renders of the vocal corpus and a probe set, for fitting the riding |

## Fitting

| Script | What it does |
|---|---|
| `fit_model.py` | Vocal Rider: the riding model's continuous constants by CMA-ES |

## Models

| Script | What it does |
|---|---|
| `model.py` | the ctypes front end of `model.c` |
| `compare_model.py` | the riding model against the Vocal Rider renders |

## Null tests

| Script | What it does |
|---|---|
| `null_presets.py` | Flatwater Vocal Leveler's factory presets against Vocal Rider's own |

## Validation

| Script | What it does |
|---|---|
| `check_vst3.py` | Flatwater Vocal Leveler: the VST3 against the renderer, stereo and automation |
| `check_port.py` | the C++ ride against `model.c` |

## C models and their builds

Each `.c` file is a per-sample model loaded with ctypes; its `build_<name>.bat` compiles it into a git-ignored DLL
with the newest Visual Studio found by vswhere.

| File | What it does |
|---|---|
| `model.c`, `build_model.bat` | Flatwater Vocal Leveler's riding model, the same as the plug-in's |
