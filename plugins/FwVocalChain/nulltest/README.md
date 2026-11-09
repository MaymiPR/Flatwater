# Flatwater Vocal Chain null tests

The scripts that measure CLA Vocals, fit and train Flatwater Vocal Chain's models and null the two against each other. They need
Windows, Python 3.12 or newer with the root `requirements.txt` (`requirements-train.txt` for the learned blocks), the
original installed, the tools built (`build/`) and `local.yaml` at the repository root; the suite-wide modules they
import (hosting, state, corpora, meters) and the `local.yaml` keys are in
[scripts/nulltest/README.md](../../../scripts/nulltest/README.md). Run a script from this folder with
`python -I <script>.py`; its docstring gives its arguments. Renders, captures and result files stay git-ignored; the
results are published in [NULLTESTS.md](../NULLTESTS.md).

## Entry points

| File | Role |
|---|---|
| `nulltest.py` | every case: the VST3 against the reference, aligned by both latencies |
| `cv.py` | the harness: renders the reference through pedalboard with any of its preset-array values set through the state |

## Measuring

| Script | What it does |
|---|---|
| `measure_stages.py` | CLA Vocals: each stage and colour through `cv.py` |
| `measure_campaign.py` | CLA Vocals: every control at every amount, level dependence, every pair of stages |
| `measure_campaign_rr.py` | the campaign's parts that need RefRender: delays at 40 to 240 bpm, the Mono / Stereo component, changes during playback |
| `analyse_campaign.py` | prints the campaign's tables: the laws of every control, level dependence, pair interactions |
| `probe_parameters.py` | CLA Vocals' components and VST3 parameters |
| `probe_deess.py` | the main de-esser's structure: the split, the detector, the gain law |
| `probe_splice.py` | how the doubler's pitch shifter picks its step at a jump |
| `measure_doubler_rates.py` | the doubler at other sample rates, from click trains |

## Fitting

| Script | What it does |
|---|---|
| `fit_eq.py` | CLA Vocals: Bass and Treble per colour and fader step |
| `fit_eq_tables.py` | CLA Vocals: Bass and Treble at all 201 fader positions as low-order filters |
| `fit_eq_refine.py` | a second pass over the EQ table positions left above -85 dB |
| `fit_fixed.py` | the chain's fixed filter per sample rate, an order-8 IIR |
| `fit_deess.py` | the main de-esser |
| `fit_compress.py` | Compress per colour and fader step |
| `export_compress.py` | writes `plugins/FwVocalChain/FwVocalChainCompress.h` from the Compress fits |
| `fit_doubler2.py` | the doubler's voices measured on click trains |
| `fit_rates_lead.py` | the doubler's LFO lead correction at other rates |
| `pack_reverb.py` | packs the reverb's measured impulse responses for the plug-in |

## Training

| Script | What it does |
|---|---|
| `render_trainset.py` | training renders for Flatwater Vocal Chain's learned Compress correction |
| `prep_compress.py` | the Compress correction's training cache |
| `null_compress_baseline.py` | the fitted model's null on the training examples, the correction's starting point |
| `train_compress.py` | Flatwater Vocal Chain's learned Compress correction (gray-box) |
| `fit_compress_shrink.py` | how much of the correction to keep per colour and fader step, on the validation set |
| `check_wall.py` | Wall's remaining regressions per validation vocal |

## Null tests

| Script | What it does |
|---|---|
| `null_presets.py` | Flatwater Vocal Chain's 43 factory presets against CLA Vocals' own |
| `null_compress_grid.py` | Compress alone on vocals no fit or training used |
| `compare_doubler.py` | the Wide and Spreader doublers compared in behaviour |

## Validation

| Script | What it does |
|---|---|
| `check_compress_net.py` | the plug-in's learned Compress correction against PyTorch's |

## C models and their builds

Each `.c` file is a per-sample model loaded with ctypes; its `build_<name>.bat` compiles it into a git-ignored DLL
with the newest Visual Studio found by vswhere.

| File | What it does |
|---|---|
| `comp.c`, `build_comp.bat` | Flatwater Vocal Chain's Compress model |
| `deess.c`, `build_deess.bat` | Flatwater Vocal Chain's main de-esser model |
