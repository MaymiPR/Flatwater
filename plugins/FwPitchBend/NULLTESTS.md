# Flatwater Pitch Bend: null tests

Flatwater Pitch Bend against the reference (its Stereo component), both at 48 kHz unless said. With every control at 0
both pass the voice through with the same delay, and that is null-tested sample for sample. Once anything shifts,
Flatwater Pitch Bend is its own pitch shifter (a pitch detector and overlap-added grains, not a copy), so the shifter
is judged by what is heard (pitch, envelope, level, timing) on the scorecard below, and the modulators, which are exact
laws, are null-tested through the Mix control. How each part was identified is in [MEASUREMENTS.md](MEASUREMENTS.md).

## Settings

### Summary

| part | cases | result |
|---|---:|---|
| No shift (every control at 0): the delayed voice | sung and spoken takes, vowels | −67 dB on the sung take; −57.9 dB worst on voice, the delay within 0.0002 samples (scorecard M4) |
| The modulators (M1, M2, AM, PT; their laws, timing and modulation of each other), read through Mix | 38 judged, 1 reported | 38 of 38 within their targets (0.003 rms for an exact law away from jumps, 0.01 for a fitted one) |
| The shifter's scorecard: pitch accuracy, formant envelope, level, onsets and jumps, noise, Flatten, the gate | 12 judged measures (one more reported) | 11 of 12 within their thresholds (the miss: a few level cells where the reference's own level varies by more than 1 dB within one setting) |
| The 40 factory presets on a sung and a spoken take: level, pitch track and tone | 80 pairs | 62 of 80 meet every target (67 with the reference's own LFO tables loaded into ours) |
| Sample nulls with a shift, a steady vowel | Formant 0.01 … ±6 | −27 dB at a time scale of 1 (−31 at 200 Hz), −13 … −19 dB at Formant +0.5 … +2 |
| Sample nulls with a shift, a voice | sung take | no cancellation (the reference resynthesises the voice from its own period estimates) |

### The modulators

`plugins/FwPitchBend/nulltest/null_mods.py` (`measurements/FwPitchBend/mods/null.json`): each case's control value over time,
read through Mix on both sides, ours against the reference; rms after 0.1 s, over the whole curve and away from its
jumps (more than 600 samples from a step or a wrap). The cases are described in MEASUREMENTS.md, sections 3 to 6.

| case | class | rms | away from jumps | max | result |
|---|---|---|---|---|---|
| depth25 | exact | 0.0000 | 0.0000 | 0.0006 | pass |
| shape | exact | 0.0001 | 0.0001 | 0.0017 | pass |
| free3.7Hz | exact | 0.0004 | 0.0004 | 0.0026 | pass |
| beat90 | exact | 0.0002 | 0.0002 | 0.0018 | pass |
| phase.25 | exact | 0.0020 | 0.0001 | 0.1156 | pass |
| warp0.1 | exact | 0.0016 | 0.0001 | 0.1016 | pass |
| level0.5 | exact | 0.0001 | 0.0001 | 0.0005 | pass |
| seq4 | exact | 0.0032 | 0.0007 | 0.1160 | pass |
| smooth100 | fitted | 0.0002 | 0.0002 | 0.0018 | pass |
| am | fitted | 0.0018 | 0.0018 | 0.0639 | pass |
| am10_100 | fitted | 0.0006 | 0.0006 | 0.0091 | pass |
| am1_1000 | fitted | 0.0059 | 0.0059 | 0.0312 | pass |
| am100_10 | fitted | 0.0022 | 0.0022 | 0.0091 | pass |
| am1000_1 | fitted | 0.0019 | 0.0019 | 0.0142 | pass |
| am30_300 | fitted | 0.0005 | 0.0005 | 0.0032 | pass |
| am300_1 | fitted | 0.0008 | 0.0008 | 0.0077 | pass |
| am_gate | fitted | 0.0011 | 0.0009 | 0.0241 | pass |
| pt_glide | fitted | 0.0049 | 0.0049 | 0.0096 | pass |
| pt_sung | fitted | 0.0058 | 0.0058 | 0.1369 | pass |
| pt_spoken | report | 0.0309 | 0.0309 | 0.2537 | report |
| mod_level | exact | 0.0001 | 0.0001 | 0.0010 | pass |
| mod_ratefree | exact | 0.0003 | 0.0003 | 0.0020 | pass |
| mod_ratesync | exact | 0.0004 | 0.0004 | 0.0022 | pass |
| mod_warp | exact | 0.0016 | 0.0001 | 0.1018 | pass |
| mod_phase | exact | 0.0020 | 0.0001 | 0.1156 | pass |
| mod_levelstep | exact | 0.0001 | 0.0001 | 0.0010 | pass |
| mod_ratestep_sync | exact | 0.0027 | 0.0003 | 0.1097 | pass |
| mod_ratestep_free | exact | 0.0007 | 0.0007 | 0.0054 | pass |
| mod_warpstep | exact | 0.0016 | 0.0001 | 0.1019 | pass |
| mod_phasestep | exact | 0.0021 | 0.0003 | 0.1363 | pass |
| mod_amlevel | fitted | 0.0004 | 0.0004 | 0.0061 | pass |
| mod_ptlevel | fitted | 0.0033 | 0.0033 | 0.0064 | pass |
| seq_warp | exact | 0.0019 | 0.0005 | 0.0770 | pass |
| smooth10 | fitted | 0.0007 | 0.0007 | 0.0164 | pass |
| smooth1000 | fitted | 0.0008 | 0.0008 | 0.0021 | pass |
| mod_amattack | fitted | 0.0008 | 0.0008 | 0.0057 | pass |
| mod_amrelease | fitted | 0.0009 | 0.0009 | 0.0090 | pass |
| pt_smooth300 | fitted | 0.0036 | 0.0036 | 0.0159 | pass |
| am_spoken | fitted | 0.0030 | 0.0030 | 0.0174 | pass |

PT on the spoken take is reported, not judged: two pitch detectors on speech disagree where the voice is rough (the
falling, creaky end of a phrase), which neither reproduces from the other.

### The shifter's scorecard

`plugins/FwPitchBend/nulltest/check_shifter.py` (`measurements/FwPitchBend/shifter/ours.json`; the reference's numbers in
`measurements/FwPitchBend/shifter/`). The same signals and measures on both shifters:

| metric | reference | ours | threshold | result |
|---|---|---|---|---|
| M1 vowel grid: worst median / worst p90 (c) / gross | 0.41 / 1.01 / 0 | 0.41 / 0.70 / 0 | ≤ 1 / ≤ 3 / 0 | pass |
| M1 at 44.1 and 96 kHz: worst median, worst p90 (c) | 0.06, 0.13 | 0.04, 0.05 | ≤ 1, ≤ 3 | pass |
| M1b sung phrases: median p90 (c) / max gross | 5.96 / 0.118 | 6.29 / 0.099 | ≤ 10 / ≤ 0.15 | pass |
| M2 envelope against F + β: worst error (st) / envelope error against the reference (dB) | 0.34 / n/a | 0.24 / +0.34 | ≤ 0.5 / ≤ +1 | pass |
| M3 level cells within ±1 dB | 143 cells | 139 of 143 (worst +1.35) | all | **miss** |
| M4 no-shift delay error (samples) | n/a | 0.0002 | ±0.05 | pass |
| M4 no-shift residual on voice (dB) | −57 … −68 | −57.9 worst (−82 … −92 sung) | < −55 | pass |
| M4 onset settle, worst against the reference | n/a | +4 ms | ≤ +10 | pass |
| M4 jump settle, worst against the reference | n/a | +5 ms | ≤ +10 | pass |
| M4 onset delay (report only, samples) | 69 … 131 | 105 … 106 | ±20 | 37 off |
| M5 noise level against −0.6 dB / centroid against F + β | n/a | 0.5 dB / 0.08 st | ±1 / ±0.5 | pass |
| M6 Flatten steady / vibrato p90 / lock | 0.16 c / 8.5 c / 54 ms | 0.23 c / 6.5 c / 45 ms | ≤ 1 / ≤ 10 / ≤ 60 | pass |
| M7 gate / SNR | −50 dBFS / 10 dB | −51 dBFS / 10 dB | −51 ± 3 / ≤ 10 | pass |

### The factory presets

The reference's 40 presets loaded into it against Flatwater Pitch Bend's (Flatwater names): level, the pitch track
where both outputs are clearly voiced, and the long-term spectrum, on a sung and a spoken take. Median pitch
difference 2.3 cents on the sung take and 6.5 on the spoken one. Every pair, and why the misses miss:
[Presets](#presets).

### Sample nulls with a shift

Click probes and pulse trains (MEASUREMENTS.md, section 11, Sample nulls with a shift) show where the reference's grains sit and what they read; ours read the same for Formant up to +1.5. On a
steady 150 Hz vowel the residual (raw, then with ours moved by a fraction of a sample and matched in level):

| setting | raw (dB) | lined up (dB) |
|---|---:|---:|
| Formant +0.01 | −27.2 | −46.9 |
| Formant +0.5 | −14.4 | −31.7 |
| Formant +1 | −12.9 | −25.5 |
| Formant −1 | −14.6 | −26.7 |
| Formant −3 | −6.6 | −13.7 |
| Formant −6 | −9.4 | −16.1 |
| Pitch +5 | −0.6 | −9.4 |

On a sung take, even at Formant +0.01, the reference's output against its own input, delayed, is +0.3 dB: it
rebuilds a moving voice grain by grain from its own pitch-period estimates, so no recreation that does not share its
detector's exact estimates nulls it; ours reaches about −7 dB per 100 ms window at best alignment.

## Presets

Each of the reference's 40 presets loaded into its state (its stored values, hidden ones included, and its two
shape tables), against Flatwater Pitch Bend's preset of the same settings (Flatwater names), both stereo at
48 kHz and 120 bpm from song position 0, on a sung and a spoken take of the vocal corpus (about 8 s each). Made by
`plugins/FwPitchBend/nulltest/null_presets.py`. Our shifter is our own method, so the outputs are compared by what is heard,
not sample by sample (after the first 0.1 s):

- **level**: the output's rms, ours minus the reference's, dB (target within 1.5);
- **pitch**: where both outputs are clearly voiced, the median difference of their pitch tracks in cents (target 15),
  and the share of frames more than 50 cents apart (target 10 %);
- **tone**: the rms difference of the long-term spectra, 1/6 octave from 100 Hz to 8 kHz, levels matched, dB
  (target 3);
- **null**: the output residual, dB, for the record.

A preset marked *shape substituted* uses one of our own LFO shapes where the reference's is a drawn one (we do
not ship those); its second line is the same preset with the reference's table loaded into ours, which shows how
much of the difference is the shape.

In a blend (Mix well under 100 %) the pitch column measures which of the two voices a tracker hears frame by
frame, which is decided where they are close in level; MEASUREMENTS.md (Factory presets) checks those presets'
modulation directly.

62 of 80 preset and take pairs meet every target.

### The sung take

| preset | level | pitch | tone | null |  |
|---|---|---|---|---|---|
| Character / Cartoon | -0.0 | 0.6 c / 0 % | 2.0 | 1.7 | pass |
| Character / Deeper Voice | +0.2 | 0.6 c / 0 % | 1.0 | 1.1 | pass |
| Character / Higher Register | +0.0 | 1.2 c / 0 % | 1.6 | 3.0 | pass |
| Character / Groggy Voice | +0.1 | 1.5 c / 0 % | 1.3 | 2.4 | pass |
| Character / Pushy | +0.1 | 1.2 c / 0 % | 1.2 | 4.3 | pass |
| Character / Tiny Voice | -0.1 | 0.7 c / 0 % | 2.5 | 3.1 | pass |
| Character / Younger | +0.4 | 0.7 c / 0 % | 1.5 | 4.8 | pass |
| Harmony / Bass Backing | +0.0 | 0.2 c / 1 % | 1.0 | -6.3 | pass |
| Harmony / Fifth Above Blend | +0.1 | 0.5 c / 0 % | 1.0 | -1.0 | pass |
| Harmony / Fourth Above | -0.0 | 0.7 c / 0 % | 1.7 | 0.8 | pass |
| Harmony / Octave Below Blend | +0.0 | 1 c / 0 % | 1.0 | 0.5 | pass |
| Double / Stutter Twin (shape substituted: M1) | +0.1 | 15.2 c / 5 % | 2.2 | 0.3 | **miss** |
| &nbsp;&nbsp;with the reference's shape | +0.0 | 9.2 c / 1 % | 2.2 | -0.2 | pass |
| Double / Loose Double | -0.1 | 3.4 c / 1 % | 0.8 | -5.7 | pass |
| Double / Tape Double | +0.0 | 3.1 c / 1 % | 1.3 | -3.6 | pass |
| Double / Thin Double | +0.8 | 0.8 c / 1 % | 2.7 | -1.2 | pass |
| Double / Tracked Double High | -0.1 | 3.6 c / 1 % | 1.2 | -5.3 | pass |
| Double / Tracked Double Low | +0.2 | 3.1 c / 1 % | 1.0 | -3.6 | pass |
| Movement / Wavering Voice | +0.4 | 2.3 c / 1 % | 0.7 | 2.0 | pass |
| Movement / Alarm | -0.5 | 8.9 c / 1 % | 1.5 | 2.7 | pass |
| Movement / Breathing Formant | +0.1 | 0.7 c / 0 % | 1.0 | 0.7 | pass |
| Movement / Deep Warble | -0.1 | n/a | 2.4 | 3.2 | pass |
| Movement / Drawl | -0.1 | 4.7 c / 1 % | 1.6 | 2.6 | pass |
| Movement / Fan Wobble | +0.3 | 0 c / 0 % | 1.6 | -3.5 | pass |
| Movement / Pitch Follower Down (shape substituted: M2) | +0.1 | 10.1 c / 0 % | 2.1 | 2.9 | pass |
| &nbsp;&nbsp;with the reference's shape | +0.1 | 10.2 c / 0 % | 1.2 | 2.7 | pass |
| Movement / Shape Shifter | -0.2 | 0.9 c / 0 % | 1.5 | 1.0 | pass |
| Movement / Shiver | +0.8 | 2.6 c / 0 % | 3.9 | 1.1 | **miss** |
| Movement / Submerged (shape substituted: M1) | -0.2 | 1.2 c / 1 % | 1.5 | 0.4 | pass |
| &nbsp;&nbsp;with the reference's shape | -0.1 | 1 c / 0 % | 0.9 | 1.3 | pass |
| Movement / Warped Tape (shape substituted: M1) | +0.3 | 1.9 c / 17 % | 3.6 | 1.2 | **miss** |
| &nbsp;&nbsp;with the reference's shape | +0.3 | 1.3 c / 1 % | 2.0 | 3.3 | pass |
| Movement / Wobble (shape substituted: M1) | -0.1 | 48 c / 46 % | 2.6 | 3.0 | **miss** |
| &nbsp;&nbsp;with the reference's shape | +0.0 | 3.1 c / 0 % | 1.6 | 1.5 | pass |
| Robot / Alien Steps (shape substituted: M2) | -0.1 | 3.7 c / 2 % | 2.9 | 1.4 | pass |
| &nbsp;&nbsp;with the reference's shape | -0.1 | 3.6 c / 2 % | 1.7 | 3.5 | pass |
| Robot / Arcade Steps (shape substituted: M2) | -0.6 | 35.4 c / 20 % | 3.3 | 3.0 | **miss** |
| &nbsp;&nbsp;with the reference's shape | -0.5 | 10 c / 3 % | 3.1 | 3.6 | **miss** |
| Robot / Glide Robot | +1.4 | 10.3 c / 0 % | 1.8 | 3.9 | pass |
| Robot / Vinyl Scrub | +0.4 | 8.9 c / 0 % | 1.0 | 3.1 | pass |
| Robot / Stepping Robot (shape substituted: M2) | -1.1 | 3.6 c / 3 % | 3.9 | 3.0 | **miss** |
| &nbsp;&nbsp;with the reference's shape | -0.8 | 4.3 c / 0 % | 2.2 | 3.3 | pass |
| Sequence / Chopped Melody (shape substituted: M2) | +0.8 | 6.3 c / 1 % | 2.0 | 3.5 | pass |
| &nbsp;&nbsp;with the reference's shape | +1.2 | 6 c / 1 % | 1.7 | 3.4 | pass |
| Sequence / Long Journey (shape substituted: M2) | +0.4 | 11.1 c / 3 % | 2.9 | 2.9 | pass |
| &nbsp;&nbsp;with the reference's shape | -0.0 | 11.6 c / 4 % | 1.7 | 2.9 | pass |
| Sequence / Lullaby Steps | -0.3 | 11.8 c / 0 % | 1.0 | 2.3 | pass |
| Sequence / Plain Sequence | -0.7 | 0.9 c / 23 % | 3.6 | -1.2 | **miss** |
| Sequence / Rising Steps | -0.3 | 8.7 c / 1 % | 0.9 | 2.7 | pass |
| Sequence / Twin Sequences | -0.5 | 1.1 c / 1 % | 1.1 | 1.0 | pass |

### The spoken take

| preset | level | pitch | tone | null |  |
|---|---|---|---|---|---|
| Character / Cartoon | +0.3 | 2.3 c / 0 % | 1.4 | 2.2 | pass |
| Character / Deeper Voice | +0.7 | 2.4 c / 0 % | 0.8 | 1.4 | pass |
| Character / Higher Register | +0.7 | 6.5 c / 1 % | 2.2 | 3.7 | pass |
| Character / Groggy Voice | -1.1 | 2.8 c / 1 % | 1.1 | 2.6 | pass |
| Character / Pushy | -0.2 | 6.4 c / 0 % | 0.8 | 3.6 | pass |
| Character / Tiny Voice | -0.2 | 5.8 c / 1 % | 1.8 | 2.7 | pass |
| Character / Younger | +0.4 | 1.6 c / 0 % | 0.6 | 0.6 | pass |
| Harmony / Bass Backing | +0.0 | 1.9 c / 20 % | 0.3 | -8.8 | **miss** |
| Harmony / Fifth Above Blend | -0.0 | 2.1 c / 3 % | 0.4 | -3.2 | pass |
| Harmony / Fourth Above | +0.0 | n/a | 0.7 | 0.4 | pass |
| Harmony / Octave Below Blend | -0.8 | 2 c / 0 % | 1.1 | -0.9 | pass |
| Double / Stutter Twin (shape substituted: M1) | +0.4 | 16.9 c / 8 % | 1.2 | 0.5 | **miss** |
| &nbsp;&nbsp;with the reference's shape | +0.2 | 15.3 c / 5 % | 0.7 | 0.4 | **miss** |
| Double / Loose Double | +0.2 | 3.9 c / 1 % | 0.4 | -5.1 | pass |
| Double / Tape Double | -0.4 | 4.1 c / 12 % | 1.2 | -3.8 | **miss** |
| Double / Thin Double | -0.6 | 1.6 c / 3 % | 0.7 | -5.5 | pass |
| Double / Tracked Double High | -0.1 | 2.4 c / 2 % | 0.6 | -5.3 | pass |
| Double / Tracked Double Low | -0.3 | 3.5 c / 10 % | 0.9 | -4.3 | pass |
| Movement / Wavering Voice | +0.0 | 8.1 c / 0 % | 1.0 | 3.8 | pass |
| Movement / Alarm | -0.7 | 14.5 c / 6 % | 1.1 | 2.8 | pass |
| Movement / Breathing Formant | +0.4 | 4.1 c / 0 % | 0.5 | 1.2 | pass |
| Movement / Deep Warble | +0.4 | n/a | 2.0 | 3.2 | pass |
| Movement / Drawl | +0.3 | 15.9 c / 7 % | 0.8 | 3.5 | **miss** |
| Movement / Fan Wobble | -0.0 | 0 c / 0 % | 0.3 | -4.4 | pass |
| Movement / Pitch Follower Down (shape substituted: M2) | -0.7 | 34.5 c / 24 % | 2.4 | 2.7 | **miss** |
| &nbsp;&nbsp;with the reference's shape | -0.9 | 35.5 c / 24 % | 2.0 | 2.4 | **miss** |
| Movement / Shape Shifter | -0.3 | 12.8 c / 6 % | 1.2 | 3.0 | pass |
| Movement / Shiver | -0.0 | 10.3 c / 5 % | 2.3 | 1.7 | pass |
| Movement / Submerged (shape substituted: M1) | +0.6 | 4.4 c / 2 % | 1.0 | 0.9 | pass |
| &nbsp;&nbsp;with the reference's shape | +0.5 | 4.5 c / 2 % | 0.7 | 0.7 | pass |
| Movement / Warped Tape (shape substituted: M1) | +0.0 | 3.2 c / 6 % | 1.2 | 3.3 | pass |
| &nbsp;&nbsp;with the reference's shape | -0.0 | 3.6 c / 0 % | 0.6 | 3.3 | pass |
| Movement / Wobble (shape substituted: M1) | +0.4 | 51.2 c / 54 % | 1.0 | 3.4 | **miss** |
| &nbsp;&nbsp;with the reference's shape | +0.5 | 7.7 c / 2 % | 0.7 | 2.0 | pass |
| Robot / Alien Steps (shape substituted: M2) | +0.5 | 9.3 c / 8 % | 2.0 | 3.4 | pass |
| &nbsp;&nbsp;with the reference's shape | +0.8 | 9 c / 9 % | 1.4 | 3.5 | pass |
| Robot / Arcade Steps (shape substituted: M2) | -1.1 | 24 c / 9 % | 0.9 | 2.4 | **miss** |
| &nbsp;&nbsp;with the reference's shape | -1.0 | 18.6 c / 4 % | 0.9 | 2.5 | **miss** |
| Robot / Glide Robot | +1.1 | 24.5 c / 25 % | 1.2 | 3.5 | **miss** |
| Robot / Vinyl Scrub | +0.2 | 22.5 c / 23 % | 0.9 | 3.1 | **miss** |
| Robot / Stepping Robot (shape substituted: M2) | -0.2 | 10.5 c / 3 % | 2.4 | 3.5 | pass |
| &nbsp;&nbsp;with the reference's shape | -0.1 | 10.5 c / 6 % | 1.6 | 3.0 | pass |
| Sequence / Chopped Melody (shape substituted: M2) | +0.3 | 12 c / 6 % | 1.8 | 2.7 | pass |
| &nbsp;&nbsp;with the reference's shape | +0.7 | 10.9 c / 8 % | 1.5 | 3.0 | pass |
| Sequence / Long Journey (shape substituted: M2) | +0.7 | 15.3 c / 9 % | 2.2 | 3.2 | **miss** |
| &nbsp;&nbsp;with the reference's shape | +0.2 | 15.2 c / 7 % | 1.0 | 3.0 | **miss** |
| Sequence / Lullaby Steps | +0.5 | 10.8 c / 4 % | 1.6 | 4.0 | pass |
| Sequence / Plain Sequence | -0.9 | 61.3 c / 75 % | 1.5 | -1.3 | **miss** |
| Sequence / Rising Steps | -0.3 | 7.4 c / 1 % | 0.8 | 2.9 | pass |
| Sequence / Twin Sequences | -0.0 | 2.6 c / 2 % | 0.8 | 3.4 | pass |
