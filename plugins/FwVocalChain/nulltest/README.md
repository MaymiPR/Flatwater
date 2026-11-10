# Flatwater Vocal Chain null tests

The scripts that measure the reference, fit and train Flatwater Vocal Chain's models and null the two against each other. They
need Windows, Python 3.12 or newer with the root `requirements.txt` (`requirements-train.txt` for the learned blocks),
the reference installed, the tools built (`build/`, the native helpers by `python scripts/build_native.py`) and
`local.yaml` at the repository root; the suite-wide modules they import (the reference host, the saved state, the
corpora, the report) and the `local.yaml` keys are in [scripts/nulltest/README.md](../../../scripts/nulltest/README.md).
Run a script from this folder with `python -I <script>.py`; its docstring gives its arguments. Renders, captures and
result files stay git-ignored. The report is [NULLTESTS.md](../NULLTESTS.md), written from
`measurements/FwVocalChain/nulltests.json` by `nulltest.py --report` and never edited by hand.

## Entry points

| File | What it does |
|---|---|
| `nulltest.py` | Phase 4 nulls: the reference (cv_ref) against our Flatwater Vocal Chain VST3 (pedalboard), the same settings and input, aligned (both report their latency) |
| `first_null.py` | The first null test of Flatwater Vocal Chain: proves the harness, the hosts and the build before any measurement |
| `ch_ref.py` | Renders the reference (Stereo component) through pedalboard with any of its 68 preset-array values set through the state (scripts/nulltest/ref_state_lib.py) |
| `nulltest_lib.py` | Generic null-test and measurement helpers for the house null-test harnesses |

## Measuring

| File | What it does |
|---|---|
| `measure_campaign.py` | Phase 3, the full campaign on the reference (Stereo component unless said |
| `measure_campaign_rr.py` | Phase 3, the campaign's parts that need tools/RefRender (a host tempo, the Mono/Stereo component, parameter changes during playback): the delays at 40  |
| `measure_doubler_rates.py` | The doubler at other sample rates: a click train through the reference's Pitch stage alone (fader +10, every other colour clear |
| `measure_stages.py` | Phase 3 measurements of the reference (Stereo component), through plugins/FwVocalChain/nulltest/ch_ref.py (pedalboard |

## Probes

| File | What it does |
|---|---|
| `probe_deess.py` | Probes of the main de-esser on the reference (every colour clear, Noise off), to pin its structure before fitting: the split |
| `probe_parameters.py` | Phase 1 probe of the reference (the shell VST3, the local install named by local.yaml's ref_vst3): the components the shell offers, each one's VST3 parameters (name |
| `probe_splice.py` | How the reference's pitch shifter (Wide, Spreader) picks its step at a jump |

## Fitting

| File | What it does |
|---|---|
| `fit_compress.py` | Compress, per colour and fader step (-10  |
| `fit_compress_shrink.py` | How much of the learned Compress correction to keep, per colour and fader step: on the validation set (render_trainset.py val: held-out vocals at peaks -12 |
| `fit_deess.py` | The main de-esser (always in the chain), fitted on the reference with every colour clear |
| `fit_doubler.py` | Each voice measured on click trains through the reference (Pitch at +10, every other colour clear, the output less the output with Pitch clear |
| `fit_rates_lead.py` | (the doubler at other rates) the LFO lead's correction from a click comparison: ours - reference ~ -slope * dlead |

## Training

| File | What it does |
|---|---|
| `train_compress.py` | Learned Compress correction for Flatwater Vocal Chain (gray-box, the learned-block procedure): the fitted compressor (Compress.c |

## Renders

| File | What it does |
|---|---|
| `render_compress_cache.py` | The learned Compress correction's training cache (train_compress.py): every example of render_trainset.py, and the Phase 4 fit cases (fit_compress.py: the tone ladders |
| `render_trainset.py` | Training renders for Flatwater Vocal Chain's learned Compress correction (the learned-block procedure) |

## Null tests

| File | What it does |
|---|---|
| `null_compress_baseline.py` | The learned Compress correction's starting point: on render_trainset.py's examples, the fitted model's null (fit_compress.py's model: the clear chain's shaper |
| `null_compress_grid.py` | Compress alone (every other colour clear, the Noise off) through the reference and our renderer, on vocals no fit or training has used (vocal_corpus parts 32  |
| `null_presets.py` | Flatwater Vocal Chain's 43 factory presets against the reference's own: the reference set from its preset file (the faders, colours and Balance it stores |

## Checks and comparisons

| File | What it does |
|---|---|
| `check_compress_net.py` | The plug-in's learned Compress correction against PyTorch's: ChRender (every colour clear but Compress |
| `check_wall.py` | Wall's remaining regressions (steps -7, -6 and 0 in every training): per validation vocal, the null before and after the plug-in's pair of nets |
| `compare_doubler.py` | The doubler's behaviour against the reference (the Pitch stage's Wide and Spreader colours pick their splice points from the signal, so they are compared in behaviour |

## Reading

| File | What it does |
|---|---|
| `read_campaign.py` | Reads measure_campaign.py's results (reference/fwvocalchain/meas/campaign/*.json) and prints each section's tables: the laws of every control over all its amounts |

## Native helpers

Built by `python scripts/build_native.py FwVocalChain` into `build/native/FwVocalChain/`.

| File | What it does |
|---|---|
| `native/Compress.c` | Flatwater Vocal Chain's Compress model for fitting (loaded with ctypes by fit_compress.py |
| `native/DeEss.c` | Flatwater Vocal Chain's main de-esser model for fitting (loaded with ctypes by fit_deess.py |

## The plug-in's scripts

In [plugins/FwVocalChain/scripts/](../scripts/): the scripts that write under `Source/` or `resources/`
(`make_`, `export_`, `pack_`) and the editor measurements (`measure_ui.py`, `compare_ui.py`, `fit_<part>.py`).

| File | What it does |
|---|---|
| `../scripts/export_compress.py` | Writes plugins/FwVocalChain/Source/dsp/Compress.h from Phase 4's Compress fits: per colour its linear filter per rate (reference/fwvocalchain/meas/compress_linear.json |
| `../scripts/export_deess.py` | Writes plugins/FwVocalChain/Source/dsp/DeEss.h from the main de-esser's fit (nulltest/fit_deess.py fit): the parameters, the look-ahead |
| `../scripts/export_eq.py` | Bass and Treble |
| `../scripts/export_eq_refine.py` | A second pass over plugins/FwVocalChain/Source/dsp/EqTables.h for the positions export_eq_tables.py left above -85 dB (Bass Upper at fader 0 and up |
| `../scripts/export_eq_tables.py` | Bass and Treble exactly |
| `../scripts/export_fixed.py` | The chain's fixed filter (every colour clear, at a linear level) per sample rate: an order-8 IIR fitted |
| `../scripts/export_net.py` | Writes Flatwater Vocal Chain's learned Compress correction (build/vocalchainnet/c<colour><tag>.pt |
| `../scripts/measure_ui.py` | Measures the original vocal chain editor (its skin, extracted into the git-ignored reference/fwvocalchain/skin/) and prints a report |
| `../scripts/pack_reverb.py` | The reverb's impulse responses, packed for the plug-in (convolution, approved 2026-10-28) |
