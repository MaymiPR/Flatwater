# Flatwater Vocal Comp null tests

The scripts that measure Renaissance Vox, fit and train Flatwater Vocal Comp's models and null the two against each other. They need
Windows, Python 3.12 or newer with the root `requirements.txt` (`requirements-train.txt` for the learned blocks), the
original installed, the tools built (`build/`) and `local.yaml` at the repository root; the suite-wide modules they
import (hosting, state, corpora, meters) and the `local.yaml` keys are in
[scripts/nulltest/README.md](../../../scripts/nulltest/README.md). Run a script from this folder with
`python -I <script>.py`; its docstring gives its arguments. Renders, captures and result files stay git-ignored; the
results are published in [NULLTESTS.md](../NULLTESTS.md).

## Entry points

| File | Role |
|---|---|
| `nulltest.py` | every case: runs `null_fwvocalcomp.py` with the arguments given |
| `null_fwvocalcomp.py` | the null test against the reference (`--quick`, `--tune key=value`, `--out report.md`); its helpers are imported by Flatwater Comp's scripts |

## Measuring

| Script | What it does |
|---|---|
| `probe_kernel.py` | Renaissance Vox: the fast peak reading of isolated short patterns |
| `probe_ramp.py` | Renaissance Vox: sample-accurate gain around events on a DC carrier |
| `probe_slow_dc.py` | Renaissance Vox: the slow path on level steps of several lengths |
| `probe_gate.py` | Renaissance Vox: the gate's gain trajectory on a DC carrier |

## Fitting

| Script | What it does |
|---|---|
| `fit_ramp2.py` | Renaissance Vox: the gain ramp law on the DC-carrier data |
| `fit_smoothing.py` | Renaissance Vox: the gain-smoothing constants against reference renders |

## Training

| Script | What it does |
|---|---|
| `render_trainset.py` | training and validation renders for Flatwater Vocal Comp's learned slow path |
| `train_net.py` | Flatwater Vocal Comp's learned slow path (a small GRU) |
| `score_net.py` | scores a trained slow-path network on the null-test signals |

## Validation

| Script | What it does |
|---|---|
| `check_vst3.py` | the built Flatwater Vocal Comp VST3 against its offline renderer (identical) |

## Editor, meters and control feel

| Script | What it does |
|---|---|
| `fit_meter_energy.py` | fits the Renaissance Vox Energy meter |
| `fit_meter_output.py` | tries rules for the Renaissance Vox output meter |
