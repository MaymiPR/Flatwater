# Suite-wide null-test modules

The modules every plug-in's null tests import: `local.yaml`, hosting the references, their saved state, the
corpora and the meter readers, and the control-feel scripts in [scripts/feel/](../feel/README). Each plug-in's own
scripts are in `plugins/<Id>/nulltest/` with a README of their own; a script there adds this folder to its
`sys.path`. They need Windows, Python 3.12 or newer with `requirements.txt` (`requirements-train.txt` for the
learned blocks), the originals installed, the tools built (`build/`), and `local.yaml` at the repository root.
Nothing rendered is committed: reference renders go to the git-ignored `renders/`, `build/` and `reference/`
folders.

## local.yaml keys

| Key | Read by | Value |
|---|---|---|
| `ref_vst3` | `refhost.py`, `plugins/FwPitchBend/nulltest/vb.py` and every script that hosts a reference | the shell VST3 that holds every original |
| `ref_name`, `<Id>.ref_name` | the standard keys of `local.example.yaml`; the scripts here name their components in code | the reference's component name, for example `H-Comp Stereo` |
| `ref_presets_dir` | `read_ref_presets.py`, `plugins/FwPitchBend/nulltest/read_ref_presets.py` | the vendor's shared preset folder (Renaissance Vox, Renaissance Bass, Vocal Bender) |
| `<Id>.ref_presets` | `read_ref_presets.py` in the `nulltest/` of FwBassLeveler, FwComp, FwDelay, FwBassXLift and FwVocalLeveler, `plugins/FwDelay/nulltest/null_presets.py`, `plugins/FwVocalChain/nulltest/measure_campaign.py`, `scripts/make_presets.py` | the reference's factory preset file for that plug-in |
| `fl_packs` | `bass_corpus.py`, `vocal_corpus.py` | FL Studio's sample packs folder (decoded once into a git-ignored cache) |
| `gpu_python` | the `train_*.py` scripts | the interpreter of the CUDA virtual environment |

## Hosting

| Script | What it does |
|---|---|
| `local_config.py` | reads `local.yaml`; `get(key)` returns a value or stops with the key's name |
| `refhost.py` | renders through a reference with pedalboard (fresh instances, dropout retries) or `tools/RefRender`; the `render_*` helpers per reference |
| `refstate.py` | reads and writes the parameter array inside a reference's saved VST3 state |

## Reference presets

| Script | What it does |
|---|---|
| `read_ref_presets.py` | Renaissance Vox and Renaissance Bass factory and artist presets, converted to real units by the reference itself |

## Corpora and probe signals

| Script | What it does |
|---|---|
| `bass_corpus.py` | 36 seeded bass parts from FL Studio's packs and synthetic voices; parts 0 to 23 train, 24 to 35 are held out |
| `vocal_corpus.py` | 36 seeded 20 s vocal parts from FL Studio's packs; parts 0 to 23 train, 24 to 35 are held out |
| `make_meter_probe.py` | input for `tools/MeterCapture`: level steps under a playing tone, tones and bursts |

## Editor, meters and control feel

| Script | What it does |
|---|---|
| `read_meter_frames.py` | reads a MeterCapture recording of a Renaissance-style editor: bar fill per frame |
| `fit_meter.py` | fits a meter law to a MeterCapture recording |
| `read_meter_capture_gl.py` | reads MeterCapture's `gl=1` output into an npz |
| `scripts/feel/parse_drags_any.py` | parses MeterCapture drag logs generically |
| `scripts/feel/run_feel.sh` | 50 px drags and 3-notch wheel moves on each control ([scripts/feel/README](../feel/README)) |
| `scripts/feel/scan_thumb.sh` | finds a fader's thumb |
| `scripts/feel/fader_feel.sh` | fader drag rates from the default thumbs |
| `scripts/feel/ctrl_feel.sh` | 200 px Ctrl drags by mouse step size |
| `scripts/feel/midctrl_feel.sh` | Ctrl pressed or released half way through a drag |
