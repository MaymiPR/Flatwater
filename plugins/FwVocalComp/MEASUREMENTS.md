# Flatwater Vocal Comp: measurements

The current values of every stage of Flatwater Vocal Comp, measured on the original it recreates (Renaissance Vox),
with the method and the residual or confidence of each. The data (JSON, CSV tables, plots) is in
[measurements/FwVocalComp/](../../measurements/FwVocalComp); large raw arrays (`.npz`) are left out and regenerate
with the scripts.

## Setup

| Item | Value |
|---|---|
| Reference | Renaissance Vox (Waves), RVox Mono and RVox Stereo components, from a licensed install |
| Host | `pedalboard` 0.9.26 through `scripts/nulltest/refhost.py`, 512-sample buffers, 48 kHz unless noted |
| Fixed settings | Gate off (`-Inf`), Gain 0 unless noted |
| Signals | 1 kHz sine bursts (steady state read from the last 0.4 s), 12 kHz sine for sub-millisecond envelopes, step signals, square and noise, impulses, DC level steps, isolated sample patterns |
| Latency compensation | pedalboard removes the reported latency, so impulse offsets are 0 by construction; the number compared is the reported latency |
| Host defect | about 12 % of renders return with a block of silence or shifted by up to 0.8 s (a buffer dropped between the host and the reference); `refhost.render` detects silence where the input is not silent and renders again, after which 120 repeats of one setting agree to the 0.01 dB printed (without the re-render the first static sweep had random dips of up to 300 dB) |
| Identification scripts | `plugins/FwVocalComp/nulltest/probe_*.py` (probes) and `plugins/FwVocalComp/nulltest/fit_*.py` (fits), Python 3.12 or newer with the packages in `requirements.txt` |

## 1. Latency, parameter maps and fixed gain

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Reported latency | 64 samples at 44.1 and 48 kHz, 128 at 88.2 and 96 kHz, 256 at 176.4 and 192 kHz | the host's latency report per rate | exact; the same figures as the product page |
| Compression map | raw 0 ... 1 to -36 ... 0 dB, linear | parameter text read back | exact |
| Gain map | raw 0 ... 1 to -36 ... 0 dB, linear | parameter text read back | exact |
| Gate map | raw 0 ... 1 to -80 ... 0 dB, linear; raw 0 shows `-Inf dB` | parameter text read back | exact |
| Fixed gain | -0.0873 dB (x 0.99) at Compression 0 and Gain 0, at every level and frequency, no distortion | 1 kHz tones | exact |
| Gain control | a pure output trim: output peak = (level at Gain 0) + Gain; 0 dBFS in gives -12.09 dBFS out at Gain -12 | tones at several levels | exact |
| Ceiling | none: at Compression 0, +12 dBFS in gives +11.91 dBFS out at every Gain setting, and a 2.0 impulse train comes out at 1.98 | over-full-scale tones and impulses | exact (the manual's limiter ceiling is not found) |
| Parameter smoothing | Compression and Gain changes take effect within one 0.167 ms envelope window of the block boundary (no ramp); a Gate change glides in about 1.3 ms (10 % at 0.3 ms, 99 % at 1.3 ms) | a 12 kHz sine, the parameter stepped between blocks | exact |

Method notes: every item is rendered through `refhost.render`; the 12 kHz sine gives a 0.167 ms envelope resolution.

## 2. Detector model

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Level read | a four-phase true-peak detector: the largest of abs(x) and the outputs of three parallel first-order all-passes (a + z^-1) / (1 + a z^-1) with a = 0.1607, 0.3569, 0.6308 (fractional delays of about 3/4, 1/2 and 1/4 sample) | isolated patterns of 1 to 8 samples in silence (`plugins/FwVocalComp/nulltest/probe_kernel.py`), 190 patterns | fit error 0.0006 in amplitude (0.17 for the sample peak, 0.05 for a fitted interpolation kernel) |
| Consequences | steady sines of any frequency and phase read as their true peak; square, saw and pulse signals read up to 1.5 times their sample peak; a single-sample impulse reads exactly 1.0 | the same patterns | exact |
| RMS-like readings on dense signals | a square wave and a sine of equal RMS get gains of 15.8 and 16.0 dB; of equal peak 13.9 against 16.0 dB; noise of the same RMS gets 10.1 dB (a 13 dB crest factor) | square, sine and noise at Compression -24 | measured; explained by the true-peak read plus the slow path of section 5 |
| Stereo | one gain for both channels, driven by the louder one: L -40 dB with R -10 dB gives +8.80 dB on each, L alone +22.71 dB; the Mono and Stereo components give the same gain on identical input; checked with unequal and alternating channels | unequal channels | exact |

Method notes: `plugins/FwVocalComp/nulltest/probe_kernel.py` fits the detector on raw sample patterns; the stereo checks render unequal and alternating channels through `refhost.render`.

## 3. Look-ahead

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Audio delay | 64 samples (the reported latency); the lengths below scale with it at 88.2 to 192 kHz | the latency report, impulse offsets | exact |
| Gain hold | the instant gain of each sample held as a minimum over 65 samples | `plugins/FwVocalComp/nulltest/probe_ramp.py`, `plugins/FwVocalComp/nulltest/fit_gain_hold.py` | error 0.05 dB around single-sample and 24-sample events |
| Smoothing | two moving averages of 39 and 26 samples in the linear gain domain | the same | the same; the residuals do not depend on the sample rate |
| Attack as seen from outside | the gain moves ahead of the input step, 0.3 ms before it, and is complete within 0.3 ms; a 1 ms tone burst already gets the full steady-state gain (+8.80 dB at -10 dBFS, Compression -24) | 12 kHz sine steps and bursts | exact |

Method notes: the look-ahead reads as an attack 0.3 ms before the step because the gain of a future sample is applied through the 64-sample delay.

## 4. Static law (the compressor)

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Gain of a steady sine of peak amplitude x | gain = 0.99 M / (1 + (M - 1) x) with M = 10^(-Compression / 20): a gain that falls with the detected amplitude along a hyperbola, no separate threshold, ratio or knee | a grid of Compression 0 ... -36 dB in 1 dB steps and a 1 kHz sine from -100 to +12 dBFS in 1 dB steps, 4181 cells | median error 0.00002 dB, 99th percentile 0.0005 dB; 13 of 4181 cells differ by more than 0.05 dB (host dropouts and -100 dB tones) |
| Endpoints | out = x + G(x, c) with G(-80 dBFS) = -c - 0.09 dB (full make-up) and G(0 dBFS) = -0.09 dB for every Compression c | 1 kHz sines at peak level x | exact |
| Effective slope at -10 dBFS | 0.76 / 0.52 / 0.31 / 0.18 / 0.09 / 0.05 for c = -6 ... -36 in steps of 6 | `static_summary` in the data | measured |
| Level where the gain is 3 dB below its low-level value | c - 6 dB for c <= -18; -6 at c = -6, -16 at c = -12 (2 dB resolution) | the static sweep | measured |
| Knee | no sharp knee at any setting; the slope falls smoothly from 1 to its 0 dBFS value over about 50 dB | the static sweep | measured |
| Frequency response | flat, +-0.0 dB, 20 Hz ... 20 kHz at Compression 0; at Compression -24 the gain is 0.04 dB higher at 20 Hz at -50 dB and 0.28 dB higher at 20 Hz at -20 dB (the detector follows the waveform at very low frequency) | sine sweeps at two levels | measured |
| THD | below -147 dB for 1 kHz at -40 ... 0 dBFS in all four settings tried (the numerical noise floor; the gain is applied smoothly) | 1 kHz tones | measured |

Method notes: the closed form replaces an interpolated table of `static_gain.csv`; the grid is rendered through `refhost.render` and the closed form is fitted to it.

## 5. Slow path

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Release from outside | slow, exponential-like and program dependent: time to 63 % of the move 424 ms (2.9 dB move), 384 ms (5.6 dB), 290 ms (14.5 dB), 240 ms (24.7 dB); 90 % in 970 / 920 / 767 / 634 ms | tone bursts | measured |
| Reduction left after a burst | a burst after silence gets its full reduction at once; the reduction left after it grows with its length: 2 ms at -10 dB leaves 0.07 dB, 100 ms 3.2 dB, 400 ms 14.6 dB of 15.0 dB | bursts of several lengths | measured |
| Level-dependent onset | a step from -40 to -10 dB starts to charge after 5 ms, to -20 dB after 8 ms, to -30 dB after about 25 ms | level steps on a DC carrier (`plugins/FwVocalComp/nulltest/probe_slow_dc.py`) | measured |
| Fitted model | a slow level path in dB: the peak over a 16 ms window, a one-pole stage (rise 35 ms, fall 0.75 ms), a second stage with a 0.05 dB dead band (rise 68 ms, fall 488 ms); the applied gain is the smaller of the fast and the slow gain | constants from the DC steps, refined on the whole test set (`plugins/FwVocalComp/nulltest/fit_smoothing.py`) | 0.56 dB on the DC steps; fitted, not identified |
| Open | the reference reads Gaussian noise as about 3.5 to 3.8 times its RMS (a peak-like level over a window of several milliseconds); the best window for noise (about 10 ms at -20 dB, 4 ms at -30 dB) disagrees with the one for tone steps (about 1 to 3 ms); three smoothing structures (the gain, the amplitude level, the dB level) stop at the same score, so the missing piece is how the live level is formed over a few milliseconds, or a stage not found | `plugins/FwVocalComp/nulltest/fit_smoothing.py` | the fitted time constants are the best fit over its signals, not measurements |

Method notes: the fast path (sections 2 to 4) is exact; the slow path is the one fitted part, and the learned block of section 6 corrects what the fit misses.

## 6. Learned slow path

The reference's own gain reads sample by sample with the gate off (output / delayed input / fixed gain). Against it the
identified fast path is an upper bound (the reference gain is above it on 0.0 to 0.1 % of the samples), and where the
two differ the reference compresses more than the fitted slow path: on dense noise at heavy compression it holds an
almost constant gain 2 to 3 dB below even the fast ratio (the gain of the larger peaks over a long window); on a 100 Hz
tone it holds the fast ratio's value at the waveform peak with 0.02 dB ripple, where the fitted path sits 0.5 dB too
high. A plain peak hold of the fast ratio fits the low tone but loses 10 to 20 dB elsewhere, so the slow path is left
to a small learned block. It is a learned model
of the reference's slow path, not an identification of it.

| Item | Value |
|---|---|
| Structure around the block | slow = max(fitted slow ratio x exp(c), floor); gain = min(fast, slow); every other stage exact |
| Network | a GRU of 48 units stepped every 1/12000 s (4 samples at 48 kHz, a phase accumulator at other rates); inputs per step: ln of the step's peak level, the smallest fast ratio and the last fitted slow ratio, ln M, running minima of the fast ratio over 1 to 20 ms, running maxima of the level over 10 to 300 ms; c applies to the following samples, moving linearly between the last two finished steps (causal) |
| Guards | the floor is the static-law ratio at the loudest level of the last second, so a steady signal is never compressed below its fast ratio; while the fast ratio stays flat for 20 ms (a steady tone) the correction fades out over 20 ms; without them the network cost 10 to 60 dB on steady sines and level steps the fitted path already nulled |
| Target | the reference's sample-wise gain, then the null residual itself |
| Dataset | 3600 random 2 s renders of the reference's mono component (voice, speech-like noise, white and pink noise, tones 25 Hz to 10 kHz, steps, AM, sweeps, drums, mixes; peaks -60 ... 0 dBFS; Compression 0 ... -36 dB, gate off), the plug-in's internals dumped by `VcRender dump=1` (`plugins/FwVocalComp/nulltest/render_trainset.py`) |
| Training | `plugins/FwVocalComp/nulltest/train_net.py`: supervised on the gain first, then fine-tuned on the null residual with the compression settings balanced; training on the residual alone does not move (the slow ratio gets no gradient while it is above the fast ratio) |
| Export | `plugins/FwVocalComp/scripts/export_net.py` to `plugins/FwVocalComp/Source/dsp/Net.h` |
| Result, settings grid (672 cases, 48 kHz) | worst -10.1 dB without the net, -22.8 dB with it; median -71.4 and -70.2 dB; 396 cases below -60 dB either way ([the settings grid](NULLTESTS.md#grid)) |
| Result, null-test set | worst -9.3 to -19.3 dB; 44 of 117 rows better by more than 1 dB, 4 worse ([NULLTESTS.md](NULLTESTS.md)) |
| Per signal, Compression -24 / -36, without and with the net | white and pink noise -13 / -10 to -27 / -24 dB (the same at 44.1 to 192 kHz and in stereo); voice-like -31 / -21 to -40 / -30 dB; 100 Hz tone -30 / -24 to -48 / -51 dB; sweeps -36 / -29 to -49 / -48 dB |
| Cases worse with the net | AM at Compression -12 (-59 to -49 dB); level steps at light compression (grid median at -6 dB: -105 to -88 dB); steady tones unchanged |
| C++ equals PyTorch | -70 dB or lower on the null-test signals |
| Other rates | the gains at 44.1 and 96 kHz equal those at 48 kHz |
| CPU | about 3.5 % of one core for linked stereo (twice that in dual mono) |

## 7. Gate

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Law | a downward expander with a hard knee, 2:1: gain = (level - threshold) below the threshold, 0 dB above; gate -40 dB gives -2.09 dB at -42 dB and -0.59 dB at -40.5 dB | tones stepped in 0.5 dB around the threshold | exact to 0.01 dB (the manual says soft knee) |
| Level it reads | the peak level of the signal, rising at once and falling toward the live level as a one-pole in linear amplitude with 130 ms (the default of the hidden gate-release parameter) | DC level steps across the threshold (`plugins/FwVocalComp/nulltest/probe_gate.py`) | 0.0004 dB rms, worst 0.09 dB |
| Release from outside | after a 20 dB downward step with the gate at -30 dB: 50 % at 479 ms, 63 % at 531 ms, 90 % at 666 ms | level steps | measured; follows from the 130 ms one-pole |
| Path | the gate gain passes through the same two moving averages as the look-ahead, so it opens just ahead of a rise; the gate and the compressor detect the input in parallel and their gains multiply | steady tones below the gate threshold at Compression -12 and -36 | within 0.01 dB |
| Hidden release | the gate-release parameter is not exposed by the VST3, so values other than its default are not separated from the fixed behaviour | | open |

Method notes: `plugins/FwVocalComp/nulltest/probe_gate.py` fits the gate on DC steps; the parallel structure is checked on tones under the threshold.

## 8. Not measured, and why

- The gain-reduction meter's ballistics and the Energy and Output meter values: the VST3 exposes only Bypass,
  Compression, Gate, Gain (plus generic MIDI controllers), no meter parameters, so the meters are compared on the
  editor, not through a host (the meter laws: [Flatwater Bass Lift's MEASUREMENTS.md, section
  10](../FwBassLift/MEASUREMENTS.md#10-meters); the comparison: [Validation](#validation)).
- Transients with the compressor and the limiter engaged together: only steady tones were driven through both.
- Real dry vocal stems: every voice signal in the null tests is synthetic.

## 9. Differences from the product documentation

1. The manual calls the Gain control a limiter ceiling; measured, it is an exact post-trim and nothing limits at
   Compression 0.
2. The manual and the product page call the gate knee soft; measured, it is a hard-knee 2:1 expander on the peak
   level of the signal.

## Null-test results

The generated [NULLTESTS.md](NULLTESTS.md) in brief; the residual is
`20 log10(rms(ours - reference) / rms(reference))` in dB.

`plugins/FwVocalComp/nulltest/vc_ref.py`:

| Set | Result |
|---|---|
| Steady tones, any setting (Compression, Gate, Gain, input level, 44.1 to 192 kHz, mono and stereo) | -95 to -120 dB (a null) |
| Compression 0 with any Gate and Gain | -120 dB and below (a null) |
| Level steps, sweeps, speech-like material | -30 to -65 dB, worse at heavy compression |
| Dense noise at heavy compression | -24 to -33 dB (-10 to -25 dB without the learned slow path) |
| The 672-case settings grid | median -70 dB, worst -23 dB |

The compressor's slow path is the fitted model plus a small learned correction (trained on renders of the reference,
guarded so steady signals stay exact). Every case: [NULLTESTS.md, Grid](NULLTESTS.md#grid),
[NULLTESTS.md](NULLTESTS.md).

## Validation

| Check | Result |
|---|---|
| CHECK sites | 39 |
| `ctest` | pass |
| pluginval at strictness 10 | pass |
| FocusCheck | pass |
| VST3 equals renderer (48 / 96 kHz) | not recorded |
| Render regression cases | 4 |
| CPU per 60 s of stereo at 48 kHz | about 2.1 s (3.5 % of one core, the learned slow path on) |
| The learned slow path, C++ against PyTorch on the null-test signals | -70 dB or lower |
| The learned slow path at 44.1 / 96 kHz (5 signals x 3 settings) | the same gains as at 48 kHz; steady tones unchanged |
| Meters (`tools/MeterCapture`, bass line and kicks, Compression -12 dB) | Energy, Output and Gain reduction within 2.0 / 2.3 / 2.0 px on 90 % of frames |
| Unit tests cover | latency, static law, gate, gate and compressor together, attack and release, finite output on extreme input, block-size invariance, stereo link, processor parameters, state round trip, Mono mode, factory presets, the learned slow path leaving steady tones and Compression 0 exactly as without it |

The commands: `ctest --test-dir build -C Release --output-on-failure`; pluginval 1.0.4 at strictness 10 as the
release workflow runs it; `FocusCheck <plug-in>.vst3`; `python nulltest/nulltest.py --vst3` (the built VST3 against
the offline renderer); `python scripts/render_regress.py --build build`; CPU: the offline renderer on 60 s of stereo
at 48 kHz.

## CPU

| Configuration | Cost |
|---|---|
| Linked stereo at 48 kHz, the learned slow path on | about 3.5 % of one core |
| Dual mono | about twice that |

## Scripts that replace earlier ones

Every script named on this page by its current name; the earlier names stay in the history.

| Script | Replaces |
|---|---|
| `nulltest/fit_gain_hold.py` | the gain hold's length and smoothing (fit_ramp2.py) |
| `nulltest/vc_ref.py` | the harness and the null test of the settings (null_fwvocalcomp.py) |
| `nulltest/nulltest.py --vst3` | the VST3-equals-renderer check (check_vst3.py) |

## Provenance

The reference is the original named in Setup, run from a licensed install.

- Measured: renders of the reference's output (tones, level steps, noise, synthetic voice) at its settings and sample
  rates, through its VST3 in a host.
- Fitted or learned from the reference's output: the detector, the static gain law, the look-ahead and the gate
  identified from those renders (sections 2 to 4 and 7); the slow path fitted (section 5); a small recurrent net (our
  own weights, `plugins/FwVocalComp/Source/dsp/Net.h`) trained on captures of its output for the slow part of the
  level control (section 6).
- Redrawn: the editor, drawn in code in Flatwater's own dark look (`shared/ui/Theme.h`) on the original's
  measured layout (the positions and sizes of the controls), with our own colours and shading; no table of the
  original's pixels ships.
- Presets: the original's factory preset values under our own names and groups, the suite's standing rule; the
  preset files themselves are not copied.
- Shipped on recorded approval: none beyond the above.
- None of the original's code, artwork, preset files, fonts or other files ships with Flatwater Vocal Comp.
