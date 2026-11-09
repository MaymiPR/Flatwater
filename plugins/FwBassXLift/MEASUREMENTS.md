# Flatwater BassX Lift: measurements

The current values of every stage of Flatwater BassX Lift, measured on the original it recreates (MaxxBass), with the
method and the residual or confidence of each. The data and plots are in
[measurements/FwBassXLift/](../../measurements/FwBassXLift). F stands for the Freq setting and D for 10^(Decay/20);
G is the generated part, what the Harmonics monitor plays.

## Setup

| Item | Value |
|---|---|
| Reference | MaxxBass (Waves), the MaxxBass Stereo and MaxxBass Mono components, from a licensed install |
| Host | pedalboard loading the reference's VST3, 512-sample buffers, 48 kHz; every control is a VST3 parameter, so `scripts/nulltest/refhost.py` `render_mb` sets them all (the rest at their defaults) and no state writing is needed; `python -I nulltest/measure_maxxbass.py` reproduces the first battery |
| Analysis | sine tones by synchronous detection over whole periods; level steps by the Hilbert envelope; the linear path by impulse responses |
| Identification scripts | `plugins/FwBassXLift/nulltest/measure_linear.py` (small-signal response), `plugins/FwBassXLift/nulltest/measure_k2.py` (two-tone kernels), `plugins/FwBassXLift/nulltest/extract_g2.py` / `plugins/FwBassXLift/nulltest/extract_g3.py` (the gain over time), `plugins/FwBassXLift/nulltest/model.py` (the Python model), `plugins/FwBassXLift/nulltest/render_trainset_extract.py`, `plugins/FwBassXLift/nulltest/train_net.py` |
| Data files | `g_small_signal.json`, `k_out_table.json`, `static_gain_50hz.json`, `harmonics.csv`, `level.csv`, `level_ratio.png`, `dynamics_steps.png`, `recovery_F80.json` and others in `measurements/FwBassXLift/` |

## 1. Structure, latency and the faders

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Latency | 0 samples at 44.1, 48, 96 and 192 kHz, reported and measured, with Harmonics off and on | click | exact |
| Structure | Audio = HP3(F) x + OrigBass LP3(F) x + Harmonics G(x); G does not depend on either fader | tones, impulses | exact to -148 to -152 dB; G against the faders -277 dB |
| Channels | each channel is processed on its own: a left-only input leaves the right output silent (-300 dB); a loud left channel does not change a quiet right channel's G (-47.98 dB both ways, h2 -90.2 both ways); Mono equals each Stereo channel (-274 dB); unlike the Bass Lift reference, whose generated bass is a shared mono sum | tones per channel | exact |
| Input | a plain trim up to 0 dB; +6 and +12 act as 0 dB, as Flatwater BassX Lift's fader does; G follows it non-linearly (-6 dB input gives -4.4 dB G) | tones | measured |
| Monitors | Original Bass plays LP3(F) x and Harmonics plays G, both pre-fader (not changed by the Original Bass or Harmonics faders) | tones | exact |
| Harmonics fader | an exact gain on G up to +10.96 dB; settings above give the same output (+11 and +12 give +10.96) | tones | exact |
| Smoothing | none: Input, Original Bass and Harmonics changes take effect at the block boundary; a Freq change settles within about 4 ms (filter state); Input shows a 2 % settling of G over 5 ms (its dynamics) | parameter steps | measured |
| Output clip | a hard clip at exactly +-1.0 on the final output; G alone is never limited (the Bass Lift reference clips at 32767/32768) | loud tones | -89.5 dB against an ideal clip of the unclipped output |

## 2. The original path

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Crossover | HP3(F) and LP3(F) are bilinear Butterworth filters, the same pair as Flatwater Bass Lift's; the Audio path is HP3 + g LP3 with the Original Bass gain g on LP3 | impulse responses at Freq 32 / 80 / 256 Hz and Original Bass -inf, -6, 0, +12 dB | -78 to -101 dB for every setting tried |
| At Original Bass 0 dB | a flat all-pass (an impulse response with sum 1 and energy 1); linear | levels 0.01 and 0.5 | -93 dB between levels |

## 3. The generated path

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| At rest (very low level) | an exact LTI response -K(F) LP3(F) HP_G, bilinear Butterworth sections; HP_G at 6 dB/oct is HP2(16 Hz)^2, at 12 dB/oct HP2(F) HP2(16 Hz), at 24 dB/oct HP2(0.8 F)^2; below about 8 Hz the reference leaks a little DC (-50 to -98 dB), the mark of single-precision filter coefficients | impulse responses at 1e-4 (`plugins/FwBassXLift/nulltest/measure_linear.py`) | 0.009 to 0.035 dB, 0.2 degrees; linear to -58 dB; bit-exact time-invariant |
| High-pass on G | applies to G only, not to the original path: at Freq 80, h1 of G is -4.1 / -21.6 / -36.1 dB at 20 Hz and -2.2 / -10.3 / -14.3 at 40 Hz for 6 / 12 / 24 dB/oct; equal above Freq | tones | measured |
| Output scale K(F) | carried as the measured value for each of the 225 whole-Hz settings (2.77 at 32 Hz, 4.53 at 80 Hz, 5.59 at 256 Hz); smooth, but no simple formula fits better than 0.9 % | `k_out_table.json` | each fitted to -46 to -77 dB |
| Harmonic product | 8 D \* u \* HP2_B(u), u = LP3(F) x, followed by the same HP_G, with HP2_B a Butterworth high-pass at F/3 held within 20 to 40 Hz; Decay is an exact gain on it (-9.00 / +6.00 dB for Decay -24 / -9) | 190 two-tone kernels per setting (`plugins/FwBassXLift/nulltest/measure_k2.py`) | sum and difference terms -46 to -50 dB at Freq 50 to 256 (the difference terms differ only below 10 Hz, the DC-leak region) |
| Product against linear | 7.98 +- 0.01 x D at every Freq: the level control's ceiling is exactly 1, and K(F) is a separate output scale | kernels | measured |
| Decay feedback | the product feeds back: y = w (1 + 8 D HP2_B(y)); the 3rd harmonic grows with D^2; the reference applies the loop with one sample of delay: when 8 D b0 abs(w) exceeds 1 (strong Decay, high Ratio, loud low bass) it oscillates at Nyquist, as seen in its own output, and y is held within +-1 | static harmonics | h1 -56 to -62 dB, h2 -40 to -55 dB |
| G content | a strong component at the input frequency itself plus mostly 2nd and 3rd harmonics; -20 dBFS tones at Freq 80 and Decay -15, in dB re input: 40 Hz h1 -10.3, h2 -35.6, h3 -50.9, h4 -66; 60 Hz -6.6 / -37.8 / -47.0; 100 Hz -7.5 / -48.3 / -53.1 | `harmonics.csv`, 105 rows over Freq, Decay and f0 | measured |
| Decay | acts mainly on the even harmonics, about 1 dB per dB: at 40 Hz h2 is -44.6 / -35.6 / -29.6 dB and h4 -75 / -66 / -60 for Decay -24 / -15 / -9, while h1 and h3 barely move (-10.3; -51 to -49); the Bass Lift reference's hidden Decay also scales the even-harmonic product | tones | measured |
| Against the Bass Lift reference | not the same tuning: on the same tones (Freq 80) the Bass Lift reference's generated fundamental is 8 to 15 dB stronger and its harmonics 15 to 25 dB stronger (40 Hz at -30 dBFS: h2 -19.4 against -42.3 dB re input) | tones | measured |

## 4. The level control

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Placement | a gain g of at most 1 on u ahead of the generator (w = g u); independent of Decay; it depends on the High-pass setting (a 20 Hz tone at -6 dBFS: -10.7 / -1.7 / -0.4 dB for 6 / 12 / 24 dB/oct), and its level curves for different tone frequencies are shifted copies of each other by about G's own small-signal response: the level control listens after HP_G | the 2nd harmonic predicted from g | 0.05 dB over -84 to 0 dBFS |
| Static law | 50 Hz, Freq 80, gain re its cap: Ratio 1 falls from 0 dB below -102 dBFS to -8 dB at -60 dBFS, then -9 dB to -36 dBFS and -17 dB at 0 dBFS; above Ratio 1 the low-level knee moves up about 12 dB per doubling of ln Ratio, and the high-level slope tends to 1 - 1/Ratio; at low Freq (45 Hz and under) and Ratio 1 the resting gain is 1/8 of the cap | `static_gain_50hz.json`, `level_fine_50hz.json` | measured |
| Seen on the output | at Ratio 1 G is compressed: h1 goes from -4.1 dB re input at -60 dBFS to -13.1 at 0 dBFS (50 Hz); Ratio above 1 is upward compression: quiet input gets G at up to +4 dB re input (a ceiling reached at -60 to -36 dBFS for Ratio 2 and 4), loud input less; the harmonics grow faster than the input | `level.csv`, `level_ratio.png` | measured |
| Steps | a 50 Hz step -40 -> -10 -> -40 dBFS: at Ratio 1, G settles within 1 dB in 34 / 51 / 72 ms after the rise for Response 10 / 20 / 30, overshooting +1.5 / +3.0 / +3.8 dB; after the fall it undershoots -6 to -10 dB and settles in 48 to 58 ms; at Ratio 4 the rise settles faster (16 to 40 ms) with less overshoot | `dynamics_steps.png` | measured |
| Response on steady material | 10 or 30 ms against 20 ms changes G by -25 to -29 dB re G on a steady amplitude-modulated bass (an attack / release time, small on steady material) | AM bass | measured |
| Recovery and attack | the level control recovers fully within 0.1 to 0.3 s of silence and, after silence, attacks over a time set by Response: at Ratio 1, 10 ms into a burst the gain is still -6.9 / -3.9 / -2.7 dB for Response 10 / 20 / 30, settling in about 4 x Response | `plugins/FwBassXLift/nulltest/probe_recovery.py`, `recovery_F80.json` | measured |
| Gain over time | a fast drop at onsets, a release of hundreds of ms, and ripple within each cycle that adds odd harmonics at high level | extracted from renders by least squares through the exact structure (Gauss-Newton with the generator's exact Jacobian, `plugins/FwBassXLift/nulltest/extract_g2.py`, `plugins/FwBassXLift/nulltest/extract_g3.py`) | the model then re-renders the reference's G at -50 dB median (-38 dB on bass notes, -27 to -33 dB on kicks) |

Method notes: no analytical detector was found for the gain law (its knee and slope both move with Ratio, plus the
resting state), so the plug-in learns g (section 5).

## 5. Learned block

The plug-in keeps every identified part exact and learns only the level control g.

| Item | Value |
|---|---|
| Structure | a GRU with 48 units; inputs abs(x), abs(u) and abs(z) (compressed with asinh; z = K(F) HP_G(u), the signal the level control listens to), ln Ratio, Response, ln(F/80) and the High-pass setting; output ln g = -softplus(.), so g is at most 1; runs on its own 48 kHz clock at any host rate |
| Target | the reference's generated part, trained end to end: the generator's forward and adjoint recursions run in C (`plugins/FwBassXLift/nulltest/generator.c`), HP_G as FFT convolutions on the GPU |
| Dataset | about 13 900 training renders of the reference (tones, steps, AM, two-tones, noise, kicks, bass lines, pinned pairs, sweeps) over random Freq, Ratio, Response, Decay and High-pass, onset-heavy (kicks, steps, bass lines, a third of it at Ratio 1), with long fine-tunes at a lower learning rate |
| Held-out result | the validation set at -39.1 dB; the generated part in the null test -42.7 dB (-35.9 before the onset-heavy material and fine-tunes, which also brought kicks and steps 3 to 5 dB closer) |
| C++ against PyTorch | -71 to -117 dB on most examples, -43 dB on a kick |
| Weights | `plugins/FwBassXLift/Source/dsp/Net.h` |

It is a learned model of the reference's gain control, not an identification of its algorithm.

## 6. The model against the reference

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Fixed cases | median -48.5 dB on the whole output, -42.7 dB on the generated part (Harmonics monitor); by signal (generated part): white noise -49 dB, quiet tones -59, tones and sweeps -40 to -49, pink noise -43, level steps -34, bass notes -27, kick-like transients -19 | 12 signals x 8 settings over Freq, Ratio, Response, Decay, High-pass and the faders, [NULLTESTS.md](NULLTESTS.md) | a model, not a null |
| Random grid | median -35.2 dB whole output, -29.2 dB generated part (p90 -19 dB); no parameter range stands out (generated part -26 to -31 dB median in every Freq, Ratio, Decay and High-pass band) | 120 random settings over every control with the four hardest signals (`plugins/FwBassXLift/nulltest/null_grid.py`, `plugins/FwBassXLift/NULLTESTS-grid.json`) | a model |
| The structure's ceiling | with the reference's own extracted gain the grid reaches -37.1 dB median against the network's -28.1 on the same 36 cases: bass notes -31 against -24, pink noise -46 against -36, the tone -39 against -31, kicks -21 against -18; kicks are mostly a limit of the structure (partly the reference's own monitor clipping at +-1 on the first kick peak), the rest mostly the learned level control | `plugins/FwBassXLift/nulltest/null_oracle_grid.py` | |
| Worst cases | loud material at extreme settings (Freq 256 / Ratio 4 / Decay -12 with a -3 dBFS tone: +2.5 dB; Decay -9): there the reference's own Decay loop goes unstable and turns chaotic (Nyquist content -7 dB, h2 above h1, output pinned at +-1); even the reference's own extracted gain reproduces it only to -4 dB, and the structure with that gain reaches -20 to -27 dB at Decay -9 | the grid | open |
| 96 kHz | within 1.8 dB of 48 kHz | the fixed cases | |

## 7. Declined changes

| Change | Measured effect |
|---|---|
| The network without z and the High-pass input | stopped at -22 dB on the generated part |
| Explicit envelope followers of abs(z) as extra inputs (3 to 100 ms peak, 10 / 50 ms power) | no gain: the network does not lack them |
| The previous output sample as an input (teacher-forced) | worse in closed loop |
| An explicit detector instead of the GRU: two attack / release followers in cascade on abs(z) with learned time constants scaled by Response, Freq and Ratio, and a learned per-sample gain law (`plugins/FwBassXLift/nulltest/train_net_det.py`) | -30.8 dB on the validation set against -39.1 for the GRU, weakest on kicks (-17 dB) and steps (-21 dB): the reference's attack is richer than two followers; its fitted time constants (about 5 to 11 ms attack, 22 ms release, then a 15 to 22 ms smoother) agree with the recovery probe |

## 8. Meters

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Law | the bars rise at once and release as a one-pole on linear amplitude, as the Bass Lift reference's but faster: 55.8 to 56.4 dB/s over three releases, a time constant of 0.155 s (the Bass Lift reference 0.325 s) | films with `tools/MeterCapture`: 50 Hz bursts at -6 and -20 dBFS, then silence | measured |
| Scale | 3.79 px per dB with -40 dB at y 274.7 (the rounded ends inset the bar from the tick marks); below -40 dB, while audio runs, a small dot stays at the bottom (y 279 to 283); with no audio at all, nothing | frames | measured |
| Peak readouts | -6.0, then -4.9 from the release overshoot; ours match | frames | exact |
| Flatwater BassX Lift against the reference | within 1 px on 88 % of frames, 3 px on 93 % | the same films | measured |

## 9. Editor behaviour

Measured on the reference editor with the `tools/MeterCapture` drag, click and double-click scripts (synthetic mouse
moves, parameters read back after each step, cursor visibility checked mid-drag, screenshots on hover, press, drag,
release and mouse away).

| Quantity | Value |
|---|---|
| Value entry | a double-click on a knob, a fader or the Freq slider opens an entry box with the whole value selected, and Enter applies it (Ratio 2.5, Input -6, Freq 100 all took); a single click on the value text does nothing, Ctrl-click does nothing, Alt-click resets to the default |
| Entry box | white, black text, a 1 px (100, 100, 100) line on the left and right only, the system's blue selection (0, 120, 215); knob value x 106 to 159, y 374 to 387 (13 px text); fader x 553 to 583, y 320 to 337 (15 px text); Freq x 380 to 429, y 297 to 309, showing the bare number; ours match the boxes to the pixel and the text within 1 to 2 px |
| Cursor | hidden while a fader, knob or the Freq slider is dragged; Flatwater BassX Lift puts it back on the control |
| Faders and the Freq slider | follow the mouse 1:1 (a fader's 165 px travel, the slider's 264 px) |
| Knobs | about 500 px for the full range when the mouse moves 5 px or more per move, about 1000 px when it moves 2 px or less per move; Ctrl a quarter of either rate, keeping the slow / fast split (the first drag read about 3000 px for the full range with Ctrl); Shift does nothing special |
| Drag check | ours give the same values on every test drag (Ratio 1.00 -> 1.12 -> 1.18, Decay -15.0 -> -16.2 -> -16.8, Input -4.8 against -4.9 dB, Freq 80 -> 119 Hz) |
| Selection marks | a click marks a control until another one is clicked; hovering shows nothing; knobs: a ring at radius 20.5 (yellow (241, 227, 7), brightest at 3 and 9 o'clock, gone at the top and bottom); the Freq thumb: the same ring at 6 px; faders: a 13 x 2 px bar under the thumb ((85, 81, 10)) plus a glow of the line's own colour over the thumb; the Filter box: a 2 px outline 2.5 px outside its border ((85, 82, 24) at the ends, fading toward the middle of the top and bottom); the Monitor column: a 1 px line under the buttons (pure yellow in the middle, cosine fade) |
| Groups | a lasso dragged on the background (a 1 px pure white rectangle) selects every control it touches, live; Freq is not part of groups; the first in the control order (Input, Original Bass, Ratio, Harmonics, Decay, Response, Monitor, Filter) is the primary (yellow), the others members (blue: knob ring (7, 168, 229), fader bar (9, 60, 80), Filter outline (22, 66, 84)); clicking a member makes it the primary and keeps the group; clicking the background clears |
| Group drags | dragging a member moves the group by equal steps on the reference's 0 to 1 parameter scale (Original Bass +0.093, Input +0.089, Harmonics +0.090 for a 15 px drag), each member clamped at its own ends (Input stops at 0 dB, the top 2/3 of its scale); choice members step one setting per mouse move; ours give the same values on the same drags (Ratio 1.12 -> 1.25 -> 1.00, Decay -16.2 -> -15.0 -> -17.4, Filter 12 -> 24 -> 6 dB/oct) |

## 10. Wheel and Ctrl across the suite's references

Measured the same way on the Vocal Comp, Bass Lift and BassX Lift references (`scripts/feel/`: 50 px drags, 200 px
Ctrl drags in mouse steps of 1, 2, 3 and 10 px, each from the default value in a fresh instance; 3-notch wheel moves).
The Comp reference's knob law is in [plugins/FwComp/MEASUREMENTS.md](../FwComp/MEASUREMENTS.md).

| Quantity | Value |
|---|---|
| Wheel | every control takes it over its whole area (the faders take clicks on the thumb only), 1/72 of its range per notch as on the Comp reference, a stepped control one step per notch; Ctrl changes nothing; implemented in the shared rule (`shared/ui/Wheel.h`), the editors forwarding the wheel to the control under the mouse, with measured scale factors where a control's range is not the reference's (Flatwater BassX Lift Input 1.6, its range being 2/3 of the reference's scale; Freq 1.2 on Flatwater Bass Lift and Flatwater BassX Lift) |
| Faders and sliders | the thumb follows the mouse (the Bass Lift reference's Freq at 1.53 times its track: the whole range in 98 px); no slow / fast difference without Ctrl; Ctrl gives about 0.19 of that rate on mouse steps of 3 px and more and about 0.037 on steps up to 2 px (the Bass Lift reference's Freq 0.28 / 0.038, the BassX Lift reference's Freq 0.23 / 0.067); implemented as measured rates (`Fader` in shared/ui/RenUI, `WavesDrag` for the Flat kit) |
| Knobs | as the Comp reference's: Ctrl a quarter of the rate, keeping the slow / fast split |
| Result | the three plug-ins give the reference's values on all of these scripts within the display's last digit (Freq on the wheel 1 Hz apart: its steps are whole Hz) |
| Ctrl pressed or released during a drag | on all four references the rate switches at once, both ways (the Comp reference's Threshold -30.0, the Vocal Comp reference -14.0, the BassX Lift reference's Ratio 1.41); a host that keeps the keyboard focus (as FL Studio does) sends the plug-in no key messages, and JUCE refreshes its modifier keys only from key messages and mouse buttons, so the controls read the key itself during a drag (`ctrlHeldNow` in `shared/ui/Wheel.h`); reproduced in `MeterCapture hostwin=1` (the editor in a plain Win32 window that keeps the focus); Flatwater Comp, Flatwater Vocal Comp, Flatwater Bass Lift and Flatwater BassX Lift give the reference's values in both modes (Flatwater Comp -30.0, Flatwater Vocal Comp -14.3, Flatwater BassX Lift 1.41); scripts `scripts/feel/midctrl_feel.sh`, modes `midctrl` / `ctrlup` (the drag starts plain, Ctrl goes down or up halfway, 10 px steps) |
| The cursor during a fader drag | hidden while dragging, and put back where it was grabbed, moved as far as the thumb moved (the Vocal Comp reference's Gain: a 100 px drag ends on 235, a 60 px Ctrl drag on 223; the Bass Lift reference's Freq: 234 and 268); Flatwater Vocal Comp 238 and 223 (the 3 px from its Gain taper, -24.0 dB where the reference reads -23.4); Flatwater Bass Lift 233 and 268; MeterCapture prints where the cursor is after the release |

## CPU

| Configuration | Cost |
|---|---|
| Stereo in real time, with the learned block (SSE2 / NEON inner loops) | about 7 % of one core |

## Provenance

The reference is the original MaxxBass, run from a licensed install.

- Measured: renders of the reference's output (tones, steps, bass material) at its settings, through its VST3 in a
  host; films and drag scripts of its editor.
- Fitted or learned from the reference's output: the band split, harmonic generator and filters identified from those
  renders; a small recurrent net for the level control (our own weights, `plugins/FwBassXLift/Source/dsp/Net.h`)
  trained end to end through the identified structure on captures of its output.
- Redrawn: the editor, drawn in code in Flatwater's own dark look (`shared/ui/Theme.h`) on the original's
  measured layout (the positions and sizes of the controls), with our own colours and shading; no table of the
  original's pixels ships.
- Presets: the original's factory preset values under our own names and groups, the suite's standing rule; the
  preset files themselves are not copied.
- Shipped on recorded approval: none beyond the above.
- None of the original's code, artwork, preset files, fonts or other files ships with Flatwater BassX Lift.
