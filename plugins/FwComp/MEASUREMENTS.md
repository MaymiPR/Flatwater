# Flatwater Comp: measurements

The current values of every stage of Flatwater Comp, measured on the original it recreates (H-Comp), with the method
and the residual or confidence of each. The data and plots are in [measurements/fwcomp/](../../measurements/fwcomp).

## Setup

| Item | Value |
|---|---|
| Reference | H-Comp (Waves), the H-Comp Stereo and H-Comp Mono components, from a licensed install |
| Host | pedalboard through `nulltest/refhost.py` `render_hc`, which sets every user control (all are VST3 parameters; the rest at their defaults, Analog Off unless stated); `tools/RefRender` (a small JUCE host, `refhost.render_ref`) where pedalboard cannot feed the side-chain bus or give a host tempo; 48 kHz unless noted; `python -I nulltest/measure_hcomp.py [section ...]` reproduces the first battery |
| Parameter order | Sync is set before Release: the reference does not keep a Release value set while it is synced to a note |
| Gain readout | the detector reads the sample peak, so an alternating +-a signal (Nyquist) is a constant level to it, and with Analog Off the output is g[n] x[n]: the gain trace is y / x, exact at every sample; static curves, attack, release, Punch, Mix and the limiter are measured that way, and sine staircases check the static curves |
| Identification scripts | `nulltest/ident_hc_detector.py`, `ident_hc_struct.py` (detector, smoothing, order of the stages), `ident_hc_release*.py`, `ident_hc_w0.py`, `ident_hc_real.py`, `ident_hc_rel3.py` to `ident_hc_rel8.py` (release), `ident_hc_analog.py`, `fit_hc_analog*.py` (Analog stage); the C models `hcfast*.c`; `cma_hc_release.py` (CMA-ES) |
| Tables | `static_h.npz`, `makeup*.csv`, `release_law*.json`, `release_w0.json`, `analog_model.json`, `analog_noise.csv` in `measurements/fwcomp/`, exported to `plugins/FwComp/FwCompTables.h` by `tools/export_fwcomp_tables.py` |

## 1. Path and latency

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Latency | 64 samples at 44.1 / 48 kHz, reported and compensated; 128 at 88.2 to 96 kHz, 256 at 176.4 to 192 kHz | an impulse at Analog Off with the compressor idle | exact |
| Idle path | Analog Off, threshold 0 or ratio 1: an exact identity (impulse response 1.0, flat to 0.000 dB) | impulse | exact |
| Stereo | linked: a loud left channel compresses a quiet right one by the same gain (-10.01 dB both); the Mono component equals a Stereo channel | tones per channel | exact |
| Side-chain | the plug-in offers a side-chain bus, but the reference ignores it: a loud key does not compress a quiet main signal and a silent key does not stop a loud one; the manual has no side-chain | `tools/RefRender` with `sc=` | exact |
| Sample rates | 44.1 to 192 kHz: the same makeup, static gain, attack (12.3 ms at 63 %) and release (72 ms) | the battery per rate | within 0.02 dB and 2 % |

## 2. Detector

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Level | the sample peak, max of abs(L), abs(R) (linked): a sine, a square wave and an alternating signal of the same peak give the same reduction; an 8 kHz sine (6 samples a period) gets 0.94 dB less, as its samples miss the crest | `freqdet` | 0.001 dB |
| Hold | held over the next 64 samples of the delayed audio (h[t] = max a[t+1 .. t+64]); sizes x2 at 88.2 to 96 kHz, x4 at 176.4 to 192 kHz; full reduction down to 400 Hz, less below (200 Hz -0.46 dB, 80 Hz -1.2, 30 Hz -1.8 dB of gain reduction at threshold -24 / ratio 4) | `freqdet`, `ident_hc_detector.py` | exact |
| Look-ahead seen at the output | the gain starts to fall 1.25 ms (60 samples) before a step reaches the output, at every rate | steps | exact |
| Punch | delays the detector by exactly 1 ms per unit, round(P fs / 1000) samples: the gain starts to move at (Punch - 1.25) ms after a step (Punch 1: -0.25 ms, 10: 8.75, 30: 28.75 ms), so transients pass before the gain reacts | steps | exact |

## 3. Static curve and makeup

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Static curve | x = (1 - 1/r) max(0, 20 log10(peak + 0.001) - T), GR = h(x) with one measured table h (x - h saturates at about 0.55 to 0.6 dB, the knee); no hysteresis | 42 measured curves (`static_h.npz`); staircases up and down | 0.014 dB rms; staircases agree within 0.008 dB |
| Closed form | GR = (1 - 1/r) knee(Ld - T - 0.76), Ld = 20 log10(peak + 0.00106) (a floor at -59.5 dBFS, which makes the curves at low thresholds approach the ratio only slowly), the knee quadratic over 3.0 dB | 20 curves (thresholds 0 to -48, ratios 1.5 to 50, 0.5 dB steps; `knee.csv`, `knee.png`, `static.csv`) | 0.11 dB rms, 0.33 dB worst |
| Makeup | automatic, k(r) max(0, -9 - T) f(Release): 0 for thresholds above -9 dB, then about linear in the threshold; k per ratio, interpolated in 1/r (0.47 dB per dB at ratio 50, +0.31 at ratio 3, +0.10 at 1.5, 0 at 1.1); f a table over the release time (0.257 at 3 ms, 1 at 100 ms, 1.041 at 3 s); independent of attack (examples: +2.76 dB at -18 / 3, +18.35 at -48 / 50) | dense grid in `makeup.csv` (25 thresholds x 18 ratios), `makeup_release.csv` | measured table |

## 4. Attack and smoothing

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Attack | a one-pole on the linear gain, falling only, time constant 0.99 x Attack, after the release stage; its 90 % / 63 % time ratio is 1.88 to 1.92 (a one-pole on linear gain gives 1.87, on dB 2.30); with the look-ahead counted the time constant is the Attack value (7 ms gives 7.2, 20 gives 20.0, 100 gives 98.5 ms) | steps on the alternating probe | putting the attack before the release fits worse (section 11) |
| Smoothing | two moving averages of 38 and 27 samples on the gain | `ident_hc_struct.py` | a free FIR fit leaves 6e-4 rms |
| Steady low tones | the reference's reduction on a steady 55 Hz tone depends on the attack time (-7.0 / -6.1 / -4.8 / -3.4 dB of gain at Attack 0.5 / 7 / 30 / 100 ms; at 1 kHz it does not): its level detection averages across a low waveform's cycles, which the held peak, the release law and the attack on the gain do not; a pilot tone through both (`pilot_cycle.py`) shows its gain settling higher, not dipping inside the cycles; removing the floor R >= S does not create the dependence | `nulltest/sweep_fwcomp.py`, `pilot_cycle.py` | covered by the learned correction (section 6) |

## 5. Release

The law, in the gain-reduction domain, with two states (`ident_hc_release2.py`, `ident_hc_w0.py`):

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Target R | the reduction after the static curve releases at an effective release time min(Release, Release w0(S), K / (R - S)^p) and never below S (`hcfast2.c hc_set_r_floor`, `FwCompTables.h rFloorAtS`) | release probes | without the floor the refit reaches -42.09 dB against -43.81 with it |
| Slow state S | charges toward out + 0.032 (R - out), out the reduction after the attack (the previous sample's), as a one-pole on the linear gain (142 ms; the reference's own hidden crest time is 149.6 ms); releases at Fs(S) / Release + As(S) dB/s with a release time of at least 16.8 ms; charges only when its source rises by more than rounding (src > S + 1e-9; any margin from 1e-12 to 1e-6 gives the same output), level or falling it releases | CMA-ES over all the tables (`cma_hc_release.py`, 250 generations) | objective -43.97 dB |
| Start-up | the effective release time is pulled toward the Release time by exp(-t / 90 ms) after a reset | start-up probes and program material from 0.05 s | null test median -40.0 -> -41.9 dB when added; threshold -24 / ratio 10 / Release 300 -21.9 -> -31.5; the defaults lose about 3 dB in their first half second only |
| Release time seen from outside | after sustained compression the 63 % point is 0.75 x Release (3 -> 3.0, 100 -> 76, 1000 -> 753, 3000 -> 2258 ms); the gain rises at a constant rate in dB and stops sharply at the threshold: the release acts on the detector's level, not on the gain | steps | measured |
| Rate against the reduction | a function of the reduction itself, the same at every ratio: after a 1 s charge at Release 300 the reduction falls 86 / 110 / 167 / 306 / 1364 dB/s at 17 / 19 / 23 / 29 / 39 dB (ratios 10, 2, 50, 50, 50), not of the level over the threshold | probes | measured |
| Program dependence | the release depends on how long the compression lasted, not on its depth: after 5 / 20 / 100 / 200 / 350 / 500 / 700 ms bursts the 63 % point is 0.7 / 2.8 / 31 / 43 / 65 / 73 / 76 ms (Release 100), then constant; attack and ratio barely change it | `release_pd` | measured |
| Fast part | after a short charge (10 to 50 ms) the reduction falls along one curve whatever its depth (to about 15.8 dB 4 ms after the end, 12.9 at 8 ms, 11 at 20 ms, from 23, 29 or 39 dB); longer charges slow it (500 ms: down to 27 dB at 4 ms from 29); below about 12 dB there is no fast part; it barely depends on the Release time | probes | measured |
| Settled behaviour | with a 1 s quiet lead-in: bursts of 2 to 5 ms release almost at once at every depth (gone within 4 ms); 20 ms bursts nearly so; 50 ms and longer release progressively more slowly, up to about 500 ms of compression, after which the release no longer changes; a 4 ms burst with Attack 7 ms releases at once too; a slow average of R with a time constant of 148 to 159 ms (fitted twice) explains much of this | `ident_hc_rel4.py`, 312 probes | measured |
| Start-up of the reference | for the first 0.1 to 0.3 s after a reset the reference releases slowly with no reduction to show for it (a pitch spike that releases at once after 0.3 s of quiet holds its reduction after 0.02 s, and builds up over repeated spikes); it lasts 0.2 to 0.4 s at every Release time (30 ms to 3 s); probes with a short quiet lead-in measure this start-up, so the fits use cases past it; the probes prefer a longer, step-like start-up, program material about 85 ms | start-up probes | measured |
| Steady tones | a steady 100 Hz tone keeps its reduction almost still between peaks (0.05 dB ripple at Release 300) | probes | the w0 term releases ten times faster there |
| Repeated events | 2 ms pulses every 7 ms release fully between the first pulses, but after about 100 ms (Release 100; 75 ms at Release 300, 250 ms at Release 30; sooner at greater depth) the reduction hardly recovers between pulses (5 dB pulses: 4.8 dB minimum after 0.5 s); 2 ms every 20 ms builds up only at depth: the slow state's structure | pulse trains | measured |
| Memory through the tail | after a 500 ms burst and a quiet gap, a 5 ms burst still releases slowly when the gap is 100 ms (the gain already back to 0.05 dB), and at once after 200 ms or more | `ident_hc_rel8.py collectmem`, 90 double-burst probes | measured |
| Memory follows the output | with a long attack (Fat Kick preset: Attack 33 ms) the target swings to 9 dB on every half-cycle of a kick but the output only reaches about 1.5 dB, and the reference releases almost to 0 between half-cycles, while a slow state charged from R held 5.7 dB; hence the charge toward out + blend (R - out) | presets on program signals | when added (CMA-ES, 1034 cases, objective -42.7 -> -43.7 dB): presets' median -32.0 -> -35.5 dB (Fat Kick drums -12.9 -> -21.7, Drum Punch voice -21.6 -> -38.0, Pumping Kit voice -33.1 -> -44.5, Drum Knockout drums -24.7 -> -41.7); the null test's worst -10.5 -> -16.7, its median -41.9 -> -40.3 |
| Memory at short Release | on a sustained bass note with Attack 17.7 and Release 13.5 ms (Vintage Drum Crunch preset) the reference keeps 6 to 9 dB between cycles; hence the floor on the slow state's release time | presets | when added (objective -43.70 -> -43.81 dB): Vintage Drum Crunch bass -8.4 -> -17.0 dB, Lo-Fi Radio bass -24.3 -> -27.5, Drum Punch voice -38.0 -> -33.5; presets' median -35.5 -> -34.8; the null test the same or better everywhere (deep slow setting -31.5 -> -33.5) |
| Final joint fit | the blend (0.032) and the floor (16.8 ms) fitted with all the tables | CMA-ES, 250 generations | objective -43.81 -> -43.97 dB; the null test's median -40.3 -> -41.4 dB (worst -16.8); the presets' median -34.8 -> -35.6 dB (Vintage Drum Crunch bass -17.0 -> -19.1, Lo-Fi Radio bass -27.5 -> -31.1) |

Method notes: the CMA-ES objective is the energy-weighted mean null in dB over the settled probes, the memory probes,
realistic envelopes and the null-test material (x3), all past the start-up, plus the 26 factory presets' settings on
program signals (`ident_hc_rel8.py collectpresets`, 104 cases x3); least squares and exact gradients stall on this
law's rough error surface. The first CMA-ES refit of the two-state form (722 cases) took the mean null from -46.8 to
-49.0 dB and the null test's median from -37.2 to -39.6 dB: at the defaults the drum loop -20.7 -> -31.7 dB with the
voice-like signal and the bass line unchanged (-62.5, -58.5); threshold -30 / ratio 4 / attack 0.5 / release 50 better
by 8 to 20 dB (bass line -18.6 -> -39.4); lost: the 5 Hz AM tone at the defaults (-68 -> -52) and steady tones at
Release 1000. Adding the presets took their null median from -30.4 to -31.3 dB.

## 6. Learned gain correction

| Item | Value |
|---|---|
| Reason | one-parameter sweeps from Threshold -24, Ratio 4 (`nulltest/sweep_fwcomp.py`) without it: voice, drums, bass and noise all -22 to -26 dB; long attacks on bass -15 dB (15 ms) to -8 dB (30 to 60 ms); Release 300 on voice -17; and the steady low-tone dependence of section 4 |
| Structure | a GRU with 64 units reads Flatwater Comp's own states every 16 samples (the held peak against the threshold, the target and release states R and S, the reduction after the attack and out, the settings; `FwCompRender trace=`) and corrects the compressor's gain in dB, held over the step (`plugins/FwComp/FwCompNet.h`, `GainNet` in `FwCompDsp.h`) |
| Guards | no correction without reduction, nor while the target stayed within 0.05 dB over 20 ms (a steady tone above about 750 Hz, which the analytical model nulls already; without this guard the correction cost 1 kHz tones 20 to 30 dB) |
| Dataset | 4300 cases rendered through both (`render_trainset_fwcomp.py`; our side refreshed by `nulltest/refresh_trainset_fwcomp.py` after a model change, the reference's renders kept): random settings and signals, hot drums, bass, plucks and voice up to +3 dBFS, the null test's and the presets' settings on every signal at random levels, densely at the common settings; Mix 100, Output cancelling the makeup |
| Training | PyTorch (`nulltest/train_fwcomp.py`), 800 epochs; the loss is the null itself for the weak cases (worse than -35 dB) plus a penalty on every case more than 0.5 dB worse than without the correction (or than -60 dB if it was better), the weak cases alone for the first 300 epochs |
| Held-out result | 200 unseen cases: the weak ones -26.1 -> -36.5 dB, all -48.8 -> -52.3; steady 55 Hz under Attack 30: -4.77 dB (the reference -4.79) |
| Platforms | the DSP and the correction give identical bits on Windows, Linux and macOS (section 9) |

## 7. Mix, output and the limiter

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Mix | a linear blend of the dry input (no makeup) and the compressed signal | the alternating probe | exact to 0.0002 |
| Output | a pure gain, +-18 dB, after the Mix | tones | exact |
| Limiter | a 64-sample look-ahead (lead 1) of the would-be output, min(1, 1 / held), through the same two moving averages; both Limiter Off and On keep the output under 0 dBFS as a look-ahead peak limiter, not a clipper: the gain ramps down over the last 63 samples before a peak (an S-shaped ramp) and recovers in about 1.2 ms (Off) or 1.4 ms (On; modelled as a 7-sample longer hold); a 1 kHz sine 6 dB over comes out undistorted (h3 -116 dB), a 100 Hz sine gets clipper-like odd harmonics (h3 -20 dB) because the gain recovers between its peaks; Off lets a single-sample spike through at 1.0006, On at 0.9997 | the alternating probe | with a constant compressor gain exact (worst 0.0002; a spike overshoots to 1.002) |
| Limiter with a moving compressor gain | the reference's limiter sees the true future output, which a 64-sample latency cannot give; the estimate used is the current compressor gain times the peak of the pending input | heavy limiting (+12 dB into it) | -16 to -43 dB |

## 8. Analog stage

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Position | the last stage: after the output gain and the limiter, applied to the dry part too, per channel; its offsets stay the same in output units under Output -18 / +12, Mix 0 and heavy compression | applied to the Analog-Off output | -73 dB against the reference with the limiter working |
| Model | y = (1 + gain) L(x) + H2(g(H1(x))) + noise: first-order sections (mode 3: a band-pass biquad first, f0 183 Hz, Q 0.52), g a measured table with free odd and even parts; memoryless in level (a level step changes mode 3's dip at once, both ways) | `ident_hc_analog.py`, `fit_hc_analog*.py`, `analog_model.json` | on loud material (peaks 0.3 to 0.7): mode 4 -82 to -100 dB, mode 1 -72 to -78, mode 2 -44 to -56 (its noise alone limits it to about -54 to -69), mode 3 -29 to -64 (median -40); quiet signals null less (mode 4 at -60 dBFS: -40 dB), the offset being of the order of 0.001 |
| Mode 1 | flat -0.26 dB at -20 dBFS; harmonics (1 kHz at 0 dBFS, re fundamental) h2 -54, h3 -58; noise -102 dBFS | small-signal response, harmonics, noise with silence in | measured |
| Mode 2 | a dip of -0.77 dB around 1 to 2 kHz; h3 -48 (odd harmonics); noise -83 dBFS rising toward high frequencies | the same | measured |
| Mode 3 | a level-dependent dip (-4.8 dB at 1 kHz for -40 dBFS, -3.1 at -20, -0.5 at 0); strong h2 / h3 (-22 / -33); noise -101 | the same | measured |
| Mode 4 | flat within 0.1 dB, no noise (-109), only odd harmonics that are larger at low levels (h3 -27 dB re a -40 dBFS tone, -67 at 0 dBFS: crossover-like distortion) | the same | measured |
| Noise | stationary, independent per channel (L / R correlation 0.05 or less), unchanged under a tone: white Gaussian noise shaped to the measured spectrum (`analog_noise.csv`, 40 bands): -101.7 / -82.7 / -101.2 / -108.8 dBFS for modes 1 to 4 | silence and tones | measured; modes 1 to 3 match in response, harmonics and noise spectrum, mode 4 nulls sample by sample |

Full tables: `results.json` `analog`.

## 9. Tempo sync and determinism

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Sync | Release = note x 240000 / tempo (ms), with the same 0.75 factor; BPM uses the plug-in's own tempo (120 by default), Host follows the host's tempo (60 BPM: 1/16 gives 250 ms; 180 BPM: 83 ms) and falls back to 120 when the host gives none; long notes at slow tempos fall back to the next shorter straight or dotted note (2 bars -> 1 bar -> 1/2D -> 1/2 -> 1/4D) once they pass about 3.3 to 4 s | `tools/RefRender` with a host tempo; the points in `results.json` `sync_host` | measured; a fresh instance with Host sync: 17.86 ms for 1/64T at 140 BPM, -90 dB |
| The reference after a synced preset | after running synced (BPM or Host) and switching Sync to ms, the reference keeps the synced release time internally while its Release parameter shows another value, and takes the shown value again only once Release itself changes (coming from Host, not even then in one test); Flatwater Comp always follows its parameters, so nulls after a synced preset reload the reference, and the preset null test uses a fresh reference per preset | presets | with one shared instance two presets read -8 to -10 dB that were really -31 to -37 |
| Determinism | exp and log come from `Source/common/FwMath.h`, built from +, -, x, / and the exact frexp / ldexp / floor (within 1e-14 of the C library's; `FwCompTests`), and GCC and Clang do not fuse a multiply and an add (`-ffp-contract=off`): the DSP and the learned correction give the same bits on Windows, Linux and macOS; with the C library's functions the slow state's charge decision flipped on the last bit (MSVC, glibc and Apple's libm round differently) and the mix cases read -46 to -71 dB against the Windows goldens | the render regression's mix cases, with and without the correction | hash-identical on all three |

## 10. The model against the reference

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Null test | 168 cases: median -48.7 dB, worst -22.1 (Threshold -24, Ratio 10, Release 300 on a 5 Hz AM tone) | [NULLTESTS.md](NULLTESTS.md) | median a close match, worst a model |
| Factory presets | the 26 presets: median -44.0 dB, worst -20.8 | [NULLTESTS-presets.md](NULLTESTS-presets.md), a fresh reference per preset | |
| The last change | the rounding tie and the retrained correction, against the build before: null test median -47.0 -> -48.7 dB, worst -24.5 -> -22.1 (31 cases better and 26 worse by more than 1 dB; the largest loss the 5 Hz AM case, -31.8 -> -22.1); presets median -43.7 -> -44.0 dB, worst -20.8 both | the same null tests on the Windows build | |
| Before the correction was added | null test median -41.4 dB (worst -16.8); presets median -35.6 | the same | |

## 11. Declined changes

| Change | Measured effect |
|---|---|
| Release rate against R with a 2-D fast part, fitted on the start-up probes (`ident_hc_rel3.py`) | not adopted: fitted on the start-up, not the settled behaviour |
| Release time scaled by a table of the distance between R and its slow average (`ident_hc_rel4.py`, `hcfast4.c`), charging upward only or both ways (both fits better); a second, 30 ms average (`hcfast5.c`); a learned correction of the release time (8 tanh units) from the distances to 156 / 30 / 5 ms averages (`hcfast6.c`, exact gradients through `hcgrad.c`, `train_hc_release2.py`, 0.4 s per gradient over 482 cases) | steady tones (100 Hz: -27 -> -54 dB at threshold -30) and modulated signals better, but the voice-like signal and the bass line lost (-59 -> -13 dB at the defaults): at a pitch spike the distance between R and its average is the same as after a 50 ms burst, yet the reference releases the first at once and the second over tens of milliseconds; fitting stalls from every start (finite differences and exact gradients alike, also with softened gates) |
| w0 = 1 (no w0 term) | fixes steady tones, loses the voice-like signal (-62 -> -8 dB) |
| The attack before the release | fits worse (-46.1 dB mean null) |
| Charging the slow state purely from the output (blend 0) | breaks: before the attack starts the output is 0 dB, the slow state takes its release branch and is floored at the target, so it jumps to the full reduction at once |
| Letting R fall below the slow state (no floor R >= S) | on noise with Attack 33 ms the slow state and the floor hold each other at about 2 dB while the reference stays at 0 to 0.3 dB, but without the floor the refit reaches -42.09 dB against -43.81 |
| The slow state charging on exact equality | the null test median falls to -14.4 dB |
| The correction trained on the mean null in dB over all cases | the good cases' gradients drowned the weak ones |
| The correction without the steady-tone guard | 1 kHz tones lose 20 to 30 dB |
| The correction with little data at the common settings | the default drum loop at its +2.7 dBFS level went from -31.6 to -24 dB until the common settings were added densely |

## 12. The VU needle and the limiter lamp

Filmed with `tools/MeterCapture gl=1`, which reads the editor's OpenGL back buffer just before each buffer swap: exact
design pixels at 63 fps, without the DAW and without the screen. Outside the DAW the reference's editor renders (about
62 OpenGL frames a second, 48 draw calls each) but its frames never reach the window, which stays white; the editor
takes its meter values from the processing only while the host's transport advances, so MeterCapture gives the
plug-in a playing transport. Probes: `nulltest/make_vu_probe.py` (tones, 5 / 50 / 300 ms bursts, drums, voice),
`make_vu_cal.py` (a level staircase and sine / square pairs) and descending steps; for GR the probe carries a
-54 dBFS 6170 Hz pilot whose gain through the compressor shows its gain during the silences too (`pilot_gain.py`).
Angles: `vu_angles.py`; pivot: `pivot_fit.py`; fits: `fit_gr_needle.py`, `fit_io_needle.py`; the print:
`measure_vu_face.py`. Summary: `measurements/fwcomp/needle.json`; the law in `plugins/FwComp/FwCompNeedle.h`.

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Positions | 71 needle angles: position k = round(70 v), each at a whole number of degrees from vertical, -58, -57, -55 ... 57, 59 (the table in `FwCompNeedle.h`) | 11185 filmed frames, a line fit per position | within 0.15 degrees of the whole degree |
| Pivot | (323.42, 220.86) (pixel centres at +0.5), 2.7 px right of the scale's centre | the lines of 800 frames | within 0.1 px |
| Needle | (124, 0, 0), 2 px across (equivalent width 2.0 to 2.4 px), from under the pivot's bump to its tip at 111.5 px; the rivet at 104.7 px: a red disc of radius 2.85 with a cream (248, 238, 158) centre of radius 1.15 | `needle_core_fit.py`, `rivet_cmp.py`; compared with our dial rendered by `tools/VuRender` (`compare_needle.py`) | measured |
| Shadow | the needle's outline moved straight down whatever the angle, (-0.01, +4.70) px, a neutral darkening (x0.747 on R, G and B at its middle), Gaussian across (depth 0.266, sigma 0.94 px), from under the bump to 110.2 px; drawn as three strokes fitted to that profile | `needle_shadow_fit.py`, on 23 and 7 positions of the two faces | ours measured the same way: (-0.01, +4.70), 0.265, sigma 0.94, to 109.9 |
| GR | v is the compressor's linear gain before makeup (steady tones: 31.75 + 117.17 g degrees); the deepest gain since the last frame shows at once (a 5 ms burst moves the needle to 38); the return is two one-poles in cascade on the linear gain, 28.9 and 142.6 ms; on the probe's drums (threshold -30, ratio 4) the reference's needle averages 11.5 dB and the law on our own gain 11.4 (voice 5.6 / 5.8) | `fit_gr_needle.py` with the pilot | steady tones within 1.5 degrees; 97 % of frames within one step of the pilot-gain prediction |
| In / Out | v = (peak dBFS + 48) / 48, the peak over the frame's interval (a sine and a square of the same peak read alike; -48 dBFS and below rest on the stop); a rise shows at once and starts a hold that decays from that moment (amplitude, 0.327 s: 26.6 dB/s into silence), so a burst's needle falls from its onset, and at the end of a 300 ms burst it jumps down to where that fall has reached; the first drop after a rise shows the level at once (unless the hold is still higher: a long tone drops straight to its silence); every later drop decays toward the level with the same 0.327 s until the next rise | `fit_io_needle.py`; Out on the reference's own output | 97.5 % of frames within one step (probe, staircase, descending steps) |
| Faces | GR: 48 24 12 6 3 0, the red band from 6 to 0; In / Out: dBFS, 48 36 24 18 12 6 3 0 with 24 at the top, the red band from 24 to 0, other dividers, ticks and dots (`needle.json`) | `measure_vu_face.py` | the printed arc's centre is (320.3, 220.7), about 0.4 px up-left of the centre ours draws about; the numbers, dots and ticks measured individually sit within about 0.5 px |
| Limiter lamp | lights whenever the output limiter (or the clip, with Limiter Off) acts: at full, fading to 0.64 (0.64 + 0.36 e^(-t/0.393 s)), off 0.833 s after the last trigger; a new trigger brings it back to full; lit, the dome glows amber (142, 115, 0) at its centre, dark by radius 8 | films | measured |
| Ours, filmed | a screen grab at 38 fps: our GR needle averages 11.5 dB on the drums (the reference 11.5) and 5.9 on the voice (5.6) | films | measured |

## 13. Editor behaviour

Measured with the `tools/MeterCapture` drag and click scripts on the reference editor; every knob with 100 px moves in
mouse steps of 1 to 30 px, 12 ms apart, and the wheel (`drag=x,y,n,0,wheel`, n notches; logs parsed by
`nulltest/parse_drags.py`).

| Quantity | Value |
|---|---|
| Direction | knobs turn clockwise when dragged up (Threshold toward -48 dB, since 0 dB sits on the left) |
| Continuous knobs (Threshold, Ratio, Mix, Output, Punch, Attack, Release) | the same share of their range per pixel whatever the knob: 0.001 per px on mouse steps up to 2 px, 0.002 from 3 px (about 1000 px for the full range on slow moves: -18 -> -19.4 dB for 30 px); Ctrl gives a quarter of either (0.00025 / 0.0005), so slow and fast still differ |
| Analog (5 positions) | one position per 50 px dragged, at any speed and with Ctrl alike |
| Release in BPM / Host (the note, 20 values) | one value per 30 px slow, 20 px fast, 120 px with Ctrl slow and 66.7 px with Ctrl fast, counted from where the drag began |
| Wheel | 1/72 of the range per notch on the continuous knobs, one position per notch on the stepped ones; Ctrl changes nothing |
| Selection | hovering shows nothing; a click selects a control (a thin yellow ring, colour (222, 222, 6), on the knob's cap at radius 21 for the small knobs) and shows its value in a dark box (fill (65, 49, 36), a light 1 px frame) on the value slot under the small knobs or on the cap of the large ones; a double-click opens the white entry box there; Alt-click resets |
| Result | Flatwater Comp gives the reference's values on every one of these scripts (the wheel within the display's last digit) |

## CPU

| Configuration | Cost |
|---|---|
| 60 s of stereo at 48 kHz through the offline renderer, without the learned correction | 1.39 s |
| The same with the learned correction | 1.99 s (about 1 % of a core more) |

## Provenance

The reference is the original H-Comp, run from a licensed install.

- Measured: renders of the reference's output (tones, steps, drums, voice, bass) at its settings, the Analog stage's
  response at levels and frequencies, and films of its VU needle and limiter lamp.
- Fitted or learned from the reference's output: the compressor's detector and release law, the Analog stage's
  measured tables (`plugins/FwComp/FwCompTables.h`), the needle's motion law; a small learned gain correction (our own
  weights, `plugins/FwComp/FwCompNet.h`) trained on captures of its output.
- Redrawn: the editor, drawn in code in Flatwater's own dark look (`Source/common/FwTheme.h`) on the original's
  measured layout (the positions and sizes of the controls), with our own colours and shading; no table of the
  original's pixels ships.
- Presets: the original's factory preset values under our own names and groups, the suite's standing rule; the
  preset files themselves are not copied.
- Shipped on recorded approval: none beyond the above.
- None of the original's code, artwork, preset files, fonts or other files ships with Flatwater Comp.
