# Flatwater Bass Lift: measurements

The current values of every stage of Flatwater Bass Lift, measured on the original it recreates (Renaissance Bass),
with the method and the residual or confidence of each. The data and plots are in
[measurements/FwBassLift/](../../measurements/FwBassLift). F stands for the Freq setting throughout.

## Setup

| Item | Value |
|---|---|
| Reference | Renaissance Bass (Waves), the RBass Stereo and RBass Mono components, from a licensed install |
| Host | pedalboard loading the reference's VST3 (`scripts/nulltest/refhost.py`, `render_rbass`), 512-sample buffers, 48 kHz |
| Hidden parameters | the parameters the VST3 does not expose (Original Bass In / Out, Response, Ratio, Decay and the rest) are written into the reference's saved state, which holds all its parameter values (`scripts/nulltest/ref_state_lib.py`); the reference ignores some otherwise valid states depending on the payload size, so every write is padded and verified by reading it back; Freq, Intensity and Gain are set as normal parameters; Input (the first in the original's parameter order) stays at 0 when written this way and is not measured |
| Signals | sine tones (component amplitudes and phases by synchronous detection over whole periods), level steps, DC-free parameter sweeps, two-tone kernels, AM and beating tones, kicks, bass notes and a riff |
| Identification scripts | `plugins/FwBassLift/nulltest/measure_generator.py`, `plugins/FwBassLift/nulltest/measure_sweep.py`, `plugins/FwBassLift/nulltest/measure_kernel.py`, `plugins/FwBassLift/nulltest/render_fitdata.py`, `plugins/FwBassLift/nulltest/model_lib.py` (the Python model) |
| Data files | `linear_paths.json`, `k2_grid6.json`, `levels_decay0.json`, `cs_vs_f0.json`, `am70.json`, `beat70.json`, `dyn_env70.json` and others in `measurements/FwBassLift/` |
| Generated part | Intensity is an exact gain on the generated path, so a second render at another Intensity splits the generated part off the reference's output in every null test |

## 1. Path, latency and output trim

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Latency | 0 samples at 44.1, 48, 96 and 192 kHz, reported and measured | click | exact |
| Gain | a pure output trim (0 / -6 / -12 / -30 dB) | tones | exact to 0.01 dB |
| Gain smoothing | an Out Gain change glides in over about 25 ms (50 % at 14 ms, 90 % at 25 ms) | parameter step during a tone | measured |
| Above about 2 kHz | untouched: 0.00 dB at 10 kHz; 0.17 dB at 1 kHz with Freq 160 | tones | measured |

## 2. The original path

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Original Bass Out | a third-order Butterworth high-pass at F; with Freq 80: -24.5 dB at 20 Hz, -10 at 30 Hz, -2.2 at 40 Hz at Intensity 0, and -34.8 / -25.9 / -22.3 dB at Intensity -24 (about 24 dB per octave below Freq once the generated signal is turned down) | tones, fitted against bilinear Butterworth sections (`linear_paths.json`) | -60 to -75 dB against a bilinear HP3 on the renders; a small extra low-frequency gain difference, below -57 dB, is not explained |
| Original Bass In | LP3(F) + HP3(F), a second-order all-pass: the low end passes, with a small dip just below Freq (-3 dB at 30 Hz with Freq 80) where the generated band and the original overlap | tones | as above |

## 3. The generated path

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Low-level response | -C0 \* LP3(F/3) \* HP3(F) \* HP3(F), C0 about 290 to 300 (about +49.5 dB); a resonant boost just above Freq: +13.4 dB at 82 Hz (Freq 80, Intensity 0, -30 dBFS tone), +12.2 dB at 168 Hz (Freq 160); with Intensity -24 it shrinks to +1.6 dB, so it is the generated path, not a fixed EQ | tones over F = 40 to 256 Hz | log-magnitude error 0.0044 rms |
| Harmonic product | a second-order term u \* b followed by HP3(F/3) and HP3(F), with u = LP3(F/3) HP3(F) x and b = LP3(F) x | 190 two-tone kernels on a grid where many pairs share the sum frequency (`plugins/FwBassLift/nulltest/measure_kernel.py`) | free post-filter and factors: 0.04 dB / 0.05 degrees rms; the closed form: 0.085 dB / 0.3 degrees rms |
| Harmonic content | mainly the 2nd and 3rd harmonics, the 4th and 5th 20 to 30 dB lower; for -20 dBFS tones with Freq 80 the 2nd harmonic is -14.8 dB re input at 50 Hz and -29.6 dB at 120 Hz; a strong component at the input frequency itself (+8.4 dB at 80 Hz); content falls away above about 2 x Freq | tones | measured |
| Decay (hidden) | the gain of the harmonic product: Decay 0 removes the even harmonics completely (2nd harmonic -166 dB) and leaves the rest unchanged; it reads back only 0 or 1; level effect -7.4 dB at 0 against -6.6 at 1 (-10 dBFS step) | tones, steps | exact |
| Intensity | an exact gain on the generated path: for a 50 Hz tone the generated fundamental is -23.2 / -1.0 / +12.5 dB re input at Intensity -12 / 0 / +12 | tones | -147 dB as a gain, until the generated limiter (section 5) |
| Response (hidden) | no measurable effect on these tests | 0 / 5 / 15 / 30 ms | within 0.001 dB |
| Stereo | the generated bass is mono, from the sum of both channels, added to both: with signal only on the left the right output carries the generated part at -29 dB; a loud left channel lowers the generated level of a quiet right channel (2nd harmonic -74.6 -> -92.7 dB), so the level control is shared | tones per channel | measured |

## 4. Gain control of the generated path

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Level dependence | strongly non-linear: the generated harmonics grow about 2 dB per dB at low level (-99.8 dB at -60 dBFS, -76.7 at -48, -55.8 at -36) and level off towards 1 dB per dB at high level (-25.6 at -12, -14.9 at 0 dBFS); the generated fundamental is compressed (-52.8 dB at -60 dBFS, -18.0 at -12, -19.0 at 0 dBFS) | tone levels | measured |
| Ratio (hidden) | 1.0 and 0.63 give identical output in the identification renders; below about 0.5 the compression gets stronger (0.25: -5.95 dB at 0 dBFS); the first step battery read a steady gain of -24.4 / -6.6 / -5.1 dB for Ratio 0.25 / 0.63 / 1.0 (-10 dBFS step); Flatwater Bass Lift keeps the parameter (the presets use 0.63) and does not use it | steps, sweeps | measured |
| Where the gain acts | mostly after the first HP3(F), with about 5 % of it (in dB) ahead of it (`alpha`); a gain fully ahead of the high-passes gives the right 3rd harmonic on low notes but the wrong one near F, and the reverse when fully after | a free gain trajectory solved under each placement (`plugins/FwBassLift/nulltest/fit_gain_trace.py`), complete models fitted | fitted |
| Dynamics | a second-order, asymmetric response: about 10 ms of delay, then the gain settles within 30 to 40 ms after a rise and more slowly after a fall, without overshoot; seen on the output, the generated part follows a step within about 50 to 100 ms and after a step down overshoots briefly, settling within about 100 ms; modelled as two attack / release followers in cascade | level steps of a 70 Hz tone, the ratio of Hilbert envelopes (`dyn_env70.json`) | fitted |
| Ripple | the gain over one period is a sawtooth locked to the zero crossings of LP3(F/3) x: it jumps at each zero crossing and slides down through the half-cycle; the first follower reproduces it by falling almost at once and rising slowly | recovered gain per period | fitted |
| Level band | a resonant low-pass near F (LP2 at 1.0 F, Q 1.35, times LP1 at 10 F): compression is strongest for notes just below F and lighter above it | `cs_vs_f0.json` | fitted |
| Freq dependence | the follower times scale as (80/F)^0.34 and (80/F)^0.19 | the four Freq settings 40 / 80 / 160 / 256 fitted together | fitted |
| Two level detectors | CS = C0 (1 + E/E0)^-p (1 + EL/El0)^-pL, EL an attack / release follower on abs(l): a wide detector and a second gain stage reading l itself; its ripple makes the 3rd harmonic of low notes, locked to the zero crossings of l | pinning test (`plugins/FwBassLift/nulltest/render_pin.py`): a low carrier (15 to 40 Hz) plus a strong tone at 157 to 220 Hz that the wide detector sees and the l band does not; the pinning tone lowers the overall gain, but the carrier's 3rd harmonic hardly changes (20 Hz at -26 dB: +4.5 -> +1.7 dB re h1 with the pinning tone at -3 dBFS) where a single detector predicts a collapse to -8 dB | measured |
| The l-driven stage alone | with the wide detector pinned by a -3 dBFS tone near 200 Hz, the gain over one period of l is a sawtooth that jumps up at each zero crossing and slides down through the half-cycle, deeper for lower notes (+-50 % at 10 Hz); fitted alone it is a follower with a slower attack (14 ms) than release (3.7 ms) on abs(l) | `plugins/FwBassLift/nulltest/render_pinned.py`, `plugins/FwBassLift/nulltest/fit_lstage.py` | h1 within +-1 dB, h3 -8 to +4 dB, except the lowest notes (10 to 12 Hz, 6 to 10 dB weak); in the combined fit the waveform data keep this stage small, so its shape is not settled |
| Kicks at high Freq | the reference lets the first generated peak of a kick through about 3 dB higher at Freq 160 / 256 (its gain reacts later there) | kicks | open |

Method notes: the analytical gain control was fitted on time-domain renders across the four Freq settings, on
transients (kicks, bass notes and a riff at Freq 80 / 160 / 256, each against the reference's unlimited generated
signal, `plugins/FwBassLift/nulltest/render_transients.py`) and on the steady harmonic and pinning sets together (`plugins/FwBassLift/nulltest/fit_combo.py`).
It is a fitted model, not an identified one: the exact detectors, the 3rd harmonic of 40 Hz notes (4 to 10 dB weak
when pinned) and of the lowest notes (10 to 12 Hz), and kicks at Freq 160 / 256 (-11 / -7 dB on the unlimited
generated signal) stay open. The plug-in replaces it with the learned block of section 7.

## 5. The generated-signal limiter

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Threshold | Intensity is an exact gain on the generated part until it peaks near 0.7; above that a limiter on the generated signal alone holds it under 0.703 (the product's clip-free stage) | the reference's own unlimited signal (its Intensity -24 render scaled up, which never reaches the limiter) against its limited one | measured |
| Timing | the gain drops at once to what each sample needs to stay under 0.703 and recovers as a one-pole in the gain domain with 28.4 ms | the same | median -42 dB, kicks -32 to -43 dB |

## 6. The output stage

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Clip | a hard clip to the 16-bit range, -1 to 32767/32768, no knee and no time constant: the gain drops instantly on a +26 dB step and returns instantly | loud 1 kHz tones, where nothing is generated | exact |
| Ceiling | the output never exceeds 0 dBFS: inputs of 0 to +6 dBFS with Intensity +24 all peak at 0.0 dBFS | tones | exact |

## 7. Learned block

The plug-in keeps the identified structure exact (filters, the gain ahead of and after the first high-pass, the
harmonic product, the limiter, the clip) and learns only the level-to-gain block.

| Item | Value |
|---|---|
| Structure | a GRU with 32 units; inputs abs(x), abs(b), abs(l), abs(u) (compressed with asinh) and ln(Freq/80); outputs ln g_pre and ln g_post, sample by sample; magnitudes only, so the gain is symmetric and Decay 0 leaves no even harmonics, as in the reference |
| Target | the exact generated part of the reference |
| Dataset | 900 reference renders (tones, steps, AM, two-tones, noise, kicks, bass lines, pinned pairs, sweeps; Freq 32 to 256; Decay 0 / 1), `plugins/FwBassLift/nulltest/render_trainset.py`; trained with PyTorch (`plugins/FwBassLift/nulltest/train_net.py`), exported by `plugins/FwBassLift/scripts/export_net.py` into `plugins/FwBassLift/Source/dsp/GainNet.h` |
| Held-out result | 80 held-out renders: median -37.7 dB (the fitted analytical model -25.3 dB) |
| Rates | the network runs on its own 48 kHz clock (the rate it was trained at) at any host rate: the null at 44.1 and 96 kHz is within 0.2 dB of 48 kHz |
| C++ against PyTorch | -74 to -91 dB |

It is a learned model of the reference's gain control, not an identification of its algorithm.

## 8. The model against the reference

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Whole output | median -36.8 dB | 12 signals x 8 settings, [NULLTESTS.md](NULLTESTS.md) (`plugins/FwBassLift/nulltest/bf_ref.py`) | a model, not a null |
| Generated part alone | median -31.5 dB; by setting -34 to -36 dB at Freq 32 / 40, -30 to -31 at Freq 80, -26 at Freq 160, -17 at Freq 256 (whose worst cases are very low notes with a generated part 38 dB under the output) | the same | a model |
| Weakest signals | kick-like transients (-22 dB) and a 30 Hz tone (-22 dB) | the same | |

## 9. Declined changes

| Change | Measured effect |
|---|---|
| The fitted analytical gain control (section 4) in place of the learned block | -28.9 dB on the whole output, -23.2 on the generated part (the learned block: -36.8 / -31.5); held out -25.3 against -37.7 |
| A fit to the harmonic sets alone | reproduces the pinning behaviour well, fails on everything else (null median -12.7 dB) |
| A level-domain release for the generated limiter | -22 dB against -42 for the gain-domain one-pole |
| A plain delay of the level, for kicks at high Freq | everything worse |
| Freq-dependent attack times (fitted: slower at high Freq) | only a slight improvement on kicks at Freq 160 / 256 |

## 10. Meters

The reference's meters cannot be read through the plug-in interface, so they were filmed: `tools/MeterCapture` hosts
the plug-in, plays a test signal through it in real time, records its output, and grabs the editor about 40 times a
second; `scripts/nulltest/read_meter_frames.py` turns the frames into bar heights (numbers in
[measurements/FwBassLift/meters](../../measurements/FwBassLift/meters); the screenshots stay out of the repository).
Signals: tone bursts and level steps at 25 / 50 / 300 / 1000 Hz, 5 / 20 / 100 ms bursts (`scripts/nulltest/render_meter_probe.py`),
a bass line and kicks. The same law drives every Flatwater meter (`shared/ui/Widgets.h`).

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Law | each frame takes the highest peak of the last 17 ms or so; the bar jumps up to it at once and otherwise moves toward it as a one-pole on linear amplitude, time constant 0.325 s; in silence a straight 26.7 dB/s fall (the same on all three meters, from any level); with the signal still playing, a drop to a lower level settles gently (-3.3 to -8.7 dB takes about a second), and an onset peak above a steady tone eases back over about 1 s | `scripts/nulltest/fit_meter.py` on the output meter against the recorded audio | 1.1 px rms (0.2 dB) over the whole probe |
| Scale | 5.17 px per dB, 0 dB at the top of a 155 px track (30 dB), on all three meters | frames | measured |
| Readouts | hold the highest peak until clicked | frames | measured |
| On music | the bars agree with the reference within 3.5 px (0.7 dB) on 90 % of the frames | the bass line, kicks | measured |
| Not matched | the Original Bass bar sometimes drops at once to a lower level after a long sustained tone (a 2 s tone stopping) instead of easing down; Flatwater Bass Lift eases down there too | frames | open |
| Implementation | the audio thread only collects peaks; the editor applies the law at 60 frames per second with the real time between frames, so the movement does not depend on the host's buffer size | | |

The same films of the original Flatwater Vocal Comp recreates (Compression -12 dB, the same probe and bass-line
signals; bar heights in [measurements/FwVocalComp/meters](../../measurements/FwVocalComp/meters)):

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Output meter | 36 dB over the 155 px track, 4.3 px/dB; the same law, a one-pole on linear amplitude with 0.325 s (26.7 dB/s in silence) | the bass-line capture | 6 px rms, most frames within 1 to 2 px |
| Energy meter | the input level, 80 dB over the track, 1.97 px/dB; two such stages in a row, 0.341 s then 0.094 s; the second stage makes the fall start gently | `plugins/FwVocalComp/nulltest/fit_meter_energy.py` | 2.2 px rms |
| Gain reduction meter | the compressor's reduction itself with no meter smoothing (a 5 ms burst shows its full reduction for one frame and is gone in the next); Flatwater Vocal Comp shows the deepest reduction since the last frame | bursts | measured |
| Not matched | after a steady test tone that stops or steps down, the reference's output bar sometimes snaps down at once instead of easing (as the Original Bass meter above); no rule tried in `plugins/FwVocalComp/nulltest/fit_meter_output.py` (a falling line with several hold rules, updates only on change) explains it; it does not show on the bass line | frames | open |
| Flatwater Vocal Comp on the bass line | Energy, Output and Gain reduction within 2.0, 2.3 and 2.0 px on 90 % of the frames | the bass-line capture | measured |

## Null-test results

The generated [NULLTESTS.md](NULLTESTS.md) in brief; the residual is
`20 log10(rms(ours - reference) / rms(reference))` in dB.

`plugins/FwBassLift/nulltest/bf_ref.py`, 12 signals x 8 settings (Freq, Intensity, Gain, Original Bass In / Out): median
-36.8 dB on the whole output and -31.5 dB on the created harmonics alone.

- Per signal (median over the settings): tones at or above Freq, sweeps, white noise -43 to -49 dB; bass notes, level
  steps, pink noise and tones below Freq -31 to -40 dB; kick-like transients -28 dB.
- Worst cases: Freq 160 with Original Bass Out on very low input (30 Hz tone -4 dB, kicks -9 dB, 60 Hz tone -16 dB):
  the output is then only the created bass, and its level is off there.
- How: the filters and the harmonic generator were identified exactly; the gain control of the created bass is a small
  recurrent network (32 units, about 6 % of one core) trained on renders of the reference, so it is a learned model of
  that part, not its algorithm. Meters move like the reference's (measured from its editor).
- Every case: [NULLTESTS.md](NULLTESTS.md).

## Validation

| Check | Result |
|---|---|
| CHECK sites | 28 |
| `ctest` | pass |
| pluginval at strictness 10 | pass |
| FocusCheck | pass |
| VST3 equals renderer (48 / 96 kHz) | not recorded |
| Render regression cases | 3 |
| CPU per 60 s of stereo at 48 kHz | the learned block about 3.6 s (6 %) |
| The learned gain block, C++ against PyTorch on held-out renders | -74 ... -91 dB |
| Null at 44.1 / 96 kHz against 48 kHz (5 signals) | within 0.2 dB |
| Meters (`tools/MeterCapture`, bass line and kicks) | Original Bass within 3.5 px (0.7 dB) on 90 % of frames; output and harmonics within 8 px, the rest being the generated level |
| Unit tests cover | original path In and Out, latency 0, Intensity exact on the generated part, low-level generated gain, Decay switching the harmonic product, output clip, Gain trim, block-size invariance, silence, stereo, processor parameters and state round trip |

The commands: `ctest --test-dir build -C Release --output-on-failure`; pluginval 1.0.4 at strictness 10 as the
release workflow runs it; `FocusCheck <plug-in>.vst3`; `python nulltest/nulltest.py --vst3` (the built VST3 against
the offline renderer); `python scripts/render_regress.py --build build`; CPU: the offline renderer on 60 s of stereo
at 48 kHz.

## CPU

| Configuration | Cost |
|---|---|
| Stereo in real time, with the learned block | about 6 % of one core |

## Scripts that replace earlier ones

Every script named on this page by its current name; the earlier names stay in the history.

| Script | Replaces |
|---|---|
| `nulltest/fit_combo.py` | the combined fit whose constants ship; the earlier joint multi-Freq fit (fit_multif2.py) is removed |
| `nulltest/measure_kernel.py` | the two-tone second-order kernels (measure_k2.py) |
| `nulltest/fit_gain_trace.py` | the generated path's gain over time by least squares (extract_cs.py) |
| `nulltest/bf_ref.py` | the harness and the null test of the settings (null_fwbasslift.py) |

## Provenance

The reference is the original Renaissance Bass, run from a licensed install.

- Measured: renders of the reference's output (tones, two-tone kernels, level steps, bass material) at its settings,
  through its VST3 in a host; films of its meters.
- Fitted or learned from the reference's output: the filter and harmonic structure identified from those renders; a
  small learned gain block (our own weights, `plugins/FwBassLift/Source/dsp/GainNet.h`) trained on captures of its
  output; the meter law.
- Redrawn: the editor, drawn in code in Flatwater's own dark look (`shared/ui/Theme.h`) on the original's
  measured layout (the positions and sizes of the controls), with our own colours and shading; no table of the
  original's pixels ships.
- Presets: the original's factory preset values under our own names and groups, the suite's standing rule; the
  preset files themselves are not copied.
- Shipped on recorded approval: none beyond the above.
- None of the original's code, artwork, preset files, fonts or other files ships with Flatwater Bass Lift.
