# Flatwater Delay null tests

The scripts that measure the reference, fit and train Flatwater Delay's models and null the two against each other. They
need Windows, Python 3.12 or newer with the root `requirements.txt` (`requirements-train.txt` for the learned blocks),
the reference installed, the tools built (`build/`, the native helpers by `python scripts/build_native.py`) and
`local.yaml` at the repository root; the suite-wide modules they import (the reference host, the saved state, the
corpora, the report) and the `local.yaml` keys are in [scripts/nulltest/README.md](../../../scripts/nulltest/README.md).
Run a script from this folder with `python -I <script>.py`; its docstring gives its arguments. Renders, captures and
result files stay git-ignored. The report is [NULLTESTS.md](../NULLTESTS.md), written from
`measurements/FwDelay/nulltests.json` by `nulltest.py --report` and never edited by hand.

## Entry points

| File | What it does |
|---|---|
| `nulltest.py` | Flatwater Delay (offline renderer of the plug-in DSP, plugins/FwDelay/tools/DeRender) against the reference (de_ref.py), residual in dB |
| `first_null.py` | The first null test of Flatwater Delay: proves the harness, the hosts and the build before any measurement |
| `de_ref.py` | Renders the reference offline (tools/RefRender, a JUCE host with a tempo) for the Phase 3 measurements |
| `nulltest_lib.py` | Generic null-test and measurement helpers for the house null-test harnesses |

## Measuring

| File | What it does |
|---|---|
| `measure_analog.py` | The reference's Analog modes (its stage colours the input: Mix 0 gives the stage's output alone) |

## Probes

| File | What it does |
|---|---|
| `probe_analog_tf.py` | An Analog mode's transfer function at a low level, the reference against DeRender: a multitone of integer-Hz sines (random phases, each at the given level |
| `probe_lfo_trace.py` | The delay's trace d(t) under modulation, from the phase of a steady sine through the wet path (Mix 100, Feedback 0), the reference against DeRender |
| `probe_mod.py` | Modulation nulls over Rate x Delay x Depth (ms mode, Feedback 0, Analog Off), voice-like signal |

## Fitting

| File | What it does |
|---|---|
| `fit_lfo.py` | Fits the reference's modulation (Phase 3): the LFO moves the target time T = D0 (1 + k depth/100 shape(phase)), and the delay follows T with the glide law (d' = |

## Null tests

| File | What it does |
|---|---|
| `null_presets.py` | Null test of Flatwater Delay's factory presets against the reference's OWN presets: each preset array of the reference's preset file (the local install's |
| `null_presets_values.py` | Null test of Flatwater Delay's factory presets against the reference with the same preset values: the reference set by exact normalised values (its own tapers |

## Reading

| File | What it does |
|---|---|
| `read_ref_presets.py` | Reads the reference's presets (the factory preset file of the local install, named by local.yaml's FwDelay.ref_presets) and writes their settings |

## The plug-in's scripts

In [plugins/FwDelay/scripts/](../scripts/): the scripts that write under `Source/` or `resources/`
(`make_`, `export_`, `pack_`) and the editor measurements (`measure_ui.py`, `compare_ui.py`, `fit_<part>.py`).

| File | What it does |
|---|---|
| `../scripts/export_analog_tables.py` | Writes plugins/FwDelay/Source/dsp/AnalogTables.h: the reference's Analog modes |
| `../scripts/fit_display.py` | Fits the segment font of the reference's time display (reference/fwdelay, git-ignored screenshots) and prints the geometry Flatwater Delay's display draws with |
| `../scripts/fit_meter_housing.py` | Fits the reference's output meter housing (reference/fwdelay/st_1_hover.png, git-ignored) as a shading model (a measurement): the frame |
| `../scripts/fit_tap_pad.py` | Fits the reference's tap pad (reference/fwdelay/notes: x_3_hover released, x_4_down pressed |
| `../scripts/measure_ui.py` | Measures the reference editor on its screenshots (reference/fwdelay, git-ignored): each knob's centre and radius |
