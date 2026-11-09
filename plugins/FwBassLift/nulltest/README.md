# Flatwater Bass Lift null tests

The scripts that measure Renaissance Bass, fit and train Flatwater Bass Lift's models and null the two against each other. They need
Windows, Python 3.12 or newer with the root `requirements.txt` (`requirements-train.txt` for the learned blocks), the
original installed, the tools built (`build/`) and `local.yaml` at the repository root; the suite-wide modules they
import (hosting, state, corpora, meters) and the `local.yaml` keys are in
[scripts/nulltest/README.md](../../../scripts/nulltest/README.md). Run a script from this folder with
`python -I <script>.py`; its docstring gives its arguments. Renders, captures and result files stay git-ignored; the
results are published in [NULLTESTS.md](../NULLTESTS.md).

## Entry points

| File | Role |
|---|---|
| `nulltest.py` | every case: runs `null_fwbasslift.py` with the arguments given |
| `null_fwbasslift.py` | the null test against the reference (`--quick`, `--out report.md`, renderer keys); imported by Flatwater BassX Lift's scripts |

## Corpora and probe signals

| Script | What it does |
|---|---|
| `render_pin.py` | Flatwater Bass Lift identification: a low carrier plus a pinning tone the wide detector sees and the l band does not |
| `render_pinned.py` | low carriers with the wide detector pinned by a strong tone |
| `render_transients.py` | transient material through Renaissance Bass at several Freq settings, the generated part split off |
| `render_fitdata.py` | the Renaissance Bass fitting set: the generated path alone, split off with two Intensity settings |

## Measuring

| Script | What it does |
|---|---|
| `measure_generator.py` | Renaissance Bass: the harmonic generator |
| `measure_k2.py` | Renaissance Bass: the generator's second-order kernel from two-tone tests |
| `measure_sweep.py` | Renaissance Bass: harmonic responses against the fundamental, placing the filters around the nonlinearity |
| `extract_cs.py` | Renaissance Bass: the generated path's time-varying gain by least squares |

## Fitting

| Script | What it does |
|---|---|
| `fit_twodet.py` | Renaissance Bass: the two-detector gain model on steady harmonics |
| `fit_multif2.py` | Renaissance Bass: a joint time-domain fit over four Freq settings |
| `fit_combo.py` | Renaissance Bass: the combined time-domain and steady-state fit |
| `fit_lstage.py` | Renaissance Bass: the l-driven gain stage alone on the pinned set |

## Models

| Script | What it does |
|---|---|
| `model.py` | Python model of Renaissance Bass's generated path |

## Training

| Script | What it does |
|---|---|
| `render_trainset.py` | training and validation renders for Flatwater Bass Lift's learned gain block |
| `train_net.py` | Flatwater Bass Lift's learned level-to-gain block (gray-box) |

## C models and their builds

Each `.c` file is a per-sample model loaded with ctypes; its `build_<name>.bat` compiles it into a git-ignored DLL
with the newest Visual Studio found by vswhere.

| File | What it does |
|---|---|
| `fastdsp.c`, `build_fastdsp.bat` | attack and release follower for `model.py` (a Python fallback exists) |
