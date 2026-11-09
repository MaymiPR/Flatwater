# Flatwater Comp null tests

The scripts that measure H-Comp, fit and train Flatwater Comp's models and null the two against each other. They need
Windows, Python 3.12 or newer with the root `requirements.txt` (`requirements-train.txt` for the learned blocks), the
original installed, the tools built (`build/`) and `local.yaml` at the repository root; the suite-wide modules they
import (hosting, state, corpora, meters) and the `local.yaml` keys are in
[scripts/nulltest/README.md](../../../scripts/nulltest/README.md). Run a script from this folder with
`python -I <script>.py`; its docstring gives its arguments. Renders, captures and result files stay git-ignored; the
results are published in [NULLTESTS.md](../NULLTESTS.md).

## Entry points

| File | Role |
|---|---|
| `nulltest.py` | every case: `null_fwcomp.py` with the arguments given, then `null_presets.py` |
| `null_fwcomp.py` | the null test against the reference (`--quick`, `--out report.md`, `--fs`); imported by Flatwater Delay's scripts |

## Reference presets

| Script | What it does |
|---|---|
| `read_ref_presets.py` | H-Comp factory presets, converted by the reference |

## Corpora and probe signals

| Script | What it does |
|---|---|
| `make_vu_probe.py` | the WAV for filming H-Comp's VU needle: steady tones, bursts, the drum loop and a voice-like signal |
| `make_vu_cal.py` | the calibration staircase for H-Comp's In / Out needle |

## Measuring

| Script | What it does |
|---|---|
| `measure_reference.py` | H-Comp: linear path, static law, detector, release and Analog sections |

## Fitting

| Script | What it does |
|---|---|
| `fit_struct.py` | H-Comp: the structure of the look-ahead gain path on spikes and bursts |
| `fit_detector.py` | H-Comp: the gain path refined on clean spike and burst cases |
| `fit_release.py` | H-Comp: the fast and slow release envelopes on burst responses |
| `fit_release2.py` | H-Comp: the two-state release law (`gain_fast2.c`) |
| `fit_release_w0.py` | H-Comp: the release-time factor w0 with the slow state charging from the output |
| `fit_release_real.py` | H-Comp: the release law refitted on realistic envelopes |
| `fit_release3.py` | H-Comp: the release, third form (`gain_fast3.c`) |
| `fit_release4.py` | H-Comp: the release, fourth and fifth forms, on probes from a settled state |
| `fit_release6.py` | H-Comp: the release, sixth form, with a learned release-time factor |
| `fit_release8.py` | H-Comp: the plug-in's release law refitted on settled probes |
| `fit_release_cma.py` | H-Comp: the release law by CMA-ES on its rough error surface |
| `probe_analog.py` | H-Comp: the Analog modes' probes (determinism, memory, static curve) |
| `fit_analog.py` | H-Comp: the Analog stage as a Wiener-Hammerstein offset |
| `fit_analog2.py` | the Analog stage in its general form, for H-Comp and H-Delay |

## Models

| Script | What it does |
|---|---|
| `model.py` | Python model of H-Comp's gain path |

## Training

| Script | What it does |
|---|---|
| `render_trainset.py` | training set for Flatwater Comp's learned gain correction |
| `render_trainset_refresh.py` | refreshes Flatwater Comp's side of a training set after a model change |
| `train_net.py` | Flatwater Comp's learned gain correction (a GRU over its own states) |
| `train_release.py` | H-Comp's release law trained end to end with exact gradients |
| `train_release2.py` | the same law with Adam on `gain_grad.c`'s gradient |

## Null tests

| Script | What it does |
|---|---|
| `null_presets.py` | Flatwater Comp's factory presets against H-Comp's own |
| `null_sweep.py` | one-parameter sweeps of the Flatwater Comp null from a base setting |

## Validation

| Script | What it does |
|---|---|
| `check_vst3.py` | the built Flatwater Comp VST3 against its renderer, tempo sync included |

## Editor, meters and control feel

| Script | What it does |
|---|---|
| `read_vu_angles.py` | the needle angle per frame of a filmed VU meter |
| `fit_pivot.py` | the VU needle's pivot from many frames |
| `measure_vu_face.py` | a VU face's print in polar coordinates about the scale's pivot |
| `fit_needle_core.py` | H-Comp's needle line per strip position |
| `fit_needle_shadow.py` | H-Comp's needle shadow on the filmed frames |
| `compare_needle.py` | pixel comparison of Flatwater Comp's needle with the filmed one |
| `compare_rivet.py` | the needle tip and rivet against Flatwater Comp's |
| `fit_gr_needle.py` | H-Comp's GR needle ballistics |
| `fit_io_needle.py` | H-Comp's In / Out needle law |
| `measure_pilot_gain.py` | the compressor's gain over time from a pilot tone in a MeterCapture run |
| `measure_pilot_cycle.py` | the gain inside each cycle of a low tone, H-Comp against Flatwater Comp |
| `parse_drags.py` | parses MeterCapture drag logs: each knob's value before and after |

## C models and their builds

Each `.c` file is a per-sample model loaded with ctypes; its `build_<name>.bat` compiles it into a git-ignored DLL
with the newest Visual Studio found by vswhere.

| File | What it does |
|---|---|
| `gain_fast.c`, `build_gain_fast.bat` | H-Comp's identified gain path |
| `gain_fast2.c`, `build_gain_fast2.bat` | the two-state release law, the plug-in's form |
| `gain_fast3.c`, `build_gain_fast3.bat` | the release law, third form |
| `gain_fast4.c`, `build_gain_fast4.bat` | the release law, fourth form |
| `gain_fast5.c`, `build_gain_fast5.bat` | the release law, fifth form |
| `gain_fast6.c`, `build_gain_fast6.bat` | the release law, sixth form (learned release-time factor) |
| `gain_grad.c`, `build_gain_grad.bat` | the sixth form with its exact gradient, for training |
