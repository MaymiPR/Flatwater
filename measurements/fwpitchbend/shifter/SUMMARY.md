# Flatwater Pitch Bend: the reference's pitch / formant shifter, measured (Phase 1, track B2)

A pitch shifter does not null sample for sample, so this is a behaviour battery: what the reference does to pitch,
formant envelope, level, timing, unvoiced and difficult input, and Flatten, measured from its audio, plus enough of
its structure to build our own shifter in Phase 3 and a scorecard to judge it by.

- Script: `nulltest/measure_pitchbend_shifter.py` (`noshift structure beta pitch formant level timing unvoiced flatten
  plots`, or `all`). Renders are cached in `nulltest/renders/pbshift/` (git-ignored); a re-run renders only what is
  new. About 2,600 renders in all, 1 to 2 s each with four in parallel.
- Results: one JSON per command in this folder, plus `formant_envelope_shift.png`, `structure_marker_maps.png` and
  `timing_onset_settle.png`.
- Setup: the reference's Mono component, Mix 100. Whole semitones go through the host parameters (normalised
  (st + 12) / 24). Cents and Flatten go through the saved state (Fine on). The default is 48 kHz.
- Material: vowel stand-ins (a pulse train through three resonators: /a/ 800/1150/2900 Hz, /i/ 300/2300/3000, /u/
  320/800/2300) at 80 to 600 Hz. Real voice comes from the decoded FL Studio phrases (the read-only cache in the main
  checkout's `reference/fwvocalleveler/samples`): 14 sung phrases, 13 rap / spoken phrases and 4 "processed" phrases
  (effected voice).
- f0 comes from our own YIN (cumulative-mean-normalised difference, parabolic refinement, first dip within 0.05 of
  the deepest). The tracker's floor is shift 0 against the delayed input: p90 0.02 c on vowels, 0.05 c on sung
  phrases, 3 to 6 c on rap.

## 1. Pitch accuracy (`pitch.json`, `timing.json`)

| material | cases | per-case median error | per-case p90 abs error | gross (> 50 c or unvoiced) |
|---|---|---|---|---|
| vowels /a/ /i/ /u/, f0 80 .. 600 Hz, shifts -12 -7 -1 0 +1 +5 +12 | 147 | within ±0.41 c (median of medians 0.00) | ≤ 1.01 c | 0 |
| Fine on, ±13, ±50, ±350, ±1150 cents, f0 150 / 300 | 16 | within ±0.13 c | ≤ 0.16 c | 0 |
| 44.1 and 96 kHz, -7 0 +5 +12, f0 150 / 300 | 16 | within ±0.06 c | ≤ 0.13 c | 0 |
| sung phrases, 6 shifts | 84 | median -0.11 c | median 5.96 c | median 0, max 0.118 |
| rap / spoken phrases | 78 | median -0.2 c | median 20.4 c | median 0.107, max 1.0 (spoken word at ±12) |
| processed phrases (effected, aperiodic) | 24 | n/a | n/a | median 0.75: mostly not shifted |

- **The steady ratio is exact.** The output f0 equals the input f0 × 2^(shift/12) to a fraction of a cent at every f0,
  shift and sample rate.
- **Vibrato is followed.** At 30 and 100 c of vibrato at 5 Hz, the output's f0 curve lags the input's by 0 to 1 ms at
  -7 and by 4 to 7 ms at +5 (7 ms at f0 150 Hz, 4 ms at 300 Hz). Depth is 0.994 to 1.024 of the input's. The error
  with the lag taken out is p90 1.4 to 4.4 c.
- **Onset from silence.** At shift 0 the output is within 20 c in 2.5 to 10 ms (the tracker's floor). When shifted it
  takes 39 to 41 ms at f0 100 Hz, 27 to 29 ms at 200 Hz and 14.5 to 19 ms at 400 Hz (`timing_onset_settle.png`). At
  -7 the first 30 ms come out near the input's own f0 (-27 to -30 c from it, 100 and 200 Hz): the shift engages once
  the pitch is found.
- **Pitch jumps** (the excitation continuous, f0 stepped):
  - 150 → 225 and 225 → 150 Hz settle in 29 to 40 ms (our tracker on the input: 8.5 to 18 ms).
  - 150 → 160 Hz settles in 7.5 to 10 ms.
  - Octave up, 200 → 400 Hz: 126 to 133 ms.
  - **Octave down, 400 → 200 Hz: never within the remaining 1.25 s.** The reference keeps the 400 Hz period: at -7
    the output sits at 267 Hz (= 400 × 2^(-7/12)) instead of 133.5 Hz.

## 2. Formant envelope (`formant.json`, `beta.json`, `structure.json`, `formant_envelope_shift.png`)

Method: the factor α with out(f) ≈ in(f / α) plus a free gain, fitted over 200 to 4500 Hz.

- Vowels: harmonic amplitudes against the resonators' own envelope.
- Sung phrases: the harmonic-peak envelope averaged over voiced frames.

**Formant at Pitch 0 moves the envelope by its setting.** Medians, in semitones:

| setting | -12 | -7 | -3 | +3 | +7 | +12 |
|---|---|---|---|---|---|---|
| vowels | -11.97 | -7.02 | -2.97 | +3.00 | +7.06 | +11.93 |
| sung | n/a | -6.68 | -2.90 | +3.25 | +7.37 | +13.06 |

Envelope error after the shift is 0.8 to 2.7 dB (vowels) and 1.1 to 2.5 dB (sung). Without the shift it is 4.9 to 9.9 dB.

**Pitch alone moves the envelope a little.** Formants are only partly preserved. The marker probes (item 7) measure
the grain time scale directly:

- β(P) = clip(0.418 P, ±2.51) semitones, rms 0.012 st over 16 settings from -12 to +12.
- The same at f0 250 Hz. The slope is 5/12 to within the fit; β saturates at |P| ≥ 6.
- The envelope fits agree. Vowels: -2.49 (-12), -2.70 (-7), -2.01 (-5), -0.64 (-1), +0.46 (+1), +2.04 (+5),
  +2.32 (+7), +2.18 (+12). Sung: -2.46, -2.22, -1.83, +2.59, +2.83. The sung +12 fit is unreliable: the harmonics
  are too sparse.

**The two add.** The envelope and grain time-scale shift is F + β(P):

| P / F | probe or envelope | F + β |
|---|---|---|
| +5 / -5 | grain scale -2.92 | -2.92 |
| -7 / +7 | envelope 4.38 | 4.49 |
| +5 / +5 | envelope 6.99 | 7.08 |
| -7 / -7 | envelope -9.70 | -9.51 |
| +12 / -12 | envelope -9.15 | -9.49 |

Error at the fitted α is 1.2 to 4.2 dB under Pitch. The shape is not perfectly kept: the grain window smears it.

## 3. Level (`level.json`; output rms minus input rms, dB, from 0.5 s on)

- **No shift:** 0.00 on voice and vowels, -0.02 on noise.
- **Voiced material:**

  | setting | output minus input (dB) |
  |---|---|
  | P -12 | -1.9 .. -3.2 |
  | P -7 | -0.6 .. -2.8 |
  | P ±1 | -1.2 .. +0.3 |
  | P +5 | -1.1 .. +1.0 |
  | P +12 | +0.4 .. +1.8 |
  | F +7 | -2.0 .. -2.6 |
  | F +12 | -2.4 .. -3.5 |
  | F -7 | -0.8 .. +1.4 |
  | F -12 | -0.1 .. -1.7 |
  | -7 / +7 | -2.4 .. -3.6 |

- **Noise and sibilant band:** -0.6 at every setting except P ±1, which is +1.7 to +1.9.
- **Level-independent:** the /a/ vowel from -40 to -6 dBFS changes by 0.3 dB at most per setting. The shifter is
  homogeneous.
- **Impulses at no shift are the exception.** An isolated click's kernel sum is 0.998 at amplitude 0.005, 0.931 at 0.2
  and 0.844 at 1.0. A bare 200 Hz pulse train has gain 0.91 (0.02) down to 0.56 (0.5).

## 4. Delay and timing (`noshift.json`, `timing.json`, `structure.json`)

**At Pitch 0 / Formant 0 the wet path is a fixed fractional delay:**

| sample rate | delay | ms | the fraction |
|---|---|---|---|
| 48 kHz | 105.531 samples | 2.1986 | 17/32 |
| 96 kHz | 211.063 samples | 2.1986 | 1/16 |
| 44.1 kHz | 120.438 samples | 2.731 | 7/16 |

- **It is independent of everything we varied:** f0 (100 to 600 Hz), material (vowels, sung, vibrato, glide, noise),
  level, and Fine on / off.
- **What is left after the delay** (48 kHz): -57 to -68 dB on voice and vowels, -81 to -87 dB at 96 kHz.
- **Noise leaves more:** -16.6 dB, almost all above 20 kHz. The interpolator is flat within ±0.3 dB to 22 kHz and
  -8.4 dB at 23.5 kHz. A 64-tap FIR explains noise to -35.8 dB; its main taps are 0.606 / 0.693 at 105 / 106.
- **The 44.1 kHz exception:** the 600 Hz vowel leaves -25 dB there.
- **The earlier "106 samples, correlation 0.69, +3 dB" on noise** came from an integer lag compared with a half-sample
  delay. At 105.53 the correlation is 0.987.
- **The reference reports 0 latency.** At Mix 50 the dry signal and the 2.2 ms wet path therefore form a comb.
  Derived, not measured: the first notch would be at about 227 Hz.

**When shifted, the read delay moves** (marker probes on a 150 Hz vowel):

| setting | read delay (samples) |
|---|---|
| +12 | 128 .. 337 |
| +5 | 154 .. 404 |
| +1 | 137 .. 173 |
| -1 | 203 .. 887 |
| -7 | 149 .. 356 |
| -12 | 215 .. 294 |
| F +7 | 366 .. 518 |

Onset delay (the 32-sample rms reaching -20 dB under steady):

| material | -7 | 0 | +5 / +12 |
|---|---|---|---|
| vowels | 117 .. 128 | 105 .. 106 | 69 .. 83 |
| noise and band bursts | 127 .. 131 | 105 .. 106 | 76 .. 80 |

**Shifted onsets fade in.** The first 20 ms of the output are 3.6 to 6.7 dB lower, relative to steady, than the
input's first 20 ms (vowels). Noise bursts are 4.5 to 4.7 dB lower. Unshifted onsets match the input within 0.2 dB.

## 5. Unvoiced input, silence, chords (`unvoiced.json`)

**Silence → exactly 0** at every setting.

**Noise is re-granulated, not passed through.**

- At any shift the output is uncorrelated with the delayed input (|r| ≤ 0.02). P ±1 is the exception: r ∓0.28 / 0.30
  and the level +1.8 dB.
- The level is -0.6 dB.
- The spectrum moves by the grain time scale F + β, not by the pitch ratio. Band 4 to 10 kHz, centroid 6924 Hz:
  - 8030 at P +12 (+2.56 st)
  - 7784 at P +5 (+2.03)
  - 6003 at P -7 and P -12 (-2.47)
  - 10377 / 4622 at F ±7 (±7.0)
- Noise bursts at a shift come out 3.3 dB lower overall, with grain tails 44 dB down between the bursts.

**Chords are mostly not shifted.** Shares of the energy:

| chord | +5 | +12 | -7 |
|---|---|---|---|
| two sines 220 + 330 Hz (common f0 110) | 97 % at the shifted partials | 37 % shifted | 36 % shifted |
| harmonic triad C3-E3-G3 | 18 % shifted, 79 % at the original partials | 21 % shifted, 74 % at the original | 52 % shifted |
| A2 + E3 | 14 % shifted, 84 % at the original | 19 % shifted, 77 % at the original | 58 % shifted |

The level drops 0.8 to 1.9 dB.

**There is a gate.**

- A 150 Hz vowel is shifted from -50 dBFS rms up and not shifted at -52 or below. Below the gate the output keeps the
  input's f0, at -1.2 to -1.8 dB.
- In white noise the shift holds down to 10 dB SNR (p90 ≤ 2.2 c, no gross errors). At 5 dB SNR it fails (gross
  0.56 to 0.62).

**Failure cases:**

- **A bare pulse train** (flat spectrum, no formants) is not shifted: the output f0 is a sub-harmonic, the level -10 dB.
- **A single narrow resonance** (bandwidth 60 Hz at 1 kHz, f0 100 or 200 Hz) is mistracked: wrong shifts or none,
  gross 1.0. The same at 500 Hz with f0 100 Hz. With a 200 Hz bandwidth, or with three formants, the shift is exact.

## 6. Flatten (`flatten.json`; Flatten on through the state, notes 0 5 12 21 23)

**Note naming.** Note 0, which the editor calls C2, is **130.81 Hz** (MIDI 48, A4 = 440 Hz). This is the naming where
middle C (261.63 Hz) is C3. Note 23 ("B3") is 493.9 Hz.

| input | output against the note |
|---|---|
| steady vowels 110 .. 300 Hz | median within ±0.16 c, p90 ≤ 1.13 c (≤ 0.14 c at 150 / 300 Hz) |
| vowel with 50 c vibrato | p90 2.8 .. 8.5 c: the vibrato is mostly removed |
| glide 110 → 330 Hz | median +1.9 .. +3.7 c, p90 3.2 .. 6.2 c (a small lag) |
| sung phrases | p90 1.8 .. 6.5 c |

- **Lock time after an onset from silence:** 26 to 54 ms.
- **Pitch adds on top of Flatten:** note C3 (261.63) with Pitch +500 c gives 349.23 Hz (median 0.03 c).

## 7. Structure (`structure.json`, `beta.json`, `structure_marker_maps.png`)

The probe: a 0.02 click is added to a steady /a/ at one position per render, and the render without it is
subtracted. Each copy of the click in the output shows where that input instant is read, and how strongly. Before
the click the difference is exactly 0, and the copies are click-shaped.

- **No shift.** One copy at 105.53 samples, the kernel above. The gain is 0.985 to 1.003 at any phase in the period.
- **Pitch-synchronous overlap-add with time-scaled grains.**
  - Copies of one input instant repeat at the output period. At +12 (P_out 160) every instant appears twice, 160
    samples apart. At -12 some input stretches are not read at all (a grain is skipped).
  - Within a grain the delay changes linearly with the marker position. The slope gives the grain time scale:
    output samples per input sample 0.865 at +12 (2.50 st) and 1.155 at -7 (-2.50 st).
  - At P 0 / F +7 the slope is 0.667 (7.01 st): the formant shifter is the same mechanism.
  - A grain spans about 1.4 input periods (about 456 input samples at +12, 150 Hz). Successive grains crossfade over
    about 220 input samples (0.7 period).
  - The grain sum is not flat across the period: copies per marker sum to 0.66 to 1.5 per output period, depending on
    the phase.
- **Inharmonic content follows the grain scale, not the pitch.** A small tone (0.003) on a /u/ vowel:
  - The response's centroid moves +2.17 / +2.48 st (3070 Hz, P +5 / +12), -2.49 st (-7), and +1.99 / +2.69 / -2.60 st
    at 5070 Hz.
  - Under 1.1 % of the energy stays within 300 Hz of the tone, and none at tone × ratio.
  - It is spread into lines |f0_out - f0_in| apart: 50 Hz at ±5 / -7 on a 150 Hz vowel.
- **The grain placement depends on the signal.** At -12, 38 of 80 single 0.02 clicks moved it, leaving a
  vowel-waveform difference instead of click copies; at the other settings 0 to 2 did. It also depends on the pitch
  detection: see the pulse-train, narrow-resonance and octave-down cases.

## Proposed behaviour metrics (a scorecard for our shifter, each against the reference's number)

| # | metric | the reference | threshold for ours |
|---|---|---|---|
| M1 | pitch, vowel grid (147 cases): per-case median / p90 / gross | ±0.41 c / ≤ 1.01 c / 0 | ≤ 1 c / ≤ 3 c / 0 |
| M1b | pitch, sung phrases: median p90, max gross | 5.96 c, 0.118 | ≤ 10 c, ≤ 0.15 |
| M2 | envelope shift α (st) against F + clip(5P/12, ±2.5), vowels | within ±0.3 st (median) | within ±0.5 st; envelope error ≤ reference + 1 dB |
| M3 | level per (P, F) and material (`level.json` table) | as tabled | within ±1 dB of the table |
| M4 | no-shift delay; onset delay; settle after onset / after a 3-semitone jump | 105.53 samples; table; 15 .. 41 ms / 29 .. 40 ms | ±0.05 samples; ±20 samples; ≤ reference + 10 ms |
| M5 | unvoiced: level, spectral centroid shift | -0.6 dB, F + β st | ±1 dB, ±0.5 st |
| M6 | Flatten: steady median, vibrato p90, lock time | ±0.16 c, ≤ 8.5 c, 26 .. 54 ms | ≤ 1 c, ≤ 10 c, ≤ 60 ms |
| M7 | gate and robustness: no-shift threshold, SNR without gross errors | -51 dBFS rms, 10 dB | -51 ± 3 dBFS, ≤ 10 dB |
| M8 | behaviours to copy or decide against deliberately: octave-down hysteresis, chords left mostly unshifted, noise re-granulated, ±1 noise +1.8 dB | as measured | a written decision each |

The thresholds are proposals, set wider than the tracker's floor and the spread across materials.

## Proposed classification of the stages

| stage | class | the measurement that decided it |
|---|---|---|
| Mix crossfade, Bypass | exact | known: linear dry + m (wet - dry) |
| No-shift path: fractional delay and interpolator | exact | delay 105.53125 / 211.0625 / 120.4375 samples (1/32-sample values), -57 .. -87 dB on voice; the kernel fits noise to -35.8 dB |
| Output pitch ratio | exact | ≤ 0.41 c median at every f0, shift, rate and in cents |
| Grain time scale (formant under Pitch) and Formant | exact | clip(0.418 P, ±2.51) + F, rms 0.012 st; the same at 150 and 250 Hz; additive (+5 / -5 → -2.92) |
| Flatten target | exact | MIDI 48 + note at A4 = 440 Hz, ≤ 0.16 c; Pitch adds on top |
| Grain length, crossfade, window and gain ripple | fittable | the marker maps give the copies' amplitude against position; a parametric window fit is the next step |
| Level per setting | fittable | homogeneous in level, a stable table per (P, F); the per-material spread is ±1.5 dB |
| Detector timing (onset 15 .. 41 ms, jump 29 .. 40 ms, gate -51 dBFS, vibrato lag 0 .. 7 ms) | fittable | repeatable numbers per f0; the internals are not needed to match them |
| Detector decisions on hard material (octave hysteresis, narrow resonance, pulse trains, chords, processed voice) | random-or-proprietary (behaviour only) | mistracking is gross and signal-specific; match it statistically (M1b, M7, M8), not sample for sample |
| Grain placement (pitch marks) | random-or-proprietary | a 0.02 click moves it (38 of 80 at -12); outputs cannot be expected to correlate once shifted |
| Unvoiced and noise output | random (statistics fittable) | uncorrelated with the input at any shift; level and centroid follow the rules above |
| Click / impulse level dependence at no shift | learnable (low priority) | kernel sum 0.998 → 0.844 from amplitude 0.005 to 1.0; tonal and noise input are linear |

## 8. Added in Phase 3 (2026-11-03): level per material, the time scale's limit, speech

**The voiced level follows the waveform's peakiness** (`nulltest/measure_pitchbend_level_grid.py`, `levelgrid.json`).
Pitch -12 .. +12 by Formant -12 .. +12 (142 settings) over 20 materials: vowels /a/ /i/ /u/ at 80, 150, 220, 300 Hz,
/a/ with its formant bandwidths halved and doubled, 4 sung and 2 spoken phrases.

- At one setting the level moves with the material by 0.76 dB rms (median over the settings), up to 3.5 dB.
- Most of it follows the input's kurtosis (E x^4 / (E x^2)^2 over 20 ms, the windows' logs weighted by their energy):
  per setting a straight line in ln kurtosis leaves 0.40 dB rms (median). The slope is -1 .. -2.7 dB per unit at
  most settings: a low voice or wide formants (a peakier waveform) comes out quieter. At +5 the /a/ vowel goes from
  -2.6 dB at 80 Hz to +0.6 at 440 Hz; at 110 Hz it is -0.5 with the bandwidths halved and -3.5 with them doubled.
- f0 as a second variable takes off 0.0 .. 0.13 dB more.
- The marker probes agree: the copies' gain is lowest just after the excitation pulse, where a peaky waveform has its
  energy (0.65 .. 0.9 there against 1.4 just before the pulse at +5 and -7).
- Not explained by it: where F + beta equals Pitch (+5 / +3, -5 / -3) the level drops by about 3 dB; at Pitch +12
  with Formant -3 .. -5, 300 Hz vowels drop by 4.5 .. 7 dB (220 and 400 Hz do not).

**The grain time scale stops at +-12 semitones.** Pitch +12 / Formant +12 (F + beta = 14.5) sounds as F + beta = 12:
our shifter's long-term spectrum against the reference's is 1.98 dB rms (sung) and 1.30 (spoken) read at 12, 3.94
and 3.33 read at 14.5; the same at -12 / -12 (0.53 and 1.10 at -12, 3.17 and 3.29 at -14.5) and 5 / 12.

**Speech and the voicing decision** (the spoken take, Pitch 0, fixed Formant):

- Frames (10 ms) whose output correlates with the delayed input above 0.9 (passed): 0 .. 36 of 795 on the reference,
  46 .. 59 on ours, at a median input aperiodicity of 0.2 .. 0.4 (voiced but unclear); the reference shifts or re-granulates them.
- The pitch tracks of the two outputs, both clearly voiced, part by more than 50 cents in 0.4 .. 6.5 % of the frames at
  fixed settings.
- On a fast glide (146 to 98 Hz over 0.25 s) the reference's output follows the input within 0 .. 7 cents at Formant
  -12 .. -1, but trails it with the Formant up (+23 c at +1, +34 at +6, +47 at +12); on a 120 Hz vowel with 100 cents
  of vibrato it lags 1 .. 2 ms at Formant -6 and 0 and 13 ms at +6 (ours 11 .. 14 ms at all three).
- With the Formant moved by an LFO (Drawl) the same glide comes out 30 .. 50 cents under the input on the reference,
  which none of the fixed settings does.

## Not measured

- The modulators (LFO, step, AM, PT).
- Mix at other values against shifted output.
- Stereo linking (everything here is the Mono component).
- The grain time scale for cents values with Fine on (only whole semitones were probed).
- Structure probes at 44.1 / 96 kHz.
- Timing at 44.1 / 96 kHz.
- Pitch accuracy below 80 Hz input, and above 600 Hz input.
