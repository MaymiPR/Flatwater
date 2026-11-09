# Flatwater BassX Lift null tests

The scripts that measure MaxxBass, fit and train Flatwater BassX Lift's models and null the two against each other. They need
Windows, Python 3.12 or newer with the root `requirements.txt` (`requirements-train.txt` for the learned blocks), the
original installed, the tools built (`build/`) and `local.yaml` at the repository root; the suite-wide modules they
import (hosting, state, corpora, meters) and the `local.yaml` keys are in
[scripts/nulltest/README.md](../../../scripts/nulltest/README.md). Run a script from this folder with
`python -I <script>.py`; its docstring gives its arguments. Renders, captures and result files stay git-ignored; the
results are published in [NULLTESTS.md](../NULLTESTS.md).

## Entry points

| File | Role |
|---|---|
| `nulltest.py` | every case: the DSP through `BxRender` against the reference |

## Reference presets

| Script | What it does |
|---|---|
| `read_ref_presets.py` | MaxxBass factory presets, converted by the reference |

## Measuring

| Script | What it does |
|---|---|
| `probe_recovery.py` | MaxxBass level control: recovery in silence and the attack at an onset |
| `measure_linear.py` | MaxxBass: the small-signal transfer of the generated part |
| `measure_k2.py` | MaxxBass: the generated part's second-order kernel in the flat-gain region |
| `extract_g2.py` | MaxxBass: the level-control gain by forward least squares |
| `extract_g3.py` | MaxxBass: the level-control gain by Gauss-Newton with the generator's exact Jacobian |

## Fitting

| Script | What it does |
|---|---|
| `null_oracle_grid.py` | MaxxBass: grid cases re-rendered through the identified structure with the reference's own gain |

## Models

| Script | What it does |
|---|---|
| `model.py` | MaxxBass model pieces shared by the identification scripts |

## Training

| Script | What it does |
|---|---|
| `render_trainset_extract.py` | MaxxBass training set: the generated part and its extracted level-control gain |
| `train_net.py` | Flatwater BassX Lift's learned level control (gray-box) |
| `train_net_det.py` | the alternative: an explicit detector with a learned gain law |

## Null tests

| Script | What it does |
|---|---|
| `null_grid.py` | Flatwater BassX Lift on random settings, with a breakdown by parameter range |

## C models and their builds

Each `.c` file is a per-sample model loaded with ctypes; its `build_<name>.bat` compiles it into a git-ignored DLL
with the newest Visual Studio found by vswhere.

| File | What it does |
|---|---|
| `generator.c`, `build_generator.bat` | the Flatwater BassX Lift generator loop and its exact linearisation |
| `features.c`, `build_features.bat` | the Flatwater BassX Lift training feature helpers |
