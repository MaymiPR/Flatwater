# Flatwater Comp null tests

The scripts that measure the reference, fit and train Flatwater Comp's models and null the two against each other. They
need Windows, Python 3.12 or newer with the root `requirements.txt` (`requirements-train.txt` for the learned blocks),
the reference installed, the tools built (`build/`, the native helpers by `python scripts/build_native.py`) and
`local.yaml` at the repository root; the suite-wide modules they import (the reference host, the saved state, the
corpora, the report) and the `local.yaml` keys are in [scripts/nulltest/README.md](../../../scripts/nulltest/README.md).
Run a script from this folder with `python -I <script>.py`; its docstring gives its arguments. Renders, captures and
result files stay git-ignored. The report is [NULLTESTS.md](../NULLTESTS.md), written from
`measurements/FwComp/nulltests.json` by `nulltest.py --report` and never edited by hand.

## Entry points

| File | What it does |
|---|---|
| `nulltest.py` | Null tests of Flatwater Comp: the null scripts in order, the numbers NULLTESTS.md publish |
| `first_null.py` | The first null test of Flatwater Comp: proves the harness, the hosts and the build before any measurement |
| `co_ref.py` | Flatwater Comp (offline renderer of the plug-in DSP) against the reference, residual in dB |
| `nulltest_lib.py` | Generic null-test and measurement helpers for the house null-test harnesses |

## Measuring

| File | What it does |
|---|---|
| `measure_pilot_cycle.py` | The gain inside each cycle of a low tone, the reference against Flatwater Comp: a -40 dBFS 6170 Hz pilot on the tone, demodulated from input and output |
| `measure_pilot_gain.py` | The compressor's gain over time from a MeterCapture run of vu_probe_pilot.f32: the -54 dBFS 6170 Hz pilot (no harmonic of the 300 Hz tones) |
| `measure_reference.py` | Phase 3 measurements of the reference (private analysis |
| `measure_vu_face.py` | Measures a VU face's print from MeterCapture gl=1 crops (needle removed: the median of frames with the needle in three places) |

## Probes

| File | What it does |
|---|---|
| `probe_analog.py` | Identify the reference's Analog modes (compressor idle: threshold 0, ratio 1) |

## Fitting

| File | What it does |
|---|---|
| `fit_analog.py` | The Analog stage, general form: y = L(x) + H2( g( H1(x) ) ), on the datasets of probe_analog.py |
| `fit_analog_offset.py` | Fit the Analog stage, y = x + H2( g( H1(x) ) ) (a Wiener-Hammerstein offset added to the signal), on the datasets of probe_analog.py collect |
| `fit_detector.py` | Refine the reference's gain path on the clean spike / burst cases (fit_struct.py, output -18 dB so the output limiter never acts) |
| `fit_gr_needle.py` | Fits the reference's GR needle ballistics: the compressor's gain g(t) (measure_pilot_gain.py, 1 ms steps) through a candidate meter law |
| `fit_io_needle.py` | Fits the reference's In / Out needle (MeterCapture gl=1 films) with the law read off the step tests (MEASUREMENTS.md) |
| `fit_needle_core.py` | The reference's needle line per strip position (needle_set.npz): a weighted line fit on the red needle (weight = redness), its angle against the uniform strip law |
| `fit_needle_shadow.py` | Fits the reference's needle shadow on the filmed frames (needle_set.npz from the per-position frames): the dial's darkening (1 - frame / clean dial |
| `fit_pivot.py` | The VU needle's pivot from MeterCapture gl=1 frames: a robust line through the needle's pixels in each of many frames, then the point nearest all lines |
| `fit_release.py` | Identify the reference's release (fast and slow envelopes) on burst responses |
| `fit_release_law.py` | The release law the plug-in uses (the two-state form, native/ReleaseLaw.c), fitted with CMA-ES |
| `fit_release_real.py` | Refit the reference's release law on realistic envelopes |
| `fit_release_time_factor.py` | Fit rel_eff = min(release, release * w0(S), K / (R - S)^p) with the slow state charging from the output (ReleaseLaw.c), on the burst cases |
| `fit_struct.py` | Search the structure of the reference's look-ahead gain path on spike and burst responses |

## Training

| File | What it does |
|---|---|
| `train_net.py` | Trains Flatwater Comp's learned gain correction (run in the PyTorch environment, local.yaml's gpu_python): a GRU over Flatwater Comp's own states and settings |
| `train_release.py` | Train the reference's release law end to end with exact gradients (PyTorch), on the probes exported to build/hc_torch_set.npz |

## Renders

| File | What it does |
|---|---|
| `render_trainset.py` | Training set for Flatwater Comp's learned gain correction (plugins/FwComp/nulltest/train_net.py) |
| `render_trainset_refresh.py` | Refreshes Flatwater Comp's side of an existing training set (render_trainset.py) after a change to its analytical model: each case keeps its input |
| `render_vu_calibration.py` | Calibration probe for the reference's In / Out needle (filmed with MeterCapture gl=1): 1 s of silence, a 300 Hz sine staircase from 0 to -60 dBFS |
| `render_vu_probe.py` | WAV for filming the reference's VU needle in FL: a 1 s count-in click, then steady 300 Hz tones (constant gain reduction), bursts of 5 / 50 / 300 ms |

## Null tests

| File | What it does |
|---|---|
| `null_presets.py` | Null test of Flatwater Comp's factory presets against the reference with the same settings (the reference's own presets): each preset on program-like signals |
| `null_sweep.py` | One-parameter sweeps of the Flatwater Comp null from a base setting (to find which control the weak nulls come from) |

## Checks and comparisons

| File | What it does |
|---|---|
| `compare_needle.py` | Pixel comparison of Flatwater Comp's VU needle (plugins/FwComp/tools/CoVuRender) with the reference's filmed needle at the same strip positions (needle_set_<face>.npz) |
| `compare_rivet.py` | The reference's needle tip and rivet against Flatwater Comp's (needle_set_io.npz / needle_set_wc.npz): patches in needle coordinates (s along, n across, 0.1 px) |

## Reading

| File | What it does |
|---|---|
| `read_drags.py` | Parses MeterCapture drag-script logs: for each drag, the knob's parameter before and after, as text values |
| `read_ref_presets.py` | Reads the reference's presets (the factory preset file of the local install, named by local.yaml's FwComp.ref_presets) and writes their settings |
| `read_vu_angles.py` | Needle angle per frame from scripts/nulltest/read_meter_capture_gl.py output (MeterCapture gl=1 films): rays from the pivot, each scored by how red it is |

## Helper modules

| File | What it does |
|---|---|
| `model_lib.py` | A Python model of the reference's gain path, built up as it is identified (the fit_*.py scripts beside it) |
| `release_law_lib.py` | The release law of Flatwater Comp in its two-state form (native/ReleaseLaw.c, the plug-in's form): the C model's binding, the knot tables and their packing |

## Native helpers

Built by `python scripts/build_native.py FwComp` into `build/native/FwComp/`.

| File | What it does |
|---|---|
| `native/GainPath.c` | The reference's gain path as identified so far, per sample (loaded with ctypes by model_lib.py) |
| `native/ReleaseLaw.c` | The reference's release as a two-state law in the gain-reduction domain (loaded by release_law_lib.py) |

## The plug-in's scripts

In [plugins/FwComp/scripts/](../scripts/): the scripts that write under `Source/` or `resources/`
(`make_`, `export_`, `pack_`) and the editor measurements (`measure_ui.py`, `compare_ui.py`, `fit_<part>.py`).

| File | What it does |
|---|---|
| `../scripts/export_net.py` | Writes Flatwater Comp's trained gain correction (build/fwcompnet/<name>.pt |
| `../scripts/export_tables.py` | Writes plugins/FwComp/Source/dsp/Tables.h from the Flatwater Comp measurements: the static curve's constants (knee_model), the makeup slopes per ratio (makeup.csv) |
| `../scripts/fit_knobs.py` | Measures the reference Hybrid-line knobs (reference/fwcomp/knobs, git-ignored captures) and prints a report: each knob's radius, edge and cast shadow |
