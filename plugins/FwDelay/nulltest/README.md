# Flatwater Delay null tests

The scripts that measure H-Delay, fit and train Flatwater Delay's models and null the two against each other. They need
Windows, Python 3.12 or newer with the root `requirements.txt` (`requirements-train.txt` for the learned blocks), the
original installed, the tools built (`build/`) and `local.yaml` at the repository root; the suite-wide modules they
import (hosting, state, corpora, meters) and the `local.yaml` keys are in
[scripts/nulltest/README.md](../../../scripts/nulltest/README.md). Run a script from this folder with
`python -I <script>.py`; its docstring gives its arguments. Renders, captures and result files stay git-ignored; the
results are published in [NULLTESTS.md](../NULLTESTS.md).

## Entry points

| File | Role |
|---|---|
| `nulltest.py` | every case against the reference (`--quick`, `--out report.md`, `--fs`, `--band`) |
| `hd.py` | the harness: renders the reference offline through `tools/RefRender` with a host tempo; settings by short names |

## Reference presets

| Script | What it does |
|---|---|
| `read_ref_presets.py` | H-Delay factory presets, converted by the reference |

## Measuring

| Script | What it does |
|---|---|
| `probe_analog_tf.py` | an H-Delay Analog mode's low-level transfer function against DeRender |
| `probe_lfo_trace.py` | the delay trace under modulation from a steady sine's phase, against DeRender |
| `probe_mod.py` | modulation nulls over Rate, Delay and Depth |
| `measure_analog.py` | H-Delay's Analog modes, with the probe set of the H-Comp Analog stage |

## Fitting

| Script | What it does |
|---|---|
| `fit_lfo.py` | H-Delay: the modulation's LFO and the glide law of the delay time |

## Null tests

| Script | What it does |
|---|---|
| `null_presets_values.py` | Flatwater Delay's presets against H-Delay set to the same values |
| `null_presets.py` | Flatwater Delay's presets against H-Delay's own preset state |
