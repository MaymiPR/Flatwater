# Suite-wide null-test modules

The modules every plug-in's null tests import: `local.yaml`, the reference host, the references' saved state, the
corpora, the meter readers and the null-test report, and the control-feel scripts in
[scripts/feel/](../feel/README). Each plug-in's own scripts are in `plugins/<Id>/nulltest/` with a README of their own;
its harness `<ab>_ref.py` (the plug-in's two-letter abbreviation) holds what is particular to its reference and adds
this folder to `sys.path`. They need Windows, Python 3.12 or newer with `requirements.txt` (`requirements-train.txt`
for the learned blocks), the references installed, the tools built (`build/`; the native helpers by
`python scripts/build_native.py`) and `local.yaml` at the repository root. Nothing rendered is committed: reference
renders go to the git-ignored `renders/`, `build/` and `reference/` folders.

## local.yaml keys

| Key | Read by | Value |
|---|---|---|
| `ref_vst3` | `refhost.py` and every harness | the shell VST3 that holds every reference |
| `<Id>.ref_name` | each plug-in's harness `<ab>_ref.py` | the reference's stereo component in the shell; the harness has a default for every plug-in but Flatwater BassX Lift, whose key is required |
| `FwVocalComp.ref_mono_name` | `plugins/FwVocalComp/nulltest/vc_ref.py` | the reference's mono component (a default otherwise) |
| `ref_presets_dir` | `read_ref_presets.py`, `plugins/FwPitchBend/nulltest/read_ref_presets.py` | the vendor's shared preset folder |
| `<Id>.ref_presets` | `read_ref_presets.py` in the `nulltest/` of FwBassLeveler, FwComp, FwDelay, FwBassXLift and FwVocalLeveler, `plugins/FwDelay/nulltest/null_presets.py`, `plugins/FwVocalChain/nulltest/measure_campaign.py`, `scripts/make_presets.py` | the reference's factory preset file for that plug-in |
| `fl_packs` | `bass_corpus_lib.py`, `vocal_corpus_lib.py` | FL Studio's sample packs folder (decoded once into a git-ignored cache) |
| `gpu_python` | the `train_*.py` scripts | the interpreter of the CUDA virtual environment |

## Hosting and state

| Module | What it does |
|---|---|
| `local_config_lib.py` | reads `local.yaml`: `get(key)` returns a value or stops with the key's name; `local_file(path)` finds a local data file under its earlier name while only that one exists (`renamed_local_files.txt`) |
| `refhost.py` | the reference host: fresh pedalboard instances with dropout retries, `tools/RefRender` on the shell or on one of our built VST3 bundles, our offline renderers, the VST3-equals-renderer check |
| `ref_state_lib.py` | reads and writes the parameter array inside a reference's saved VST3 state |
| `renamed_local_files.txt` | the local data files renamed in 1.1 (no vendor word names a file a script reads), each with its earlier name |

## Reports

| Module | What it does |
|---|---|
| `report_lib.py` | each plug-in's `NULLTESTS.md`, written from `measurements/<Id>/nulltests.json` (the report as data: a section per kind of case, a part per null script); `nulltest.py --report` calls it |

## Reference presets

| Script | What it does |
|---|---|
| `read_ref_presets.py` | the factory and artist presets of Flatwater Bass Lift's and Flatwater Vocal Comp's references, converted to real units by the reference itself, into `build/ref_presets_<Id>.json` |

## Corpora and probe signals

| Module | What it does |
|---|---|
| `bass_corpus_lib.py` | 36 seeded bass parts from FL Studio's packs and synthetic voices; parts 0 to 23 train, 24 to 35 are held out |
| `vocal_corpus_lib.py` | 36 seeded 20 s vocal parts from FL Studio's packs; parts 0 to 23 train, 24 to 35 are held out |
| `render_meter_probe.py` | input for `tools/MeterCapture`: level steps under a playing tone, tones and bursts |

## Editor, meters and control feel

| Script | What it does |
|---|---|
| `read_meter_frames.py` | reads a MeterCapture recording of an editor: bar fill per frame |
| `fit_meter.py` | fits a meter law to a MeterCapture recording |
| `read_meter_capture_gl.py` | reads MeterCapture's `gl=1` output into an npz |
| `scripts/feel/read_drags_any.py` | parses MeterCapture drag logs generically |
| `scripts/feel/run_feel.sh` | 50 px drags and 3-notch wheel moves on each control ([scripts/feel/README](../feel/README)) |
| `scripts/feel/scan_thumb.sh` | finds a fader's thumb |
| `scripts/feel/fader_feel.sh` | fader drag rates from the default thumbs |
| `scripts/feel/ctrl_feel.sh` | 200 px Ctrl drags by mouse step size |
| `scripts/feel/midctrl_feel.sh` | Ctrl pressed or released half way through a drag |
