# Flatwater Pitch Bend: null tests

Flatwater Pitch Bend against the reference (its Stereo component), both at 48 kHz unless said. With every control at 0
the two are compared sample for sample ([No shift](#no-shift)): a null on the sung take (-68.8 dB) and close on
steady vowels (-53.7 to -58.0 dB; one of the four a null at -60.6), but not a null on the spoken take (+3.6 dB), where the two delay the voice by
different times (67 samples apart). Once anything shifts,
Flatwater Pitch Bend is its own pitch shifter (a pitch detector and overlap-added grains, not a copy), so the shifter
is judged by what is heard (pitch, envelope, level, timing) on the scorecard below, and the modulators, which are exact
laws, are null-tested through the Mix control. How each part was identified is in [MEASUREMENTS.md](MEASUREMENTS.md).

## Settings

### Summary

| part | cases | result |
|---|---:|---|
| No shift (every control at 0), ours against the reference sample for sample | 7 | the sung take -68.8 dB (a null); steady vowels -53.7 to -58.0 dB (close) and -60.6 dB (a null); white noise -27.2 dB (a model); the spoken take +3.6 dB (not a null: the two are 67 samples apart there). Ours against its own delayed input: -57.9 dB worst on voice, the delay within 0.0002 samples (scorecard M4) |
| The modulators (M1, M2, AM, PT; their laws, timing and modulation of each other), read through Mix | 38 judged, 1 reported | 38 of 38 within their targets (0.003 rms for an exact law away from jumps, 0.01 for a fitted one) |
| The shifter's scorecard: pitch accuracy, formant envelope, level, onsets and jumps, noise, Flatten, the gate | 22 judged rows, 13 reported | 20 of 22 within their thresholds (the misses: M3, a few level cells where the reference's own level varies by more than 1 dB within one setting) |
| The 40 factory presets on a sung and a spoken take: level, pitch track and tone | 80 pairs | 62 of 80 meet every target (67 with the reference's own LFO tables loaded into ours) |
| Sample nulls with a shift, a steady 150 Hz vowel | 7 settings | -26.7 dB at Formant +0.01, -12.7 to -14.4 dB at Formant +0.5, +1 and -1, as rendered (not a null) |
| Sample nulls with a shift, a voice | sung take | no cancellation (the reference resynthesises the voice from its own period estimates; measured before the 1.0.0 release and not reproduced by a script in this tree) |

### No shift

Flatwater Pitch Bend at its defaults (Pitch 0, Formant 0, Mix 100, no modulation) against the reference at the same settings, sample for sample (Stereo, 48000 Hz; the residual after the first 0.1 s, as rendered, dB re the reference output; the lag is the shift of ours against the reference in samples). Produced by `plugins/FwPitchBend/nulltest/null_noshift.py` (`measurements/FwPitchBend/noshift_null.json`).

| case | residual (dB) | lag (samples) |
|---|---:|---:|
| sung take | -68.8 | 0 |
| spoken take | 3.6 | 67 |
| vowel, 100 Hz | -57.9 | 0 |
| vowel, 150 Hz | -60.6 | 0 |
| vowel, 300 Hz | -53.7 | 0 |
| vowel, 150 Hz, vibrato 100 cents | -58.0 | 0 |
| white noise, -18 dBFS rms | -27.2 | 0 |

median -57.9 dB, best -68.8, worst 3.6; 7 cases

Reproduce: `python -I plugins/FwPitchBend/nulltest/null_noshift.py --save`

### The modulators

`plugins/FwPitchBend/nulltest/null_mods.py` (`measurements/FwPitchBend/mods/null.json`): each case's control value over time, read through Mix on both sides, ours against the reference; rms after 0.1 s, over the whole curve and away from its jumps (more than 600 samples from a step or a wrap), in parts of the control's range. An exact law is judged away from the jumps (target 0.003 rms), a fitted one over the whole curve (0.01). The cases are described in MEASUREMENTS.md, sections 3 to 6.

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
| am10_100 | fitted | 0.0006 | 0.0006 | 0.0091 | pass |
| am1_1000 | fitted | 0.0059 | 0.0059 | 0.0312 | pass |
| am100_10 | fitted | 0.0022 | 0.0022 | 0.0091 | pass |
| am1000_1 | fitted | 0.0019 | 0.0019 | 0.0142 | pass |
| am30_300 | fitted | 0.0005 | 0.0005 | 0.0032 | pass |
| am300_1 | fitted | 0.0008 | 0.0008 | 0.0077 | pass |
| am_spoken | fitted | 0.0030 | 0.0030 | 0.0174 | pass |

38 of 38 judged cases within their targets; 1 reported, not judged.

PT on the spoken take is reported, not judged: two pitch detectors on speech disagree where the voice is rough (the falling, creaky end of a phrase), which neither reproduces from the other.

Reproduce: `python -I plugins/FwPitchBend/nulltest/null_mods.py --save`

### The shifter's scorecard

`plugins/FwPitchBend/nulltest/check_shifter.py` (`measurements/FwPitchBend/shifter/ours.json`; the reference's numbers in `measurements/FwPitchBend/shifter/`, from `measure_shifter.py`). The same signals and measures on both shifters; a row with no threshold met or missed is reported, not judged:

| metric | measure | reference | ours | threshold | result |
|---|---|---|---|---|---|
| M1 | 44.1/96k: \|median\| max, p90 max (c) | 0.06, 0.13 | 0.04, 0.05 | <= 1, <= 3 | pass |
| M1 | vowel grid abs p90 max (c) | 1.01 | 0.7 | <= 3 | pass |
| M1 | vowel grid gross max | 0 | 0 | 0 | pass |
| M1 | vowel grid \|median\| max (c) | 0.41 | 0.41 | <= 1 | pass |
| M1b | sung phrases: max gross | 0.118 | 0.099 | <= 0.15 | pass |
| M1b | sung phrases: median p90 (c) | 5.96 | 6.29 | <= 10 | pass |
| M2 | envelope err minus ref, worst (dB) | 0 | 0.34 | <= +1 | pass |
| M2 | envelope \|median - (F+beta)\| max (st) | 0.34 | 0.24 | <= 0.5 | pass |
| M3 | level cells within 1 dB of ref | 143 cells | 139 of 143 | all | **miss** |
| M3 | level worst diff (dB) | 0 | +1.35 +0/-12 | +-1 | **miss** |
| M4 | jump settle, worst minus ref (ms) | 0 | 5 | <= +10 | pass |
| M4 | no-shift delay error max (samples) | 0 | 0.0002 | <= 0.05 | pass |
| M4 | no-shift residual on voice, worst (dB) | -57 | -57.9 | < -55 | pass |
| M4 | onset delay, worst \|ours - ref\| | 0 | 37 | +-20 samples | report |
| M4 | onset settle, worst minus ref (ms) | 0 | 4 | <= +10 | pass |
| M5 | band 4-10k centroid vs F+beta (st) | 0.06 | 0.08 | +-0.5 | pass |
| M5 | noise level vs -0.6 dB, worst (dB) | 0.05 | 0.5 | +-1 | pass |
| M6 | Flatten lock after onset, max (ms) | 54 | 45 | <= 60 | pass |
| M6 | Flatten steady \|median\| max (c) | 0.16 | 0.23 | <= 1 | pass |
| M6 | Flatten vibrato p90 max (c) | 8.49 | 6.49 | <= 10 | pass |
| M7 | gate: lowest shifted level (dBFS) | -50 | -51 | -51 +-3 | pass |
| M7 | lowest SNR with no gross errors (dB) | 10 | 10 | <= 10 | pass |
| M8 | chord A2 P+12 shifted share | 0.186 | 0.234 | report | report |
| M8 | chord A2 P+5 shifted share | 0.137 | 0.157 | report | report |
| M8 | chord A2 P-7 shifted share | 0.581 | 0.59 | report | report |
| M8 | chord C3 P+12 shifted share | 0.209 | 0.194 | report | report |
| M8 | chord C3 P+5 shifted share | 0.18 | 0.212 | report | report |
| M8 | chord C3 P-7 shifted share | 0.521 | 0.5 | report | report |
| M8 | chord sines P+12 shifted share | 0.374 | 0.403 | report | report |
| M8 | chord sines P+5 shifted share | 0.966 | 0.966 | report | report |
| M8 | chord sines P-7 shifted share | 0.358 | 0.576 | report | report |
| M8 | noise at P +-1 level (dB) | 1.8 | -1.09, -1.09, -0.24, -0.21, -0.23, -0.26 | not copied (decided) | report |
| M8 | noise re-granulated: max \|corr\| | 0.02 | 0.069 | report | report |
| M8 | octave down 400->200 settle (ms) | never | 19.6, 14.6 | follows (decided) | report |
| blk | block 64 vs 1/512/4096, worst null | None | -279 | < -90 dB | pass |

20 of 22 judged measures within their thresholds.

Reproduce: `python -I plugins/FwPitchBend/nulltest/check_shifter.py all --save`

### The factory presets

The reference's 40 presets loaded into it against Flatwater Pitch Bend's (Flatwater names): level, the pitch track
where both outputs are clearly voiced, and the long-term spectrum, on a sung and a spoken take. Median pitch
difference 2.3 cents on the sung take and 6.5 on the spoken one (`measurements/FwPitchBend/presets_null.json`). Every pair, and why the misses miss:
[Presets](#presets).

### Sample nulls with a shift

Click probes and pulse trains (MEASUREMENTS.md, section 11, Sample nulls with a shift) show where the reference's grains sit and what they read; ours read the same for Formant up to +1.5. On a steady 150 Hz vowel the residual (dB re the reference output; as rendered, then with ours moved by up to 3 samples, in fractions of one, and matched in level). None of these is a null. Produced by `plugins/FwPitchBend/nulltest/null_shift.py` (`measurements/FwPitchBend/shift_null.json`).

| setting | raw (dB) | lined up (dB) |
|---|---:|---:|
| Formant +0.01 | -26.7 | -47.1 |
| Formant +0.5 | -14.3 | -32.1 |
| Formant +1 | -12.7 | -25.9 |
| Formant -1 | -14.4 | -26.9 |
| Formant -3 | -7.1 | -16.1 |
| Formant -6 | -10.4 | -16.5 |
| Pitch +5 | 4.0 | -0.9 |

Reproduce: `python -I plugins/FwPitchBend/nulltest/null_shift.py --save`

On a sung take, even at Formant +0.01, the reference's output against its own input, delayed, is +0.3 dB: it rebuilds a moving voice grain by grain from its own pitch-period estimates, so no recreation that does not share its detector's exact estimates nulls it; ours reaches about -7 dB per 100 ms window at best alignment (measured before the 1.0.0 release and not reproduced by a script in this tree).

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
frame, which is decided where they are close in level; MEASUREMENTS.md (Phase 3) checks those presets' modulation
directly.

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

Reproduce: `python -I plugins/FwPitchBend/nulltest/null_presets.py --jobs 1`
