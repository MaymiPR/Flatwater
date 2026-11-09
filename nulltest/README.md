# Null-test and measurement harness

The scripts that measure the originals, fit and train the models, and null-test every Flatwater plug-in against its
original. They need Windows, Python 3.12 or newer with `requirements.txt` (`requirements-train.txt` for the learned
blocks), the originals installed, the tools built (`build/`), and `local.yaml` at the repository root. Run a script
from this folder with `python -I <script>.py`; its docstring gives its arguments. Nothing rendered here is committed:
reference renders go to the git-ignored `renders/`, `build/` and `reference/` folders.

## local.yaml keys

| Key | Read by | Value |
|---|---|---|
| `ref_vst3` | `refhost.py`, `pb_host.py` and every script that hosts a reference | the shell VST3 that holds every original |
| `ref_name`, `<Id>.ref_name` | the standard keys of `local.example.yaml`; the scripts here name their components in code | the reference's component name, for example `H-Comp Stereo` |
| `ref_presets_dir` | `read_ref_presets.py`, `read_ref_presets_pb.py` | the vendor's shared preset folder (Renaissance Vox, Renaissance Bass, Vocal Bender) |
| `<Id>.ref_presets` | `read_ref_presets_br.py`, `_hc`, `_hd`, `_mb`, `_vr`, `nulltest_fwdelay_refpresets.py`, `campaign_vocalchain.py`, `tools/make_presets.py` | the reference's factory preset file for that plug-in |
| `fl_packs` | `bass_corpus.py`, `vocal_corpus.py` | FL Studio's sample packs folder (decoded once into a git-ignored cache) |
| `gpu_python` | the `train_*.py` scripts | the interpreter of the CUDA virtual environment |

## Hosting

| Script | What it does |
|---|---|
| `local_config.py` | reads `local.yaml`; `get(key)` returns a value or stops with the key's name |
| `refhost.py` | renders through a reference with pedalboard (fresh instances, dropout retries) or `tools/RefRender`; the `render_*` helpers per reference |
| `refstate.py` | reads and writes the parameter array inside a reference's saved VST3 state |
| `refdelay.py` | renders H-Delay offline through `tools/RefRender` with a host tempo; settings by short names |
| `cv_ref.py` | renders CLA Vocals through pedalboard with any of its preset-array values set through the state |
| `pb_host.py` | renders Vocal Bender (RefRender), Flatwater Pitch Bend's processor and its VST3 |
| `pb_state.py` | writes the Vocal Bender settings only its editor sets (modulation slots, Fine, the modulators) into its state |

## Reference presets

| Script | What it does |
|---|---|
| `read_ref_presets.py` | Renaissance Vox and Renaissance Bass factory and artist presets, converted to real units by the reference itself |
| `read_ref_presets_br.py` | Bass Rider factory presets |
| `read_ref_presets_hc.py` | H-Comp factory presets, converted by the reference |
| `read_ref_presets_hd.py` | H-Delay factory presets, converted by the reference |
| `read_ref_presets_mb.py` | MaxxBass factory presets, converted by the reference |
| `read_ref_presets_pb.py` | Vocal Bender factory presets in Flatwater Pitch Bend's terms; drawn shapes are only named, never stored |
| `read_ref_presets_vr.py` | Vocal Rider factory presets |

## Corpora and probe signals

| Script | What it does |
|---|---|
| `bass_corpus.py` | 36 seeded bass parts from FL Studio's packs and synthetic voices; parts 0 to 23 train, 24 to 35 are held out |
| `vocal_corpus.py` | 36 seeded 20 s vocal parts from FL Studio's packs; parts 0 to 23 train, 24 to 35 are held out |
| `br_probes.py` | the Bass Rider note-detector probe set: steady notes, legato dips, decays, re-attacks, gaps |
| `meter_probe.py` | input for `tools/MeterCapture`: level steps under a playing tone, tones and bursts |
| `make_vu_probe.py` | the WAV for filming H-Comp's VU needle: steady tones, bursts, the drum loop and a voice-like signal |
| `make_vu_cal.py` | the calibration staircase for H-Comp's In / Out needle |
| `render_pin.py` | Flatwater Bass Lift identification: a low carrier plus a pinning tone the wide detector sees and the l band does not |
| `render_pinned.py` | low carriers with the wide detector pinned by a strong tone |
| `render_transients.py` | transient material through Renaissance Bass at several Freq settings, the generated part split off |
| `fwbasslift_fitdata.py` | the Renaissance Bass fitting set: the generated path alone, split off with two Intensity settings |

## Measuring

| Script | What it does |
|---|---|
| `measure_ref_comp.py` | H-Comp: linear path, static law, detector, release and Analog sections |
| `measure_bassleveler.py` | Bass Rider: the applied gain as output over input, per section of MEASUREMENTS.md |
| `measure_vocalleveler.py` | Vocal Rider: the applied gain sample by sample, per section of MEASUREMENTS.md |
| `measure_vocalchain.py` | CLA Vocals: each stage and colour through `cv_ref.py` |
| `campaign_vocalchain.py` | CLA Vocals: every control at every amount, level dependence, every pair of stages |
| `campaign_vocalchain_rr.py` | the campaign's parts that need RefRender: delays at 40 to 240 bpm, the Mono / Stereo component, changes during playback |
| `analyse_vocalchain.py` | prints the campaign's tables: the laws of every control, level dependence, pair interactions |
| `measure_pitchbend_mods.py` | Vocal Bender's modulators measured through its Mix (an exact crossfade) |
| `measure_pitchbend_shifter.py` | Vocal Bender's pitch and formant shifter measured as behaviour, not as a null |
| `measure_pitchbend_control.py` | what the shifter keys on: the knobs' own values or the modulated totals |
| `measure_pitchbend_level_flatten.py` | the shifter's voiced level under Flatten |
| `measure_pitchbend_level_grid.py` | the voiced level per material at fixed settings, the reference against ours |
| `measure_pitchbend_modpitch.py` | a modulated Pitch, the reference against ours: when a stepped Pitch lands |
| `probe_bassleveler.py` | Bass Rider's components and VST3 parameters (names, defaults, value texts) |
| `probe_vocalleveler.py` | Vocal Rider's components and VST3 parameters |
| `probe_vocalchain.py` | CLA Vocals' components and VST3 parameters |
| `probe_deess.py` | the main de-esser's structure: the split, the detector, the gain law |
| `probe_vocalchain_splice.py` | how the doubler's pitch shifter picks its step at a jump |
| `rates_vocalchain_doubler.py` | the doubler at other sample rates, from click trains |
| `probe_hd_analog_tf.py` | an H-Delay Analog mode's low-level transfer function against DeRender |
| `probe_hd_lfo_trace.py` | the delay trace under modulation from a steady sine's phase, against DeRender |
| `probe_hd_mod.py` | modulation nulls over Rate, Delay and Depth |
| `probe_recovery_mb.py` | MaxxBass level control: recovery in silence and the attack at an onset |
| `br_traces.py` | Bass Rider's applied gain on musical material, beside the probes |
| `br_corpus_ref.py` | Bass Rider's riding on the bass corpus at a set of settings |
| `vr_ref.py` | Vocal Rider renders of the vocal corpus and a probe set, for fitting the riding |
| `ident_ref_basslift_nl.py` | Renaissance Bass: the harmonic generator |
| `ident_ref_basslift_k2.py` | Renaissance Bass: the generator's second-order kernel from two-tone tests |
| `ident_ref_basslift_sweep.py` | Renaissance Bass: harmonic responses against the fundamental, placing the filters around the nonlinearity |
| `ident_mb_linear.py` | MaxxBass: the small-signal transfer of the generated part |
| `ident_mb_k2.py` | MaxxBass: the generated part's second-order kernel in the flat-gain region |
| `extract_cs.py` | Renaissance Bass: the generated path's time-varying gain by least squares |
| `extract_g2.py` | MaxxBass: the level-control gain by forward least squares |
| `extract_g3.py` | MaxxBass: the level-control gain by Gauss-Newton with the generator's exact Jacobian |
| `ident_kernel.py` | Renaissance Vox: the fast peak reading of isolated short patterns |
| `ident_ramp.py` | Renaissance Vox: sample-accurate gain around events on a DC carrier |
| `ident_slow_dc.py` | Renaissance Vox: the slow path on level steps of several lengths |
| `ident_gate.py` | Renaissance Vox: the gate's gain trajectory on a DC carrier |
| `ident_hd_analog.py` | H-Delay's Analog modes, with the probe set of the H-Comp Analog stage |

## Fitting

| Script | What it does |
|---|---|
| `ident_hc_struct.py` | H-Comp: the structure of the look-ahead gain path on spikes and bursts |
| `ident_hc_detector.py` | H-Comp: the gain path refined on clean spike and burst cases |
| `ident_hc_release.py` | H-Comp: the fast and slow release envelopes on burst responses |
| `ident_hc_release2.py` | H-Comp: the two-state release law (`hcfast2.c`) |
| `ident_hc_w0.py` | H-Comp: the release-time factor w0 with the slow state charging from the output |
| `ident_hc_real.py` | H-Comp: the release law refitted on realistic envelopes |
| `ident_hc_rel3.py` | H-Comp: the release, third form (`hcfast3.c`) |
| `ident_hc_rel4.py` | H-Comp: the release, fourth and fifth forms, on probes from a settled state |
| `ident_hc_rel6.py` | H-Comp: the release, sixth form, with a learned release-time factor |
| `ident_hc_rel8.py` | H-Comp: the plug-in's release law refitted on settled probes |
| `cma_hc_release.py` | H-Comp: the release law by CMA-ES on its rough error surface |
| `ident_hc_analog.py` | H-Comp: the Analog modes' probes (determinism, memory, static curve) |
| `fit_hc_analog.py` | H-Comp: the Analog stage as a Wiener-Hammerstein offset |
| `fit_hc_analog2.py` | the Analog stage in its general form, for H-Comp and H-Delay |
| `fit_hd_lfo.py` | H-Delay: the modulation's LFO and the glide law of the delay time |
| `fit_ramp2.py` | Renaissance Vox: the gain ramp law on the DC-carrier data |
| `tune_fwvocalcomp.py` | Renaissance Vox: the gain-smoothing constants against reference renders |
| `fit_twodet.py` | Renaissance Bass: the two-detector gain model on steady harmonics |
| `fit_multiF2.py` | Renaissance Bass: a joint time-domain fit over four Freq settings |
| `fit_combo.py` | Renaissance Bass: the combined time-domain and steady-state fit |
| `fit_lstage.py` | Renaissance Bass: the l-driven gain stage alone on the pinned set |
| `oracle_grid_mb.py` | MaxxBass: grid cases re-rendered through the identified structure with the reference's own gain |
| `br_fit.py` | Bass Rider: the note detector of the riding model by CMA-ES on probes and traces |
| `br_polish.py` | Bass Rider: per-setting values polished one at a time on a grid |
| `br_lock.py` | Bass Rider: the learned lock of the ride after a note's start |
| `br_export.py` | writes the fitted detector constants into `plugins/FwBassLeveler/Source/dsp/Ride.h` |
| `design_hilbert.py` | the quadrature allpass pair of Flatwater Bass Leveler's envelopes |
| `vr_fit.py` | Vocal Rider: the riding model's continuous constants by CMA-ES |
| `fit_vocalchain_eq.py` | CLA Vocals: Bass and Treble per colour and fader step |
| `fit_vocalchain_eqtables.py` | CLA Vocals: Bass and Treble at all 201 fader positions as low-order filters |
| `fit_vocalchain_eqrefine.py` | a second pass over the EQ table positions left above -85 dB |
| `fit_vocalchain_fixed.py` | the chain's fixed filter per sample rate, an order-8 IIR |
| `fit_vocalchain_deess.py` | the main de-esser |
| `fit_vocalchain_compress.py` | Compress per colour and fader step |
| `gen_vocalchain_compress.py` | writes `plugins/FwVocalChain/Source/dsp/Compress.h` from the Compress fits |
| `fit_vocalchain_doubler2.py` | the doubler's voices measured on click trains |
| `rates_lead_fit.py` | the doubler's LFO lead correction at other rates |
| `pack_vocalchain_reverb.py` | packs the reverb's measured impulse responses for the plug-in |

## Models

| Script | What it does |
|---|---|
| `hcmodel.py` | Python model of H-Comp's gain path |
| `mbmodel.py` | MaxxBass model pieces shared by the identification scripts |
| `fwbasslift_model.py` | Python model of Renaissance Bass's generated path |
| `brmodel.py` | the Python side of `brmodel.c`: parameters, control mapping, the ride in dB |
| `brcompare.py` | the riding model against Bass Rider, case by case |
| `br_corpus_eval.py` | the riding model against Bass Rider on the bass corpus |
| `vrmodel.py` | the ctypes front end of `vrmodel.c` |
| `vr_eval.py` | the riding model against the Vocal Rider renders |

## Training

| Script | What it does |
|---|---|
| `render_trainset.py` | training and validation renders for Flatwater Bass Lift's learned gain block |
| `train_gain.py` | Flatwater Bass Lift's learned level-to-gain block (gray-box) |
| `render_trainset_mb.py` | MaxxBass training set: the generated part and its extracted level-control gain |
| `train_fwbassxlift.py` | Flatwater BassX Lift's learned level control (gray-box) |
| `train_fwbassxlift_det.py` | the alternative: an explicit detector with a learned gain law |
| `render_trainset_fwvocalcomp.py` | training and validation renders for Flatwater Vocal Comp's learned slow path |
| `train_fwvocalcomp.py` | Flatwater Vocal Comp's learned slow path (a small GRU) |
| `eval_fwvocalcompnet.py` | scores a trained slow-path network on the null-test signals |
| `render_trainset_fwcomp.py` | training set for Flatwater Comp's learned gain correction |
| `refresh_trainset_fwcomp.py` | refreshes Flatwater Comp's side of a training set after a model change |
| `train_fwcomp.py` | Flatwater Comp's learned gain correction (a GRU over its own states) |
| `train_hc_release.py` | H-Comp's release law trained end to end with exact gradients |
| `train_hc_release2.py` | the same law with Adam on `hcgrad.c`'s gradient |
| `render_trainset_vocalchain.py` | training renders for Flatwater Vocal Chain's learned Compress correction |
| `prep_vocalchain_compress.py` | the Compress correction's training cache |
| `baseline_vocalchain_compress.py` | the fitted model's null on the training examples, the correction's starting point |
| `train_vocalchain_compress.py` | Flatwater Vocal Chain's learned Compress correction (gray-box) |
| `shrink_vocalchain_compress.py` | how much of the correction to keep per colour and fader step, on the validation set |
| `diag_vocalchain_wall.py` | Wall's remaining regressions per validation vocal |

## Null tests

| Script | What it does |
|---|---|
| `nulltest_fwvocalcomp.py` | Flatwater Vocal Comp against Renaissance Vox |
| `nulltest_fwbasslift.py` | Flatwater Bass Lift against Renaissance Bass |
| `nulltest_fwbassxlift.py` | Flatwater BassX Lift against MaxxBass |
| `nullgrid_fwbassxlift.py` | Flatwater BassX Lift on random settings, with a breakdown by parameter range |
| `nulltest_fwcomp.py` | Flatwater Comp against H-Comp |
| `nulltest_fwcomp_presets.py` | Flatwater Comp's factory presets against H-Comp's own |
| `sweep_fwcomp.py` | one-parameter sweeps of the Flatwater Comp null from a base setting |
| `nulltest_fwdelay.py` | Flatwater Delay against H-Delay |
| `nulltest_fwdelay_presets.py` | Flatwater Delay's presets against H-Delay set to the same values |
| `nulltest_fwdelay_refpresets.py` | Flatwater Delay's presets against H-Delay's own preset state |
| `nulltest_fwbassleveler.py` | Flatwater Bass Leveler against Bass Rider |
| `nulltest_fwbassleveler_presets.py` | Flatwater Bass Leveler's factory presets against Bass Rider's own |
| `nulltest_fwvocalleveler.py` | Flatwater Vocal Leveler against Vocal Rider |
| `nulltest_fwvocalleveler_presets.py` | Flatwater Vocal Leveler's factory presets against Vocal Rider's own |
| `nulltest_fwvocalchain.py` | Flatwater Vocal Chain's VST3 against CLA Vocals, aligned by both latencies |
| `nulltest_fwvocalchain_presets.py` | Flatwater Vocal Chain's 43 factory presets against CLA Vocals' own |
| `grid_vocalchain_compress.py` | Compress alone on vocals no fit or training used |
| `behaviour_vocalchain_doubler.py` | the Wide and Spreader doublers compared in behaviour |
| `null_pitchbend_mods.py` | Flatwater Pitch Bend's modulators against Vocal Bender's, through Mix |
| `null_pitchbend_presets.py` | Flatwater Pitch Bend's 40 factory presets against Vocal Bender's own |
| `score_pitchbend_shifter.py` | the shifter's scorecard (M1 to M8) on our shifter alone |

## Validation

| Script | What it does |
|---|---|
| `check_plugin.py` | the built Flatwater Vocal Comp VST3 against its offline renderer (identical) |
| `check_fwcomp_plugin.py` | the built Flatwater Comp VST3 against its renderer, tempo sync included |
| `validate_fwbassleveler.py` | Flatwater Bass Leveler: the VST3 against the renderer, and the release checks |
| `validate_fwvocalleveler.py` | Flatwater Vocal Leveler: the VST3 against the renderer, stereo and automation |
| `first_null_fwpitchbend.py` | proves the hosts and the build before any DSP (PASS or FAIL per check) |
| `check_fwbassleveler_port.py` | the C++ ride against the fitted model `brmodel.c` |
| `check_fwvocalleveler_port.py` | the C++ ride against `vrmodel.c` |
| `check_vocalchain_compressnet.py` | the plug-in's learned Compress correction against PyTorch's |

## Editor, meters and control feel

| Script | What it does |
|---|---|
| `meter_frames.py` | reads a MeterCapture recording of a Renaissance-style editor: bar fill per frame |
| `fit_meter.py` | fits a meter law to a MeterCapture recording |
| `fit_meter_energy.py` | fits the Renaissance Vox Energy meter |
| `fit_meter_vox.py` | tries rules for the Renaissance Vox output meter |
| `read_mc_gl.py` | reads MeterCapture's `gl=1` output into an npz |
| `vu_angles.py` | the needle angle per frame of a filmed VU meter |
| `pivot_fit.py` | the VU needle's pivot from many frames |
| `measure_vu_face.py` | a VU face's print in polar coordinates about the scale's pivot |
| `needle_core_fit.py` | H-Comp's needle line per strip position |
| `needle_shadow_fit.py` | H-Comp's needle shadow on the filmed frames |
| `compare_needle.py` | pixel comparison of Flatwater Comp's needle with the filmed one |
| `rivet_cmp.py` | the needle tip and rivet against Flatwater Comp's |
| `fit_gr_needle.py` | H-Comp's GR needle ballistics |
| `fit_io_needle.py` | H-Comp's In / Out needle law |
| `pilot_gain.py` | the compressor's gain over time from a pilot tone in a MeterCapture run |
| `pilot_cycle.py` | the gain inside each cycle of a low tone, H-Comp against Flatwater Comp |
| `read_br_film.py` | a film of Bass Rider or Flatwater Bass Leveler as per-frame readings |
| `parse_drags.py` | parses MeterCapture drag logs: each knob's value before and after |
| `parse_drags_any.py` | parses MeterCapture drag logs generically |
| `feel/run_feel.sh` | 50 px drags and 3-notch wheel moves on each control ([feel/README](feel/README)) |
| `feel/scan_thumb.sh` | finds a fader's thumb |
| `feel/fader_feel.sh` | fader drag rates from the default thumbs |
| `feel/ctrl_feel.sh` | 200 px Ctrl drags by mouse step size |
| `feel/midctrl_feel.sh` | Ctrl pressed or released half way through a drag |

## C models and their builds

Each `.c` file is a per-sample model loaded with ctypes; its `build_<name>.bat` compiles it into a git-ignored DLL
with the newest Visual Studio found by vswhere.

| File | What it does |
|---|---|
| `fastdsp.c`, `build_fastdsp.bat` | attack and release follower for `fwbasslift_model.py` (a Python fallback exists) |
| `mbfast.c`, `build_mbfast.bat` | the Flatwater BassX Lift generator loop and its exact linearisation |
| `mbfeat.c`, `build_mbfeat.bat` | the Flatwater BassX Lift training feature helpers |
| `hcfast.c`, `build_hcfast.bat` | H-Comp's identified gain path |
| `hcfast2.c`, `build_hcfast2.bat` | the two-state release law, the plug-in's form |
| `hcfast3.c`, `build_hcfast3.bat` | the release law, third form |
| `hcfast4.c`, `build_hcfast4.bat` | the release law, fourth form |
| `hcfast5.c`, `build_hcfast5.bat` | the release law, fifth form |
| `hcfast6.c`, `build_hcfast6.bat` | the release law, sixth form (learned release-time factor) |
| `hcgrad.c`, `build_hcgrad.bat` | the sixth form with its exact gradient, for training |
| `brmodel.c`, `build_brmodel.bat` | Flatwater Bass Leveler's riding model with every constant a parameter |
| `vrmodel.c`, `build_vrmodel.bat` | Flatwater Vocal Leveler's riding model, the same as the plug-in's |
| `comp.c`, `build_comp.bat` | Flatwater Vocal Chain's Compress model |
| `deess.c`, `build_deess.bat` | Flatwater Vocal Chain's main de-esser model |
